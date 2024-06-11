//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once
#include "InflatorPackagecids.h"
#include "InflatorPackageDataexchange.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"

using namespace Steinberg;

namespace VSTGUI {
	//------------------------------------------------------------------------
	//  VU meter view
	//------------------------------------------------------------------------
	class MyVuMeter : public CControl {
	private:
		enum StyleEnum
		{
			StyleHorizontal = 0,
			StyleVertical,
		};
	public:
		enum Style
		{
			kHorizontal = 1 << StyleHorizontal,
			kVertical = 1 << StyleVertical,
		};

		MyVuMeter(const CRect& size, int32_t style = kVertical)
			: CControl(size, nullptr, 0)
			, style(style)
		{
			vuOnColor = kWhiteCColor;
			vuOffColor = kBlackCColor;

			rectOn(size.left, size.top, size.right, size.bottom);
			rectOff(size.left, size.top, size.right, size.bottom);

			setWantsIdle(true);
		}
		MyVuMeter(const MyVuMeter& vuMeter)
			: CControl(vuMeter)
			, style(vuMeter.style)
			, vuOnColor(vuMeter.vuOnColor)
			, vuOffColor(vuMeter.vuOffColor)
			, rectOn(vuMeter.rectOn)
			, rectOff(vuMeter.rectOff)
		{
			setWantsIdle(true);
		}

		void setStyle(int32_t newStyle) { style = newStyle; invalid(); }
		int32_t getStyle() const { return style; }

		virtual void setVuOnColor(CColor color) {
			if (vuOnColor != color) { vuOnColor = color; setDirty(true); }
		}
		CColor getVuOnColor() const { return vuOnColor; }

		virtual void setVuOffColor(CColor color) {
			if (vuOffColor != color) { vuOffColor = color; setDirty(true); }
		}
		CColor getVuOffColor() const { return vuOffColor; }

		// overrides
		void setDirty(bool state) override
		{
			CView::setDirty(state);
		};
		void draw(CDrawContext* _pContext) override {
			CRect _rectOn(rectOn);
			CRect _rectOff(rectOff);
			CPoint pointOn;
			CPoint pointOff;
			CDrawContext* pContext = _pContext;

			bounceValue();

			float newValue = getValueNormalized(); // normalize

			if (style & kHorizontal)
			{
				auto tmp = (CCoord)((int32_t)(newValue)*getViewSize().getWidth());
				pointOff(tmp, 0);

				_rectOff.left += tmp;
				_rectOn.right = tmp + rectOn.left;
			}
			else
			{
				auto tmp = (CCoord)((int32_t)(newValue * getViewSize().getHeight()));
				pointOn(0, tmp);

				//_rectOff.bottom = tmp + rectOff.top;
				//_rectOn.top += tmp;
				_rectOn.top = _rectOff.bottom - tmp;
			}

			pContext->setFillColor(vuOffColor);
			pContext->drawRect(rectOff, kDrawFilled);

			pContext->setFillColor(vuOnColor);
			pContext->drawRect(_rectOn, kDrawFilled);

			setDirty(false);
		};
		void setViewSize(const CRect& newSize, bool invalid = true) override
		{
			CControl::setViewSize(newSize, invalid);
			rectOn = getViewSize();
			rectOff = getViewSize();
		};
		bool sizeToFit() override {
			if (getDrawBackground())
			{
				CRect vs(getViewSize());
				vs.setWidth(getDrawBackground()->getWidth());
				vs.setHeight(getDrawBackground()->getHeight());
				setViewSize(vs);
				setMouseableArea(vs);
				return true;
			}
			return false;
		};
		
		void onIdle() override {
			invalid();
		};
		

		CLASS_METHODS(MyVuMeter, CControl)

	protected:
		~MyVuMeter() noexcept override
		{
			//setOnBitmap(nullptr);
			//setOffBitmap(nullptr);
		};

		int32_t     style;

		CColor		vuOnColor;
		CColor		vuOffColor;

		CRect    rectOn;
		CRect    rectOff;
	};
}

namespace yg331 {

//------------------------------------------------------------------------
// VuMeterController
//------------------------------------------------------------------------
template <typename ControllerType>
class VuMeterController 
	: public VSTGUI::IController
	, public VSTGUI::ViewListenerAdapter
{
public:
	VuMeterController(ControllerType* _mainController) :
		mainController(_mainController),
		vuMeterInL(nullptr),
		vuMeterInR(nullptr),
		vuMeterOutL(nullptr),
		vuMeterOutR(nullptr),
		vuMeterGR(nullptr)
	{
	}
	~VuMeterController() override
	{
		if (vuMeterInL)  viewWillDelete(vuMeterInL);
		if (vuMeterInR)  viewWillDelete(vuMeterInR);
		if (vuMeterOutL) viewWillDelete(vuMeterOutL);
		if (vuMeterOutR) viewWillDelete(vuMeterOutR);
		if (vuMeterGR)   viewWillDelete(vuMeterGR);

		mainController->removeUIVuMeterController(this);
	}

	void setMeterValue(double val, int32_t tag)
	{
		if (tag == kIn) {
			if (!inMeter) return;
			inMeter->setValue(val);
			inMeter->setDirty(true);
		}
		if (tag == kOut) {
			if (!outMeter) return;
			outMeter->setValue(val);
			outMeter->setDirty(true);
		}
		if (tag == kGR) {
			if (!grMeter) return;
			grMeter->setValue(val);
			grMeter->setDirty(true);
		}
	}
	void setVuMeterValue(double inL, double inR, double outL, double outR, double GR)
	{
		if (vuMeterInL) {
			vuMeterInL->setValueNormalized(inL);
			vuMeterInL->setDirty(true);
		}
		if (vuMeterInR) {
			vuMeterInR->setValueNormalized(inR);
			vuMeterInR->setDirty(true);
		}
		else if (vuMeterOutL) {
			vuMeterOutL->setValueNormalized(outL);
			vuMeterOutL->setDirty(true);
		}
		else if (vuMeterOutR) {
			vuMeterOutR->setValueNormalized(outR);
			vuMeterOutR->setDirty(true);
		}
		else if (vuMeterGR) {
			vuMeterGR->setValueNormalized(GR);
			vuMeterGR->setDirty(true);
		}
	}

private:
	using CControl = VSTGUI::CControl;
	using CView = VSTGUI::CView;
	using CParamDisplay = VSTGUI::CParamDisplay;
	using MyVuMeter = VSTGUI::CVuMeter;
	using UTF8String = VSTGUI::UTF8String;
	using UIAttributes = VSTGUI::UIAttributes;
	using IUIDescription = VSTGUI::IUIDescription;

	//--- from IControlListener ----------------------
	void valueChanged(CControl* /*pControl*/) override {}
	void controlBeginEdit(CControl* /*pControl*/) override {}
	void controlEndEdit(CControl* pControl) override {}
	//--- is called when a view is created -----
	CView* verifyView(
		CView* view,
		const UIAttributes& /*attributes*/,
		const IUIDescription* /*description*/) override
	{
		if (MyVuMeter* control = dynamic_cast<MyVuMeter*>(view); control) {
			if (control->getTag() == kInVuPPML) {
				vuMeterInL = control;
				vuMeterInL->registerViewListener(this);
				///vuMeterInL->setValue(0.0);
			}
			if (control->getTag() == kInVuPPMR) {
				vuMeterInR = control;
				vuMeterInR->registerViewListener(this);
				//vuMeterInR->setValue(0.0);
			}
			if (control->getTag() == kOutVuPPML) {
				vuMeterOutL = control;
				vuMeterOutL->registerViewListener(this);
				//vuMeterOutL->setValue(0.0);
			}
			if (control->getTag() == kOutVuPPMR) {
				vuMeterOutR = control;
				vuMeterOutR->registerViewListener(this);
				//vuMeterOutR->setValue(0.0);
			}
			if (control->getTag() == kMeter) {
				vuMeterGR = control;
				vuMeterGR->registerViewListener(this);
				//vuMeterGR->setValue(0.0);
			}
		}

		return view;
	}
	//--- from IViewListenerAdapter ----------------------
	//--- is called when a view will be deleted: the editor is closed -----
	void viewWillDelete(CView* view) override
	{
		if (dynamic_cast<MyVuMeter*>(view) == vuMeterInL && vuMeterInL) {
			vuMeterInL->unregisterViewListener(this);
			vuMeterInL = nullptr;
		}
		if (dynamic_cast<MyVuMeter*>(view) == vuMeterInR && vuMeterInR) {
			vuMeterInR->unregisterViewListener(this);
			vuMeterInR = nullptr;
		}
		if (dynamic_cast<MyVuMeter*>(view) == vuMeterOutL && vuMeterOutL) {
			vuMeterOutL->unregisterViewListener(this);
			vuMeterOutL = nullptr;
		}
		if (dynamic_cast<MyVuMeter*>(view) == vuMeterOutR && vuMeterOutR) {
			vuMeterOutR->unregisterViewListener(this);
			vuMeterOutR = nullptr;
		}
		if (dynamic_cast<MyVuMeter*>(view) == vuMeterGR && vuMeterGR) {
			vuMeterGR->unregisterViewListener(this);
			vuMeterGR = nullptr;
		}
	}

	ControllerType* mainController;
	MyVuMeter* vuMeterInL;
	MyVuMeter* vuMeterInR;
	MyVuMeter* vuMeterOutL;
	MyVuMeter* vuMeterOutR;
	MyVuMeter* vuMeterGR;
};

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
class InflatorPackageController 
	: public Steinberg::Vst::EditControllerEx1
	, public Steinberg::Vst::IDataExchangeReceiver
	, public VSTGUI::VST3EditorDelegate
{
public:
	//------------------------------------------------------------------------
	InflatorPackageController() = default;
	~InflatorPackageController() SMTG_OVERRIDE = default;

	using UIVuMeterController = VuMeterController<InflatorPackageController>;

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
	Steinberg::tresult    PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
		                                                    Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult    PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag,
		                                                    Steinberg::Vst::ParamValue valueNormalized,
		                                                    Steinberg::Vst::String128 string) SMTG_OVERRIDE;
	Steinberg::tresult    PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag,
		                                                    Steinberg::Vst::TChar* string,
		                                                    Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

	//---from ComponentBase-----
	// EditController
	Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API receiveText(const char* text) SMTG_OVERRIDE;
	void PLUGIN_API update(Steinberg::FUnknown* changedUnknown, Steinberg::int32 message) SMTG_OVERRIDE;
	void editorAttached(Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;
	void editorRemoved (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;


	//---from VST3EditorDelegate-----------
	/** verify a view after it was created */
	/*
	VSTGUI::CView* PLUGIN_API verifyView(VSTGUI::CView* view, 
											const VSTGUI::UIAttributes& attributes,
											const VSTGUI::IUIDescription* description, 
											VSTGUI::VST3Editor* editor) SMTG_OVERRIDE;
	*/
	
	/** called when a sub controller should be created.
		The controller is now owned by the editor, which will call forget() if it is a CBaseObject,
		release() if it is a Steinberg::FObject or it will be simply deleted if the frame gets
		closed. */
	VSTGUI::IController* createSubController (VSTGUI::UTF8StringPtr name, 
	                                          const VSTGUI::IUIDescription* description,
	                                          VSTGUI::VST3Editor* editor) SMTG_OVERRIDE
	{
		if (VSTGUI::UTF8StringView(name) == "myVuMeterController")
		{
			auto* controller = new UIVuMeterController(this);
			addUIVuMeterController(controller);
			return controller;
		}
		return nullptr;
	};

	// IDataExchangeReceiver
	void PLUGIN_API queueOpened (Steinberg::Vst::DataExchangeUserContextID userContextID,
		                         Steinberg::uint32 blockSize,
		                         Steinberg::TBool& dispatchOnBackgroundThread) SMTG_OVERRIDE;
	void PLUGIN_API queueClosed (Steinberg::Vst::DataExchangeUserContextID userContextID) SMTG_OVERRIDE;
	void PLUGIN_API onDataExchangeBlocksReceived (Steinberg::Vst::DataExchangeUserContextID userContextID,
		                                          Steinberg::uint32 numBlocks, 
		                                          Steinberg::Vst::DataExchangeBlock* blocks,
		                                          Steinberg::TBool onBackgroundThread) SMTG_OVERRIDE;

	

	//---Internal functions-------
	void addUIVuMeterController(UIVuMeterController* controller)
	{
		vuMeterControllers.push_back(controller);
	};
	void removeUIVuMeterController(UIVuMeterController* controller)
	{
		auto it = std::find(vuMeterControllers.begin(), vuMeterControllers.end(), controller);
		if (it != vuMeterControllers.end())
			vuMeterControllers.erase(it);
	};

	//---Interface---------
	DEFINE_INTERFACES
		// Here you can add more supported VST3 interfaces
		DEF_INTERFACE(IDataExchangeReceiver)
	END_DEFINE_INTERFACES(EditController)
	DELEGATE_REFCOUNT(EditController)

	//------------------------------------------------------------------------
private:
	// editor list
	typedef std::vector<Steinberg::Vst::EditorView*> EditorVector;
	EditorVector editors;

	// zoom title-value struct
	struct ZoomFactor {
		const Steinberg::tchar* title;
		double factor;

		ZoomFactor(const Steinberg::tchar* title, double factor) : title(title), factor(factor) {}
	};
	typedef std::vector<ZoomFactor> ZoomFactorVector;
	ZoomFactorVector zoomFactors;

	// sub-controller list
	using UIVuMeterControllerList = std::vector<UIVuMeterController*>;
	UIVuMeterControllerList vuMeterControllers;

	// data exchange
	Steinberg::Vst::DataExchangeReceiverHandler dataExchange{ this };
};
	
} // namespace yg331
 