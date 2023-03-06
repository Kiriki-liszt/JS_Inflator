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
	kParamOutVuPPML,		// Output VuPPM
	kParamOutVuPPMR,
	kParamBypass		// ByPass
};