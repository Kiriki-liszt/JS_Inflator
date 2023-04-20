//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

// #include "InflatorPackagecids.h"
#include "InflatorPackageprocessor.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"

#include <stdio.h>

namespace r8b {
	CSyncObject CDSPRealFFTKeeper::StateSync;
	CDSPRealFFT::CObjKeeper CDSPRealFFTKeeper::FFTObjects[31];

	CSyncObject CDSPFIRFilterCache::StateSync;
	CPtrKeeper< CDSPFIRFilter* > CDSPFIRFilterCache::Objects;
	int CDSPFIRFilterCache::ObjCount = 0;
} // namespace r8b


using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// InflatorPackageProcessor
	//------------------------------------------------------------------------
	InflatorPackageProcessor::InflatorPackageProcessor() :
		fInput(init_Input),
		fOutput(init_Output),
		fEffect(init_Effect),
		fCurve(init_Curve),
		curvepct(init_curvepct),
		curveA(init_curveA),
		curveB(init_curveB),
		curveC(init_curveC),
		curveD(init_curveD),
		bBypass(init_Bypass),
		bIn(init_In),
		bSplit(init_Split),
		bClip(init_Clip),
		fParamOS(init_OS),
		fParamOSOld(init_OS),
		fInVuPPML(init_VU),
		fInVuPPMR(init_VU),
		fOutVuPPML(init_VU),
		fOutVuPPMR(init_VU),
		fInVuPPMLOld(init_VU),
		fInVuPPMROld(init_VU),
		fOutVuPPMLOld(init_VU),
		fOutVuPPMROld(init_VU),
		fMeter(init_VU),
		fMeterOld(init_VU),
		fParamZoom(init_Zoom),
		fParamPhase(init_Phase),
		fpdL(1), fpdR(1)
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

		in_0  = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_1  = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_2  = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		in_3  = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_0 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_1 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_2 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		out_3 = (Vst::Sample64**)malloc(sizeof(Vst::Sample64*) * 2);
		if ((in_0  == NULL) || (in_1  == NULL) || (in_2  == NULL) || (in_3  == NULL)) return kResultFalse;
		if ((out_0 == NULL) || (out_1 == NULL) || (out_2 == NULL) || (out_3 == NULL)) return kResultFalse;
		
		for (int c = 0; c < 2; c++) {
			in_0[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
			in_1[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
			in_2[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
			in_3[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 8);
			out_0[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample);
			out_1[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 2);
			out_2[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 4);
			out_3[c] = (Vst::Sample64*)malloc(sizeof(Vst::Sample64) * maxSample * 8);
			if ((in_0[c] == NULL) || (in_1[c] == NULL) ||
				(in_2[c] == NULL) || (in_3[c] == NULL)) return kResultFalse;
			if ((out_0[c] == NULL) || (out_1[c] == NULL) ||
				(out_2[c] == NULL) || (out_3[c] == NULL)) return kResultFalse;

			upSample_2x_1[c].set_coefs(hiir::coef_2x_1);
			upSample_4x_1[c].set_coefs(hiir::coef_4x_1);
			upSample_4x_2[c].set_coefs(hiir::coef_4x_2);
			upSample_8x_1[c].set_coefs(hiir::coef_8x_1);
			upSample_8x_2[c].set_coefs(hiir::coef_8x_2);
			upSample_8x_3[c].set_coefs(hiir::coef_8x_3);
			dnSample_2x_1[c].set_coefs(hiir::coef_2x_1);
			dnSample_4x_2[c].set_coefs(hiir::coef_4x_1);
			dnSample_4x_2[c].set_coefs(hiir::coef_4x_2);
			dnSample_8x_1[c].set_coefs(hiir::coef_8x_1);
			dnSample_8x_2[c].set_coefs(hiir::coef_8x_2);
			dnSample_8x_3[c].set_coefs(hiir::coef_8x_3);

			if (OS_Lin[c + 0]) { OS_Lin[c + 0].reset(); }
			if (OS_Lin[c + 2]) { OS_Lin[c + 2].reset(); }
					
			/*
			upSample_2x_Lin[c] = new r8b::CDSPResampler(1.0, 2.0, maxSample * 32);
			upSample_4x_Lin[c] = new r8b::CDSPResampler(1.0, 4.0, maxSample * 32, 2.1);
			upSample_8x_Lin[c] = new r8b::CDSPResampler(1.0, 8.0, maxSample * 32, 2.1);
			dnSample_2x_Lin[c] = new r8b::CDSPResampler(2.0, 1.0, maxSample * 32);
			dnSample_4x_Lin[c] = new r8b::CDSPResampler(4.0, 1.0, maxSample * 32, 2.1);
			dnSample_8x_Lin[c] = new r8b::CDSPResampler(8.0, 1.0, maxSample * 32, 2.1);
			*/
		}
		
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

		for (int c = 0; c < 2; c++) {
			free(in_0[c]);
			free(in_1[c]);
			free(in_2[c]);
			free(in_3[c]);
			free(out_0[c]);
			free(out_1[c]);
			free(out_2[c]);
			free(out_3[c]);
			if (OS_Lin[c + 0]) { OS_Lin[c + 0].reset(); }
			if (OS_Lin[c + 2]) { OS_Lin[c + 2].reset(); }
		}
		free( in_0);
		free( in_1);
		free( in_2);
		free( in_3);
		free(out_0);
		free(out_1);
		free(out_2);
		free(out_3);

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
		fInVuPPMLOld = init_VU;
		fInVuPPMROld = init_VU;
		fOutVuPPMLOld = init_VU;
		fOutVuPPMROld = init_VU;

		fMeter = init_VU;
		fMeterOld = init_VU;

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
							fInput = value;
							break;
						case kParamEffect:
							fEffect = value;
							break;
						case kParamCurve:
							fCurve = value;
							break;
						case kParamClip:
							bClip = (value > 0.5f);
							break;
						case kParamOutput:
							fOutput = value;
							break;
						case kParamBypass:
							bBypass = (value > 0.5f);
							break;
						case kParamIn:
							bIn = (value > 0.5f);
							break;
						case kParamOS:
							fParamOSOld = fParamOS;
							fParamOS = convert_to_OS(value);
							if (fParamOS != overSample_1x) {
								for (int c = 0; c < 2; c++) {
									if (OS_Lin[c + 0]) { OS_Lin[c + 0].reset(); }
									if (OS_Lin[c + 2]) { OS_Lin[c + 2].reset(); }

									if (fParamOS == overSample_2x) {
										OS_Lin[c + 0] = new r8b::CDSPResampler(1.0, 2.0, maxSample * 2);
										OS_Lin[c + 2] = new r8b::CDSPResampler(2.0, 1.0, maxSample * 32);
									}
									else if (fParamOS == overSample_4x) {
										OS_Lin[c + 0] = new r8b::CDSPResampler(1.0, 4.0, maxSample * 4, 2.1);
										OS_Lin[c + 2] = new r8b::CDSPResampler(4.0, 1.0, maxSample * 32, 2.1);
									}
									else {
										OS_Lin[c + 0] = new r8b::CDSPResampler24(1.0, 8.0, maxSample * 8, 2.0);
										OS_Lin[c + 2] = new r8b::CDSPResampler24(8.0, 1.0, maxSample * 32, 2.0);
									}
									dly_up = OS_Lin[c + 0]->getInLenBeforeOutPos(0);
									dly_dn = OS_Lin[c + 2]->getInLenBeforeOutPos(0);
								}
							}
							if (fParamOSOld != fParamOS) sendTextMessage("OS");
							break;
						case kParamZoom:
							fParamZoom = value;
							break;
						case kParamSplit:
							bSplit = (value > 0.5f);
							break;
						case kParamPhase:
							fParamPhase = (value > 0.5f);
							sendTextMessage("OS");
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

		fInVuPPML = init_VU;
		fInVuPPMR = init_VU;
		fOutVuPPML = init_VU;
		fOutVuPPMR = init_VU;

		fMeter = init_VU;
		fMeterOld = init_VU;
		Meter = init_VU;

		//---in bypass mode outputs should be like inputs-----
		if (bBypass)
		{
			if (in[0] != out[0]) { memcpy(out[0], in[0], sampleFramesSize); }
			if (in[1] != out[1]) { memcpy(out[1], in[1], sampleFramesSize); }

			if (data.symbolicSampleSize == Vst::kSample32) {
				processVuPPM_In<Vst::Sample32>((Vst::Sample32**)in, data.numSamples);
			}
			else if (data.symbolicSampleSize == Vst::kSample64) {
				processVuPPM_In<Vst::Sample64>((Vst::Sample64**)in, data.numSamples);
			}

			fMeter = 0.0;

			fInVuPPML = VuPPMconvert(fInVuPPML);
			fInVuPPMR = VuPPMconvert(fInVuPPMR);
			fOutVuPPML = fInVuPPML;
			fOutVuPPMR = fInVuPPMR;

		}
		else {
			if (data.symbolicSampleSize == Vst::kSample32) {
				overSampling<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, getSampleRate, data.numSamples);
			}
			else {
				overSampling<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, getSampleRate, data.numSamples);
			}

			Meter /= data.numSamples;
			Meter *= fInVuPPML;
			Meter *= 0.2;
			fMeter = 0.7 * log10(20.0 * Meter);

			fInVuPPML = VuPPMconvert(fInVuPPML);
			fInVuPPMR = VuPPMconvert(fInVuPPMR);
			fOutVuPPML = VuPPMconvert(fOutVuPPML);
			fOutVuPPMR = VuPPMconvert(fOutVuPPMR);
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

			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamMeter, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fMeter, index2);
			}

		}
		fInVuPPMLOld = fInVuPPML;
		fInVuPPMROld = fInVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;

		fMeterOld = fMeter;

		return kResultOk;
	}


	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
		Band_Split_set(&Band_Split_L, 240.0, 2400.0, newSetup.sampleRate);
		Band_Split_set(&Band_Split_R, 240.0, 2400.0, newSetup.sampleRate);


		for (int c = 0; c < 2; c++) {
			in_0[c] = (Vst::Sample64*)realloc(in_0[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock);
			in_1[c] = (Vst::Sample64*)realloc(in_1[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 2);
			in_2[c] = (Vst::Sample64*)realloc(in_2[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 4);
			in_3[c] = (Vst::Sample64*)realloc(in_3[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 8);
			out_0[c] = (Vst::Sample64*)realloc(out_0[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock);
			out_1[c] = (Vst::Sample64*)realloc(out_1[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 2);
			out_2[c] = (Vst::Sample64*)realloc(out_2[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 4);
			out_3[c] = (Vst::Sample64*)realloc(out_3[c], sizeof(Vst::Sample64) * newSetup.maxSamplesPerBlock * 8);
			if ((in_0[c] == NULL) || (in_1[c] == NULL) ||
				(in_2[c] == NULL) || (in_3[c] == NULL)) return kResultFalse;
			if ((out_0[c] == NULL) || (out_1[c] == NULL) ||
				(out_2[c] == NULL) || (out_3[c] == NULL)) return kResultFalse;

			upSample_2x_1[c].clear_buffers();
			upSample_4x_1[c].clear_buffers();
			upSample_4x_2[c].clear_buffers();
			upSample_8x_1[c].clear_buffers();
			upSample_8x_2[c].clear_buffers();
			upSample_8x_3[c].clear_buffers();
			dnSample_2x_1[c].clear_buffers();
			dnSample_4x_1[c].clear_buffers();
			dnSample_4x_2[c].clear_buffers();
			dnSample_8x_1[c].clear_buffers();
			dnSample_8x_2[c].clear_buffers();
			dnSample_8x_3[c].clear_buffers();

			/*
			upSample_2x_Lin[c]->clear();
			upSample_4x_Lin[c]->clear();
			upSample_8x_Lin[c]->clear();
			dnSample_2x_Lin[c]->clear();
			dnSample_4x_Lin[c]->clear();
			dnSample_8x_Lin[c]->clear();
			*/

		}

		//--- called before any processing ----
		return AudioEffect::setupProcessing(newSetup);
	} 

	uint32 PLUGIN_API InflatorPackageProcessor::getLatencySamples() 
	{
		if (fParamPhase) {
			if      (fParamOS == overSample_1x) return 0;
			else if (fParamOS == overSample_2x) return 3286;
			else if (fParamOS == overSample_4x) return 3337;
			else                                return 3401;
		}

		if      (fParamOS == overSample_1x) return 0;
		else if (fParamOS == overSample_2x) return hiir::os_2x_latency;
		else if (fParamOS == overSample_4x) return hiir::os_4x_latency;
		else                                return hiir::os_8x_latency;
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

		Vst::ParamValue savedInput  = 0.0;
		Vst::ParamValue savedEffect = 0.0;
		Vst::ParamValue savedCurve  = 0.0;
		Vst::ParamValue savedOutput = 0.0;
		Vst::ParamValue savedOS     = 0.0;
		int32           savedClip   = 0;
		int32           savedIn     = 0;
		int32           savedSplit  = 0;
		Vst::ParamValue savedZoom   = 0.0;
		Vst::ParamValue savedLin    = 0;
		int32           savedBypass = 0;

		if (streamer.readDouble(savedInput)  == false) return kResultFalse;
		if (streamer.readDouble(savedEffect) == false) return kResultFalse;
		if (streamer.readDouble(savedCurve)  == false) return kResultFalse;
		if (streamer.readDouble(savedOutput) == false) return kResultFalse;
		if (streamer.readDouble(savedOS)     == false) return kResultFalse;
		if (streamer.readInt32(savedClip)    == false) return kResultFalse;
		if (streamer.readInt32(savedIn)      == false) return kResultFalse;
		if (streamer.readInt32(savedSplit)   == false) return kResultFalse;
		if (streamer.readDouble(savedZoom)   == false) return kResultFalse;
		if (streamer.readDouble(savedLin)    == false) return kResultFalse;
		if (streamer.readInt32(savedBypass)  == false) return kResultFalse;

		fInput     = savedInput;
		fEffect    = savedEffect;
		fCurve     = savedCurve;
		fOutput    = savedOutput;
		fParamOS   = convert_to_OS(savedOS);
		bClip      = savedClip   > 0;
		bIn        = savedIn     > 0;
		bSplit     = savedSplit  > 0;
		fParamZoom = savedZoom;
		fParamPhase= savedLin;
		bBypass    = savedBypass > 0;

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

		streamer.writeDouble(fInput);
		streamer.writeDouble(fEffect);
		streamer.writeDouble(fCurve);
		streamer.writeDouble(fOutput);
		streamer.writeDouble(convert_from_OS(fParamOS));
		streamer.writeInt32(bClip ? 1 : 0);
		streamer.writeInt32(bIn ? 1 : 0);
		streamer.writeInt32(bSplit ? 1 : 0);
		streamer.writeDouble(fParamZoom);
		streamer.writeDouble(fParamPhase);
		streamer.writeInt32(bBypass ? 1 : 0);

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

		fInVuPPML = tmpL;
		fInVuPPMR = tmpR;
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
		fOutVuPPML = tmpL;
		fOutVuPPMR = tmpR;
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

			inputSampleL *= In_db;
			inputSampleR *= In_db;

			if (!bIn && bClip) {
				if      (inputSampleL >  1.0) inputSampleL =  1.0;
				else if (inputSampleL < -1.0) inputSampleL = -1.0;
				if      (inputSampleR >  1.0) inputSampleR =  1.0;
				else if (inputSampleR < -1.0) inputSampleR = -1.0;
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

			inputSampleL *= Out_db;
			inputSampleR *= Out_db;

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


	Vst::Sample64 InflatorPackageProcessor::process_inflator(Vst::Sample64 inputSample) 
	{
		Vst::Sample64 drySample = inputSample;
		Vst::Sample64 wet = fEffect;
		Vst::Sample64 sign;

		if (inputSample > 0.0) sign =  1.0;
		else                   sign = -1.0;

		Vst::Sample64 s1 = fabs(inputSample);
		Vst::Sample64 s2 = s1 * s1;
		Vst::Sample64 s3 = s2 * s1;
		Vst::Sample64 s4 = s2 * s2;

		if      (s1 >= 2.0) inputSample = 0.0;
		else if (s1 >  1.0) inputSample = (2.0 * s1) - s2;
		else                inputSample = (curveA * s1) +
		                                  (curveB * s2) +
		                                  (curveC * s3) -
		                                  (curveD * (s2 - (2.0 * s3) + s4));

		inputSample *= sign;

		if (wet != 1.0) {
			inputSample = (drySample * (1.0 - wet)) + (inputSample * wet);
		}

		if (bClip) {
			if      (inputSample >  1.0) inputSample =  1.0;
			else if (inputSample < -1.0) inputSample = -1.0;
		}

		// Meter += 20.0 * (log10(inputSample) - log10(dry));
		Meter += 20.0 * log10(inputSample / drySample);
		return inputSample;
	}

	template <typename SampleType>
	void InflatorPackageProcessor::processAudio(
		SampleType** inputs, 
		SampleType** outputs, 
		Vst::SampleRate getSampleRate, 
		long long sampleFrames
	)
	{
		if (!bIn) {
			memcpy(outputs[0], inputs[0], sizeof(SampleType) * sampleFrames);
			memcpy(outputs[1], inputs[1], sizeof(SampleType) * sampleFrames);
			return;
		}

		SampleType* In_L =  (SampleType*)inputs[0];
		SampleType* In_R =  (SampleType*)inputs[1];
		SampleType* Out_L = (SampleType*)outputs[0];
		SampleType* Out_R = (SampleType*)outputs[1];

		curvepct = fCurve - 0.5;
		curveA = 1.5 + curvepct; 
		curveB = -(curvepct + curvepct); 
		curveC = curvepct - 0.5; 
		curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	

		while (--sampleFrames >= 0)
		{
			Vst::Sample64 inputSampleL = *In_L;
			Vst::Sample64 inputSampleR = *In_R;

			if (bSplit) {
				Band_Split_L.LP.R = Band_Split_L.LP.I + Band_Split_L.LP.C * (inputSampleL - Band_Split_L.LP.I);
				Band_Split_L.LP.I = 2 * Band_Split_L.LP.R - Band_Split_L.LP.I;

				Band_Split_L.HP.R = (1 - Band_Split_L.HP.C) * Band_Split_L.HP.I + Band_Split_L.HP.C * inputSampleL;
				Band_Split_L.HP.I = 2 * Band_Split_L.HP.R - Band_Split_L.HP.I;

				Band_Split_R.LP.R = Band_Split_R.LP.I + Band_Split_R.LP.C * (inputSampleR - Band_Split_R.LP.I);
				Band_Split_R.LP.I = 2 * Band_Split_R.LP.R - Band_Split_R.LP.I;

				Band_Split_R.HP.R = (1 - Band_Split_R.HP.C) * Band_Split_R.HP.I + Band_Split_R.HP.C * inputSampleR;
				Band_Split_R.HP.I = 2 * Band_Split_R.HP.R - Band_Split_R.HP.I;

				Vst::Sample64 inputSampleL_L = Band_Split_L.LP.R;
				Vst::Sample64 inputSampleL_H = inputSampleL - Band_Split_L.HP.R;
				Vst::Sample64 inputSampleL_M = Band_Split_L.HP.R - Band_Split_L.LP.R;

				Vst::Sample64 inputSampleR_L = Band_Split_R.LP.R;
				Vst::Sample64 inputSampleR_H = inputSampleR - Band_Split_R.HP.R;
				Vst::Sample64 inputSampleR_M = Band_Split_R.HP.R - Band_Split_R.LP.R;

				inputSampleL = 
					process_inflator(inputSampleL_L) + 
					process_inflator(inputSampleL_M * Band_Split_L.G) * Band_Split_L.GR + 
					process_inflator(inputSampleL_H);
				inputSampleR =
					process_inflator(inputSampleR_L) +
					process_inflator(inputSampleR_M * Band_Split_R.G) * Band_Split_R.GR +
					process_inflator(inputSampleR_H);
			}
			else {
				inputSampleL = process_inflator(inputSampleL);
				inputSampleR = process_inflator(inputSampleR);
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
		long len_1 = sampleFrames;
		long len_2 = len_1 * 2;
		long len_4 = len_1 * 4;
		long len_8 = len_1 * 8;
		Vst::SampleRate SR_1 = getSampleRate;
		Vst::SampleRate SR_2 = getSampleRate * 2.0;
		Vst::SampleRate SR_4 = getSampleRate * 4.0;
		Vst::SampleRate SR_8 = getSampleRate * 8.0;

		for (int c = 0; c < 2; c++) {
			memset(in_0[c], 0, sampleFrames);
			memset(in_1[c], 0, sampleFrames * 2);
			memset(in_2[c], 0, sampleFrames * 4);
			memset(in_3[c], 0, sampleFrames * 8);
			memset(out_0[c], 0, sampleFrames);
			memset(out_1[c], 0, sampleFrames * 2);
			memset(out_2[c], 0, sampleFrames * 4);
			memset(out_3[c], 0, sampleFrames * 8);
		}

		proc_in<SampleType>((SampleType**)inputs, in_0, len_1); // 32 -> 64bit & dither?
		processVuPPM_In<Vst::Sample64>(in_0, len_1);
		OS_stages(true, len_1); // 1x -> nx

		if      (fParamOS == overSample_1x) processAudio<Vst::Sample64>(in_0, out_0, SR_1, len_1);
		else if (fParamOS == overSample_2x) processAudio<Vst::Sample64>(in_1, out_1, SR_2, len_2);
		else if (fParamOS == overSample_4x) processAudio<Vst::Sample64>(in_2, out_2, SR_4, len_4);
		else                                processAudio<Vst::Sample64>(in_3, out_3, SR_8, len_8);

		OS_stages(false, len_1);
		proc_out<SampleType>(out_0, (SampleType**)outputs, len_1);
		processVuPPM_Out<SampleType>((SampleType**)outputs, len_1);
		return;
	}
	



	void InflatorPackageProcessor::OS_stages(bool isUp, long remaining) {
		long len_1 = remaining;
		long len_2 = len_1 * 2;
		long len_4 = len_1 * 4;
		long len_8 = len_1 * 8;

		if (fParamOS == overSample_1x) return;

		for (uint32_t c = 0; c < 2; ++c) {
			if (!fParamPhase && isUp) {
				if (fParamOS == overSample_2x) {
					upSample_2x_1[c].process_block((Vst::Sample64*)in_1[c], (Vst::Sample64*)in_0[c], len_1);
				}
				else if (fParamOS == overSample_4x) {
					upSample_4x_1[c].process_block((Vst::Sample64*)in_1[c], (Vst::Sample64*)in_0[c], len_1);
					upSample_4x_2[c].process_block((Vst::Sample64*)in_2[c], (Vst::Sample64*)in_1[c], len_2);
				}
				else {
					upSample_8x_1[c].process_block((Vst::Sample64*)in_1[c], (Vst::Sample64*)in_0[c], len_1);
					upSample_8x_2[c].process_block((Vst::Sample64*)in_2[c], (Vst::Sample64*)in_1[c], len_2);
					upSample_8x_3[c].process_block((Vst::Sample64*)in_3[c], (Vst::Sample64*)in_2[c], len_4);
				}
			}
			else if (!fParamPhase && !isUp) {
				if (fParamOS == overSample_2x) {
					dnSample_2x_1[c].process_block((Vst::Sample64*)out_0[c], (Vst::Sample64*)out_1[c], len_1);
				}
				else if (fParamOS == overSample_4x) {
					dnSample_4x_1[c].process_block((Vst::Sample64*)out_1[c], (Vst::Sample64*)out_2[c], len_2);
					dnSample_4x_2[c].process_block((Vst::Sample64*)out_0[c], (Vst::Sample64*)out_1[c], len_1);
				}
				else {
					dnSample_8x_1[c].process_block((Vst::Sample64*)out_2[c], (Vst::Sample64*)out_3[c], len_4);
					dnSample_8x_2[c].process_block((Vst::Sample64*)out_1[c], (Vst::Sample64*)out_2[c], len_2);
					dnSample_8x_3[c].process_block((Vst::Sample64*)out_0[c], (Vst::Sample64*)out_1[c], len_1);
				}
			}
			else if (fParamPhase && isUp) {
				double* outPtr;

				int numInputSamples = (int)len_1;

				int inputCounter = 0;
				int outputCounter = 0;

				while (numInputSamples > 0) {

					int samplesToProcess = (std::min)(numInputSamples, (int)maxSample);
					int numUpSampledSamples = 0;

					if (fParamOS == overSample_2x) {
						numUpSampledSamples = OS_Lin[c]->process(&in_0[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							std::copy(outPtr, outPtr + numUpSampledSamples, &in_1[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
					else if (fParamOS == overSample_4x) {						
						numUpSampledSamples = OS_Lin[c]->process(&in_0[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							if (outputCounter + numUpSampledSamples > len_4) numUpSampledSamples = len_4 - outputCounter;
							std::copy(outPtr, outPtr + numUpSampledSamples, &in_2[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
					else {
						numUpSampledSamples = OS_Lin[c]->process(&in_0[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							if (outputCounter + numUpSampledSamples > len_8) numUpSampledSamples = len_8 - outputCounter;
							std::copy(outPtr, outPtr + numUpSampledSamples, &in_3[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
				}
			}
			else if (fParamPhase && !isUp) { // LinPhase && !isUp
				double* outPtr;
				int numInputSamples = 0;
				if      (fParamOS == overSample_2x) { numInputSamples = (int)len_2; }
				else if (fParamOS == overSample_4x) { numInputSamples = (int)len_4; }
				else                                { numInputSamples = (int)len_8; }
				int inputCounter = 0;
				int outputCounter = 0;
				while (numInputSamples > 0) {
					if (fParamOS == overSample_2x) {
						int samplesToProcess = (std::min)(numInputSamples, (int)maxSample * 2);
						int numUpSampledSamples = 0;
						numUpSampledSamples = OS_Lin[c+2]->process(&out_1[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							std::copy(outPtr, outPtr + numUpSampledSamples, &out_0[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
					else if (fParamOS == overSample_4x) {
						int samplesToProcess = (std::min)(numInputSamples, (int)maxSample * 4);
						int numUpSampledSamples = 0;
						numUpSampledSamples = OS_Lin[c+2]->process(&out_2[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							if (outputCounter + numUpSampledSamples > len_1) numUpSampledSamples = len_1 - outputCounter;
							std::copy(outPtr, outPtr + numUpSampledSamples, &out_0[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
					else {
						int samplesToProcess = (std::min)(numInputSamples, (int)maxSample * 8);
						int numUpSampledSamples = 0;
						numUpSampledSamples = OS_Lin[c+2]->process(&out_3[c][inputCounter], (int)samplesToProcess, outPtr);
						inputCounter += samplesToProcess;
						numInputSamples -= samplesToProcess;
						if (numUpSampledSamples > 0) {
							if (outputCounter + numUpSampledSamples > len_1) { numUpSampledSamples = len_1 - outputCounter; }
							std::copy(outPtr, outPtr + numUpSampledSamples, &out_0[c][outputCounter]);
							outputCounter += numUpSampledSamples;
						}
					}
				}
			}
			else {
			}
		}
	}


} // namespace yg331


