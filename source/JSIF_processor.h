//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "JSIF_shared.h"
#include "JSIF_dataexchange.h"

#include "public.sdk/source/vst/vstaudioeffect.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <deque>

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

#include <CDSPResampler.h>

namespace yg331 {

static constexpr int maxTap = 512;

class Kaiser {
public:
	static inline double Ino(double x)
	{
		double d = 0, ds = 1, s = 1;
		do
		{
			d += 2;
			ds *= x * x / (d * d);
			s += ds;
		} while (ds > s * 1e-6);
		return s;
	};

	static void calcFilter(double Fs, double Fa, double Fb, int M, double Att, double* dest)
	{
		// Kaiser windowed FIR filter "DIGITAL SIGNAL PROCESSING, II" IEEE Press pp 123-126.

		int Np = (M - 1) / 2;
		double A[maxTap] = { 0, };
		double Alpha; //actually, Beta. This Alpha is multiplied by pi.
		double Inoalpha;

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
		{
			dest[Np + j] = A[j] * Ino(Alpha * std::sqrt(1.0 - ((double)(j * j) / (double)(Np * Np)))) / Inoalpha;
		}
		for (int j = 0; j < Np; j++)
		{
			dest[j] = dest[M - 1 - j];
		}

	};
};

// Buffers ------------------------------------------------------------------
typedef struct _Flt {
	double coef alignas(16)[maxTap] = { 0, };
	double buff alignas(16)[maxTap] = { 0, };
    int TAP_SIZE = 0;
    int TAP_HALF = 0;
    int TAP_HALF_HALF = 0;
    int TAP_CONDITON = 0;
} Flt;

class Decibels
{
public:
	template <typename Type>
	static Type decibelsToGain(
		Type decibels,
		Type minusInfinityDb = Type(defaultMinusInfinitydB)
	)
	{
		return decibels > minusInfinityDb 
		                ? std::pow(Type(10.0), decibels * Type(0.05))
		                : Type();
	}

	template <typename Type>
	static Type gainToDecibels(
		Type gain,
		Type minusInfinityDb = Type(defaultMinusInfinitydB)
	)
	{
		return gain > Type() 
		            ? (std::max)(minusInfinityDb, static_cast<Type> (std::log10(gain)) * Type(20.0))
		            : minusInfinityDb;
	}

private:
	enum { defaultMinusInfinitydB = -100 };
	Decibels() = delete; 
};

class LevelEnvelopeFollower
{
public:
	LevelEnvelopeFollower() = default;

	~LevelEnvelopeFollower() {
		state.clear();
		state.shrink_to_fit();
	}

	void setChannel(const int channels) {
		state.resize(channels, 0.0);
	}

	enum detectionType {Peak, RMS};
	void setType(detectionType _type)
	{
		type = _type;
	}

	void setDecay(double val)
	{
		DecayInSeconds = val;
	};

	void prepare(const double& fs)
	{
		sampleRate = fs;

		double attackTimeInSeconds = 0.1 * DecayInSeconds;
		alphaAttack = exp(-1.0 / (sampleRate * attackTimeInSeconds)); 

		double releaseTimeInSeconds = DecayInSeconds; 
		alphaRelease = exp(-1.0 / (sampleRate * releaseTimeInSeconds));  

		for (auto& s : state)
			s = Decibels::gainToDecibels(0.0);
	};

	template <typename SampleType>
	void update(SampleType** channelData, int numChannels, int numSamples)
	{
		if (numChannels <= 0) return;
		if (numSamples <= 0) return;
		if (numChannels > state.size()) return;

		for (int ch = 0; ch < numChannels; ch++) {
			for (int i = 0; i < numSamples; i++) {
				if (type == Peak) {
					double in = Decibels::gainToDecibels(std::abs(channelData[ch][i]));
					if (in > state[ch])
						state[ch] = alphaAttack * state[ch] + (1.0 - alphaAttack) * in;
					else
						state[ch] = alphaRelease * state[ch] + (1.0 - alphaRelease) * in;
				}
				else {
					double pwr = Decibels::gainToDecibels(std::abs(channelData[ch][i]) * std::abs(channelData[ch][i]));
					state[ch] = alphaRelease * state[ch] + (1.0 - alphaRelease) * pwr;
				}
				
			}
		} 
	};
	
	double getEnv(int channel) {
		if (channel < 0) return 0.0;
		if (channel >= state.size()) return 0.0;

		if (type == Peak) return Decibels::decibelsToGain(state[channel]);
		else return std::sqrt(Decibels::decibelsToGain(state[channel]));
	};

private:
	double sampleRate = 0.0;

	double DecayInSeconds = 0.5;
	double DecayCoef = 0.99992;

	detectionType type = Peak;

	std::vector<double> state;
	double alphaAttack = 0.0;
	double alphaRelease = 0.0;
};

//------------------------------------------------------------------------
static constexpr Steinberg::Vst::DataExchangeBlock
	InvalidDataExchangeBlock = { nullptr, 0, Steinberg::Vst::InvalidDataExchangeBlockID };

//------------------------------------------------------------------------
//  JSIF_Processor
//------------------------------------------------------------------------
class JSIF_Processor : public Steinberg::Vst::AudioEffect
{
public:
	JSIF_Processor();
	~JSIF_Processor() SMTG_OVERRIDE;

	// Create function
	static Steinberg::FUnknown* createInstance(void* /*context*/)
	{
		return (Steinberg::Vst::IAudioProcessor*)new JSIF_Processor;
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;

	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	/** Try to set (host => plug-in) a wanted arrangement for inputs and outputs. */
	Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, 
	                                                 Steinberg::int32 numIns,
	                                                 Steinberg::Vst::SpeakerArrangement* outputs, 
	                                                 Steinberg::int32 numOuts) SMTG_OVERRIDE;


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

	//------------------------------------------------------------------------
	// IConnectionPoint overrides:
	//------------------------------------------------------------------------
	/** Connects this instance with another connection point. */
	Steinberg::tresult PLUGIN_API connect(Steinberg::Vst::IConnectionPoint* other) SMTG_OVERRIDE;

	/** Disconnects a given connection point from this. */
	Steinberg::tresult PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint* other) SMTG_OVERRIDE;

	/** Called when a message has been sent from the connection point to this. */
	//Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

	//==============================================================================

protected:

	float VuPPMconvert(float plainValue);

	template <typename SampleType>
	void processVuPPM_In(SampleType** inputs, Steinberg::int32 numChannels, Steinberg::int32 sampleFrames);

	template <typename SampleType>
	void processAudio(SampleType** inputs, SampleType** outputs, Steinberg::int32 numChannels, Steinberg::Vst::SampleRate getSampleRate, long long sampleFrames);
	template <typename SampleType>
	void latencyBypass(SampleType** inputs, SampleType** outputs, Steinberg::int32 numChannels, Steinberg::Vst::SampleRate getSampleRate, long long sampleFrames);

	Steinberg::Vst::Sample64 process_inflator(Steinberg::Vst::Sample64 inputSample);
    
    void notSetupProcessing(Steinberg::Vst::ProcessSetup& newSetup)
    {
        Steinberg::Vst::SpeakerArrangement arr;
        getBusArrangement(Steinberg::Vst::BusDirections::kInput, 0, arr);
        numChannels = static_cast<uint16_t> (Steinberg::Vst::SpeakerArr::getChannelCount(arr));

        for (Steinberg::int32 channel = 0; channel < numChannels; channel++)
            Band_Split_set(&Band_Split[channel], 240.0, 2400.0, newSetup.sampleRate);

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
        buff[0].resize(newSetup.maxSamplesPerBlock, 0.0);
        buff[1].resize(newSetup.maxSamplesPerBlock, 0.0);
        
        setupProcessing_checked = true;

        //FDebugPrint("[ FDebugPrint ] setupProcessing\n");
    };
    Steinberg::TBool setupProcessing_checked = false;

	inline void Band_Split_set(Band_Split_t* filter, Steinberg::Vst::ParamValue Fc_L, Steinberg::Vst::ParamValue Fc_H, Steinberg::Vst::SampleRate Fs) {
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

	std::deque<double> latency_q[2];

	using ParamValue = Steinberg::Vst::ParamValue;
	using Sample64   = Steinberg::Vst::Sample64;
	
	// Plugin controls ------------------------------------------------------------------
	ParamValue fInput;
	ParamValue fOutput;
	ParamValue fEffect;
	ParamValue fCurve;
	ParamValue fParamZoom;
	ParamValue fParamPhase;

	bool            bBypass;
	bool            bIn;
	bool            bClip;
	bool            bSplit;

	// Internal values --------------------------------------------------------------
	Sample64   curvepct;
	Sample64   curveA;
	Sample64   curveB;
	Sample64   curveC;
	Sample64   curveD;
	Band_Split_t    Band_Split[2];
	overSample      fParamOS, fParamOSOld;

	// VU metering ----------------------------------------------------------------
	LevelEnvelopeFollower VuInput, VuOutput;

	static constexpr double init_meter = -std::numeric_limits<double>::infinity();
	ParamValue Meter = init_meter;
	std::vector<ParamValue> buff[2];
	std::vector<ParamValue> fInputVu;
	std::vector<ParamValue> fOutputVu;
	ParamValue fMeterVu;

	// Oversamplers ------------------------------------------------------------------
	r8b::CDSPResampler24	upSample_2x_Lin[2];
	r8b::CDSPResampler24	upSample_4x_Lin[2];
	r8b::CDSPResampler24	upSample_8x_Lin[2];
	r8b::CDSPResampler24	dnSample_2x_Lin[2];
	r8b::CDSPResampler24	dnSample_4x_Lin[2];
	r8b::CDSPResampler24	dnSample_8x_Lin[2];

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

	static constexpr Steinberg::int32 upTap_21 = 85;
	static constexpr Steinberg::int32 upTap_41 = 83;
	static constexpr Steinberg::int32 upTap_42 = 31;
	static constexpr Steinberg::int32 upTap_81 = 87;
	static constexpr Steinberg::int32 upTap_82 = 33;
	static constexpr Steinberg::int32 upTap_83 = 21;

	static constexpr Steinberg::int32 dnTap_21 = 113;
	static constexpr Steinberg::int32 dnTap_41 = 113;
	static constexpr Steinberg::int32 dnTap_42 = 31;
	static constexpr Steinberg::int32 dnTap_81 = 113;
	static constexpr Steinberg::int32 dnTap_82 = 33;
	static constexpr Steinberg::int32 dnTap_83 = 21;

	void Fir_x2_up(Sample64* in, Sample64* out, Steinberg::int32 channel);
	void Fir_x2_dn(Sample64* in, Sample64* out, Steinberg::int32 channel);
	void Fir_x4_up(Sample64* in, Sample64* out, Steinberg::int32 channel);
	void Fir_x4_dn(Sample64* in, Sample64* out, Steinberg::int32 channel);
	void Fir_x8_up(Sample64* in, Sample64* out, Steinberg::int32 channel);
	void Fir_x8_dn(Sample64* in, Sample64* out, Steinberg::int32 channel);

	static constexpr Steinberg::int32 latency_r8b_x2 = 3388;
	static constexpr Steinberg::int32 latency_r8b_x4 = 3431;
	static constexpr Steinberg::int32 latency_r8b_x8 = 3465;
	static constexpr Steinberg::int32 latency_Fir_x2 = 49;
	static constexpr Steinberg::int32 latency_Fir_x4 = 56;
	static constexpr Steinberg::int32 latency_Fir_x8 = 60;

	// data exchange
	void acquireNewExchangeBlock();
	std::unique_ptr<Steinberg::Vst::DataExchangeHandler> dataExchange;
	Steinberg::Vst::DataExchangeBlock currentExchangeBlock{ InvalidDataExchangeBlock };
	uint16_t numChannels{ 0 };
};
//------------------------------------------------------------------------
} // namespace yg331
