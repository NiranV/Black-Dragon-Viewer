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
#include "llcheckboxctrl.h"
#include "lllayoutstack.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "llview.h"

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
	//mCommitCallbackRegistrar.add("Sidebar.AddWidget", boost::bind(&LLSideBar::onClickAddWidget, this));
	//mCommitCallbackRegistrar.add("Sidebar.CreateWidget", boost::bind(&LLSideBar::createWidget, this));
	mCommitCallbackRegistrar.add("Sidebar.ToggleEdit", boost::bind(&LLSideBar::toggleEditMode, this));
	mCommitCallbackRegistrar.add("Sidebar.Load", boost::bind(&LLSideBar::loadWidgetList, this));

	mCommitCallbackRegistrar.add("Sidebar.Next", boost::bind(&LLSideBar::onClickNext, this));
	mCommitCallbackRegistrar.add("Sidebar.Previous", boost::bind(&LLSideBar::onClickPrevious, this));

	mCommitCallbackRegistrar.add("Sidebar.Type", boost::bind(&LLSideBar::onTypeSelection, this, _1, _2));

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

//BD
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
	else
		type = BDSidebarItem::SidebarType::SLIDER;
	item->mType = type;

	item->mLabel = getChild<LLUICtrl>("label")->getValue();
	item->mPanelName = llformat("widget_%i", mWidgetCount);

	//BD - Checkbox stuff
	//     Nothing.

	//BD - Slider stuff
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

	//BD - Radio stuff
	if (getChild<LLUICtrl>("2_radio")->getValue())
		item->mRadioCount = 2;
	else if (getChild<LLUICtrl>("3_radio")->getValue())
		item->mRadioCount = 3;
	else if (getChild<LLUICtrl>("4_radio")->getValue())
		item->mRadioCount = 4;
	else
		item->mRadioCount = 1;
	//item->mRadioCount = getChild<LLUICtrl>("radio_count")->getValue();
	item->mRadioValues.mV[VX] = getChild<LLUICtrl>("radio_val1")->getValue().asReal();
	item->mRadioValues.mV[VY] = getChild<LLUICtrl>("radio_val2")->getValue().asReal();
	item->mRadioValues.mV[VZ] = getChild<LLUICtrl>("radio_val3")->getValue().asReal();
	item->mRadioValues.mV[VW] = getChild<LLUICtrl>("radio_val4")->getValue().asReal();
	item->mRadioLabel1 = getChild<LLUICtrl>("radio_label1")->getValue();
	item->mRadioLabel2 = getChild<LLUICtrl>("radio_label2")->getValue();
	item->mRadioLabel3 = getChild<LLUICtrl>("radio_label3")->getValue();
	item->mRadioLabel4 = getChild<LLUICtrl>("radio_label4")->getValue();

	//BD - Button stuff
	//item->mIsToggle = getChild<LLUICtrl>("is_toggle")->getValue();

	mSidebarItems.push_back(item);
	mWidgetCount++;

	saveWidgetList();
	loadWidgetList();

	LLSD empty;
	empty.clear();
	//BD - Empty all widgets when we're done.
	getChild<LLUICtrl>("debug_setting")->setValue(empty);
	getChild<LLUICtrl>("is_checkbox")->setValue(empty);
	getChild<LLUICtrl>("is_radio")->setValue(empty);
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
	getChild<LLUICtrl>("2_radio")->setValue(empty);
	getChild<LLUICtrl>("3_radio")->setValue(empty);
	getChild<LLUICtrl>("4_radio")->setValue(empty);
	//item->mRadioCount = getChild<LLUICtrl>("radio_count")->getValue();
	getChild<LLUICtrl>("radio_val1")->setValue(empty);
	getChild<LLUICtrl>("radio_val2")->setValue(empty);
	getChild<LLUICtrl>("radio_val3")->setValue(empty);
	getChild<LLUICtrl>("radio_val4")->setValue(empty);
	getChild<LLUICtrl>("radio_label1")->setValue(empty);
	getChild<LLUICtrl>("radio_label2")->setValue(empty);
	getChild<LLUICtrl>("radio_label3")->setValue(empty);
	getChild<LLUICtrl>("radio_label4")->setValue(empty);
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
				LL_WARNS("Sidebar") << layout_panel->getName() << " (" << item->mPanelName << ")" << LL_ENDL;
				if (item->mPanelName == layout_panel->getName())
				{
					LL_WARNS("Sidebar") << "Deleted: " << layout_panel->getName() << " (" << item->mPanelName << ")" << LL_ENDL;
					mSidebarItems.erase(iter);
					break;
				}
			}
		}
	}

	saveWidgetList();
}

void LLSideBar::toggleEditMode()
{
	if (mEditMode)
	{
		saveWidgetList();
	}
	else
	{
		mCurrentPage = "page1";
	}
	mEditMode = !mEditMode;
}

bool LLSideBar::loadWidgetList()
{
	mWidgetCount = 0;
	LLSD widget;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "sidebar.xml");
	llifstream infile;
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
		view->setVisible(false);
		LL_INFOS("Sidebar") << "Available are: " << view->getName() << LL_ENDL;
	}

	//BD - Delete all items so we can write it a new to prevent doubles.
	//     Plain simple method.
	mSidebarItems.clear();

	S32 count = 0;
	while (!infile.eof())
	{
		if (LLSDSerialize::fromXML(widget, infile) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Sidebar") << "Failed to parse file" << filename << LL_ENDL;
			return FALSE;
		}

		BDSidebarItem::SidebarType type;
		if (widget["type"].asString() == "0")
			type = BDSidebarItem::SidebarType::CHECKBOX;
		else if (widget["type"].asString() == "1")
			type = BDSidebarItem::SidebarType::RADIO;
		else
			type = BDSidebarItem::SidebarType::SLIDER;

		LLLayoutPanel::Params panel_p;
		panel_p.name = llformat("widget_%i", count);
		panel_p.background_visible = false;
		panel_p.has_border = false;
		panel_p.mouse_opaque = false;
		if (type == BDSidebarItem::SidebarType::SLIDER)
			panel_p.min_dim = 14;
		else
			panel_p.min_dim = 16;
		panel_p.auto_resize = false;
		panel_p.user_resize = false;

		LLLayoutPanel* layout_panel = layout_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
		std::string panel_name;
		if (type == BDSidebarItem::SidebarType::SLIDER)
			panel_name = "slider";
		else if (type == BDSidebarItem::SidebarType::RADIO)
			panel_name = "radio";
		else
			panel_name = "checkbox";

		
		LLPanel* panel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_sidebar_tab_" + panel_name + ".xml", NULL, LLPanel::child_registry_t::instance());
	
		if (panel && layout_panel)
		{
			layout_panel->addChild(panel);
			layout_stack->addPanel(layout_panel);

			if (type == BDSidebarItem::SidebarType::SLIDER)
			{
				LL_WARNS("Posing") << "Inserting slider: " << widget["debug_setting"].asString() << " " << widget["label"].asString() << LL_ENDL;
				LLSliderCtrl* ctrl = panel->getChild<LLSliderCtrl>("slider");
				ctrl->setLabel(widget["label"].asString());
				if (widget["array"].asInteger() == 0)
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitX, _1, widget["debug_setting"].asString()));
				else if (widget["array"].asInteger() == 1)
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitY, _1, widget["debug_setting"].asString()));
				else if (widget["array"].asInteger() == 2)
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitZ, _1, widget["debug_setting"].asString()));
				else if (widget["array"].asInteger() == 3)
					ctrl->setCommitCallback(boost::bind(&LLFloaterPreference::onCommitW, _1, widget["debug_setting"].asString()));
				else
				{
					LL_WARNS("Posing") << "Setting normal slider" << LL_ENDL;
					ctrl->setControlVariable(gSavedSettings.getControl(widget["debug_setting"].asString()));
				}
				ctrl->setValue(gSavedSettings.getF32(widget["debug_setting"].asString()));
				ctrl->setMinValue(widget["min_val"].asReal());
				ctrl->setMaxValue(widget["max_val"].asReal());
				ctrl->setIncrement(widget["increment"].asReal());
				ctrl->setPrecision(widget["decimals"].asInteger());
			}
			//BD - Currently not functional due to the inability to change sub-radios.
			else if (type == BDSidebarItem::SidebarType::RADIO)
			{
				LL_WARNS("Posing") << "Inserting radio: " << widget["debug_setting"].asString() << " " << widget["label"].asString() << LL_ENDL;
				LLRadioGroup* ctrl = panel->getChild<LLRadioGroup>("radio");
				ctrl->setControlVariable(gSavedSettings.getControl(widget["debug_setting"].asString()));
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
			else
			{
				LL_WARNS("Posing") << "Inserting checkbox: " << widget["debug_setting"].asString() << " " << widget["label"].asString() << LL_ENDL;
				LLCheckBoxCtrl* ctrl = panel->getChild<LLCheckBoxCtrl>("checkbox");
				ctrl->setControlVariable(gSavedSettings.getControl(widget["debug_setting"].asString()));
				ctrl->setLabel(widget["label"].asString());
			}
			panel->getChild<LLUICtrl>("remove_widget")->setMouseDownCallback(boost::bind(&LLSideBar::deleteWidget, this, _1));

			BDSidebarItem* item = new BDSidebarItem;
			//BD - General stuff
			item->mDebugSetting = widget["debug_setting"].asString();
			item->mType = type;
			item->mLabel = widget["label"].asString();

			item->mOrder = widget["order"].asInteger();
			//record["panel"] = item->mPanel;
			item->mPanelName = layout_panel->getName();

			//BD - Checkbox stuff
			//     Nothing.

			//BD - Slider stuff
			item->mDecimals = widget["decimals"].asReal();
			item->mIncrement = widget["increment"].asReal();
			item->mMaxValue = widget["max_val"].asReal();
			item->mMinValue = widget["min_val"].asReal();
			item->mArray = widget["array"].asInteger();

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

			//BD - Button stuff
			item->mIsToggle = widget["toggle"].asBoolean();

			mSidebarItems.push_back(item);
			mWidgetCount++;
		}
		count++;
	}
	infile.close();
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
			//record["panel"] = item->mPanel;
			record["panel_name"] = item->mPanelName;

			//BD - Checkbox stuff
			//     Nothing.

			//BD - Slider stuff
			record["decimals"] = item->mDecimals;
			record["increment"] = item->mIncrement;
			record["max_val"] = item->mMaxValue;
			record["min_val"] = item->mMinValue;
			record["array"] = item->mArray;

			//BD - Radio stuff
			record["radio_count"] = item->mRadioCount;
			record["value1"] = item->mRadioValues.mV[VX];
			record["value2"] = item->mRadioValues.mV[VY];
			record["value3"] = item->mRadioValues.mV[VZ];
			record["value4"] = item->mRadioValues.mV[VW];
			record["label1"] = item->mRadioLabel1;
			record["label2"] = item->mRadioLabel2;
			record["label3"] = item->mRadioLabel3;
			record["label4"] = item->mRadioLabel4;

			//BD - Button stuff
			record["toggle"] = item->mIsToggle;

			LLSDSerialize::toXML(record, file);
		}
	}
	file.close();
}

void LLSideBar::onClickNext()
{
	bool is_checkbox = getChild<LLButton>("is_checkbox")->getValue();
	bool is_slider = getChild<LLButton>("is_slider")->getValue();
	bool is_radio = getChild<LLButton>("is_radio")->getValue();
	if (mCurrentPage == "page1")
	{
		if (is_checkbox)
		{
			//BD - We are done here, checkbox only has the general page.
			createWidget();
		}
		else if (is_slider)
		{
			mCurrentPage = "slider1";
		}
		else if (is_radio)
		{
			mCurrentPage = "radio1";
		}
	}
	else if (mCurrentPage == "slider1")
	{
		mCurrentPage = "slider2";
	}
	else if (mCurrentPage == "radio1")
	{
		mCurrentPage = "radio2";
	}
	else if (mCurrentPage == "radio2")
	{
		mCurrentPage = "radio3";
	}
	else
	{
		createWidget();
		mCurrentPage = "page1";
	}

	LL_WARNS("Sidebar") << "Current page: " << mCurrentPage << LL_ENDL;
	getChild<LLButton>("previous")->setEnabled(mCurrentPage != "page1");

	getChild<LLPanel>("page1")->setVisible(mCurrentPage == "page1");
	getChild<LLPanel>("slider1")->setVisible(mCurrentPage == "slider1");
	getChild<LLPanel>("slider2")->setVisible(mCurrentPage == "slider2");
	getChild<LLPanel>("radio1")->setVisible(mCurrentPage == "radio1");
	getChild<LLPanel>("radio2")->setVisible(mCurrentPage == "radio2");
	getChild<LLPanel>("radio3")->setVisible(mCurrentPage == "radio3");
}

void LLSideBar::onClickPrevious()
{
	if (mCurrentPage == "slider2")
	{
		mCurrentPage = "slider1";
	}
	else if (mCurrentPage == "radio3")
	{
		mCurrentPage = "radio2";
	}
	else if (mCurrentPage == "radio2")
	{
		mCurrentPage = "radio1";
	}
	else// if (mCurrentPage == "radio1"
	//	|| mCurrentPage == "slider1")
	{
		mCurrentPage = "page1";
	}

	LL_WARNS("Sidebar") << "Current page: " << mCurrentPage << LL_ENDL;

	getChild<LLButton>("previous")->setEnabled(mCurrentPage != "page1");

	getChild<LLPanel>("page1")->setVisible(mCurrentPage == "page1");
	getChild<LLPanel>("slider1")->setVisible(mCurrentPage == "slider1");
	getChild<LLPanel>("slider2")->setVisible(mCurrentPage == "slider2");
	getChild<LLPanel>("radio1")->setVisible(mCurrentPage == "radio1");
	getChild<LLPanel>("radio2")->setVisible(mCurrentPage == "radio2");
	getChild<LLPanel>("radio3")->setVisible(mCurrentPage == "radio3");
}

void LLSideBar::onTypeSelection(LLUICtrl* ctrl, const LLSD& param)
{
	bool is_checkbox = getChild<LLButton>("is_checkbox")->getValue();
	bool is_slider = getChild<LLButton>("is_slider")->getValue();
	bool is_radio = getChild<LLButton>("is_radio")->getValue();

	getChild<LLButton>("next")->setEnabled(is_slider || is_checkbox || is_radio);

	getChild<LLButton>("is_slider")->setEnabled(!is_checkbox && !is_radio);
	getChild<LLButton>("is_radio")->setEnabled(!is_checkbox && !is_slider);
	getChild<LLButton>("is_checkbox")->setEnabled(!is_slider && !is_radio);
}