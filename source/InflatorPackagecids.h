//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace yg331 {
//------------------------------------------------------------------------
static const Steinberg::FUID kInflatorPackageProcessorUID (0xA6B5CA9F, 0x4C4F5B93, 0x88F83777, 0x4504BD37);
static const Steinberg::FUID kInflatorPackageControllerUID (0x8565C317, 0x780F5A1E, 0x8E35F396, 0xB6745FC7);

#define InflatorPackageVST3Category "Fx"

//------------------------------------------------------------------------
} // namespace yg331

enum {
	kParamInput = 0,	// Input
	kParamEffect,		// Effect
	kParamCurve,		// Curve
	kParamClip,			// Clip
	kParamOutput,		// Output
	kParamInVuPPML,		// Input VuPPM
	kParamInVuPPMR,
	kParamOutVuPPML,	// Output VuPPM
	kParamOutVuPPMR,
	kParamBypass,		// ByPass
	kParamOS,
	kParamSplit,
	kParamZoom,
};

#include <vector>
namespace yg331 {
	//------------------------------------------------------------------------	
	struct ZoomFactor {
		const Steinberg::tchar* title;
		double factor;

		ZoomFactor(const Steinberg::tchar* title, double factor) : title(title), factor(factor) {}
	};
	typedef std::vector<ZoomFactor> ZoomFactorVector;

	typedef enum {
		overSample_1x,
		overSample_2x,
		overSample_4x,
		overSample_8x,
		overSample_num = 3
	} overSample;

	typedef struct _SVF {
		Steinberg::Vst::Sample64 C = 0.0;
		Steinberg::Vst::Sample64 R = 0.0;
		Steinberg::Vst::Sample64 I = 0.0;
	} SVF;

	typedef struct _BS {
		SVF LP;
		SVF HP;
		Steinberg::Vst::Sample64 G  = 0.0;
		Steinberg::Vst::Sample64 GR = 0.0;
	} Band_Split;

	static const Steinberg::Vst::ParamValue
		init_Input     = 0.5,
		init_Effect    = 1.0,
		init_Curve     = 0.5,
		init_Output    = 1.0,
		init_VU        = 0.0,
		init_Zoom      = 2.0 / 6.0;

	static const Steinberg::Vst::Sample64
		init_curvepct  = init_Curve - 0.5,
		init_curveA    = 1.5 + init_curvepct,
		init_curveB    = -(init_curvepct + init_curvepct),
		init_curveC    = init_curvepct - 0.5,
		init_curveD    = 0.0625 - init_curvepct * 0.25 + (init_curvepct * init_curvepct) * 0.25;

	static const bool
		init_Clip      = false,
		init_Bypass    = false,
		init_Split     = false;

	static const overSample
		init_OS        = overSample_1x;

	//------------------------------------------------------------------------
} // namespace yg331
