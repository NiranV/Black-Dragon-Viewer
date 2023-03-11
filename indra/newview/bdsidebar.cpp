/**
*
* Copyright (C) 2018, NiranV Dean
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
*/

#include "llviewerprecompiledheaders.h"

#include "bdsidebar.h"

// viewer includes
#include "llfloaterpreference.h"
#include "llviewercamera.h"
#include "pipeline.h"
//#include "llsdserialize.h"

#include "llsliderctrl.h"
//#include "llradiogroup.h"
//#include "llcombobox.h"
//#include "llcheckboxctrl.h"
//#include "lllayoutstack.h"
//#include "lluictrl.h"
//#include "lluictrlfactory.h"

// system includes
//#include <iomanip>

LLSideBar *gSideBar = NULL;

LLSideBar::LLSideBar(const LLRect& rect)
:	LLPanel()
{
	//BD
	/*mCommitCallbackRegistrar.add("Sidebar.ToggleEdit", boost::bind(&LLSideBar::toggleEditMode, this));
	mCommitCallbackRegistrar.add("Sidebar.Load", boost::bind(&LLSideBar::loadWidgetList, this));

	mCommitCallbackRegistrar.add("Sidebar.Create", boost::bind(&LLSideBar::createWidget, this));

	mCommitCallbackRegistrar.add("Sidebar.Type", boost::bind(&LLSideBar::onTypeSelection, this));

	mCommitCallbackRegistrar.add("Sidebar.Add", boost::bind(&LLSideBar::onAddTest, this));
	mCommitCallbackRegistrar.add("Sidebar.Delete", boost::bind(&LLSideBar::onDeleteTest, this));*/

	buildFromFile("panel_machinima.xml");
}

LLSideBar::~LLSideBar()
{
}

// virtual
void LLSideBar::draw()
{
	if (gSideBar->getParentByType<LLLayoutPanel>()->getVisibleAmount() > 0.01f)
	{
		if (mUpdateTimer.getElapsedTimeF32() > 3.f)
		{
			mShadowDistY->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
			mShadowDistZ->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
			mShadowDistW->setEnabled(!gPipeline.RenderShadowAutomaticDistance);

			//refreshCreationControls();
			mUpdateTimer.reset();
		}

		LLPanel::draw();
	}
}

BOOL LLSideBar::postBuild()
{
	//BD - Legacy stuff.
	mShadowResX = getChild<LLUICtrl>("RenderShadowResolution_X");
	mShadowResY = getChild<LLUICtrl>("RenderShadowResolution_Y");
	mShadowResZ = getChild<LLUICtrl>("RenderShadowResolution_Z");
	mShadowResW = getChild<LLUICtrl>("RenderShadowResolution_W");

	mShadowDistX = getChild<LLUICtrl>("RenderShadowDistance_X");
	mShadowDistY = getChild<LLUICtrl>("RenderShadowDistance_Y");
	mShadowDistZ = getChild<LLUICtrl>("RenderShadowDistance_Z");
	mShadowDistW = getChild<LLUICtrl>("RenderShadowDistance_W");

	mProjectorResX = getChild<LLUICtrl>("RenderProjectorShadowResolution_X");
	mProjectorResY = getChild<LLUICtrl>("RenderProjectorShadowResolution_Y");

	mVignetteX = getChild<LLUICtrl>("ExodusRenderVignette_X");
	mVignetteY = getChild<LLUICtrl>("ExodusRenderVignette_Y");
	mVignetteZ = getChild<LLUICtrl>("ExodusRenderVignette_Z");

	mCameraAngle = getChild<LLSliderCtrl>("CameraAngle");

	/*mTestStack = getChild<LLLayoutStack>("test_stack");

	//BD - General stuff.
	mIsCheckbox = getChild<LLButton>("is_checkbox");
	mIsSlider = getChild<LLButton>("is_slider");
	mIsRadio = getChild<LLButton>("is_radio");
	mIsTitle = getChild<LLButton>("is_title");
	mIsTab = getChild<LLButton>("is_tab");

	mDebugCombo = getChild<LLComboBox>("debug_setting");
	mLabelEditor = getChild<LLLineEditor>("label");
	mNextBtn = getChild<LLButton>("next");

	mScrollPanel = getChild<LLPanel>("scroll_panel");
	mWidgetsStack = getChild<LLLayoutStack>("widgets_stack");
	mMainLayout = getChild<LLLayoutPanel>("create_layout");

	//BD - Slider stuff.
	mDecimalsEditor = getChild<LLLineEditor>("decimals");
	mIncrementEditor = getChild<LLLineEditor>("increment");
	mMinEditor = getChild<LLLineEditor>("min_value");
	mMaxEditor = getChild<LLLineEditor>("max_value");
	mIsX = getChild<LLButton>("is_x");
	mIsY = getChild<LLButton>("is_y");
	mIsZ = getChild<LLButton>("is_z");
	mIsW = getChild<LLButton>("is_w");

	//BD - Radio stuff.
	//mRadio2 = getChild<LLButton>("2_radio");
	//mRadio3 = getChild<LLButton>("3_radio");
	//mRadio4 = getChild<LLButton>("4_radio");
	//mRadioCount = getChild<LLLineEditor>("radio_count");
	//mRadioVal1 = getChild<LLLineEditor>("radio_val1");
	//mRadioVal2 = getChild<LLLineEditor>("radio_val2");
	//mRadioVal3 = getChild<LLLineEditor>("radio_val3");
	//mRadioVal4 = getChild<LLLineEditor>("radio_val4");
	//mRadioLabel1 = getChild<LLLineEditor>("radio_label1");
	//mRadioLabel2 = getChild<LLLineEditor>("radio_label2");
	//mRadioLabel3 = getChild<LLLineEditor>("radio_label3");
	//mRadioLabel4 = getChild<LLLineEditor>("radio_label4");

	mWidgetCount = 0;
	//BD - Fix for rare invisible sidebar. 
	mFirstTime = true;
	//BD - Fix for rare wrongly visible trash buttons.
	mEditMode = false;

	//loadWidgetList();

	LLComboBox* settings_combo = getChild<LLComboBox>("debug_setting");
	struct f : public LLControlGroup::ApplyFunctor
	{
		LLComboBox* combo;
		f(LLComboBox* c) : combo(c) {}
		virtual void apply(const std::string& name, LLControlVariable* control)
		{
			if (!control->isHiddenFromSettingsEditor())
			{
				combo->add(name, (void*)control);
			}
		}
	} func(settings_combo);

	std::string key = "all";
	gSavedSettings.applyToAll(&func);
	gSavedPerAccountSettings.applyToAll(&func);

	settings_combo->sortByName();*/
	return TRUE;
}

//BD - Refresh the create-a-widget controls periodically.
/*void LLSideBar::refreshCreationControls()
{
	bool is_checkbox = mIsCheckbox->getValue();
	bool is_slider = mIsSlider->getValue();
	bool is_radio = mIsRadio->getValue();
	bool is_title = mIsTitle->getValue();
	bool is_tab = mIsTab->getValue();

	std::string debug = mDebugCombo->getValue();
	std::string label = mLabelEditor->getValue();
	
	if (((is_checkbox || is_slider || is_radio) && (debug.empty() || label.empty()))
		|| (is_title && label.empty()) )
	{
		//BD - Disable the create button if not all necessary values have been set.
		mNextBtn->setEnabled(false);
	}
	else
	{
		mNextBtn->setEnabled(is_slider || is_checkbox || is_radio || is_title || is_tab);
	}
}*/

//BD - Refresh all controls
void LLSideBar::refreshGraphicControls()
{
	LLVector4 vec4 = gSavedSettings.getVector4("RenderShadowResolution");
	mShadowResX->setValue(vec4.mV[VX]);
	mShadowResY->setValue(vec4.mV[VY]);
	mShadowResZ->setValue(vec4.mV[VZ]);
	mShadowResW->setValue(vec4.mV[VW]);

	vec4 = gSavedSettings.getVector4("RenderShadowDistance");
	mShadowDistX->setValue(vec4.mV[VX]);
	mShadowDistY->setValue(vec4.mV[VY]);
	mShadowDistZ->setValue(vec4.mV[VZ]);
	mShadowDistW->setValue(vec4.mV[VW]);

	LLVector2 vec2 = gSavedSettings.getVector2("RenderProjectorShadowResolution");
	mProjectorResX->setValue(vec2.mV[VX]);
	mProjectorResY->setValue(vec2.mV[VY]);

	LLVector3 vec3 = gSavedSettings.getVector3("ExodusRenderVignette");
	mVignetteX->setValue(vec3.mV[VX]);
	mVignetteY->setValue(vec3.mV[VY]);
	mVignetteZ->setValue(vec3.mV[VZ]);

	LLViewerCamera* viewer_camera = LLViewerCamera::getInstance();
	mCameraAngle->setMaxValue(viewer_camera->getMaxView());
	mCameraAngle->setMinValue(viewer_camera->getMinView());
}

void LLSideBar::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseEnter(x, y, mask);
}

void LLSideBar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseLeave(x, y, mask);
} 

BOOL LLSideBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x, y, mask);
}

void LLSideBar::setVisibleForMouselook(bool visible)
{
	//	//BD - Hide UI In Mouselook
	if (gSavedSettings.getBOOL("AllowUIHidingInML"))
	{
		this->setVisible(visible);
	}
}

//BD - I might want to completely redo this into panels and make it even more static
//     since the otherwise superior and easier method of using layout_stacks has a big
//     issue of not being able to delete it's childs without crashing the Viewer for
//     some reason. Adding more widgets and simply just making the old ones invisible
//     over time bogs down the loading and saving process, causing the Viewer to freeze
//     increasingly longer with each addition. If i can't get this under control there
//     is only the way of packaging everything into normal panels and doing it super
//     fixed which i wanted to prevent in the first place.
/*void LLSideBar::createWidget()
{
	BDSidebarItem* item = new BDSidebarItem;
	//BD - General stuff
	item->mDebugSetting = mDebugCombo->getValue();

	BDSidebarItem::SidebarType type;
	if (getChild<LLUICtrl>("is_checkbox")->getValue())
		type = BDSidebarItem::SidebarType::CHECKBOX;
	else if (getChild<LLUICtrl>("is_radio")->getValue())
		type = BDSidebarItem::SidebarType::RADIO;
	else if (getChild<LLUICtrl>("is_slider")->getValue())
		type = BDSidebarItem::SidebarType::SLIDER;
	else if (getChild<LLUICtrl>("is_title")->getValue())
		type = BDSidebarItem::SidebarType::TITLE;
	else
		type = BDSidebarItem::SidebarType::TAB;
	item->mType = type;

	item->mLabel = mLabelEditor->getValue();
	item->mPanelName = llformat("widget_%i", mWidgetCount);

	//BD - Checkbox stuff
	//     Nothing.

	//BD - Slider stuff
	if (type == 2)
	{
		item->mDecimals = mDecimalsEditor->getValue();
		item->mIncrement = mIncrementEditor->getValue().asReal();
		item->mMaxValue = mMaxEditor->getValue().asReal();
		item->mMinValue = mMinEditor->getValue().asReal();
		if (mIsX->getValue())
			item->mArray = 0;
		else if (mIsY->getValue())
			item->mArray = 1;
		else if (mIsZ->getValue())
			item->mArray = 2;
		else if (mIsW->getValue())
			item->mArray = 3;
		else
			item->mArray = -1;
	}
	//BD - Radio stuff
	else if (type == 1)
	{
		//if (mRadio2->getValue())
		//	item->mRadioCount = 2;
		//else if (mRadio3->getValue())
		//	item->mRadioCount = 3;
		//else if (mRadio4->getValue())
		//	item->mRadioCount = 4;
		//else
		//	item->mRadioCount = 1;
		//item->mRadioCount = getChild<LLUICtrl>("radio_count")->getValue();
		//item->mRadioValues.mV[VX] = getChild<LLUICtrl>("radio_val1")->getValue().asReal();
		//item->mRadioValues.mV[VY] = getChild<LLUICtrl>("radio_val2")->getValue().asReal();
		//item->mRadioValues.mV[VZ] = getChild<LLUICtrl>("radio_val3")->getValue().asReal();
		//item->mRadioValues.mV[VW] = getChild<LLUICtrl>("radio_val4")->getValue().asReal();
		//item->mRadioLabel1 = getChild<LLUICtrl>("radio_label1")->getValue();
		//item->mRadioLabel2 = getChild<LLUICtrl>("radio_label2")->getValue();
		//item->mRadioLabel3 = getChild<LLUICtrl>("radio_label3")->getValue();
		//item->mRadioLabel4 = getChild<LLUICtrl>("radio_label4")->getValue();
	}

	mSidebarItems.push_back(item);
	mWidgetCount++;

	saveWidgetList();
	//loadWidgetList();

	//BD - Empty all widgets when we're done.
	//     Don't empty debugs in case we are doing multi-slider options also making
	//     it easier to go from one debug setting to the next if you are working down
	//     a whole type of debug settings like RenderDeferred~ and Camera~ for instance.
	LLSD empty;
	empty.clear();

	//BD - General Stuff
	mIsCheckbox->setValue(empty);
	mIsSlider->setValue(empty);
	mIsTitle->setValue(empty);
	mIsTab->setValue(empty);
	//mIsRadio->setValue(empty);
	mLabelEditor->setValue(empty);

	//BD - Checkbox stuff
	//     Nothing.

	//BD - Slider stuff
	mDecimalsEditor->setValue(empty);
	mIncrementEditor->setValue(empty);
	mMaxEditor->setValue(empty);
	mMinEditor->setValue(empty);
	mIsX->setValue(empty);
	mIsY->setValue(empty);
	mIsZ->setValue(empty);
	mIsW->setValue(empty);

	//BD - Radio stuff
	//mRadio2->setValue(empty);
	//mRadio3->setValue(empty);
	//mRadio4->setValue(empty);
	//mRadioCount->setValue(empty);
	//mRadioVal1->setValue(empty);
	//mRadioVal2->setValue(empty);
	//mRadioVal3->setValue(empty);
	//mRadioVal4->setValue(empty);
	//mRadioLabel1->setValue(empty);
	//mRadioLabel2->setValue(empty);
	//mRadioLabel3->setValue(empty);
	//mRadioLabel4->setValue(empty);

	//BD - Refresh our controls.
	onTypeSelection();
}

void LLSideBar::deleteWidget(LLUICtrl* ctrl)
{
	LLView* layout_panel = ctrl->getParent()->getParent();

	if (layout_panel)
	{
		for (std::vector<BDSidebarItem*>::iterator iter = mSidebarItems.begin();
			iter != mSidebarItems.end(); ++iter)
		{
			BDSidebarItem* item = (*iter);
			if (item)
			{
				if (item->mPanelName == layout_panel->getName())
				{
					mSidebarItems.erase(iter);
					break;
				}
			}
		}
	}

	saveWidgetList();
	//loadWidgetList();
}

void LLSideBar::toggleEditMode()
{
	if (mEditMode)
	{
		saveWidgetList();
	}

	mEditMode = !mEditMode;
	mMainLayout->setVisible(mEditMode);

	for (std::vector<BDSidebarItem*>::iterator iter = mSidebarItems.begin();
		iter != mSidebarItems.end(); ++iter)
	{
		BDSidebarItem* item = (*iter);
		if (item)
			item->mRemoveBtn->setVisible(mEditMode);
	}
}

//BD
bool LLSideBar::loadWidgetList()
{
	mWidgetCount = 0;
	LLSD widget;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sidebar.xml");
	llifstream infile;

	//BD - Use the defaults coming with the Viewer if we don't have any custom layout yet.
	//     Should this fail we'll bail out anyway.
	if (!gDirUtilp->fileExists(filename))
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "sidebar.xml");
	}

	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Sidebar") << "Cannot find file in: " << filename << LL_ENDL;
		return FALSE;
	}

	if (!mFirstTime)
		mWidgetsStack->translate(0, -mOffset - 5);

	//BD - Delete all items so we can write it a new to prevent doubles.
	//     Plain simple method.
	mSidebarItems.clear();

	mOffset = 0;
	S32 count = 0;
	while (!infile.eof())
	{
		if (LLSDSerialize::fromXML(widget, infile) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Sidebar") << "Failed to parse file" << filename << LL_ENDL;
			return FALSE;
		}

		BDSidebarItem::SidebarType type = BDSidebarItem::SidebarType::CHECKBOX;
		switch (widget["type"].asInteger())
		{
		case 0:
			type = BDSidebarItem::SidebarType::CHECKBOX;
			break;
		case 1:
			type = BDSidebarItem::SidebarType::RADIO;
			break;
		case 2:
			type = BDSidebarItem::SidebarType::SLIDER;
			break;
		case 3:
			type = BDSidebarItem::SidebarType::TITLE;
			break;
		case 4:
			type = BDSidebarItem::SidebarType::TAB;
			break;
		}

		std::string debug_str = widget["debug_setting"].asString();
		LLControlVariable* controlp = gSavedSettings.getControl(debug_str);
		if (!controlp && (type == 0 || type == 1 || type == 2))
		{
			LL_WARNS("Sidebar") << "Corrupt or missing essential debug, skipping widget creation" << LL_ENDL;
			continue;
		}

		std::string panel_name;
		LLLayoutPanel::Params panel_p;
		panel_p.name = llformat("widget_%i", count);
		panel_p.background_visible = false;
		panel_p.has_border = false;
		panel_p.mouse_opaque = false;
		panel_p.auto_resize = false;
		panel_p.user_resize = false;
		switch (type)
		{
		case BDSidebarItem::SidebarType::SLIDER:
			panel_p.min_dim = 15;
			panel_name = "slider";
			break;
		case BDSidebarItem::SidebarType::RADIO:
			panel_p.min_dim = 16;
			panel_name = "radio";
			break;
		case BDSidebarItem::SidebarType::CHECKBOX:
			panel_p.min_dim = 16;
			panel_name = "checkbox";
			break;
		case BDSidebarItem::SidebarType::TITLE:
			panel_p.min_dim = 28;
			panel_name = "title";
			break;
		case BDSidebarItem::SidebarType::TAB:
			panel_p.min_dim = 10;
			panel_name = "tab";
			break;
		}

		LLLayoutPanel* layout_panel = layout_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p, mWidgetsStack);
		LLPanel* panel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_sidebar_tab_" + panel_name + ".xml", layout_panel, LLPanel::child_registry_t::instance());
	
		if (panel && layout_panel)
		{
			layout_panel->addChild(panel);
			//mWidgetsStack->addPanel(layout_panel);

			BDSidebarItem* item = new BDSidebarItem;
			//BD - General stuff
			item->mDebugSetting = debug_str;
			item->mType = type;
			item->mLabel = widget["label"].asString();
			item->mOrder = widget["order"].asInteger();
			item->mPanelName = layout_panel->getName();
			item->mPanel = layout_panel;
			item->mRemoveBtn = panel->getChild<LLButton>("remove_widget");
			item->mRemoveBtn->setMouseDownCallback(boost::bind(&LLSideBar::deleteWidget, this, _1));
			item->mRemoveBtn->setVisible(mEditMode);

			if (type == BDSidebarItem::SidebarType::SLIDER)
			{
				LLSD value = controlp->getValue();
				LLSliderCtrl* ctrl = panel->getChild<LLSliderCtrl>("slider");
				LLVector4 vec4;

				vec4.setValue(value);
				ctrl->setLabel(widget["label"].asString());

				//BD - Slider stuff
				item->mDecimals = widget["decimals"].asReal();
				item->mIncrement = widget["increment"].asReal();
				item->mMaxValue = widget["max_val"].asReal();
				item->mMinValue = widget["min_val"].asReal();
				item->mArray = widget["array"].asInteger();

				if (item->mArray == 0)
				{
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitX, _1, debug_str));
					value = vec4.mV[VX];
				}
				else if (item->mArray == 1)
				{
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitY, _1, debug_str));
					value = vec4.mV[VY];
				}
				else if (item->mArray == 2)
				{
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitZ, _1, debug_str));
					value = vec4.mV[VZ];
				}
				else if (item->mArray == 3)
				{
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitW, _1, debug_str));
					value = vec4.mV[VW];
				}
				else
				{
					ctrl->setControlVariable(controlp);
				}

				ctrl->setMinValue(item->mMinValue);
				ctrl->setMaxValue(item->mMaxValue);
				ctrl->setIncrement(item->mIncrement);
				ctrl->setPrecision(item->mDecimals);
				ctrl->setValue(value);
			}
			//BD - Currently not functional due to the inability to change sub-radios.
			else if (type == BDSidebarItem::SidebarType::RADIO)
			{
				LLRadioGroup* ctrl = panel->getChild<LLRadioGroup>("radio");
				ctrl->setControlVariable(controlp);

				//BD - Radio stuff
				item->mRadioCount = widget["radio_count"].asInteger();
				item->mRadioValues.mV[VX] = widget["value1"].asReal();
				item->mRadioValues.mV[VY] = widget["value2"].asReal();
				item->mRadioValues.mV[VZ] = widget["value3"].asReal();
				item->mRadioValues.mV[VW] = widget["value4"].asReal();
				item->mRadioLabel1 = widget["label1"].asString();
				item->mRadioLabel2 = widget["label2"].asString();
				item->mRadioLabel3 = widget["label3"].asString();
				item->mRadioLabel4 = widget["label4"].asString();

				//BD - Can't do any of this atm. Need to implement for the radiobutton first.
				LLRadioCtrl* radio1 = ctrl->getChild<LLRadioCtrl>("radio_1");
				radio1->setValue(widget["value1"].asReal());
				LLRadioCtrl* radio2 = ctrl->getChild<LLRadioCtrl>("radio_2");
				radio2->setValue(widget["value2"].asReal());
				LLRadioCtrl* radio3 = ctrl->getChild<LLRadioCtrl>("radio_3");
				radio3->setValue(widget["value3"].asReal());
				LLRadioCtrl* radio4 = ctrl->getChild<LLRadioCtrl>("radio_4");
				radio4->setValue(widget["value4"].asReal());
				if (widget["radio_count"].asInteger() < 2)
					removeChild(radio2);
				if (widget["radio_count"].asInteger() < 3)
					removeChild(radio3);
				if (widget["radio_count"].asInteger() < 4)
					removeChild(radio4);
			}
			else if (type == BDSidebarItem::SidebarType::CHECKBOX)
			{
				LLCheckBoxCtrl* ctrl = panel->getChild<LLCheckBoxCtrl>("checkbox");
				ctrl->setControlVariable(controlp);
				ctrl->setLabel(item->mLabel);

				//BD - Checkbox stuff
				//     Nothing.
			}
			else if (type == BDSidebarItem::SidebarType::TITLE)
			{
				LLTextBase* ctrl = panel->getChild<LLTextBase>("title");
				ctrl->setValue(item->mLabel);

				//BD - Title stuff
				//     Nothing.
			}
			else
			{
				//BD - Tab stuff
				//     Nothing.
			}

			mSidebarItems.push_back(item);
			mWidgetCount++;

			//BD - Add our size to the scroll panel.
			mOffset += panel_p.min_dim;
		}
		count++;
	}
	infile.close();

	//BD - Now resize our scroll panel.
	LLRect rect = mScrollPanel->getRect();
	rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.mRight, 5 + mOffset);
	mScrollPanel->setRect(rect);
	mWidgetsStack->translate(0, 5 + mOffset);

	mFirstTime = false;
	return TRUE;
}

void LLSideBar::saveWidgetList()
{
	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sidebar.xml");
	LLSD record;
	llofstream file;
	file.open(full_path.c_str());
	for (std::vector<BDSidebarItem*>::iterator iter = mSidebarItems.begin();
		iter != mSidebarItems.end(); ++iter)
	{
		BDSidebarItem* item = (*iter);
		if (item)
		{
			//BD - General stuff
			record["debug_setting"] = item->mDebugSetting;
			record["type"] = item->mType;
			record["label"] = item->mLabel;
			record["order"] = item->mOrder;
			record["panel_name"] = item->mPanelName;

			//BD - Checkbox stuff
			//     Nothing.

			//BD - Slider stuff
			if (item->mType == 2)
			{ 
				record["decimals"] = item->mDecimals;
				record["increment"] = item->mIncrement;
				record["max_val"] = item->mMaxValue;
				record["min_val"] = item->mMinValue;
				record["array"] = item->mArray;
			}
			//BD - Radio stuff
			else if (item->mType == 1)
			{
				//record["radio_count"] = item->mRadioCount;
				record["value1"] = item->mRadioValues.mV[VX];
				record["value2"] = item->mRadioValues.mV[VY];
				record["value3"] = item->mRadioValues.mV[VZ];
				record["value4"] = item->mRadioValues.mV[VW];
				record["label1"] = item->mRadioLabel1;
				record["label2"] = item->mRadioLabel2;
				record["label3"] = item->mRadioLabel3;
				record["label4"] = item->mRadioLabel4;
			}

			LLSDSerialize::toXML(record, file);
		}
	}
	file.close();
}

void LLSideBar::onTypeSelection()
{
	bool is_checkbox = mIsCheckbox->getValue();
	bool is_slider = mIsSlider->getValue();
	bool is_radio = mIsRadio->getValue();
	bool is_title = mIsTitle->getValue();
	bool is_tab = mIsTab->getValue();

	getChild<LLPanel>("slider2")->setEnabled(is_slider);
	mDecimalsEditor->setEnabled(is_slider);
	mIncrementEditor->setEnabled(is_slider);
	mMinEditor->setEnabled(is_slider);
	mMaxEditor->setEnabled(is_slider);
	mLabelEditor->setEnabled(is_slider || is_checkbox || is_radio || is_title);
	mDebugCombo->setEnabled(is_slider || is_checkbox);

	getChild<LLPanel>("slider2")->setVisible(is_slider);
	mDecimalsEditor->setVisible(is_slider);
	mIncrementEditor->setVisible(is_slider);
	mMinEditor->setVisible(is_slider);
	mMaxEditor->setVisible(is_slider);

	mIsSlider->setEnabled(!is_checkbox && !is_radio && !is_title && !is_tab);
	//mIsRadio->setEnabled(!is_checkbox && !is_slider);
	mIsCheckbox->setEnabled(!is_slider && !is_radio && !is_title && !is_tab);
	mIsTitle->setEnabled(!is_checkbox && !is_slider && !is_radio && !is_tab);
	mIsTab->setEnabled(!is_checkbox && !is_slider && !is_radio && !is_title);
}





void LLSideBar::onDeleteTest()
{
	for (std::vector<LLLayoutPanel*>::iterator iter = mSidebarPanels.begin();
		iter != mSidebarPanels.end(); ++iter)
	{
		LLLayoutPanel* item = *iter;
		if (item)
		{
			mTestStack->LLLayoutStack::removeChild(static_cast<LLView*>(*iter));
			//mTestStack->removeChild(static_cast<LLView*>(item));
		}
	}
}

//BD
void LLSideBar::onAddTest()
{
	{
		//BDSidebarItem::SidebarType type = BDSidebarItem::SidebarType::CHECKBOX;

		std::string debug_str = widget["debug_setting"].asString();
		LLControlVariable* controlp = gSavedSettings.getControl(debug_str);
		if (!controlp && (type == 0 || type == 1 || type == 2))
		{
			LL_WARNS("Sidebar") << "Corrupt or missing essential debug, skipping widget creation" << LL_ENDL;
			continue;
		}

		std::string panel_name;
		LLLayoutPanel::Params panel_p;
		panel_p.name = "widget_test";
		panel_p.background_visible = true;
		panel_p.has_border = true;
		panel_p.mouse_opaque = false;
		panel_p.auto_resize = false;
		panel_p.user_resize = false;
		panel_p.min_dim = 16;
		//panel_name = "checkbox";

		LLLayoutPanel* layout_panel = layout_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p, NULL);
		layout_panel->setName("widget_test");
		mTestStack->addPanel(layout_panel, LLLayoutStack::NO_ANIMATE);
		//LLPanel* panel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_sidebar_tab_" + panel_name + ".xml", layout_panel, LLPanel::child_registry_t::instance());

		//if (panel && layout_panel)
		{
			//panel->setName()
			//layout_panel->addChild(panel);
			//mTestStack->addPanel(layout_panel);

			//LLButton* button = panel->getChild<LLButton>("remove_widget");
			//button->setMouseDownCallback(boost::bind(&LLSideBar::deleteWidget, this, _1));
			//button->setVisible(mEditMode);

			//LLCheckBoxCtrl* ctrl = panel->getChild<LLCheckBoxCtrl>("checkbox");
			//ctrl->setControlVariable(controlp);
			//ctrl->setLabel(item->mLabel);

			//BD - Add our size to the scroll panel.
			//mOffset += panel_p.min_dim;

			mSidebarPanels.push_back(layout_panel);
		}
	}
}*/