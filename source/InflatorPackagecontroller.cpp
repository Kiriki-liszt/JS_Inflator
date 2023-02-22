//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "InflatorPackagecontroller.h"
#include "InflatorPackagecids.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include "pluginterfaces/base/ustring.h"


#include "base/source/fstreamer.h"

#include "param.h"

#include <cstdio>
#include <cmath>

using namespace Steinberg;

namespace yg331 {

	//------------------------------------------------------------------------
	// SliderParameter Declaration
	// example of custom parameter (overwriting to and fromString)
	//------------------------------------------------------------------------
	class SliderParameter : public Vst::RangeParameter
	{
	public:
		SliderParameter(UString256 title, Vst::ParamValue minPlain, Vst::ParamValue maxPlain, Vst::ParamValue defaultValuePlain, int32 flags, int32 id);

		Vst::ParamValue toPlain(Vst::ParamValue normValue) const SMTG_OVERRIDE;						/** Converts a normalized value to plain value (e.g. 0.5 to 10000.0Hz). */
		Vst::ParamValue toNormalized(Vst::ParamValue plainValue) const SMTG_OVERRIDE;				/** Converts a plain value to a normalized value (e.g. 10000 to 0.5). */

		void toString(Vst::ParamValue normValue, Vst::String128 string) const SMTG_OVERRIDE;
		bool fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const SMTG_OVERRIDE;
	};

	//------------------------------------------------------------------------
	// GainParameter Implementation
	//------------------------------------------------------------------------
	SliderParameter::SliderParameter(UString256 title, Vst::ParamValue minPlain, Vst::ParamValue maxPlain, Vst::ParamValue defaultValuePlain, int32 flags, int32 id)
	{
		Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(title);
		Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

		setMin(minPlain);
		setMax(maxPlain);

		info.flags = flags;
		info.id = id;
		info.stepCount = 0;
		info.defaultNormalizedValue = valueNormalized = toNormalized(defaultValuePlain);
		info.unitId = Vst::kRootUnitId;
	}
	//         mM, string
	// param,  db, gain
	//   0.5,   0,  1.0
	//     0, -12,  0.25
	//     1, +12,  3.98

	Vst::ParamValue SliderParameter::toPlain(Vst::ParamValue normValue) const
	{
		Vst::ParamValue plainValue = ((getMax() - getMin()) * normValue) + getMin();
		if (plainValue > getMax()) plainValue = getMax();
		else if (plainValue < getMin()) plainValue = getMin();
		return plainValue;
	}

	Vst::ParamValue SliderParameter::toNormalized(Vst::ParamValue plainValue) const
	{
		Vst::ParamValue normValue = (plainValue - getMin()) / (getMax() - getMin());
		if (normValue > 1.0) normValue = 1.0;
		else if (normValue < 0.0) normValue = 0.0;
		return normValue;
	}

	//------------------------------------------------------------------------
	void SliderParameter::toString(Vst::ParamValue normValue, Vst::String128 string) const
	{
		Vst::ParamValue plainValue = SliderParameter::toPlain(normValue);

		char text[32];
		snprintf(text, 32, "%.2f", plainValue);

		Steinberg::UString(string, 128).fromAscii(text);
	}

	//------------------------------------------------------------------------
	bool SliderParameter::fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const
	{
		String wrapper((Vst::TChar*)string); // don't know buffer size here!
		Vst::ParamValue plainValue;
		if (wrapper.scanFloat(plainValue))
		{
			normValue = SliderParameter::toNormalized(plainValue);
			return true;
		}
		return false;
	}









//------------------------------------------------------------------------
// InflatorPackageController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	int32 stepCount;
	int32 flags;
	int32 tag;
	Vst::ParamValue defaultVal;
	Vst::ParamValue minPlain;
	Vst::ParamValue maxPlain;
	Vst::ParamValue defaultPlain;

	tag = kParamInput;
	flags = Vst::ParameterInfo::kCanAutomate;
	minPlain = -12.0;
	maxPlain =  12.0;
	defaultPlain = 0.0;
	auto* gainParamIn = new SliderParameter(USTRING("Input"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(gainParamIn);

	tag = kParamOutput;
	flags = Vst::ParameterInfo::kCanAutomate;
	minPlain = -12.0;
	maxPlain =   0.0;
	defaultPlain = 0.0;
	auto* gainParamOut = new SliderParameter(USTRING("Output"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(gainParamOut);

	
	tag = kParamInVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain =  6.0;
	defaultPlain = -44.0;
	auto* InVuPPML = new SliderParameter(USTRING("InVuPPML"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(InVuPPML);
	tag = kParamInVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* InVuPPMR = new SliderParameter(USTRING("InVuPPMR"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(InVuPPMR);
	tag = kParamOutVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* OutVuPPML = new SliderParameter(USTRING("OutVuPPML"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(OutVuPPML);
	tag = kParamOutVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* OutVuPPMR = new SliderParameter(USTRING("OutVuPPMR"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(OutVuPPMR);
	

	// Here you could register some parameters

	tag = kParamEffect;
	stepCount = 0;
	defaultVal = 1.0;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Effect"), STR16("%"), stepCount, defaultVal, flags, tag);

	tag = kParamCurve;
	stepCount = 0;
	defaultVal = 0.5;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Curve"), nullptr, stepCount, defaultVal, flags, tag);

	tag = kParamClip;
	stepCount = 1;
	defaultVal = 0;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Clip"), nullptr, stepCount, defaultVal, flags, tag);

	/*
	tag = kParamInVuPPML;
	stepCount = 0;
	defaultVal = 0.0;
	flags = Vst::ParameterInfo::kIsReadOnly;
	parameters.addParameter(STR16("InVuPPML"), nullptr, stepCount, defaultVal, flags, tag);
	tag = kParamInVuPPMR;
	stepCount = 0;
	defaultVal = 0.0;
	flags = Vst::ParameterInfo::kIsReadOnly;
	parameters.addParameter(STR16("InVuPPMR"), nullptr, stepCount, defaultVal, flags, tag);

	tag = kParamOutVuPPML;
	stepCount = 0;
	defaultVal = 0.0;
	flags = Vst::ParameterInfo::kIsReadOnly;
	parameters.addParameter(STR16("OutVuPPML"), nullptr, stepCount, defaultVal, flags, tag);
	tag = kParamOutVuPPMR;
	stepCount = 0;
	defaultVal = 0.0;
	flags = Vst::ParameterInfo::kIsReadOnly;
	parameters.addParameter(STR16("OutVuPPMR"), nullptr, stepCount, defaultVal, flags, tag);
	*/

	
	tag = kParamBypass;
	stepCount = 1;
	defaultVal = 0;
	flags = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass;
	parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);
	


	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (!state) 
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float savedInput = 0.f;
	if (streamer.readFloat(savedInput) == false)
		return kResultFalse;
	setParamNormalized(kParamInput, savedInput);

	float savedEffect = 0.f;
	if (streamer.readFloat(savedEffect) == false)
		return kResultFalse;
	setParamNormalized(kParamEffect, savedEffect);

	float savedCurve = 0.f;
	if (streamer.readFloat(savedCurve) == false)
		return kResultFalse;
	setParamNormalized(kParamCurve, savedCurve);

	int32 savedClip = 0;
	if (streamer.readInt32(savedClip) == false)
		return kResultFalse;
	setParamNormalized(kParamClip, savedClip ? 1 : 0);

	float savedOutput = 0.f;
	if (streamer.readFloat(savedOutput) == false)
		return kResultFalse;
	setParamNormalized(kParamOutput, savedOutput);


	// jump the GainReduction
	// streamer.seek(sizeof(float), kSeekCurrent);

	int32 bypassState = 0;
	if (streamer.readInt32(bypassState) == false)
		return kResultFalse;
	setParamNormalized(kParamBypass, bypassState ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API InflatorPackageController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "InflatorPackageeditor.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API InflatorPackageController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace yg331
