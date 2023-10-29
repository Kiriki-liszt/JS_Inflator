//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once
#include "InflatorPackagecids.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

using namespace Steinberg;

namespace yg331 {

	//------------------------------------------------------------------------
	// SliderParameter Declaration
	// example of custom parameter (overwriting to and fromString)
	//------------------------------------------------------------------------
	class SliderParameter : public Vst::RangeParameter
	{
	public:
		SliderParameter(
			const Vst::TChar* title,
			int32             tag       = -1,
			const Vst::TChar* units     = nullptr,
			Vst::ParamValue   minPlain  = -12,
			Vst::ParamValue   maxPlain  = +12,
			Vst::ParamValue   defaultValuePlain = 0,
			int32             stepCount = 0,
			int32             flags     = Vst::ParameterInfo::kCanAutomate,
			Vst::UnitID       unitID    = Vst::kRootUnitId
		);

		void toString  (Vst::ParamValue normValue, Vst::String128 string)      const SMTG_OVERRIDE;
		Vst::ParamValue toPlain(Vst::ParamValue _valueNormalized) const SMTG_OVERRIDE;
		bool fromString(const Vst::TChar* string,  Vst::ParamValue& normValue) const SMTG_OVERRIDE;
	};


	//------------------------------------------------------------------------
	//  InflatorPackageController
	//------------------------------------------------------------------------
	class InflatorPackageController : public Steinberg::Vst::EditControllerEx1
	{
	public:
		//------------------------------------------------------------------------
		InflatorPackageController() = default;
		~InflatorPackageController() SMTG_OVERRIDE = default;

		// Create function
		static Steinberg::FUnknown* createInstance(void* /*context*/)
		{
			return (Steinberg::Vst::IEditController*)new InflatorPackageController;
		}

		// IPluginBase
		Steinberg::tresult    PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
		Steinberg::tresult    PLUGIN_API terminate() SMTG_OVERRIDE;

		// EditController
		Steinberg::tresult    PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
		Steinberg::tresult    PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult    PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

		Steinberg::tresult    PLUGIN_API setParamNormalized(
			Steinberg::Vst::ParamID     tag,
			Steinberg::Vst::ParamValue  value
		) SMTG_OVERRIDE;
		Steinberg::tresult    PLUGIN_API getParamStringByValue(
			Steinberg::Vst::ParamID     tag,
			Steinberg::Vst::ParamValue  valueNormalized,
			Steinberg::Vst::String128   string
		) SMTG_OVERRIDE;
		Steinberg::tresult    PLUGIN_API getParamValueByString(
			Steinberg::Vst::ParamID     tag,
			Steinberg::Vst::TChar*      string,
			Steinberg::Vst::ParamValue& valueNormalized
		) SMTG_OVERRIDE;

		//---from ComponentBase-----
		tresult receiveText(const char* text) SMTG_OVERRIDE;

		void PLUGIN_API update(Steinberg::FUnknown* changedUnknown, Steinberg::int32 message) SMTG_OVERRIDE;
		void editorAttached(Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;
		void editorRemoved (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;

		//---Interface---------
		DEFINE_INTERFACES
			// Here you can add more supported VST3 interfaces
			// DEF_INTERFACE (Vst::IXXX)
			END_DEFINE_INTERFACES(EditController)
			DELEGATE_REFCOUNT(EditController)

		//------------------------------------------------------------------------
	private:
		typedef std::vector<Steinberg::Vst::EditorView*> EditorVector;
		EditorVector editors;

		ZoomFactorVector zoomFactors;
	};
	
} // namespace yg331
 