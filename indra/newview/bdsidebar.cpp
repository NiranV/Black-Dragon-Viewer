/** 
* @file bdsidebar.cpp
* @brief LLSideBar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
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
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* 
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "bdsidebar.h"

// viewer includes
#include "llfloaterpreference.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "llsdserialize.h"

#include "llsliderctrl.h"
#include "llradiogroup.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "lllayoutstack.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "llview.h"
#include "llcombobox.h"

// system includes
#include <iomanip>

LLSideBar *gSideBar = NULL;

LLSideBar::LLSideBar(const LLRect& rect)
:	LLPanel()
{
//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX", boost::bind(&LLFloaterPreference::onCommitX, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY", boost::bind(&LLFloaterPreference::onCommitY, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ", boost::bind(&LLFloaterPreference::onCommitZ, _1, _2));
//	//BD - Vector4
	mCommitCallbackRegistrar.add("Pref.ArrayW", boost::bind(&LLFloaterPreference::onCommitW, _1, _2));

	//BD
	mCommitCallbackRegistrar.add("Sidebar.ToggleEdit", boost::bind(&LLSideBar::toggleEditMode, this));
	mCommitCallbackRegistrar.add("Sidebar.Load", boost::bind(&LLSideBar::loadWidgetList, this));

	mCommitCallbackRegistrar.add("Sidebar.Create", boost::bind(&LLSideBar::createWidget, this));

	mCommitCallbackRegistrar.add("Sidebar.Type", boost::bind(&LLSideBar::onTypeSelection, this));

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
		mShadowDistY->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
		mShadowDistZ->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
		mShadowDistW->setEnabled(!gPipeline.RenderShadowAutomaticDistance);

		//refreshGraphicControls();
		LLPanel::draw();
	}
}

BOOL LLSideBar::postBuild()
{
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
	mWidgetCount = 0;
	mCurrentPage = "page1";

	loadWidgetList();

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

	settings_combo->sortByName();
	return TRUE;
}

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
void LLSideBar::createWidget()
{
	BDSidebarItem* item = new BDSidebarItem;
	//BD - General stuff
	item->mDebugSetting = getChild<LLUICtrl>("debug_setting")->getValue();

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

	item->mLabel = getChild<LLUICtrl>("label")->getValue();
	item->mPanelName = llformat("widget_%i", mWidgetCount);

	//BD - Checkbox stuff
	//     Nothing.

	//BD - Slider stuff
	if (type == 2)
	{
		item->mDecimals = getChild<LLUICtrl>("decimals")->getValue();
		item->mIncrement = getChild<LLUICtrl>("increment")->getValue().asReal();
		item->mMaxValue = getChild<LLUICtrl>("max_value")->getValue().asReal();
		item->mMinValue = getChild<LLUICtrl>("min_value")->getValue().asReal();
		if (getChild<LLUICtrl>("is_x")->getValue())
			item->mArray = 0;
		else if (getChild<LLUICtrl>("is_y")->getValue())
			item->mArray = 1;
		else if (getChild<LLUICtrl>("is_z")->getValue())
			item->mArray = 2;
		else if (getChild<LLUICtrl>("is_w")->getValue())
			item->mArray = 3;
		else
			item->mArray = -1;
	}
	//BD - Radio stuff
	else if (type == 1)
	{

		if (getChild<LLUICtrl>("2_radio")->getValue())
			item->mRadioCount = 2;
		else if (getChild<LLUICtrl>("3_radio")->getValue())
			item->mRadioCount = 3;
		else if (getChild<LLUICtrl>("4_radio")->getValue())
			item->mRadioCount = 4;
		else
			item->mRadioCount = 1;
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

	//BD - Button stuff
	//item->mIsToggle = getChild<LLUICtrl>("is_toggle")->getValue();

	mSidebarItems.push_back(item);
	mWidgetCount++;

	saveWidgetList();
	loadWidgetList();

	//BD - Empty all widgets when we're done.
	//     Don't empty debugs in case we are doing multi-slider options also making
	//     it easier to go from one debug setting to the next if you are working down
	//     a whole type of debug settings like RenderDeferred~ and Camera~ for instance.
	LLSD empty;
	empty.clear();

	//BD - General Stuff
	//getChild<LLUICtrl>("debug_setting")->setValue(empty);
	getChild<LLUICtrl>("is_checkbox")->setValue(empty);
	getChild<LLUICtrl>("is_slider")->setValue(empty);
	getChild<LLUICtrl>("is_title")->setValue(empty);
	getChild<LLUICtrl>("is_tab")->setValue(empty);
	//getChild<LLUICtrl>("is_radio")->setValue(empty);
	getChild<LLUICtrl>("label")->setValue(empty);

	//BD - Checkbox stuff
	//     Nothing.

	//BD - Slider stuff
	getChild<LLUICtrl>("decimals")->setValue(empty);
	getChild<LLUICtrl>("increment")->setValue(empty);
	getChild<LLUICtrl>("max_value")->setValue(empty);
	getChild<LLUICtrl>("min_value")->setValue(empty);
	getChild<LLUICtrl>("is_x")->setValue(empty);
	getChild<LLUICtrl>("is_y")->setValue(empty);
	getChild<LLUICtrl>("is_z")->setValue(empty);
	getChild<LLUICtrl>("is_w")->setValue(empty);

	//BD - Radio stuff
	//getChild<LLUICtrl>("2_radio")->setValue(empty);
	//getChild<LLUICtrl>("3_radio")->setValue(empty);
	//getChild<LLUICtrl>("4_radio")->setValue(empty);
	//getChild<LLUICtrl>("radio_count")->setValue(empty);
	//getChild<LLUICtrl>("radio_val1")->setValue(empty);
	//getChild<LLUICtrl>("radio_val2")->setValue(empty);
	//getChild<LLUICtrl>("radio_val3")->setValue(empty);
	//getChild<LLUICtrl>("radio_val4")->setValue(empty);
	//getChild<LLUICtrl>("radio_label1")->setValue(empty);
	//getChild<LLUICtrl>("radio_label2")->setValue(empty);
	//getChild<LLUICtrl>("radio_label3")->setValue(empty);
	//getChild<LLUICtrl>("radio_label4")->setValue(empty);

	//BD - Refresh our controls.
	onTypeSelection();
}

void LLSideBar::deleteWidget(LLUICtrl* ctrl)
{
	LLView* layout_panel = ctrl->getParent()->getParent();

	if (layout_panel)
	{
		layout_panel->setVisible(false);

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
	loadWidgetList();
}

void LLSideBar::toggleEditMode()
{
	if (mEditMode)
	{
		saveWidgetList();
	}

	mEditMode = !mEditMode;
	getChild<LLLayoutPanel>("create_layout")->setVisible(mEditMode);
	LLLayoutStack* layout_stack = getChild<LLLayoutStack>("widgets_stack");
	const LLView::child_list_t* list = layout_stack->getChildList();
	for (LLView::child_list_t::const_iterator it = list->begin();
		it != list->end(); ++it)
	{
		LLView* view = *it;
		if (view->getVisible())
			view->getChild<LLUICtrl>("remove_widget")->setVisible(mEditMode);
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

	//BD - Kill any previous layout panels we may have still there.
	LLLayoutStack* layout_stack = getChild<LLLayoutStack>("widgets_stack");
	const LLView::child_list_t* list = layout_stack->getChildList();
	for (LLView::child_list_t::const_iterator it = list->begin();
		it != list->end(); ++it)
	{
		LLView* view = *it;
		if (view->getVisible())
			view->setVisible(false);
	}

	getChild<LLLayoutStack>("widgets_stack")->translate(0, -mOffset - 5);

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

		LLLayoutPanel* layout_panel = layout_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
		LLPanel* panel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_sidebar_tab_" + panel_name + ".xml", NULL, LLPanel::child_registry_t::instance());
	
		if (panel && layout_panel)
		{
			std::string debug_str = widget["debug_setting"].asString();

			layout_panel->addChild(panel);
			layout_stack->addPanel(layout_panel);

			panel->getChild<LLUICtrl>("remove_widget")->setMouseDownCallback(boost::bind(&LLSideBar::deleteWidget, this, _1));
			panel->getChild<LLUICtrl>("remove_widget")->setVisible(mEditMode);

			BDSidebarItem* item = new BDSidebarItem;
			//BD - General stuff
			item->mDebugSetting = debug_str;
			item->mType = type;
			item->mLabel = widget["label"].asString();
			item->mOrder = widget["order"].asInteger();
			item->mPanelName = layout_panel->getName();

			if (type == BDSidebarItem::SidebarType::SLIDER)
			{
				LLControlVariable* controlp = gSavedSettings.getControl(debug_str);
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
				ctrl->setControlVariable(gSavedSettings.getControl(debug_str));

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
				/*LLRadioCtrl* radio1 = ctrl->getChild<LLRadioCtrl>("radio_1");
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
				*/
			}
			else if (type == BDSidebarItem::SidebarType::CHECKBOX)
			{
				LLCheckBoxCtrl* ctrl = panel->getChild<LLCheckBoxCtrl>("checkbox");
				ctrl->setControlVariable(gSavedSettings.getControl(debug_str));
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

				//BD - Button stuff
				//item->mIsToggle = widget["toggle"].asBoolean();
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
	//     FIXME: The sidebar scroll container panel turns invisible randomly sometimes.
	//     No idea why, haven't paying attention too much to what i did when it happened.
	//     I'm sure i'll find it soon.
	LLRect rect = getChild<LLPanel>("scroll_panel")->getRect();
	rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.mRight, 5 + mOffset);
	getChild<LLPanel>("scroll_panel")->setRect(rect);
	getChild<LLLayoutStack>("widgets_stack")->translate(0, 5 + mOffset);

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

			//BD - Button stuff
			//record["toggle"] = item->mIsToggle;

			LLSDSerialize::toXML(record, file);
		}
	}
	file.close();
}

void LLSideBar::onTypeSelection()
{
	bool is_checkbox = getChild<LLButton>("is_checkbox")->getValue();
	bool is_slider = getChild<LLButton>("is_slider")->getValue();
	bool is_radio = getChild<LLButton>("is_radio")->getValue();
	bool is_title = getChild<LLButton>("is_title")->getValue();
	bool is_tab = getChild<LLButton>("is_tab")->getValue();

	getChild<LLPanel>("slider2")->setEnabled(is_slider);
	getChild<LLLineEditor>("decimals")->setEnabled(is_slider);
	getChild<LLLineEditor>("increment")->setEnabled(is_slider);
	getChild<LLLineEditor>("min_value")->setEnabled(is_slider);
	getChild<LLLineEditor>("max_value")->setEnabled(is_slider);
	getChild<LLLineEditor>("label")->setEnabled(is_slider || is_checkbox || is_radio || is_title);
	getChild<LLComboBox>("debug_setting")->setEnabled(is_slider || is_checkbox);

	getChild<LLButton>("next")->setEnabled(is_slider || is_checkbox || is_radio || is_title || is_tab);

	getChild<LLPanel>("slider2")->setVisible(is_slider);
	getChild<LLLineEditor>("decimals")->setVisible(is_slider);
	getChild<LLLineEditor>("increment")->setVisible(is_slider);
	getChild<LLLineEditor>("min_value")->setVisible(is_slider);
	getChild<LLLineEditor>("max_value")->setVisible(is_slider);

	getChild<LLButton>("is_slider")->setEnabled(!is_checkbox && !is_radio && !is_title && !is_tab);
	//getChild<LLButton>("is_radio")->setEnabled(!is_checkbox && !is_slider);
	getChild<LLButton>("is_checkbox")->setEnabled(!is_slider && !is_radio && !is_title && !is_tab);
	getChild<LLButton>("is_title")->setEnabled(!is_checkbox && !is_slider && !is_radio && !is_tab);
	getChild<LLButton>("is_tab")->setEnabled(!is_checkbox && !is_slider && !is_radio && !is_title);
}