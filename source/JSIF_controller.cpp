//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "JSIF_controller.h"
#include "JSIF_cids.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include "pluginterfaces/base/ustring.h"
#include "base/source/fstring.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3groupcontroller.h"
#include "vstgui/vstgui_uidescription.h"
#include "vstgui/uidescription/detail/uiviewcreatorattributes.h"

using namespace Steinberg;

static const std::string kAttrVuOnColor  = "vu-on-color";
static const std::string kAttrVuOffColor = "vu-off-color";

namespace VSTGUI {
class myVuMeterFactory : public ViewCreatorAdapter
{
public:
    //register this class with the view factory
    myVuMeterFactory() { UIViewFactory::registerViewCreator(*this); }

    //return an unique name here
    IdStringPtr getViewName() const SMTG_OVERRIDE { return "My Vu Meter"; }

    //return the name here from where your custom view inherites.
    //	Your view automatically supports the attributes from it.
    IdStringPtr getBaseViewName() const SMTG_OVERRIDE { return UIViewCreator::kCControl; }

    //create your view here.
    //	Note you don't need to apply attributes here as
    //	the apply method will be called with this new view
    CView* create(const UIAttributes& attributes, const IUIDescription* description) const SMTG_OVERRIDE
    {
        return new MyVuMeter(CRect(0,0,100,20), 2);
    }
    bool apply(
        CView* view,
        const UIAttributes& attributes,
        const IUIDescription* description) const SMTG_OVERRIDE
    {
        auto* vuMeter = dynamic_cast<MyVuMeter*> (view);

        if (!vuMeter)
            return false;

        const auto* attr = attributes.getAttributeValue(UIViewCreator::kAttrOrientation);
        if (attr)
            vuMeter->setStyle(*attr == UIViewCreator::strVertical ? MyVuMeter::kVertical : MyVuMeter::kHorizontal);

        CColor color;
        if (UIViewCreator::stringToColor(attributes.getAttributeValue(kAttrVuOnColor), color, description))
            vuMeter->setVuOnColor(color);
        if (UIViewCreator::stringToColor(attributes.getAttributeValue(kAttrVuOffColor), color, description))
            vuMeter->setVuOffColor(color);

        return true;
    }

    bool getAttributeNames(StringList& attributeNames) const SMTG_OVERRIDE
    {
        attributeNames.emplace_back(UIViewCreator::kAttrOrientation);
        attributeNames.emplace_back(kAttrVuOnColor);
        attributeNames.emplace_back(kAttrVuOffColor);
        return true;
    }

    AttrType getAttributeType(const std::string& attributeName) const SMTG_OVERRIDE
    {
        if (attributeName == UIViewCreator::kAttrOrientation)
            return kListType;
        if (attributeName == kAttrVuOnColor)
            return kColorType;
        if (attributeName == kAttrVuOffColor)
            return kColorType;
        return kUnknownType;
    }

    //------------------------------------------------------------------------
    bool getAttributeValue(
        CView* view,
        const string& attributeName,
        string& stringValue,
        const IUIDescription* desc) const SMTG_OVERRIDE
    {
        auto* vuMeter = dynamic_cast<MyVuMeter*> (view);

        if (!vuMeter)
            return false;

        if (attributeName == UIViewCreator::kAttrOrientation)
        {
            if (vuMeter->getStyle() & MyVuMeter::kVertical)
                stringValue = UIViewCreator::strVertical;
            else
                stringValue = UIViewCreator::strHorizontal;
            return true;
        }
        else if (attributeName == kAttrVuOnColor)
        {
            UIViewCreator::colorToString(vuMeter->getVuOnColor(), stringValue, desc);
            return true;
        }
        else if (attributeName == kAttrVuOffColor)
        {
            UIViewCreator::colorToString(vuMeter->getVuOffColor(), stringValue, desc);
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------
    bool getPossibleListValues(
        const string& attributeName,
        ConstStringPtrList& values) const SMTG_OVERRIDE
    {
        if (attributeName == UIViewCreator::kAttrOrientation)
        {
            return UIViewCreator::getStandardAttributeListValues(UIViewCreator::kAttrOrientation, values);
        }
        return false;
    }

};

//create a static instance so that it registers itself with the view factory
myVuMeterFactory __gmyVuMeterFactory;

Steinberg::tresult PLUGIN_API GUIEditor::canResize()
{
    return Steinberg::kResultFalse;
}

} // namespace VSTGUI

namespace yg331 {

//------------------------------------------------------------------------
// VuMeterController
//------------------------------------------------------------------------
template<> void JSIF_Controller::UIVuMeterController::setVuMeterValue(double inL, double inR, double outL, double outR, double GR)
{
    if (vuMeterInL)  { vuMeterInL-> setValueNormalized(inL);  vuMeterInL-> setDirty(true); }
    if (vuMeterInR)  { vuMeterInR-> setValueNormalized(inR);  vuMeterInR-> setDirty(true); }
    if (vuMeterOutL) { vuMeterOutL->setValueNormalized(outL); vuMeterOutL->setDirty(true); }
    if (vuMeterOutR) { vuMeterOutR->setValueNormalized(outR); vuMeterOutR->setDirty(true); }
    if (vuMeterGR)   { vuMeterGR->  setValueNormalized(GR);   vuMeterGR->  setDirty(true); }
}

template<> VSTGUI::CView* JSIF_Controller::UIVuMeterController::verifyView(
                                            VSTGUI::CView* view,
                                            const VSTGUI::UIAttributes&   /*attributes*/,
                                            const VSTGUI::IUIDescription* /*description*/
)
{
    if (VSTGUI::CVuMeter* control = dynamic_cast<VSTGUI::CVuMeter*>(view); control) {
        if (control->getTag() == kInVuPPML)  { vuMeterInL  = control; vuMeterInL-> registerViewListener(this); }
        if (control->getTag() == kInVuPPMR)  { vuMeterInR  = control; vuMeterInR-> registerViewListener(this); }
        if (control->getTag() == kOutVuPPML) { vuMeterOutL = control; vuMeterOutL->registerViewListener(this); }
        if (control->getTag() == kOutVuPPMR) { vuMeterOutR = control; vuMeterOutR->registerViewListener(this); }
        if (control->getTag() == kMeter)     { vuMeterGR   = control; vuMeterGR->  registerViewListener(this); }
    }
    return view;
}

template<> void JSIF_Controller::UIVuMeterController::viewWillDelete(VSTGUI::CView* view)
{
    if (dynamic_cast<VSTGUI::CVuMeter*>(view) == vuMeterInL  && vuMeterInL)  { vuMeterInL-> unregisterViewListener(this); vuMeterInL  = nullptr; }
    if (dynamic_cast<VSTGUI::CVuMeter*>(view) == vuMeterInR  && vuMeterInR)  { vuMeterInR-> unregisterViewListener(this); vuMeterInR  = nullptr; }
    if (dynamic_cast<VSTGUI::CVuMeter*>(view) == vuMeterOutL && vuMeterOutL) { vuMeterOutL->unregisterViewListener(this); vuMeterOutL = nullptr; }
    if (dynamic_cast<VSTGUI::CVuMeter*>(view) == vuMeterOutR && vuMeterOutR) { vuMeterOutR->unregisterViewListener(this); vuMeterOutR = nullptr; }
    if (dynamic_cast<VSTGUI::CVuMeter*>(view) == vuMeterGR   && vuMeterGR)   { vuMeterGR->  unregisterViewListener(this); vuMeterGR   = nullptr; }
}

//------------------------------------------------------------------------
// GainParameter Implementation
//------------------------------------------------------------------------
SliderParameter::SliderParameter(
	const Vst::TChar* title,
	int32             tag,
	const Vst::TChar* units,
	Vst::ParamValue   minPlain,
	Vst::ParamValue   maxPlain,
	Vst::ParamValue   defaultValuePlain,
	int32             stepCount,
	int32             flags,
	Vst::UnitID       unitID
)
{
	UString(info.title, str16BufferSize(Vst::String128)).assign(title);
	if (units)
		UString(info.units, str16BufferSize(Vst::String128)).assign(units);

	setMin(minPlain);
	setMax(maxPlain);

	info.flags     = flags;
	info.id        = tag;
	info.stepCount = stepCount;
	info.defaultNormalizedValue = valueNormalized = toNormalized(defaultValuePlain);
	info.unitId    = Vst::kRootUnitId;
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
// JSIF_Controller Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::initialize(FUnknown* context)
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

    tag          = kParamInput;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = -12.0;
    maxPlain     = 12.0;
    defaultPlain = 0.0;
    stepCount    = 0;
    auto* ParamIn = new Vst::RangeParameter(STR16("Input"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamIn->setPrecision(2);
    parameters.addParameter(ParamIn);

    tag          = kParamOutput;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = -12.0;
    maxPlain     = 0.0;
    defaultPlain = 0.0;
    stepCount    = 0;
    auto* ParamOut = new Vst::RangeParameter(STR16("Output"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamOut->setPrecision(2);
    parameters.addParameter(ParamOut);

    tag          = kParamEffect;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = 0;
    maxPlain     = 100;
    defaultPlain = 0;
    stepCount    = 0;
    auto* ParamEffect = new Vst::RangeParameter(STR16("Effect"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamEffect->setPrecision(2);
    parameters.addParameter(ParamEffect);

    tag          = kParamCurve;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = -50;
    maxPlain     = 50;
    defaultPlain = 0;
    stepCount    = 0;
    auto* ParamCurve = new Vst::RangeParameter(STR16("Curve"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamCurve->setPrecision(2);
    parameters.addParameter(ParamCurve);

    tag          = kParamClip;
    stepCount    = 1;
    defaultVal   = init_Clip ? 1 : 0;
    flags        = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList;
    parameters.addParameter(STR16("Clip"), nullptr, stepCount, defaultVal, flags, tag);

    Vst::StringListParameter* OS = new Vst::StringListParameter(STR("OS"), kParamOS);
    OS->appendString(STR("x1"));
    OS->appendString(STR("x2"));
    OS->appendString(STR("x4"));
    OS->appendString(STR("x8"));
    OS->setNormalized(OS->toNormalized(0));
    parameters.addParameter(OS);

    tag          = kParamSplit;
    stepCount    = 1;
    defaultVal   = init_Split ? 1 : 0;
    flags        = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList;
    parameters.addParameter(STR16("Split"), nullptr, stepCount, defaultVal, flags, tag);

    Vst::StringListParameter* Phase = new Vst::StringListParameter(STR("Phase"), kParamPhase);
    Phase->appendString(STR("Min"));
    Phase->appendString(STR("Max"));
    defaultVal = init_Phase ? 1 : 0;
    Phase->setNormalized(Phase->toNormalized(defaultVal));
    parameters.addParameter(Phase);

    tag          = kParamIn;
    stepCount    = 1;
    defaultVal   = init_In ? 1 : 0;
    flags        = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList;
    parameters.addParameter(STR16("In"), nullptr, stepCount, defaultVal, flags, tag);

    tag          = kParamBypass;
    stepCount    = 1;
    defaultVal   = init_Bypass ? 1 : 0;
    flags        = Vst::ParameterInfo::kIsBypass;
    parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);

	// GUI only parameter
	if (zoomFactors.empty())
	{
		zoomFactors.push_back(ZoomFactor(STR("50%"),  0.50)); // 0/6
		zoomFactors.push_back(ZoomFactor(STR("75%"),  0.75)); // 1/6
		zoomFactors.push_back(ZoomFactor(STR("100%"), 1.00)); // 2/6
		zoomFactors.push_back(ZoomFactor(STR("125%"), 1.25)); // 3/6
		zoomFactors.push_back(ZoomFactor(STR("150%"), 1.50)); // 4/6
		zoomFactors.push_back(ZoomFactor(STR("175%"), 1.75)); // 5/6
		zoomFactors.push_back(ZoomFactor(STR("200%"), 2.00)); // 6/6
	}

	Vst::StringListParameter* zoomParameter = new Vst::StringListParameter(STR("Zoom"), kParamZoom);
	for (ZoomFactorVector::const_iterator it = zoomFactors.begin(), end = zoomFactors.end(); it != end; ++it)
	{
		zoomParameter->appendString(it->title);
	}
	zoomParameter->setNormalized(zoomParameter->toNormalized(0));
	zoomParameter->addDependent(this);
	uiParameters.addParameter(zoomParameter);

	Vst::StringListParameter* GuiSwitch = new Vst::StringListParameter(STR("GUI Switch"), kGuiSwitch);
	GuiSwitch->appendString(STR("Original"));
	GuiSwitch->appendString(STR("Twarch"));
	GuiSwitch->setNormalized(GuiSwitch->toNormalized(0));
	GuiSwitch->addDependent(this);
	uiParameters.addParameter(GuiSwitch);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::terminate()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	getParameterObject(kParamZoom)->removeDependent(this);
	getParameterObject(kGuiSwitch)->removeDependent(this);
	//---do not forget to call parent ------
	return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::setComponentState(IBStream* state)
{
	// loading 'Processor' state into editController

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
	int32           savedIn     = 0;
	int32           savedSplit  = 0;
	Vst::ParamValue savedZoom   = 0.0;
	Vst::ParamValue savedPhase  = 0.0;
	int32           savedBypass = 0;

	if (streamer.readDouble(savedInput)  == false) return kResultFalse;
	if (streamer.readDouble(savedEffect) == false) return kResultFalse;
	if (streamer.readDouble(savedCurve)  == false) return kResultFalse;
	if (streamer.readDouble(savedOutput) == false) return kResultFalse;
	if (streamer.readDouble(savedOS)     == false) return kResultFalse;
	if (streamer.readInt32 (savedClip)   == false) return kResultFalse;
	if (streamer.readInt32 (savedIn)     == false) return kResultFalse;
	if (streamer.readInt32 (savedSplit)  == false) return kResultFalse;
	if (streamer.readDouble(savedZoom)   == false) return kResultFalse;
	if (streamer.readDouble(savedPhase)  == false) return kResultFalse;
	if (streamer.readInt32 (savedBypass) == false) return kResultFalse;

	setParamNormalized(kParamInput,  savedInput);
	setParamNormalized(kParamEffect, savedEffect);
	setParamNormalized(kParamCurve,  savedCurve);
	setParamNormalized(kParamOutput, savedOutput);
	setParamNormalized(kParamOS,     savedOS);
	setParamNormalized(kParamClip,   savedClip   ? 1 : 0);
	setParamNormalized(kParamIn,     savedIn     ? 1 : 0);
	setParamNormalized(kParamSplit,  savedSplit  ? 1 : 0);
	setParamNormalized(kParamZoom,   savedZoom);
	setParamNormalized(kParamPhase,  savedPhase);
	setParamNormalized(kParamBypass, savedBypass ? 1 : 0);

    stateInput  = savedInput;
    stateEffect = savedEffect;
    stateCurve  = savedCurve;
    stateOutput = savedOutput;
    stateOS     = savedOS;
    stateClip   = savedClip;
    stateIn     = savedIn;
    stateSplit  = savedSplit;
    stateZoom   = savedZoom;
    statePhase  = savedPhase;
    stateBypass = savedBypass;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::setState(IBStream* state)
{
	// Loading UI state independently <- reload because of AUv2 overwrites GUI parameters to default AFTER setComponentState. 

	// Moreover, in case of Bitwig, createView is called before setState. 
	// So saved GUI type state is not correctly reproduced. 
	// Also, Editors are added while GUI is opening -> so have to manually save created base view and call exchangeView. 
	
	// Here you get the state of the controller
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    Vst::ParamValue savedInput  = 0.0;
    Vst::ParamValue savedEffect = 0.0;
    Vst::ParamValue savedCurve  = 0.0;
    Vst::ParamValue savedOutput = 0.0;
    Vst::ParamValue savedOS     = 0.0;
    int32           savedClip   = 0;
    int32           savedIn     = 0;
    int32           savedSplit  = 0;
    Vst::ParamValue savedZoom   = 0.0;
    Vst::ParamValue savedPhase  = 0.0;
    Vst::ParamValue savedGUI    = 0.0;
    //int32           savedBypass = 0;
    
    if (streamer.readDouble(savedInput)  == false) savedInput  = 0.5;
    if (streamer.readDouble(savedEffect) == false) savedEffect = 0.0;
    if (streamer.readDouble(savedCurve)  == false) savedCurve  = 0.5;
    if (streamer.readDouble(savedOutput) == false) savedOutput = 1.0;
    if (streamer.readDouble(savedOS)     == false) savedOS     = 0.0;
    if (streamer.readInt32 (savedClip)   == false) savedClip   = 0;
    if (streamer.readInt32 (savedIn)     == false) savedIn     = 1;
    if (streamer.readInt32 (savedSplit)  == false) savedSplit  = 0;
    if (streamer.readDouble(savedZoom)   == false) savedZoom   = 0.0;
    if (streamer.readDouble(savedPhase)  == false) savedPhase  = 0.0;
    if (streamer.readDouble(savedGUI)    == false) savedGUI    = 0.0;
    //if (streamer.readInt32 (savedBypass) == false) return kResultFalse;

    setParamNormalized(kParamInput,  savedInput);
    setParamNormalized(kParamEffect, savedEffect);
    setParamNormalized(kParamCurve,  savedCurve);
    setParamNormalized(kParamOutput, savedOutput);
    setParamNormalized(kParamOS,     savedOS);
    setParamNormalized(kParamClip,   savedClip   ? 1 : 0);
    setParamNormalized(kParamIn,     savedIn     ? 1 : 0);
    setParamNormalized(kParamSplit,  savedSplit  ? 1 : 0);
    setParamNormalized(kParamZoom,   savedZoom);
    setParamNormalized(kParamPhase,  savedPhase);
    setParamNormalized(kGuiSwitch,   savedGUI);
    //setParamNormalized(kParamBypass, savedBypass ? 1 : 0);

    stateInput  = savedInput;
    stateEffect = savedEffect;
    stateCurve  = savedCurve;
    stateOutput = savedOutput;
    stateOS     = savedOS;
    stateClip   = savedClip;
    stateIn     = savedIn;
    stateSplit  = savedSplit;
    stateZoom   = savedZoom;
    statePhase  = savedPhase;
    stateGUI    = savedGUI;
    //stateBypass = savedBypass;

    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::getState(IBStream* state)
{
	// saving UI only state

	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor
    IBStreamer streamer(state, kLittleEndian);
    
    stateInput  = getParamNormalized (kParamInput);
    stateEffect = getParamNormalized (kParamEffect);
    stateCurve  = getParamNormalized (kParamCurve);
    stateOutput = getParamNormalized (kParamOutput);
    stateOS     = getParamNormalized (kParamOS);
    stateClip   = getParamNormalized (kParamClip);
    stateIn     = getParamNormalized (kParamIn);
    stateSplit  = getParamNormalized (kParamSplit);
    stateZoom   = getParamNormalized (kParamZoom);
    statePhase  = getParamNormalized (kParamPhase);
	stateGUI    = getParamNormalized (kGuiSwitch);
    
    streamer.writeDouble(stateInput);
    streamer.writeDouble(stateEffect);
    streamer.writeDouble(stateCurve);
    streamer.writeDouble(stateOutput);
    streamer.writeDouble(stateOS);
    streamer.writeInt32(stateClip ? 1 : 0);
    streamer.writeInt32(stateIn ? 1 : 0);
    streamer.writeInt32(stateSplit ? 1 : 0);
    streamer.writeDouble(stateZoom);
    streamer.writeDouble(statePhase);
    streamer.writeDouble(stateGUI);
    //streamer.writeInt32(stateBypass ? 1 : 0);

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API JSIF_Controller::createView(FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual(name, Vst::ViewType::kEditor))
	{
        VSTGUI::GUIEditor* view;
		// create your editor here and return a IPlugView ptr of it
		if (stateGUI == 0.0) 
			view = new VSTGUI::GUIEditor(this, "Original", "JSIF_editor.uidesc");
		else
			view = new VSTGUI::GUIEditor(this, "Twarch", "JSIF_editor.uidesc");
        view->setGuiState(stateGUI);
        
        std::vector<double> _zoomFactors;
        _zoomFactors.push_back(0.50);
        _zoomFactors.push_back(0.75);
        _zoomFactors.push_back(1.00);
        _zoomFactors.push_back(1.25);
        _zoomFactors.push_back(1.50);
        _zoomFactors.push_back(1.75);
        _zoomFactors.push_back(2.00);
        view->setAllowedZoomFactors(_zoomFactors);
		view->setZoomFactor(0.5);
		view->setIdleRate(1.0/60.0);

		setKnobMode(Steinberg::Vst::KnobModes::kLinearMode);

		return view;
	}
	return nullptr;
}

VSTGUI::IController* JSIF_Controller::createSubController(
	VSTGUI::UTF8StringPtr name,
	const VSTGUI::IUIDescription* description,
	VSTGUI::VST3Editor* editor) 
{
	if (VSTGUI::UTF8StringView(name) == "myVuMeterController")
	{
		auto* controller = new UIVuMeterController(this);
		addUIVuMeterController(controller);
		return controller;
	}
	return nullptr;
};


//------------------------------------------------------------------------
tresult JSIF_Controller::receiveText(const char* text)
{
	// received from Component
	if (text)
	{	
		// Latency report
		if (strcmp("OS\n", text)) {
            getComponentHandler()->restartComponent(Vst::kLatencyChanged);
            //FDebugPrint("[ FDebugPrint ] restartComponent\n");
		}
	}		
	return kResultOk;
}
	
//------------------------------------------------------------------------
void PLUGIN_API JSIF_Controller::update(FUnknown* changedUnknown, int32 message)
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
	if (param->getInfo().id == kGuiSwitch)
	{
        stateGUI = param->getNormalized();
        for (EditorVector::const_iterator it = editors.begin(), end = editors.end(); it != end; ++it)
        {
            /*
            // CPluginView
            tresult PLUGIN_API canResize() SMTG_OVERRIDE
                { return kResultFalse; }
            tresult PLUGIN_API checkSizeConstraint(ViewRect* rect) SMTG_OVERRIDE
                { return kResultFalse; }
            */
            VSTGUI::GUIEditor* editor = dynamic_cast<VSTGUI::GUIEditor*>(*it);
            if (editor) {
                if (editor->getGuiState() != stateGUI) {
                    if      (stateGUI == 0.0) editor->exchangeView("Original");
                    else if (stateGUI == 1.0) editor->exchangeView("Twarch");
                    editor->setGuiState(stateGUI);
                }
            }
        }
	}
}

//------------------------------------------------------------------------
void JSIF_Controller::editorAttached(Steinberg::Vst::EditorView* editor)
{
    VSTGUI::GUIEditor* _editor = dynamic_cast<VSTGUI::GUIEditor*>(editor);
    if (_editor) {
        if (_editor->getGuiState() != stateGUI) {
            if      (stateGUI == 0.0) _editor->exchangeView("Original");
            else if (stateGUI == 1.0) _editor->exchangeView("Twarch");
            _editor->setGuiState(stateGUI);
        }
    }
	editors.push_back(_editor);
}

//------------------------------------------------------------------------
void JSIF_Controller::editorRemoved(Steinberg::Vst::EditorView* editor)
{
	editors.erase(std::find(editors.begin(), editors.end(), editor));
}



//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::setParamNormalized(Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized(tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

//------------------------------------------------------------------------
// DataExchangeController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API JSIF_Controller::notify(Vst::IMessage* message)
{
    if (!message)
        return kInvalidArgument;
    
	if (dataExchange.onMessage(message))
		return kResultTrue;

	return EditControllerEx1::notify(message);
}


//------------------------------------------------------------------------
void PLUGIN_API JSIF_Controller::queueOpened(
	Vst::DataExchangeUserContextID userContextID,
	uint32 blockSize,
	TBool& dispatchOnBackgroundThread)
{
	//FDebugPrint("Data Exchange Queue opened.\n");
}

//------------------------------------------------------------------------
void PLUGIN_API JSIF_Controller::queueClosed(Vst::DataExchangeUserContextID userContextID)
{
	//FDebugPrint("Data Exchange Queue closed.\n");
}

//------------------------------------------------------------------------
void PLUGIN_API JSIF_Controller::onDataExchangeBlocksReceived(
	Vst::DataExchangeUserContextID userContextID,
	uint32 numBlocks,
	Vst::DataExchangeBlock* blocks,
	TBool onBackgroundThread
)
{
	for (auto index = 0u; index < numBlocks; ++index)
	{
		auto dataBlock = toDataBlock(blocks[index]);

		if (!vuMeterControllers.empty()) {
			for (auto iter = vuMeterControllers.begin(); iter != vuMeterControllers.end(); iter++) {
				(*iter)->setVuMeterValue(
					dataBlock->inL, dataBlock->inR,
					dataBlock->outL, dataBlock->outR,
					dataBlock->gR
				);
			}
			
		}
	}
}
	//------------------------------------------------------------------------
} // namespace yg331

/*

 //------------------------------------------------------------------------
 // optionMenuController
 //------------------------------------------------------------------------
 optionMenuController::optionMenuController(Steinberg::Vst::Parameter* parameter, Steinberg::Vst::EditController* editController)
     : parameter(parameter)
     , editController(editController)
 {
     if (parameter)
     {
         //parameter->addRef();
         parameter->addDependent(this);
     }
     vstgui_assert(parameter->getInfo().stepCount > 0);
 }

 //------------------------------------------------------------------------
 optionMenuController::~optionMenuController()
 {
     if (parameter)
     {
         parameter->removeDependent(this);
         //parameter->release();
     }
     for (const auto& c : controls)
         c->forget();
 }

 //------------------------------------------------------------------------
 VSTGUI::CView* optionMenuController::verifyView(VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description)
 {
     auto* control = dynamic_cast<VSTGUI::CControl*>(view);
     if (control)
     {
         addControl(control);
         // control->setListener(this); // <- why?
         if (parameter)
             parameter->changed();
     }
     return view;
 }
 // make sure that our UI only parameters doesn't call the following three EditController methods: beginEdit, endEdit, performEdit
 //------------------------------------------------------------------------
 void optionMenuController::valueChanged(VSTGUI::CControl* pControl)
 {
     Steinberg::Vst::ParamValue normValue = pControl->getValueNormalized();
 //    editController->performEdit(parameter->getInfo().id, normValue);
     parameter->setNormalized(normValue);
 }

 //------------------------------------------------------------------------
 void optionMenuController::controlBeginEdit(VSTGUI::CControl* pControl)
 {
 //    for (const auto& c : controls)
 //        c->setMouseEnabled(c == pControl);
 //    if (parameter)
 //        editController->beginEdit(getParameterID());
 }

 //------------------------------------------------------------------------
 void optionMenuController::controlEndEdit(VSTGUI::CControl* pControl)
 {
 //    if (parameter)
 //        editController->endEdit(getParameterID());
 //    update(parameter, kChanged);
 }

 void optionMenuController::addControl(VSTGUI::CControl* control)
 {
     if (containsControl(control))
         return;
     control->remember();
     controls.push_back(control);
     Steinberg::Vst::ParamValue value = 0.;
     if (parameter)
     {
         value = editController->getParamNormalized(getParameterID());
     }
     else
     {
         if (auto ctrl = controls.front())
             value = ctrl->getValueNormalized();
     }
     auto* display = dynamic_cast<VSTGUI::CParamDisplay*> (control);
     if (display)
         display->setValueToStringFunction([this](float value, char utf8String[256], VSTGUI::CParamDisplay* display) {
         return convertValueToString(value, utf8String);
             });

     if (parameter)
         parameter->deferUpdate();
     else
         updateControlValue(value);
 }

 void optionMenuController::removeControl(VSTGUI::CControl* control)
 {
     for (const auto& c : controls)
     {
         if (c == control)
         {
             controls.remove(control);
             control->forget();
             return;
         }
     }
 }

 bool optionMenuController::containsControl(VSTGUI::CControl* control)
 {
     return std::find(controls.begin(), controls.end(), control) != controls.end();
 }

 Steinberg::Vst::ParamID optionMenuController::getParameterID()
 {
     if (parameter)
         return parameter->getInfo().id;
     VSTGUI::CControl* control = controls.front();
     if (control)
         return static_cast<Steinberg::Vst::ParamID> (control->getTag());
     return 0xFFFFFFFF;
 }

 void PLUGIN_API optionMenuController::update(FUnknown* changedUnknown, Steinberg::int32 message)
 {
     if (message == IDependent::kChanged && parameter)
     {
         updateControlValue(editController->getParamNormalized(getParameterID()));
     }
 }

 bool optionMenuController::convertValueToString(float value, char utf8String[256])
 {
     if (parameter)
     {
         Steinberg::Vst::String128 utf16Str;
         if (parameter && parameter->getInfo().stepCount)
         {
             // convert back to normalized value
             value = (float)editController->plainParamToNormalized(getParameterID(), (Steinberg::Vst::ParamValue)value);
         }
         editController->getParamStringByValue(getParameterID(), value, utf16Str);
         Steinberg::String utf8Str(utf16Str);
         utf8Str.toMultiByte(Steinberg::kCP_Utf8);
         utf8Str.copyTo8(utf8String, 0, 256);
         return true;
     }
     return false;
 }

 void optionMenuController::updateControlValue(Steinberg::Vst::ParamValue value)
 {
     bool mouseEnabled = true;
     Steinberg::Vst::ParamValue defaultValue = 0.5;
     float minValue = 0.f;
     float maxValue = 1.f;

     if (parameter)
     {
         defaultValue = parameter->getInfo().defaultNormalizedValue;

         if (parameter->getInfo().flags & Steinberg::Vst::ParameterInfo::kIsReadOnly)
             mouseEnabled = false;

         value = parameter->toPlain(value);
         defaultValue = parameter->toPlain(defaultValue);
         minValue = (float)parameter->toPlain((Steinberg::Vst::ParamValue)minValue);
         maxValue = (float)parameter->toPlain((Steinberg::Vst::ParamValue)maxValue);
     }

     for (const auto& c : controls)
     {
         c->setMouseEnabled(mouseEnabled);
         if (parameter)
         {
             c->setDefaultValue((float)defaultValue);
             c->setMin(minValue);
             c->setMax(maxValue);
         }

         c->setMin(minValue);
         c->setMax(maxValue);

         auto getParamStringByIndex = [&](int32_t i) {
             Steinberg::Vst::String128 utf16Str;
             editController->getParamStringByValue(
                 getParameterID(),
                 (Steinberg::Vst::ParamValue)i /
                 (Steinberg::Vst::ParamValue)parameter->getInfo().stepCount,
                 utf16Str);
             Steinberg::String utf8Str(utf16Str);
             utf8Str.toMultiByte(Steinberg::kCP_Utf8);
             return utf8Str;
             };

         // COptionMenu ONLY
         auto optMenu = dynamic_cast<VSTGUI::COptionMenu*> (c);
         optMenu->removeAllEntry();
         for (Steinberg::int32 i = 0; i <= parameter->getInfo().stepCount; i++)
             optMenu->addEntry(getParamStringByIndex(i).text8());
         c->setValue((float)value - minValue);

         c->valueChanged();
         c->invalid();
     }
 }

 
*/
