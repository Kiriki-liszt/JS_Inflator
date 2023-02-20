#pragma once

enum {
	kParamInput = 0,	// Input
	kParamEffect,		// Effect
	kParamCurve,		// Curve
	kParamClip,			// Clip
	kParamOutput,		// Output
	kParamInVuPPM,		// Input VuPPM
	kParamOutVuPPM,		// Output VuPPM
	kParamBypass		// ByPass
};

static constexpr double Unity = 1.0;
static constexpr double Factor = 0.7; // GainParamConverter
static constexpr double db_12p = 3.9810717055349722;
static constexpr double db_12m = 0.251188643150958;