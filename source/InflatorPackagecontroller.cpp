//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "InflatorPackagecids.h"
#include "InflatorPackagecontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3groupcontroller.h"

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
	Vst::ParamValue SliderParameter::toPlain(Vst::ParamValue _valueNormalized) const {

		if (_valueNormalized == 1.00) return 6.0;
		if (_valueNormalized >= 0.96) return 6.0;
		if (_valueNormalized >= 0.92) return 5.0;
		if (_valueNormalized >= 0.88) return 4.0;
		if (_valueNormalized >= 0.84) return 3.0;
		if (_valueNormalized >= 0.80) return 2.0;
		if (_valueNormalized >= 0.76) return 1.0;
		if (_valueNormalized >= 0.72) return 0.0;
		if (_valueNormalized >= 0.68) return -1.0;
		if (_valueNormalized >= 0.64) return -2.0;
		if (_valueNormalized >= 0.60) return -3.0;
		if (_valueNormalized >= 0.56) return -4.0;
		if (_valueNormalized >= 0.52) return -5.0;
		if (_valueNormalized >= 0.48) return -6.0;
		if (_valueNormalized >= 0.44) return -8.0;
		if (_valueNormalized >= 0.40) return -10.0;
		if (_valueNormalized >= 0.36) return -12.0;
		if (_valueNormalized >= 0.32) return -14.0;
		if (_valueNormalized >= 0.28) return -16.0;
		if (_valueNormalized >= 0.24) return -18.0;
		if (_valueNormalized >= 0.20) return -22.0;
		if (_valueNormalized >= 0.16) return -26.0;
		if (_valueNormalized >= 0.12) return -30.0;
		if (_valueNormalized >= 0.08) return -34.0;
		if (_valueNormalized >= 0.04) return -38.0;
		return -42.0;

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

		tag = kParamInput;
		flags = Vst::ParameterInfo::kCanAutomate;
		minPlain = -12.0;
		maxPlain = 12.0;
		defaultPlain = 0.0;
		stepCount = 0;
		auto* ParamIn = new Vst::RangeParameter(STR16("Input"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
		ParamIn->setPrecision(2);
		parameters.addParameter(ParamIn);

		tag = kParamOutput;
		flags = Vst::ParameterInfo::kCanAutomate;
		minPlain = -12.0;
		maxPlain = 0.0;
		defaultPlain = 0.0;
		stepCount = 0;
		auto* ParamOut = new Vst::RangeParameter(STR16("Output"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
		ParamOut->setPrecision(2);
		parameters.addParameter(ParamOut);




		tag = kParamInVuPPML;
		flags = Vst::ParameterInfo::kIsReadOnly;
		minPlain = -44.0;
		maxPlain = 6.0;
		defaultPlain = -44.0;
		auto* InVuPPML = new SliderParameter(USTRING("InVuPPML"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(InVuPPML);
		tag = kParamInVuPPMR;
		auto* InVuPPMR = new SliderParameter(USTRING("InVuPPMR"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(InVuPPMR);
		tag = kParamOutVuPPML;
		auto* OutVuPPML = new SliderParameter(USTRING("OutVuPPML"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(OutVuPPML);
		tag = kParamOutVuPPMR;
		auto* OutVuPPMR = new SliderParameter(USTRING("OutVuPPMR"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, 0, flags);
		parameters.addParameter(OutVuPPMR);


		tag = kParamEffect;
		flags = Vst::ParameterInfo::kCanAutomate;
		minPlain = 0;
		maxPlain = 100;
		defaultPlain = 100;
		stepCount = 0;
		auto* ParamEffect = new Vst::RangeParameter(STR16("Effect"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
		ParamEffect->setPrecision(2);
		parameters.addParameter(ParamEffect);

		tag = kParamCurve;
		flags = Vst::ParameterInfo::kCanAutomate;
		minPlain = -50;
		maxPlain = 50;
		defaultPlain = 0;
		stepCount = 0;
		auto* ParamCurve = new Vst::RangeParameter(STR16("Curve"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
		ParamCurve->setPrecision(2);
		parameters.addParameter(ParamCurve);

		/*
		tag = kParamCurve;
		stepCount = 0;
		defaultVal = init_Curve;
		flags = Vst::ParameterInfo::kCanAutomate; 
		parameters.addParameter(STR16("Curve"), nullptr, stepCount, defaultVal, flags, tag);
		*/

		tag = kParamClip;
		stepCount = 1;
		defaultVal = init_Clip ? 1 : 0;
		flags = Vst::ParameterInfo::kCanAutomate;
		parameters.addParameter(STR16("Clip"), nullptr, stepCount, defaultVal, flags, tag);

		tag = kParamBypass;
		stepCount = 1;
		defaultVal = init_Bypass ? 1 : 0;
		flags = Vst::ParameterInfo::kCanAutomate;
		parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);

		Vst::StringListParameter* OS = new Vst::StringListParameter(STR("OS"), kParamOS);
		OS->appendString(STR("x1"));
		OS->appendString(STR("x2"));
		OS->appendString(STR("x4"));
		OS->appendString(STR("x8")); // stepCount is automzed!
		OS->setNormalized(OS->toNormalized(0));
		OS->addDependent(this);
		parameters.addParameter(OS);


		tag = kParamSplit;
		stepCount = 1;
		defaultVal = init_Split ? 1 : 0;
		flags = Vst::ParameterInfo::kCanAutomate;
		parameters.addParameter(STR16("Split"), nullptr, stepCount, defaultVal, flags, tag);

		if (zoomFactors.empty())
		{
			zoomFactors.push_back(ZoomFactor(STR("50%"),  0.5 * 0.5));  // 0/6
			zoomFactors.push_back(ZoomFactor(STR("75%"),  0.5 * 0.75)); // 1/6
			zoomFactors.push_back(ZoomFactor(STR("100%"), 0.5 * 1.0));  // 2/6
			zoomFactors.push_back(ZoomFactor(STR("125%"), 0.5 * 1.25)); // 3/6
			zoomFactors.push_back(ZoomFactor(STR("150%"), 0.5 * 1.5));  // 4/6
			zoomFactors.push_back(ZoomFactor(STR("175%"), 0.5 * 1.75)); // 5/6
			zoomFactors.push_back(ZoomFactor(STR("200%"), 0.5 * 2.0));  // 6/6
		}

		Vst::StringListParameter* zoomParameter = new Vst::StringListParameter(STR("Zoom"), kParamZoom);
		for (ZoomFactorVector::const_iterator it = zoomFactors.begin(), end = zoomFactors.end(); it != end; ++it)
		{
			zoomParameter->appendString(it->title);
		}
		zoomParameter->setNormalized(zoomParameter->toNormalized(2)); // toNorm(2) == 100%
		zoomParameter->addDependent(this);
		parameters.addParameter(zoomParameter);

		tag = kParamMeter;
		stepCount = 0;
		defaultVal = 0;
		flags = Vst::ParameterInfo::kCanAutomate;
		parameters.addParameter(STR16("Meter"), nullptr, stepCount, defaultVal, flags, tag);

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
		if (!state)
			return kResultFalse;

		IBStreamer streamer(state, kLittleEndian);

		Vst::ParamValue savedInput  = 0.0;
		Vst::ParamValue savedEffect = 0.0;
		Vst::ParamValue savedCurve  = 0.0;
		Vst::ParamValue savedOutput = 0.0;
		Vst::ParamValue savedOS     = 0.0;
		int32           savedClip   = 0;
		int32           savedBypass = 0;
		int32           savedSplit  = 0;
		Vst::ParamValue savedZoom = 0.0;

		if (streamer.readDouble(savedInput)  == false) return kResultFalse;
		if (streamer.readDouble(savedEffect) == false) return kResultFalse;
		if (streamer.readDouble(savedCurve)  == false) return kResultFalse;
		if (streamer.readDouble(savedOutput) == false) return kResultFalse;
		if (streamer.readDouble(savedOS)     == false) return kResultFalse;
		if (streamer.readInt32 (savedClip)   == false) return kResultFalse;
		if (streamer.readInt32 (savedBypass) == false) return kResultFalse;
		if (streamer.readInt32 (savedSplit)  == false) return kResultFalse;
		if (streamer.readDouble(savedZoom)   == false) return kResultFalse;

		setParamNormalized(kParamInput,  savedInput);
		setParamNormalized(kParamEffect, savedEffect);
		setParamNormalized(kParamCurve,  savedCurve);
		setParamNormalized(kParamOutput, savedOutput);
		setParamNormalized(kParamOS,     savedOS);
		setParamNormalized(kParamClip,   savedClip   ? 1 : 0);
		setParamNormalized(kParamBypass, savedBypass ? 1 : 0);
		setParamNormalized(kParamSplit,  savedSplit  ? 1 : 0);
		setParamNormalized(kParamZoom,   savedZoom);

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
	tresult InflatorPackageController::receiveText(const char* text)
	{
		// received from Component
		if (text)
		{	
			// Latency report
			if (strcmp("OS\n", text)) {
			 	FUnknownPtr<Vst::IComponentHandler>handler(componentHandler);
			 	handler->restartComponent(Vst::kLatencyChanged);
			}
		}		
		return kResultOk;
	}
	
	//------------------------------------------------------------------------
	void PLUGIN_API InflatorPackageController::update(FUnknown* changedUnknown, int32 message)
	{
		EditControllerEx1::update(changedUnknown, message);

		// GUI Resizing
		// check 'zoomtest' code at
		// https://github.com/steinbergmedia/vstgui/tree/vstgui4_10/vstgui/tests/uidescription%20vst3/source
		
		Vst::Parameter* param = FCast<Vst::Parameter>(changedUnknown);
		if (!param)
			return;

		if (param->getInfo().id == kParamZoom)
		{
			size_t index = static_cast<size_t> (param->toPlain(param->getNormalized()));

			if (index >= zoomFactors.size())
				return;

			for (EditorVector::const_iterator it = editors.begin(), end = editors.end(); it != end; ++it)
			{
				VSTGUI::VST3Editor* editor = dynamic_cast<VSTGUI::VST3Editor*>(*it);
				if (editor)
					editor->setZoomFactor(zoomFactors[index].factor);
			}
		}
	}
	//------------------------------------------------------------------------
	void InflatorPackageController::editorAttached(Steinberg::Vst::EditorView* editor)
	{
		editors.push_back(editor);
	}

	//------------------------------------------------------------------------
	void InflatorPackageController::editorRemoved(Steinberg::Vst::EditorView* editor)
	{
		editors.erase(std::find(editors.begin(), editors.end(), editor));
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
