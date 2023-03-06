//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

using namespace Steinberg;

namespace yg331 {

	//------------------------------------------------------------------------
	//  InflatorPackageProcessor
	//------------------------------------------------------------------------
	class InflatorPackageProcessor : public Steinberg::Vst::AudioEffect
	{
	public:
		InflatorPackageProcessor();
		~InflatorPackageProcessor() SMTG_OVERRIDE;

		// Create function
		static Steinberg::FUnknown* createInstance(void* /*context*/)
		{
			return (Steinberg::Vst::IAudioProcessor*)new InflatorPackageProcessor;
		}

		//--- ---------------------------------------------------------------------
		// AudioEffect overrides:
		//--- ---------------------------------------------------------------------
		/** Called at first after constructor */
		Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;

		/** Called at the end before destructor */
		Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

		tresult PLUGIN_API setBusArrangements(
			Vst::SpeakerArrangement* inputs, int32 numIns,
			Vst::SpeakerArrangement* outputs, int32 numOuts
		) SMTG_OVERRIDE;


		/** Switch the Plug-in on/off */
		Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

		/** Will be called before any process call */
		Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;

		/** Asks if a given sample size is supported see SymbolicSampleSizes. */
		Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

		/** Here we go...the process call */
		Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

		/** For persistence */
		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

		//------------------------------------------------------------------------
	protected:
		//==============================================================================
		template <typename SampleType>
		void processAudio(SampleType** inputs, SampleType** outputs, Vst::Sample64 getSampleRate, int32 sampleFrames, int32 precision);

		template <typename SampleType>
		void processVuPPM(SampleType** inputs, int32 sampleFrames);

		float VuPPMconvert(float plainValue);

		Vst::Sample32 fInput;
		Vst::Sample32 fOutput;
		Vst::Sample32 fEffect;
		Vst::Sample32 fCurve;

		Vst::Sample32 fInVuPPML;
		Vst::Sample32 fInVuPPMR;
		Vst::Sample32 fOutVuPPML;
		Vst::Sample32 fOutVuPPMR;
		Vst::Sample32 fInVuPPMLOld;
		Vst::Sample32 fInVuPPMROld;
		Vst::Sample32 fOutVuPPMLOld;
		Vst::Sample32 fOutVuPPMROld;
		// int32 currentProcessMode;

		bool bBypass{ false };
		bool bClip{ false };

		uint32_t fpdL = 1;
		uint32_t fpdR = 1;
		
	};
} // namespace yg331
