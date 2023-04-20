/**
 *
 * Open source (under the MIT license) high-quality professional audio sample
 * rate converter (SRC) / resampler C++ library.  Features routines for SRC,
 * both up- and downsampling, to/from any sample rate, including non-integer
 * sample rates: it can be also used for conversion to/from SACD/DSD sample
 * rates, and even go beyond that.  SRC routines were implemented in a
 * multi-platform C++ code, and have a high level of optimality. Also suitable
 * for fast general-purpose 1D time-series resampling / interpolation (with
 * relaxed filter parameters).
 *
 * For more information, please visit
 * https://github.com/avaneev/r8brain-free-src
 *
 * @section license License
 *
 * The MIT License (MIT)
 *
 * r8brain-free-src Copyright (c) 2013-2022 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Please credit the creator of this library in your documentation in the
 * following way: "Sample rate converter designed by Aleksey Vaneev of
 * Voxengo"
 *
 * @version 6.2
 */


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

 // @file r8bconf.h

#if !defined( R8BASSERT )
#define R8BASSERT( e )
#endif // !defined( R8BASSERT )

#if !defined( R8BCONSOLE )
#define R8BCONSOLE( ... )
#endif // !defined( R8BCONSOLE )

#if !defined( R8B_BASECLASS )
#define R8B_BASECLASS :: r8b :: CStdClassAllocator
#endif // !defined( R8B_BASECLASS )

#if !defined( R8B_MEMALLOCCLASS )
#define R8B_MEMALLOCCLASS :: r8b :: CStdMemAllocator
#endif // !defined( R8B_MEMALLOCCLASS )

#if !defined( R8B_FILTER_CACHE_MAX )
#define R8B_FILTER_CACHE_MAX 96
#endif // !defined( R8B_FILTER_CACHE_MAX )

#if !defined( R8B_FRACBANK_CACHE_MAX )
#define R8B_FRACBANK_CACHE_MAX 12
#endif // !defined( R8B_FRACBANK_CACHE_MAX )

#if !defined( R8B_FLTTEST )
#define R8B_FLTTEST 0
#endif // !defined( R8B_FLTTEST )

#if !defined( R8B_FASTTIMING )
#define R8B_FASTTIMING 0
#endif // !defined( R8B_FASTTIMING )

#if !defined( R8B_EXTFFT )
#define R8B_EXTFFT 0
#endif // !defined( R8B_EXTFFT )

#if !defined( R8B_IPP )
#define R8B_IPP 0
#endif // !defined( R8B_IPP )

#if !defined( R8B_PFFFT_DOUBLE )
#define R8B_PFFFT_DOUBLE 0
#endif // !defined( R8B_PFFFT_DOUBLE )

#if !defined( R8B_PFFFT )
#define R8B_PFFFT 0
#else // !defined( R8B_PFFFT )
#if R8B_PFFFT && R8B_PFFFT_DOUBLE
#error r8brain-free-src: R8B_PFFFT and R8B_PFFFT_DOUBLE collision.
#endif // R8B_PFFFT && R8B_PFFFT_DOUBLE
#endif // !defined( R8B_PFFFT )

#if R8B_PFFFT
#define R8B_FLOATFFT 1
#endif // R8B_PFFFT

#if !defined( R8B_FLOATFFT )
#define R8B_FLOATFFT 0
#endif // !defined( R8B_FLOATFFT )



#if defined( _WIN32 )
#include <windows.h>
#else // defined( _WIN32 )
#include <pthread.h>
#endif // defined( _WIN32 )

#if defined( __SSE4_2__ ) || defined( __SSE4_1__ ) || \
	defined( __SSSE3__ ) || defined( __SSE3__ ) || defined( __SSE2__ ) || \
	defined( __x86_64__ ) || defined( _M_AMD64 ) || defined( _M_X64 ) || \
	defined( __amd64 )

#include <immintrin.h>

// #define R8B_SSE2
// #define R8B_SIMD_ISH

#elif defined( __aarch64__ ) || defined( __arm64__ )

#include <arm_neon.h>

// #define R8B_NEON

#if !defined( __APPLE__ )
// #define R8B_SIMD_ISH // Shuffled interpolation is inefficient on M1.
#endif // !defined( __APPLE__ )

#endif // ARM64

namespace r8b {

#define R8B_VERSION "6.2"
#define R8B_PI 3.14159265358979324
#define R8B_2PI 6.28318530717958648
#define R8B_3PI 9.42477796076937972
#define R8B_PId2 1.57079632679489662

#define R8BNOCTOR( ClassName ) \
	private: \
		ClassName( const ClassName& ) { } \
		ClassName& operator = ( const ClassName& ) { return( *this ); }


	class CStdClassAllocator
	{
	public:
		void* operator new     (const size_t, void* const p) { return(p); }
		void* operator new     (const size_t n) { return(::malloc(n)); }
		void* operator new[](const size_t n) { return(::malloc(n)); }
		void  operator delete  (void* const p) { ::free(p); }
		void  operator delete[](void* const p) { ::free(p); }
	};
	class CStdMemAllocator : public CStdClassAllocator
	{
	public:
		static void* allocmem(const size_t Size) { return(::malloc(Size)); }
		static void* reallocmem(void* const p, const size_t Size) { return(::realloc(p, Size)); }
		static void  freemem(void* const p) { ::free(p); }
	};

	template< typename T >
	inline T* alignptr(T* const ptr, const uintptr_t align) { return((T*)(((uintptr_t)ptr + align - 1) & ~(align - 1))); }

	template< typename T >
	class CFixedBuffer : public R8B_MEMALLOCCLASS
	{
		R8BNOCTOR(CFixedBuffer);

	public:
		CFixedBuffer() : Data0(NULL), Data(NULL) {}
		CFixedBuffer(const int Capacity) { R8BASSERT(Capacity > 0 || Capacity == 0); Data0 = allocmem(Capacity * sizeof(T) + Alignment); Data = (T*)alignptr(Data0, Alignment); R8BASSERT(Data0 != NULL || Capacity == 0); }
		~CFixedBuffer() { freemem(Data0); }
		void alloc(const int Capacity) { R8BASSERT(Capacity > 0 || Capacity == 0); freemem(Data0); Data0 = allocmem(Capacity * sizeof(T) + Alignment); Data = (T*)alignptr(Data0, Alignment); R8BASSERT(Data0 != NULL || Capacity == 0); }
		void realloc(const int PrevCapacity, const int NewCapacity) { R8BASSERT(PrevCapacity >= 0); R8BASSERT(NewCapacity >= 0); void* const NewData0 = allocmem(NewCapacity * sizeof(T) + Alignment); T* const NewData = (T*)alignptr(NewData0, Alignment); const size_t CopySize = (PrevCapacity > NewCapacity ? NewCapacity : PrevCapacity) * sizeof(T); if (CopySize > 0) { memcpy(NewData, Data, CopySize); } freemem(Data0); Data0 = NewData0; Data = NewData; R8BASSERT(Data0 != NULL || NewCapacity == 0); }
		void free() { freemem(Data0); Data0 = NULL; Data = NULL; }
		T* getPtr() const { return(Data); }
		operator T* () const { return(Data); }

	private:
		static const size_t Alignment = 64;
		void* Data0;
		T* Data;
	};

	template< typename T >
	class CPtrKeeper
	{
		R8BNOCTOR(CPtrKeeper);

	public:
		CPtrKeeper() : Object(NULL) { }

		template< typename T2 >
		CPtrKeeper(T2 const aObject) : Object(aObject) { }
		~CPtrKeeper() { delete Object; }

		template< typename T2 >
		void operator = (T2 const aObject) { reset(); Object = aObject; }

		T operator -> () const { return(Object); }
		operator T () const { return(Object); }
		void reset() { T DelObj = Object; Object = NULL; delete DelObj; }
		T unkeep() { T ResObject = Object; Object = NULL; return(ResObject); }

	private:
		T Object;
	};

	class CSyncObject
	{
		R8BNOCTOR(CSyncObject);

	public:
		CSyncObject()
		{
#if defined( _WIN32 )
			InitializeCriticalSectionAndSpinCount(&CritSec, 2000);
#else // defined( _WIN32 )
			pthread_mutexattr_t MutexAttrs;
			pthread_mutexattr_init(&MutexAttrs);
			pthread_mutexattr_settype(&MutexAttrs, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&Mutex, &MutexAttrs);
			pthread_mutexattr_destroy(&MutexAttrs);
#endif // defined( _WIN32 )
		}

		~CSyncObject()
		{
#if defined( _WIN32 )
			DeleteCriticalSection(&CritSec);
#else // defined( _WIN32 )
			pthread_mutex_destroy(&Mutex);
#endif // defined( _WIN32 )
		}

		void acquire()
		{
#if defined( _WIN32 )
			EnterCriticalSection(&CritSec);
#else // defined( _WIN32 )
			pthread_mutex_lock(&Mutex);
#endif // defined( _WIN32 )
		}

		void release()
		{
#if defined( _WIN32 )
			LeaveCriticalSection(&CritSec);
#else // defined( _WIN32 )
			pthread_mutex_unlock(&Mutex);
#endif // defined( _WIN32 )
		}

	private:
#if defined( _WIN32 )
		CRITICAL_SECTION CritSec; ///< Standard Windows critical section
		///< structure.
#else // defined( _WIN32 )
		pthread_mutex_t Mutex; ///< pthread.h mutex object.
#endif // defined( _WIN32 )
	};

	class CSyncKeeper
	{
		R8BNOCTOR(CSyncKeeper);

	public:
		CSyncKeeper() : SyncObj(NULL) { }
		CSyncKeeper(CSyncObject* const aSyncObj) : SyncObj(aSyncObj) { if (SyncObj != NULL) { SyncObj->acquire(); } }
		CSyncKeeper(CSyncObject& aSyncObj) : SyncObj(&aSyncObj) { SyncObj->acquire(); }
		~CSyncKeeper() { if (SyncObj != NULL) { SyncObj->release(); } }

	private:
		CSyncObject* SyncObj;
	};


#define R8BSYNC( SyncObject ) R8BSYNC_( SyncObject, __LINE__ )
#define R8BSYNC_( SyncObject, id ) R8BSYNC__( SyncObject, id )
#define R8BSYNC__( SyncObject, id ) CSyncKeeper SyncKeeper##id( SyncObject )

	class CSineGen
	{
	public:
		CSineGen() { }
		CSineGen(const double si, const double ph) : svalue1(sin(ph)), svalue2(sin(ph - si)), sincr(2.0 * cos(si)) { }
		CSineGen(const double si, const double ph, const double g) : svalue1(sin(ph)* g), svalue2(sin(ph - si)* g), sincr(2.0 * cos(si)) { }
		void init(const double si, const double ph) { svalue1 = sin(ph); svalue2 = sin(ph - si); sincr = 2.0 * cos(si); }
		void init(const double si, const double ph, const double g) { svalue1 = sin(ph) * g; svalue2 = sin(ph - si) * g; sincr = 2.0 * cos(si); }
		double generate() { const double res = svalue1; svalue1 = sincr * res - svalue2; svalue2 = res; return(res); }

	private:
		double svalue1;
		double svalue2;
		double sincr;
	};

	inline int getBitOccupancy(const int v) { static const uint8_t OccupancyTable[] = { 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 }; const int tt = v >> 16; if (tt != 0) { const int t = v >> 24; return(t != 0 ? 24 + OccupancyTable[t & 0xFF] : 16 + OccupancyTable[tt]); } else { const int t = v >> 8; return(t != 0 ? 8 + OccupancyTable[t] : OccupancyTable[v]); } }
	inline double clampr(const double Value, const double minv, const double maxv) { if (Value < minv) { return(minv); } else { if (Value > maxv) { return(maxv); } else { return(Value); } } }
	inline double sqr(const double x) { return(x * x); }
	inline double pow_a(const double v, const double p) { return(exp(p * log(fabs(v) + 1e-300))); }
	inline double gauss(const double v) { return(exp(-(v * v))); }
	inline double asinh(const double v) { return(log(v + sqrt(v * v + 1.0))); }
	inline double besselI0(const double x) { const double ax = fabs(x); double y; if (ax < 3.75) { y = x / 3.75; y *= y; return(1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492 + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))))); } y = 3.75 / ax; return(exp(ax) / sqrt(ax) * (0.39894228 + y * (0.1328592e-1 + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2 + y * (-0.2057706e-1 + y * (0.2635537e-1 + y * (-0.1647633e-1 + y * 0.392377e-2))))))))); }

} // namespace r8b


/**
 * @file CDSPProcessor.h
 */

namespace r8b {

	class CDSPProcessor : public R8B_BASECLASS
	{
		R8BNOCTOR(CDSPProcessor);

	public:
		CDSPProcessor() { }
		virtual ~CDSPProcessor() { }

		virtual int getInLenBeforeOutPos(const int ReqOutPos) const = 0;
		virtual int getLatency() const = 0;
		virtual double getLatencyFrac() const = 0;
		virtual int getMaxOutLen(const int MaxInLen) const = 0;
		virtual void clear() = 0;
		virtual int process(double* ip, int l0, double*& op0) = 0;
	};
} // namespace r8b


/**
 * @file CDSPHBUpsampler.h
 */

namespace r8b {

	class CDSPHBUpsampler : public CDSPProcessor
	{
	public:

		static void getHBFilter(
			const double ReqAtten,
			const int SteepIndex,
			const double*& flt,
			int& fltt,
			double& att
		)
		{

			static const double HBKernel_4A[4] = { 6.1729335650971517e-001, -1.5963945620743250e-001, 5.5073370934086312e-002, -1.4603578989932850e-002, };
			static const double HBKernel_5A[5] = { 6.2068807424902472e-001, -1.6827573634467302e-001, 6.5263016720721170e-002, -2.2483331611592005e-002, 5.2917326684281110e-003, };
			static const double HBKernel_6A[6] = { 6.2187202340480707e-001, -1.7132842113816371e-001, 6.9019169178765674e-002, -2.5799728312695277e-002, 7.4880112525741666e-003, -1.2844465869952567e-003, };
			static const double HBKernel_7A[7] = { 6.2354494135775851e-001, -1.7571220703702045e-001, 7.4529843603968457e-002, -3.0701736822442153e-002, 1.0716755639039573e-002, -2.7833422930759735e-003, 4.1118797093875510e-004, };
			static const double HBKernel_8A[8] = { 6.2488363107953926e-001, -1.7924942606514119e-001, 7.9068155655640557e-002, -3.4907523415495731e-002, 1.3710256799907897e-002, -4.3991142586987933e-003, 1.0259190163889602e-003, -1.3278941979339359e-004, };
			static const double HBKernel_9A[9] = { 6.2597763804021977e-001, -1.8216414325139055e-001, 8.2879104876726728e-002, -3.8563442248249404e-002, 1.6471530499739394e-002, -6.0489108881335227e-003, 1.7805283804140392e-003, -3.7533200112729561e-004, 4.3172840558735476e-005, };
			static const double HBKernel_10A[10] = { 6.2688767582974092e-001, -1.8460766807559420e-001, 8.6128943000481864e-002, -4.1774474147006607e-002, 1.9014801985747346e-002, -7.6870397465866507e-003, 2.6264590175341853e-003, -7.1106660285478562e-004, 1.3645852036179345e-004, -1.4113888783332969e-005, };
			static const double HBKernel_11A[11] = { 6.2667167706948146e-001, -1.8407153342635879e-001, 8.5529995610836046e-002, -4.1346831462361310e-002, 1.8844831691322637e-002, -7.7125170365394992e-003, 2.7268674860562087e-003, -7.9745028501057233e-004, 1.8116344606360795e-004, -2.8569149754241848e-005, 2.3667022010173616e-006, };
			static const double HBKernel_12A[12] = { 6.2747849730367999e-001, -1.8623616784506747e-001, 8.8409755898467945e-002, -4.4207468821462342e-002, 2.1149175945115381e-002, -9.2551508371115209e-003, 3.5871562170822330e-003, -1.1923167653750219e-003, 3.2627812189920129e-004, -6.9106902511490413e-005, 1.0122897863125124e-005, -7.7531878906846174e-007, };
			static const double HBKernel_13A[13] = { 6.2816416252367324e-001, -1.8809076955230414e-001, 9.0918539867353029e-002, -4.6765502683599310e-002, 2.3287520498995663e-002, -1.0760627245014184e-002, 4.4853922948425683e-003, -1.6438775426910800e-003, 5.1441312354764978e-004, -1.3211725685765050e-004, 2.6191319837779187e-005, -3.5802430606313093e-006, 2.5491278270628601e-007, };
			static const double HBKernel_14A[14] = { 6.2875473120929948e-001, -1.8969941936903847e-001, 9.3126094480960403e-002, -4.9067251179869126e-002, 2.5273008851199916e-002, -1.2218646153393291e-002, 5.4048942085580280e-003, -2.1409919546078581e-003, 7.4250292812927973e-004, -2.1924542206832172e-004, 5.3015808983125091e-005, -9.8743034923598196e-006, 1.2650391141650221e-006, -8.4146674637474946e-008, };
			static const int FltCountA = 11;
			static const int FlttBaseA = 4;
			static const double FltAttensA[FltCountA] = { 54.5176, 66.3075, 89.5271, 105.2842, 121.0063, 136.6982, 152.3572, 183.7962, 199.4768, 215.1364, 230.7526, };
			static const double* const FltPtrsA[FltCountA] = { HBKernel_4A, HBKernel_5A, HBKernel_6A, HBKernel_7A, HBKernel_8A, HBKernel_9A, HBKernel_10A, HBKernel_11A, HBKernel_12A, HBKernel_13A, HBKernel_14A, };

			static const double HBKernel_2B[2] = { 5.7361525854329076e-001, -7.5092074924827903e-002, };
			static const double HBKernel_3B[3] = { 5.9277038608066912e-001, -1.0851340190268854e-001, 1.5813570475513079e-002, };
			static const double HBKernel_4B[4] = { 6.0140277542879617e-001, -1.2564483854574138e-001, 2.7446500598038322e-002, -3.2051079559057435e-003, };
			static const double HBKernel_5B[5] = { 6.0818642429088932e-001, -1.3981140187175697e-001, 3.8489164054503623e-002, -7.6218861797853104e-003, 7.5772358130952392e-004, };
			static const double HBKernel_6B[6] = { 6.1278392271464355e-001, -1.5000053762513338e-001, 4.7575323511364960e-002, -1.2320702802243476e-002, 2.1462442592348487e-003, -1.8425092381892940e-004, };
			static const double HBKernel_7B[7] = { 6.1610372263478952e-001, -1.5767891882524138e-001, 5.5089691170294691e-002, -1.6895755656366061e-002, 3.9416643438213977e-003, -6.0603623791604668e-004, 4.5632602433393365e-005, };
			static const double HBKernel_8B[8] = { 6.1861282914465976e-001, -1.6367179451225150e-001, 6.1369861342939716e-002, -2.1184466539006987e-002, 5.9623357510842061e-003, -1.2483098507454090e-003, 1.7099297537964702e-004, -1.1448313239478885e-005, };
			static const int FltCountB = 7;
			static const int FlttBaseB = 2;
			static const double FltAttensB[FltCountB] = { 56.6007, 83.0295, 123.4724, 152.4411, 181.2501, 209.9472, 238.5616, };
			static const double* const FltPtrsB[FltCountB] = { HBKernel_2B, HBKernel_3B, HBKernel_4B, HBKernel_5B, HBKernel_6B, HBKernel_7B, HBKernel_8B, };

			static const double HBKernel_2C[2] = { 5.6430278013478008e-001, -6.4338068855763375e-002, };
			static const double HBKernel_3C[3] = { 5.8706402915551448e-001, -9.9362380958670449e-002, 1.2298637065869358e-002, };
			static const double HBKernel_4C[4] = { 5.9896586134984675e-001, -1.2111680603434927e-001, 2.4763118076458895e-002, -2.6121758132212989e-003, };
			static const double HBKernel_5C[5] = { 6.0626808285230716e-001, -1.3588224032740795e-001, 3.5544305238309003e-002, -6.5127022377289654e-003, 5.8255449565950768e-004, };
			static const double HBKernel_6C[6] = { 6.1120171263351242e-001, -1.4654486853757870e-001, 4.4582959299131253e-002, -1.0840543858123995e-002, 1.7343706485509962e-003, -1.3363018567985596e-004, };
			static const int FltCountC = 5;
			static const int FlttBaseC = 2;
			static const double FltAttensC[FltCountC] = { 89.0473, 130.8951, 172.3192, 213.4984, 254.5186, };
			static const double* const FltPtrsC[FltCountC] = { HBKernel_2C, HBKernel_3C, HBKernel_4C, HBKernel_5C, HBKernel_6C, };

			static const double HBKernel_1D[1] = { 5.0188900022775451e-001, };
			static const double HBKernel_2D[2] = { 5.6295152180538044e-001, -6.2953706070191726e-002, };
			static const double HBKernel_3D[3] = { 5.8621968728755036e-001, -9.8080551656524531e-002, 1.1860868761997080e-002, };
			static const double HBKernel_4D[4] = { 5.9835028657163591e-001, -1.1999986086623511e-001, 2.4132530854004228e-002, -2.4829565686819706e-003, };
			static const int FltCountD = 4;
			static const int FlttBaseD = 1;
			static const double FltAttensD[FltCountD] = { 54.4754, 113.2139, 167.1447, 220.6519, };
			static const double* const FltPtrsD[FltCountD] = { HBKernel_1D, HBKernel_2D, HBKernel_3D, HBKernel_4D, };

			static const double HBKernel_1E[1] = { 5.0047102586416625e-001, };
			static const double HBKernel_2E[2] = { 5.6261293163933568e-001, -6.2613067826620017e-002, };
			static const double HBKernel_3E[3] = { 5.8600808139396787e-001, -9.7762185880067784e-002, 1.1754104554493029e-002, };
			static const double HBKernel_4E[4] = { 5.9819599352772002e-001, -1.1972157555011861e-001, 2.3977305567947922e-002, -2.4517235455853992e-003, };
			static const int FltCountE = 4;
			static const int FlttBaseE = 1;
			static const double FltAttensE[FltCountE] = { 66.5391, 137.3173, 203.2997, 268.8550, };
			static const double* const FltPtrsE[FltCountE] = { HBKernel_1E, HBKernel_2E, HBKernel_3E, HBKernel_4E, };

			static const double HBKernel_1F[1] = { 5.0007530666642896e-001, };
			static const double HBKernel_2F[2] = { 5.6252823610146030e-001, -6.2528244608044792e-002, };
			static const double HBKernel_3F[3] = { 5.8595514744674237e-001, -9.7682725156791952e-002, 1.1727577711117231e-002, };
			static const int FltCountF = 3;
			static const int FlttBaseF = 1;
			static const double FltAttensF[FltCountF] = { 82.4633, 161.4049, 239.4313, };
			static const double* const FltPtrsF[FltCountF] = { HBKernel_1F, HBKernel_2F, HBKernel_3F, };

			static const double HBKernel_1G[1] = { 5.0001882524896712e-001, };
			static const double HBKernel_2G[2] = { 5.6250705922479682e-001, -6.2507059756378394e-002, };
			static const double HBKernel_3G[3] = { 5.8594191201187384e-001, -9.7662868266991207e-002, 1.1720956255134043e-002, };
			static const int FltCountG = 3;
			static const int FlttBaseG = 1;
			static const double FltAttensG[FltCountG] = { 94.5052, 185.4886, 275.5501, };
			static const double* const FltPtrsG[FltCountG] = { HBKernel_1G, HBKernel_2G, HBKernel_3G, };


			int k = 0;

			if (SteepIndex <= 0) { while (k != FltCountA - 1 && FltAttensA[k] < ReqAtten) { k++; } flt = FltPtrsA[k]; fltt = FlttBaseA + k; att = FltAttensA[k]; }
			else if (SteepIndex == 1) { while (k != FltCountB - 1 && FltAttensB[k] < ReqAtten) { k++; } flt = FltPtrsB[k]; fltt = FlttBaseB + k; att = FltAttensB[k]; }
			else if (SteepIndex == 2) { while (k != FltCountC - 1 && FltAttensC[k] < ReqAtten) { k++; } flt = FltPtrsC[k]; fltt = FlttBaseC + k; att = FltAttensC[k]; }
			else if (SteepIndex == 3) { while (k != FltCountD - 1 && FltAttensD[k] < ReqAtten) { k++; } flt = FltPtrsD[k]; fltt = FlttBaseD + k; att = FltAttensD[k]; }
			else if (SteepIndex == 4) { while (k != FltCountE - 1 && FltAttensE[k] < ReqAtten) { k++; } flt = FltPtrsE[k]; fltt = FlttBaseE + k; att = FltAttensE[k]; }
			else if (SteepIndex == 5) { while (k != FltCountF - 1 && FltAttensF[k] < ReqAtten) { k++; } flt = FltPtrsF[k]; fltt = FlttBaseF + k; att = FltAttensF[k]; }
			else { while (k != FltCountG - 1 && FltAttensG[k] < ReqAtten) { k++; } flt = FltPtrsG[k]; fltt = FlttBaseG + k; att = FltAttensG[k]; }
		}

		CDSPHBUpsampler(
			const double ReqAtten,
			const int SteepIndex,
			const bool IsThird,
			const double PrevLatency,
			const bool aDoConsumeLatency = true
		)
			: DoConsumeLatency(aDoConsumeLatency)
		{
			static const CConvolveFn FltConvFn[14] = { &CDSPHBUpsampler::convolve1, &CDSPHBUpsampler::convolve2, &CDSPHBUpsampler::convolve3, &CDSPHBUpsampler::convolve4, &CDSPHBUpsampler::convolve5, &CDSPHBUpsampler::convolve6, &CDSPHBUpsampler::convolve7, &CDSPHBUpsampler::convolve8, &CDSPHBUpsampler::convolve9, &CDSPHBUpsampler::convolve10, &CDSPHBUpsampler::convolve11, &CDSPHBUpsampler::convolve12, &CDSPHBUpsampler::convolve13, &CDSPHBUpsampler::convolve14 };

			const double* fltp0;
			int fltt;
			double att;

			getHBFilter(ReqAtten, SteepIndex, fltp0, fltt, att);

			fltp = alignptr(FltBuf, 16);
			memcpy(fltp, fltp0, fltt * sizeof(fltp[0]));

			convfn = FltConvFn[fltt - 1];
			fll = fltt - 1;
			fl2 = fltt;
			flo = fll + fl2;
			BufRP = Buf + fll;

			LatencyFrac = PrevLatency * 2.0;
			Latency = (int)LatencyFrac;
			LatencyFrac -= Latency;

			R8BASSERT(Latency >= 0);

			if (DoConsumeLatency) { flb = BufLen - fll; }
			else { Latency += fl2 + fl2; flb = BufLen - flo; }

			R8BCONSOLE("CDSPHBUpsampler: sti=%i third=%i taps=%i att=%.1f "
				"io=2/1\n", SteepIndex, (int)IsThird, fltt, att);

			clear();
		}

		virtual int getInLenBeforeOutPos(const int ReqOutPos) const { return(fl2 + (int)((Latency + LatencyFrac + ReqOutPos) * 0.5)); }
		virtual int getLatency() const { return(DoConsumeLatency ? 0 : Latency); }
		virtual double getLatencyFrac() const { return(LatencyFrac); }
		virtual int getMaxOutLen(const int MaxInLen) const { R8BASSERT(MaxInLen >= 0); return(MaxInLen * 2); }
		virtual void clear() { if (DoConsumeLatency) { LatencyLeft = Latency; BufLeft = 0; } else { LatencyLeft = 0; BufLeft = fl2; } WritePos = 0; ReadPos = flb; /* Set "read" position to account for filter's latency. */ memset(&Buf[ReadPos], 0, (BufLen - flb) * sizeof(Buf[0])); }

		virtual int process(double* ip, int l, double*& op0)
		{
			R8BASSERT(l >= 0);

			double* op = op0;

			while (l > 0)
			{
				

				const int b = (((l) < ((((BufLen - WritePos) < (flb - BufLeft)) ? (BufLen - WritePos) : (flb - BufLeft)))) ? (l) : ((((BufLen - WritePos) < (flb - BufLeft)) ? (BufLen - WritePos) : (flb - BufLeft))));

				double* const wp1 = Buf + WritePos;
				memcpy(wp1, ip, b * sizeof(wp1[0]));
				const int ec = flo - WritePos;

				if (ec > 0) { memcpy(wp1 + BufLen, ip, (((b) < (ec)) ? (b) : (ec)) * sizeof(wp1[0])); }

				ip += b;
				WritePos = (WritePos + b) & BufLenMask;
				l -= b;
				BufLeft += b;

				const int c = BufLeft - fl2;

				if (c > 0)
				{
					double* const opend = op + c * 2;
					(*convfn)(op, opend, fltp, BufRP, ReadPos);

					op = opend;
					ReadPos = (ReadPos + c) & BufLenMask;
					BufLeft -= c;
				}
			}

			int ol = (int)(op - op0);

			if (LatencyLeft != 0)
			{
				if (LatencyLeft >= ol) { LatencyLeft -= ol; return(0); }

				ol -= LatencyLeft;
				op0 += LatencyLeft;
				LatencyLeft = 0;
			}

			return(ol);
		}

	private:
		static const int BufLenBits = 9;
		static const int BufLen = 1 << BufLenBits;
		static const int BufLenMask = BufLen - 1;
		double Buf[BufLen + 27];
		double FltBuf[14 + 2];
		const double* BufRP;
		double* fltp;
		int fll;
		int fl2;
		int flo;
		int flb;
		double LatencyFrac;
		int Latency;
		int LatencyLeft;
		int BufLeft;
		int WritePos;
		int ReadPos;
		bool DoConsumeLatency;

		typedef void(*CConvolveFn)(double* op, double* const opend,
			const double* const flt, const double* const rp0, int rpos);
		CConvolveFn convfn;

#define R8BHBC1( fn ) \
	static void fn( double* op, double* const opend, const double* const flt, \
		const double* const rp0, int rpos ) \
	{ \
		while( op != opend ) \
		{ \
			const double* const rp = rp0 + rpos; \
			op[ 0 ] = rp[ 0 ];

#define R8BHBC2 \
			rpos = ( rpos + 1 ) & BufLenMask; \
			op += 2; \
		} \
	}

		R8BHBC1(convolve1)
			op[1] = flt[0] * (rp[1] + rp[0]);
		R8BHBC2

			R8BHBC1(convolve2)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]);
		R8BHBC2

			R8BHBC1(convolve3)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]);
		R8BHBC2

			R8BHBC1(convolve4)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]);
		R8BHBC2

			R8BHBC1(convolve5)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]);
		R8BHBC2

			R8BHBC1(convolve6)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]);
		R8BHBC2

			R8BHBC1(convolve7)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]);
		R8BHBC2

			R8BHBC1(convolve8)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]);
		R8BHBC2

			R8BHBC1(convolve9)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]);
		R8BHBC2

			R8BHBC1(convolve10)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]);
		R8BHBC2

			R8BHBC1(convolve11)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]);
		R8BHBC2

			R8BHBC1(convolve12)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]);
		R8BHBC2

			R8BHBC1(convolve13)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]) + flt[12] * (rp[13] + rp[-12]);
		R8BHBC2

			R8BHBC1(convolve14)
			op[1] = flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]) + flt[12] * (rp[13] + rp[-12]) + flt[13] * (rp[14] + rp[-13]);
		R8BHBC2

#undef R8BHBC1
#undef R8BHBC2
	};
} // namespace r8b


/**
 * @file CDSPHBDownsampler.h
 */

namespace r8b {

	class CDSPHBDownsampler : public CDSPProcessor
	{
	public:

		CDSPHBDownsampler(
			const double ReqAtten,
			const int SteepIndex,
			const bool IsThird,
			const double PrevLatency
		)
		{
			static const CConvolveFn FltConvFn[14] = { &CDSPHBDownsampler::convolve1, &CDSPHBDownsampler::convolve2, &CDSPHBDownsampler::convolve3, &CDSPHBDownsampler::convolve4, &CDSPHBDownsampler::convolve5, &CDSPHBDownsampler::convolve6, &CDSPHBDownsampler::convolve7, &CDSPHBDownsampler::convolve8, &CDSPHBDownsampler::convolve9, &CDSPHBDownsampler::convolve10, &CDSPHBDownsampler::convolve11, &CDSPHBDownsampler::convolve12, &CDSPHBDownsampler::convolve13, &CDSPHBDownsampler::convolve14 };

			const double* fltp0;
			int fltt;
			double att;

			CDSPHBUpsampler::getHBFilter(ReqAtten, SteepIndex, fltp0, fltt, att);

			fltp = alignptr(FltBuf, 16);
			memcpy(fltp, fltp0, fltt * sizeof(fltp[0]));

			convfn = FltConvFn[fltt - 1];
			fll = fltt;
			fl2 = fltt - 1;
			flo = fll + fl2;
			flb = BufLen - fll;
			BufRP1 = Buf1 + fll;
			BufRP2 = Buf2 + fll - 1;

			LatencyFrac = PrevLatency * 0.5;
			Latency = (int)LatencyFrac;
			LatencyFrac -= Latency;

			R8BASSERT(Latency >= 0);

			R8BCONSOLE("CDSPHBDownsampler: taps=%i third=%i att=%.1f io=1/2\n",
				fltt, (int)IsThird, att);

			clear();
		}
		virtual int getInLenBeforeOutPos(const int ReqOutPos) const { return(flo + (int)((Latency + LatencyFrac + ReqOutPos) * 2.0)); }
		virtual int getLatency() const { return(0); }
		virtual double getLatencyFrac() const { return(LatencyFrac); }
		virtual int getMaxOutLen(const int MaxInLen) const { R8BASSERT(MaxInLen >= 0); return((MaxInLen + 1) >> 1); }
		virtual void clear() { LatencyLeft = Latency; BufLeft = 0; WritePos1 = 0; WritePos2 = 0; ReadPos = flb; /* Set "read" position to account for filter's latency. */ memset(&Buf1[ReadPos], 0, (BufLen - flb) * sizeof(Buf1[0])); memset(&Buf2[ReadPos], 0, (BufLen - flb) * sizeof(Buf2[0])); }

		virtual int process(double* ip, int l, double*& op0)
		{
			R8BASSERT(l >= 0);

			double* op = op0;

			while (l > 0)
			{
				if (WritePos1 != WritePos2)
				{
					double* const wp2 = Buf2 + WritePos2; *wp2 = *ip;
					if (WritePos2 < flo) { wp2[BufLen] = *ip; }
					ip++; WritePos2 = WritePos1; l--; BufLeft++;
				}
				
					
				const int b1 = ((((l + 1) >> 1) < ((((BufLen - WritePos1) < (flb - BufLeft)) ? (BufLen - WritePos1) : (flb - BufLeft)))) ? ((l + 1) >> 1) : ((((BufLen - WritePos1) < (flb - BufLeft)) ? (BufLen - WritePos1) : (flb - BufLeft))));

				const int b2 = b1 - (b1 * 2 > l);

				double* wp1 = Buf1 + WritePos1;
				double* wp2 = Buf2 + WritePos1;
				double* const ipe = ip + b2 * 2;

				while (ip != ipe) { *wp1 = ip[0]; *wp2 = ip[1]; wp1++; wp2++; ip += 2; }
				if (b1 != b2) { *wp1 = *ip; ip++; }

				const int ec = flo - WritePos1;

				if (ec > 0)
				{
					wp1 = Buf1 + WritePos1;
					memcpy(wp1 + BufLen, wp1, (((b1) < (ec)) ? (b1) : (ec)) * sizeof(wp1[0]));
					wp2 = Buf2 + WritePos1;
					memcpy(wp2 + BufLen, wp2, (((b2) < (ec)) ? (b2) : (ec)) * sizeof(wp2[0]));
				}

				WritePos1 = (WritePos1 + b1) & BufLenMask;
				WritePos2 = (WritePos2 + b2) & BufLenMask;
				l -= b1 + b2;
				BufLeft += b2;

				const int c = BufLeft - fl2;

				if (c > 0)
				{
					double* const opend = op + c;
					(*convfn)(op, opend, fltp, BufRP1, BufRP2, ReadPos);

					op = opend;
					ReadPos = (ReadPos + c) & BufLenMask;
					BufLeft -= c;
				}
			}

			int ol = (int)(op - op0);

			if (LatencyLeft != 0)
			{
				if (LatencyLeft >= ol) { LatencyLeft -= ol; return(0); }

				ol -= LatencyLeft;
				op0 += LatencyLeft;
				LatencyLeft = 0;
			}

			return(ol);
		}

	private:
		static const int BufLenBits = 10;
		static const int BufLen = 1 << BufLenBits;
		static const int BufLenMask = BufLen - 1;
		double Buf1[BufLen + 27];
		double Buf2[BufLen + 27];
		double FltBuf[14 + 2];
		const double* BufRP1;
		const double* BufRP2;
		double* fltp;
		double LatencyFrac;
		int Latency;
		int fll;
		int fl2;
		int flo;
		int flb;
		int LatencyLeft;
		int BufLeft;
		int WritePos1;
		int WritePos2;
		int ReadPos;

		typedef void(*CConvolveFn)(
			double* op,
			double* const opend,
			const double* const flt,
			const double* const rp01,
			const double* const rp02,
			int rpos
			); ///< Convolution function type.
		CConvolveFn convfn; ///< Convolution function in use.

#define R8BHBC1( fn ) \
	static void fn( double* op, double* const opend, const double* const flt, \
		const double* const rp01, const double* const rp02, int rpos ) \
	{ \
		while( op != opend ) \
		{ \
			const double* const rp1 = rp01 + rpos; \
			const double* const rp = rp02 + rpos;

#define R8BHBC2 \
			rpos = ( rpos + 1 ) & BufLenMask; \
			op++; \
		} \
	}


		R8BHBC1(convolve1)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]);
		R8BHBC2

			R8BHBC1(convolve2)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]);
		R8BHBC2

			R8BHBC1(convolve3)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]);
		R8BHBC2

			R8BHBC1(convolve4)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]);
		R8BHBC2

			R8BHBC1(convolve5)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]);
		R8BHBC2

			R8BHBC1(convolve6)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]);
		R8BHBC2

			R8BHBC1(convolve7)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]);
		R8BHBC2

			R8BHBC1(convolve8)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]);
		R8BHBC2

			R8BHBC1(convolve9)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]);
		R8BHBC2

			R8BHBC1(convolve10)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]);
		R8BHBC2

			R8BHBC1(convolve11)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]);
		R8BHBC2

			R8BHBC1(convolve12)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]);
		R8BHBC2

			R8BHBC1(convolve13)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]) + flt[12] * (rp[13] + rp[-12]);
		R8BHBC2

			R8BHBC1(convolve14)
			op[0] = rp1[0] + flt[0] * (rp[1] + rp[0]) + flt[1] * (rp[2] + rp[-1]) + flt[2] * (rp[3] + rp[-2]) + flt[3] * (rp[4] + rp[-3]) + flt[4] * (rp[5] + rp[-4]) + flt[5] * (rp[6] + rp[-5]) + flt[6] * (rp[7] + rp[-6]) + flt[7] * (rp[8] + rp[-7]) + flt[8] * (rp[9] + rp[-8]) + flt[9] * (rp[10] + rp[-9]) + flt[10] * (rp[11] + rp[-10]) + flt[11] * (rp[12] + rp[-11]) + flt[12] * (rp[13] + rp[-12]) + flt[13] * (rp[14] + rp[-13]);
		R8BHBC2


#undef R8BHBC1
#undef R8BHBC2
	};

	// ---------------------------------------------------------------------------

} // namespace r8b





/**
 * @file CDSPSincFilterGen.h
 */

namespace r8b {

	class CDSPSincFilterGen
	{
	public:
		double Len2; ///< Required half filter kernel's length in samples (can be a fractional value). Final physical kernel length will be provided in the KernelLen variable. Len2 should be >= 2.
		double Len2i; ///< = 1.0 / Len2, initialized and used by some window functions for optimization (should not be initialized by the caller).
		int KernelLen; ///< Resulting length of the filter kernel, this variable is set after the call to one of the "init" functions.
		int fl2; ///< Internal "half kernel length" value. This value can be used as filter's latency in samples (taps), this variable is set after the call to one of the "init" functions.

		union
		{
			struct
			{
				double Freq1; ///< Required corner circular frequency 1 [0; pi]. Used only in the generateBand() function.
				double Freq2; ///< Required corner circular frequency 2 [0; pi]. Used only in the generateBand() function. The range [Freq1; Freq2] defines a pass band for the generateBand() function.
			};

			struct
			{
				double FracDelay; ///< Fractional delay in the range [0; 1], used only in the generateFrac() function. Note that the FracDelay parameter is actually inversed. At 0.0 value it produces 1 sample delay (with the latency equal to fl2), at 1.0 value it produces 0 sample delay (with the latency ///< equal to fl2 - 1).
			};
		};


		/* Window function type. */
		enum EWindowFunctionType
		{
			wftCosine, ///< Generalized cosine window function. No parameters required. The "Power" parameter is optional.
			wftKaiser, ///< Kaiser window function. Requires the "Beta" parameter. The "Power" parameter is optional.
			wftGaussian ///< Gaussian window function. Requires the "Sigma" parameter. The "Power" parameter is optional.
		};

		typedef double(CDSPSincFilterGen ::* CWindowFunc)(); ///< Window calculation function pointer type.

		void initWindow(const EWindowFunctionType WinType = wftCosine, const double* const Params = NULL, const bool UsePower = false) { R8BASSERT(Len2 >= 2.0); fl2 = (int)floor(Len2); KernelLen = fl2 + fl2 + 1; setWindow(WinType, Params, UsePower, true); }
		void initBand(const EWindowFunctionType WinType = wftCosine, const double* const Params = NULL, const bool UsePower = false) { R8BASSERT(Len2 >= 2.0); fl2 = (int)floor(Len2); KernelLen = fl2 + fl2 + 1; setWindow(WinType, Params, UsePower, true); }
		void initHilbert(const EWindowFunctionType WinType = wftCosine, const double* const Params = NULL, const bool UsePower = false) { R8BASSERT(Len2 >= 2.0); fl2 = (int)floor(Len2); KernelLen = fl2 + fl2 + 1; setWindow(WinType, Params, UsePower, true); }
		void initFrac(const EWindowFunctionType WinType = wftCosine, const double* const Params = NULL, const bool UsePower = false) { R8BASSERT(Len2 >= 2.0); fl2 = (int)ceil(Len2); KernelLen = fl2 + fl2; setWindow(WinType, Params, UsePower, false, FracDelay); }
		double calcWindowHann() { return(0.5 + 0.5 * w1.generate()); }
		double calcWindowHamming() { return(0.54 + 0.46 * w1.generate()); }
		double calcWindowBlackman() { return(0.42 + 0.5 * w1.generate() + 0.08 * w2.generate()); }
		double calcWindowNuttall() { return(0.355768 + 0.487396 * w1.generate() + 0.144232 * w2.generate() + 0.012604 * w3.generate()); }
		double calcWindowBlackmanNuttall() { return(0.3635819 + 0.4891775 * w1.generate() + 0.1365995 * w2.generate() + 0.0106411 * w3.generate()); }
		double calcWindowKaiser() { const double n = 1.0 - sqr(wn * Len2i + KaiserLen2Frac); wn++; if (n <= 0.0) { return(0.0); } return(besselI0(KaiserBeta * sqrt(n)) * KaiserMul); }
		double calcWindowGaussian() { const double f = exp(-0.5 * sqr(wn * GaussianSigmaI + GaussianSigmaFrac)); wn++; return(f); }
		void generateWindow(double* op, CWindowFunc wfunc = &CDSPSincFilterGen::calcWindowBlackman) { op += fl2; double* op2 = op; int l = fl2; if (Power < 0.0) { *op = (*this.*wfunc)(); while (l > 0) { const double v = (*this.*wfunc)(); op++; op2--; *op = v; *op2 = v; l--; } } else { *op = pow_a((*this.*wfunc)(), Power); while (l > 0) { const double v = pow_a((*this.*wfunc)(), Power); op++; op2--; *op = v; *op2 = v; l--; } } }
		void generateBand(double* op, CWindowFunc wfunc = &CDSPSincFilterGen::calcWindowBlackman) { CSineGen f2(Freq2, 0.0, 1.0 / R8B_PI); f2.generate(); op += fl2; double* op2 = op; const double pw = Power; int t = 1; if (Freq1 < 0x1p-42) { if (pw < 0.0) { *op = Freq2 * (*this.*wfunc)() / R8B_PI; while (t <= fl2) { const double v = f2.generate() * (*this.*wfunc)() / t; op++; op2--; *op = v; *op2 = v; t++; } } else { *op = Freq2 * pow_a((*this.*wfunc)(), pw) / R8B_PI; while (t <= fl2) { const double v = f2.generate() * pow_a((*this.*wfunc)(), pw) / t; op++; op2--; *op = v; *op2 = v; t++; } } } else { CSineGen f1(Freq1, 0.0, 1.0 / R8B_PI); f1.generate(); if (pw < 0.0) { *op = (Freq2 - Freq1) * (*this.*wfunc)() / R8B_PI; while (t <= fl2) { const double v = (f2.generate() - f1.generate()) * (*this.*wfunc)() / t; op++; op2--; *op = v; *op2 = v; t++; } } else { *op = (Freq2 - Freq1) * pow_a((*this.*wfunc)(), pw) / R8B_PI; while (t <= fl2) { const double v = (f2.generate() - f1.generate()) * pow_a((*this.*wfunc)(), pw) / t; op++; op2--; *op = v; *op2 = v; t++; } } } }
		void generateHilbert(double* op, CWindowFunc wfunc = &CDSPSincFilterGen::calcWindowBlackman) { static const double fvalues[2] = { 0.0, 2.0 / R8B_PI }; op += fl2; double* op2 = op; (*this.*wfunc)(); *op = 0.0; int t = 1; if (Power < 0.0) { while (t <= fl2) { const double v = fvalues[t & 1] * (*this.*wfunc)() / t; op++; op2--; *op = v; *op2 = -v; t++; } } else { while (t <= fl2) { const double v = fvalues[t & 1] * pow_a((*this.*wfunc)(), Power) / t; op++; op2--; *op = v; *op2 = -v; t++; } } }
		void generateFrac(double* op, CWindowFunc wfunc = &CDSPSincFilterGen::calcWindowBlackman, const int opinc = 1) { R8BASSERT(opinc != 0); const double pw = Power; const double fd = FracDelay; int t = -fl2; if (t + fd < -Len2) { (*this.*wfunc)(); *op = 0.0; op += opinc; t++; } double f = sin(fd * R8B_PI) / R8B_PI; if ((t & 1) != 0) { f = -f; } int IsZeroX = (fabs(fd - 1.0) < 0x1p-42); int mt = 0 - IsZeroX; IsZeroX = (IsZeroX || (fabs(fd) < 0x1p-42)); if (pw < 0.0) { while (t < mt) { *op = f * (*this.*wfunc)() / (t + fd); op += opinc; t++; f = -f; } if (IsZeroX) /* t + FracDelay == 0 */ { *op = (*this.*wfunc)(); } else { *op = f * (*this.*wfunc)() / fd; /* t == 0 */ } mt = fl2 - 2; while (t < mt) { op += opinc; t++; f = -f; *op = f * (*this.*wfunc)() / (t + fd); } op += opinc; t++; f = -f; const double ut = t + fd; *op = (ut > Len2 ? 0.0 : f * (*this.*wfunc)() / ut); } else { while (t < mt) { *op = f * pow_a((*this.*wfunc)(), pw) / (t + fd); op += opinc; t++; f = -f; } if (IsZeroX) /* t + FracDelay == 0 */ { *op = pow_a((*this.*wfunc)(), pw); } else { *op = f * pow_a((*this.*wfunc)(), pw) / fd; /* t == 0 */ } mt = fl2 - 2; while (t < mt) { op += opinc; t++; f = -f; *op = f * pow_a((*this.*wfunc)(), pw) / (t + fd); } op += opinc; t++; f = -f; const double ut = t + FracDelay; *op = (ut > Len2 ? 0.0 : f * pow_a((*this.*wfunc)(), pw) / ut); } }

	private:
		double Power; ///< The power factor used to raise the window function. Equals a negative value if the power factor should not be used.
		CSineGen w1; ///< Cosine wave 1 for window function.
		CSineGen w2; ///< Cosine wave 2 for window function.
		CSineGen w3; ///< Cosine wave 3 for window function.

		union
		{
			struct
			{
				double KaiserBeta; ///< Kaiser window function's "Beta" coefficient.
				double KaiserMul; ///< Kaiser window function's divisor, inverse.
				double KaiserLen2Frac; ///< Equals FracDelay / Len2.
			};

			struct
			{
				double GaussianSigmaI; ///< Gaussian window function's "Sigma" coefficient, inverse.
				double GaussianSigmaFrac; ///< Equals FracDelay / GaussianSigma.
			};
		};

		int wn; ///< Window function integer position. 0 - center of the window unction. This variable may not be used by some window functions.
		void setWindowKaiser(const double* Params, const bool UsePower, const bool IsCentered) { wn = (IsCentered ? 0 : -fl2); if (Params == NULL) { KaiserBeta = 9.5945013206755156; Power = (UsePower ? 1.9718457932433306 : -1.0); } else { KaiserBeta = clampr(Params[0], 1.0, 350.0); Power = (UsePower ? fabs(Params[1]) : -1.0); } KaiserMul = 1.0 / besselI0(KaiserBeta); Len2i = 1.0 / Len2; KaiserLen2Frac = FracDelay * Len2i; }
		void setWindowGaussian(const double* Params, const bool UsePower, const bool IsCentered) { wn = (IsCentered ? 0 : -fl2); if (Params == NULL) { GaussianSigmaI = 1.0; Power = -1.0; } else { GaussianSigmaI = clampr(fabs(Params[0]), 1e-1, 100.0); Power = (UsePower ? fabs(Params[1]) : -1.0); } GaussianSigmaI *= Len2; GaussianSigmaI = 1.0 / GaussianSigmaI; GaussianSigmaFrac = FracDelay * GaussianSigmaI; }
		void setWindow(const EWindowFunctionType WinType, const double* const Params, const bool UsePower, const bool IsCentered, const double UseFracDelay = 0.0) { FracDelay = UseFracDelay; if (WinType == wftCosine) { if (IsCentered) { w1.init(R8B_PI / Len2, R8B_PId2); w2.init(R8B_2PI / Len2, R8B_PId2); w3.init(R8B_3PI / Len2, R8B_PId2); } else { const double step1 = R8B_PI / Len2; w1.init(step1, R8B_PId2 - step1 * fl2 + step1 * FracDelay); const double step2 = R8B_2PI / Len2; w2.init(step2, R8B_PId2 - step2 * fl2 + step2 * FracDelay); const double step3 = R8B_3PI / Len2; w3.init(step3, R8B_PId2 - step3 * fl2 + step3 * FracDelay); } Power = (UsePower && Params != NULL ? Params[0] : -1.0); } else if (WinType == wftKaiser) { setWindowKaiser(Params, UsePower, IsCentered); } else if (WinType == wftGaussian) { setWindowGaussian(Params, UsePower, IsCentered); } }
	};

} // namespace r8b


/**
 * @file fft4g.h
 *
 * @brief Wrapper class for Takuya OOURA's FFT functions.
 *
 * Functions from the FFT package by: Copyright(C) 1996-2001 Takuya OOURA
 * http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
 *
 * Modified and used with permission granted by the license.
 *
 * Here, the original "fft4g.c" file was wrapped into the "ooura_fft" class.
 */

namespace r8b {

	class ooura_fft
	{
		friend class CDSPRealFFT;

	private:
		typedef double FPType;

		static void cdft(int n, int isgn, FPType* a, int* ip, FPType* w) { if (n > (ip[0] << 2)) { makewt(n >> 2, ip, w); } if (n > 4) { if (isgn >= 0) { bitrv2(n, ip + 2, a); cftfsub(n, a, w); } else { bitrv2conj(n, ip + 2, a); cftbsub(n, a, w); } } else if (n == 4) { cftfsub(n, a, w); } }
		static void rdft(int n, int isgn, FPType* a, int* ip, FPType* w) { int nw, nc; double xi; nw = ip[0]; if (n > (nw << 2)) { nw = n >> 2; makewt(nw, ip, w); } nc = ip[1]; if (n > (nc << 2)) { nc = n >> 2; makect(nc, ip, w + nw); } if (isgn >= 0) { if (n > 4) { bitrv2(n, ip + 2, a); cftfsub(n, a, w); rftfsub(n, a, nc, w + nw); } else if (n == 4) { cftfsub(n, a, w); } xi = a[0] - a[1]; a[0] += a[1]; a[1] = xi; } else { a[1] = 0.5 * (a[0] - a[1]); a[0] -= a[1]; if (n > 4) { rftbsub(n, a, nc, w + nw); bitrv2(n, ip + 2, a); cftbsub(n, a, w); } else if (n == 4) { cftfsub(n, a, w); } } }
		static void ddct(int n, int isgn, FPType* a, int* ip, FPType* w) { int j, nw, nc; double xr; nw = ip[0]; if (n > (nw << 2)) { nw = n >> 2; makewt(nw, ip, w); } nc = ip[1]; if (n > nc) { nc = n; makect(nc, ip, w + nw); } if (isgn < 0) { xr = a[n - 1]; for (j = n - 2; j >= 2; j -= 2) { a[j + 1] = a[j] - a[j - 1]; a[j] += a[j - 1]; } a[1] = a[0] - xr; a[0] += xr; if (n > 4) { rftbsub(n, a, nc, w + nw); bitrv2(n, ip + 2, a); cftbsub(n, a, w); } else if (n == 4) { cftfsub(n, a, w); } } dctsub(n, a, nc, w + nw); if (isgn >= 0) { if (n > 4) { bitrv2(n, ip + 2, a); cftfsub(n, a, w); rftfsub(n, a, nc, w + nw); } else if (n == 4) { cftfsub(n, a, w); } xr = a[0] - a[1]; a[0] += a[1]; for (j = 2; j < n; j += 2) { a[j - 1] = a[j] - a[j + 1]; a[j] += a[j + 1]; } a[n - 1] = xr; } }
		static void ddst(int n, int isgn, FPType* a, int* ip, FPType* w) { int j, nw, nc; double xr; nw = ip[0]; if (n > (nw << 2)) { nw = n >> 2; makewt(nw, ip, w); } nc = ip[1]; if (n > nc) { nc = n; makect(nc, ip, w + nw); } if (isgn < 0) { xr = a[n - 1]; for (j = n - 2; j >= 2; j -= 2) { a[j + 1] = -a[j] - a[j - 1]; a[j] -= a[j - 1]; } a[1] = a[0] + xr; a[0] -= xr; if (n > 4) { rftbsub(n, a, nc, w + nw); bitrv2(n, ip + 2, a); cftbsub(n, a, w); } else if (n == 4) { cftfsub(n, a, w); } } dstsub(n, a, nc, w + nw); if (isgn >= 0) { if (n > 4) { bitrv2(n, ip + 2, a); cftfsub(n, a, w); rftfsub(n, a, nc, w + nw); } else if (n == 4) { cftfsub(n, a, w); } xr = a[0] - a[1]; a[0] += a[1]; for (j = 2; j < n; j += 2) { a[j - 1] = -a[j] - a[j + 1]; a[j] -= a[j + 1]; } a[n - 1] = -xr; } }
		static void dfct(int n, FPType* a, FPType* t, int* ip, FPType* w) { int j, k, l, m, mh, nw, nc; double xr, xi, yr, yi; nw = ip[0]; if (n > (nw << 3)) { nw = n >> 3; makewt(nw, ip, w); } nc = ip[1]; if (n > (nc << 1)) { nc = n >> 1; makect(nc, ip, w + nw); } m = n >> 1; yi = a[m]; xi = a[0] + a[n]; a[0] -= a[n]; t[0] = xi - yi; t[m] = xi + yi; if (n > 2) { mh = m >> 1; for (j = 1; j < mh; j++) { k = m - j; xr = a[j] - a[n - j]; xi = a[j] + a[n - j]; yr = a[k] - a[n - k]; yi = a[k] + a[n - k]; a[j] = xr; a[k] = yr; t[j] = xi - yi; t[k] = xi + yi; } t[mh] = a[mh] + a[n - mh]; a[mh] -= a[n - mh]; dctsub(m, a, nc, w + nw); if (m > 4) { bitrv2(m, ip + 2, a); cftfsub(m, a, w); rftfsub(m, a, nc, w + nw); } else if (m == 4) { cftfsub(m, a, w); } a[n - 1] = a[0] - a[1]; a[1] = a[0] + a[1]; for (j = m - 2; j >= 2; j -= 2) { a[2 * j + 1] = a[j] + a[j + 1]; a[2 * j - 1] = a[j] - a[j + 1]; } l = 2; m = mh; while (m >= 2) { dctsub(m, t, nc, w + nw); if (m > 4) { bitrv2(m, ip + 2, t); cftfsub(m, t, w); rftfsub(m, t, nc, w + nw); } else if (m == 4) { cftfsub(m, t, w); } a[n - l] = t[0] - t[1]; a[l] = t[0] + t[1]; k = 0; for (j = 2; j < m; j += 2) { k += l << 2; a[k - l] = t[j] - t[j + 1]; a[k + l] = t[j] + t[j + 1]; } l <<= 1; mh = m >> 1; for (j = 0; j < mh; j++) { k = m - j; t[j] = t[m + k] - t[m + j]; t[k] = t[m + k] + t[m + j]; } t[mh] = t[m + mh]; m = mh; } a[l] = t[0]; a[n] = t[2] - t[1]; a[0] = t[2] + t[1]; } else { a[1] = a[0]; a[2] = t[0]; a[0] = t[1]; } }
		static void dfst(int n, FPType* a, FPType* t, int* ip, FPType* w) { int j, k, l, m, mh, nw, nc; double xr, xi, yr, yi; nw = ip[0]; if (n > (nw << 3)) { nw = n >> 3; makewt(nw, ip, w); } nc = ip[1]; if (n > (nc << 1)) { nc = n >> 1; makect(nc, ip, w + nw); } if (n > 2) { m = n >> 1; mh = m >> 1; for (j = 1; j < mh; j++) { k = m - j; xr = a[j] + a[n - j]; xi = a[j] - a[n - j]; yr = a[k] + a[n - k]; yi = a[k] - a[n - k]; a[j] = xr; a[k] = yr; t[j] = xi + yi; t[k] = xi - yi; } t[0] = a[mh] - a[n - mh]; a[mh] += a[n - mh]; a[0] = a[m]; dstsub(m, a, nc, w + nw); if (m > 4) { bitrv2(m, ip + 2, a); cftfsub(m, a, w); rftfsub(m, a, nc, w + nw); } else if (m == 4) { cftfsub(m, a, w); } a[n - 1] = a[1] - a[0]; a[1] = a[0] + a[1]; for (j = m - 2; j >= 2; j -= 2) { a[2 * j + 1] = a[j] - a[j + 1]; a[2 * j - 1] = -a[j] - a[j + 1]; } l = 2; m = mh; while (m >= 2) { dstsub(m, t, nc, w + nw); if (m > 4) { bitrv2(m, ip + 2, t); cftfsub(m, t, w); rftfsub(m, t, nc, w + nw); } else if (m == 4) { cftfsub(m, t, w); } a[n - l] = t[1] - t[0]; a[l] = t[0] + t[1]; k = 0; for (j = 2; j < m; j += 2) { k += l << 2; a[k - l] = -t[j] - t[j + 1]; a[k + l] = t[j] - t[j + 1]; } l <<= 1; mh = m >> 1; for (j = 1; j < mh; j++) { k = m - j; t[j] = t[m + k] + t[m + j]; t[k] = t[m + k] - t[m + j]; } t[0] = t[m + mh]; m = mh; } a[l] = t[0]; } a[0] = 0; }

		/* -------- initializing routines -------- */

		static void makewt(int nw, int* ip, FPType* w) { int j, nwh; double delta, x, y; ip[0] = nw; ip[1] = 1; if (nw > 2) { nwh = nw >> 1; delta = atan(1.0) / nwh; w[0] = 1; w[1] = 0; w[nwh] = cos(delta * nwh); w[nwh + 1] = w[nwh]; if (nwh > 2) { for (j = 2; j < nwh; j += 2) { x = cos(delta * j); y = sin(delta * j); w[j] = x; w[j + 1] = y; w[nw - j] = y; w[nw - j + 1] = x; } bitrv2(nw, ip + 2, w); } } }
		static void makect(int nc, int* ip, FPType* c) { int j, nch; double delta; ip[1] = nc; if (nc > 1) { nch = nc >> 1; delta = atan(1.0) / nch; c[0] = cos(delta * nch); c[nch] = 0.5 * c[0]; for (j = 1; j < nch; j++) { c[j] = 0.5 * cos(delta * j); c[nc - j] = 0.5 * sin(delta * j); } } }

		/* -------- child routines -------- */

		static void bitrv2(int n, int* ip, FPType* a) { int j, j1, k, k1, l, m, m2; double xr, xi, yr, yi; ip[0] = 0; l = n; m = 1; while ((m << 3) < l) { l >>= 1; for (j = 0; j < m; j++) { ip[m + j] = ip[j] + l; } m <<= 1; } m2 = 2 * m; if ((m << 3) == l) { for (k = 0; k < m; k++) { for (j = 0; j < k; j++) { j1 = 2 * j + ip[k]; k1 = 2 * k + ip[j]; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += 2 * m2; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 -= m2; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += 2 * m2; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; } j1 = 2 * k + m2 + ip[k]; k1 = j1 + m2; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; } } else { for (k = 1; k < m; k++) { for (j = 0; j < k; j++) { j1 = 2 * j + ip[k]; k1 = 2 * k + ip[j]; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += m2; xr = a[j1]; xi = a[j1 + 1]; yr = a[k1]; yi = a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; } } } }
		static void bitrv2conj(int n, int* ip, FPType* a) { int j, j1, k, k1, l, m, m2; double xr, xi, yr, yi; ip[0] = 0; l = n; m = 1; while ((m << 3) < l) { l >>= 1; for (j = 0; j < m; j++) { ip[m + j] = ip[j] + l; } m <<= 1; } m2 = 2 * m; if ((m << 3) == l) { for (k = 0; k < m; k++) { for (j = 0; j < k; j++) { j1 = 2 * j + ip[k]; k1 = 2 * k + ip[j]; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += 2 * m2; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 -= m2; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += 2 * m2; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; } k1 = 2 * k + ip[k]; a[k1 + 1] = -a[k1 + 1]; j1 = k1 + m2; k1 = j1 + m2; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; k1 += m2; a[k1 + 1] = -a[k1 + 1]; } } else { a[1] = -a[1]; a[m2 + 1] = -a[m2 + 1]; for (k = 1; k < m; k++) { for (j = 0; j < k; j++) { j1 = 2 * j + ip[k]; k1 = 2 * k + ip[j]; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; j1 += m2; k1 += m2; xr = a[j1]; xi = -a[j1 + 1]; yr = a[k1]; yi = -a[k1 + 1]; a[j1] = yr; a[j1 + 1] = yi; a[k1] = xr; a[k1 + 1] = xi; } k1 = 2 * k + ip[k]; a[k1 + 1] = -a[k1 + 1]; a[k1 + m2 + 1] = -a[k1 + m2 + 1]; } } }
		static void cftfsub(int n, FPType* a, const FPType* w) { int j, j1, j2, j3, l; double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i; l = 2; if (n > 8) { cft1st(n, a, w); l = 8; while ((l << 2) < n) { cftmdl(n, l, a, w); l <<= 2; } } if ((l << 2) == n) { for (j = 0; j < l; j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = a[j + 1] + a[j1 + 1]; x1r = a[j] - a[j1]; x1i = a[j + 1] - a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; a[j2] = x0r - x2r; a[j2 + 1] = x0i - x2i; a[j1] = x1r - x3i; a[j1 + 1] = x1i + x3r; a[j3] = x1r + x3i; a[j3 + 1] = x1i - x3r; } } else { for (j = 0; j < l; j += 2) { j1 = j + l; x0r = a[j] - a[j1]; x0i = a[j + 1] - a[j1 + 1]; a[j] += a[j1]; a[j + 1] += a[j1 + 1]; a[j1] = x0r; a[j1 + 1] = x0i; } } }
		static void cftbsub(int n, FPType* a, const FPType* w) { int j, j1, j2, j3, l; double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i; l = 2; if (n > 8) { cft1st(n, a, w); l = 8; while ((l << 2) < n) { cftmdl(n, l, a, w); l <<= 2; } } if ((l << 2) == n) { for (j = 0; j < l; j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = -a[j + 1] - a[j1 + 1]; x1r = a[j] - a[j1]; x1i = -a[j + 1] + a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i - x2i; a[j2] = x0r - x2r; a[j2 + 1] = x0i + x2i; a[j1] = x1r - x3i; a[j1 + 1] = x1i - x3r; a[j3] = x1r + x3i; a[j3 + 1] = x1i + x3r; } } else { for (j = 0; j < l; j += 2) { j1 = j + l; x0r = a[j] - a[j1]; x0i = -a[j + 1] + a[j1 + 1]; a[j] += a[j1]; a[j + 1] = -a[j + 1] - a[j1 + 1]; a[j1] = x0r; a[j1 + 1] = x0i; } } }
		static void cft1st(int n, FPType* a, const FPType* w) { int j, k1, k2; double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i; double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i; x0r = a[0] + a[2]; x0i = a[1] + a[3]; x1r = a[0] - a[2]; x1i = a[1] - a[3]; x2r = a[4] + a[6]; x2i = a[5] + a[7]; x3r = a[4] - a[6]; x3i = a[5] - a[7]; a[0] = x0r + x2r; a[1] = x0i + x2i; a[4] = x0r - x2r; a[5] = x0i - x2i; a[2] = x1r - x3i; a[3] = x1i + x3r; a[6] = x1r + x3i; a[7] = x1i - x3r; wk1r = w[2]; x0r = a[8] + a[10]; x0i = a[9] + a[11]; x1r = a[8] - a[10]; x1i = a[9] - a[11]; x2r = a[12] + a[14]; x2i = a[13] + a[15]; x3r = a[12] - a[14]; x3i = a[13] - a[15]; a[8] = x0r + x2r; a[9] = x0i + x2i; a[12] = x2i - x0i; a[13] = x0r - x2r; x0r = x1r - x3i; x0i = x1i + x3r; a[10] = wk1r * (x0r - x0i); a[11] = wk1r * (x0r + x0i); x0r = x3i + x1r; x0i = x3r - x1i; a[14] = wk1r * (x0i - x0r); a[15] = wk1r * (x0i + x0r); k1 = 0; for (j = 16; j < n; j += 16) { k1 += 2; k2 = 2 * k1; wk2r = w[k1]; wk2i = w[k1 + 1]; wk1r = w[k2]; wk1i = w[k2 + 1]; wk3r = wk1r - 2 * wk2i * wk1i; wk3i = 2 * wk2i * wk1r - wk1i; x0r = a[j] + a[j + 2]; x0i = a[j + 1] + a[j + 3]; x1r = a[j] - a[j + 2]; x1i = a[j + 1] - a[j + 3]; x2r = a[j + 4] + a[j + 6]; x2i = a[j + 5] + a[j + 7]; x3r = a[j + 4] - a[j + 6]; x3i = a[j + 5] - a[j + 7]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; x0r -= x2r; x0i -= x2i; a[j + 4] = wk2r * x0r - wk2i * x0i; a[j + 5] = wk2r * x0i + wk2i * x0r; x0r = x1r - x3i; x0i = x1i + x3r; a[j + 2] = wk1r * x0r - wk1i * x0i; a[j + 3] = wk1r * x0i + wk1i * x0r; x0r = x1r + x3i; x0i = x1i - x3r; a[j + 6] = wk3r * x0r - wk3i * x0i; a[j + 7] = wk3r * x0i + wk3i * x0r; wk1r = w[k2 + 2]; wk1i = w[k2 + 3]; wk3r = wk1r - 2 * wk2r * wk1i; wk3i = 2 * wk2r * wk1r - wk1i; x0r = a[j + 8] + a[j + 10]; x0i = a[j + 9] + a[j + 11]; x1r = a[j + 8] - a[j + 10]; x1i = a[j + 9] - a[j + 11]; x2r = a[j + 12] + a[j + 14]; x2i = a[j + 13] + a[j + 15]; x3r = a[j + 12] - a[j + 14]; x3i = a[j + 13] - a[j + 15]; a[j + 8] = x0r + x2r; a[j + 9] = x0i + x2i; x0r -= x2r; x0i -= x2i; a[j + 12] = -wk2i * x0r - wk2r * x0i; a[j + 13] = -wk2i * x0i + wk2r * x0r; x0r = x1r - x3i; x0i = x1i + x3r; a[j + 10] = wk1r * x0r - wk1i * x0i; a[j + 11] = wk1r * x0i + wk1i * x0r; x0r = x1r + x3i; x0i = x1i - x3r; a[j + 14] = wk3r * x0r - wk3i * x0i; a[j + 15] = wk3r * x0i + wk3i * x0r; } }
		static void cftmdl(int n, int l, FPType* a, const FPType* w) { int j, j1, j2, j3, k, k1, k2, m, m2; double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i; double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i; m = l << 2; for (j = 0; j < l; j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = a[j + 1] + a[j1 + 1]; x1r = a[j] - a[j1]; x1i = a[j + 1] - a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; a[j2] = x0r - x2r; a[j2 + 1] = x0i - x2i; a[j1] = x1r - x3i; a[j1 + 1] = x1i + x3r; a[j3] = x1r + x3i; a[j3 + 1] = x1i - x3r; } wk1r = w[2]; for (j = m; j < l + m; j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = a[j + 1] + a[j1 + 1]; x1r = a[j] - a[j1]; x1i = a[j + 1] - a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; a[j2] = x2i - x0i; a[j2 + 1] = x0r - x2r; x0r = x1r - x3i; x0i = x1i + x3r; a[j1] = wk1r * (x0r - x0i); a[j1 + 1] = wk1r * (x0r + x0i); x0r = x3i + x1r; x0i = x3r - x1i; a[j3] = wk1r * (x0i - x0r); a[j3 + 1] = wk1r * (x0i + x0r); } k1 = 0; m2 = 2 * m; for (k = m2; k < n; k += m2) { k1 += 2; k2 = 2 * k1; wk2r = w[k1]; wk2i = w[k1 + 1]; wk1r = w[k2]; wk1i = w[k2 + 1]; wk3r = wk1r - 2 * wk2i * wk1i; wk3i = 2 * wk2i * wk1r - wk1i; for (j = k; j < l + k; j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = a[j + 1] + a[j1 + 1]; x1r = a[j] - a[j1]; x1i = a[j + 1] - a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; x0r -= x2r; x0i -= x2i; a[j2] = wk2r * x0r - wk2i * x0i; a[j2 + 1] = wk2r * x0i + wk2i * x0r; x0r = x1r - x3i; x0i = x1i + x3r; a[j1] = wk1r * x0r - wk1i * x0i; a[j1 + 1] = wk1r * x0i + wk1i * x0r; x0r = x1r + x3i; x0i = x1i - x3r; a[j3] = wk3r * x0r - wk3i * x0i; a[j3 + 1] = wk3r * x0i + wk3i * x0r; } wk1r = w[k2 + 2]; wk1i = w[k2 + 3]; wk3r = wk1r - 2 * wk2r * wk1i; wk3i = 2 * wk2r * wk1r - wk1i; for (j = k + m; j < l + (k + m); j += 2) { j1 = j + l; j2 = j1 + l; j3 = j2 + l; x0r = a[j] + a[j1]; x0i = a[j + 1] + a[j1 + 1]; x1r = a[j] - a[j1]; x1i = a[j + 1] - a[j1 + 1]; x2r = a[j2] + a[j3]; x2i = a[j2 + 1] + a[j3 + 1]; x3r = a[j2] - a[j3]; x3i = a[j2 + 1] - a[j3 + 1]; a[j] = x0r + x2r; a[j + 1] = x0i + x2i; x0r -= x2r; x0i -= x2i; a[j2] = -wk2i * x0r - wk2r * x0i; a[j2 + 1] = -wk2i * x0i + wk2r * x0r; x0r = x1r - x3i; x0i = x1i + x3r; a[j1] = wk1r * x0r - wk1i * x0i; a[j1 + 1] = wk1r * x0i + wk1i * x0r; x0r = x1r + x3i; x0i = x1i - x3r; a[j3] = wk3r * x0r - wk3i * x0i; a[j3 + 1] = wk3r * x0i + wk3i * x0r; } } }
		static void rftfsub(int n, FPType* a, int nc, const FPType* c) { int j, k, kk, ks, m; double wkr, wki, xr, xi, yr, yi; m = n >> 1; ks = 2 * nc / m; kk = 0; for (j = 2; j < m; j += 2) { k = n - j; kk += ks; wkr = 0.5 - c[nc - kk]; wki = c[kk]; xr = a[j] - a[k]; xi = a[j + 1] + a[k + 1]; yr = wkr * xr - wki * xi; yi = wkr * xi + wki * xr; a[j] -= yr; a[j + 1] -= yi; a[k] += yr; a[k + 1] -= yi; } }
		static void rftbsub(int n, FPType* a, int nc, const FPType* c) { int j, k, kk, ks, m; double wkr, wki, xr, xi, yr, yi; a[1] = -a[1]; m = n >> 1; ks = 2 * nc / m; kk = 0; for (j = 2; j < m; j += 2) { k = n - j; kk += ks; wkr = 0.5 - c[nc - kk]; wki = c[kk]; xr = a[j] - a[k]; xi = a[j + 1] + a[k + 1]; yr = wkr * xr + wki * xi; yi = wkr * xi - wki * xr; a[j] -= yr; a[j + 1] = yi - a[j + 1]; a[k] += yr; a[k + 1] = yi - a[k + 1]; } a[m + 1] = -a[m + 1]; }
		static void dctsub(int n, FPType* a, int nc, const FPType* c) { int j, k, kk, ks, m; double wkr, wki, xr; m = n >> 1; ks = nc / n; kk = 0; for (j = 1; j < m; j++) { k = n - j; kk += ks; wkr = c[kk] - c[nc - kk]; wki = c[kk] + c[nc - kk]; xr = wki * a[j] - wkr * a[k]; a[j] = wkr * a[j] + wki * a[k]; a[k] = xr; } a[m] *= c[0]; }
		static void dstsub(int n, FPType* a, int nc, const FPType* c) { int j, k, kk, ks, m; double wkr, wki, xr; m = n >> 1; ks = nc / n; kk = 0; for (j = 1; j < m; j++) { k = n - j; kk += ks; wkr = c[kk] - c[nc - kk]; wki = c[kk] + c[nc - kk]; xr = wki * a[k] - wkr * a[j]; a[k] = wkr * a[k] + wki * a[j]; a[j] = xr; } a[m] *= c[0]; }
	};

} // namespace r8b




/**
 * @file CDSPRealFFT.h
 */

namespace r8b {
	class CDSPRealFFT : public R8B_BASECLASS
	{
		R8BNOCTOR(CDSPRealFFT);

		friend class CDSPRealFFTKeeper;

	public:
		double getInvMulConst() const { return(InvMulConst); }
		int getLenBits() const { return(LenBits); }
		int getLen() const { return(Len); }
		void forward(double* const p) const { ooura_fft::rdft(Len, 1, p, wi.getPtr(), wd.getPtr()); }
		void inverse(double* const p) const { ooura_fft::rdft(Len, -1, p, wi.getPtr(), wd.getPtr()); }
		void multiplyBlocksZP(const double* const aip, double* const aop) const { const double* ip = aip; double* op = aop; int i; for (i = 0; i < Len; i++) { op[i] *= ip[i]; } }
		void convertToZP(double* const ap) const { double* const p = ap; int i = 2; while (i < Len) { p[i + 1] = p[i]; i += 2; } }

	private:
		int LenBits; ///< Length of FFT block (expressed as Nth power of 2).
		int Len; ///< Length of FFT block (number of real values).
		double InvMulConst; ///< Inverse FFT multiply constant.
		CDSPRealFFT* Next; ///< Next object in a singly-linked list.

		CFixedBuffer< int > wi; ///< Working buffer (ints).
		CFixedBuffer< double > wd; ///< Working buffer (doubles).

		class CObjKeeper
		{
			R8BNOCTOR(CObjKeeper);

		public:
			CObjKeeper() : Object(NULL) { }
			~CObjKeeper() { delete Object; }
			CObjKeeper& operator = (CDSPRealFFT* const aObject) { Object = aObject; return(*this); }
			operator CDSPRealFFT* () const { return(Object); }

		private:
			CDSPRealFFT* Object; ///< FFT object being kept.
		};

		CDSPRealFFT() { }
		CDSPRealFFT(const int aLenBits) : LenBits(aLenBits), Len(1 << aLenBits), InvMulConst(2.0 / Len) { wi.alloc((int)ceil(2.0 + sqrt((double)(Len >> 1)))); wi[0] = 0; wd.alloc(Len >> 1); }

		~CDSPRealFFT() { delete Next; }
	};

	class CDSPRealFFTKeeper : public R8B_BASECLASS
	{
		R8BNOCTOR(CDSPRealFFTKeeper);

	public:
		CDSPRealFFTKeeper() : Object(NULL) { }
		CDSPRealFFTKeeper(const int LenBits) { Object = acquire(LenBits); }
		~CDSPRealFFTKeeper() { if (Object != NULL) { release(Object); } }
		const CDSPRealFFT* operator -> () const { R8BASSERT(Object != NULL); return(Object); }
		void init(const int LenBits) { if (Object != NULL) { if (Object->LenBits == LenBits) { return; } release(Object); } Object = acquire(LenBits); }
		void reset() { if (Object != NULL) { release(Object); Object = NULL; } }

	private:
		CDSPRealFFT* Object; ///< FFT object.

		static CSyncObject StateSync; ///< FFTObjects synchronizer.
		static CDSPRealFFT::CObjKeeper FFTObjects[]; ///< Pool of FFT objects of various lengths.
		CDSPRealFFT* acquire(const int LenBits) { R8BASSERT(LenBits > 0 && LenBits <= 30); R8BSYNC(StateSync); if (FFTObjects[LenBits] == NULL) { return(new CDSPRealFFT(LenBits)); } CDSPRealFFT* ffto = FFTObjects[LenBits]; FFTObjects[LenBits] = ffto->Next; return(ffto); }
		void release(CDSPRealFFT* const ffto) { R8BSYNC(StateSync); ffto->Next = FFTObjects[ffto->LenBits]; FFTObjects[ffto->LenBits] = ffto; }
	};
} // namespace r8b


/**
 * @file CDSPFIRFilter.h
 */


namespace r8b {

	// Enumeration of filter's phase responses.
	enum EDSPFilterPhaseResponse
	{
		fprLinearPhase = 0,
		fprMinPhase // no use
	};

	class CDSPFIRFilter : public R8B_BASECLASS
	{
		R8BNOCTOR(CDSPFIRFilter);

		friend class CDSPFIRFilterCache;

	public:
		~CDSPFIRFilter() { R8BASSERT(RefCount == 0); delete Next; }


		static double getLPMinTransBand() { return(0.5); }          // @return The minimal allowed low-pass filter's transition band, in percent.
		static double getLPMaxTransBand() { return(45.0); }         // @return The maximal allowed low-pass filter's transition band, in percent.
		static double getLPMinAtten() { return(49.0); }         // @return The minimal allowed low-pass filter's stop-band attenuation, in decibel.
		static double getLPMaxAtten() { return(218.0); }        // @return The maximal allowed low-pass filter's stop-band attenuation, in decibel.
		bool          isZeroPhase()     const { return(IsZeroPhase); }  // @return "True" if kernel block of *this filter has zero-phase response.
		int           getLatency()      const { return(Latency); }      // @return Filter's latency, in samples (integer part).
		double        getLatencyFrac()  const { return(LatencyFrac); }  // @return Filter's latency, in samples (fractional part). Always zero for linear-phase filters.
		int           getKernelLen()    const { return(KernelLen); }    // @return Filter kernel length, in samples. Not to be confused with the block length.
		int           getBlockLenBits() const { return(BlockLenBits); } // @return Filter's block length, expressed as Nth power of 2. The actual length is twice as large due to zero-padding.
		const double* getKernelBlock() const { return(KernelBlock); }
		void          unref(); // This function should be called when the filter obtained via the filter cache is no longer needed.

	private:
		double                  ReqNormFreq;  ///< Required normalized frequency, 0 to 1 inclusive.
		double                  ReqTransBand; ///< Required transition band in percent, as passed by the user.
		double                  ReqAtten;     ///< Required stop-band attenuation in decibel, as passed by the user (positive value).
		EDSPFilterPhaseResponse ReqPhase;     ///< Required filter's phase response.
		double                  ReqGain;      ///< Required overall filter's gain.
		CDSPFIRFilter* Next;         ///< Next FIR filter in cache's list.
		int                     RefCount;     ///< The number of references made to *this FIR filter.
		bool                    IsZeroPhase;  ///< "True" if kernel block of *this filter has zero-phase response.
		int                     Latency;      ///< Filter's latency in samples (integer part).
		double                  LatencyFrac;  ///< Filter's latency in samples (fractional part).
		int                     KernelLen;    ///< Filter kernel length, in samples.
		int                     BlockLenBits; ///< Block length used to store *this FIR filter, expressed as Nth power of 2. This value is used directly by the convolver.
		CFixedBuffer< double >  KernelBlock;  ///< FIR filter buffer, capacity equals to 1 << ( BlockLenBits + 1 ). Second part of the buffer contains zero-padding to allow alias-free convolution. Address-aligned.

		CDSPFIRFilter() : RefCount(1) { }

		void buildLPFilter(const double* const ExtAttenCorrs)
		{
			const double tb = ReqTransBand * 0.01;
			double pwr;
			double fo1;
			double hl;
			double atten = -ReqAtten;

			if (tb >= 0.25)
			{
				if (ReqAtten >= 117.0) { atten -= 1.60; }
				else if (ReqAtten >= 60.0) { atten -= 1.91; }
				else { atten -= 2.25; }
			}
			else if (tb >= 0.10)
			{
				if (ReqAtten >= 117.0) { atten -= 0.69; }
				else if (ReqAtten >= 60.0) { atten -= 0.73; }
				else { atten -= 1.13; }
			}
			else
			{
				if (ReqAtten >= 117.0) { atten -= 0.21; }
				else if (ReqAtten >= 60.0) { atten -= 0.25; }
				else { atten -= 0.36; }
			}

			static const int    AttenCorrCount = 264;
			static const double AttenCorrMin = 49.0;
			static const double AttenCorrDiff = 176.25;
			int AttenCorr = (int)floor((-atten - AttenCorrMin) * AttenCorrCount / AttenCorrDiff + 0.5);

			AttenCorr = (((AttenCorrCount) < ((((0) > (AttenCorr)) ? (0) : (AttenCorr)))) ? (AttenCorrCount) : ((((0) > (AttenCorr)) ? (0) : (AttenCorr)))); // min(AttenCorrCount, max(0, AttenCorr));
			

			if (ExtAttenCorrs != NULL)
			{
				atten -= ExtAttenCorrs[AttenCorr];
			}
			else if (tb >= 0.25)
			{
				static const double AttenCorrScale = 101.0;
				static const signed char AttenCorrs[] = { -127, -127, -125, -125, -122, -119, -115, -110, -104, -97, -91, -82, -75, -24, -16, -6, 4, 14, 24, 29, 30, 32, 37, 44, 51, 57, 63, 67, 65, 50, 53, 56, 58, 60, 63, 64, 66, 68, 74, 77, 78, 78, 78, 79, 79, 60, 60, 60, 61, 59, 52, 47, 41, 36, 30, 24, 17, 9, 0, -8, -10, -11, -14, -13, -18, -25, -31, -38, -44, -50, -57, -63, -68, -74, -81, -89, -96, -101, -104, -107, -109, -110, -86, -84, -85, -82, -80, -77, -73, -67, -62, -55, -48, -42, -35, -30, -20, -11, -2, 5, 6, 6, 7, 11, 16, 21, 26, 34, 41, 46, 49, 52, 55, 56, 48, 49, 51, 51, 52, 52, 52, 52, 52, 51, 51, 50, 47, 47, 50, 48, 46, 42, 38, 35, 31, 27, 24, 20, 16, 12, 11, 12, 10, 8, 4, -1, -6, -11, -16, -19, -17, -21, -24, -27, -32, -34, -37, -38, -40, -41, -40, -40, -42, -41, -44, -45, -43, -41, -34, -31, -28, -24, -21, -18, -14, -10, -5, -1, 2, 5, 8, 7, 4, 3, 2, 2, 4, 6, 8, 9, 9, 10, 10, 10, 10, 9, 8, 9, 11, 14, 13, 12, 11, 10, 8, 7, 6, 5, 3, 2, 2, -1, -1, -3, -3, -4, -4, -5, -4, -6, -7, -9, -5, -1, -1, 0, 1, 0, -2, -3, -4, -5, -5, -8, -13, -13, -13, -12, -13, -12, -11, -11, -9, -8, -7, -5, -3, -1, 2, 4, 6, 9, 10, 11, 14, 18, 21, 24, 27, 30, 34, 37, 37, 39, 40 };

				atten -= AttenCorrs[AttenCorr] / AttenCorrScale;
			}
			else if (tb >= 0.10)
			{
				static const double AttenCorrScale = 210.0;
				static const signed char AttenCorrs[] = { -113, -118, -122, -125, -126, -97, -95, -92, -92, -89, -82, -75, -69, -48, -42, -36, -30, -22, -14, -5, -2, 1, 6, 13, 22, 28, 35, 41, 48, 55, 56, 56, 61, 65, 71, 77, 81, 83, 85, 85, 74, 74, 73, 72, 71, 70, 68, 64, 59, 56, 49, 52, 46, 42, 36, 32, 26, 20, 13, 7, -2, -6, -10, -15, -20, -27, -33, -38, -44, -43, -48, -53, -57, -63, -69, -73, -75, -79, -81, -74, -76, -77, -77, -78, -81, -80, -80, -78, -76, -65, -62, -59, -56, -51, -48, -44, -38, -33, -25, -19, -13, -5, -1, 2, 7, 13, 17, 21, 25, 30, 35, 40, 45, 50, 53, 56, 57, 55, 58, 59, 62, 64, 67, 67, 68, 68, 62, 61, 61, 59, 59, 57, 57, 55, 52, 48, 42, 38, 35, 31, 26, 20, 15, 13, 10, 7, 3, -2, -8, -13, -17, -23, -28, -34, -37, -40, -41, -45, -48, -50, -53, -57, -59, -62, -63, -63, -57, -57, -56, -56, -54, -54, -53, -49, -48, -41, -38, -33, -31, -26, -23, -18, -12, -9, -7, -7, -3, 0, 5, 9, 14, 16, 20, 22, 21, 23, 25, 27, 28, 29, 34, 33, 35, 33, 31, 30, 29, 29, 26, 26, 25, 24, 20, 19, 15, 10, 8, 4, 1, -2, -6, -10, -16, -19, -23, -26, -27, -30, -34, -39, -43, -47, -51, -52, -54, -56, -58, -59, -62, -63, -66, -65, -65, -64, -59, -57, -54, -52, -48, -44, -42, -37, -32, -22, -17, -10, -3, 5, 13, 22, 30, 40, 50, 60, 72 };

				atten -= AttenCorrs[AttenCorr] / AttenCorrScale;
			}
			else
			{
				static const double AttenCorrScale = 196.0;
				static const signed char AttenCorrs[] = { -15, -17, -20, -20, -20, -21, -20, -16, -17, -18, -17, -13, -12, -11, -9, -7, -5, -4, -1, 1, 3, 4, 5, 6, 7, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 10, 11, 10, 10, 8, 10, 11, 10, 11, 11, 13, 14, 15, 19, 27, 26, 23, 18, 14, 8, 4, -2, -6, -12, -17, -23, -28, -33, -37, -42, -46, -49, -53, -57, -60, -61, -64, -65, -67, -66, -66, -66, -65, -64, -61, -59, -56, -52, -48, -42, -38, -31, -27, -19, -13, -7, -1, 8, 14, 22, 29, 37, 45, 52, 59, 66, 73, 80, 86, 91, 96, 100, 104, 108, 111, 114, 115, 117, 118, 120, 120, 118, 117, 114, 113, 111, 107, 103, 99, 95, 89, 84, 78, 72, 66, 60, 52, 44, 37, 30, 21, 14, 6, -3, -11, -18, -26, -34, -43, -51, -58, -65, -73, -78, -85, -90, -97, -102, -107, -113, -115, -118, -121, -125, -125, -126, -126, -126, -125, -124, -121, -119, -115, -111, -109, -101, -102, -95, -88, -81, -73, -67, -63, -54, -47, -40, -33, -26, -18, -11, -5, 2, 8, 14, 19, 25, 31, 36, 37, 43, 47, 49, 51, 52, 57, 57, 56, 57, 58, 58, 58, 57, 56, 52, 52, 50, 48, 44, 41, 39, 37, 33, 31, 26, 24, 21, 18, 14, 11, 8, 4, 2, -2, -5, -7, -9, -11, -13, -15, -16, -18, -19, -20, -23, -24, -24, -25, -27, -26, -27, -29, -30, -31, -32, -35, -36, -39, -40, -44, -46, -51, -54, -59, -63, -69, -76, -83, -91, -98 };

				atten -= AttenCorrs[AttenCorr] / AttenCorrScale;
			}

			pwr = 7.43932822146293e-8 * sqr(atten) + 0.000102747434588003 * cos(0.00785021930010397 * atten) * cos(0.633854318781239 + 0.103208573657699 * atten) - 0.00798132247867036 - 0.000903555213543865 * atten - 0.0969365532127236 * exp(0.0779275237937911 * atten) - 1.37304948662012e-5 * atten * cos(0.00785021930010397 * atten);

			if (pwr <= 0.067665322581)
			{
				if (tb >= 0.25)
				{
					hl = 2.6778150875894 / tb + 300.547590563091 * atan(atan(2.68959772209918 * pwr)) / (5.5099277187035 * tb - tb * tanh(cos(asinh(atten))));
					fo1 = 0.987205355829873 * tb + 1.00011788929851 * atan2(-0.321432067051302 - 6.19131357321578 * sqrt(pwr), hl + -1.14861472207245 / (hl - 14.1821147585957) + pow(0.9521145021664, pow(atan2(1.12018764830637, tb), 2.10988901686912 * hl - 20.9691278378345)));
				}
				else if (tb >= 0.10)
				{
					hl = (1.56688617018066 + 142.064321294568 * pwr + 0.00419441117131136 * cos(243.633511747297 * pwr) - 0.022953443903576 * atten - 0.026629568860284 * cos(127.715550622571 * pwr)) / tb;
					fo1 = 0.982299356642411 * tb + 0.999441744774215 * asinh((-0.361783054039583 - 5.80540593623676 * sqrt(pwr)) / hl);
				}
				else
				{
					hl = (2.45739657014937 + 269.183679500541 * pwr * cos(5.73225668178813 + atan2(cosh(0.988861169868941 - 17.2201556280744 * pwr), 1.08340138240431 * pwr))) / tb;
					fo1 = 2.291956939 * tb + 0.01942450693 * sqr(tb) * hl - 4.67538973161837 * pwr * tb - 1.668433124 * tb * pow(pwr, pwr);
				}
			}
			else
			{
				if (tb >= 0.25)
				{
					hl = (1.50258368698213 + 158.556968859477 * asinh(pwr) * tanh(57.9466246871383 * tanh(pwr)) - 0.0105440479814834 * atten) / tb;
					fo1 = 0.994024401639321 * tb + (-0.236282717577215 - 6.8724924545387 * sqrt(sin(pwr))) / hl;
				}
				else if (tb >= 0.10)
				{
					hl = (1.50277377248945 + 158.222625721046 * asinh(pwr) * tanh(1.02875299001715 + 42.072277322604 * pwr) - 0.0108380943845632 * atten) / tb;
					fo1 = 0.992539376734551 * tb + (-0.251747813037178 - 6.74159892452584 * sqrt(tanh(tanh(tan(pwr))))) / hl;
				}
				else
				{
					hl = (1.15990238966306 * pwr - 5.02124037125213 * sqr(pwr) - 0.158676856669827 * atten * cos(1.1609073390614 * pwr - 6.33932586197475 * pwr * sqr(pwr))) / tb;
					fo1 = 0.867344453126885 * tb + 0.052693817907757 * tb * log(pwr) + 0.0895511178735932 * tb * atan(59.7538527741309 * pwr) - 0.0745653568081453 * pwr * tb;
				}
			}

			double WinParams[2];
			WinParams[0] = 125.0;
			WinParams[1] = pwr;

			CDSPSincFilterGen sinc;
			sinc.Len2 = 0.25 * hl / ReqNormFreq;
			sinc.Freq1 = 0.0;
			sinc.Freq2 = R8B_PI * (1.0 - fo1) * ReqNormFreq;
			sinc.initBand(CDSPSincFilterGen::wftKaiser, WinParams, true);

			KernelLen = sinc.KernelLen;
			BlockLenBits = getBitOccupancy(KernelLen - 1) + R8B_EXTFFT;
			const int BlockLen = 1 << BlockLenBits;

			KernelBlock.alloc(BlockLen * 2);
			sinc.generateBand(&KernelBlock[0], &CDSPSincFilterGen::calcWindowKaiser);

			// if (ReqPhase == fprLinearPhase)
			IsZeroPhase = true;
			Latency = sinc.fl2;
			LatencyFrac = 0.0;


			CDSPRealFFTKeeper ffto(BlockLenBits + 1);

			// if (IsZeroPhase)

			// Calculate DC gain.

			double s = 0.0;
			int i;

			for (i = 0; i < KernelLen; i++) { s += KernelBlock[i]; }

			s = ffto->getInvMulConst() * ReqGain / s;

			// Time-shift the filter so that zero-phase response is produced.
			// Simultaneously multiply by "s".

			for (i = 0; i <= sinc.fl2; i++) { KernelBlock[i] = KernelBlock[sinc.fl2 + i] * s; }
			for (i = 1; i <= sinc.fl2; i++) { KernelBlock[BlockLen * 2 - i] = KernelBlock[i]; }

			memset(&KernelBlock[sinc.fl2 + 1], 0, (BlockLen * 2 - KernelLen) * sizeof(KernelBlock[0]));

			ffto->forward(KernelBlock);
			ffto->convertToZP(KernelBlock);

			R8BCONSOLE("CDSPFIRFilter: flt_len=%i latency=%i nfreq=%.4f "
				"tb=%.1f att=%.1f gain=%.3f\n", KernelLen, Latency,
				ReqNormFreq, ReqTransBand, ReqAtten, ReqGain);
		}
	};

	class CDSPFIRFilterCache : public R8B_BASECLASS
	{
		R8BNOCTOR(CDSPFIRFilterCache);

		friend class CDSPFIRFilter;

	public:
		static int getObjCount() { R8BSYNC(StateSync); return(ObjCount); }

		static CDSPFIRFilter& getLPFilter(
			const double ReqNormFreq,
			const double ReqTransBand,
			const double ReqAtten,
			const EDSPFilterPhaseResponse ReqPhase,
			const double ReqGain,
			const double* const AttenCorrs = NULL)
		{
			R8BASSERT(ReqNormFreq > 0.0 && ReqNormFreq <= 1.0);
			R8BASSERT(ReqTransBand >= CDSPFIRFilter::getLPMinTransBand());
			R8BASSERT(ReqTransBand <= CDSPFIRFilter::getLPMaxTransBand());
			R8BASSERT(ReqAtten >= CDSPFIRFilter::getLPMinAtten());
			R8BASSERT(ReqAtten <= CDSPFIRFilter::getLPMaxAtten());
			R8BASSERT(ReqGain > 0.0);

			R8BSYNC(StateSync);

			CDSPFIRFilter* PrevObj = NULL;
			CDSPFIRFilter* CurObj = Objects;

			while (CurObj != NULL)
			{
				if (CurObj->ReqNormFreq == ReqNormFreq &&
					CurObj->ReqTransBand == ReqTransBand &&
					CurObj->ReqGain == ReqGain &&
					CurObj->ReqAtten == ReqAtten &&
					CurObj->ReqPhase == ReqPhase)
				{
					break;
				}

				if (CurObj->Next == NULL && ObjCount >= R8B_FILTER_CACHE_MAX)
				{
					if (CurObj->RefCount == 0)
					{
						// Delete the last filter which is not used.

						PrevObj->Next = NULL;
						delete CurObj;
						ObjCount--;
					}
					else
					{
						// Move the last filter to the top of the list since it
						// seems to be in use for a long time.

						PrevObj->Next = NULL;
						CurObj->Next = Objects.unkeep();
						Objects = CurObj;
					}

					CurObj = NULL;
					break;
				}

				PrevObj = CurObj;
				CurObj = CurObj->Next;
			}

			if (CurObj != NULL)
			{
				CurObj->RefCount++;

				if (PrevObj == NULL) { return(*CurObj); }

				// Remove the filter from the list temporarily.

				PrevObj->Next = CurObj->Next;
			}
			else
			{
				// Create a new filter object (with RefCount == 1) and build the
				// filter kernel.

				CurObj = new CDSPFIRFilter();
				CurObj->ReqNormFreq = ReqNormFreq;
				CurObj->ReqTransBand = ReqTransBand;
				CurObj->ReqAtten = ReqAtten;
				CurObj->ReqPhase = ReqPhase;
				CurObj->ReqGain = ReqGain;
				ObjCount++;

				CurObj->buildLPFilter(AttenCorrs);
			}

			// Insert the filter at the start of the list.

			CurObj->Next = Objects.unkeep();
			Objects = CurObj;

			return(*CurObj);
		}

	private:
		static CSyncObject StateSync; ///< Cache state synchronizer.
		static CPtrKeeper< CDSPFIRFilter* > Objects; ///< The chain of cached objects.
		static int ObjCount; ///< The number of objects currently preset in the cache.
	};

	// ---------------------------------------------------------------------------
	// CDSPFIRFilter PUBLIC
	// ---------------------------------------------------------------------------

	inline void CDSPFIRFilter::unref()
	{
		R8BSYNC(CDSPFIRFilterCache::StateSync);

		RefCount--;
	}

	// ---------------------------------------------------------------------------

} // namespace r8b



/**
 * @file CDSPBlockConvolver.h
 */

namespace r8b {

	class CDSPBlockConvolver : public CDSPProcessor
	{
	public:

		CDSPBlockConvolver(
			CDSPFIRFilter& aFilter,
			const int aUpFactor,
			const int aDownFactor,
			const double PrevLatency = 0.0,
			const bool aDoConsumeLatency = true
		)
			: Filter(&aFilter)
			, UpFactor(aUpFactor)
			, DownFactor(aDownFactor)
			, BlockLen2(2 << Filter -> getBlockLenBits())
			, DoConsumeLatency(aDoConsumeLatency)
		{
			R8BASSERT(UpFactor > 0);
			R8BASSERT(DownFactor > 0);
			R8BASSERT(PrevLatency >= 0.0);

			int fftinBits;
			UpShift = getBitOccupancy(UpFactor) - 1;

			if ((1 << UpShift) == UpFactor)
			{
				fftinBits = Filter->getBlockLenBits() + 1 - UpShift;
				PrevInputLen = (Filter->getKernelLen() - 1 + UpFactor - 1) / UpFactor;
				InputLen = BlockLen2 - PrevInputLen * UpFactor;
			}
			else
			{
				UpShift = -1;
				fftinBits = Filter->getBlockLenBits() + 1;
				PrevInputLen = Filter->getKernelLen() - 1;
				InputLen = BlockLen2 - PrevInputLen;
			}

			OutOffset = (Filter->isZeroPhase() ? Filter->getLatency() : 0);
			LatencyFrac = Filter->getLatencyFrac() + PrevLatency * UpFactor;
			Latency = (int)LatencyFrac;
			const int InLatency = Latency + Filter->getLatency() - OutOffset;
			LatencyFrac -= Latency;
			LatencyFrac /= DownFactor;

			Latency += InputLen + Filter->getLatency();

			int fftoutBits;
			InputDelay = 0;
			DownSkipInit = 0;
			DownShift = getBitOccupancy(DownFactor) - 1;

			if ((1 << DownShift) == DownFactor)
			{
				fftoutBits = Filter->getBlockLenBits() + 1 - DownShift;

				if (DownFactor > 1)
				{
					if (UpShift > 0) { R8BASSERT(UpShift == 0); }
					else
					{
						const int ilc = InputLen & (DownFactor - 1);
						PrevInputLen += ilc;
						InputLen -= ilc;
						Latency -= ilc;

						const int lc = InLatency & (DownFactor - 1);

						if (lc > 0) { InputDelay = DownFactor - lc; }
						if (!DoConsumeLatency) { Latency /= DownFactor; }
					}
				}
			}
			else
			{
				fftoutBits = Filter->getBlockLenBits() + 1;
				DownShift = -1;

				if (!DoConsumeLatency && DownFactor > 1) { DownSkipInit = Latency % DownFactor; Latency /= DownFactor; }
			}

			R8BASSERT(Latency >= 0);

			fftin = new CDSPRealFFTKeeper(fftinBits);

			if (fftoutBits == fftinBits) { fftout = fftin; }
			else { ffto2 = new CDSPRealFFTKeeper(fftoutBits); fftout = ffto2; }

			WorkBlocks.alloc(BlockLen2 * 2 + PrevInputLen);
			CurInput = &WorkBlocks[0];
			CurOutput = &WorkBlocks[BlockLen2]; // CurInput and
			// CurOutput are address-aligned.
			PrevInput = &WorkBlocks[BlockLen2 * 2];

			clear();

			R8BCONSOLE("CDSPBlockConvolver: flt_len=%i in_len=%i io=%i/%i "
				"fft=%i/%i latency=%i\n", Filter->getKernelLen(), InputLen,
				UpFactor, DownFactor, (*fftin)->getLen(), (*fftout)->getLen(),
				getLatency());
		}

		virtual ~CDSPBlockConvolver() { Filter->unref(); }
		virtual int getInLenBeforeOutPos(const int ReqOutPos) const { return((int)((Latency + (double)ReqOutPos * DownFactor) / UpFactor + LatencyFrac * DownFactor / UpFactor)); }
		virtual int getLatency() const { return(DoConsumeLatency ? 0 : Latency); }
		virtual double getLatencyFrac() const { return(LatencyFrac); }
		virtual int getMaxOutLen(const int MaxInLen) const { R8BASSERT(MaxInLen >= 0); return((MaxInLen * UpFactor + DownFactor - 1) / DownFactor); }

		virtual void clear() { memset(&PrevInput[0], 0, PrevInputLen * sizeof(PrevInput[0])); if (DoConsumeLatency) { LatencyLeft = Latency; } else { LatencyLeft = 0; if (DownShift > 0) { memset(&CurOutput[0], 0, (BlockLen2 >> DownShift) * sizeof(CurOutput[0])); } else { memset(&CurOutput[BlockLen2 - OutOffset], 0, OutOffset * sizeof(CurOutput[0])); memset(&CurOutput[0], 0, (InputLen - OutOffset) * sizeof(CurOutput[0])); } } memset(CurInput, 0, InputDelay * sizeof(CurInput[0])); InDataLeft = InputLen - InputDelay; UpSkip = 0; DownSkip = DownSkipInit; }

		virtual int process(double* ip, int l0, double*& op0)
		{
			R8BASSERT(l0 >= 0);
			R8BASSERT(UpFactor / DownFactor <= 1 || ip != op0 || l0 == 0);

			double* op = op0;
			int l = l0 * UpFactor;
			l0 = 0;

			while (l > 0)
			{
				const int Offs = InputLen - InDataLeft;

				if (l < InDataLeft)
				{
					InDataLeft -= l;

					if (UpShift >= 0)
					{
						memcpy(&CurInput[Offs >> UpShift], ip,
							(l >> UpShift) * sizeof(CurInput[0]));
					}
					else
					{
						copyUpsample(ip, &CurInput[Offs], l);
					}

					copyToOutput(Offs - OutOffset, op, l, l0);
					break;
				}

				const int b = InDataLeft;
				l -= b;
				InDataLeft = InputLen;
				int ilu;

				if (UpShift >= 0)
				{
					const int bu = b >> UpShift;
					memcpy(&CurInput[Offs >> UpShift], ip,
						bu * sizeof(CurInput[0]));

					ip += bu;
					ilu = InputLen >> UpShift;
				}
				else
				{
					copyUpsample(ip, &CurInput[Offs], b);
					ilu = InputLen;
				}

				const size_t pil = PrevInputLen * sizeof(CurInput[0]);
				memcpy(&CurInput[ilu], PrevInput, pil);
				memcpy(PrevInput, &CurInput[ilu - PrevInputLen], pil);

				(*fftin)->forward(CurInput);

				if (UpShift > 0) { mirrorInputSpectrum(CurInput); }

				// if (Filter->isZeroPhase()) == always true for lin-phase
				(*fftout)->multiplyBlocksZP(Filter->getKernelBlock(), CurInput);

				if (DownShift > 0)
				{
					const int z = BlockLen2 >> DownShift;
					const double* const kb = Filter->getKernelBlock();
					double* const p = CurInput;
					p[1] = kb[z] * p[z] - kb[z + 1] * p[z + 1];
				}

				(*fftout)->inverse(CurInput);

				copyToOutput(Offs - OutOffset, op, b, l0);

				double* const tmp = CurInput;
				CurInput = CurOutput;
				CurOutput = tmp;
			}

			return(l0);
		}

	private:
		CDSPFIRFilter* Filter;
		CPtrKeeper< CDSPRealFFTKeeper* > fftin;
		CPtrKeeper< CDSPRealFFTKeeper* > ffto2;
		CDSPRealFFTKeeper* fftout;
		int UpFactor;
		int DownFactor;
		int BlockLen2;
		int OutOffset;
		int PrevInputLen;
		int InputLen;
		double LatencyFrac;
		int Latency;
		int UpShift;
		int DownShift;
		int InputDelay;
		double* PrevInput;
		double* CurInput;
		double* CurOutput;
		int InDataLeft;
		int LatencyLeft;
		int UpSkip;
		int DownSkip;
		int DownSkipInit;
		CFixedBuffer< double > WorkBlocks;
		bool DoConsumeLatency;

		void copyUpsample(double*& ip0, double* op, int l0) { int b = (((UpSkip) < (l0)) ? (UpSkip) : (l0)) /*min(UpSkip, l0)*/; if (b != 0) { UpSkip -= b; l0 -= b; *op = 0.0; op++; while (--b != 0) { *op = 0.0; op++; } } double* ip = ip0; const int upf = UpFactor; int l = l0 / upf; int lz = l0 - l * upf; if (upf == 3) { while (l != 0) { op[0] = *ip; op[1] = 0.0; op[2] = 0.0; ip++; op += upf; l--; } } else if (upf == 5) { while (l != 0) { op[0] = *ip; op[1] = 0.0; op[2] = 0.0; op[3] = 0.0; op[4] = 0.0; ip++; op += upf; l--; } } else { const size_t zc = (upf - 1) * sizeof(op[0]); while (l != 0) { *op = *ip; ip++; memset(op + 1, 0, zc); op += upf; l--; } } if (lz != 0) { *op = *ip; ip++; op++; UpSkip = upf - lz; while (--lz != 0) { *op = 0.0; op++; } } ip0 = ip; }

		void copyToOutput(int Offs, double*& op0, int b, int& l0) { if (Offs < 0) { if (Offs + b <= 0) { Offs += BlockLen2; } else { copyToOutput(Offs + BlockLen2, op0, -Offs, l0); b += Offs; Offs = 0; } } if (LatencyLeft != 0) { if (LatencyLeft >= b) { LatencyLeft -= b; return; } Offs += LatencyLeft; b -= LatencyLeft; LatencyLeft = 0; } const int df = DownFactor; if (DownShift > 0) { int Skip = Offs & (df - 1); if (Skip > 0) { Skip = df - Skip; b -= Skip; Offs += Skip; } if (b > 0) { b = (b + df - 1) >> DownShift; memcpy(op0, &CurOutput[Offs >> DownShift], b * sizeof(op0[0])); op0 += b; l0 += b; } } else { if (df > 1) { const double* ip = &CurOutput[Offs + DownSkip]; int l = (b + df - 1 - DownSkip) / df; DownSkip += l * df - b; double* op = op0; l0 += l; op0 += l; while (l > 0) { *op = *ip; ip += df; op++; l--; } } else { memcpy(op0, &CurOutput[Offs], b * sizeof(op0[0])); op0 += b; l0 += b; } } }

		template< typename T >
		void mirrorInputSpectrum(T* const p) { const int bl1 = BlockLen2 >> UpShift; const int bl2 = bl1 + bl1; int i; for (i = bl1 + 2; i < bl2; i += 2) { p[i] = p[bl2 - i]; p[i + 1] = -p[bl2 - i + 1]; } p[bl1] = p[1]; p[bl1 + 1] = (T)0; p[1] = p[0]; for (i = 1; i < UpShift; i++) { const int z = bl1 << i; memcpy(&p[z], p, z * sizeof(p[0])); p[z + 1] = (T)0; } }
	};

} // namespace r8b

namespace r8b {

	class CDSPResampler : public CDSPProcessor
	{
	public:

		CDSPResampler(
			const double SrcSampleRate,
			const double DstSampleRate,
			const int    aMaxInLen,
			const double ReqTransBand = 2.0,
			const double ReqAtten = 206.91,
			const EDSPFilterPhaseResponse ReqPhase = fprLinearPhase
		)
			: StepCapacity(0)
			, StepCount(0)
			, MaxInLen(aMaxInLen)
			, CurMaxOutLen(aMaxInLen)
			, LatencyFrac(0.0)
		{
			R8BASSERT(SrcSampleRate > 0.0);
			R8BASSERT(DstSampleRate > 0.0);
			R8BASSERT(MaxInLen > 0);

			R8BCONSOLE("* CDSPResampler: src=%.1f dst=%.1f len=%i tb=%.1f "
				"att=%.2f ph=%i\n", SrcSampleRate, DstSampleRate, aMaxInLen,
				ReqTransBand, ReqAtten, (int)ReqPhase);


			if (SrcSampleRate == DstSampleRate) { return; }

			TmpBufCapacities[0] = 0;
			TmpBufCapacities[1] = 0;
			CurTmpBuf = 0;



			// Try some common efficient ratios requiring only a single step.

			const int CommonRatioCount = 5;
			const int CommonRatios[CommonRatioCount][2] = { { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 2 }, { 3, 4 } };
			// 1/2, 1/3, 2/3, 3/2, 3/4

			int i;
			for (i = 0; i < CommonRatioCount; i++)
			{
				const int num = CommonRatios[i][0];
				const int den = CommonRatios[i][1];

				if (SrcSampleRate * num == DstSampleRate * den)
				{
					addProcessor(new CDSPBlockConvolver(
						CDSPFIRFilterCache::getLPFilter(1.0 / (num > den ? num : den),
							ReqTransBand,
							ReqAtten,
							ReqPhase,
							num
						),
						num,
						den,
						LatencyFrac)
					);

					createTmpBuffers();
					return; // End CDSPResampler and return
				}
			}

			// Try whole-number power-of-2 or 3*power-of-2 upsampling.
			for (i = 2; i <= 3; i++)
			{
				bool WasFound = false;
				int c = 0;

				while (true)
				{	// Finds if dst is multiple of (2, 4, 8, 16, ...) or (3, 6, 12, 24, ...)
					const double NewSR = SrcSampleRate * (i << c);
					if (NewSR == DstSampleRate) { WasFound = true; break; }
					if (NewSR > DstSampleRate) { break; }
					c++;
				}

				if (WasFound) // if it is multiple of (2, 4, 8, 16, ...) or (3, 6, 12, 24, ...)
				{
					addProcessor(new CDSPBlockConvolver(
						CDSPFIRFilterCache::getLPFilter(1.0 / i, ReqTransBand, ReqAtten, ReqPhase, i),
						i,
						1,
						LatencyFrac)
					);

					const bool IsThird = (i == 3); // true if (3, 6, 12, 24, ...)

					// c is from 2^(c) or 3^(c). So, if x8 upsampling, we need c == 2 -> (0,1,2) == 3 upsamplers
					for (i = 0; i < c; i++) { addProcessor(new CDSPHBUpsampler(ReqAtten, i, IsThird, LatencyFrac)); }

					createTmpBuffers();
					return; // End CDSPResampler and return
				}
			} // End whole-number power-of-2 or 3*power-of-2 upsampling.


			// if( DstSampleRate * 2.0 > SrcSampleRate )
			// ex, src = 1.0, dst = 5.0 -> true
			// ex, src = 1.0, dst = 0.8 -> true
			// ex, src = 1.0, dst = 0.5 -> false
			// ex, src = 1.0, dst = 0.2 -> false
			// Upsampling or fractional downsampling down to 2X.
			// Try intermediate interpolated resampling with subsequent 2X or 3X upsampling.
			// Do not use intermediate interpolation if whole stepping is available as it performs very fast.
			// Add steps using intermediate interpolation.
			// DELETED

			// End upsampling


			// Use downsampling steps, including power-of-2 downsampling.

			double CheckSR = DstSampleRate * 4.0; // 4.0, 8.0, 16.0, 32.0,  64.0, 
			int    c = 0;              // x^(c), c = 0,   1,   2,    3,     4, ...
			double FinGain = 1.0;      //            1.0, 0.5, 0.25, 0.125, 0.0625, ...

			while (CheckSR <= SrcSampleRate)
			{
				c++;
				CheckSR *= 2.0;
				FinGain *= 0.5;
			}
			// ex, src = 1.0, dst = 0.5
			// checksr = 0.5 * 4.0 = 2.0; -> No while
			// c = 0; FinGain = 1.0;
			// 
			// ex, src = 1.0, dst = 0.25
			// CheckSR = 0.25 * 4.0 = 1.0; -> while
			// c = 1; FinGain = 0.5;
			// CheckSR = 2.0; -> No while, end.

			const int SrcSRDiv = (1 << c); // how much you downsample, if 1.0 -> 0.5 : SrcSRDiv = 1 // if 1.0 -> 0.25 : SrcSRDiv = 2
			int downf;
			double NormFreq = 0.5;
			bool UseInterp = false;
			bool IsThird = false;

			for (downf = 2; downf <= 3; downf++)
			{
				// if : downsample power of 2 or 3
				// Dst = 0.25, SRDiv = 2.0, downf = 2.0 -> (0.25 * 2.0 * 2.0 = 1.0) == Src
				// Dst = 0.50, SRDiv = 1.0, downf = 2.0 -> (0.50 * 1.0 * 2.0 = 1.0) == Src
				if (DstSampleRate * SrcSRDiv * downf == SrcSampleRate)
				{
					NormFreq = 1.0 / downf;
					UseInterp = false;
					IsThird = (downf == 3);
					break;
				}
				// NormFreq = 0.5, if power of 2
			}

			for (i = 0; i < c; i++)
			{
				addProcessor(new CDSPHBDownsampler(ReqAtten, c - 1 - i, IsThird, LatencyFrac));
			}
			// Use fixed, very relaxed 2X downsampling filters, 
			// that at the final stage only guarantee stop-band between 0.75 and pi.
			// 0.5-0.75 range will be aliased to 0.25-0.5 range which will
			// then be filtered out by the final filter.

			addProcessor(new CDSPBlockConvolver(
				CDSPFIRFilterCache::getLPFilter(NormFreq, ReqTransBand, ReqAtten, ReqPhase, FinGain),
				1,
				downf,
				LatencyFrac)
			);

			createTmpBuffers();
			return;
		}

		virtual ~CDSPResampler() { int i; for (i = 0; i < StepCount; i++) { delete Steps[i]; } }

		virtual int getInLenBeforeOutPos(const int ReqOutPos) const { R8BASSERT(ReqOutPos >= 0); int ReqInSamples = ReqOutPos; int c = StepCount; while (--c >= 0) { ReqInSamples = Steps[c]->getInLenBeforeOutPos(ReqInSamples); } return(ReqInSamples); }
		int getInLenBeforeOutStart(const int ReqOutPos = 0) { R8BASSERT(ReqOutPos >= 0); int inc = 0; int outc = 0; while (true) { double ins = 0.0; double* op; outc += process(&ins, 1, op); if (outc > ReqOutPos) { clear(); return(inc); } inc++; } }
		int getInputRequiredForOutput(const int ReqOutSamples) const { if (ReqOutSamples < 1) { return(0); } return(getInLenBeforeOutPos(ReqOutSamples - 1) + 1); }

		virtual int    getLatency() const { return(0); }
		virtual double getLatencyFrac() const { return(LatencyFrac); }
		virtual int    getMaxOutLen(const int/* MaxInLen */) const { return(CurMaxOutLen); }
		virtual void   clear() { int i; for (i = 0; i < StepCount; i++) { Steps[i]->clear(); } }

		virtual int process(double* ip0, int l, double*& op0) { R8BASSERT(l >= 0); double* ip = ip0; int i; for (i = 0; i < StepCount; i++) { double* op = TmpBufs[i & 1]; l = Steps[i]->process(ip, l, op); ip = op; } op0 = ip; return(l); }

		template< typename Tin, typename Tout >
		void oneshot(const Tin* ip, int iplen, Tout* op, int oplen)
		{
			CFixedBuffer< double > Buf(MaxInLen);
			bool IsZero = false;

			while (oplen > 0)
			{
				int     rc;
				double* p;
				int     i;

				if (iplen == 0)
				{
					rc = MaxInLen;
					p = &Buf[0];

					if (!IsZero) { IsZero = true; memset(p, 0, MaxInLen * sizeof(p[0])); }
				}
				else
				{
					rc = (((iplen) < (MaxInLen)) ? (iplen) : (MaxInLen));

					if (sizeof(Tin) == sizeof(double)) { p = (double*)ip; }
					else { p = &Buf[0]; for (i = 0; i < rc; i++) { p[i] = ip[i]; } }

					ip += rc;
					iplen -= rc;
				}

				double* op0;
				int wc = process(p, rc, op0);
				wc = (((oplen) < (wc)) ? (oplen) : (wc));

				for (i = 0; i < wc; i++)
				{
					op[i] = (Tout)op0[i];
				}

				op += wc;
				oplen -= wc;
			} // End : while (oplen > 0)

			clear();
		} // End : oneShot

	private:
		CFixedBuffer< CDSPProcessor* > Steps;               ///< Array of processing steps.
		int                            StepCapacity;        ///< The capacity of the Steps array.
		int                            StepCount;           ///< The number of created processing steps.
		int                            MaxInLen;            ///< Maximal input length.
		CFixedBuffer< double >         TmpBufAll;           ///< Buffer containing both temporary buffers.
		double* TmpBufs[2];          ///< Temporary output buffers.
		int                            TmpBufCapacities[2]; ///< Capacities of temporary buffers, updated during processing steps building.
		int                            CurTmpBuf;           ///< Current temporary buffer.
		int                            CurMaxOutLen;        ///< Current maximal output length.
		double                         LatencyFrac;         ///< Current fractional latency. After object's construction, equals to the remaining fractional latency in the output.



		void addProcessor(CDSPProcessor* const Proc)
		{
			if (StepCount == StepCapacity) // Reallocate and increase Steps array's capacity.
			{
				const int NewCapacity = StepCapacity + 8;
				Steps.realloc(StepCapacity, NewCapacity);
				StepCapacity = NewCapacity;
			}

			LatencyFrac = Proc->getLatencyFrac();
			CurMaxOutLen = Proc->getMaxOutLen(CurMaxOutLen);

			if (CurMaxOutLen > TmpBufCapacities[CurTmpBuf]) { TmpBufCapacities[CurTmpBuf] = CurMaxOutLen; }

			CurTmpBuf ^= 1;

			Steps[StepCount] = Proc;
			StepCount++;
		}



		void createTmpBuffers()
		{
			const int ol = TmpBufCapacities[0] + TmpBufCapacities[1];
			if (ol > 0) { TmpBufAll.alloc(ol); TmpBufs[0] = &TmpBufAll[0]; TmpBufs[1] = &TmpBufAll[TmpBufCapacities[0]]; }

			R8BCONSOLE("* CDSPResampler: init done\n");
		}
	};




	class CDSPResampler16 : public CDSPResampler
	{
	public:
		CDSPResampler16(
			const double SrcSampleRate,
			const double DstSampleRate,
			const int aMaxInLen,
			const double ReqTransBand = 2.0
		)
			: CDSPResampler(
				SrcSampleRate,
				DstSampleRate,
				aMaxInLen,
				ReqTransBand,
				136.45,
				fprLinearPhase
			)
		{
		}
	};

	class CDSPResampler16IR : public CDSPResampler
	{
	public:
		CDSPResampler16IR(
			const double SrcSampleRate,
			const double DstSampleRate,
			const int aMaxInLen,
			const double ReqTransBand = 2.0
		)
			: CDSPResampler(
				SrcSampleRate,
				DstSampleRate,
				aMaxInLen,
				ReqTransBand,
				109.56,
				fprLinearPhase
			)
		{
		}
	};

	class CDSPResampler24 : public CDSPResampler
	{
	public:
		CDSPResampler24(
			const double SrcSampleRate,
			const double DstSampleRate,
			const int aMaxInLen,
			const double ReqTransBand = 2.0
		)
			: CDSPResampler(
				SrcSampleRate,
				DstSampleRate,
				aMaxInLen,
				ReqTransBand,
				180.15,
				fprLinearPhase
			)
		{
		}
	};

} // namespace r8b




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
	typedef Downsampler2xFpu <coef_2x_1_num>  dnSample_2x_1;
	typedef Downsampler2xFpu <coef_4x_1_num>  dnSample_4x_1;
	typedef Downsampler2xFpu <coef_4x_2_num>  dnSample_4x_2;
	typedef Downsampler2xFpu <coef_8x_1_num>  dnSample_8x_1;
	typedef Downsampler2xFpu <coef_8x_2_num>  dnSample_8x_2;
	typedef Downsampler2xFpu <coef_8x_3_num>  dnSample_8x_3;
} 
//------------------------------------------------------------------------
// HIIR library End
//------------------------------------------------------------------------



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
		void OS_stages(bool isUp, long remaining);
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
		Band_Split      Band_Split_L, Band_Split_R;
		overSample      fParamOS, fParamOSOld;
		uint32          fpdL, fpdR;


		// VU metering ----------------------------------------------------------------
		Vst::ParamValue fInVuPPML, fInVuPPMR;
		Vst::ParamValue fOutVuPPML, fOutVuPPMR;
		Vst::ParamValue fInVuPPMLOld, fInVuPPMROld;
		Vst::ParamValue fOutVuPPMLOld, fOutVuPPMROld;
		Vst::ParamValue fMeter, fMeterOld;
		Vst::ParamValue Meter = 0.0;

		
		// Buffers ------------------------------------------------------------------
		Steinberg::Vst::Sample64** in_0;
		Steinberg::Vst::Sample64** in_1;
		Steinberg::Vst::Sample64** in_2;
		Steinberg::Vst::Sample64** in_3;
		Steinberg::Vst::Sample64** out_0;
		Steinberg::Vst::Sample64** out_1;
		Steinberg::Vst::Sample64** out_2;
		Steinberg::Vst::Sample64** out_3;

		// Oversamplers ------------------------------------------------------------------
		hiir::upSample_2x_1   upSample_2x_1[2];
		hiir::upSample_4x_1   upSample_4x_1[2];
		hiir::upSample_4x_2   upSample_4x_2[2];
		hiir::upSample_8x_1   upSample_8x_1[2];
		hiir::upSample_8x_2   upSample_8x_2[2];
		hiir::upSample_8x_3   upSample_8x_3[2];
		hiir::dnSample_2x_1   dnSample_2x_1[2];
		hiir::dnSample_4x_1   dnSample_4x_1[2];
		hiir::dnSample_4x_2   dnSample_4x_2[2];
		hiir::dnSample_8x_1   dnSample_8x_1[2];
		hiir::dnSample_8x_2   dnSample_8x_2[2];
		hiir::dnSample_8x_3   dnSample_8x_3[2];

		#define maxSample 32768

		r8b::CPtrKeeper< r8b::CDSPResampler* > OS_Lin[4];
	};
} // namespace yg331
//------------------------------------------------------------------------
// VST3 plugin End
//------------------------------------------------------------------------