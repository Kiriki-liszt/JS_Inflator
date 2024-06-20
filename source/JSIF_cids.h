//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace yg331 {
//------------------------------------------------------------------------
static const Steinberg::FUID kJSIF_ProcessorUID  (0xA6B5CA9F, 0x4C4F5B93, 0x88F83777, 0x4504BD37);
static const Steinberg::FUID kJSIF_ControllerUID (0x8565C317, 0x780F5A1E, 0x8E35F396, 0xB6745FC7);

#define JSIF_VST3Category "Fx"
enum {
    kParamInput = 0,    // Input
    kParamEffect,        // Effect
    kParamCurve,        // Curve
    kParamClip,            // Clip
    kParamOutput,        // Output
    kInVuPPML,        // Input VuPPM
    kInVuPPMR,
    kOutVuPPML,    // Output VuPPM
    kOutVuPPMR,
    kMeter,
    kParamOS,
    kParamSplit,
    kParamZoom,
    kParamPhase,
    kParamIn,        // ByPass
    kParamBypass,
    kGuiSwitch = 1000
};
//------------------------------------------------------------------------
} // namespace yg331
