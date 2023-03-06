//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "InflatorPackageprocessor.h"
#include "InflatorPackagecids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"


#include "param.h"

using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// InflatorPackageProcessor
	//------------------------------------------------------------------------
	InflatorPackageProcessor::InflatorPackageProcessor():
		fInput(0.5), fOutput(1.0), fEffect(1.0), fCurve(0.5),
		fInVuPPML(0.0), fInVuPPMR(0.0), fOutVuPPML(0.0), fOutVuPPMR(0.0),
		fInVuPPMLOld(0.0), fInVuPPMROld(0.0), fOutVuPPMLOld(0.0), fOutVuPPMROld(0.0)
	{
		//--- set the wanted controller for our processor
		setControllerClass(kInflatorPackageControllerUID);
	}

	//------------------------------------------------------------------------
	InflatorPackageProcessor::~InflatorPackageProcessor()
	{}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::initialize(FUnknown* context)
	{
		// Here the Plug-in will be instantiated

		//---always initialize the parent-------
		tresult result = AudioEffect::initialize(context);
		// if everything Ok, continue
		if (result != kResultOk)
		{
			return result;
		}

		//--- create Audio IO ------
		addAudioInput (STR16("Stereo In"),  Steinberg::Vst::SpeakerArr::kStereo);
		addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

		/* If you don't need an event bus, you can remove the next line */
		// addEventInput(STR16("Event In"), 1);

		fpdL = 1.0; while (fpdL < 16386) fpdL = rand() * UINT32_MAX;
		fpdR = 1.0; while (fpdR < 16386) fpdR = rand() * UINT32_MAX;

		return kResultOk;
	}


	tresult PLUGIN_API InflatorPackageProcessor::setBusArrangements(
		Vst::SpeakerArrangement* inputs, int32 numIns,
		Vst::SpeakerArrangement* outputs, int32 numOuts)
	{
		// we only support one in and output bus and these busses must have the same number of channels
		if (numIns == 1 && numOuts == 1 && Vst::SpeakerArr::getChannelCount(inputs[0]) == 2 && Vst::SpeakerArr::getChannelCount(outputs[0]) == 2)
			return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
		return kResultFalse;
	}


	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

		//---do not forget to call parent ------
		return AudioEffect::terminate();
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setActive(TBool state)
	{
		if (state)
		{
			sendTextMessage("InflatorPackageProcessor::setActive (true)");
		}
		else
		{
			sendTextMessage("InflatorPackageProcessor::setActive (false)");
		}

		// reset the VuMeter value
		fInVuPPMLOld = 0.f;
		fInVuPPMROld = 0.f;
		fOutVuPPMLOld = 0.f;
		fOutVuPPMROld = 0.f;

		//--- called when the Plug-in is enable/disable (On/Off) -----
		return AudioEffect::setActive(state);
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::process(Vst::ProcessData& data)
	{
		//---1) Read inputs parameter changes-----------

		Vst::IParameterChanges* paramChanges = data.inputParameterChanges;

		if (paramChanges)
		{
			int32 numParamsChanged = paramChanges->getParameterCount();

			for (int32 index = 0; index < numParamsChanged; index++)
			{
				Vst::IParamValueQueue* paramQueue = paramChanges->getParameterData(index);

				if (paramQueue)
				{
					Vst::ParamValue value;
					int32 sampleOffset;
					int32 numPoints = paramQueue->getPointCount();

					/*/*/
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
						switch (paramQueue->getParameterId()) {
						case kParamInput:
							fInput = (float)value;
							break;
						case kParamEffect:
							fEffect = (float)value;
							break;
						case kParamCurve:
							fCurve = (float)value;
							break;
						case kParamClip:
							bClip = (value > 0.5f);
							break;
						case kParamOutput:
							fOutput = (float)value;
							break;
						case kParamBypass:
							bBypass = (value > 0.5f);
						}
					}
				}
			}
		}

		if (data.numInputs == 0 || data.numOutputs == 0) {
			return kResultOk;
		}

		//---get audio buffers----------------
		uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
		void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
		void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
		double getSampleRate = processSetup.sampleRate;


		//---check if silence---------------
		// normally we have to check each channel (simplification)
		if (data.inputs[0].silenceFlags != 0) // if flags is not zero => then it means that we have silent!
		{
			// in the example we are applying a gain to the input signal
			// As we know that the input is only filled with zero, the output will be then filled with zero too!

			data.outputs[0].silenceFlags = 0;

			if (data.inputs[0].silenceFlags & (uint64)1) { // Left
				if (in[0] != out[0])
				{
					memset(out[0], 0, sampleFramesSize); // this is faster than applying a gain to each sample!
				}
				data.outputs[0].silenceFlags |= (uint64)1 << (uint64)0;
			}

			if (data.inputs[0].silenceFlags & (uint64)2) { // Right
				if (in[1] != out[1])
				{
					memset(out[1], 0, sampleFramesSize); // this is faster than applying a gain to each sample!
				}
				data.outputs[0].silenceFlags |= (uint64)1 << (uint64)1;
			}

			if (data.inputs[0].silenceFlags & (uint64)3) {
				return kResultOk;
			}
		}

		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		fInVuPPML = 0.f;
		fInVuPPMR = 0.f;
		fOutVuPPML = 0.f;
		fOutVuPPMR = 0.f;


		//---in bypass mode outputs should be like inputs-----
		if (bBypass)
		{
			if (in[0] != out[0]) { memcpy(out[0], in[0], sampleFramesSize); }
			if (in[1] != out[1]) { memcpy(out[1], in[1], sampleFramesSize); }

			if (data.symbolicSampleSize == Vst::kSample32) {
				processVuPPM<Vst::Sample32>((Vst::Sample32**)in, data.numSamples);
			}
			else if (data.symbolicSampleSize == Vst::kSample64) {
				processVuPPM<Vst::Sample64>((Vst::Sample64**)in, data.numSamples);
			}
		}
		else
		{
			if (data.symbolicSampleSize == Vst::kSample32) {
				processAudio<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, getSampleRate, data.numSamples, Vst::kSample32);
			}
			else {
				processAudio<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, getSampleRate, data.numSamples, Vst::kSample64);
			}
		}

		//---3) Write outputs parameter changes-----------
		Vst::IParameterChanges* outParamChanges = data.outputParameterChanges;
		// a new value of VuMeter will be send to the host
		// (the host will send it back in sync to our controller for updating our editor)
		if (outParamChanges && (fInVuPPMLOld != fInVuPPML || fInVuPPMROld != fInVuPPMR || fOutVuPPMLOld != fOutVuPPML || fOutVuPPMLOld != fOutVuPPMR) )
		{
			int32 index = 0;
			Vst::IParamValueQueue* paramQueue = outParamChanges->addParameterData(kParamInVuPPML, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fInVuPPML, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamInVuPPMR, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fInVuPPMR, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamOutVuPPML, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fOutVuPPML, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamOutVuPPMR, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fOutVuPPMR, index2);
			}

		}
		fInVuPPMLOld = fInVuPPML;
		fInVuPPMROld = fInVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;

		return kResultOk;
	}


	template <typename SampleType>
	void InflatorPackageProcessor::processAudio(SampleType** inputs, SampleType** outputs, Vst::Sample64 getSampleRate, int32 sampleFrames, int32 precision)
	{
		SampleType* In_L = (SampleType*)inputs[0];
		SampleType* In_R = (SampleType*)inputs[1];
		SampleType* Out_L = (SampleType*)outputs[0];
		SampleType* Out_R = (SampleType*)outputs[1];
		SampleType tmpInL = 0.0; /*/ VuPPM /*/
		SampleType tmpInR = 0.0; /*/ VuPPM /*/
		SampleType tmpOutL = 0.0; /*/ VuPPM /*/
		SampleType tmpOutR = 0.0; /*/ VuPPM /*/

		Vst::Sample64 wet = fEffect;
		Vst::Sample64 In_db = expf(logf(10.f) * (24.0 * fInput - 12.0) / 20.f);
		Vst::Sample64 Out_db = expf(logf(10.f) * (12.0 * fOutput - 12.0) / 20.f);

		Vst::Sample64 curvepct = fCurve - 0.5;
		Vst::Sample64 curveA = 1.5 + curvepct;			// 1 + (curve + 50) / 100
		Vst::Sample64 curveB = -(curvepct + curvepct);	// - curve / 50
		Vst::Sample64 curveC = curvepct - 0.5;			// (curve - 50) / 100
		Vst::Sample64 curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	// 1 / 16 - curve / 400 + curve ^ 2 / (4 * 10 ^ 4)

		Vst::Sample64 s1_L;
		Vst::Sample64 s1_R;
		Vst::Sample64 s2_L;
		Vst::Sample64 s2_R;
		Vst::Sample64 s3_L;
		Vst::Sample64 s3_R;
		Vst::Sample64 s4_L;
		Vst::Sample64 s4_R;

		Vst::Sample64 signL;
		Vst::Sample64 signR;

		while (--sampleFrames >= 0)
		{
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;

			/*/ VuPPM /*/
			inputSampleL *= In_db;
			inputSampleR *= In_db;
			if (inputSampleL > tmpInL) { tmpInL = inputSampleL; }
			if (inputSampleR > tmpInR) { tmpInR = inputSampleR; }


			if (fabs(inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
			if (fabs(inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;
			Vst::Sample64 drySampleL = inputSampleL;
			Vst::Sample64 drySampleR = inputSampleR;

			if (bClip) {
				if (inputSampleL > 1.0)
					inputSampleL = 1.0;
				else if (inputSampleL < -1.0)
					inputSampleL = -1.0;

				if (inputSampleR > 1.0)
					inputSampleR = 1.0;
				else if (inputSampleR < -1.0)
					inputSampleR = -1.0;
			}

			if (inputSampleL > 0.0)
				signL = 1.0;
			else
				signL = -1.0;

			if (inputSampleR > 0.0)
				signR = 1.0;
			else
				signR = -1.0;

			s1_L = fabs(inputSampleL);
			s2_L = s1_L * s1_L;
			s3_L = s2_L * s1_L;
			s4_L = s2_L * s2_L;

			s1_R = fabs(inputSampleR);
			s2_R = s1_R * s1_R;
			s3_R = s2_R * s1_R;
			s4_R = s2_R * s2_R;

			if (s1_L >= 2.0)
				inputSampleL = 0.0;
			else if (s1_L > 1.0)
				inputSampleL = (2.0 * s1_L) - s2_L;
			else
				inputSampleL = (curveA * s1_L) + (curveB * s2_L) + (curveC * s3_L) - (curveD * (s2_L - (2.0 * s3_L) + s4_L));

			if (s1_R >= 2.0)
				inputSampleR = 0.0;
			else if (s1_R > 1.0)
				inputSampleR = (2.0 * s1_R) - s2_R;
			else
				inputSampleR = (curveA * s1_R) + (curveB * s2_R) + (curveC * s3_R) - (curveD * (s2_R - (2.0 * s3_R) + s4_R));

			inputSampleL *= signL;
			inputSampleR *= signR;

			if (wet != 1.0) {
				inputSampleL = (drySampleL * (1.0 - wet)) + (inputSampleL * wet);
				inputSampleR = (drySampleR * (1.0 - wet)) + (inputSampleR * wet);
			}


			if (bClip) {
				if (inputSampleL > 1.0)
					inputSampleL = 1.0;
				else if (inputSampleL < -1.0)
					inputSampleL = -1.0;

				if (inputSampleR > 1.0)
					inputSampleR = 1.0;
				else if (inputSampleR < -1.0)
					inputSampleR = -1.0;
			}

			inputSampleL *= Out_db;
			inputSampleR *= Out_db;
			if (inputSampleL > tmpOutL) { tmpOutL = inputSampleL; }
			if (inputSampleR > tmpOutR) { tmpOutR = inputSampleR; }


			if (precision == 0) {
				//begin 32 bit stereo floating point dither
				int expon; frexpf((float)inputSampleL, &expon);
				fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
				inputSampleL += ((double(fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
				frexpf((float)inputSampleR, &expon);
				fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
				inputSampleR += ((double(fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
				//end 32 bit stereo floating point dither
			}
			else {
				//begin 64 bit stereo floating point dither
				//int expon; frexp((double)inputSampleL, &expon);
				fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
				//inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
				//frexp((double)inputSampleR, &expon);
				fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
				//inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
				//end 64 bit stereo floating point dither
			}
			*Out_L = inputSampleL;
			*Out_R = inputSampleR;

			In_L++;
			In_R++;
			Out_L++;
			Out_R++;
		}
		fInVuPPML = VuPPMconvert(tmpInL);
		fInVuPPMR = VuPPMconvert(tmpInR);
		fOutVuPPML = VuPPMconvert(tmpOutL);
		fOutVuPPMR = VuPPMconvert(tmpOutR);
		return;
	}


	template <typename SampleType>
	void InflatorPackageProcessor::processVuPPM(SampleType** inputs, int32 sampleFrames)
	{
		SampleType* In_L = (SampleType*)inputs[0];
		SampleType* In_R = (SampleType*)inputs[1];

		SampleType tmpInL = 0.0; /*/ VuPPM /*/
		SampleType tmpInR = 0.0; /*/ VuPPM /*/

		while (--sampleFrames >= 0)
		{
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;
			if (inputSampleL > tmpInL) { tmpInL = inputSampleL; }
			if (inputSampleR > tmpInR) { tmpInR = inputSampleR; }
			In_L++;
			In_R++;
		}

		fInVuPPML = VuPPMconvert(tmpInL);
		fInVuPPMR = VuPPMconvert(tmpInR);
		fOutVuPPML = fInVuPPML;
		fOutVuPPMR = fInVuPPMR;

		return;
	}

	float InflatorPackageProcessor::VuPPMconvert(float plainValue)
	{
		float dB = 20 * log10f(plainValue);
		float normValue;

		if (dB >= 6.0) normValue = 1.0;
		else if (dB >= 5.0) normValue = 0.96;
		else if (dB >= 4.0) normValue = 0.92;
		else if (dB >= 3.0) normValue = 0.88;
		else if (dB >= 2.0) normValue = 0.84;
		else if (dB >= 1.0) normValue = 0.80;
		else if (dB >= 0.0) normValue = 0.76;
		else if (dB >= -1.0) normValue = 0.72;
		else if (dB >= -2.0) normValue = 0.68;
		else if (dB >= -3.0) normValue = 0.64;
		else if (dB >= -4.0) normValue = 0.60;
		else if (dB >= -5.0) normValue = 0.56;
		else if (dB >= -6.0) normValue = 0.52;
		else if (dB >= -8.0) normValue = 0.48;
		else if (dB >= -10) normValue = 0.44;
		else if (dB >= -12) normValue = 0.40;
		else if (dB >= -14) normValue = 0.36;
		else if (dB >= -16) normValue = 0.32;
		else if (dB >= -18) normValue = 0.28;
		else if (dB >= -22) normValue = 0.24;
		else if (dB >= -26) normValue = 0.20;
		else if (dB >= -30) normValue = 0.16;
		else if (dB >= -34) normValue = 0.12;
		else if (dB >= -38) normValue = 0.08;
		else if (dB >= -42) normValue = 0.04;
		else normValue = 0.0;

		return normValue;
	}



	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
		//--- called before any processing ----
		return AudioEffect::setupProcessing(newSetup);
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::canProcessSampleSize(int32 symbolicSampleSize)
	{
		// by default kSample32 is supported
		if (symbolicSampleSize == Vst::kSample32)
			return kResultTrue;

		// disable the following comment if your processing support kSample64
		if (symbolicSampleSize == Vst::kSample64)
			return kResultTrue; 

		return kResultFalse;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setState(IBStream* state)
	{
		// called when we load a preset, the model has to be reloaded
		IBStreamer streamer(state, kLittleEndian);

		float savedInput = 0.f;
		if (streamer.readFloat(savedInput) == false)
			return kResultFalse;

		float savedEffect = 0.f;
		if (streamer.readFloat(savedEffect) == false)
			return kResultFalse;

		float savedCurve = 0.f;
		if (streamer.readFloat(savedCurve) == false)
			return kResultFalse;

		int32 savedClip = 0;
		if (streamer.readInt32(savedClip) == false)
			return kResultFalse;

		float savedOutput = 0.f;
		if (streamer.readFloat(savedOutput) == false)
			return kResultFalse;

		int32 savedBypass = 0;
		if (streamer.readInt32(savedBypass) == false)
			return kResultFalse;

		fInput = savedInput;
		fEffect = savedEffect;
		fCurve = savedCurve;
		bClip = savedClip > 0;
		fOutput = savedOutput;
		bBypass = savedBypass > 0;

		if (Vst::Helpers::isProjectState(state) == kResultTrue)
		{
			// we are in project loading context...

			// Example of using the IStreamAttributes interface
			FUnknownPtr<Vst::IStreamAttributes> stream(state);
			if (stream)
			{
				if (Vst::IAttributeList* list = stream->getAttributes())
				{
					// get the full file path of this state
					Vst::TChar fullPath[1024];
					memset(fullPath, 0, 1024 * sizeof(Vst::TChar));
					if (list->getString(Vst::PresetAttributes::kFilePathStringType, fullPath,
						1024 * sizeof(Vst::TChar)) == kResultTrue)
					{
						// here we have the full path ...
					}
				}
			}
		}


		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::getState(IBStream* state)
	{
		// here we need to save the model
		IBStreamer streamer(state, kLittleEndian);

		streamer.writeFloat(fInput);
		streamer.writeFloat(fEffect);
		streamer.writeFloat(fCurve);
		streamer.writeInt32(bClip ? 1 : 0);
		streamer.writeFloat(fOutput);
		streamer.writeInt32(bBypass ? 1 : 0);

		return kResultOk;
	}

	//------------------------------------------------------------------------


} // namespace yg331
