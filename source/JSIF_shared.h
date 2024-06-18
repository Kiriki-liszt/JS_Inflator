#pragma once

#include "pluginterfaces/vst/vsttypes.h"

namespace yg331 {
//------------------------------------------------------------------------
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
	Steinberg::Vst::Sample64 G = 0.0;
	Steinberg::Vst::Sample64 GR = 0.0;
	Steinberg::Vst::SampleRate SR = 0.0;
} Band_Split;

static const Steinberg::Vst::ParamValue
init_Input = 0.5,
init_Effect = 0.0,
init_Curve = 0.5,
init_Output = 1.0,
init_VU = 0.0,
init_Zoom = 0.0 / 6.0;

static const Steinberg::Vst::Sample64
init_curvepct = init_Curve - 0.5,
init_curveA = 1.5 + init_curvepct,
init_curveB = -(init_curvepct + init_curvepct),
init_curveC = init_curvepct - 0.5,
init_curveD = 0.0625 - init_curvepct * 0.25 + (init_curvepct * init_curvepct) * 0.25;

static const bool
init_Clip = false,
init_Bypass = false,
init_In = true,
init_Split = false,
init_Phase = false; // Min phase

static const overSample
init_OS = overSample_1x;
//------------------------------------------------------------------------
} // namespace yg331
