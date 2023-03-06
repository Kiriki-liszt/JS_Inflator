//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "InflatorPackagecontroller.h"
#include "InflatorPackagecids.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace yg331 {

	//------------------------------------------------------------------------
	// GainParameter Implementation
	//------------------------------------------------------------------------
	SliderParameter::SliderParameter(
		const Vst::TChar* title,
		int32 tag,
		const Vst::TChar* units,
		Vst::ParamValue minPlain,
		Vst::ParamValue maxPlain,
		Vst::ParamValue defaultValuePlain,
		int32 stepCount,
		int32 flags,
		Vst::UnitID unitID
	)
	{
		UString(info.title, str16BufferSize(Vst::String128)).assign(title);
		if (units)
			UString(info.units, str16BufferSize(Vst::String128)).assign(units);

		setMin(minPlain);
		setMax(maxPlain);

		info.flags = flags;
		info.id = tag;
		info.stepCount = stepCount;
		info.defaultNormalizedValue = valueNormalized = toNormalized(defaultValuePlain);
		info.unitId = Vst::kRootUnitId;
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

	//------------------------------------------------------------------------
	// InflatorPackageController Implementation
	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::initialize(FUnknown* context)
	{
		// Here the Plug-in will be instantiated

		//---do not forget to call parent ------
		tresult result = EditControllerEx1::initialize(context);
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

		flags = Vst::ParameterInfo::kCanAutomate;
		minPlain = -12.0;
		maxPlain = 12.0;
		defaultPlain = 0.0;

		tag = kParamInput;
		auto* gainParamIn = new SliderParameter(USTRING("Input"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(gainParamIn);
		tag = kParamOutput;
		auto* gainParamOut = new SliderParameter(USTRING("Output"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(gainParamOut);

		tag = kParamInVuPPML;
		flags = Vst::ParameterInfo::kIsReadOnly;
		minPlain = -44.0;
		maxPlain = 6.0;
		defaultPlain = -44.0;
		auto* InVuPPML = new SliderParameter(USTRING("InVuPPML"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(InVuPPML);
		tag = kParamInVuPPMR;
		flags = Vst::ParameterInfo::kIsReadOnly;
		minPlain = -44.0;
		maxPlain = 6.0;
		defaultPlain = -44.0;
		auto* InVuPPMR = new SliderParameter(USTRING("InVuPPMR"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(InVuPPMR);
		tag = kParamOutVuPPML;
		flags = Vst::ParameterInfo::kIsReadOnly;
		minPlain = -44.0;
		maxPlain = 6.0;
		defaultPlain = -44.0;
		auto* OutVuPPML = new SliderParameter(USTRING("OutVuPPML"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(OutVuPPML);
		tag = kParamOutVuPPMR;
		flags = Vst::ParameterInfo::kIsReadOnly;
		minPlain = -44.0;
		maxPlain = 6.0;
		defaultPlain = -44.0;
		auto* OutVuPPMR = new SliderParameter(USTRING("OutVuPPMR"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(OutVuPPMR);


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

		tag = kParamBypass;
		stepCount = 1;
		defaultVal = 0;
		flags = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass;
		parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);



		return result;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

		//---do not forget to call parent ------
		return EditControllerEx1::terminate();
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::setComponentState(IBStream* state)
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
	tresult PLUGIN_API InflatorPackageController::setState(IBStream* state)
	{
		// Here you get the state of the controller

		return kResultTrue;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::getState(IBStream* state)
	{
		// Here you are asked to deliver the state of the controller (if needed)
		// Note: the real state of your plug-in is saved in the processor

		return kResultTrue;
	}

	//------------------------------------------------------------------------
	IPlugView* PLUGIN_API InflatorPackageController::createView(FIDString name)
	{
		// Here the Host wants to open your editor (if you have one)
		if (FIDStringsEqual(name, Vst::ViewType::kEditor))
		{
			// create your editor here and return a IPlugView ptr of it
			auto* view = new VSTGUI::VST3Editor(this, "view", "InflatorPackageeditor.uidesc");
			return view;
		}
		return nullptr;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue value)
	{
		// called by host to update your parameters
		tresult result = EditControllerEx1::setParamNormalized(tag, value);
		return result;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
	{
		// called by host to get a string for given normalized value of a specific parameter
		// (without having to set the value!)
		return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API InflatorPackageController::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
	{
		// called by host to get a normalized value from a string representation of a specific parameter
		// (without having to set the value!)
		return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
	}

	//------------------------------------------------------------------------



	


} // namespace yg331
