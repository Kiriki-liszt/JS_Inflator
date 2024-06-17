//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "JSIF_processor.h"
#include "JSIF_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/futils.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"

#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"


#if defined( _WIN32 )
    #include <windows.h>
#else // defined( _WIN32 )
    #include <pthread.h>
#endif // defined( _WIN32 )

#if defined( __SSE4_2__ ) || defined( __SSE4_1__ ) || \
    defined( __SSSE3__ ) || defined( __SSE3__ ) || defined( __SSE2__ ) || \
    defined( __x86_64__ ) || defined( __amd64 ) || defined( _M_X64 ) || \
    defined( _M_AMD64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP == 2 )

    #if defined( _MSC_VER )
        #include <intrin.h>
    #else // defined( _MSC_VER )
        #include <emmintrin.h>
    #endif // defined( _MSC_VER )

    #define R8B_SSE2
    #define R8B_SIMD_ISH

#elif defined( __aarch64__ ) || defined( __arm64 )

    #include <arm_neon.h>

    #define R8B_NEON

    #if !defined( __APPLE__ )
        #define R8B_SIMD_ISH // Shuffled interpolation is inefficient on M1.
    #endif // !defined( __APPLE__ )

#endif // AArch64



using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// JSIF_Processor
	//------------------------------------------------------------------------
	JSIF_Processor::JSIF_Processor() :
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
		fParamZoom(init_Zoom),
		fParamPhase(init_Phase),
		upSample_2x_Lin{ {1.0, 2.0, 1 * 2}, {1.0, 2.0, 1 * 2} },
		dnSample_2x_Lin{ {2.0, 1.0, 1 * 2}, {2.0, 1.0, 1 * 2} },
		upSample_4x_Lin{ {1.0, 4.0, 1 * 4, 2.1}, {1.0, 4.0, 1 * 4, 2.1} },
		dnSample_4x_Lin{ {4.0, 1.0, 1 * 4, 2.1}, {4.0, 1.0, 1 * 4, 2.1} },
		upSample_8x_Lin{ {1.0, 8.0, 1 * 8, 2.2}, {1.0, 8.0, 1 * 8, 2.2} },
		dnSample_8x_Lin{ {8.0, 1.0, 1 * 8, 2.2}, {8.0, 1.0, 1 * 8, 2.2} }
	{
		//--- set the wanted controller for our processor
		setControllerClass(kJSIF_ControllerUID);
	}

	//------------------------------------------------------------------------
	JSIF_Processor::~JSIF_Processor()
	{}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::initialize(FUnknown* context)
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
		addAudioInput (STR16("Audio Input"),  Vst::SpeakerArr::kStereo);
		addAudioOutput(STR16("Audio Output"), Vst::SpeakerArr::kStereo);

		/* If you don't need an event bus, you can remove the next line */
		// addEventInput(STR16("Event In"), 1);

		for (int channel = 0; channel < 2; channel++) {
			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, upTap_21, 100.0, upSample_21[channel].coef); //
			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, upTap_41, 100.0, upSample_41[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 49000.0, upTap_42, 100.0, upSample_42[channel].coef);
			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, upTap_81, 100.0, upSample_81[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 50000.0, upTap_82, 100.0, upSample_82[channel].coef);
			Kaiser::calcFilter(384000.0, 0.0, 98000.0, upTap_83, 100.0, upSample_83[channel].coef);

			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, dnTap_21, 100.0, dnSample_21[channel].coef); //
			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, dnTap_41, 100.0, dnSample_41[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 49000.0, dnTap_42, 100.0, dnSample_42[channel].coef);
			Kaiser::calcFilter( 96000.0, 0.0, 24000.0, dnTap_81, 100.0, dnSample_81[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 50000.0, dnTap_82, 100.0, dnSample_82[channel].coef);
			Kaiser::calcFilter(384000.0, 0.0, 98000.0, dnTap_83, 100.0, dnSample_83[channel].coef);

			for (int i = 0; i < upTap_21; i++) upSample_21[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_41; i++) upSample_41[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_42; i++) upSample_42[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_81; i++) upSample_81[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_82; i++) upSample_82[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_83; i++) upSample_83[channel].coef[i] *= 2.0;
		}

		// latency_r8b_x2 = -1 + 2 * upSample_2x_Lin[0].getInLenBeforeOutPos(1) +1;
		// latency_r8b_x4 = -1 + 2 * upSample_4x_Lin[0].getInLenBeforeOutPos(1);
		// latency_r8b_x8 = -1 + 2 * upSample_8x_Lin[0].getInLenBeforeOutPos(1);
		
		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
		fInputVu.clear();
		fInputVu.shrink_to_fit();
		fOutputVu.clear();
		fOutputVu.shrink_to_fit();
		buff[0].clear();
		buff[0].shrink_to_fit();
		buff[1].clear();
		buff[1].shrink_to_fit();
		//---do not forget to call parent ------
		return AudioEffect::terminate();
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::canProcessSampleSize(int32 symbolicSampleSize)
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
	tresult PLUGIN_API JSIF_Processor::setBusArrangements(
		Vst::SpeakerArrangement* inputs,  int32 numIns,
		Vst::SpeakerArrangement* outputs, int32 numOuts)
	{
		if (numIns == 1 && numOuts == 1)
		{
			// the host wants Mono => Mono (or 1 channel -> 1 channel)
			if (Vst::SpeakerArr::getChannelCount(inputs[0]) == 1 &&
				Vst::SpeakerArr::getChannelCount(outputs[0]) == 1)
			{
				auto* bus = FCast<Vst::AudioBus>(audioInputs.at(0));
				if (bus)
				{
					// check if we are Mono => Mono, if not we need to recreate the busses
					if (bus->getArrangement() != inputs[0])
					{
						getAudioInput(0)->setArrangement(inputs[0]);
						getAudioInput(0)->setName(STR16("Mono In"));
						getAudioOutput(0)->setArrangement(outputs[0]);
						getAudioOutput(0)->setName(STR16("Mono Out"));
					}
					return kResultOk;
				}
			}
			// the host wants something else than Mono => Mono,
			// in this case we are always Stereo => Stereo
			else
			{
				auto* bus = FCast<Vst::AudioBus>(audioInputs.at(0));
				if (bus)
				{
					tresult result = kResultFalse;

					// the host wants 2->2 (could be LsRs -> LsRs)
					if (Vst::SpeakerArr::getChannelCount(inputs[0]) == 2 &&
						Vst::SpeakerArr::getChannelCount(outputs[0]) == 2)
					{
						getAudioInput(0)->setArrangement(inputs[0]);
						getAudioInput(0)->setName(STR16("Stereo In"));
						getAudioOutput(0)->setArrangement(outputs[0]);
						getAudioOutput(0)->setName(STR16("Stereo Out"));
						result = kResultTrue;
					}
					// the host want something different than 1->1 or 2->2 : in this case we want stereo
					else if (bus->getArrangement() != Vst::SpeakerArr::kStereo)
					{
						getAudioInput(0)->setArrangement(Vst::SpeakerArr::kStereo);
						getAudioInput(0)->setName(STR16("Stereo In"));
						getAudioOutput(0)->setArrangement(Vst::SpeakerArr::kStereo);
						getAudioOutput(0)->setName(STR16("Stereo Out"));
						result = kResultFalse;
					}

					return result;
				}
			}
		}
		return kResultFalse;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::connect(Vst::IConnectionPoint* other)
	{
		auto result = Vst::AudioEffect::connect(other);
		if (result == kResultTrue)
		{
			auto configCallback = [this](
				Vst::DataExchangeHandler::Config& config,
				const Vst::ProcessSetup& setup
				)
				{
					Vst::SpeakerArrangement arr;
					getBusArrangement(Vst::BusDirections::kInput, 0, arr);
					numChannels = static_cast<uint16_t> (Vst::SpeakerArr::getChannelCount(arr));
					auto sampleSize = sizeof(float);

					config.blockSize = (numChannels * sampleSize) + sizeof(DataBlock);
					config.numBlocks = 2;
					config.alignment = 16;
					config.userContextID = 0;
					return true;
				};

			dataExchange = std::make_unique<Vst::DataExchangeHandler>(this, configCallback);
			dataExchange->onConnect(other, getHostContext());
		}
		return result;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::disconnect(Vst::IConnectionPoint* other)
	{
		if (dataExchange)
		{
			dataExchange->onDisconnect(other);
			dataExchange.reset();
		}
		return AudioEffect::disconnect(other);
	}


	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
        notSetupProcessing(newSetup);
		//--- called before any processing ----
		return AudioEffect::setupProcessing(newSetup);
	}

	uint32 PLUGIN_API JSIF_Processor::getLatencySamples()
	{
        //FDebugPrint("[ FDebugPrint ] getLatencySamples\n");
		if (fParamPhase) {
			if      (fParamOS == overSample_1x) return 0;
			else if (fParamOS == overSample_2x) return latency_r8b_x2;
			else if (fParamOS == overSample_4x) return latency_r8b_x4;
			else                                return latency_r8b_x8;
		}
		else {
			if      (fParamOS == overSample_1x) return 0;
			else if (fParamOS == overSample_2x) return latency_Fir_x2;
			else if (fParamOS == overSample_4x) return latency_Fir_x4;
			else                                return latency_Fir_x8;
		}
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::setActive(TBool state)
	{
		//--- called when the Plug-in is enable/disable (On/Off) -----

		if (state) {
			if (dataExchange)
				dataExchange->onActivate(processSetup);
		}
		else {
			if (dataExchange)
				dataExchange->onDeactivate();
		}
        //FDebugPrint("[ FDebugPrint ] setActive - %s\n", state?"true":"false");
		return AudioEffect::setActive(state);
	}

	//------------------------------------------------------------------------
	void JSIF_Processor::acquireNewExchangeBlock()
	{
		currentExchangeBlock = dataExchange->getCurrentOrNewBlock();
		if (auto block = toDataBlock(currentExchangeBlock))
		{
			block->sampleRate = static_cast<uint32_t> (processSetup.sampleRate);
			//block->numChannels = numChannels;
			//block->sampleSize = sizeof(float);
			block->numSamples = 0;
			block->inL = 0.0;
			block->inR = 0.0;
			block->outL = 0.0;
			block->outR = 0.0;
			block->gR = 0.0;
		}
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::process(Vst::ProcessData& data)
	{		
        if(!setupProcessing_checked)
        {
            notSetupProcessing(processSetup);
        }
        
        
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
						case kParamInput:  fInput      = value;          break;
						case kParamEffect: fEffect     = value;          break;
						case kParamCurve:  fCurve      = value;          break;
						case kParamClip:   bClip       = (value > 0.5f); break;
						case kParamOutput: fOutput     = value;          break;
						case kParamBypass: bBypass     = (value > 0.5f); break;
						case kParamIn:     bIn         = (value > 0.5f); break;
						case kParamZoom:   fParamZoom  = value;          break;
						case kParamSplit:  bSplit      = (value > 0.5f); break;
                        case kParamPhase:
                                fParamPhase = (value > 0.5f);
                                setupProcessing_checked = false; 
                                sendTextMessage("OS");
                                break;
                        case kParamOS:
                                fParamOS    = static_cast<overSample>(Steinberg::FromNormalized<ParamValue> (value, overSample_num));
                                setupProcessing_checked = false;
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
		int32 numChannels = data.inputs[0].numChannels;


		//---check if silence---------------
		// check if all channel are silent then process silent
		if (data.inputs[0].silenceFlags == Vst::getChannelMask(data.inputs[0].numChannels))
		{
			// mark output silence too (it will help the host to propagate the silence)
			data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

			// the plug-in has to be sure that if it sets the flags silence that the output buffer are
			// clear
			for (int32 channel = 0; channel < numChannels; channel++)
			{
				// do not need to be cleared if the buffers are the same (in this case input buffer are already cleared by the host)
				if (in[channel] != out[channel])
				{
					memset(out[channel], 0, sampleFramesSize);
				}
			}
			return kResultOk;
		}

		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		for (auto& loop : fInputVu) loop = init_meter;
		for (auto& loop : fOutputVu) loop = init_meter;
		fMeterVu = init_meter;
		

		//---in bypass mode outputs should be like inputs-----
		if (bBypass)
		{
			if (data.symbolicSampleSize == Vst::kSample32) {
				latencyBypass<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, getSampleRate, data.numSamples);

			}
			else if (data.symbolicSampleSize == Vst::kSample64) {
				latencyBypass<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, getSampleRate, data.numSamples);
			}

			fMeterVu = 0.0;
		}
		else {
			if (data.symbolicSampleSize == Vst::kSample32) {
				processAudio<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, getSampleRate, data.numSamples);
			}
			else {
				processAudio<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, getSampleRate, data.numSamples);
			}

			long div = data.numSamples;

			Meter /= (double)div;
			Meter /= (double)numChannels;
			Meter *= 1000.0;

			fMeterVu = 0.4 * log10(std::abs(Meter));
			// FDebugPrint("Meter = %f \n", fMeterVu);
		}
        double monoIn = 0.0;
        for (auto& loop : fInputVu) {loop = VuPPMconvert(loop);  monoIn += loop;}
		for (auto& loop : fOutputVu) loop = VuPPMconvert(loop);
        monoIn /= (double)numChannels;
        fMeterVu *= monoIn;
        
		if (currentExchangeBlock.blockID == Vst::InvalidDataExchangeBlockID)
			acquireNewExchangeBlock();

		if (auto block = toDataBlock(currentExchangeBlock))
		{
			int fps = block->sampleRate / 30;
			auto numSamples = static_cast<uint32> (data.numSamples);
			while (numSamples > 0)
			{
				uint32 numSamplesFreeInBlock = block->sampleRate - block->numSamples;
				uint32 numSamplesToCopy = std::min<uint32>(numSamplesFreeInBlock, numSamples);
				numSamplesToCopy = std::min<uint32>(numSamplesToCopy, fps);

				/*
				for (auto channel = 0; channel < input.numChannels; ++channel)
				{
					auto blockChannelData = &block->samples[0] + block->numSamples;
					auto inputChannel = input.channelBuffers32[channel] + (data.numSamples - numSamples);
					memcpy(blockChannelData, inputChannel, numSamplesToCopy * sizeof(float));
				}
				*/
				block->numSamples += numSamplesToCopy;

				if (block->numSamples >= fps) // 1sec
				{
					block->inL = (fInputVu.size() > 0) ? fInputVu[0] : 0.0;
					block->inR = (fInputVu.size() > 1) ? fInputVu[1] : 0.0;
					block->outL = (fOutputVu.size() > 0) ? fOutputVu[0] : 0.0;
					block->outR = (fOutputVu.size() > 1) ? fOutputVu[1] : 0.0;
					block->gR = fMeterVu;
					// FDebugPrint("%f | %f \n", block->inL, block->outL);
					dataExchange->sendCurrentBlock();
					acquireNewExchangeBlock();
					block = toDataBlock(currentExchangeBlock);
					if (block == nullptr)
						break;
				}

				numSamples -= numSamplesToCopy;
			}
		}

		return kResultOk;
	}


	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::setState(IBStream* state)
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
        fParamOS   = static_cast<overSample>(Steinberg::FromNormalized<ParamValue> (savedOS, overSample_num));
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
	tresult PLUGIN_API JSIF_Processor::getState(IBStream* state)
	{
		// here we need to save the model
		IBStreamer streamer(state, kLittleEndian);

		streamer.writeDouble(fInput);
		streamer.writeDouble(fEffect);
		streamer.writeDouble(fCurve);
		streamer.writeDouble(fOutput);
		streamer.writeDouble(Steinberg::ToNormalized<ParamValue> (static_cast<ParamValue>(fParamOS), overSample_num));
		streamer.writeInt32(bClip ? 1 : 0);
		streamer.writeInt32(bIn ? 1 : 0);
		streamer.writeInt32(bSplit ? 1 : 0);
		streamer.writeDouble(fParamZoom);
		streamer.writeDouble(fParamPhase);
		streamer.writeInt32(bBypass ? 1 : 0);

		return kResultOk;
	}


	float JSIF_Processor::VuPPMconvert(float plainValue)
	{
		float dB = 20 * log10f(plainValue);
		float normValue;

		if      (dB >=  6.0) normValue = 1.0;
		else if (dB >=  5.0) normValue = 0.96;
		else if (dB >=  4.0) normValue = 0.92;
		else if (dB >=  3.0) normValue = 0.88;
		else if (dB >=  2.0) normValue = 0.84;
		else if (dB >=  1.0) normValue = 0.80;
		else if (dB >=  0.0) normValue = 0.76;
		else if (dB >= -1.0) normValue = 0.72;
		else if (dB >= -2.0) normValue = 0.68;
		else if (dB >= -3.0) normValue = 0.64;
		else if (dB >= -4.0) normValue = 0.60;
		else if (dB >= -5.0) normValue = 0.56;
		else if (dB >= -6.0) normValue = 0.52;
		else if (dB >= -8.0) normValue = 0.48;
		else if (dB >= -10 ) normValue = 0.44;
		else if (dB >= -12 ) normValue = 0.40;
		else if (dB >= -14 ) normValue = 0.36;
		else if (dB >= -16 ) normValue = 0.32;
		else if (dB >= -18 ) normValue = 0.28;
		else if (dB >= -22 ) normValue = 0.24;
		else if (dB >= -26 ) normValue = 0.20;
		else if (dB >= -30 ) normValue = 0.16;
		else if (dB >= -34 ) normValue = 0.12;
		else if (dB >= -38 ) normValue = 0.08;
		else if (dB >= -42 ) normValue = 0.04;
		else normValue = 0.0;

		return normValue;
	}

	//------------------------------------------------------------------------
	template <typename SampleType>
	void JSIF_Processor::processVuPPM_In(SampleType** inputs, int32 numChannels, int32 sampleFrames)
	{
		VuInput.update(inputs, numChannels, sampleFrames);
		for (int ch = 0; ch < numChannels; ch++)
		{
			fInputVu[ch] = VuInput.getEnv(ch);
		}
		return;
	}


	Vst::Sample64 JSIF_Processor::process_inflator(Vst::Sample64 inputSample) 
	{
		// Vst::Sample64 drySample = inputSample;
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

		return inputSample;
	}

	template <typename SampleType>
	void JSIF_Processor::latencyBypass(
		SampleType** inputs,
		SampleType** outputs,
		int32 numChannels,
		Vst::SampleRate getSampleRate,
		long long sampleFrames
	)
	{
		for (int32 channel = 0; channel < numChannels; channel++)
		{
			SampleType* ptrIn  = (SampleType*) inputs[channel];
			SampleType* ptrOut = (SampleType*)outputs[channel];
			int32 samples = sampleFrames;
			
			if (fParamOS == overSample_1x) {
				memcpy(ptrOut, ptrIn, sizeof(SampleType) * sampleFrames);
				continue;
			}

			int32 latency = 0;
			if (!fParamPhase) {
				if      (fParamOS == overSample_2x) latency = latency_Fir_x2;
				else if (fParamOS == overSample_4x) latency = latency_Fir_x4;
				else if (fParamOS == overSample_8x) latency = latency_Fir_x8;
			}
			else {
				if      (fParamOS == overSample_2x) latency = latency_r8b_x2;
				else if (fParamOS == overSample_4x) latency = latency_r8b_x4;
				else if (fParamOS == overSample_8x) latency = latency_r8b_x8;
			}
			if (latency != latency_q[channel].size()) {
				int32 diff = latency - (int32)latency_q[channel].size();
				if (diff > 0) 
					for (int i = 0; i < diff; i++) latency_q[channel].push_back(0.0);
				else 
					for (int i = 0; i < -diff; i++) latency_q[channel].pop_front();
			}

			while (--samples >= 0)
			{
				double inin = *ptrIn;
				*ptrOut = (SampleType)latency_q[channel].front();
				latency_q[channel].pop_front();
				latency_q[channel].push_back(inin);

				ptrIn++;
				ptrOut++;
			}
		}
		VuInput.update(inputs, numChannels, sampleFrames);
		VuOutput.update(outputs, numChannels, sampleFrames);
 
		for (int ch = 0; ch < numChannels; ch++)
		{
			fInputVu[ch] = VuInput.getEnv(ch);
			fOutputVu[ch] = VuOutput.getEnv(ch);
		}

		return;
	}


	template <typename SampleType>
	void JSIF_Processor::processAudio(
		SampleType** inputs, 
		SampleType** outputs, 
		int32 numChannels,
		Vst::SampleRate getSampleRate, 
		long long sampleFrames
	)
	{
		Vst::Sample64 In_db  = expf(logf(10.f) * (24.0 * fInput  - 12.0) / 20.f);
		Vst::Sample64 Out_db = expf(logf(10.f) * (12.0 * fOutput - 12.0) / 20.f);

		curvepct = fCurve - 0.5;
		curveA =        1.5 + curvepct; 
		curveB = -(curvepct + curvepct); 
		curveC =   curvepct - 0.5; 
		curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	

		Meter = 0.0;
		double t = 0.0;
		for (int32 channel = 0; channel < numChannels; channel++)
		{
			SampleType* ptrIn  = (SampleType*) inputs[channel];
			SampleType* ptrOut = (SampleType*)outputs[channel];
			double* buff_in = buff[channel].data();

			int32 latency = 0;
			if (!fParamPhase) {
				if      (fParamOS == overSample_2x) latency = latency_Fir_x2;
				else if (fParamOS == overSample_4x) latency = latency_Fir_x4;
				else if (fParamOS == overSample_8x) latency = latency_Fir_x8;
			}
			else {
				if      (fParamOS == overSample_2x) latency = latency_r8b_x2;
				else if (fParamOS == overSample_4x) latency = latency_r8b_x4;
				else if (fParamOS == overSample_8x) latency = latency_r8b_x8;
			}
			if (latency != latency_q[channel].size()) {
				int32 diff = latency - (int32)latency_q[channel].size();
				if (diff > 0) {
					for (int i = 0; i < diff; i++) latency_q[channel].push_back(0.0);
				}
				else {
					for (int i = 0; i < -diff; i++) latency_q[channel].pop_front();
				}
			}

			int32 samples = sampleFrames;

			while (--samples >= 0)
			{
				Vst::Sample64 inputSample = *ptrIn;

				inputSample *= In_db;

				if (bClip) {
					if      (inputSample >  1.0) inputSample =  1.0;
					else if (inputSample < -1.0) inputSample = -1.0;
				}

				if      (inputSample >  2.0) inputSample =  2.0;
				else if (inputSample < -2.0) inputSample = -2.0;

				Vst::Sample64 drySample = inputSample;

				int32 oversampling = 1;
				if      (fParamOS == overSample_2x) oversampling = 2;
				else if (fParamOS == overSample_4x) oversampling = 4;
				else if (fParamOS == overSample_8x) oversampling = 8;

				double up_x[8];
				double up_y[8];

				// Upsampling
				if              (fParamOS == overSample_1x) up_x[0] = inputSample;
				else {
					if (!fParamPhase) {
						if      (fParamOS == overSample_2x) Fir_x2_up(&inputSample, up_x, channel);
						else if (fParamOS == overSample_4x) Fir_x4_up(&inputSample, up_x, channel);
						else                                Fir_x8_up(&inputSample, up_x, channel);
					}
					else {
						double* upSample_buff;
						if      (fParamOS == overSample_2x) upSample_2x_Lin[channel].process(&inputSample, 1, upSample_buff);
						else if (fParamOS == overSample_4x) upSample_4x_Lin[channel].process(&inputSample, 1, upSample_buff);
						else                                upSample_8x_Lin[channel].process(&inputSample, 1, upSample_buff);
						memcpy(up_x, upSample_buff, sizeof(Vst::Sample64) * oversampling);
					}
				}

				// Processing
				for (int k = 0; k < oversampling; k++) {
					if (!bIn) {
						up_y[k] = up_x[k];
						continue;
					}
					Vst::Sample64 sampleOS = up_x[k];
					if (bSplit) {
						Band_Split[channel].LP.R =      Band_Split[channel].LP.I  + Band_Split[channel].LP.C * (sampleOS - Band_Split[channel].LP.I);
						Band_Split[channel].LP.I =  2 * Band_Split[channel].LP.R  - Band_Split[channel].LP.I;

						Band_Split[channel].HP.R = (1 - Band_Split[channel].HP.C) * Band_Split[channel].HP.I + Band_Split[channel].HP.C * sampleOS;
						Band_Split[channel].HP.I =  2 * Band_Split[channel].HP.R  - Band_Split[channel].HP.I;

						Vst::Sample64 inputSample_L = Band_Split[channel].LP.R;
						Vst::Sample64 inputSample_H = sampleOS - Band_Split[channel].HP.R;
						Vst::Sample64 inputSample_M = Band_Split[channel].HP.R - Band_Split[channel].LP.R;

						up_y[k] = process_inflator(inputSample_L) +
						          process_inflator(inputSample_M * Band_Split[channel].G) * Band_Split[channel].GR +
						          process_inflator(inputSample_H);
					}
					else {
						up_y[k] = process_inflator(sampleOS);
						// up_y[k] = up_x[k];
					}
					if (bClip && up_y[k] >  1.0) up_y[k] =  1.0;
					if (bClip && up_y[k] < -1.0) up_y[k] = -1.0;
				}

				// Downsampling
				if              (fParamOS == overSample_1x) inputSample = up_y[0];
				else {
					if (!fParamPhase) {
						if      (fParamOS == overSample_2x) Fir_x2_dn(up_y, &inputSample, channel);
						else if (fParamOS == overSample_4x) Fir_x4_dn(up_y, &inputSample, channel);
						else                                Fir_x8_dn(up_y, &inputSample, channel);
					}
					else {
						double* dnSample_buff;
						if      (fParamOS == overSample_2x) dnSample_2x_Lin[channel].process(up_y, oversampling, dnSample_buff);
						else if (fParamOS == overSample_4x) dnSample_4x_Lin[channel].process(up_y, oversampling, dnSample_buff);
						else                                dnSample_8x_Lin[channel].process(up_y, oversampling, dnSample_buff);
						inputSample = *dnSample_buff;
					}
				}


				// Latency compensate
				Vst::Sample64 delayed;
				latency_q[channel].push_back(drySample);
				delayed = latency_q[channel].front();
				latency_q[channel].pop_front();
				*buff_in = delayed;  buff_in++;
				inputSample = (delayed * (1.0 - fEffect)) + (inputSample * fEffect);
				t += std::abs(inputSample) - std::abs(delayed);
				// Meter += 20.0 * (std::log10(std::abs(inputSample)) - std::log10(std::abs(delayed)));

				inputSample *= Out_db;

				*ptrOut = (SampleType)inputSample;

				ptrIn++;
				ptrOut++;
			}
		}
		Meter = 80.0 - t;
		Meter *= fEffect;

		double* inplace[2] = { buff[0].data(), buff[1].data() };

		VuInput.update(inplace, numChannels, sampleFrames);
		VuOutput.update(outputs, numChannels, sampleFrames);

		for (int ch = 0; ch < numChannels; ch++)
		{
			fInputVu[ch]  = VuInput.getEnv(ch);
			fOutputVu[ch] = VuOutput.getEnv(ch);
			// FDebugPrint("%d | %f : %f \n", ch, fInputVu[ch], fOutputVu[ch]);
		}

		return;
	}

	/// Fir Linear Oversamplers

	// 1 in 2 out
	void JSIF_Processor::Fir_x2_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
		Vst::Sample64 inputSample = *in;

		const size_t upTap_21_size = sizeof(double) * (upTap_21 - 1) / 2;
		memmove(upSample_21[channel].buff + 1, upSample_21[channel].buff, upTap_21_size);
		upSample_21[channel].buff[0] = inputSample;
		__m128d _acc_a = _mm_setzero_pd();
		for (int i = 0, j = 0; i < upTap_21; i += 4, j += 2) {
			__m128d coef_a = _mm_load_pd(&upSample_21[channel].coef[i]);
			__m128d coef_b = _mm_load_pd(&upSample_21[channel].coef[i + 2]);
			__m128d buff_a = _mm_load_pd(&upSample_21[channel].buff[j]);
			__m128d _mul_a = _mm_mul_pd(coef_a, _mm_shuffle_pd(buff_a, buff_a, 0)); 
			__m128d _mul_b = _mm_mul_pd(coef_b, _mm_shuffle_pd(buff_a, buff_a, 3)); 
			        _acc_a = _mm_add_pd(_acc_a, _mul_a);
			        _acc_a = _mm_add_pd(_acc_a, _mul_b);
		}
		_mm_store_pd(out, _acc_a);
	}
	// 1 in 4 out
	void JSIF_Processor::Fir_x4_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel)
	{
		Vst::Sample64 inputSample = *in;

		const size_t  upTap_41_size = sizeof(double) * (upTap_41 - 1)/2;
		memmove(upSample_41[channel].buff + 1, upSample_41[channel].buff, upTap_41_size);
		upSample_41[channel].buff[0] = inputSample;
		__m128d _acc_in = _mm_setzero_pd();
		for (int i = 0, j = 0; i < upTap_41; i += 4, j += 2) {
			__m128d coef_a = _mm_load_pd(&upSample_41[channel].coef[i]);
			__m128d coef_b = _mm_load_pd(&upSample_41[channel].coef[i+2]);
			__m128d buff = _mm_load_pd(&upSample_41[channel].buff[j]);
			__m128d _mul_a = _mm_mul_pd(coef_a, _mm_shuffle_pd(buff, buff, 0));
			__m128d _mul_b = _mm_mul_pd(coef_b, _mm_shuffle_pd(buff, buff, 3));
			_acc_in = _mm_add_pd(_acc_in, _mul_a);
			_acc_in = _mm_add_pd(_acc_in, _mul_b);
		}

		const size_t upTap_42_size = sizeof(double) * (upTap_42);
		memmove(upSample_42[channel].buff + 4, upSample_42[channel].buff, upTap_42_size);
		_mm_storel_pd(upSample_42[channel].buff + 3, _acc_in);
		_mm_storel_pd(upSample_42[channel].buff + 2, _acc_in);
		_mm_storeh_pd(upSample_42[channel].buff + 1, _acc_in);
		_mm_storeh_pd(upSample_42[channel].buff    , _acc_in);
		__m128d _acc_in_a = _mm_setzero_pd();
		__m128d _acc_in_b = _mm_setzero_pd();
		for (int i = 0; i < upTap_42; i += 2) {
			__m128d coef_a = _mm_load_pd(&upSample_42[channel].coef[i    ]);
			__m128d buff_a = _mm_load_pd(&upSample_42[channel].buff[i + 2]);
			//__m128d coef_b = _mm_load_pd(&upSample_42[channel].coef[i    ]);
			__m128d buff_b = _mm_load_pd(&upSample_42[channel].buff[i    ]);
			__m128d _mul_a = _mm_mul_pd(coef_a, buff_a);
			__m128d _mul_b = _mm_mul_pd(coef_a, buff_b);
			     _acc_in_a = _mm_add_pd(_acc_in_a, _mul_a);
			     _acc_in_b = _mm_add_pd(_acc_in_b, _mul_b);
		}
		_mm_store_pd(out, _acc_in_a);
		_mm_store_pd(out + 2, _acc_in_b);
	}
	// 1 in 8 out
	void JSIF_Processor::Fir_x8_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel)
	{
		Vst::Sample64 inputSample = *in;

		const size_t upTap_81_size = sizeof(double) * (upTap_81 - 1) / 2;
		memmove(upSample_81[channel].buff + 1, upSample_81[channel].buff, upTap_81_size);
		upSample_81[channel].buff[0] = inputSample;
		__m128d _acc_in_1 = _mm_setzero_pd();
		for (int i = 0, j = 0; i < upTap_81; i += 4, j += 2) {
			__m128d coef_1 = _mm_load_pd(&upSample_81[channel].coef[i]);
			__m128d coef_2 = _mm_load_pd(&upSample_81[channel].coef[i+2]);
			__m128d buff_1 = _mm_load_pd(&upSample_81[channel].buff[j]);
			__m128d _mul_1 = _mm_mul_pd(coef_1, _mm_shuffle_pd(buff_1, buff_1, 0));
			__m128d _mul_2 = _mm_mul_pd(coef_2, _mm_shuffle_pd(buff_1, buff_1, 3));
			     _acc_in_1 = _mm_add_pd(_acc_in_1, _mul_1);
			     _acc_in_1 = _mm_add_pd(_acc_in_1, _mul_2);
		}

		const size_t upTap_82_size = sizeof(double) * (upTap_82);
		memmove(upSample_82[channel].buff + 4, upSample_82[channel].buff, upTap_82_size);
		_mm_storel_pd(&upSample_82[channel].buff[3], _acc_in_1);
		_mm_storel_pd(&upSample_82[channel].buff[2], _acc_in_1);
		_mm_storeh_pd(&upSample_82[channel].buff[1], _acc_in_1);
		_mm_storeh_pd(&upSample_82[channel].buff[0], _acc_in_1);
		__m128d _acc_in_2a = _mm_setzero_pd();
		__m128d _acc_in_2b = _mm_setzero_pd();
		for (int i = 0; i < upTap_82; i += 2) {
			__m128d coef_2a = _mm_load_pd(&upSample_82[channel].coef[i    ]);
			__m128d buff_2a = _mm_load_pd(&upSample_82[channel].buff[i + 2]);
			//__m128d coef_2b = _mm_load_pd(&upSample_82[channel].coef[i    ]);
			__m128d buff_2b = _mm_load_pd(&upSample_82[channel].buff[i    ]);
			__m128d _mul_2a = _mm_mul_pd(coef_2a, buff_2a);
			__m128d _mul_2b = _mm_mul_pd(coef_2a, buff_2b);
			     _acc_in_2a = _mm_add_pd(_acc_in_2a, _mul_2a);
			     _acc_in_2b = _mm_add_pd(_acc_in_2b, _mul_2b);
		}

		const size_t upTap_83_size = sizeof(double) * (upTap_83);
		memmove(upSample_83[channel].buff + 8, upSample_83[channel].buff, upTap_83_size);
		_mm_storel_pd(&upSample_83[channel].buff[7], _acc_in_2a);
		_mm_storel_pd(&upSample_83[channel].buff[6], _acc_in_2a);
		_mm_storeh_pd(&upSample_83[channel].buff[5], _acc_in_2a);
		_mm_storeh_pd(&upSample_83[channel].buff[4], _acc_in_2a);
		_mm_storel_pd(&upSample_83[channel].buff[3], _acc_in_2b);
		_mm_storel_pd(&upSample_83[channel].buff[2], _acc_in_2b);
		_mm_storeh_pd(&upSample_83[channel].buff[1], _acc_in_2b);
		_mm_storeh_pd(&upSample_83[channel].buff[0], _acc_in_2b);
		__m128d _acc_in_3a = _mm_setzero_pd();
		__m128d _acc_in_3b = _mm_setzero_pd();
		__m128d _acc_in_3c = _mm_setzero_pd();
		__m128d _acc_in_3d = _mm_setzero_pd();
		for (int i = 0; i < upTap_83; i += 2) { // make_8(upTap_83) == sizeof_buff > upTap_83 + 6
			__m128d coef_3a = _mm_load_pd(&upSample_83[channel].coef[i    ]);
			__m128d buff_3a = _mm_load_pd(&upSample_83[channel].buff[i + 6]);
			//__m128d coef_3b = _mm_load_pd(&upSample_83[channel].coef[i    ]);
			__m128d buff_3b = _mm_load_pd(&upSample_83[channel].buff[i + 4]);
			//__m128d coef_3c = _mm_load_pd(&upSample_83[channel].coef[i    ]);
			__m128d buff_3c = _mm_load_pd(&upSample_83[channel].buff[i + 2]);
			//__m128d coef_3d = _mm_load_pd(&upSample_83[channel].coef[i    ]);
			__m128d buff_3d = _mm_load_pd(&upSample_83[channel].buff[i    ]);
			__m128d _mul_3a = _mm_mul_pd(coef_3a, buff_3a);
			__m128d _mul_3b = _mm_mul_pd(coef_3a, buff_3b);
			__m128d _mul_3c = _mm_mul_pd(coef_3a, buff_3c);
			__m128d _mul_3d = _mm_mul_pd(coef_3a, buff_3d);
			     _acc_in_3a = _mm_add_pd(_acc_in_3a, _mul_3a);
			     _acc_in_3b = _mm_add_pd(_acc_in_3b, _mul_3b);
			     _acc_in_3c = _mm_add_pd(_acc_in_3c, _mul_3c);
			     _acc_in_3d = _mm_add_pd(_acc_in_3d, _mul_3d);
		}
		_mm_store_pd(out    , _acc_in_3a);
		_mm_store_pd(out + 2, _acc_in_3b);
		_mm_store_pd(out + 4, _acc_in_3c);
		_mm_store_pd(out + 6, _acc_in_3d);
	}


	// 2 in 1 out
	void JSIF_Processor::Fir_x2_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
        const size_t dnTap_21_size = sizeof(double) * (dnTap_21 - 2);
        memmove(dnSample_21[channel].buff + 3, dnSample_21[channel].buff + 1, dnTap_21_size);
        dnSample_21[channel].buff[2] = in[0];
        dnSample_21[channel].buff[1] = in[1];

        enum {
            a0b0, // 00(2)
            a1b0, // 01(2) == loadr
            a0b1, // 10(2)
            a1b1  // 11(2)
        };
        static constexpr int TAP_HALF = dnTap_21 / 2;
        static constexpr int TAP_CONDITON = dnTap_21 % 4;

        double acc_a[2] = { 0,0 };

#if defined( R8B_SSE2 )

        __m128d _acc_a = _mm_setzero_pd();
        for (int i = 0; i < TAP_HALF - 1; i += 2) {
            __m128d coef_a = _mm_load_pd(&dnSample_21[channel].coef[i]);
            __m128d buff_a = _mm_load_pd(&dnSample_21[channel].buff[2 + i]);
            __m128d buff_ = _mm_loadu_pd(&dnSample_21[channel].buff[2 + dnTap_21 - 1 - i - 1]); // may be unaligned
            __m128d buff_b = _mm_shuffle_pd(buff_, buff_, a1b0);  // == loadr
            __m128d _mul_a = _mm_mul_pd(coef_a, _mm_add_pd(buff_a, buff_b));
            _acc_a = _mm_add_pd(_acc_a, _mul_a);
        }
        _mm_store_pd(acc_a, _acc_a);

#elif defined( R8B_NEON )

        float64x2_t _acc_a = vdupq_n_f64(0.0); // == set
        for (int i = 0; i < TAP_HALF - 1; i += 2) {
            float64x2_t coef_a = vld1q_f64(&dnSample_21[channel].coef[i]);
            float64x2_t buff_a = vld1q_f64(&dnSample_21[channel].buff[2 + i]);
            float64x2_t buff_ =  vld1q_f64(&dnSample_21[channel].buff[2 + dnTap_21 - 1 - i - 1]); // may be unaligned
            float64x2_t buff_b = vextq_f64(buff_, buff_, a1b0);  // == loadr
            float64x2_t _mul_a = vmulq_f64(coef_a, vaddq_f64(buff_a, buff_b));
            _acc_a = vaddq_f64(_acc_a, _mul_a);
        }
        vst1q_f64(acc_a, _acc_a);
        
        // FDebugBreak("Should not be accesed, it will stop build");

#else // SIMD

        FDebugBreak("Should not be accesed, it will stop build");

#endif // SIMD
        
#if defined(SIMDE_X86_SSE2_NATIVE)
        FDebugBreak("Should not be accesed, it will stop build");
#elif defined(SIMDE_ARM_NEON_A64V8_NATIVE)
    FDebugBreak("Should not be accesed, it will stop build");
#endif
            
        double acc = acc_a[0] + acc_a[1];
        if (TAP_CONDITON == 1)
        {
            acc += dnSample_21[channel].coef[TAP_HALF] * dnSample_21[channel].buff[2 + TAP_HALF];
        }
        else if (TAP_CONDITON == 3)
        {
            acc += dnSample_21[channel].coef[TAP_HALF - 1] * (dnSample_21[channel].buff[2 + TAP_HALF - 1] + dnSample_21[channel].buff[2 + TAP_HALF + 1]);
            acc += dnSample_21[channel].coef[TAP_HALF - 0] *  dnSample_21[channel].buff[2 + TAP_HALF];
        }
        *out = acc;
        return;
	}
	// 4 in 1 out
	void JSIF_Processor::Fir_x4_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
		double inter_42[4];
		double inter_21[2];

		const size_t dnTap_42_size = sizeof(double) * (dnTap_42);
		memmove(dnSample_42[channel].buff + 4, dnSample_42[channel].buff, dnTap_42_size);
		dnSample_42[channel].buff[4] = in[0]; // buff[3]
		dnSample_42[channel].buff[3] = in[1];
		dnSample_42[channel].buff[2] = in[2];
		dnSample_42[channel].buff[1] = in[3];
		__m128d _acc_out_a = _mm_setzero_pd();
		__m128d _acc_out_b = _mm_setzero_pd();
		for (int i = 0; i < dnTap_42; i += 2) { // tt >= dnTap_42+3
			__m128d coef_a = _mm_load_pd(&dnSample_42[channel].coef[i    ]);
			__m128d buff_a = _mm_load_pd(&dnSample_42[channel].buff[i + 4]); 
			//__m128d coef_b = _mm_load_pd(&dnSample_42[channel].coef[i    ]);
			__m128d buff_b = _mm_load_pd(&dnSample_42[channel].buff[i + 2]);
			__m128d _mul_a = _mm_mul_pd(coef_a, buff_a);
			__m128d _mul_b = _mm_mul_pd(coef_a, buff_b);
			    _acc_out_a = _mm_add_pd(_acc_out_a, _mul_a);
			    _acc_out_b = _mm_add_pd(_acc_out_b, _mul_b);
		}
		_mm_store_pd(inter_42    , _acc_out_a);
		_mm_store_pd(inter_42 + 2, _acc_out_b);

		const size_t dnTap_41_size = sizeof(double) * (dnTap_41);
		memmove(dnSample_41[channel].buff + 3, dnSample_41[channel].buff + 1, dnTap_41_size);
		dnSample_41[channel].buff[2] = inter_42[0] + inter_42[1];
		dnSample_41[channel].buff[1] = inter_42[2] + inter_42[3];
		__m128d _acc_out = _mm_setzero_pd();
		for (int i = 0; i < dnTap_41; i += 2) {
			__m128d coef = _mm_load_pd(&dnSample_41[channel].coef[i    ]);
			__m128d buff = _mm_load_pd(&dnSample_41[channel].buff[i + 2]);
			__m128d _mul = _mm_mul_pd(coef, buff);
			    _acc_out = _mm_add_pd(_acc_out, _mul);
		}
		_mm_store_pd(inter_21, _acc_out);
		*out = inter_21[0] + inter_21[1];
	}
	// 8 in 1 out
	void JSIF_Processor::Fir_x8_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
		double inter_84[8];
		double inter_42[4];
		double inter_21[2];
		const size_t dnTap_83_size = sizeof(double) * (dnTap_83);
		memmove(dnSample_83[channel].buff + 9, dnSample_83[channel].buff + 1, dnTap_83_size);
		dnSample_83[channel].buff[8] = in[0];
		dnSample_83[channel].buff[7] = in[1];
		dnSample_83[channel].buff[6] = in[2];
		dnSample_83[channel].buff[5] = in[3];
		dnSample_83[channel].buff[4] = in[4];
		dnSample_83[channel].buff[3] = in[5];
		dnSample_83[channel].buff[2] = in[6];
		dnSample_83[channel].buff[1] = in[7];
		__m128d _acc_out_3a = _mm_setzero_pd();
		__m128d _acc_out_3b = _mm_setzero_pd();
		__m128d _acc_out_3c = _mm_setzero_pd();
		__m128d _acc_out_3d = _mm_setzero_pd();
		for (int i = 0; i < dnTap_83; i += 2) {
			__m128d coef_3a = _mm_load_pd(&dnSample_83[channel].coef[i    ]);
			__m128d buff_3a = _mm_load_pd(&dnSample_83[channel].buff[i + 8]); 
			//__m128d coef_3b = _mm_load_pd(&dnSample_83[channel].coef[i    ]);
			__m128d buff_3b = _mm_load_pd(&dnSample_83[channel].buff[i + 6]);
			//__m128d coef_3c = _mm_load_pd(&dnSample_83[channel].coef[i    ]);
			__m128d buff_3c = _mm_load_pd(&dnSample_83[channel].buff[i + 4]);
			//__m128d coef_3d = _mm_load_pd(&dnSample_83[channel].coef[i    ]);
			__m128d buff_3d = _mm_load_pd(&dnSample_83[channel].buff[i + 2]);
			__m128d _mul_3a = _mm_mul_pd(coef_3a, buff_3a);
			__m128d _mul_3b = _mm_mul_pd(coef_3a, buff_3b);
			__m128d _mul_3c = _mm_mul_pd(coef_3a, buff_3c);
			__m128d _mul_3d = _mm_mul_pd(coef_3a, buff_3d);
			    _acc_out_3a = _mm_add_pd(_acc_out_3a, _mul_3a);
			    _acc_out_3b = _mm_add_pd(_acc_out_3b, _mul_3b);
			    _acc_out_3c = _mm_add_pd(_acc_out_3c, _mul_3c);
			    _acc_out_3d = _mm_add_pd(_acc_out_3d, _mul_3d);
		}
		_mm_store_pd(inter_84    , _acc_out_3a);
		_mm_store_pd(inter_84 + 2, _acc_out_3b);
		_mm_store_pd(inter_84 + 4, _acc_out_3c);
		_mm_store_pd(inter_84 + 6, _acc_out_3d);

		const size_t dnTap_82_size = sizeof(double) * (dnTap_82);
		memmove(dnSample_82[channel].buff + 5, dnSample_82[channel].buff + 1, dnTap_82_size);
		dnSample_82[channel].buff[4] = inter_84[0] + inter_84[1];
		dnSample_82[channel].buff[3] = inter_84[2] + inter_84[3];
		dnSample_82[channel].buff[2] = inter_84[4] + inter_84[5];
		dnSample_82[channel].buff[1] = inter_84[6] + inter_84[7];
		__m128d _acc_out_2a = _mm_setzero_pd();
		__m128d _acc_out_2b = _mm_setzero_pd();
		for (int i = 0; i < dnTap_82; i += 2) { 
			__m128d coef_2a = _mm_load_pd(&dnSample_82[channel].coef[i    ]);
			__m128d buff_2a = _mm_load_pd(&dnSample_82[channel].buff[i + 4]); 
			//__m128d coef_2b = _mm_load_pd(&dnSample_82[channel].coef[i    ]);
			__m128d buff_2b = _mm_load_pd(&dnSample_82[channel].buff[i + 2]);
			__m128d _mul_2a = _mm_mul_pd(coef_2a, buff_2a);
			__m128d _mul_2b = _mm_mul_pd(coef_2a, buff_2b);
			    _acc_out_2a = _mm_add_pd(_acc_out_2a, _mul_2a);
			    _acc_out_2b = _mm_add_pd(_acc_out_2b, _mul_2b);
		}
		_mm_store_pd(inter_42    , _acc_out_2a);
		_mm_store_pd(inter_42 + 2, _acc_out_2b);

		const size_t dnTap_81_size = sizeof(double) * (dnTap_81);
		memmove(dnSample_81[channel].buff + 2, dnSample_81[channel].buff, dnTap_81_size);
		dnSample_81[channel].buff[2] = inter_42[0] + inter_42[1];
		dnSample_81[channel].buff[1] = inter_42[2] + inter_42[3];
		__m128d _acc_out = _mm_setzero_pd();
		for (int i = 0; i < dnTap_81; i += 2) {
			__m128d coef = _mm_load_pd(&dnSample_81[channel].coef[i    ]);
			__m128d buff = _mm_load_pd(&dnSample_81[channel].buff[i + 2]);
			__m128d _mul = _mm_mul_pd(coef, buff);
			    _acc_out = _mm_add_pd(_acc_out, _mul);
		}
		_mm_store_pd(inter_21, _acc_out);
		*out = inter_21[0] + inter_21[1];
	}

} // namespace yg331
