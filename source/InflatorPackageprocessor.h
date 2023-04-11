//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once


//------------------------------------------------------------------------
// HIIR library Start
//------------------------------------------------------------------------
#include <array>
#include <cassert>
#include <cstdio>

namespace hiir {

	constexpr int coef_2x_1_num = 10;
	constexpr int coef_4x_1_num = 4;
	constexpr int coef_4x_2_num = 10;
	constexpr int coef_8x_1_num = 3;
	constexpr int coef_8x_2_num = 4;
	constexpr int coef_8x_3_num = 10;

	const unsigned int os_1x_latency = 0;
	const unsigned int os_2x_latency = 4;
	const unsigned int os_4x_latency = 5;
	const unsigned int os_8x_latency = 6;

	/*
		2×, 125 dB
		¯¯¯¯¯¯¯¯¯¯
		Total delay : 4.0 spl
		Bandwidth   : 20213 Hz
		Coefficients: 10

		4×, 120 dB
		¯¯¯¯¯¯¯¯¯¯
		Total delay : 5.0 spl
		Bandwidth : 20428 Hz

		8×, 129 dB
		¯¯¯¯¯¯¯¯¯¯
		Total delay : 6.0 spl
		Bandwidth   : 19006 Hz
	*/
	const double coef_2x_1[coef_2x_1_num] = {
		0.026280277370383145,
		0.099994562200117765,
		0.20785425737827937,
		0.33334081139102473,
		0.46167004060691091,
		0.58273462309510859,
		0.69172302956824328,
		0.78828933879250873,
		0.87532862123185262,
		0.9580617608216595
	};

	const double coef_4x_1[coef_4x_1_num] = {
		0.041180778598107023,
		0.1665604775598164,
		0.38702422374344198,
		0.74155297339931314
	};
	const double coef_4x_2[coef_4x_2_num] = {
		0.028143361249169534,
		0.10666337918578024,
		0.22039215120527197,
		0.35084569997865528,
		0.48197792985533633,
		0.60331147102003924,
		0.7102921937907698,
		0.80307423332343497,
		0.88500411159151648,
		0.96155188130366132
	};

	const double coef_8x_1[coef_8x_1_num] = {
		0.055006689941721712,
		0.24080823797621789,
		0.64456305579326278
	};
	const double coef_8x_2[coef_8x_2_num] = {
		0.039007192037884615,
		0.15936900049878461,
		0.37605311415523662,
		0.7341922816405595
	};
	const double coef_8x_3[coef_8x_3_num] = {
		0.020259561571818407,
		0.07811926635621437,
		0.165767029808395,
		0.27285653675862664,
		0.38920012889842326,
		0.50682082231815651,
		0.62096457947279504,
		0.73023836303677192,
		0.83631543394763719,
		0.94366551176113678
	};

	template <typename DT>
	class StageDataTpl {
	public:
		DT	_coef{ 0.f };  // a_{n-2}
		DT	_mem{ 0.f };  // y of the stage
	};

	// Template parameters:
	// REMAINING: Number of remaining coefficients to process, >= 0
	//        DT: Data type (float or double)
	template <int REMAINING, typename DT>
	class StageProcTpl {

		static_assert ((REMAINING >= 0), "REMAINING must be >= 0");

	public:
		static inline void process_sample_pos(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;
		static inline void process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;

	protected:

	private:

	private:
		StageProcTpl() = delete;
		StageProcTpl(const StageProcTpl <REMAINING, DT>& other) = delete;
		StageProcTpl(StageProcTpl <REMAINING, DT>&& other) = delete;
		~StageProcTpl() = delete;
		StageProcTpl <REMAINING, DT>& operator = (const StageProcTpl <REMAINING, DT>& other) = delete;
		StageProcTpl <REMAINING, DT>& operator = (StageProcTpl <REMAINING, DT>&& other) = delete;
		bool operator == (const StageProcTpl <REMAINING, DT>& other) = delete;
		bool operator != (const StageProcTpl <REMAINING, DT>& other) = delete;
	};

	template <typename DT>
	class StageProcTpl <0, DT> {
	public:
		static inline void process_sample_pos(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;
		static inline void process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;

	private:
		StageProcTpl() = delete;
		StageProcTpl(const StageProcTpl <0, DT>& other) = delete;
		StageProcTpl(StageProcTpl <0, DT>&& other) = delete;
		~StageProcTpl() = delete;
		StageProcTpl <0, DT>& operator = (const StageProcTpl <0, DT>& other) = delete;
		StageProcTpl <0, DT>& operator = (StageProcTpl <0, DT>&& other) = delete;
		bool operator == (const StageProcTpl <0, DT>& other) = delete;
		bool operator != (const StageProcTpl <0, DT>& other) = delete;
	};

	template <typename DT>
	class StageProcTpl <1, DT> {
	public:
		static inline void process_sample_pos(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;
		static inline void process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept;

	private:
		StageProcTpl() = delete;
		StageProcTpl(const StageProcTpl <1, DT>& other) = delete;
		StageProcTpl(StageProcTpl <1, DT>&& other) = delete;
		~StageProcTpl() = delete;
		StageProcTpl <1, DT>& operator = (const StageProcTpl <1, DT>& other) = delete;
		StageProcTpl <1, DT>& operator = (StageProcTpl <1, DT>&& other) = delete;
		bool operator == (const StageProcTpl <1, DT>& other) = delete;
		bool operator != (const StageProcTpl <1, DT>& other) = delete;
	};

	template <int REMAINING, typename DT>
	void StageProcTpl <REMAINING, DT>::process_sample_pos(
		const int nbr_coefs,
		DT& spl_0,
		DT& spl_1,
		StageDataTpl <DT>* stage_arr
	) noexcept
	{
		const int      cnt = nbr_coefs + 2 - REMAINING;

		DT             tmp_0 = spl_0;
		tmp_0 -= stage_arr[cnt]._mem;
		tmp_0 *= stage_arr[cnt]._coef;
		tmp_0 += stage_arr[cnt - 2]._mem;

		DT             tmp_1 = spl_1;
		tmp_1 -= stage_arr[cnt + 1]._mem;
		tmp_1 *= stage_arr[cnt + 1]._coef;
		tmp_1 += stage_arr[cnt - 1]._mem;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;

		spl_0 = tmp_0;
		spl_1 = tmp_1;

		StageProcTpl <REMAINING - 2, DT>::process_sample_pos(
			nbr_coefs,
			spl_0,
			spl_1,
			stage_arr
		);
	}

	template <typename DT>
	void StageProcTpl <1, DT>::process_sample_pos(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept
	{
		const int      cnt = nbr_coefs + 2 - 1;

		DT             tmp_0 = spl_0;
		tmp_0 -= stage_arr[cnt]._mem;
		tmp_0 *= stage_arr[cnt]._coef;
		tmp_0 += stage_arr[cnt - 2]._mem;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;
		stage_arr[cnt]._mem = tmp_0;

		spl_0 = tmp_0;
	}

	template <typename DT>
	void StageProcTpl <0, DT>::process_sample_pos(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept
	{
		const int      cnt = nbr_coefs + 2;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;
	}

	template <int REMAINING, typename DT>
	void StageProcTpl <REMAINING, DT>::process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept
	{
		const int      cnt = nbr_coefs + 2 - REMAINING;

		DT             tmp_0 = spl_0;
		tmp_0 += stage_arr[cnt]._mem;
		tmp_0 *= stage_arr[cnt]._coef;
		tmp_0 -= stage_arr[cnt - 2]._mem;

		DT             tmp_1 = spl_1;
		tmp_1 += stage_arr[cnt + 1]._mem;
		tmp_1 *= stage_arr[cnt + 1]._coef;
		tmp_1 -= stage_arr[cnt - 1]._mem;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;

		spl_0 = tmp_0;
		spl_1 = tmp_1;

		StageProcTpl <REMAINING - 2, DT>::process_sample_neg(
			nbr_coefs,
			spl_0,
			spl_1,
			stage_arr
		);
	}

	template <typename DT>
	void StageProcTpl <1, DT>::process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept
	{
		const int      cnt = nbr_coefs + 2 - 1;

		DT             tmp_0 = spl_0;
		tmp_0 += stage_arr[cnt]._mem;
		tmp_0 *= stage_arr[cnt]._coef;
		tmp_0 -= stage_arr[cnt - 2]._mem;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;
		stage_arr[cnt]._mem = tmp_0;

		spl_0 = tmp_0;
	}

	template <typename DT>
	void StageProcTpl <0, DT>::process_sample_neg(const int nbr_coefs, DT& spl_0, DT& spl_1, StageDataTpl <DT>* stage_arr) noexcept
	{
		const int      cnt = nbr_coefs + 2;

		stage_arr[cnt - 2]._mem = spl_0;
		stage_arr[cnt - 1]._mem = spl_1;
	}


	/*
	Template parameters:

	- NC: number of coefficients, > 0

	- DT: data type. Requires:
		DT::DT ();
		DT::DT (float);
		DT::DT (int);
		DT::DT (const DT &)
		DT & DT::operator = (const DT &);
		DT & DT::operator += (const DT &);
		DT & DT::operator -= (const DT &);
		DT & DT::operator *= (const DT &);
		DT operator + (DT, const DT &);
		DT operator - (DT, const DT &);
		DT operator * (DT, const DT &);

	- NCHN: number of contained scalars, if DT is a vector type.
	*/
	template <int NC, typename DT, int NCHN>
	class Upsampler2xTpl {

		static_assert ((NC > 0), "Number of coefficient must be positive.");
		static_assert ((NCHN > 0), "Number of channels must be positive.");

	public:
		typedef DT DataType;
		static constexpr int _nbr_chn = NCHN;
		static constexpr int NBR_COEFS = NC;
		static constexpr double _delay = 0;

		void		set_coefs(const double coef_arr[NBR_COEFS]) noexcept;
		inline void	process_sample(DataType& out_0, DataType& out_1, DataType input) noexcept;
		void		process_block(DataType out_ptr[], const DataType in_ptr[], long nbr_spl) noexcept;
		void		clear_buffers() noexcept;

	protected:

	private:
		// Stages 0 and 1 contain only input memories
		typedef std::array <StageDataTpl <DataType>, NBR_COEFS + 2> Filter;
		Filter		_filter;

		/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
	private:
		bool		operator == (const Upsampler2xTpl <NC, DT, NCHN>& other) = delete;
		bool		operator != (const Upsampler2xTpl <NC, DT, NCHN>& other) = delete;
	};

	/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
	template <int NC, typename DT, int NCHN>
	constexpr int 	Upsampler2xTpl <NC, DT, NCHN>::_nbr_chn;
	template <int NC, typename DT, int NCHN>
	constexpr int 	Upsampler2xTpl <NC, DT, NCHN>::NBR_COEFS;

	/* ==============================================================================
	Name: set_coefs
	Description:
		Sets filter coefficients.
		Generate them with the PolyphaseIir2Designer class.
		Call this function before doing any processing.
	Input parameters:
		- coef_arr:
			Array of coefficients.
			There should be as many coefficients as mentioned in the class template parameter.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Upsampler2xTpl <NC, DT, NCHN>::set_coefs(const double coef_arr[NBR_COEFS]) noexcept
	{
		assert(coef_arr != nullptr);
		for (int i = 0; i < NBR_COEFS; ++i) {
			_filter[i + 2]._coef = DataType(coef_arr[i]);
		}
	}

	/* ==============================================================================
	Name: process_sample
	Description:
		Upsamples (x2) the input sample, generating two output samples.
	Input parameters:
		- input: The input sample.
	Output parameters:
		- out_0: First output sample.
		- out_1: Second output sample.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Upsampler2xTpl <NC, DT, NCHN>::process_sample(DataType& out_0, DataType& out_1, DataType input) noexcept
	{
		DataType       even = input;
		DataType       odd = input;
		StageProcTpl <NBR_COEFS, DataType>::process_sample_pos(
			NBR_COEFS,
			even,
			odd,
			_filter.data()
		);
		out_0 = even;
		out_1 = odd;
	}

	/* ==============================================================================
	Name: process_block
	Description:
		Upsamples (x2) the input sample block.
		Input and output blocks may overlap, see assert() for details.
	Input parameters:
		- in_ptr: Input array, containing nbr_spl samples.
		- nbr_spl: Number of input samples to process, > 0
	Output parameters:
		- out_0_ptr: Output sample array, capacity: nbr_spl * 2 samples.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Upsampler2xTpl <NC, DT, NCHN>::process_block(DataType out_ptr[], const DataType in_ptr[], long nbr_spl) noexcept
	{
		assert(out_ptr != nullptr);
		assert(in_ptr != nullptr);
		assert(out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
		assert(nbr_spl > 0);

		long           pos = 0;
		do
		{
			process_sample(
				out_ptr[pos * 2],
				out_ptr[pos * 2 + 1],
				in_ptr[pos]
			);
			++pos;
		} while (pos < nbr_spl);
	}

	/* ==============================================================================
	Name: clear_buffers
	Description:
		Clears filter memory, as if it processed silence since an infinite amount of time.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Upsampler2xTpl <NC, DT, NCHN>::clear_buffers() noexcept
	{
		for (int i = 0; i < NBR_COEFS + 2; ++i) {
			_filter[i]._mem = DataType(0.f);
		}
	}




	template <int NC, typename DT, int NCHN>
	class Downsampler2xTpl
	{
		static_assert ((NC > 0), "Number of coefficient must be positive.");
		static_assert ((NCHN > 0), "Number of channels must be positive.");

	public:
		typedef DT DataType;
		static constexpr int _nbr_chn = NCHN;
		static constexpr int NBR_COEFS = NC;
		static constexpr double _delay = -1;

		void           set_coefs(const double coef_arr[]) noexcept;

		inline DataType process_sample(const DataType in_ptr[2]) noexcept;
		void           process_block(DataType out_ptr[], const DataType in_ptr[], long nbr_spl) noexcept;

		void           clear_buffers() noexcept;

	protected:

	private:
		// Stages 0 and 1 contain only input memories
		typedef std::array <StageDataTpl <DataType>, NBR_COEFS + 2> Filter;
		Filter         _filter;

		/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
	private:
		bool           operator == (const Downsampler2xTpl <NC, DT, NCHN>& other) = delete;
		bool           operator != (const Downsampler2xTpl <NC, DT, NCHN>& other) = delete;
	};


	template <int NC, typename DT, int NCHN>
	constexpr int 	Downsampler2xTpl <NC, DT, NCHN>::_nbr_chn;
	template <int NC, typename DT, int NCHN>
	constexpr int 	Downsampler2xTpl <NC, DT, NCHN>::NBR_COEFS;
	template <int NC, typename DT, int NCHN>
	constexpr double	Downsampler2xTpl <NC, DT, NCHN>::_delay;

	/* ==============================================================================
	Name: set_coefs
	Description:
		Sets filter coefficients. Generate them with the PolyphaseIir2Designer class.
		Call this function before doing any processing.
	Input parameters:
		- coef_arr: Array of coefficients. There should be as many coefficients as
			mentioned in the class template parameter.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Downsampler2xTpl <NC, DT, NCHN>::set_coefs(const double coef_arr[]) noexcept
	{
		assert(coef_arr != nullptr);
		for (int i = 0; i < NBR_COEFS; ++i) {
			_filter[i + 2]._coef = DataType(coef_arr[i]);
		}
	}

	/* ==============================================================================
	Name: process_sample
	Description:
		Downsamples (x2) one pair of samples, to generate one output sample.
	Input parameters:
		- in_ptr: pointer on the two samples to decimate
	Returns: Samplerate-reduced sample.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	typename Downsampler2xTpl <NC, DT, NCHN>::DataType	Downsampler2xTpl <NC, DT, NCHN>::process_sample(const DataType in_ptr[2]) noexcept
	{
		assert(in_ptr != nullptr);

		DataType       spl_0(in_ptr[1]);
		DataType       spl_1(in_ptr[0]);

		StageProcTpl <NBR_COEFS, DataType>::process_sample_pos(
			NBR_COEFS, spl_0, spl_1, _filter.data()
		);

		return DataType(0.5f) * (spl_0 + spl_1);
	}

	/* ==============================================================================
	Name: process_block
	Description:
		Downsamples (x2) a block of samples.
		Input and output blocks may overlap, see assert() for details.
	Input parameters:
		- in_ptr : Input array, containing nbr_spl * 2 samples.
		- nbr_spl: Number of samples to output, > 0
	Output parameters:
		- out_ptr: Array for the output samples, capacity: nbr_spl samples.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Downsampler2xTpl <NC, DT, NCHN>::process_block(DataType out_ptr[], const DataType in_ptr[], long nbr_spl) noexcept
	{
		assert(in_ptr != nullptr);
		assert(out_ptr != nullptr);
		assert(out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 2);
		assert(nbr_spl > 0);

		long           pos = 0;
		do
		{
			out_ptr[pos] = process_sample(&in_ptr[pos * 2]);
			++pos;
		} while (pos < nbr_spl);
	}

	/* ==============================================================================
	Name: clear_buffers
	Description:
		Clears filter memory, as if it processed silence since an infinite amount
		of time.
	============================================================================== */
	template <int NC, typename DT, int NCHN>
	void	Downsampler2xTpl <NC, DT, NCHN>::clear_buffers() noexcept
	{
		for (int i = 0; i < NBR_COEFS + 2; ++i)
		{
			_filter[i]._mem = DataType(0.f);
		}
	}

	template <int NC>
	using Upsampler2xFpu = Upsampler2xTpl <NC, double, 1>;

	template <int NC>
	using Downsampler2xFpu = Downsampler2xTpl <NC, double, 1>;

	typedef Upsampler2xFpu <coef_2x_1_num>  upSample_2x_1;
	typedef Upsampler2xFpu <coef_4x_1_num>  upSample_4x_1;
	typedef Upsampler2xFpu <coef_4x_2_num>  upSample_4x_2;
	typedef Upsampler2xFpu <coef_8x_1_num>  upSample_8x_1;
	typedef Upsampler2xFpu <coef_8x_2_num>  upSample_8x_2;
	typedef Upsampler2xFpu <coef_8x_3_num>  upSample_8x_3;
	typedef Downsampler2xFpu <coef_2x_1_num>  downSample_2x_1;
	typedef Downsampler2xFpu <coef_4x_1_num>  downSample_4x_1;
	typedef Downsampler2xFpu <coef_4x_2_num>  downSample_4x_2;
	typedef Downsampler2xFpu <coef_8x_1_num>  downSample_8x_1;
	typedef Downsampler2xFpu <coef_8x_2_num>  downSample_8x_2;
	typedef Downsampler2xFpu <coef_8x_3_num>  downSample_8x_3;
} 
//------------------------------------------------------------------------
// HIIR library End
//------------------------------------------------------------------------




//------------------------------------------------------------------------
// VST3 plugin Start
//------------------------------------------------------------------------
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "InflatorPackagecids.h"
#include <math.h>

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

		overSample convert_to_OS(Steinberg::Vst::ParamValue value) {
			value *= overSample_num;
			if      (value < overSample_2x) return overSample_1x;
			else if (value < overSample_4x) return overSample_2x;
			else if (value < overSample_8x) return overSample_4x;
			else                            return overSample_8x;
		};
		Steinberg::Vst::ParamValue convert_from_OS(overSample OS) {
			if      (OS == overSample_1x)   return (Steinberg::Vst::ParamValue)overSample_1x / overSample_num;
			else if (OS == overSample_2x)   return (Steinberg::Vst::ParamValue)overSample_2x / overSample_num;
			else if (OS == overSample_4x)   return (Steinberg::Vst::ParamValue)overSample_4x / overSample_num;
			else                            return (Steinberg::Vst::ParamValue)overSample_8x / overSample_num;
		};

		inline void Band_Split_set(Band_Split* filter, Vst::ParamValue Fc_L, Vst::ParamValue Fc_H, Vst::SampleRate Fs) {
			double M_PI = 3.14159265358979323846;   // pi
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

		float VuPPMconvert(float plainValue);

		template <typename SampleType>
		void proc_in(SampleType** inputs, Vst::Sample64** outputs, int32 sampleFrames);
		template <typename SampleType>
		void proc_out(Vst::Sample64** inputs, SampleType** outputs, int32 sampleFrames);

		template <typename SampleType>
		void processVuPPM_In(SampleType** inputs, int32 sampleFrames);
		template <typename SampleType>
		void processVuPPM_Out(SampleType** outputs, int32 sampleFrames);

		template <typename SampleType>
		void overSampling(SampleType** inputs, SampleType** outputs, Vst::SampleRate getSampleRate, int32 sampleFrames);
		template <typename SampleType>
		void processAudio(SampleType** inputs, SampleType** outputs, Vst::SampleRate getSampleRate, long long sampleFrames);

		Vst::Sample64 process_inflator(Vst::Sample64 inputSample);

		//------------------------------------------------------------------------
	protected:

		// int32 currentProcessMode;

		Vst::ParamValue fInput;
		Vst::ParamValue fOutput;
		Vst::ParamValue fEffect;
		Vst::ParamValue fCurve;

		Vst::ParamValue fInVuPPML;
		Vst::ParamValue fInVuPPMR;
		Vst::ParamValue fOutVuPPML;
		Vst::ParamValue fOutVuPPMR;

		Vst::ParamValue fInVuPPMLOld;
		Vst::ParamValue fInVuPPMROld;
		Vst::ParamValue fOutVuPPMLOld;
		Vst::ParamValue fOutVuPPMROld;

		Vst::ParamValue fMeter;
		Vst::ParamValue fMeterOld;
		Vst::ParamValue Meter = 0.0;

		Vst::Sample64   curvepct;
		Vst::Sample64   curveA;
		Vst::Sample64   curveB;
		Vst::Sample64   curveC;
		Vst::Sample64   curveD;

		bool            bBypass;
		bool            bClip;
		bool            bSplit;

		overSample      fParamOS;
		overSample      fParamOSOld;
		Vst::ParamValue fParamZoom;

		Band_Split      Band_Split_L;
		Band_Split      Band_Split_R;

		uint32          fpdL;
		uint32          fpdR;

		long long maxSample = 16385;

		Vst::AudioBusBuffers  in_0_64;
		Vst::AudioBusBuffers  in_1_64;
		Vst::AudioBusBuffers  in_2_64;
		Vst::AudioBusBuffers  in_3_64;
		Vst::AudioBusBuffers  out_0_64;
		Vst::AudioBusBuffers  out_1_64;
		Vst::AudioBusBuffers  out_2_64;
		Vst::AudioBusBuffers  out_3_64;

		hiir::upSample_2x_1   upSample_2x_1_L_64;
		hiir::upSample_2x_1   upSample_2x_1_R_64;
		hiir::upSample_4x_1   upSample_4x_1_L_64;
		hiir::upSample_4x_1   upSample_4x_1_R_64;
		hiir::upSample_4x_2   upSample_4x_2_L_64;
		hiir::upSample_4x_2   upSample_4x_2_R_64;
		hiir::upSample_8x_1   upSample_8x_1_L_64;
		hiir::upSample_8x_1   upSample_8x_1_R_64;
		hiir::upSample_8x_2   upSample_8x_2_L_64;
		hiir::upSample_8x_2   upSample_8x_2_R_64;
		hiir::upSample_8x_3   upSample_8x_3_L_64;
		hiir::upSample_8x_3   upSample_8x_3_R_64;

		hiir::downSample_2x_1 downSample_2x_1_L_64;
		hiir::downSample_2x_1 downSample_2x_1_R_64;
		hiir::downSample_4x_1 downSample_4x_1_L_64;
		hiir::downSample_4x_1 downSample_4x_1_R_64;
		hiir::downSample_4x_2 downSample_4x_2_L_64;
		hiir::downSample_4x_2 downSample_4x_2_R_64;
		hiir::downSample_8x_1 downSample_8x_1_L_64;
		hiir::downSample_8x_1 downSample_8x_1_R_64;
		hiir::downSample_8x_2 downSample_8x_2_L_64;
		hiir::downSample_8x_2 downSample_8x_2_R_64;
		hiir::downSample_8x_3 downSample_8x_3_L_64;
		hiir::downSample_8x_3 downSample_8x_3_R_64;
	};
} // namespace yg331
//------------------------------------------------------------------------
// VST3 plugin End
//------------------------------------------------------------------------