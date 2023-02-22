//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

#include "param.h"

// return larger of _Left and _Right
template <class _Ty>
_NODISCARD _Post_equal_to_(_Left < _Right ? _Right : _Left) constexpr const _Ty& (max)(const _Ty& _Left, const _Ty& _Right) noexcept(noexcept(_Left < _Right)) /* strengthened */ {return _Left < _Right ? _Right : _Left;}

// return smaller of _Left and _Right
template <class _Ty>
_NODISCARD _Post_equal_to_(_Right < _Left ? _Right : _Left) constexpr const _Ty& (min)(const _Ty& _Left, const _Ty& _Right) noexcept(noexcept(_Right < _Left)) /* strengthened */ {return _Right < _Left ? _Right : _Left;}

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
		template <typename SampleType, typename num>
		bool processAudio(SampleType** input, SampleType** output, num numChannels, num sampleFrames);

		template <typename SampleType>
		SampleType processInVuPPM(SampleType** input, int32 ch, int32 sampleFrames);

		template <typename SampleType>
		SampleType processOutVuPPM(SampleType** output, int32 ch, int32 sampleFrames);

		float InflatorPackageProcessor::VuPPMconvert(float plainValue);

		float fInput  = 0.5; 
		float fOutput = 1.0;
		float fEffect = 1.0;
		float fCurve  = 0.5;

		float fInVuPPMLOld = 0.0;
		float fInVuPPMROld = 0.0;
		float fOutVuPPMLOld = 0.0;
		float fOutVuPPMROld = 0.0;
		// int32 currentProcessMode;

		bool bHalfGain{ false };
		bool bBypass{ false };
		bool bClip{ false };

		float fIn_db  = expf(logf(10.f) * (24.0 * fInput - 12.0) / 20.f);
		float fOut_db = expf(logf(10.f) * (12.0 * fOutput - 12.0) / 20.f);

		float wet = fEffect;
		float dry = 1.0 - wet;
		float dry2 = dry * 2.0;

		float curvepct = fCurve;
		float curveA = 1.5 + curvepct;			// 1 + (curve + 50) / 100
		float curveB = -(curvepct + curvepct);	// - curve / 50
		float curveC = curvepct - 0.5;			// (curve - 50) / 100
		float curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;	// 1 / 16 - curve / 400 + curve ^ 2 / (4 * 10 ^ 4)
	};
} // namespace yg331
