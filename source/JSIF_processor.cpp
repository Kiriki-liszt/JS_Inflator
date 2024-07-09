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
        fParamZoom(init_Zoom),
        fParamPhase(init_Phase),
        bBypass(init_Bypass),
        bIn(init_In),
        bClip(init_Clip),
        bSplit(init_Split),
        curvepct(init_curvepct),
        curveA(init_curveA),
        curveB(init_curveB),
        curveC(init_curveC),
        curveD(init_curveD),
        fParamOS(init_OS)
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

		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API JSIF_Processor::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

#define clear_delete(vec) {vec.clear(); vec.shrink_to_fit();}

		clear_delete(fInputVu);
		clear_delete(fOutputVu);

		for (auto& loop : buff) 
			clear_delete(loop);
		clear_delete(buff);
		clear_delete(buff_head);

		for (auto& loop : upSample_2x_Lin) delete loop;
		for (auto& loop : upSample_4x_Lin) delete loop;
		for (auto& loop : upSample_8x_Lin) delete loop;
		for (auto& loop : dnSample_2x_Lin) delete loop;
		for (auto& loop : dnSample_4x_Lin) delete loop;
		for (auto& loop : dnSample_8x_Lin) delete loop;

		clear_delete(upSample_2x_Lin);
		clear_delete(upSample_4x_Lin);
		clear_delete(upSample_8x_Lin);
		clear_delete(dnSample_2x_Lin);
		clear_delete(dnSample_4x_Lin);
		clear_delete(dnSample_8x_Lin);

		clear_delete(upSample_21);
		clear_delete(upSample_41);
		clear_delete(upSample_42);
		clear_delete(upSample_81);
		clear_delete(upSample_82);
		clear_delete(upSample_83);
		clear_delete(dnSample_21);
		clear_delete(dnSample_41);
		clear_delete(dnSample_42);
		clear_delete(dnSample_81);
		clear_delete(dnSample_82);
		clear_delete(dnSample_83);

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
		Vst::SpeakerArrangement arr;
		getBusArrangement(Vst::BusDirections::kInput, 0, arr);
		numChannels = static_cast<uint16_t> (Vst::SpeakerArr::getChannelCount(arr));

		Band_Split.resize(numChannels);
		for (int32 channel = 0; channel < numChannels; channel++)
			Band_Split_set(&Band_Split[channel], 240.0, 2400.0, newSetup.sampleRate);

		upSample_21.resize(numChannels);
		upSample_41.resize(numChannels);
		upSample_42.resize(numChannels);
		upSample_81.resize(numChannels);
		upSample_82.resize(numChannels);
		upSample_83.resize(numChannels);

		dnSample_21.resize(numChannels);
		dnSample_41.resize(numChannels);
		dnSample_42.resize(numChannels);
		dnSample_81.resize(numChannels);
		dnSample_82.resize(numChannels);
		dnSample_83.resize(numChannels);

		upSample_2x_Lin.resize(numChannels);
		upSample_4x_Lin.resize(numChannels);
		upSample_8x_Lin.resize(numChannels);
		dnSample_2x_Lin.resize(numChannels);
		dnSample_4x_Lin.resize(numChannels);
		dnSample_8x_Lin.resize(numChannels);

		for (int channel = 0; channel < numChannels; channel++) 
		{
            memset(upSample_21[channel].coef, 0, sizeof(upSample_21[channel].coef));
            memset(upSample_41[channel].coef, 0, sizeof(upSample_41[channel].coef));
            memset(upSample_42[channel].coef, 0, sizeof(upSample_42[channel].coef));
            memset(upSample_81[channel].coef, 0, sizeof(upSample_81[channel].coef));
            memset(upSample_82[channel].coef, 0, sizeof(upSample_82[channel].coef));
            memset(upSample_83[channel].coef, 0, sizeof(upSample_83[channel].coef));
            
            memset(dnSample_21[channel].coef, 0, sizeof(dnSample_21[channel].coef));
            memset(dnSample_41[channel].coef, 0, sizeof(dnSample_41[channel].coef));
            memset(dnSample_42[channel].coef, 0, sizeof(dnSample_42[channel].coef));
            memset(dnSample_81[channel].coef, 0, sizeof(dnSample_81[channel].coef));
            memset(dnSample_82[channel].coef, 0, sizeof(dnSample_82[channel].coef));
            memset(dnSample_83[channel].coef, 0, sizeof(dnSample_83[channel].coef));
            
			// Strictly HALF_BAND
			Kaiser::calcFilter(96000.0,  0.0, 24000.0, upTap_21, 100.0, upSample_21[channel].coef); //
			Kaiser::calcFilter(96000.0,  0.0, 24000.0, upTap_41, 100.0, upSample_41[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 48000.0, upTap_42, 100.0, upSample_42[channel].coef);
			Kaiser::calcFilter(96000.0,  0.0, 24000.0, upTap_81, 100.0, upSample_81[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 48000.0, upTap_82, 100.0, upSample_82[channel].coef);
			Kaiser::calcFilter(384000.0, 0.0, 96000.0, upTap_83, 100.0, upSample_83[channel].coef);

			Kaiser::calcFilter(96000.0,  0.0, 24000.0, dnTap_21, 100.0, dnSample_21[channel].coef); //
			Kaiser::calcFilter(96000.0,  0.0, 24000.0, dnTap_41, 100.0, dnSample_41[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 48000.0, dnTap_42, 100.0, dnSample_42[channel].coef);
			Kaiser::calcFilter(96000.0,  0.0, 24000.0, dnTap_81, 100.0, dnSample_81[channel].coef); //
			Kaiser::calcFilter(192000.0, 0.0, 48000.0, dnTap_82, 100.0, dnSample_82[channel].coef);
			Kaiser::calcFilter(384000.0, 0.0, 96000.0, dnTap_83, 100.0, dnSample_83[channel].coef);

			for (int i = 0; i < upTap_21; i++) upSample_21[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_41; i++) upSample_41[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_42; i++) upSample_42[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_81; i++) upSample_81[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_82; i++) upSample_82[channel].coef[i] *= 2.0;
			for (int i = 0; i < upTap_83; i++) upSample_83[channel].coef[i] *= 2.0;


#define FLT_SET(filter, tap_size) \
filter[channel].TAP_SIZE = tap_size; \
filter[channel].TAP_HALF = tap_size / 2; \
filter[channel].TAP_HALF_HALF = filter[channel].TAP_HALF / 2; \
filter[channel].TAP_CONDITION = tap_size % 4; \
if (filter[channel].TAP_CONDITION == 1) \
{ \
	/* 0 == coef[0], coef[2], coef[4], coef[6], ... */ \
	filter[channel].START = 1; \
} \
else if (filter[channel].TAP_CONDITION == 3) \
{ \
	/* 0 == coef[1], coef[3], coef[5], coef[7], ... */ \
	filter[channel].START = 0; \
} \

			FLT_SET(upSample_21, upTap_21);
			FLT_SET(upSample_41, upTap_41);
			FLT_SET(upSample_42, upTap_42);
			FLT_SET(upSample_81, upTap_81);
			FLT_SET(upSample_82, upTap_82);
			FLT_SET(upSample_83, upTap_83);

			FLT_SET(dnSample_21, dnTap_21);
			FLT_SET(dnSample_41, dnTap_41);
			FLT_SET(dnSample_42, dnTap_42);
			FLT_SET(dnSample_81, dnTap_81);
			FLT_SET(dnSample_82, dnTap_82);
			FLT_SET(dnSample_83, dnTap_83);

			upSample_2x_Lin[channel] = new r8b::CDSPResampler24(1.0, 2.0, 1 * 2, 2.0);
			upSample_4x_Lin[channel] = new r8b::CDSPResampler24(1.0, 4.0, 1 * 4, 2.1);
			upSample_8x_Lin[channel] = new r8b::CDSPResampler24(1.0, 8.0, 1 * 8, 2.2);
			dnSample_2x_Lin[channel] = new r8b::CDSPResampler24(2.0, 1.0, 1 * 2, 2.0);
			dnSample_4x_Lin[channel] = new r8b::CDSPResampler24(4.0, 1.0, 1 * 4, 2.1);
			dnSample_8x_Lin[channel] = new r8b::CDSPResampler24(8.0, 1.0, 1 * 8, 2.2);
		}

		VuInput.setChannel(numChannels);
		VuInput.setType(LevelEnvelopeFollower::Peak);
		VuInput.setDecay(3.0);
		VuInput.prepare(newSetup.sampleRate);

		VuOutput.setChannel(numChannels);
		VuOutput.setType(LevelEnvelopeFollower::Peak);
		VuOutput.setDecay(3.0);
		VuOutput.prepare(newSetup.sampleRate);

		fInputVu.resize(numChannels, 0.0);
		fOutputVu.resize(numChannels, 0.0);

		buff.resize(numChannels);
		buff_head.resize(numChannels);
		for (int channel = 0; channel < numChannels; channel++)
		{
			buff[channel].resize(newSetup.maxSamplesPerBlock, 0.0);
			buff_head[channel] = buff[channel].data();
		}

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
						                   sendTextMessage("OS");
						                   break;
                        case kParamOS:
						                   fParamOS    = static_cast<overSample>(Steinberg::FromNormalized<ParamValue> (value, overSample_num));
						                   sendTextMessage("OS");
						                   break;
						}
					}
				}
			}
		}

		if (data.numInputs == 0 || data.numOutputs == 0) 
		{
			return kResultOk;
		}

		//---get audio buffers----------------
		uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
		void** in  = getChannelBuffersPointer(processSetup, data.inputs[0]);
		void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
		Vst::SampleRate SampleRate = processSetup.sampleRate;
		int32 numChannels = data.inputs[0].numChannels;

		// init VuMeters
		for (auto& loop : fInputVu) loop = init_meter;
		for (auto& loop : fOutputVu) loop = init_meter;
		fMeterVu = init_meter;

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
			// Even Silence, we should process VU meter and send data. 
			// return kResultOk;
		}
		else
		{
			data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

			//---in bypass mode outputs should be like inputs-----
			if (bBypass)
			{
				if (data.symbolicSampleSize == Vst::kSample32)
				{
					latencyBypass<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, SampleRate, data.numSamples);
				}
				else if (data.symbolicSampleSize == Vst::kSample64)
				{
					latencyBypass<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, SampleRate, data.numSamples);
				}

				fMeterVu = 0.0;
			}
			else {
				if (data.symbolicSampleSize == Vst::kSample32)
				{
					processAudio<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, SampleRate, data.numSamples);
				}
				else if (data.symbolicSampleSize == Vst::kSample64)
				{
					processAudio<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, SampleRate, data.numSamples);
				}

				long div = data.numSamples;

				Meter /= (double)div;
				Meter /= (double)numChannels;
				Meter *= 1000.0;

				fMeterVu = 0.4 * log10(std::abs(Meter));
				fMeterVu *= fEffect;
				fMeterVu *= bIn;
				// FDebugPrint("Meter = %f \n", fMeterVu);
			}

			double monoIn = 0.0;
			for (auto& loop : fInputVu) { loop = VuPPMconvert(loop);  monoIn += loop; }
			for (auto& loop : fOutputVu) loop = VuPPMconvert(loop);
			monoIn /= (double)numChannels;
			fMeterVu *= monoIn;
		}
        
		if (currentExchangeBlock.blockID == Vst::InvalidDataExchangeBlockID)
			acquireNewExchangeBlock();

		if (auto block = toDataBlock(currentExchangeBlock))
		{
			block->inL = (numChannels > 0) ? fInputVu[0] : 0.0;
			block->inR = (numChannels > 1) ? fInputVu[1] : ((numChannels > 0) ? fInputVu[0] : 0.0);
			block->outL = (numChannels > 0) ? fOutputVu[0] : 0.0;
			block->outR = (numChannels > 1) ? fOutputVu[1] : ((numChannels > 0) ? fOutputVu[0] : 0.0);
			block->gR = fMeterVu;
			// FDebugPrint("%f | %f \n", block->inL, block->outL);
			dataExchange->sendCurrentBlock();
			acquireNewExchangeBlock();
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


    Vst::ParamValue JSIF_Processor::VuPPMconvert(Vst::ParamValue plainValue)
	{
        Vst::ParamValue dB = 20 * log10(plainValue);
        Vst::ParamValue normValue;

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
		Vst::SampleRate SampleRate,
		int32 sampleFrames
	)
	{
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

		for (int32 channel = 0; channel < numChannels; channel++)
		{
			SampleType* ptrIn  = (SampleType*) inputs[channel];
			SampleType* ptrOut = (SampleType*)outputs[channel];
			int32 samples = sampleFrames;
			
			if (fParamOS == overSample_1x) {
				memcpy(ptrOut, ptrIn, sizeof(SampleType) * sampleFrames);
				continue;
			}

			if (latency != latency_q[channel].size()) {
				int32 diff = latency - (int32)latency_q[channel].size();
				if (diff > 0) 
					for (int i = 0; i <  diff; i++) latency_q[channel].push_back(0.0);
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
			fInputVu[ch]  = VuInput.getEnv(ch);
			fOutputVu[ch] = VuOutput.getEnv(ch);
		}

		return;
	}


	template <typename SampleType>
	void JSIF_Processor::processAudio(
		SampleType** inputs, 
		SampleType** outputs, 
		int32 numChannels,
		Vst::SampleRate SampleRate, 
		int32 sampleFrames
	)
	{
		Vst::Sample64 In_db  = expf(logf(10.f) * (24.0 * fInput  - 12.0) / 20.f);
		Vst::Sample64 Out_db = expf(logf(10.f) * (12.0 * fOutput - 12.0) / 20.f);

		curvepct = fCurve - 0.5;
		curveA =        1.5 + curvepct; 
		curveB = -(curvepct + curvepct); 
		curveC =   curvepct - 0.5; 
		curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	

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

		int32 oversampling = 1;
		if      (fParamOS == overSample_2x) oversampling = 2;
		else if (fParamOS == overSample_4x) oversampling = 4;
		else if (fParamOS == overSample_8x) oversampling = 8;

		Vst::SampleRate targetSampleRate = SampleRate * oversampling;
		
		for (int32 channel = 0; channel < numChannels; channel++)
			if (Band_Split[channel].SR != targetSampleRate) 
				Band_Split_set(&Band_Split[channel], 240.0, 2400.0, targetSampleRate);
        
        double up_x[8] = {0.0, };
        double up_y[8] = {0.0, };

		Meter = 0.0;
		double t = 0.0;

		for (int32 channel = 0; channel < numChannels; channel++)
		{
			SampleType* ptrIn  = (SampleType*) inputs[channel];
			SampleType* ptrOut = (SampleType*)outputs[channel];
			double* buff_in = buff[channel].data();
			
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
						if      (fParamOS == overSample_2x) upSample_2x_Lin[channel]->process(&inputSample, 1, upSample_buff);
						else if (fParamOS == overSample_4x) upSample_4x_Lin[channel]->process(&inputSample, 1, upSample_buff);
						else                                upSample_8x_Lin[channel]->process(&inputSample, 1, upSample_buff);
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
						if      (fParamOS == overSample_2x) dnSample_2x_Lin[channel]->process(up_y, oversampling, dnSample_buff);
						else if (fParamOS == overSample_4x) dnSample_4x_Lin[channel]->process(up_y, oversampling, dnSample_buff);
						else                                dnSample_8x_Lin[channel]->process(up_y, oversampling, dnSample_buff);
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

		VuInput.update(buff_head.data(), numChannels, sampleFrames);
		VuOutput.update(outputs, numChannels, sampleFrames);

		for (int ch = 0; ch < numChannels; ch++)
		{
			fInputVu[ch]  = VuInput.getEnv(ch);
			fOutputVu[ch] = VuOutput.getEnv(ch);
		}

		return;
	}

	/// Fir Linear Oversamplers
	void JSIF_Processor::HB_upsample(Flt* filter, Vst::Sample64* out)
	{
        // half-band
        double acc = 0.0;
        for (int coef = filter->START, buff = 0; coef < filter->TAP_HALF; coef += 2, buff++)
        {
            acc += filter->coef[coef] * (filter->buff[buff] + filter->buff[filter->TAP_HALF - buff - filter->START]);
        }

        double acc_1 = 0.0;
        double acc_2 = 0.0;
        if (filter->TAP_CONDITION == 1)
        {
            acc_1 = filter->coef[filter->TAP_HALF] * filter->buff[filter->TAP_HALF_HALF];
            acc_2 = acc;
        }
        else // if (filter->TAP_CONDITION == 3)
        {
            acc_1 = acc;
            acc_2 = filter->coef[filter->TAP_HALF] *  filter->buff[filter->TAP_HALF_HALF];
        }

        *(out  ) = acc_1;
        *(out+1) = acc_2;
	}
	void JSIF_Processor::HB_dnsample(Flt* filter, Vst::Sample64* out)
	{
		// half-band
		double acc = 0.0;
		int TAP_SIZE_1 = filter->TAP_SIZE - 1;
		for (int i = filter->START; i < filter->TAP_HALF; i+=2) // buffer + 2 is offset
		{
			acc += filter->coef[i] * (filter->buff[2 + i] + filter->buff[2 + TAP_SIZE_1 - i]);
		}
		acc += filter->coef[filter->TAP_HALF] * filter->buff[2 + filter->TAP_HALF];
		*out = acc;
	}

	// 1 in 2 out
	void JSIF_Processor::Fir_x2_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
        static constexpr size_t upTap_21_size = sizeof(double) * (upTap_21 - 1) / 2;
        memmove(upSample_21[channel].buff + 1, upSample_21[channel].buff, upTap_21_size);
        upSample_21[channel].buff[0] = *in;
        HB_upsample(&upSample_21[channel], out);
        
        return;
	}
	// 1 in 4 out
    void JSIF_Processor::Fir_x4_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel)
    {
		Vst::Sample64 inter_41[2];
		static constexpr size_t  upTap_41_size = sizeof(double) * (upTap_41 - 1) / 2;
        memmove(upSample_41[channel].buff + 1, upSample_41[channel].buff, upTap_41_size);
        upSample_41[channel].buff[0] = *in;
        HB_upsample(&upSample_41[channel], &inter_41[0]);
        
		static constexpr size_t upTap_42_size = sizeof(double) * (upTap_42 - 1) / 2;
        memmove(upSample_42[channel].buff + 1, upSample_42[channel].buff, upTap_42_size);
        upSample_42[channel].buff[0] = inter_41[0];
        HB_upsample(&upSample_42[channel], &out[0]);

        memmove(upSample_42[channel].buff + 1, upSample_42[channel].buff, upTap_42_size);
        upSample_42[channel].buff[0] = inter_41[1];
        HB_upsample(&upSample_42[channel], &out[2]);
        
        return;
    }
	// 1 in 8 out
	void JSIF_Processor::Fir_x8_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel)
	{
        Vst::Sample64 inter_81[2];
		static constexpr size_t upTap_81_size = sizeof(double) * (upTap_81 - 1) / 2;
		memmove(upSample_81[channel].buff + 1, upSample_81[channel].buff, upTap_81_size);
		upSample_81[channel].buff[0] = *in;
        HB_upsample(&upSample_81[channel], &inter_81[0]);
        
        Vst::Sample64 inter_82[4];
		static constexpr size_t upTap_82_size = sizeof(double) * (upTap_82 - 1) / 2;
        memmove(upSample_82[channel].buff + 1, upSample_82[channel].buff, upTap_82_size);
        upSample_82[channel].buff[0] = inter_81[0];
        HB_upsample(&upSample_82[channel], &inter_82[0]);

        memmove(upSample_82[channel].buff + 1, upSample_82[channel].buff, upTap_82_size);
        upSample_82[channel].buff[0] = inter_81[1];
        HB_upsample(&upSample_82[channel], &inter_82[2]);
        
		static constexpr size_t upTap_83_size = sizeof(double) * (upTap_83 - 1) / 2;
		memmove(upSample_83[channel].buff + 1, upSample_83[channel].buff, upTap_83_size);
        upSample_83[channel].buff[0] = inter_82[0];
        HB_upsample(&upSample_83[channel], &out[0]);
        
        memmove(upSample_83[channel].buff + 1, upSample_83[channel].buff, upTap_83_size);
        upSample_83[channel].buff[0] = inter_82[1];
        HB_upsample(&upSample_83[channel], &out[2]);
        
        memmove(upSample_83[channel].buff + 1, upSample_83[channel].buff, upTap_83_size);
        upSample_83[channel].buff[0] = inter_82[2];
        HB_upsample(&upSample_83[channel], &out[4]);
                
        memmove(upSample_83[channel].buff + 1, upSample_83[channel].buff, upTap_83_size);
        upSample_83[channel].buff[0] = inter_82[3];
        HB_upsample(&upSample_83[channel], &out[6]);

		return;
	}


	// 2 in 1 out
	void JSIF_Processor::Fir_x2_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
		static constexpr size_t dnTap_21_size = sizeof(double) * (dnTap_21 - 2);
        memmove(dnSample_21[channel].buff + 3, dnSample_21[channel].buff + 1, dnTap_21_size);
        dnSample_21[channel].buff[2] = in[0];
        dnSample_21[channel].buff[1] = in[1];
        HB_dnsample(&dnSample_21[channel], out);

        return;
	}
	// 4 in 1 out
    void JSIF_Processor::Fir_x4_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel)
    {
        Vst::Sample64 inter_42[2];
        static constexpr size_t dnTap_42_size = sizeof(double) * (dnTap_42 - 2);
        memmove(dnSample_42[channel].buff + 3, dnSample_42[channel].buff + 1, dnTap_42_size);
        dnSample_42[channel].buff[2] = in[0];
        dnSample_42[channel].buff[1] = in[1];
        HB_dnsample(&dnSample_42[channel], &inter_42[0]);

        memmove(dnSample_42[channel].buff + 3, dnSample_42[channel].buff + 1, dnTap_42_size);
        dnSample_42[channel].buff[2] = in[2];
        dnSample_42[channel].buff[1] = in[3];
        HB_dnsample(&dnSample_42[channel], &inter_42[1]);

        const size_t dnTap_41_size = sizeof(double) * (dnTap_41-2);
        memmove(dnSample_41[channel].buff + 3, dnSample_41[channel].buff + 1, dnTap_41_size);
        dnSample_41[channel].buff[2] = inter_42[0];
        dnSample_41[channel].buff[1] = inter_42[1];
        HB_dnsample(&dnSample_41[channel], out);

        return;
    }
	// 8 in 1 out
	void JSIF_Processor::Fir_x8_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel) 
	{
        Vst::Sample64 inter_83[4];
		static constexpr size_t dnTap_83_size = sizeof(double) * (dnTap_83-2);
		memmove(dnSample_83[channel].buff + 3, dnSample_83[channel].buff + 1, dnTap_83_size);
		dnSample_83[channel].buff[2] = in[0];
		dnSample_83[channel].buff[1] = in[1];
        HB_dnsample(&dnSample_83[channel], &inter_83[0]);
        
        memmove(dnSample_83[channel].buff + 3, dnSample_83[channel].buff + 1, dnTap_83_size);
        dnSample_83[channel].buff[2] = in[2];
        dnSample_83[channel].buff[1] = in[3];
        HB_dnsample(&dnSample_83[channel], &inter_83[1]);
        
        memmove(dnSample_83[channel].buff + 3, dnSample_83[channel].buff + 1, dnTap_83_size);
        dnSample_83[channel].buff[2] = in[4];
        dnSample_83[channel].buff[1] = in[5];
        HB_dnsample(&dnSample_83[channel], &inter_83[2]);
        
        memmove(dnSample_83[channel].buff + 3, dnSample_83[channel].buff + 1, dnTap_83_size);
        dnSample_83[channel].buff[2] = in[6];
        dnSample_83[channel].buff[1] = in[7];
        HB_dnsample(&dnSample_83[channel], &inter_83[3]);
        
        Vst::Sample64 inter_82[2];
		static constexpr size_t dnTap_82_size = sizeof(double) * (dnTap_82-2);
        memmove(dnSample_82[channel].buff + 3, dnSample_82[channel].buff + 1, dnTap_82_size);
        dnSample_82[channel].buff[2] = inter_83[0];
        dnSample_82[channel].buff[1] = inter_83[1];
        HB_dnsample(&dnSample_82[channel], &inter_82[0]);

        memmove(dnSample_82[channel].buff + 3, dnSample_82[channel].buff + 1, dnTap_82_size);
        dnSample_82[channel].buff[2] = inter_83[2];
        dnSample_82[channel].buff[1] = inter_83[3];
        HB_dnsample(&dnSample_82[channel], &inter_82[1]);

		static constexpr size_t dnTap_81_size = sizeof(double) * (dnTap_81-2);
        memmove(dnSample_81[channel].buff + 3, dnSample_81[channel].buff + 1, dnTap_81_size);
        dnSample_81[channel].buff[2] = inter_82[0];
        dnSample_81[channel].buff[1] = inter_82[1];
        HB_dnsample(&dnSample_81[channel], out);

		return;
	}

} // namespace yg331
