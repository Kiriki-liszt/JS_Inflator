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
	kParamSplit
};

namespace yg331 {
	//------------------------------------------------------------------------	
	typedef enum {
		overSample_1x,
		overSample_2x,
		overSample_4x,
		overSample_8x,
		overSample_num = 3
	} overSample;

	static const Steinberg::Vst::ParamValue
		init_Input = 0.5,
		init_Effect = 1.0,
		init_Curve = 0.5,
		init_Output = 1.0,
		init_InVuPPML = 0.0,
		init_InVuPPMR = 0.0,
		init_OutVuPPML = 0.0,
		init_OutVuPPMR = 0.0;

	static const bool
		init_Clip = false,
		init_Bypass = false,
		init_Split = false;

	static const overSample
		init_OS = overSample_1x;

	//------------------------------------------------------------------------
} // namespace yg331
