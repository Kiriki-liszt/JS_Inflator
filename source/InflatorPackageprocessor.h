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
#include <cmath>
#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#include <memory>
#include <atomic>
#include <queue>

using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	//  Non-Linear Functions (StdNL, ADAA1, ADAA2)
	//------------------------------------------------------------------------
	class StandardNL
	{
	public:
		StandardNL() = default;
		virtual ~StandardNL() {}

		virtual void initialise() {}
		virtual void prepare() {}
		virtual inline double process(double x) noexcept
		{
			return func(x);
		}

		virtual double func(double) const noexcept = 0;
		virtual double func_AD1(double) const noexcept = 0;
		virtual double func_AD2(double) const noexcept = 0;

		inline double CubicInterpolate(
			double y0, double y1,
			double y2, double y3,
			double mu = 0.5)
		{
			double a0, a1, a2, a3, mu2;

			mu2 = mu * mu;
			a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
			a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
			a2 = -0.5 * y0 + 0.5 * y2;
			a3 = y1;

			return(a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
		}

		static constexpr size_t A = 16;

		inline double kernel(double x)
		{
			if (fabs(x) < 1e-7)
				return 1;
			return A * sin(M_PI * x) * sin(M_PI * x / A) / (M_PI * M_PI * x * x);
		}

		/*
		const double LQ_1[A * 2] = {
			kernel( 15.25), kernel( 14.25), kernel( 13.25), kernel( 12.25),
			kernel( 11.25), kernel( 10.25), kernel(  9.25), kernel(  8.25),
			kernel(  7.25), kernel(  6.25), kernel(  5.25), kernel(  4.25),
			kernel(  3.25), kernel(  2.25), kernel(  1.25), kernel(  0.25),
			kernel( -0.75), kernel( -1.75), kernel( -2.75), kernel( -3.75),
			kernel( -4.75), kernel( -5.75), kernel( -6.75), kernel( -7.75),
			kernel( -8.75), kernel( -9.75), kernel(-10.75), kernel(-11.75),
			kernel(-12.75), kernel(-13.75), kernel(-14.75), kernel(-15.75)
		};
		const double LQ_2[A * 2] = {
			kernel( 15.50), kernel( 14.50), kernel( 13.50), kernel( 12.50),
			kernel( 11.50), kernel( 10.50), kernel(  9.50), kernel(  8.50),
			kernel(  7.50), kernel(  6.50), kernel(  5.50), kernel(  4.50),
			kernel(  3.50), kernel(  2.50), kernel(  1.50), kernel(  0.50),
			kernel( -0.50), kernel( -1.50), kernel( -2.50), kernel( -3.50),
			kernel( -4.50), kernel( -5.50), kernel( -6.50), kernel( -7.50),
			kernel( -8.50), kernel( -9.50), kernel(-10.50), kernel(-11.50),
			kernel(-12.50), kernel(-13.50), kernel(-14.50), kernel(-15.50)
		};
		const double LQ_3[A * 2] = {
			kernel( 15.75), kernel( 14.75), kernel( 13.75), kernel( 12.75),
			kernel( 11.75), kernel( 10.75), kernel(  9.75), kernel(  8.75),
			kernel(  7.75), kernel(  6.75), kernel(  5.75), kernel(  4.75),
			kernel(  3.75), kernel(  2.75), kernel(  1.75), kernel(  0.75),
			kernel( -0.25), kernel( -1.25), kernel( -2.25), kernel( -3.25),
			kernel( -4.25), kernel( -5.25), kernel( -6.25), kernel( -7.25),
			kernel( -8.25), kernel( -9.25), kernel(-10.25), kernel(-11.25),
			kernel(-12.25), kernel(-13.25), kernel(-14.25), kernel(-15.25)
		};
		*/
		const double LQ_0[A * 2] = {
			kernel( 15.125), kernel( 14.125), kernel( 13.125), kernel( 12.125),
			kernel( 11.125), kernel( 10.125), kernel(  9.125), kernel(  8.125),
			kernel(  7.125), kernel(  6.125), kernel(  5.125), kernel(  4.125),
			kernel(  3.125), kernel(  2.125), kernel(  1.125), kernel(  0.125),
			kernel( -0.875), kernel( -1.875), kernel( -2.875), kernel( -3.875),
			kernel( -4.875), kernel( -5.875), kernel( -6.875), kernel( -7.875),
			kernel( -8.875), kernel( -9.875), kernel(-10.875), kernel(-11.875),
			kernel(-12.875), kernel(-13.875), kernel(-14.875), kernel(-15.875)
		};
		const double LQ_1[A * 2] = {
			kernel( 15.375), kernel( 14.375), kernel( 13.375), kernel( 12.375),
			kernel( 11.375), kernel( 10.375), kernel(  9.375), kernel(  8.375),
			kernel(  7.375), kernel(  6.375), kernel(  5.375), kernel(  4.375),
			kernel(  3.375), kernel(  2.375), kernel(  1.375), kernel(  0.375),
			kernel( -0.625), kernel( -1.625), kernel( -2.625), kernel( -3.625),
			kernel( -4.625), kernel( -5.625), kernel( -6.625), kernel( -7.625),
			kernel( -8.625), kernel( -9.625), kernel(-10.625), kernel(-11.625),
			kernel(-12.625), kernel(-13.625), kernel(-14.625), kernel(-15.625)
		};
		const double LQ_2[A * 2] = {
			kernel( 15.625), kernel( 14.625), kernel( 13.625), kernel( 12.625),
			kernel( 11.625), kernel( 10.625), kernel(  9.625), kernel(  8.625),
			kernel(  7.625), kernel(  6.625), kernel(  5.625), kernel(  4.625),
			kernel(  3.625), kernel(  2.625), kernel(  1.625), kernel(  0.625),
			kernel( -0.375), kernel( -1.375), kernel( -2.375), kernel( -3.375),
			kernel( -4.375), kernel( -5.375), kernel( -6.375), kernel( -7.375),
			kernel( -8.375), kernel( -9.375), kernel(-10.375), kernel(-11.375),
			kernel(-12.375), kernel(-13.375), kernel(-14.375), kernel(-15.375)
		};
		const double LQ_3[A * 2] = {
			kernel( 15.875), kernel( 14.875), kernel( 13.875), kernel( 12.875),
			kernel( 11.875), kernel( 10.875), kernel(  9.875), kernel(  8.875),
			kernel(  7.875), kernel(  6.875), kernel(  5.875), kernel(  4.875),
			kernel(  3.875), kernel(  2.875), kernel(  1.875), kernel(  0.875),
			kernel( -0.125), kernel( -1.125), kernel( -2.125), kernel( -3.125),
			kernel( -4.125), kernel( -5.125), kernel( -6.125), kernel( -7.125),
			kernel( -8.125), kernel( -9.125), kernel(-10.125), kernel(-11.125),
			kernel(-12.125), kernel(-13.125), kernel(-14.125), kernel(-15.125)
		};
#define Lanczos_Q0(x) \
		x[ 0] * LQ_0[ 0] + x[ 1] * LQ_0[ 1] + x[ 2] * LQ_0[ 2] + x[ 3] * LQ_0[ 3] + \
		x[ 4] * LQ_0[ 4] + x[ 5] * LQ_0[ 5] + x[ 6] * LQ_0[ 6] + x[ 7] * LQ_0[ 7] + \
		x[ 8] * LQ_0[ 8] + x[ 9] * LQ_0[ 9] + x[10] * LQ_0[10] + x[11] * LQ_0[11] + \
		x[12] * LQ_0[12] + x[13] * LQ_0[13] + x[14] * LQ_0[14] + x[15] * LQ_0[15] + \
		x[16] * LQ_0[16] + x[17] * LQ_0[17] + x[18] * LQ_0[18] + x[19] * LQ_0[19] + \
		x[20] * LQ_0[20] + x[21] * LQ_0[21] + x[22] * LQ_0[22] + x[23] * LQ_0[23] + \
		x[24] * LQ_0[24] + x[25] * LQ_0[25] + x[26] * LQ_0[26] + x[27] * LQ_0[27] + \
		x[28] * LQ_0[28] + x[29] * LQ_0[29] + x[30] * LQ_0[30] + x[31] * LQ_0[31]
#define Lanczos_Q1(x) \
		x[ 0] * LQ_1[ 0] + x[ 1] * LQ_1[ 1] + x[ 2] * LQ_1[ 2] + x[ 3] * LQ_1[ 3] + \
		x[ 4] * LQ_1[ 4] + x[ 5] * LQ_1[ 5] + x[ 6] * LQ_1[ 6] + x[ 7] * LQ_1[ 7] + \
		x[ 8] * LQ_1[ 8] + x[ 9] * LQ_1[ 9] + x[10] * LQ_1[10] + x[11] * LQ_1[11] + \
		x[12] * LQ_1[12] + x[13] * LQ_1[13] + x[14] * LQ_1[14] + x[15] * LQ_1[15] + \
		x[16] * LQ_1[16] + x[17] * LQ_1[17] + x[18] * LQ_1[18] + x[19] * LQ_1[19] + \
		x[20] * LQ_1[20] + x[21] * LQ_1[21] + x[22] * LQ_1[22] + x[23] * LQ_1[23] + \
		x[24] * LQ_1[24] + x[25] * LQ_1[25] + x[26] * LQ_1[26] + x[27] * LQ_1[27] + \
		x[28] * LQ_1[28] + x[29] * LQ_1[29] + x[30] * LQ_1[30] + x[31] * LQ_1[31]
#define Lanczos_Q2(x) \
		x[ 0] * LQ_2[ 0] + x[ 1] * LQ_2[ 1] + x[ 2] * LQ_2[ 2] + x[ 3] * LQ_2[ 3] + \
		x[ 4] * LQ_2[ 4] + x[ 5] * LQ_2[ 5] + x[ 6] * LQ_2[ 6] + x[ 7] * LQ_2[ 7] + \
		x[ 8] * LQ_2[ 8] + x[ 9] * LQ_2[ 9] + x[10] * LQ_2[10] + x[11] * LQ_2[11] + \
		x[12] * LQ_2[12] + x[13] * LQ_2[13] + x[14] * LQ_2[14] + x[15] * LQ_2[15] + \
		x[16] * LQ_2[16] + x[17] * LQ_2[17] + x[18] * LQ_2[18] + x[19] * LQ_2[19] + \
		x[20] * LQ_2[20] + x[21] * LQ_2[21] + x[22] * LQ_2[22] + x[23] * LQ_2[23] + \
		x[24] * LQ_2[24] + x[25] * LQ_2[25] + x[26] * LQ_2[26] + x[27] * LQ_2[27] + \
		x[28] * LQ_2[28] + x[29] * LQ_2[29] + x[30] * LQ_2[30] + x[31] * LQ_2[31]
#define Lanczos_Q3(x) \
		x[ 0] * LQ_3[ 0] + x[ 1] * LQ_3[ 1] + x[ 2] * LQ_3[ 2] + x[ 3] * LQ_3[ 3] + \
		x[ 4] * LQ_3[ 4] + x[ 5] * LQ_3[ 5] + x[ 6] * LQ_3[ 6] + x[ 7] * LQ_3[ 7] + \
		x[ 8] * LQ_3[ 8] + x[ 9] * LQ_3[ 9] + x[10] * LQ_3[10] + x[11] * LQ_3[11] + \
		x[12] * LQ_3[12] + x[13] * LQ_3[13] + x[14] * LQ_3[14] + x[15] * LQ_3[15] + \
		x[16] * LQ_3[16] + x[17] * LQ_3[17] + x[18] * LQ_3[18] + x[19] * LQ_3[19] + \
		x[20] * LQ_3[20] + x[21] * LQ_3[21] + x[22] * LQ_3[22] + x[23] * LQ_3[23] + \
		x[24] * LQ_3[24] + x[25] * LQ_3[25] + x[26] * LQ_3[26] + x[27] * LQ_3[27] + \
		x[28] * LQ_3[28] + x[29] * LQ_3[29] + x[30] * LQ_3[30] + x[31] * LQ_3[31]

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
			double A[256] = { 0, };
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

		typedef struct _Flt {
			double coef alignas(16)[256] = { 0, };
			double buff alignas(16)[256] = { 0, };
		} Flt;

		Flt dnSample_41;
		Flt dnSample_42;
	private:
	};


	namespace ADAAConst
	{
		constexpr double TOL = 1.0e-9;
	}

	/** Base class for 1st-order ADAA */
	class ADAA1 : public StandardNL
	{
	public:
		//ADAA1() = default;
		static constexpr size_t tap1  = 59;
		static constexpr size_t tap2  = 127;
		static constexpr size_t htap1 = 29;
		static constexpr size_t htap2 = 63;
		ADAA1() { 
			calcFilter( 96000.0, 0.0, 24000.0, tap1, 100.00, dnSample_41.coef);
			calcFilter(192000.0, 0.0, 24000.0, tap2, 100.00, dnSample_42.coef);
		}
		virtual ~ADAA1() {}

		void prepare() override
		{
			    x1 = 0.0;
			ad1_x1 = 0.0;
		}

		inline double process(double x) noexcept override
		{
			// both x and y interpolation method take effect
			// interpolation at ill condition is better with simple linear - less quantization noise
			const size_t mm = sizeof(double) * ((A * 2) - 1);
			double x_ = x;

			/*
			const size_t mm = sizeof(double) * (tap);
			memmove(upSample_42.buff + 1, upSample_42.buff, mm);
			upSample_42.buff[0] = x;

			double _x3 = 0.0;
			for (int i = 0; i < tap; i++) {
				_x3 += upSample_42.buff[i] * upSample_42.coef[i];
			}
			x = _x3;
			*/


			memmove(xx + 1, xx, mm); xx[0] = x_;

			x = Lanczos_Q3(xx);

			bool illCondition = std::abs(x - x1) < ADAAConst::TOL;
			double ad1_x = nlFunc_AD1(x);

			double _y3 = illCondition ?
				nlFunc(0.5 * (x + x1)) :
				(ad1_x - ad1_x1) / (x - x1);
			ad1_x1 = ad1_x;
			x1 = x;

			// memmove(yy + 1, yy, mm); yy[0] = _y3;
			// double out = Lanczos_Q2(yy);

			/*
			memmove(upSample_42.buff + 1, upSample_42.buff, mm);
			upSample_42.buff[0] = 0.0;

			double _x2 = 0.0;
			for (int i = 0; i < tap; i++) {
				_x2 += upSample_42.buff[i] * upSample_42.coef[i];
			}
			x = _x2;
			*/
			
			x = Lanczos_Q2(xx);
			
			illCondition = std::abs(x - x1) < ADAAConst::TOL;
			ad1_x = nlFunc_AD1(x);

			double _y2 = illCondition ?
				nlFunc(0.5 * (x + x1)) :
				(ad1_x - ad1_x1) / (x - x1);
			ad1_x1 = ad1_x;
			x1 = x;

			//memmove(yy + 1, yy, mm); yy[0] = _y2;

			/*
			memmove(upSample_42.buff + 1, upSample_42.buff, mm);
			upSample_42.buff[0] = 0.0;

			double _x1 = 0.0;
			for (int i = 0; i < tap; i++) {
				_x1 += upSample_42.buff[i] * upSample_42.coef[i];
			}
			x = _x1;
			*/
			x = Lanczos_Q1(xx);
			
			illCondition = std::abs(x - x1) < ADAAConst::TOL;
			ad1_x = nlFunc_AD1(x);

			double _y1 = illCondition ?
				nlFunc(0.5 * (x + x1)) :
				(ad1_x - ad1_x1) / (x - x1);
			ad1_x1 = ad1_x;
			x1 = x;

			//memmove(yy + 1, yy, mm); yy[0] = _y1;

			/*
			memmove(upSample_42.buff + 1, upSample_42.buff, mm);
			upSample_42.buff[0] = 0.0;

			double _x0 = 0.0;
			for (int i = 0; i < tap; i++) {
				_x0 += upSample_42.buff[i] * upSample_42.coef[i];
			}
			x = _x0;
			*/

			
			x = Lanczos_Q0(xx);
			
			illCondition = std::abs(x - x1) < ADAAConst::TOL;
			ad1_x = nlFunc_AD1(x);

			double _y0 = illCondition ?
				nlFunc(0.5 * (x + x1)) :
				(ad1_x - ad1_x1) / (x - x1);
			ad1_x1 = ad1_x;
			x1 = x;

			//memmove(yy + 1, yy, mm); yy[0] = _y0;
			/*
			const size_t mm1 = sizeof(double) * (tap1);
			memmove(dnSample_41.buff + 2, dnSample_41.buff, mm1);
			dnSample_41.buff[1] = _y3;
			dnSample_41.buff[0] = _y2;

			double out1 = dnSample_41.buff[htap1] * dnSample_41.coef[htap1];
			for (int i = 0; i < htap1; i++) {
				out1 += (dnSample_41.buff[i] + dnSample_41.buff[tap1 - 1 - i]) * dnSample_41.coef[i];
			}

			memmove(dnSample_41.buff + 2, dnSample_41.buff, mm1);
			dnSample_41.buff[1] = _y1;
			dnSample_41.buff[0] = _y0;

			double out0 = dnSample_41.buff[htap1] * dnSample_41.coef[htap1];
			for (int i = 0; i < htap1; i++) {
				out0 += (dnSample_41.buff[i] + dnSample_41.buff[tap1 - 1 - i]) * dnSample_41.coef[i];
			}

			const size_t mm2 = sizeof(double) * (tap2);
			memmove(dnSample_42.buff + 2, dnSample_42.buff, mm2);
			dnSample_42.buff[1] = out1;
			dnSample_42.buff[0] = out0;

			double out = dnSample_42.buff[htap2] * dnSample_42.coef[htap2];
			for (int i = 0; i < htap2; i++) {
				out += (dnSample_42.buff[i] + dnSample_42.buff[tap2 - 1 - i]) * dnSample_42.coef[i];
			}
			*/
			const size_t mm2 = sizeof(double) * (tap2);
			memmove(dnSample_42.buff + 4, dnSample_42.buff, mm2);
			dnSample_42.buff[3] = _y3;
			dnSample_42.buff[2] = _y2;
			dnSample_42.buff[1] = _y1;
			dnSample_42.buff[0] = _y0;
			double out = dnSample_42.buff[htap2] * dnSample_42.coef[htap2];
			for (int i = 0; i < htap2; i++) {
				out += (dnSample_42.buff[i] + dnSample_42.buff[tap2 - 1 - i]) * dnSample_42.coef[i]; // (dnSample_42.buff[i] + dnSample_42.buff[tap2 - 1 - i])
			}
			return out;
		}

	protected:
		virtual inline double nlFunc    (double x) const noexcept { return func(x); }
		virtual inline double nlFunc_AD1(double x) const noexcept { return func_AD1(x); }
		double xx[A * 2] = { 0.0, };
		double yy[A * 2] = { 0.0, };
		double     x1 = 0.0;
		double ad1_x1 = 0.0;
	private:
	};


	class ADAA2 : public ADAA1
	{
	public:
		ADAA2() = default;
		virtual ~ADAA2() {}

		void prepare() override
		{
			ADAA1::prepare();
			x2 = 0.0;
			ad2_x0 = 0.0;
			ad2_x1 = 0.0;
			d2 = 0.0;
		}

		inline double process(double x) noexcept override
		{
			bool illCondition = std::abs(x - x2) < ADAAConst::TOL;
			double d1 = calcD1(x);

			double y = illCondition ?
				fallback(x) :
				(2.0 / (x - x2)) * (d1 - d2);

			// update state
			d2 = d1;
			x2 = x1;
			x1 = x;
			ad2_x1 = ad2_x0;

			return y;
		}

	protected:
		virtual inline double nlFunc_AD2(double x) const noexcept { return func_AD2(x); }

		double x2 = 0.0;
		double ad2_x0 = 0.0;
		double ad2_x1 = 0.0;
		double d2 = 0.0;

	private:
		inline double calcD1(double x0) noexcept
		{
			bool illCondition = std::abs(x0 - x1) < ADAAConst::TOL;
			ad2_x0 = nlFunc_AD2(x0);

			double y = illCondition ?
				nlFunc_AD1(0.5 * (x0 + x1)) :
				(ad2_x0 - ad2_x1) / (x0 - x1);

			return y;
		}

		inline double fallback(double x) noexcept
		{
			double xBar = 0.5 * (x + x2);
			double delta = xBar - x1;

			bool illCondition = std::abs(delta) < ADAAConst::TOL;

			return illCondition ?
				nlFunc(0.5 * (xBar + x1)) :
				(2.0 / delta) * (nlFunc_AD1(xBar) + (ad2_x1 - nlFunc_AD2(xBar)) / delta);
		}
	};

	//------------------------------------------------------------------------
	//  what Non-Linear Processor shoud be able to do
	//------------------------------------------------------------------------
	class BaseNL
	{
	public:
		BaseNL() = default;
		virtual ~BaseNL() {}

		virtual void prepare(double) {};
		virtual void prepare(double, int) {};
		virtual void processBlock(double*, const int) {};

	private:
	};

	//------------------------------------------------------------------------
	//  BaseNL -> Inflator
	//  Type : StdNL, ADAA1, ADAA2
	//------------------------------------------------------------------------
	template <class Type>
	class Inflator : public BaseNL, public Type
	{
	public:
		Inflator()
		{
			Type::initialise();
		}

		virtual ~Inflator() {}

		void prepare(double param) override
		{
			updateCurve(param);
		}

		void prepare(double, int) override
		{
			Type::prepare();
		}

		void processBlock(double* x, const int nSamples) override
		{
			for (int n = 0; n < nSamples; ++n)
				x[n] = Type::process(x[n]);
		}

		void updateCurve(double fCurve) {
			curvepct = fCurve - 0.5;
			curveA = 1.5 + curvepct;
			curveB = -(curvepct + curvepct);
			curveC = curvepct - 0.5;
			curveD = 0.0625 - curvepct * 0.25 + (curvepct * curvepct) * 0.25;
		}

		inline double func(double x) const noexcept override
		{
			double s1 = std::abs(x);
			double s2 = s1 * s1;
			double s3 = s2 * s1;
			double s4 = s2 * s2;

			double inputSample = x;

			if      (s1 >= 2.0) inputSample = 0.0;
			else if (s1 >  1.0) inputSample = (2.0 * s1) - s2;
			else                inputSample = (curveA * s1) +
			                                  (curveB * s2) +
			                                  (curveC * s3) -
			                                  (curveD * (s2 - (2.0 * s3) + s4));
			inputSample *= signum(x);

			return inputSample;
		}

		/** First antiderivative of hard clipper */
		inline double func_AD1(double x) const noexcept override
		{
			double s1 = std::abs(x);
			double s2 = s1 * s1;
			double s3 = s2 * s1;
			double s4 = s2 * s2;
			double s5 = s3 * s2;

			double inputSample = 0.0;

			if      (s1 >= 2.0) inputSample = 4.0/3.0 + (-4.0 * curvepct * curvepct + 44.0 * curvepct - 21.0) / (480.0);
			else if (s1 >  1.0) inputSample = s2 - (s3 / 3.0) + (-4.0 * curvepct * curvepct + 44.0 * curvepct - 21.0) / (480.0);
			else                inputSample = (30.0 * curveA * s2 +
			                                   20.0 * curveB * s3 +
			                                   15.0 * curveC * s4 -
			                                          curveD * (20.0 * s3 - (30.0 * s4) + 12.0 * s5)) / 60.0;
			//inputSample *= signum(x);

			return inputSample;
		}

		/** Second antiderivative of hard clipper */
		inline double func_AD2(double x) const noexcept override
		{
			double s1 = std::abs(x);
			double s2 = s1 * s1;
			double s3 = s2 * s1;
			double s4 = s2 * s2;
			double s5 = s3 * s2;
			double s6 = s3 * s3;

			double inputSample = x;

			if (s1 >= 2.0) inputSample = 0.0;
			else if (s1 > 1.0) inputSample = (s3 - 0.25 * s4) / 3.0;
			else               inputSample = (10.0 * curveA * s3 +
			                                   5.0 * curveB * s4 +
			                                   3.0 * curveC * s5 -
			                                         curveD * (5.0 * s4 - 6.0 * s5 + 2.0 * s6)) / 60.0;

			inputSample *= signum(x);

			return inputSample;
		}

	private:
		/** Signum function to determine the sign of the input. */
		template <typename T>
		inline int signum(T val) const noexcept
		{
			return (T(0) < val) - (val < T(0));
		}
		Vst::Sample64   curvepct = 0.0;
		Vst::Sample64   curveA = 0.0;
		Vst::Sample64   curveB = 0.0;
		Vst::Sample64   curveC = 0.0;
		Vst::Sample64   curveD = 0.0;
	};

	//------------------------------------------------------------------------
	//  Non-Linear Processors
	//------------------------------------------------------------------------
	class NLProcessor
	{
	public:
		NLProcessor(double* param) 
		{
			NLSet Procs;
			Procs.push_back(std::make_unique<Inflator<StandardNL>>());
			Procs.push_back(std::make_unique<Inflator<ADAA1>>());
			Procs.push_back(std::make_unique<Inflator<ADAA2>>());
			nlProcs.push_back(std::move(Procs));

			ParamListener = param;
		};

		void prepare(double sampleRate, int samplesPerBlock) {

			for (auto& nlSet : nlProcs)
			{
				for (auto& nl : nlSet)
				{
					nl->prepare(sampleRate, samplesPerBlock);
				}
			}
		};

		void processBlock(double* block, int numSamples) {

			auto curCurve = *ParamListener;

			if (curCurve != prevCurve)
			{
				for (auto& nlSet : nlProcs)
				{
					for (auto& nl : nlSet)
					{
						nl->prepare(curCurve);
					}
				}
				prevCurve = curCurve;
			}

			auto& curNLSet = nlProcs[0]; // (int)nlParam->load()
			auto& curNLProc = curNLSet[1]; // (int)adaaParam->load()
			curNLProc->processBlock(block, numSamples);
		};

		// float getLatencySamples() const noexcept;

	private:
		// std::atomic<float>* adaaParam = nullptr;
		// std::atomic<float>* nlParam = nullptr;
		std::atomic<double*> ParamListener = nullptr;
		double prevCurve = 0.0;
		
		using NLSet = std::vector<std::unique_ptr<BaseNL>>;
		std::vector<NLSet> nlProcs;
	};

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
			double coef alignas(16) [maxTap] = { 0, };
			double buff alignas(16) [maxTap] = { 0, };
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

		const int base_latency = 31;
		std::vector<std::unique_ptr<NLProcessor>> nlProc;
	};
} // namespace yg331
//------------------------------------------------------------------------
// VST3 plugin End
//------------------------------------------------------------------------
