/** 
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llfloatercamera.h"

// Library includes
#include "llfloaterreg.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "lljoystickbutton.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llslider.h"
#include "llfirstuse.h"
#include "llhints.h"
#include "lltabcontainer.h"
#include "llvoavatarself.h"
// [RLVa:KB] - @setcam
#include "rlvactions.h"
// [/RLVa:KB]

//BD - Bone Camera
#include "llvoavatarself.h"
//BD - Unlimited Camera Presets
#include "lldiriterator.h"
#include "lluictrlfactory.h"
#include "llsdserialize.h"

static LLDefaultChildRegistry::Register<LLPanelCameraItem> r("panel_camera_item");

const F32 NUDGE_TIME = 0.25f;		// in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

// constants
#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define ZOOM "zoom"
#define PRESETS "preset_views_list"
#define CONTROLS "controls"

bool LLFloaterCamera::sFreeCamera = false;
bool LLFloaterCamera::sAppearanceEditing = false;

// Zoom the camera in and out
class LLPanelCameraZoom
:	public LLPanel
{
	LOG_CLASS(LLPanelCameraZoom);
public:
	LLPanelCameraZoom();

	/* virtual */ BOOL	postBuild();
	/* virtual */ void	draw();

protected:
	void	onZoomPlusHeldDown();
	void	onZoomMinusHeldDown();
	void	onSliderValueChanged();
	void	onCameraTrack();
	void	onCameraRotate();
	F32		getOrbitRate(F32 time);
//	//BD - Camera Rolling
	void	onRollLeftHeldDown();
	void	onRollRightHeldDown();

private:
	LLButton*	mPlusBtn;
	LLButton*	mMinusBtn;
	LLSlider*	mSlider;
//	//BD - Camera Rolling
	LLButton*	mRollLeft;
	LLButton*	mRollRight;
};

LLPanelCameraItem::Params::Params()
:	icon_over("icon_over"),
	icon_selected("icon_selected"),
	picture("picture"),
	text("text"),
	selected_picture("selected_picture"),
	mousedown_callback("mousedown_callback")
{
}

LLPanelCameraItem::LLPanelCameraItem(const LLPanelCameraItem::Params& p)
:	LLPanel(p)
{
	LLIconCtrl::Params icon_params = p.picture;
	mPicture = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mPicture);

	icon_params = p.icon_over;
	mIconOver = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mIconOver);

	icon_params = p.icon_selected;
	mIconSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mIconSelected);

	icon_params = p.selected_picture;
	mPictureSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mPictureSelected);

	LLTextBox::Params text_params = p.text;
	mText = LLUICtrlFactory::create<LLTextBox>(text_params);
	addChild(mText);

	if (p.mousedown_callback.isProvided())
	{
		setCommitCallback(initCommitCallback(p.mousedown_callback));
	}
}

void set_view_visible(LLView* parent, const std::string& name, bool visible)
{
	parent->getChildView(name)->setVisible(visible);
}

BOOL LLPanelCameraItem::postBuild()
{
	setMouseEnterCallback(boost::bind(set_view_visible, this, "hovered_icon", true));
	setMouseLeaveCallback(boost::bind(set_view_visible, this, "hovered_icon", false));
	setMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
	setRightMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
	return TRUE;
}

void LLPanelCameraItem::onAnyMouseClick()
{
	if (mCommitSignal) (*mCommitSignal)(this, LLSD());
}

void LLPanelCameraItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
	getChildView("picture")->setVisible( !value["selected"]);
	getChildView("selected_picture")->setVisible( value["selected"]);
}

static LLPanelInjector<LLPanelCameraZoom> t_camera_zoom_panel("camera_zoom_panel");

//-------------------------------------------------------------------------------
// LLPanelCameraZoom
//-------------------------------------------------------------------------------

LLPanelCameraZoom::LLPanelCameraZoom()
:	mPlusBtn( NULL ),
	mMinusBtn( NULL ),
	mSlider( NULL ),
//	//BD - Camera Rolling
	mRollLeft( NULL ),
	mRollRight( NULL )
{
	mCommitCallbackRegistrar.add("Zoom.minus", boost::bind(&LLPanelCameraZoom::onZoomMinusHeldDown, this));
	mCommitCallbackRegistrar.add("Zoom.plus", boost::bind(&LLPanelCameraZoom::onZoomPlusHeldDown, this));
	mCommitCallbackRegistrar.add("Slider.value_changed", boost::bind(&LLPanelCameraZoom::onSliderValueChanged, this));
	mCommitCallbackRegistrar.add("Camera.track", boost::bind(&LLPanelCameraZoom::onCameraTrack, this));
	mCommitCallbackRegistrar.add("Camera.rotate", boost::bind(&LLPanelCameraZoom::onCameraRotate, this));
//	//BD - Camera Rolling
	mCommitCallbackRegistrar.add("Camera.roll_left", boost::bind(&LLPanelCameraZoom::onRollLeftHeldDown, this));
	mCommitCallbackRegistrar.add("Camera.roll_right", boost::bind(&LLPanelCameraZoom::onRollRightHeldDown, this));
}

BOOL LLPanelCameraZoom::postBuild()
{
	mPlusBtn	= getChild <LLButton> ("zoom_plus_btn");
	mMinusBtn	= getChild <LLButton> ("zoom_minus_btn");
	mSlider		= getChild <LLSlider> ("zoom_slider");
//	//BD - Camera Rolling
	mRollLeft	= getChild <LLButton> ("roll_left");
	mRollRight	= getChild <LLButton> ("roll_right");

	return LLPanel::postBuild();
}

void LLPanelCameraZoom::draw()
{
	mSlider->setValue(gAgentCamera.getCameraZoomFraction());
	LLPanel::draw();
}

void LLPanelCameraZoom::onZoomPlusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val - inc);
	F32 time = mPlusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitInKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onZoomMinusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val + inc);
	F32 time = mMinusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitOutKey(getOrbitRate(time));
}

//BD - Camera Rolling
void LLPanelCameraZoom::onRollLeftHeldDown()
{
	F32 time = mRollLeft->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setRollLeftKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onRollRightHeldDown()
{
	F32 time = mRollRight->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setRollRightKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onCameraTrack()
{
	// EXP-202 when camera panning activated, remove the hint
	LLFirstUse::viewPopup( false );
}

void LLPanelCameraZoom::onCameraRotate()
{
	// EXP-202 when camera rotation activated, remove the hint
	LLFirstUse::viewPopup( false );
}

F32 LLPanelCameraZoom::getOrbitRate(F32 time)
{
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1;
	}
}

void  LLPanelCameraZoom::onSliderValueChanged()
{
	F32 zoom_level = mSlider->getValueF32();
	gAgentCamera.setCameraZoomFraction(zoom_level);
}

void activate_camera_tool()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
};

//
// Member functions
//

/*static*/ bool LLFloaterCamera::inFreeCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
	{
		return true;
	}
	return false;
}

void LLFloaterCamera::resetCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->switchMode(CAMERA_CTRL_MODE_PAN);
}

void LLFloaterCamera::onAvatarEditingAppearance(bool editing)
{
	sAppearanceEditing = editing;
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->handleAvatarEditingAppearance(editing);
}

void LLFloaterCamera::handleAvatarEditingAppearance(bool editing)
{
	//camera presets (rear, front, etc.)
	getChildView("preset_views_list")->setEnabled(!editing);
	getChildView("presets_btn")->setEnabled(!editing);

	//camera modes (object view, mouselook view)
	getChildView("camera_modes_list")->setEnabled(!editing);
	getChildView("avatarview_btn")->setEnabled(!editing);
}

void LLFloaterCamera::update()
{
	ECameraControlMode mode = determineMode();
	if (mode != mCurrMode) setMode(mode);
}


void LLFloaterCamera::toPrevMode()
{
	switchMode(mPrevMode);
}

/*static*/ void LLFloaterCamera::onLeavingMouseLook()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera)
	{
		floater_camera->updateItemsSelection();
		if(floater_camera->inFreeCameraMode())
		{
			activate_camera_tool();
		}
	}
}

LLFloaterCamera* LLFloaterCamera::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
}

void LLFloaterCamera::onOpen(const LLSD& key)
{
	LLFirstUse::viewPopup();

	mZoom->onOpen(key);

	// Returns to previous mode, see EXT-2727(View tool should remember state).
	// In case floater was just hidden and it isn't reset the mode
	// just update state to current one. Else go to previous.
	if ( !mClosed )
		updateState();
	else
		toPrevMode();
	mClosed = FALSE;
}

void LLFloaterCamera::onClose(bool app_quitting)
{
	//We don't care of camera mode if app is quitting
	if(app_quitting)
		return;
	// It is necessary to reset mCurrMode to CAMERA_CTRL_MODE_PAN so 
	// to avoid seeing an empty floater when reopening the control.
	if (mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA)
		mCurrMode = CAMERA_CTRL_MODE_PAN;
	// When mCurrMode is in CAMERA_CTRL_MODE_PAN
	// switchMode won't modify mPrevMode, so force it here.
	// It is needed to correctly return to previous mode on open, see EXT-2727.
	if (mCurrMode == CAMERA_CTRL_MODE_PAN)
		mPrevMode = CAMERA_CTRL_MODE_PAN;

	switchMode(CAMERA_CTRL_MODE_PAN);
	mClosed = TRUE;

	gAgent.setMovementLocked(FALSE);
}

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLFloater(val),
	mClosed(FALSE),
	mCurrMode(CAMERA_CTRL_MODE_PAN),
	mPrevMode(CAMERA_CTRL_MODE_PAN)
{
	LLHints::getInstance()->registerHintTarget("view_popup", getHandle());
	mCommitCallbackRegistrar.add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraItem, _2));
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)

	mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
	mZoom = findChild<LLPanelCameraZoom>(ZOOM);
	mTrack = getChild<LLJoystickCameraTrack>(PAN);
	//BD - Bone Camera
	mJointComboBox = getChild<LLComboBox>("joint_combo");

	assignButton2Mode(CAMERA_CTRL_MODE_MODES,		"avatarview_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_PAN,			"pan_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_PRESETS,		"presets_btn");

	//BD - Unlimited Camera Presets
	mPresetsScroll = this->getChild<LLScrollListCtrl>("presets_scroll", true);
	mPresetsScroll->setCommitCallback(boost::bind(&LLFloaterCamera::switchToPreset, this));

	update();

	// ensure that appearance mode is handled while building. See EXT-7796.
	handleAvatarEditingAppearance(sAppearanceEditing);

	return LLFloater::postBuild();
}

void LLFloaterCamera::fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel)
{
	// copying child list and then iterating over a copy, because list itself
	// is changed in process
	const child_list_t child_list = *panel->getChildList();
	child_list_t::const_reverse_iterator iter = child_list.rbegin();
	child_list_t::const_reverse_iterator end = child_list.rend();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanel* item = dynamic_cast<LLPanel*>(view);
		if (panel)
			list->addItem(item);
	}

}

ECameraControlMode LLFloaterCamera::determineMode()
{
	if (sAppearanceEditing)
	{
		// this is the only enabled camera mode while editing agent appearance.
		return CAMERA_CTRL_MODE_PAN;
	}

	LLTool* curr_tool = LLToolMgr::getInstance()->getCurrentTool();
	if (curr_tool == LLToolCamera::getInstance())
	{
		return CAMERA_CTRL_MODE_FREE_CAMERA;
	} 

	if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
	{
		return CAMERA_CTRL_MODE_PRESETS;
	}

	return CAMERA_CTRL_MODE_PAN;
}


void clear_camera_tool()
{
	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	if (tool_mgr->usingTransientTool() && 
		tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
	{
		tool_mgr->clearTransientTool();
	}
}


void LLFloaterCamera::setMode(ECameraControlMode mode)
{
	if (mode != mCurrMode)
	{
		mPrevMode = mCurrMode;
		mCurrMode = mode;
	}
	
	updateState();
}

void LLFloaterCamera::switchMode(ECameraControlMode mode)
{
	setMode(mode);

	switch (mode)
	{
	case CAMERA_CTRL_MODE_PRESETS:
	case CAMERA_CTRL_MODE_PAN:
		sFreeCamera = false;
		clear_camera_tool();
		break;

	case CAMERA_CTRL_MODE_FREE_CAMERA:
		sFreeCamera = true;
		activate_camera_tool();
		break;

	default:
		//normally we won't occur here
		llassert_always(FALSE);
	}
}


void LLFloaterCamera::onClickBtn(ECameraControlMode mode)
{
	// check for a click on active button
	if (mCurrMode == mode) mMode2Button[mode]->setToggleState(TRUE);
	
	switchMode(mode);

}

void LLFloaterCamera::assignButton2Mode(ECameraControlMode mode, const std::string& button_name)
{
	LLButton* button = getChild<LLButton>(button_name);
	
	button->setClickedCallback(boost::bind(&LLFloaterCamera::onClickBtn, this, mode));
	mMode2Button[mode] = button;
}

void LLFloaterCamera::updateState()
{
	getChildView(ZOOM)->setVisible(CAMERA_CTRL_MODE_PAN == mCurrMode);
	
	bool show_presets = (CAMERA_CTRL_MODE_PRESETS == mCurrMode) || (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode
																	&& CAMERA_CTRL_MODE_PRESETS == mPrevMode);
	getChildView(PRESETS)->setVisible(show_presets);
	
	bool show_camera_modes = CAMERA_CTRL_MODE_MODES == mCurrMode || (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode
																	&& CAMERA_CTRL_MODE_MODES == mPrevMode);
	getChildView("camera_modes_list")->setVisible( show_camera_modes);

	if (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode)
	{
		return;
	}

	//BD - Unlimited Camera Presets
	if (show_presets)
	{
		mPresetsScroll->clearRows();

		//BD - Add all our default presets first.
		std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "camera");
		std::string file;
		LLDirIterator dir_iter_app(dir, "*.xml");
		while (dir_iter_app.next(file))
		{
			std::string path = gDirUtilp->add(dir, file);
			std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

			LLSD preset;
			llifstream infile;
			infile.open(path);
			if (!infile.is_open())
			{
				//BD - If we can't open and read the file we shouldn't add it because we won't
				//     be able to load it later.
				LL_WARNS("Camera") << "Cannot open file in: " << path << LL_ENDL;
				continue;
			}

			//BD - Camera Preset files only have one single line, so it's either a parse failure
			//     or a success.
			S32 ret = LLSDSerialize::fromXML(preset, infile);

			//BD - We couldn't parse the file, don't bother adding it.
			if (ret == LLSDParser::PARSE_FAILURE)
			{
				LL_WARNS("Camera") << "Failed to parse file: " << path << LL_ENDL;
				continue;
			}

			//BD - Skip if this preset is meant to be invisible. This is for RLVa.
			//     Also skip Mouselook preset. We do not set "hidden" to true because
			//     it would hide the Mouselook preset from the preset editor.
			if (preset["hidden"].isDefined() || name == "Mouselook")
			{
				continue;
			}

			LLSD row;
			row["columns"][0]["column"] = "icon";
			row["columns"][0]["type"] = "icon";
			row["columns"][0]["value"] = "Cam_Preset_Side_Off";
			row["columns"][1]["column"] = "preset";
			row["columns"][1]["value"] = name;
			mPresetsScroll->addElement(row);
		}

		bool first_time = true;
		//BD - Add our custom presets at the bottom, just like with windlights.
		dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "camera");
		LLDirIterator dir_iter_user(dir, "*.xml");
		while (dir_iter_user.next(file))
		{
			std::string path = gDirUtilp->add(dir, file);
			std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

			//BD - Don't add if we've already established it as default. It's name is already
			//     in the list.
			if (!mPresetsScroll->findChild<LLScrollListItem>(name))
			{
				if (first_time)
				{
					//BD - Separate defaults from custom ones.
					mPresetsScroll->addSeparator(ADD_BOTTOM);
					first_time = false;
				}

				LLSD row;
				row["columns"][0]["column"] = "icon";
				row["columns"][0]["type"] = "icon";
				row["columns"][0]["value"] = "Cam_Preset_Side_Off";
				row["columns"][1]["column"] = "preset";
				row["columns"][1]["value"] = name;
				mPresetsScroll->addElement(row);
			}
		}
	}

	updateItemsSelection();

	//BD
	if (show_camera_modes)
	{
		mJointComboBox->clear();
		mJointComboBox->add("None", -1);
		for (auto joint : gAgentAvatarp->getSkeleton())
		{
			mJointComboBox->add(joint->getName(), joint->mJointNum);
		}
	}

	//updating buttons
	std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
	for (; iter != mMode2Button.end(); ++iter)
	{
		iter->second->setToggleState(iter->first == mCurrMode);
	}
}

void LLFloaterCamera::updateItemsSelection()
{
	//BD - Unlimited Camera Presets
	if (mPresetsScroll->isInVisibleChain())
	{
		std::string preset_name = gSavedSettings.getString("CameraPresetName");
		for (auto item : mPresetsScroll->getAllData())
		{
			if (item->getColumn(1)->getValue().asString() == preset_name)
			{
				item->setSelected(TRUE);
				break;
			}
		}
	}

	LLSD argument;
	argument["selected"] = gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK;
	getChild<LLPanelCameraItem>("mouselook_view")->setValue(argument);
	argument["selected"] = mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA;
	getChild<LLPanelCameraItem>("object_view")->setValue(argument);
}

void LLFloaterCamera::onClickCameraItem(const LLSD& param)
{
	std::string name = param.asString();

	if ("mouselook_view" == name)
	{
		gAgentCamera.changeCameraToMouselook();
	}
	else if ("object_view" == name)
	{
		LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
		if (camera_floater)
		{
			camera_floater->switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
			camera_floater->updateItemsSelection();
		}
	}
	else
	{
		LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
		if (camera_floater)
		{
			camera_floater->switchMode(CAMERA_CTRL_MODE_PAN);
			camera_floater->switchToPreset();
		}
	}
}

void LLFloaterCamera::switchToPreset()
{
	LLScrollListItem* item = mPresetsScroll->getFirstSelected();
	if (!item) return;

// [RLVa:KB] - @setcam family
	if (RlvActions::isCameraPresetLocked())
	{
		return;
	}
// [/RLVa:KB]

	sFreeCamera = false;
	clear_camera_tool();

	//BD - Unlimited Camera Presets
	std::string name = item->getColumn(1)->getValue();
	gAgentCamera.switchCameraPreset(name);

	LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
	if (camera_floater)
	{
		camera_floater->updateItemsSelection();
		camera_floater->switchMode(CAMERA_CTRL_MODE_PRESETS);
	}
}
