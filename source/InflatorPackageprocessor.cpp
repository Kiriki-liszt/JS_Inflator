//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "InflatorPackageprocessor.h"
#include "InflatorPackagecids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"

using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// InflatorPackageProcessor
	//------------------------------------------------------------------------
	InflatorPackageProcessor::InflatorPackageProcessor():
		fInput(0.5), fOutput(1.0), fEffect(1.0), fCurve(0.5),
		fInVuPPML(0.0), fInVuPPMR(0.0), fOutVuPPML(0.0), fOutVuPPMR(0.0),
		fInVuPPMLOld(0.0), fInVuPPMROld(0.0), fOutVuPPMLOld(0.0), fOutVuPPMROld(0.0),
		fParamOS(1)
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

		in_0_64.numChannels = 2;
		in_1_64.numChannels = 2;
		in_2_64.numChannels = 2;
		in_3_64.numChannels = 2;
		out_0_64.numChannels = 2;
		out_1_64.numChannels = 2;
		out_2_64.numChannels = 2;
		out_3_64.numChannels = 2;

		in_0_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_1_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_2_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_3_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_0_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_1_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_2_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_3_64.channelBuffers64 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		if ((in_0_64.channelBuffers64 == NULL) || 
			(in_1_64.channelBuffers64 == NULL) || 
			(in_2_64.channelBuffers64 == NULL) || 
			(in_3_64.channelBuffers64 == NULL)  ) return kResultFalse;
		if ((out_0_64.channelBuffers64 == NULL) || 
			(out_1_64.channelBuffers64 == NULL) || 
			(out_2_64.channelBuffers64 == NULL) || 
			(out_3_64.channelBuffers64 == NULL)  ) return kResultFalse;

		in_0_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		in_0_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		in_1_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		in_1_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		in_2_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
		in_2_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
		in_3_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
		in_3_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
		out_0_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		out_0_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		out_1_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		out_1_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
		out_2_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
		out_2_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
		out_3_64.channelBuffers64[0] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
		out_3_64.channelBuffers64[1] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
		if ((in_0_64.channelBuffers64[0] == NULL) || 
			(in_1_64.channelBuffers64[0] == NULL) || 
			(in_2_64.channelBuffers64[0] == NULL) || 
			(in_3_64.channelBuffers64[0] == NULL)) return kResultFalse;
		if ((in_0_64.channelBuffers64[1] == NULL) || 
			(in_1_64.channelBuffers64[1] == NULL) || 
			(in_2_64.channelBuffers64[1] == NULL) || 
			(in_3_64.channelBuffers64[1] == NULL)) return kResultFalse;
		if ((out_0_64.channelBuffers64[0] == NULL) || 
			(out_1_64.channelBuffers64[0] == NULL) || 
			(out_2_64.channelBuffers64[0] == NULL) || 
			(out_3_64.channelBuffers64[0] == NULL)) return kResultFalse;
		if ((out_0_64.channelBuffers64[1] == NULL) || 
			(out_1_64.channelBuffers64[1] == NULL) || 
			(out_2_64.channelBuffers64[1] == NULL) || 
			(out_3_64.channelBuffers64[1] == NULL)) return kResultFalse;

		upSample_2x_1_L_64.set_coefs(hiir::coef_2x_1);
		upSample_2x_1_R_64.set_coefs(hiir::coef_2x_1);
		upSample_4x_1_L_64.set_coefs(hiir::coef_4x_1);
		upSample_4x_1_R_64.set_coefs(hiir::coef_4x_1);
		upSample_4x_2_L_64.set_coefs(hiir::coef_4x_2);
		upSample_4x_2_R_64.set_coefs(hiir::coef_4x_2);
		upSample_8x_1_L_64.set_coefs(hiir::coef_8x_1);
		upSample_8x_1_R_64.set_coefs(hiir::coef_8x_1);
		upSample_8x_2_L_64.set_coefs(hiir::coef_8x_2);
		upSample_8x_2_R_64.set_coefs(hiir::coef_8x_2);
		upSample_8x_3_L_64.set_coefs(hiir::coef_8x_3);
		upSample_8x_3_R_64.set_coefs(hiir::coef_8x_3);
		downSample_2x_1_L_64.set_coefs(hiir::coef_2x_1);
		downSample_2x_1_R_64.set_coefs(hiir::coef_2x_1);
		downSample_4x_2_L_64.set_coefs(hiir::coef_4x_1);
		downSample_4x_2_R_64.set_coefs(hiir::coef_4x_1);
		downSample_4x_2_L_64.set_coefs(hiir::coef_4x_2);
		downSample_4x_2_R_64.set_coefs(hiir::coef_4x_2);
		downSample_8x_1_L_64.set_coefs(hiir::coef_8x_1);
		downSample_8x_1_R_64.set_coefs(hiir::coef_8x_1);
		downSample_8x_2_L_64.set_coefs(hiir::coef_8x_2);
		downSample_8x_2_R_64.set_coefs(hiir::coef_8x_2);
		downSample_8x_3_L_64.set_coefs(hiir::coef_8x_3);
		downSample_8x_3_R_64.set_coefs(hiir::coef_8x_3);

		return kResultOk;
	}


	tresult PLUGIN_API InflatorPackageProcessor::setBusArrangements(
		Vst::SpeakerArrangement* inputs,  int32 numIns,
		Vst::SpeakerArrangement* outputs, int32 numOuts)
	{
		// we only support one in and output bus and these busses must have the same number of channels
		if (numIns == 1 && 
			numOuts == 1 && 
			Vst::SpeakerArr::getChannelCount(inputs[0]) == 2 && 
			Vst::SpeakerArr::getChannelCount(outputs[0]) == 2
		)	return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);

		return kResultFalse;
	}


	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
		free( in_0_64.channelBuffers64[0]); free( in_0_64.channelBuffers64[1]);
		free( in_1_64.channelBuffers64[0]); free( in_1_64.channelBuffers64[1]);
		free( in_2_64.channelBuffers64[0]); free( in_2_64.channelBuffers64[1]);
		free( in_3_64.channelBuffers64[0]); free( in_3_64.channelBuffers64[1]);
		free(out_0_64.channelBuffers64[0]); free(out_0_64.channelBuffers64[1]);
		free(out_1_64.channelBuffers64[0]); free(out_1_64.channelBuffers64[1]);
		free(out_2_64.channelBuffers64[0]); free(out_2_64.channelBuffers64[1]);
		free(out_3_64.channelBuffers64[0]); free(out_3_64.channelBuffers64[1]);
		free( in_0_64.channelBuffers64);
		free( in_1_64.channelBuffers64);
		free( in_2_64.channelBuffers64);
		free( in_3_64.channelBuffers64);
		free(out_0_64.channelBuffers64);
		free(out_1_64.channelBuffers64);
		free(out_2_64.channelBuffers64);
		free(out_3_64.channelBuffers64);
		//---do not forget to call parent ------
		return AudioEffect::terminate();
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setActive(TBool state)
	{
		/*
		if (state)
			sendTextMessage("InflatorPackageProcessor::setActive (true)");
		else
			sendTextMessage("InflatorPackageProcessor::setActive (false)");
		*/

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
							break;
						case kParamOS:
							int32 fParamOSOld = fParamOS;
							int32 dem = floor(fmin(3.0, 4.0 * value));
							if (dem == 0) fParamOS = 1;
							else if (dem == 1) fParamOS = 2;
							else if (dem == 2) fParamOS = 4;
							else fParamOS = 8;
							if (fParamOSOld != fParamOS) sendTextMessage("OS");
							break;
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
		void** in  = getChannelBuffersPointer(processSetup, data.inputs[0]);
		void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
		Vst::SampleRate getSampleRate = processSetup.sampleRate;


		//---check if silence---------------
		if (data.inputs[0].silenceFlags != 0) // if flags is not zero => then it means that we have silent!
		{
			// As we know that the input is only filled with zero, the output will be then filled with zero too!

			data.outputs[0].silenceFlags = 0;

			if (data.inputs[0].silenceFlags & (uint64)1) { // Left
				if (in[0] != out[0]) memset(out[0], 0, sampleFramesSize);
				data.outputs[0].silenceFlags |= (uint64)1 << (uint64)0;
			}

			if (data.inputs[0].silenceFlags & (uint64)2) { // Right
				if (in[1] != out[1]) memset(out[1], 0, sampleFramesSize);
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
		if (data.symbolicSampleSize == Vst::kSample32) {
			overSampling<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, getSampleRate, data.numSamples);
		}
		else {
			overSampling<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, getSampleRate, data.numSamples);
		}

		//---3) Write outputs parameter changes-----------
		Vst::IParameterChanges* outParamChanges = data.outputParameterChanges;
		// a new value of VuMeter will be send to the host
		// (the host will send it back in sync to our controller for updating our editor)
		if (outParamChanges)
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


	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
		if (newSetup.maxSamplesPerBlock > 16384) {
			return kResultFalse;
		}

		upSample_2x_1_L_64.clear_buffers();
		upSample_2x_1_R_64.clear_buffers();
		upSample_4x_1_L_64.clear_buffers();
		upSample_4x_1_R_64.clear_buffers();
		upSample_4x_2_L_64.clear_buffers();
		upSample_4x_2_R_64.clear_buffers();
		upSample_8x_1_L_64.clear_buffers();
		upSample_8x_1_R_64.clear_buffers();
		upSample_8x_2_L_64.clear_buffers();
		upSample_8x_2_R_64.clear_buffers();
		upSample_8x_3_L_64.clear_buffers();
		upSample_8x_3_R_64.clear_buffers();
		downSample_2x_1_L_64.clear_buffers();
		downSample_2x_1_R_64.clear_buffers();
		downSample_4x_1_L_64.clear_buffers();
		downSample_4x_1_R_64.clear_buffers();
		downSample_4x_2_L_64.clear_buffers();
		downSample_4x_2_R_64.clear_buffers();
		downSample_8x_1_L_64.clear_buffers();
		downSample_8x_1_R_64.clear_buffers();
		downSample_8x_2_L_64.clear_buffers();
		downSample_8x_2_R_64.clear_buffers();
		downSample_8x_3_L_64.clear_buffers();
		downSample_8x_3_R_64.clear_buffers();

		//--- called before any processing ----
		return AudioEffect::setupProcessing(newSetup);
	}

	uint32 PLUGIN_API InflatorPackageProcessor::getLatencySamples() 
	{
		if (fParamOS == 1) return 0;
		else if (fParamOS == 2) return hiir::os_2x_latency;
		else if (fParamOS == 4) return hiir::os_4x_latency;
		else if (fParamOS == 8) return hiir::os_8x_latency;
		else return 0;
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

		int32 savedOS = 0;
		if (streamer.readInt32(savedOS) == false)
			return kResultFalse;

		fInput = savedInput;
		fEffect = savedEffect;
		fCurve = savedCurve;
		bClip = savedClip > 0;
		fOutput = savedOutput;
		bBypass = savedBypass > 0;
		fParamOS = savedOS;

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
		streamer.writeInt32(fParamOS);

		return kResultOk;
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
	template <typename SampleType>
	void InflatorPackageProcessor::processVuPPM_In(SampleType** inputs, int32 sampleFrames)
	{
		SampleType* In_L = (SampleType*)inputs[0];
		SampleType* In_R = (SampleType*)inputs[1];

		SampleType tmpL = 0.0; /*/ VuPPM /*/
		SampleType tmpR = 0.0; /*/ VuPPM /*/

		while (--sampleFrames >= 0) {
			SampleType inputSampleL = *In_L;
			SampleType inputSampleR = *In_R;

			if (inputSampleL > tmpL) { tmpL = inputSampleL; }
			if (inputSampleR > tmpR) { tmpR = inputSampleR; }

			In_L++;
			In_R++;
		}

		fInVuPPML = VuPPMconvert(tmpL);
		fInVuPPMR = VuPPMconvert(tmpR);
		return;
	}

	template <typename SampleType>
	void InflatorPackageProcessor::processVuPPM_Out(SampleType** outputs, int32 sampleFrames)
	{
		SampleType* Out_L = (SampleType*)outputs[0];
		SampleType* Out_R = (SampleType*)outputs[1];
		
		SampleType tmpL = 0.0; /*/ VuPPM /*/
		SampleType tmpR = 0.0; /*/ VuPPM /*/

		while (--sampleFrames >= 0) {
			SampleType inputSampleL = *Out_L;
			SampleType inputSampleR = *Out_R;

			if (inputSampleL > tmpL) { tmpL = inputSampleL; }
			if (inputSampleR > tmpR) { tmpR = inputSampleR; }

			Out_L++;
			Out_R++;
		}
		fOutVuPPML = VuPPMconvert(tmpL);
		fOutVuPPMR = VuPPMconvert(tmpR);
		return;
	}


	template <typename SampleType>
	void InflatorPackageProcessor::proc_in(SampleType** inputs, Vst::Sample64** outputs, int32 sampleFrames) 
	{
		SampleType* In_L = (SampleType*)inputs[0];
		SampleType* In_R = (SampleType*)inputs[1];
		Vst::Sample64* Out_L = outputs[0];
		Vst::Sample64* Out_R = outputs[1];

		SampleType In_db = expf(logf(10.f) * (24.0 * fInput - 12.0) / 20.f);

		while (--sampleFrames >= 0) {
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;

			if (fabs(inputSampleL) < 1.18e-23) inputSampleL = fpdL * 1.18e-17;
			if (fabs(inputSampleR) < 1.18e-23) inputSampleR = fpdR * 1.18e-17;

			if (!bBypass) {
				inputSampleL *= In_db;
				inputSampleR *= In_db;
			}

			*Out_L = inputSampleL;
			*Out_R = inputSampleR;

			In_L++;
			In_R++;
			Out_L++;
			Out_R++;
		}
		return;
	}
	template <typename SampleType>
	void InflatorPackageProcessor::proc_out(Vst::Sample64** inputs, SampleType** outputs, int32 sampleFrames) 
	{
		Vst::Sample64* In_L = inputs[0];
		Vst::Sample64* In_R = inputs[1];
		SampleType* Out_L = (SampleType*) outputs[0];
		SampleType* Out_R = (SampleType*) outputs[1];

		SampleType Out_db = expf(logf(10.f) * (12.0 * fOutput - 12.0) / 20.f);

		bool is_32fp = std::is_same <SampleType, Vst::Sample32>::value ? true : false;

		while (--sampleFrames >= 0) {
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;

			if (!bBypass) {
				inputSampleL *= Out_db;
				inputSampleR *= Out_db;
			}

			if (is_32fp) {
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

			*Out_L = (SampleType) inputSampleL;
			*Out_R = (SampleType) inputSampleR;

			In_L++;
			In_R++;
			Out_L++;
			Out_R++;
		}
		return;
	}

	template <typename SampleType>
	void InflatorPackageProcessor::processAudio(
		SampleType** inputs, 
		SampleType** outputs, 
		Vst::SampleRate getSampleRate, 
		long long sampleFrames
	)
	{
		if (bBypass) {
			memcpy(outputs[0], inputs[0], sizeof(SampleType) * sampleFrames);
			memcpy(outputs[1], inputs[1], sizeof(SampleType) * sampleFrames);
			return;
		}

		SampleType* In_L = (SampleType*)inputs[0];
		SampleType* In_R = (SampleType*)inputs[1];
		SampleType* Out_L = (SampleType*)outputs[0];
		SampleType* Out_R = (SampleType*)outputs[1];

		Vst::Sample64 wet = fEffect;

		Vst::Sample64 curvepct = fCurve - 0.5;
		Vst::Sample64 curveA = 1.5 + curvepct;			// 1 + (curve + 50) / 100
		Vst::Sample64 curveB = -(curvepct + curvepct);	// - curve / 50
		Vst::Sample64 curveC = curvepct - 0.5;			// (curve - 50) / 100
		Vst::Sample64 curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	
		// 1 / 16 - curve / 400 + curve ^ 2 / (4 * 10 ^ 4)

		Vst::Sample64 s1_L, s2_L, s3_L, s4_L;
		Vst::Sample64 s1_R, s2_R, s3_R, s4_R;

		Vst::Sample64 signL;
		Vst::Sample64 signR;

		while (--sampleFrames >= 0)
		{
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;

			Vst::Sample64 drySampleL = inputSampleL;
			Vst::Sample64 drySampleR = inputSampleR;

			if (bClip) {
				if      (inputSampleL >  1.0) inputSampleL =  1.0;
				else if (inputSampleL < -1.0) inputSampleL = -1.0;

				if      (inputSampleR >  1.0) inputSampleR =  1.0;
				else if (inputSampleR < -1.0) inputSampleR = -1.0;
			}

			if (inputSampleL > 0.0) signL =  1.0;
			else                    signL = -1.0;

			if (inputSampleR > 0.0) signR =  1.0;
			else                    signR = -1.0;

			s1_L = fabs(inputSampleL);
			s2_L = s1_L * s1_L;
			s3_L = s2_L * s1_L;
			s4_L = s2_L * s2_L;

			s1_R = fabs(inputSampleR);
			s2_R = s1_R * s1_R;
			s3_R = s2_R * s1_R;
			s4_R = s2_R * s2_R;

			if      (s1_L >= 2.0) inputSampleL = 0.0;
			else if (s1_L >  1.0) inputSampleL = (2.0 * s1_L) - s2_L;
			else                  inputSampleL = (curveA * s1_L) + 
			                                     (curveB * s2_L) + 
			                                     (curveC * s3_L) - 
			                                     (curveD * (s2_L - (2.0 * s3_L) + s4_L));

			if      (s1_R >= 2.0) inputSampleR = 0.0;
			else if (s1_R >  1.0) inputSampleR = (2.0 * s1_R) - s2_R;
			else                  inputSampleR = (curveA * s1_R) + 
			                                     (curveB * s2_R) + 
			                                     (curveC * s3_R) - 
			                                     (curveD * (s2_R - (2.0 * s3_R) + s4_R));

			inputSampleL *= signL;
			inputSampleR *= signR;

			if (wet != 1.0) {
				inputSampleL = (drySampleL * (1.0 - wet)) + (inputSampleL * wet);
				inputSampleR = (drySampleR * (1.0 - wet)) + (inputSampleR * wet);
			}

			if (bClip) {
				if      (inputSampleL >  1.0) inputSampleL =  1.0;
				else if (inputSampleL < -1.0) inputSampleL = -1.0;

				if      (inputSampleR >  1.0) inputSampleR =  1.0;
				else if (inputSampleR < -1.0) inputSampleR = -1.0;
			}

			*Out_L = (SampleType)inputSampleL;
			*Out_R = (SampleType)inputSampleR;

			In_L++;
			In_R++;
			Out_L++;
			Out_R++;
		}
		return;
	}

	template <typename SampleType>
	void InflatorPackageProcessor::overSampling(
		SampleType** inputs, 
		SampleType** outputs, 
		Vst::SampleRate getSampleRate,
		int32 sampleFrames
	)
	{
		const long long    len_1 = sampleFrames;
		const long long    len_2 = len_1 * 2;
		const long long    len_4 = len_2 * 2;
		const long long    len_8 = len_4 * 2;

		
		proc_in<SampleType>((SampleType**)inputs, in_0_64.channelBuffers64, sampleFrames);

		processVuPPM_In<Vst::Sample64>(in_0_64.channelBuffers64, sampleFrames);


		if (fParamOS == 1) {
			processAudio<Vst::Sample64>(in_0_64.channelBuffers64, out_0_64.channelBuffers64, getSampleRate, sampleFrames);
		}
		else if (fParamOS == 2) {
			upSample_2x_1_L_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[0], (Vst::Sample64*)in_0_64.channelBuffers64[0], len_1);
			upSample_2x_1_R_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[1], (Vst::Sample64*)in_0_64.channelBuffers64[1], len_1);

			processAudio<Vst::Sample64>((Vst::Sample64**)in_1_64.channelBuffers64, (Vst::Sample64**)out_1_64.channelBuffers64, getSampleRate, len_2);

			downSample_2x_1_L_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[0], (Vst::Sample64*)out_1_64.channelBuffers64[0], len_1);
			downSample_2x_1_R_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[1], (Vst::Sample64*)out_1_64.channelBuffers64[1], len_1);
		}
		else if (fParamOS == 4) {

			upSample_4x_1_L_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[0], (Vst::Sample64*)in_0_64.channelBuffers64[0], len_1);
			upSample_4x_1_R_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[1], (Vst::Sample64*)in_0_64.channelBuffers64[1], len_1);
			upSample_4x_2_L_64.process_block((Vst::Sample64*)in_2_64.channelBuffers64[0], (Vst::Sample64*)in_1_64.channelBuffers64[0], len_2);
			upSample_4x_2_R_64.process_block((Vst::Sample64*)in_2_64.channelBuffers64[1], (Vst::Sample64*)in_1_64.channelBuffers64[1], len_2);

			processAudio<Vst::Sample64>((Vst::Sample64**)in_2_64.channelBuffers64, (Vst::Sample64**)out_2_64.channelBuffers64, getSampleRate, len_4);

			downSample_4x_1_L_64.process_block((Vst::Sample64*)out_1_64.channelBuffers64[0], (Vst::Sample64*)out_2_64.channelBuffers64[0], len_2);
			downSample_4x_1_R_64.process_block((Vst::Sample64*)out_1_64.channelBuffers64[1], (Vst::Sample64*)out_2_64.channelBuffers64[1], len_2);
			downSample_4x_2_L_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[0], (Vst::Sample64*)out_1_64.channelBuffers64[0], len_1);
			downSample_4x_2_R_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[1], (Vst::Sample64*)out_1_64.channelBuffers64[1], len_1);
		}
		else if (fParamOS == 8) {

			upSample_8x_1_L_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[0], (Vst::Sample64*)in_0_64.channelBuffers64[0], len_1);
			upSample_8x_1_R_64.process_block((Vst::Sample64*)in_1_64.channelBuffers64[1], (Vst::Sample64*)in_0_64.channelBuffers64[1], len_1);
			upSample_8x_2_L_64.process_block((Vst::Sample64*)in_2_64.channelBuffers64[0], (Vst::Sample64*)in_1_64.channelBuffers64[0], len_2);
			upSample_8x_2_R_64.process_block((Vst::Sample64*)in_2_64.channelBuffers64[1], (Vst::Sample64*)in_1_64.channelBuffers64[1], len_2);
			upSample_8x_3_L_64.process_block((Vst::Sample64*)in_3_64.channelBuffers64[0], (Vst::Sample64*)in_2_64.channelBuffers64[0], len_4);
			upSample_8x_3_R_64.process_block((Vst::Sample64*)in_3_64.channelBuffers64[1], (Vst::Sample64*)in_2_64.channelBuffers64[1], len_4);

			processAudio<Vst::Sample64>((Vst::Sample64**)in_3_64.channelBuffers64, (Vst::Sample64**)out_3_64.channelBuffers64, getSampleRate, len_8);

			downSample_8x_1_L_64.process_block((Vst::Sample64*)out_2_64.channelBuffers64[0], (Vst::Sample64*)out_3_64.channelBuffers64[0], len_4);
			downSample_8x_1_R_64.process_block((Vst::Sample64*)out_2_64.channelBuffers64[1], (Vst::Sample64*)out_3_64.channelBuffers64[1], len_4);
			downSample_8x_2_L_64.process_block((Vst::Sample64*)out_1_64.channelBuffers64[0], (Vst::Sample64*)out_2_64.channelBuffers64[0], len_2);
			downSample_8x_2_R_64.process_block((Vst::Sample64*)out_1_64.channelBuffers64[1], (Vst::Sample64*)out_2_64.channelBuffers64[1], len_2);
			downSample_8x_3_L_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[0], (Vst::Sample64*)out_1_64.channelBuffers64[0], len_1);
			downSample_8x_3_R_64.process_block((Vst::Sample64*)out_0_64.channelBuffers64[1], (Vst::Sample64*)out_1_64.channelBuffers64[1], len_1);
		}
		else {
			processAudio<Vst::Sample64>(in_0_64.channelBuffers64, out_0_64.channelBuffers64, getSampleRate, sampleFrames);
		}


		proc_out<SampleType>(out_0_64.channelBuffers64, (SampleType**)outputs, sampleFrames);

		processVuPPM_Out<SampleType>((SampleType**)outputs, sampleFrames);

		return;
	}
} // namespace yg331
