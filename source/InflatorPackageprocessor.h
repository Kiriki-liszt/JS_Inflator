//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
// GPL v3
//------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------
// VST3 plugin Start
//------------------------------------------------------------------------
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "InflatorPackagecids.h"

#include <CDSPResampler.h>
#include <math.h>

#include <queue>

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

		/** Gets the current Latency in samples. */
		Steinberg::uint32 PLUGIN_API getLatencySamples() SMTG_OVERRIDE;

		/** Asks if a given sample size is supported see SymbolicSampleSizes. */
		Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

		/** Here we go...the process call */
		Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

		/** For persistence */
		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

		//==============================================================================

	protected:

		float VuPPMconvert(float plainValue);

		template <typename SampleType>
		void processVuPPM_In(SampleType** inputs, Steinberg::int32 numChannels, int32 sampleFrames);

		template <typename SampleType>
		void processAudio(SampleType** inputs, SampleType** outputs, Steinberg::int32 numChannels, Vst::SampleRate getSampleRate, long long sampleFrames);
		template <typename SampleType>
		void latencyBypass(SampleType** inputs, SampleType** outputs, Steinberg::int32 numChannels, Vst::SampleRate getSampleRate, long long sampleFrames);

		Vst::Sample64 process_inflator(Vst::Sample64 inputSample);

		overSample convert_to_OS(Steinberg::Vst::ParamValue value) {
			value *= overSample_num;
			if (value < overSample_2x) return overSample_1x;
			else if (value < overSample_4x) return overSample_2x;
			else if (value < overSample_8x) return overSample_4x;
			else                            return overSample_8x;
		};
		Steinberg::Vst::ParamValue convert_from_OS(overSample OS) {
			if (OS == overSample_1x)   return (Steinberg::Vst::ParamValue)overSample_1x / overSample_num;
			else if (OS == overSample_2x)   return (Steinberg::Vst::ParamValue)overSample_2x / overSample_num;
			else if (OS == overSample_4x)   return (Steinberg::Vst::ParamValue)overSample_4x / overSample_num;
			else                            return (Steinberg::Vst::ParamValue)overSample_8x / overSample_num;
		};

		inline void Band_Split_set(Band_Split* filter, Vst::ParamValue Fc_L, Vst::ParamValue Fc_H, Vst::SampleRate Fs) {
#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
			(*filter).LP.C = 0.5 * tan(M_PI * ((Fc_L / Fs) - 0.25)) + 0.5;
			(*filter).LP.R = 0.0;
			(*filter).LP.I = 0.0;
			(*filter).HP.C = 0.5 * tan(M_PI * ((Fc_H / Fs) - 0.25)) + 0.5;
			(*filter).HP.R = 0.0;
			(*filter).HP.I = 0.0;
			(*filter).G = (*filter).HP.C * (1 - (*filter).LP.C) / ((*filter).HP.C - (*filter).LP.C);
			(*filter).GR = 1 / (*filter).G;
			return;
		};

#define maxTap 128
#define halfTap 64

		inline double Ino(double x)
		{
			double d = 0, ds = 1, s = 1;
			do
			{
				d += 2;
				ds *= x * x / (d * d);
				s += ds;
			} while (ds > s * 1e-6);
			return s;
		}

		void calcFilter(double Fs, double Fa, double Fb, int M, double Att, double* dest)
		{
			// Kaiser windowed FIR filter "DIGITAL SIGNAL PROCESSING, II" IEEE Press pp 123-126.

			int Np = (M - 1) / 2;
			double A[maxTap] = { 0, };
			double Alpha;
			double Inoalpha;
			//double H[maxTap] = { 0, };

			A[0] = 2 * (Fb - Fa) / Fs;

			for (int j = 1; j <= Np; j++)
				A[j] = (sin(2.0 * j * M_PI * Fb / Fs) - sin(2.0 * j * M_PI * Fa / Fs)) / (j * M_PI);

			if (Att < 21.0)
				Alpha = 0;
			else if (Att > 50.0)
				Alpha = 0.1102 * (Att - 8.7);
			else
				Alpha = 0.5842 * pow((Att - 21), 0.4) + 0.07886 * (Att - 21);

			Inoalpha = Ino(Alpha);

			for (int j = 0; j <= Np; j++)
				dest[Np + j] = A[j] * Ino(Alpha * sqrt(1.0 - ((double)(j * j) / (double)(Np * Np)))) / Inoalpha;

			for (int j = 0; j < Np; j++)
				dest[j] = dest[M - 1 - j];
		}

		std::queue<double> latency_q[2];
		
		// Plugin controls ------------------------------------------------------------------
		Vst::ParamValue fInput;
		Vst::ParamValue fOutput;
		Vst::ParamValue fEffect;
		Vst::ParamValue fCurve;
		Vst::ParamValue fParamZoom;
		Vst::ParamValue fParamPhase;

		bool            bBypass;
		bool            bIn;
		bool            bClip;
		bool            bSplit;


		// Internal values --------------------------------------------------------------
		Vst::Sample64   curvepct;
		Vst::Sample64   curveA;
		Vst::Sample64   curveB;
		Vst::Sample64   curveC;
		Vst::Sample64   curveD;
		Band_Split      Band_Split[2];
		overSample      fParamOS, fParamOSOld;

		// VU metering ----------------------------------------------------------------
		Vst::ParamValue fInVuPPML, fInVuPPMR;
		Vst::ParamValue fOutVuPPML, fOutVuPPMR;
		Vst::ParamValue fInVuPPMLOld, fInVuPPMROld;
		Vst::ParamValue fOutVuPPMLOld, fOutVuPPMROld;
		Vst::ParamValue fMeter, fMeterOld;
		Vst::ParamValue Meter = 0.0;

		// Oversamplers ------------------------------------------------------------------
		r8b::CDSPResampler24	upSample_2x_Lin[2];
		r8b::CDSPResampler24	upSample_4x_Lin[2];
		r8b::CDSPResampler24	upSample_8x_Lin[2];
		r8b::CDSPResampler24	dnSample_2x_Lin[2];
		r8b::CDSPResampler24	dnSample_4x_Lin[2];
		r8b::CDSPResampler24	dnSample_8x_Lin[2];

		typedef struct _Flt {
			double alignas(16) coef[maxTap] = { 0, };
			double alignas(16) buff[maxTap] = { 0, };
		} Flt;

		Flt upSample_21[2];
		Flt upSample_41[2];
		Flt upSample_42[2];
		Flt upSample_81[2];
		Flt upSample_82[2];
		Flt upSample_83[2];

		Flt dnSample_21[2];
		Flt dnSample_41[2];
		Flt dnSample_42[2];
		Flt dnSample_81[2];
		Flt dnSample_82[2];
		Flt dnSample_83[2];		

		const int32 upTap_21 = 85;
		const int32 upTap_41 = 83;
		const int32 upTap_42 = 31;
		const int32 upTap_81 = 87;
		const int32 upTap_82 = 33;
		const int32 upTap_83 = 21;

		const int32 dnTap_21 = 113;
		const int32 dnTap_41 = 113;
		const int32 dnTap_42 = 31;
		const int32 dnTap_81 = 113;
		const int32 dnTap_82 = 33;
		const int32 dnTap_83 = 21;

		void Fir_x2_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel);
		void Fir_x2_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel);
		void Fir_x4_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel);
		void Fir_x4_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel);
		void Fir_x8_up(Vst::Sample64* in, Vst::Sample64* out, int32 channel);
		void Fir_x8_dn(Vst::Sample64* in, Vst::Sample64* out, int32 channel);

		const int32 latency_r8b_x2 = 3388;
		const int32 latency_r8b_x4 = 3431;
		const int32 latency_r8b_x8 = 3465;
		const int32 latency_Fir_x2 = 49;
		const int32 latency_Fir_x4 = 56;
		const int32 latency_Fir_x8 = 60;
	};
} // namespace yg331
//------------------------------------------------------------------------
// VST3 plugin End
//------------------------------------------------------------------------