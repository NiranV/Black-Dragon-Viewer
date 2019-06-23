/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterpreference.h"

#include "message.h"
#include "llfloaterautoreplacesettings.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "lldirpicker.h"
#include "lleventtimer.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterabout.h"
#include "llfavoritesbar.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterimsession.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llnavigationbar.h"
#include "llfloaterimnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "llpanellogin.h"
#include "llradiogroup.h"
#include "llsearchcombobox.h"
#include "llsky.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llviewermessage.h"
#include "llviewershadermgr.h"
#include "llviewerthrottle.h"
#include "llvoavatarself.h"
#include "llvotree.h"
#include "llvosky.h"
#include "llfloaterpathfindingconsole.h"
// linden library includes
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llstring.h"

// project includes

#include "llbutton.h"
#include "llflexibleobject.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llwindow.h"
#include "llworld.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "llviewermedia.h"
#include "llpluginclassmedia.h"
#include "llteleporthistorystorage.h"
#include "llproxy.h"
#include "llweb.h"
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a)
#include "rlvactions.h"
#include "rlvhandler.h"
// [/RLVa:KB]

//BD
#include "lldefs.h"
#include "lldiriterator.h"
#include "llviewerkeyboard.h"
#include "llprogressbar.h"
#include "lllogininstance.h"        // to check if logged in yet
#include "llsdserialize.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"
#include "llpresetsmanager.h"
#include "llfeaturemanager.h"
#include "llviewertexturelist.h"
#include "bdsidebar.h"
#include "bdfunctions.h"

//BD - Avatar Rendering Settings
#include "llfloateravatarpicker.h"
#include "llfiltereditor.h"
#include "llnamelistctrl.h"
#include "llmenugl.h"

#include "llsearchableui.h"

const F32 BANDWIDTH_UPDATER_TIMEOUT = 0.5f;
char const* const VISIBILITY_DEFAULT = "default";
char const* const VISIBILITY_HIDDEN = "hidden";

//BD
static LLPanelInjector<LLPanelVoiceDeviceSettings> t_panel_group_general("panel_voice_device_settings");
static const std::string DEFAULT_DEVICE("Default");

LLPanelVoiceDeviceSettings::LLPanelVoiceDeviceSettings()
	: LLPanel()
{
	mCtrlInputDevices = NULL;
	mCtrlOutputDevices = NULL;
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mDevicesUpdated = FALSE;
	mUseTuningMode = true;

	// grab "live" mic volume level
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");

}

LLPanelVoiceDeviceSettings::~LLPanelVoiceDeviceSettings()
{
}

BOOL LLPanelVoiceDeviceSettings::postBuild()
{
	LLSliderCtrl* volume_slider = getChild<LLSliderCtrl>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	volume_slider->setValue(mMicVolume);

	mCtrlInputDevices = getChild<LLComboBox>("voice_input_device");
	mCtrlOutputDevices = getChild<LLComboBox>("voice_output_device");

	mCtrlInputDevices->setCommitCallback(
		boost::bind(&LLPanelVoiceDeviceSettings::onCommitInputDevice, this));
	mCtrlOutputDevices->setCommitCallback(
		boost::bind(&LLPanelVoiceDeviceSettings::onCommitOutputDevice, this));

	mLocalizedDeviceNames[DEFAULT_DEVICE]				= getString("default_text");
	mLocalizedDeviceNames["No Device"]					= getString("name_no_device");
	mLocalizedDeviceNames["Default System Device"]		= getString("name_default_system_device");
	
	return TRUE;
}

// virtual
void LLPanelVoiceDeviceSettings::onVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)
	{
		initialize();	
	}
	else
	{
		cleanup();
	}
}
void LLPanelVoiceDeviceSettings::draw()
{
	refresh();

	// let user know that volume indicator is not yet available
	bool is_in_tuning_mode = LLVoiceClient::getInstance()->inTuningMode();
	getChildView("wait_text")->setVisible( !is_in_tuning_mode && mUseTuningMode);

	LLPanel::draw();

	if (is_in_tuning_mode)
	{
		const S32 num_bars = 5;
		F32 voice_power = LLVoiceClient::getInstance()->tuningGetEnergy() / LLVoiceClient::OVERDRIVEN_POWER_LEVEL;
		S32 discrete_power = llmin(num_bars, llfloor(voice_power * (F32)num_bars + 0.1f));

		for(S32 power_bar_idx = 0; power_bar_idx < num_bars; power_bar_idx++)
		{
			std::string view_name = llformat("%s%d", "bar", power_bar_idx);
			LLView* bar_view = getChild<LLView>(view_name);
			if (bar_view)
			{
				gl_rect_2d(bar_view->getRect(), LLColor4::grey, TRUE);

				LLColor4 color;
				if (power_bar_idx < discrete_power)
				{
					color = (power_bar_idx >= 3) ? LLUIColorTable::instance().getColor("OverdrivenColor") : LLUIColorTable::instance().getColor("SpeakingColor");
				}
				else
				{
					color = LLUIColorTable::instance().getColor("PanelFocusBackgroundColor");
				}

				LLRect color_rect = bar_view->getRect();
				color_rect.stretch(-1);
				gl_rect_2d(color_rect, color, TRUE);
			}
		}
	}
}

void LLPanelVoiceDeviceSettings::apply()
{
	std::string s;
	if(mCtrlInputDevices)
	{
		s = mCtrlInputDevices->getValue().asString();
		gSavedSettings.setString("VoiceInputAudioDevice", s);
		mInputDevice = s;
	}

	if(mCtrlOutputDevices)
	{
		s = mCtrlOutputDevices->getValue().asString();
		gSavedSettings.setString("VoiceOutputAudioDevice", s);
		mOutputDevice = s;
	}

	// assume we are being destroyed by closing our embedding window
	LLSliderCtrl* volume_slider = getChild<LLSliderCtrl>("mic_volume_slider");
	if(volume_slider)
	{
		F32 slider_value = (F32)volume_slider->getValue().asReal();
		gSavedSettings.setF32("AudioLevelMic", slider_value);
		mMicVolume = slider_value;
	}
}

void LLPanelVoiceDeviceSettings::cancel()
{
	gSavedSettings.setString("VoiceInputAudioDevice", mInputDevice);
	gSavedSettings.setString("VoiceOutputAudioDevice", mOutputDevice);

	if(mCtrlInputDevices)
		mCtrlInputDevices->setValue(mInputDevice);

	if(mCtrlOutputDevices)
		mCtrlOutputDevices->setValue(mOutputDevice);

	gSavedSettings.setF32("AudioLevelMic", mMicVolume);
	LLSliderCtrl* volume_slider = getChild<LLSliderCtrl>("mic_volume_slider");
	if(volume_slider)
	{
		volume_slider->setValue(mMicVolume);
	}
}

void LLPanelVoiceDeviceSettings::refresh()
{
	//grab current volume
	LLSlider* volume_slider = getChild<LLSlider>("mic_volume_slider");
	// set mic volume tuning slider based on last mic volume setting
	F32 current_volume = (F32)volume_slider->getValue().asReal();
	LLVoiceClient::getInstance()->tuningSetMicVolume(current_volume);

	// Fill in popup menus
	bool device_settings_available = LLVoiceClient::getInstance()->deviceSettingsAvailable();

	if (mCtrlInputDevices)
	{
		mCtrlInputDevices->setEnabled(device_settings_available);
	}

	if (mCtrlOutputDevices)
	{
		mCtrlOutputDevices->setEnabled(device_settings_available);
	}

	getChild<LLSlider>("mic_volume_slider")->setEnabled(device_settings_available);

	if (!device_settings_available)
	{
		// The combo boxes are disabled, since we can't get the device settings from the daemon just now.
		// Put the currently set default (ONLY) in the box, and select it.
		if (mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(mInputDevice), mInputDevice, ADD_BOTTOM);
			mCtrlInputDevices->setValue(mInputDevice);
		}
		if (mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(mOutputDevice), mOutputDevice, ADD_BOTTOM);
			mCtrlOutputDevices->setValue(mOutputDevice);
		}
	}
	else if (LLVoiceClient::getInstance()->deviceSettingsUpdated())
	{
		LLVoiceDeviceList::const_iterator device;

		if (mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for (device = LLVoiceClient::getInstance()->getCaptureDevices().begin();
				device != LLVoiceClient::getInstance()->getCaptureDevices().end();
				device++)
			{
				mCtrlInputDevices->add(getLocalizedDeviceName(device->display_name), device->full_name, ADD_BOTTOM);
			}

			// Fix invalid input audio device preference.
			if (!mCtrlInputDevices->setSelectedByValue(mInputDevice, TRUE))
			{
				mCtrlInputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceInputAudioDevice", DEFAULT_DEVICE);
				mInputDevice = DEFAULT_DEVICE;
			}
		}

		if (mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for (device = LLVoiceClient::getInstance()->getRenderDevices().begin();
				device != LLVoiceClient::getInstance()->getRenderDevices().end();
				device++)
			{
				mCtrlOutputDevices->add(getLocalizedDeviceName(device->display_name), device->full_name, ADD_BOTTOM);
			}

			// Fix invalid output audio device preference.
			if (!mCtrlOutputDevices->setSelectedByValue(mOutputDevice, TRUE))
			{
				mCtrlOutputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceOutputAudioDevice", DEFAULT_DEVICE);
				mOutputDevice = DEFAULT_DEVICE;
			}
		}
	}
}

void LLPanelVoiceDeviceSettings::initialize()
{
	mInputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	mOutputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	mMicVolume = gSavedSettings.getF32("AudioLevelMic");
	mDevicesUpdated = FALSE;

	// ask for new device enumeration
	LLVoiceClient::getInstance()->refreshDeviceLists();

	// put voice client in "tuning" mode
	if (mUseTuningMode)
	{
		LLVoiceClient::getInstance()->tuningStart();
		LLVoiceChannel::suspend();
	}
}

void LLPanelVoiceDeviceSettings::cleanup()
{
	if (mUseTuningMode)
	{
		LLVoiceClient::getInstance()->tuningStop();
		LLVoiceChannel::resume();
	}
}

// returns English name if no translation found
std::string LLPanelVoiceDeviceSettings::getLocalizedDeviceName(const std::string& en_dev_name)
{
	std::map<std::string, std::string>::const_iterator it = mLocalizedDeviceNames.find(en_dev_name);
	return it != mLocalizedDeviceNames.end() ? it->second : en_dev_name;
}

void LLPanelVoiceDeviceSettings::onCommitInputDevice()
{
	if(LLVoiceClient::getInstance())
	{
		LLVoiceClient::getInstance()->setCaptureDevice(
			mCtrlInputDevices->getValue().asString());
	}
}

void LLPanelVoiceDeviceSettings::onCommitOutputDevice()
{
	if(LLVoiceClient::getInstance())
	{
		LLVoiceClient::getInstance()->setRenderDevice(
			mCtrlInputDevices->getValue().asString());
	}
}

//control value for middle mouse as talk2push button
const static std::string MIDDLE_MOUSE_CV = "MiddleMouse";

struct LabelDef : public LLInitParam::Block<LabelDef>
{
    Mandatory<std::string> name;
    Mandatory<std::string> value;

    LabelDef()
        : name("name"),
        value("value")
    {}
};

struct LabelTable : public LLInitParam::Block<LabelTable>
{
    Multiple<LabelDef> labels;
    LabelTable()
        : labels("label")
    {}
};

class LLVoiceSetKeyDialog : public LLModalDialog
{
public:
	LLVoiceSetKeyDialog(const LLSD& key);
	~LLVoiceSetKeyDialog();
	
	/*virtual*/ BOOL postBuild();
	
	void setParent(LLFloaterPreference* parent) { mParent = parent; }
	
	BOOL handleKeyHere(KEY key, MASK mask);
	static void onCancel(void* user_data);
		
private:
	LLFloaterPreference* mParent;
};

LLVoiceSetKeyDialog::LLVoiceSetKeyDialog(const LLSD& key)
  : LLModalDialog(key),
	mParent(NULL)
{
}

//virtual
BOOL LLVoiceSetKeyDialog::postBuild()
{
	childSetAction("Cancel", onCancel, this);
	getChild<LLUICtrl>("Cancel")->setFocus(TRUE);
	
	gFocusMgr.setKeystrokesOnly(TRUE);
	
	return TRUE;
}

LLVoiceSetKeyDialog::~LLVoiceSetKeyDialog()
{
}

BOOL LLVoiceSetKeyDialog::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = TRUE;
	
	if (key == 'Q' && mask == MASK_CONTROL)
	{
		result = FALSE;
	}
	else if (mParent)
	{
		mParent->setKey(key);
	}
	closeFloater();
	return result;
}

//static
void LLVoiceSetKeyDialog::onCancel(void* user_data)
{
	LLVoiceSetKeyDialog* self = (LLVoiceSetKeyDialog*)user_data;
	self->closeFloater();
}


//BD - Custom Keyboard Layout
class LLSetKeyDialog : public LLModalDialog
{
public:
	LLSetKeyDialog(const LLSD& key);
	~LLSetKeyDialog();

	/*virtual*/ BOOL postBuild();

	void setParent(LLFloaterPreference* parent) { mParent = parent; }
	void setMode(S32 mode) { mMode = mode; }

	BOOL handleKeyHere(KEY key, MASK mask);
	void onCancel();
	void onBind();
	void onActionCommit(LLUICtrl* ctrl, const LLSD& param);
	void onMasks();
	void onOpen(const LLSD& key);

	S32 mMode;
	MASK mMask;
	KEY mKey;
	LLSD mAction;
private:
	LLFloaterPreference* mParent;
};

LLSetKeyDialog::LLSetKeyDialog(const LLSD& key)
	: LLModalDialog(key),
	mParent(NULL),
	mMode(NULL),
	mMask(MASK_NONE),
	mKey(NULL),
	mAction()
{
	mCommitCallbackRegistrar.add("Set.Masks", boost::bind(&LLSetKeyDialog::onMasks, this));
	mCommitCallbackRegistrar.add("Set.Bind", boost::bind(&LLSetKeyDialog::onBind, this));
	mCommitCallbackRegistrar.add("Set.Action", boost::bind(&LLSetKeyDialog::onActionCommit, this, _1, _2));
	mCommitCallbackRegistrar.add("Set.Cancel", boost::bind(&LLSetKeyDialog::onCancel, this));
}

//virtual
BOOL LLSetKeyDialog::postBuild()
{
	//BD - We block off keypresses like space and enter with this so it doesn't
	//     accidentally press cancel or bind but is still handled by the floater.
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);

	return TRUE;
}

LLSetKeyDialog::~LLSetKeyDialog()
{
}

void LLSetKeyDialog::onOpen(const LLSD& key)
{
	LLUICtrl* ctrl = getChild<LLUICtrl>("key_display");

	ctrl->setTextArg("[KEY]", gKeyboard->stringFromKey(mKey));
	ctrl->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));

	//BD - Show the correct bind action dropdown for the correct mode since each has
	//     different actions depending on the mode we are binding for.
	getChild<LLComboBox>(llformat("bind_action_%i", mMode))->setVisible(true);

	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

BOOL LLSetKeyDialog::handleKeyHere(KEY key, MASK mask)
{
	mKey = key;

	//BD - Ensure we have at least MASK_NONE set, always, otherwise set our typed mask.
	if (!mMask)
	{
		mMask = MASK_NONE;
	}
	else if (mask != MASK_NONE && mask != NULL)
	{
		mMask = mask;
	}

	LLUICtrl* ctrl = getChild<LLUICtrl>("key_display");
	ctrl->setTextArg("[KEY]", gKeyboard->stringFromKey(mKey));
	ctrl->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));
	LL_INFOS() << "Pressed: " << key << +"(" + gKeyboard->stringFromKey(key) << ") + "
				<< mask << +"(" << gKeyboard->stringFromMask(mask) << LL_ENDL;

	return TRUE;
}

void LLSetKeyDialog::onActionCommit(LLUICtrl* ctrl, const LLSD& param)
{
	mAction = ctrl->getValue();

	//BD - After selecting an action return the focus back to the focus catcher so
	//     we can catch keypresses again.
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

void LLSetKeyDialog::onMasks()
{
	mMask = MASK_NONE;

	if (getChild<LLUICtrl>("CTRL")->getValue())
	{
		mMask += MASK_CONTROL;
	}

	if (getChild<LLUICtrl>("SHIFT")->getValue())
	{
		mMask += MASK_SHIFT;
	}

	if (getChild<LLUICtrl>("ALT")->getValue())
	{
		mMask += MASK_ALT;
	}

	//BD - We block off keypresses like space and enter with this so it doesn't
	//     accidentally press cancel or bind but is still handled by the floater.
	getChild<LLUICtrl>("key_display")->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));

	//BD - After clicking any mask return the focus back to the focus catcher so
	//     we can catch keypresses again.
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

void LLSetKeyDialog::onCancel()
{
	this->closeFloater();
}

void LLSetKeyDialog::onBind()
{
	if (mParent && mKey != NULL && !mAction == NULL)
	{
		mParent->onAddBind(mKey, mMask, mAction);
		this->closeFloater();
	}
	else
	{
		//BD - We failed to bind, return focus to the focus button to receive inputs.
		getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
		gFocusMgr.setKeystrokesOnly(TRUE);
	}
}


//BD - Custom Keyboard Layout
class LLChangeKeyDialog : public LLModalDialog
{
public:
	LLChangeKeyDialog(const LLSD& key);
	~LLChangeKeyDialog();

	/*virtual*/ BOOL postBuild();

	void setParent(LLFloaterPreference* parent) { mParent = parent; }
	void setMode(S32 mode) { mMode = mode; }
	void setKey(KEY key) { mKey = key; }
	void setMask(MASK mask) { mMask = mask;	}

	BOOL handleKeyHere(KEY key, MASK mask);
	void onCancel();
	void onBind();
	void onMasks();
	void onOpen(const LLSD& key);

	S32 mMode;
	MASK mMask;
	KEY mKey;
private:
	LLFloaterPreference* mParent;
};

LLChangeKeyDialog::LLChangeKeyDialog(const LLSD& key)
	: LLModalDialog(key),
	mParent(NULL),
	mMode(NULL),
	mMask(MASK_NONE),
	mKey(NULL)
{
	mCommitCallbackRegistrar.add("Set.Masks", boost::bind(&LLChangeKeyDialog::onMasks, this));
	mCommitCallbackRegistrar.add("Set.Bind", boost::bind(&LLChangeKeyDialog::onBind, this));
	mCommitCallbackRegistrar.add("Set.Cancel", boost::bind(&LLChangeKeyDialog::onCancel, this));
}

//virtual
BOOL LLChangeKeyDialog::postBuild()
{
	//BD - We block off keypresses like space and enter with this so it doesn't
	//     accidentally press cancel or bind but is still handled by the floater.
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);

	return TRUE;
}

LLChangeKeyDialog::~LLChangeKeyDialog()
{
}

void LLChangeKeyDialog::onOpen(const LLSD& key)
{
	LLUICtrl* old_ctrl = getChild<LLUICtrl>("old_key_display");
	LLUICtrl* ctrl = getChild<LLUICtrl>("key_display");

	old_ctrl->setTextArg("[KEY]", gKeyboard->stringFromKey(mKey));
	old_ctrl->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));

	mKey = NULL;
	mMask = MASK_NONE;

	ctrl->setTextArg("[KEY]", gKeyboard->stringFromKey(mKey));
	ctrl->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));

	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

BOOL LLChangeKeyDialog::handleKeyHere(KEY key, MASK mask)
{
	mKey = key;

	//BD - Ensure we have at least MASK_NONE set, always, otherwise set our typed mask.
	if (!mMask)
	{
		mMask = MASK_NONE;
	}
	else if (mask != MASK_NONE && mask != NULL)
	{
		mMask = mask;
	}

	LLUICtrl* ctrl = getChild<LLUICtrl>("key_display");
	ctrl->setTextArg("[KEY]", gKeyboard->stringFromKey(mKey));
	ctrl->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));
	LL_INFOS() << "Pressed: " << key << +"(" + gKeyboard->stringFromKey(key) << ") + "
		<< mask << +"(" << gKeyboard->stringFromMask(mask) << LL_ENDL;

	return TRUE;
}

void LLChangeKeyDialog::onMasks()
{
	mMask = MASK_NONE;

	if (getChild<LLUICtrl>("CTRL")->getValue())
	{
		mMask += MASK_CONTROL;
	}

	if (getChild<LLUICtrl>("SHIFT")->getValue())
	{
		mMask += MASK_SHIFT;
	}

	if (getChild<LLUICtrl>("ALT")->getValue())
	{
		mMask += MASK_ALT;
	}

	//BD - We block off keypresses like space and enter with this so it doesn't
	//     accidentally press cancel or bind but is still handled by the floater.
	getChild<LLUICtrl>("key_display")->setTextArg("[MASK]", gKeyboard->stringFromMask(mMask));

	//BD - After clicking any mask return the focus back to the focus catcher so
	//     we can catch keypresses again.
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

void LLChangeKeyDialog::onCancel()
{
	this->closeFloater();
}

void LLChangeKeyDialog::onBind()
{
	if (mParent && mKey != NULL)
	{
		mParent->onReplaceBind(mKey, mMask);
	}
	this->closeFloater();
}



/// This must equal the maximum value set for the IndirectMaxComplexity slider in panel_preferences_graphics1.xml
static const U32 INDIRECT_MAX_ARC_OFF = 101; // all the way to the right == disabled
static const U32 MIN_INDIRECT_ARC_LIMIT = 1; // must match minimum of IndirectMaxComplexity in panel_preferences_graphics1.xml
static const U32 MAX_INDIRECT_ARC_LIMIT = INDIRECT_MAX_ARC_OFF - 1; // one short of all the way to the right...

/// These are the effective range of values for RenderAvatarMaxComplexity
static const F32 MIN_ARC_LIMIT = 20000.0f;
static const F32 MAX_ARC_LIMIT = 300000.0f;
static const F32 MIN_ARC_LOG = log(MIN_ARC_LIMIT);
static const F32 MAX_ARC_LOG = log(MAX_ARC_LIMIT);
static const F32 ARC_LIMIT_MAP_SCALE = (MAX_ARC_LOG - MIN_ARC_LOG) / (MAX_INDIRECT_ARC_LIMIT - MIN_INDIRECT_ARC_LIMIT);

// global functions 

// helper functions for getting/freeing the web browser media
// if creating/destroying these is too slow, we'll need to create
// a static member and update all our static callbacks

void handleNameTagOptionChanged(const LLSD& newvalue);	
void handleDisplayNamesOptionChanged(const LLSD& newvalue);	
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);
bool callback_clear_cache(const LLSD& notification, const LLSD& response);

//bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
//bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

bool callback_clear_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		// flag client texture cache for clearing next time the client runs
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
		LLNotificationsUtil::add("CacheWillClear");
	}

	return false;
}

bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		// clean web
		LLViewerMedia::clearAllCaches();
		LLViewerMedia::clearAllCookies();
		
		// clean nav bar history
		LLNavigationBar::getInstance()->clearHistoryCache();
		
		// flag client texture cache for clearing next time the client runs
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
		LLNotificationsUtil::add("CacheWillClear");

		LLSearchHistory::getInstance()->clearHistory();
		LLSearchHistory::getInstance()->save();
		LLSearchComboBox* search_ctrl = LLNavigationBar::getInstance()->getChild<LLSearchComboBox>("search_combo_box");
		search_ctrl->clearHistory();

		LLTeleportHistoryStorage::getInstance()->purgeItems();
		LLTeleportHistoryStorage::getInstance()->save();
	}
	
	return false;
}

void handleNameTagOptionChanged(const LLSD& newvalue)
{
	LLAvatarNameCache::setUseUsernames(gSavedSettings.getBOOL("NameTagShowUsernames"));
	LLVOAvatar::invalidateNameTags();
}

void handleDisplayNamesOptionChanged(const LLSD& newvalue)
{
	LLAvatarNameCache::setUseDisplayNames(newvalue.asBoolean());
	LLVOAvatar::invalidateNameTags();
}

void handleAppearanceCameraMovementChanged(const LLSD& newvalue)
{
	if(!newvalue.asBoolean() && gAgentCamera.getCameraMode() == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		gAgentCamera.changeCameraToDefault();
		gAgentCamera.resetView();
	}
}

/*bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option && floater )
	{
		if ( floater )
		{
			floater->setAllIgnored();
		//	LLFirstUse::disableFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}

bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( 0 == option && floater )
	{
		if ( floater )
		{
			floater->resetAllIgnored();
			//LLFirstUse::resetFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}
*/

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator)
{
	numerator = 0;
	denominator = 0;
	for (F32 test_denominator = 1.f; test_denominator < 30.f; test_denominator += 1.f)
	{
		if (fmodf((decimal_val * test_denominator) + 0.01f, 1.f) < 0.02f)
		{
			numerator = ll_round(decimal_val * test_denominator);
			denominator = ll_round(test_denominator);
			break;
		}
	}
}
// static
std::string LLFloaterPreference::sSkin = "";
//////////////////////////////////////////////
// LLFloaterPreference

class LLSettingsContextMenu : public LLListContextMenu
{
public:
	LLSettingsContextMenu(LLFloaterPreference* floater_settings)
		: mFloaterSettings(floater_settings)
	{}
protected:
	LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		registrar.add("Settings.SetRendering", boost::bind(&LLFloaterPreference::onCustomAction, mFloaterSettings, _2));
		enable_registrar.add("Settings.IsSelected", boost::bind(&LLFloaterPreference::isActionChecked, mFloaterSettings, _2, mUUIDs.front()));
		LLContextMenu* menu = createFromFile("menu_avatar_rendering_settings.xml");

		return menu;
	}

	LLFloaterPreference* mFloaterSettings;
};

class LLAvatarRenderMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { LLFloaterPreference::setNeedsUpdate(); }
};

static LLAvatarRenderMuteListObserver sAvatarRenderMuteListObserver;

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false),
	mLanguageChanged(false),
	mAvatarDataInitialized(false),
	mClickActionDirty(false),
//	//BD - Avatar Rendering Settings
	mAvatarSettingsList(NULL),
	mNeedsUpdate(false)
{
	//BD
	static bool registered_voice_dialog = false;
	if (!registered_voice_dialog)
	{
		LLFloaterReg::add("voice_set_key", "floater_select_key_voice.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLVoiceSetKeyDialog>);
		registered_voice_dialog = true;
	}

//	//BD - Custom Keyboard Layout
	static bool registered_dialog = false;
	if (!registered_dialog)
	{
		LLFloaterReg::add("set_any_key", "floater_select_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLSetKeyDialog>);
		registered_dialog = true;
	}

	static bool registered_change_dialog = false;
	if (!registered_change_dialog)
	{
		LLFloaterReg::add("change_key", "floater_change_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLChangeKeyDialog>);
		registered_change_dialog = true;
	}
	
	mCommitCallbackRegistrar.add("Pref.Cancel",					boost::bind(&LLFloaterPreference::onBtnCancel, this));
	mCommitCallbackRegistrar.add("Pref.OK",						boost::bind(&LLFloaterPreference::onBtnOK, this));
	
	mCommitCallbackRegistrar.add("Pref.ClearCache",				boost::bind(&LLFloaterPreference::onClickClearCache, this));
	mCommitCallbackRegistrar.add("Pref.WebClearCache",			boost::bind(&LLFloaterPreference::onClickBrowserClearCache, this));
	mCommitCallbackRegistrar.add("Pref.SetCache",				boost::bind(&LLFloaterPreference::onClickSetCache, this));
	mCommitCallbackRegistrar.add("Pref.ResetCache",				boost::bind(&LLFloaterPreference::onClickResetCache, this));
	mCommitCallbackRegistrar.add("Pref.ClickSkin",				boost::bind(&LLFloaterPreference::onClickSkin, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.SelectSkin",				boost::bind(&LLFloaterPreference::onSelectSkin, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetKey",			boost::bind(&LLFloaterPreference::onClickSetKey, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetMiddleMouse",	boost::bind(&LLFloaterPreference::onClickSetMiddleMouse, this));
	mCommitCallbackRegistrar.add("Pref.SetSounds",				boost::bind(&LLFloaterPreference::onClickSetSounds, this));
	mCommitCallbackRegistrar.add("Pref.ClickEnablePopup",		boost::bind(&LLFloaterPreference::onClickEnablePopup, this));
	mCommitCallbackRegistrar.add("Pref.ClickDisablePopup",		boost::bind(&LLFloaterPreference::onClickDisablePopup, this));	
	mCommitCallbackRegistrar.add("Pref.LogPath",				boost::bind(&LLFloaterPreference::onClickLogPath, this));
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",		boost::bind(&LLFloaterPreference::setHardwareDefaults, this));
	mCommitCallbackRegistrar.add("Pref.AvatarImpostorsEnable",	boost::bind(&LLFloaterPreference::onAvatarImpostorsEnable, this));
	mCommitCallbackRegistrar.add("Pref.UpdateSliderText",		boost::bind(&LLFloaterPreference::refreshUI,this));
	mCommitCallbackRegistrar.add("Pref.applyUIColor",			boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.getUIColor",				boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.BlockList",				boost::bind(&LLFloaterPreference::onClickBlockList, this));
	mCommitCallbackRegistrar.add("Pref.Proxy",					boost::bind(&LLFloaterPreference::onClickProxySettings, this));
	mCommitCallbackRegistrar.add("Pref.TranslationSettings",	boost::bind(&LLFloaterPreference::onClickTranslationSettings, this));
	mCommitCallbackRegistrar.add("Pref.AutoReplace",            boost::bind(&LLFloaterPreference::onClickAutoReplace, this));
	mCommitCallbackRegistrar.add("Pref.PermsDefault",           boost::bind(&LLFloaterPreference::onClickPermsDefault, this));
	mCommitCallbackRegistrar.add("Pref.SpellChecker",           boost::bind(&LLFloaterPreference::onClickSpellChecker, this));
	mCommitCallbackRegistrar.add("Pref.ClearLog",				boost::bind(&LLConversationLog::onClearLog, &LLConversationLog::instance()));
	mCommitCallbackRegistrar.add("Pref.DeleteTranscripts",      boost::bind(&LLFloaterPreference::onDeleteTranscripts, this));

//	//BD - Memory Allocation
	mCommitCallbackRegistrar.add("Pref.RefreshMemoryControls",	boost::bind(&LLFloaterPreference::refreshMemoryControls, this));

//	//BD - Unlimited Camera Presets
	mCommitCallbackRegistrar.add("Pref.AddCameraPreset", boost::bind(&LLFloaterPreference::onAddCameraPreset, this, true));
	mCommitCallbackRegistrar.add("Pref.RemoveCameraPreset", boost::bind(&LLFloaterPreference::onRemoveCameraPreset, this));
	mCommitCallbackRegistrar.add("Pref.ChangeCameraPreset", boost::bind(&LLFloaterPreference::onChangeCameraPreset, this));
	mCommitCallbackRegistrar.add("Pref.ResetCameraPreset", boost::bind(&LLFloaterPreference::onCameraPresetReset, this, _2));
	mCommitCallbackRegistrar.add("Pref.CameraArray", boost::bind(&LLFloaterPreference::onCameraArray, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.FocusArray", boost::bind(&LLFloaterPreference::onFocusArray, this, _1, _2));

//	//BD - Custom Keyboard Layout
	mCommitCallbackRegistrar.add("Pref.ExportControls", boost::bind(&LLFloaterPreference::onExportControls, this));
	mCommitCallbackRegistrar.add("Pref.UnbindAll", boost::bind(&LLFloaterPreference::onUnbindControls, this));
	mCommitCallbackRegistrar.add("Pref.DefaultControls", boost::bind(&LLFloaterPreference::onDefaultControls, this));
	mCommitCallbackRegistrar.add("Pref.AddBind", boost::bind(&LLFloaterPreference::onClickSetAnyKey, this));
	mCommitCallbackRegistrar.add("Pref.RemoveBind", boost::bind(&LLFloaterPreference::onRemoveBind, this));
	mCommitCallbackRegistrar.add("Pref.ChangeBind", boost::bind(&LLFloaterPreference::onListClickAction, this));
	mCommitCallbackRegistrar.add("Pref.ChangeMode", boost::bind(&LLFloaterPreference::refreshKeys, this));

//	//BD - Expandable Tabs
	mCommitCallbackRegistrar.add("Pref.Tab", boost::bind(&LLFloaterPreference::toggleTabs, this));

//	//BD - Revert to Default
	mCommitCallbackRegistrar.add("Pref.Default", boost::bind(&LLFloaterPreference::resetToDefault, this, _1));

//	//BD - Input/Output resizer
	mCommitCallbackRegistrar.add("Pref.InputOutput", boost::bind(&LLFloaterPreference::inputOutput, this));

//	//BD - Quick Graphics Presets
	mCommitCallbackRegistrar.add("Pref.PrefDelete", boost::bind(&LLFloaterPreference::deletePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefSave", boost::bind(&LLFloaterPreference::savePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefLoad", boost::bind(&LLFloaterPreference::loadPreset, this, _2));
	LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterPreference::onPresetsListChange, this));

//	//BD - Avatar Rendering Settings
	mContextMenu = new LLSettingsContextMenu(this);
	mCommitCallbackRegistrar.add("Settings.AddNewEntry", boost::bind(&LLFloaterPreference::onClickAdd, this, _2));
	mCommitCallbackRegistrar.add("Settings.SetRendering", boost::bind(&LLFloaterPreference::onCustomAction, this, _2));
	LLRenderMuteList::getInstance()->addObserver(&sAvatarRenderMuteListObserver);

	//BD
	mCommitCallbackRegistrar.add("Pref.VoiceSetNone", boost::bind(&LLFloaterPreference::onClickSetNone, this));

	//BD
	LLConversationLog::instance().addObserver(this);
	LLAvatarPropertiesProcessor::getInstance()->addObserver(gAgent.getID(), this);

	sSkin = gSavedSettings.getString("SkinCurrent");

	gSavedSettings.getControl("NameTagShowUsernames")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged, _2));
	gSavedSettings.getControl("NameTagShowFriends")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged, _2));
	gSavedSettings.getControl("UseDisplayNames")->getCommitSignal()->connect(boost::bind(&handleDisplayNamesOptionChanged, _2));

	gSavedSettings.getControl("AppearanceCameraMovement")->getCommitSignal()->connect(boost::bind(&handleAppearanceCameraMovementChanged, _2));
	mCommitCallbackRegistrar.add("UpdateFilter", boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false)); // <FS:ND/> Hook up for filtering
}

void LLFloaterPreference::processProperties( void* pData, EAvatarProcessorType type )
{
	if ( APT_PROPERTIES == type )
	{
		const LLAvatarData* pAvatarData = static_cast<const LLAvatarData*>( pData );
		if (pAvatarData && (gAgent.getID() == pAvatarData->avatar_id) && (pAvatarData->avatar_id != LLUUID::null))
		{
			storeAvatarProperties( pAvatarData );
			processProfileProperties( pAvatarData );
		}
	}	
}

void LLFloaterPreference::storeAvatarProperties( const LLAvatarData* pAvatarData )
{
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		mAvatarProperties.avatar_id		= pAvatarData->avatar_id;
		mAvatarProperties.image_id		= pAvatarData->image_id;
		mAvatarProperties.fl_image_id   = pAvatarData->fl_image_id;
		mAvatarProperties.about_text	= pAvatarData->about_text;
		mAvatarProperties.fl_about_text = pAvatarData->fl_about_text;
		mAvatarProperties.profile_url   = pAvatarData->profile_url;
		mAvatarProperties.flags		    = pAvatarData->flags;
		mAvatarProperties.allow_publish	= pAvatarData->flags & AVATAR_ALLOW_PUBLISH;

		mAvatarDataInitialized = true;
	}
}

void LLFloaterPreference::processProfileProperties(const LLAvatarData* pAvatarData )
{
	getChild<LLUICtrl>("online_searchresults")->setValue( (bool)(pAvatarData->flags & AVATAR_ALLOW_PUBLISH) );	
}

void LLFloaterPreference::saveAvatarProperties( void )
{
	const BOOL allowPublish = getChild<LLUICtrl>("online_searchresults")->getValue();

	if (allowPublish)
	{
		mAvatarProperties.flags |= AVATAR_ALLOW_PUBLISH;
	}

	//
	// NOTE: We really don't want to send the avatar properties unless we absolutely
	//       need to so we can avoid the accidental profile reset bug, so, if we're
	//       logged in, the avatar data has been initialized and we have a state change
	//       for the "allow publish" flag, then set the flag to its new value and send
	//       the properties update.
	//
	// NOTE: The only reason we can not remove this update altogether is because of the
	//       "allow publish" flag, the last remaining profile setting in the viewer
	//       that doesn't exist in the web profile.
	//
	if ((LLStartUp::getStartupState() == STATE_STARTED) && mAvatarDataInitialized && (allowPublish != mAvatarProperties.allow_publish))
	{
		mAvatarProperties.allow_publish = allowPublish;

		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate( &mAvatarProperties );
	}
}

BOOL LLFloaterPreference::postBuild()
{
	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLFloaterIMSessionTab::processChatHistoryStyleUpdate, false));

	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLViewerChat::signalChatFontChanged));

	gSavedSettings.getControl("ChatBubbleOpacity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onNameTagOpacityChange, this, _2));

	gSavedPerAccountSettings.getControl("ModelUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeModelFolder, this));
	gSavedPerAccountSettings.getControl("TextureUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeTextureFolder, this));
	gSavedPerAccountSettings.getControl("SoundUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeSoundFolder, this));
	gSavedPerAccountSettings.getControl("AnimationUploadFolder")->getSignal()->connect(boost::bind(&LLFloaterPreference::onChangeAnimationFolder, this));

	getChild<LLUICtrl>("cache_location")->setEnabled(FALSE); // make it read-only but selectable (STORM-227)
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);

	getChild<LLUICtrl>("log_path_string")->setEnabled(FALSE); // make it read-only but selectable

	getChild<LLUICtrl>("language_radio")->setCommitCallback(boost::bind(&LLFloaterPreference::onLanguageChange, this));

	getChild<LLUICtrl>("FriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"FriendIMOptions"));
	getChild<LLUICtrl>("NonFriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this, "NonFriendIMOptions"));
	getChild<LLUICtrl>("ConferenceIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this, "ConferenceIMOptions"));
	getChild<LLUICtrl>("GroupChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this, "GroupChatOptions"));
	getChild<LLUICtrl>("NearbyChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this, "NearbyChatOptions"));
	getChild<LLUICtrl>("ObjectIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this, "ObjectIMOptions"));

	//BD
	mTabContainer = getChild<LLTabContainer>("pref core");
	if (!mTabContainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
	{
		mTabContainer->selectFirstTab();
	}

	mGFXStack = getChild<LLLayoutStack>("gfx_stack");

	mRenderGlowLumWeights = { { getChild<LLUICtrl>("RenderGlowLumWeights_X"),
							getChild<LLUICtrl>("RenderGlowLumWeights_Y"),
							getChild<LLUICtrl>("RenderGlowLumWeights_Z") } };
	mRenderGlowWarmthWeights = { { getChild<LLUICtrl>("RenderGlowWarmthWeights_X"),
							getChild<LLUICtrl>("RenderGlowWarmthWeights_Y"),
							getChild<LLUICtrl>("RenderGlowWarmthWeights_Z") } };
	mExodusRenderToneAdvOptA = { { getChild<LLUICtrl>("ExodusRenderToneAdvOptA_X"),
							getChild<LLUICtrl>("ExodusRenderToneAdvOptA_Y"),
							getChild<LLUICtrl>("ExodusRenderToneAdvOptA_Z") } };
	mExodusRenderToneAdvOptB = { { getChild<LLUICtrl>("ExodusRenderToneAdvOptB_X"),
							getChild<LLUICtrl>("ExodusRenderToneAdvOptB_Y"),
							getChild<LLUICtrl>("ExodusRenderToneAdvOptB_Z") } };
	mExodusRenderGamma = { { getChild<LLUICtrl>("ExodusRenderGamma_X"),
							getChild<LLUICtrl>("ExodusRenderGamma_Y"),
							getChild<LLUICtrl>("ExodusRenderGamma_Z") } };
	mExodusRenderExposure = { { getChild<LLUICtrl>("ExodusRenderExposure_X"),
							getChild<LLUICtrl>("ExodusRenderExposure_Y"),
							getChild<LLUICtrl>("ExodusRenderExposure_Z") } };
	mExodusRenderOffset = { { getChild<LLUICtrl>("ExodusRenderOffset_X"),
							getChild<LLUICtrl>("ExodusRenderOffset_Y"),
							getChild<LLUICtrl>("ExodusRenderOffset_Z") } };
	mExodusRenderVignette = { { getChild<LLUICtrl>("ExodusRenderVignette_X"),
							getChild<LLUICtrl>("ExodusRenderVignette_Y"),
							getChild<LLUICtrl>("ExodusRenderVignette_Z") } };

	mRenderShadowDistance = { { getChild<LLUICtrl>("RenderShadowDistance_X"),
								getChild<LLUICtrl>("RenderShadowDistance_Y"),
								getChild<LLUICtrl>("RenderShadowDistance_Z"),
								getChild<LLUICtrl>("RenderShadowDistance_W") } };
	mRenderShadowResolution = { { getChild<LLUICtrl>("RenderShadowResolution_X"),
								getChild<LLUICtrl>("RenderShadowResolution_Y"),
								getChild<LLUICtrl>("RenderShadowResolution_Z"),
								getChild<LLUICtrl>("RenderShadowResolution_W") } };

	mRenderProjectorShadowResolution = { { getChild<LLUICtrl>("RenderProjectorShadowResolution_X"),
										getChild<LLUICtrl>("RenderProjectorShadowResolution_Y") } };
	mExodusRenderToneAdvOptC = { { getChild<LLUICtrl>("ExodusRenderToneAdvOptC_X"),
										getChild<LLUICtrl>("ExodusRenderToneAdvOptC_Y") } };

	mExodusRenderToneMappingTech = getChild<LLUICtrl>("ExodusRenderToneMappingTech");
	mExodusRenderToneExposure = getChild<LLUICtrl>("ExodusRenderToneExposure");
	mExodusRenderColorGradeTech = getChild<LLUICtrl>("ExodusRenderColorGradeTech");

	mRenderSpotLightReflections = getChild<LLUICtrl>("RenderSpotLightReflections");
	mRenderSpotLightImages = getChild<LLUICtrl>("RenderSpotLightImages");
	mRenderShadowAutomaticDistance = getChild<LLUICtrl>("RenderShadowAutomaticDistance");

	mRenderShadowBlurSize = getChild<LLUICtrl>("RenderShadowBlurSize");
	mRenderSSRResolution = getChild<LLUICtrl>("RenderSSRResolution");
	mRenderSSRBrightness = getChild<LLUICtrl>("RenderSSRBrightness");

	mRenderShadowBlurSize = getChild<LLUICtrl>("RenderShadowBlurSize");
	mRenderSSRResolution = getChild<LLUICtrl>("RenderSSRResolution");
	mRenderSSRBrightness = getChild<LLUICtrl>("RenderSSRBrightness");
	mRenderDepthOfFieldHighQuality = getChild<LLUICtrl>("RenderDepthOfFieldHighQuality");
	mCameraFOV = getChild<LLUICtrl>("CameraFOV");
	mCameraFNum = getChild<LLUICtrl>("CameraFNum");
	mCameraFocal = getChild<LLUICtrl>("CameraFocal");
	mCameraCoF = getChild<LLUICtrl>("CameraCoF");
	mCameraFocusTrans = getChild<LLUICtrl>("CameraFocusTrans");
	mCameraDoFRes = getChild<LLUICtrl>("CameraDoFRes");
	mRenderSSAOBlurSize = getChild<LLUICtrl>("RenderSSAOBlurSize");
	mSSAOEffect = getChild<LLUICtrl>("SSAOEffect");
	mSSAOScale = getChild<LLUICtrl>("SSAOScale");
	mSSAOMaxScale = getChild<LLUICtrl>("SSAOMaxScale");
	mSSAOFactor = getChild<LLUICtrl>("SSAOFactor");
	mRenderRiggedMotionBlurQuality = getChild<LLUICtrl>("RenderRiggedMotionBlurQuality");
	mMotionBlurQuality = getChild<LLUICtrl>("MotionBlurQuality");
	mRenderGodrays = getChild<LLUICtrl>("RenderGodrays");
	mRenderGodraysDirectional = getChild<LLUICtrl>("RenderGodraysDirectional");
	mRenderGodraysResolution = getChild<LLUICtrl>("GodraysResolution");
	mRenderGodraysMultiplier = getChild<LLUICtrl>("GodraysMultiplier");
	mRenderGodraysFalloffMultiplier = getChild<LLUICtrl>("GodraysFalloffMultiplier");

	mDisplayTabs = { { getChild<LLView>("viewer_layout_panel"),
					getChild<LLView>("lod_layout_panel"),
					getChild<LLView>("performance_layout_panel"),
					getChild<LLView>("vertex_layout_panel"),
					getChild<LLView>("deferred_layout_panel"),
					getChild<LLView>("dof_layout_panel"),
					getChild<LLView>("ao_layout_panel"),
					getChild<LLView>("motionblur_layout_panel"),
					getChild<LLView>("godrays_layout_panel"),
					getChild<LLView>("post_layout_panel"),
					getChild<LLView>("tonemapping_layout_panel"),
					getChild<LLView>("vignette_layout_panel") } };

//	//BD - Custom Keyboard Layout
	mBindModeList = this->getChild<LLScrollListCtrl>("scroll_mode", true);
	mBindModeList->setDoubleClickCallback(boost::bind(&LLFloaterPreference::onListClickAction, this));

//	//BD - Warning System
	mWarning0 = getChild<LLUICtrl>("warning_ui_size");
	mWarning1 = getChild<LLUICtrl>("warning_font_dpi");
	mWarning2 = getChild<LLUICtrl>("warning_texture_memory");
	mWarning3 = getChild<LLUICtrl>("warning_object_lod");
	mWarning4 = getChild<LLUICtrl>("warning_draw_distance");
	mWarning5 = getChild<LLUICtrl>("warning_avatars_visible");
	mWarning6 = getChild<LLUICtrl>("warning_derender_m2");
	mWarning7 = getChild<LLUICtrl>("warning_derender_ar");
	mWarning8 = getChild<LLUICtrl>("warning_derender_surface");
	mWarning9 = getChild<LLUICtrl>("warning_reflection_quality");
	mWarning10 = getChild<LLUICtrl>("warning_sky_quality");
	mWarning11 = getChild<LLUICtrl>("warning_shadow_resolution");
	mWarning12 = getChild<LLUICtrl>("warning_projector_resolution");
	mWarning13 = getChild<LLUICtrl>("warning_blur_quality");
	mWarning14 = getChild<LLUICtrl>("warning_light_resolution");

//	//BD - Memory Allocation
	mSystemMemory = getChild<LLSliderCtrl>("SystemMemory");
	mSceneMemory = getChild<LLSliderCtrl>("SceneMemory");
	mProgressBar = getChild<LLProgressBar>("progress_bar");
	mGPUMemoryLabel = getChild<LLTextBox>("MemoryUsage");
	getChild<LLTextBox>("GPUString")->setTextArg("[GPU_STRING]", llformat("%s", (const char*)(glGetString(GL_RENDERER))));

	// if floater is opened before login set default localized do not disturb message
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
	}

	// set 'enable' property for 'Clear log...' button
	changed();

	LLLogChat::setSaveHistorySignal(boost::bind(&LLFloaterPreference::onLogChatHistorySaved, this));

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	// Hook up and init for filtering
	mFilterEdit = getChild<LLSearchEditor>("search_prefs_edit");
	mFilterEdit->setKeystrokeCallback(boost::bind(&LLFloaterPreference::onUpdateFilterTerm, this, false));
	
	//BD
	mLoadBtn = findChild<LLButton>("PrefLoadButton");
	mSaveBtn = findChild<LLButton>("PrefSaveButton");
	mDeleteBtn = findChild<LLButton>("PrefDeleteButton");

	//BD - Refresh our controls at the start
	refreshGraphicControls();
	refreshCameraControls();
//	//BD - Custom Keyboard Layout
	refreshKeys();

//	//BD - Expandable Tabs
	mModifier = 0;

	//BD - TODO: Remove?
	if (!gSavedSettings.getBOOL("RememberPreferencesTabs"))
	{
		gSavedSettings.setBOOL("PrefsViewerVisible", false);
		gSavedSettings.setBOOL("PrefsLoDVisible", false);
		gSavedSettings.setBOOL("PrefsPerformanceVisible", false);
		gSavedSettings.setBOOL("PrefsVertexVisible", false);
		gSavedSettings.setBOOL("PrefsDeferredVisible", false);
		gSavedSettings.setBOOL("PrefsDoFVisible", false);
		gSavedSettings.setBOOL("PrefsAOVisible", false);
		gSavedSettings.setBOOL("PrefsMotionBlurVisible", false);
		gSavedSettings.setBOOL("PrefsGodraysVisible", false);
		gSavedSettings.setBOOL("PrefsPostVisible", false);
		gSavedSettings.setBOOL("PrefsToneMappingVisible", false);
		gSavedSettings.setBOOL("PrefsVignetteVisible", false);
	}

//	//BD - Avatar Rendering Settings
	mAvatarSettingsList = getChild<LLNameListCtrl>("render_settings_list");
	mAvatarSettingsList->setRightMouseDownCallback(boost::bind(&LLFloaterPreference::onAvatarListRightClick, this, _1, _2, _3));
	getChild<LLFilterEditor>("people_filter_input")->setCommitCallback(boost::bind(&LLFloaterPreference::onFilterEdit, this, _2));

	return TRUE;
}

void LLFloaterPreference::updateDeleteTranscriptsButton()
{
	std::vector<std::string> list_of_transcriptions_file_names;
	LLLogChat::getListOfTranscriptFiles(list_of_transcriptions_file_names);
	getChild<LLButton>("delete_transcripts")->setEnabled(list_of_transcriptions_file_names.size() > 0);
}

void LLFloaterPreference::onDoNotDisturbResponseChanged()
{
	// set "DoNotDisturbResponseChanged" TRUE if user edited message differs from default, FALSE otherwise
	bool response_changed_flag =
			LLTrans::getString("DoNotDisturbModeResponseDefault")
					!= getChild<LLUICtrl>("do_not_disturb_response")->getValue().asString();

	gSavedPerAccountSettings.setBOOL("DoNotDisturbResponseChanged", response_changed_flag );
}

LLFloaterPreference::~LLFloaterPreference()
{
	LLConversationLog::instance().removeObserver(this);

//	//BD - Avatar Rendering Settings
	delete mContextMenu;
	LLRenderMuteList::getInstance()->removeObserver(&sAvatarRenderMuteListObserver);
	LLAvatarPropertiesProcessor::getInstance()->removeObserver(gAgent.getID(), this);
}

//BD - Custom Keyboard Layout
void LLFloaterPreference::onExportControls()
{
	if (!mBindModeList)
		return;

	S32 mode = getChild<LLComboBox>("keybinding_mode")->getValue();
	gViewerKeyboard.unbindModeKeys(true, mode);

	S32 it = 0;
	while (it < mBindModeList->getItemCount())
	{
		mBindModeList->selectNthItem(it);
		LLScrollListItem* row = mBindModeList->getFirstSelected();
		MASK old_mask = MASK_NONE;
		KEY old_key = NULL;
		LLKeyboard::keyFromString(row->getColumn(2)->getValue().asString(), &old_key);
		LLKeyboard::maskFromString(row->getColumn(3)->getValue().asString(), &old_mask);
		gViewerKeyboard.bindKey(mode, old_key, old_mask, row->getColumn(1)->getValue().asString());
		it++;
	}
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");
	gViewerKeyboard.exportBindingsXML(filename);
	refreshKeys();
}

void LLFloaterPreference::onUnbindControls()
{
	//BD - Simply unbind everything and save it.
	gViewerKeyboard.unbindAllKeys(true);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");
	gViewerKeyboard.exportBindingsXML(filename);
	refreshKeys();
}

void LLFloaterPreference::onDefaultControls()
{
	gViewerKeyboard.unbindAllKeys(true);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "controls.xml");
	LL_INFOS("Settings") << "Loading default controls file from " << filename << LL_ENDL;
	if (gViewerKeyboard.loadBindingsSettings(filename))
	{
		gViewerKeyboard.exportBindingsXML(gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml"));
	}
	refreshKeys();
}

void LLFloaterPreference::onRemoveBind()
{
	if (!mBindModeList)
		return;

	mBindModeList->deleteSelectedItems();
	onExportControls();
}

void LLFloaterPreference::onAddBind(KEY key, MASK mask, std::string action)
{
	if (!mBindModeList)
		return;

	LLSD row;
	row["columns"][0]["column"] = "action";
	row["columns"][0]["value"] = getString(action);
	row["columns"][1]["column"] = "function";
	row["columns"][1]["value"] = action;
	row["columns"][2]["column"] = "button";
	row["columns"][2]["value"] = LLKeyboard::stringFromKey(key);
	row["columns"][3]["column"] = "modifiers";
	row["columns"][3]["value"] = LLKeyboard::stringFromMask(mask, true);
	mBindModeList->addElement(row);
	onExportControls();
}

void LLFloaterPreference::onReplaceBind(KEY key, MASK mask)
{
	if (!mBindModeList)
		return;

	LLScrollListItem* item = mBindModeList->getFirstSelected();
	if (item)
	{
		LLScrollListCell* column_3 = item->getColumn(2);
		LLScrollListCell* column_4 = item->getColumn(3);

		column_3->setValue(LLKeyboard::stringFromKey(key));
		column_4->setValue(LLKeyboard::stringFromMask(mask, true));
	}
	onExportControls();
}

void LLFloaterPreference::onListClickAction()
{
	S32 mode = getChild<LLComboBox>("keybinding_mode")->getValue();
	if (mBindModeList)
	{
		LLScrollListItem* row = mBindModeList->getFirstSelected();
		if (row)
		{
			LLChangeKeyDialog* dialog = LLFloaterReg::getTypedInstance<LLChangeKeyDialog>("change_key", LLSD());
			if (dialog)
			{
				MASK mask = MASK_NONE;
				KEY key = NULL;
				LLKeyboard::keyFromString(row->getColumn(2)->getValue().asString(), &key);
				LLKeyboard::maskFromString(row->getColumn(3)->getValue().asString(), &mask);

				dialog->setParent(this);
				dialog->setMode(mode);
				dialog->setKey(key);
				dialog->setMask(mask);

				LLFloaterReg::showTypedInstance<LLChangeKeyDialog>("change_key", LLSD(), TRUE);
			}
		}
	}
}

void LLFloaterPreference::refreshKeys()
{
	if (!mBindModeList)
		return;

	LLSD settings;
	llifstream infile;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");

	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Settings") << "Cannot find file " << filename << " to load." << LL_ENDL;
		return;
	}

	mBindModeList->clearRows();

	S32 mode = getChild<LLComboBox>("keybinding_mode")->getValue();
	while (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(settings, infile))
	{
		if(mode != settings["mode"].asInteger())
			continue;

		LLSD row;
		MASK mask = MASK_NONE;

		//BD - Translate to human readable text.
		LLKeyboard::maskFromString(settings["mask"].asString(), &mask);
		std::string action_str = getString(settings["function"].asString());

		row["columns"][0]["column"] = "action";
		row["columns"][0]["value"] = action_str;
		row["columns"][1]["column"] = "function";
		row["columns"][1]["value"] = settings["function"].asString();
		row["columns"][2]["column"] = "button";
		row["columns"][2]["value"] = settings["key"].asString();
		row["columns"][3]["column"] = "modifiers";
		row["columns"][3]["value"] = LLKeyboard::stringFromMask(mask, true);
		mBindModeList->addElement(row);
	}
	
	infile.close();
}

void LLFloaterPreference::onClickSetAnyKey()
{
	S32 mode = getChild<LLComboBox>("keybinding_mode")->getValue();

	//BD - Don't show the dialog if we have no action selected.
	LLSetKeyDialog* dialog = LLFloaterReg::showTypedInstance<LLSetKeyDialog>("set_any_key", LLSD(), TRUE);
	if (dialog)
	{
		dialog->setParent(this);
		dialog->setMode(mode);
	}
}

//BD - Expandable Tabs
void LLFloaterPreference::toggleTabs()
{
	mGFXStack->translate(0, -mModifier);

	LLRect rect = getChild<LLPanel>("gfx_scroll_panel")->getRect();
	mModifier = 0;

	if (gSavedSettings.getBOOL("PrefsViewerVisible"))
		mModifier += (mDisplayTabs[0]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsLoDVisible"))
		mModifier += (mDisplayTabs[1]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsPerformanceVisible"))
		mModifier += (mDisplayTabs[2]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsVertexVisible"))
		mModifier += (mDisplayTabs[3]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsDeferredVisible"))
		mModifier += (mDisplayTabs[4]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsDoFVisible"))
		mModifier += (mDisplayTabs[5]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsAOVisible"))
		mModifier += (mDisplayTabs[6]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsMotionBlurVisible"))
		mModifier += (mDisplayTabs[7]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsGodraysVisible"))
		mModifier += (mDisplayTabs[8]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsPostVisible"))
		mModifier += (mDisplayTabs[9]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsToneMappingVisible"))
		mModifier += (mDisplayTabs[10]->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsVignetteVisible"))
		mModifier += (mDisplayTabs[11]->getRect().getHeight() - 5);

	rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), (255 + mModifier));
	getChild<LLPanel>("gfx_scroll_panel")->setRect(rect);
	mGFXStack->translate(0, mModifier);
}

//BD - Input/Output resizer
void LLFloaterPreference::inputOutput()
{
	LLPanel* panel = getChild<LLPanel>("audio_media_panel");
	LLPanel* panel2 = getChild<LLPanel>("device_settings_panel");
	if(panel)
	{
		if(gSavedSettings.getBOOL("ShowDeviceSettings"))
		{
			panel->reshape(panel->getRect().getWidth(), panel->getRect().getHeight() + panel2->getRect().getHeight());
		}
		else
		{
			panel->reshape(panel->getRect().getWidth(), panel->getRect().getHeight() - panel2->getRect().getHeight());
		}
	}
}

//BD - Refresh Display Settings
void LLFloaterPreference::refreshEverything()
{
	//BD - Start with global preferences stuff
	if (mTabContainer->getCurrentPanelIndex() == 1)
	{
//		//BD - Warning System
		refreshWarnings();

		LLRect scroll_rect = mGFXStack->calcScreenRect();

		//BD - Viewer Options
		//===================
		LLRect layout_rect = mDisplayTabs[0]->calcScreenRect();
		if (scroll_rect.overlaps(layout_rect))
		{
//			//BD - Memory Allocation
			//     Since we cannot work out how to get AMD Cards to properly and accurately
			//     report back its maximum and free memory we'll just use the max, whatever
			//     that is and only show the actual used memory from SL. Only NVIDIA Cards seem
			//     to properly and accurately report back their max and free memory.
			if (gGLManager.mIsNVIDIA)
			{
				refreshMemoryControls();
			}
		}

		bool deferred_enabled = gPipeline.RenderDeferred;
		//BD - Deferred Rendering
		//=======================
		LLRect deferred_rect = mDisplayTabs[4]->calcScreenRect();
		if (scroll_rect.overlaps(deferred_rect))
		{
			bool shadows_enabled = (gPipeline.RenderShadowDetail > 0 && deferred_enabled);
			mRenderShadowResolution[0]->setEnabled(shadows_enabled);
			mRenderShadowResolution[1]->setEnabled(shadows_enabled);
			mRenderShadowResolution[2]->setEnabled(shadows_enabled);
			mRenderShadowResolution[3]->setEnabled(shadows_enabled);

			mRenderShadowAutomaticDistance->setEnabled(shadows_enabled);

			bool auto_shadow_distance = (gPipeline.RenderShadowAutomaticDistance && shadows_enabled );
			mRenderShadowDistance[0]->setEnabled(shadows_enabled);
			mRenderShadowDistance[1]->setEnabled(!auto_shadow_distance && shadows_enabled);
			mRenderShadowDistance[2]->setEnabled(!auto_shadow_distance && shadows_enabled);
			mRenderShadowDistance[3]->setEnabled(!auto_shadow_distance && shadows_enabled);

			bool projectors_enabled = (gPipeline.RenderShadowDetail > 1 && deferred_enabled);
			mRenderProjectorShadowResolution[0]->setEnabled(projectors_enabled);
			mRenderProjectorShadowResolution[1]->setEnabled(projectors_enabled);

			mRenderSpotLightReflections->setEnabled(projectors_enabled);
			mRenderSpotLightImages->setEnabled(projectors_enabled);

			bool soften_enabled = (gPipeline.RenderDeferredBlurLight && deferred_enabled);
			mRenderShadowBlurSize->setEnabled(soften_enabled);

			bool ssr_enabled = (gSavedSettings.getBOOL("RenderScreenSpaceReflections") && deferred_enabled);
			mRenderSSRResolution->setEnabled(ssr_enabled);
			mRenderSSRBrightness->setEnabled(ssr_enabled);
		}

		//BD - Depth of Field
		//===================
		LLRect dof_rect = mDisplayTabs[5]->calcScreenRect();
		if (scroll_rect.overlaps(dof_rect))
		{
			bool dof_enabled = (gPipeline.RenderDepthOfField && deferred_enabled);
			mRenderDepthOfFieldHighQuality->setEnabled(dof_enabled);
			mCameraFOV->setEnabled(dof_enabled);
			mCameraFNum->setEnabled(dof_enabled);
			mCameraFocal->setEnabled(dof_enabled);
			mCameraCoF->setEnabled(dof_enabled);
			mCameraFocusTrans->setEnabled(dof_enabled);
			mCameraDoFRes->setEnabled(dof_enabled);
		}

		//BD - Screen Space Ambient Occlusion (SSAO)
		//==========================================
		LLRect ssao_rect = mDisplayTabs[6]->calcScreenRect();
		if (scroll_rect.overlaps(ssao_rect))
		{
			bool ssao_enabled = (gPipeline.RenderDeferredSSAO && deferred_enabled);
			mRenderSSAOBlurSize->setEnabled(ssao_enabled);
			mSSAOEffect->setEnabled(ssao_enabled);
			mSSAOScale->setEnabled(ssao_enabled);
			mSSAOMaxScale->setEnabled(ssao_enabled);
			mSSAOFactor->setEnabled(ssao_enabled);
		}

		//BD - Motion Blur
		//================
		LLRect blur_rect = mDisplayTabs[7]->calcScreenRect();
		if (scroll_rect.overlaps(blur_rect))
		{
			bool blur_enabled = (gPipeline.RenderMotionBlur && deferred_enabled);
			mRenderRiggedMotionBlurQuality->setEnabled(blur_enabled);
			mMotionBlurQuality->setEnabled(blur_enabled);
		}

		//BD - Volumetric Lighting
		//========================
		LLRect volumetric_rect = mDisplayTabs[8]->calcScreenRect();
		if (scroll_rect.overlaps(volumetric_rect))
		{
			bool shadows_enabled = (gPipeline.RenderShadowDetail > 0 && deferred_enabled);
			bool volumetric_enabled = (gPipeline.RenderGodrays && shadows_enabled);
			mRenderGodrays->setEnabled(shadows_enabled);
			mRenderGodraysDirectional->setEnabled(volumetric_enabled);
			mRenderGodraysResolution->setEnabled(volumetric_enabled);
			mRenderGodraysMultiplier->setEnabled(volumetric_enabled);
			mRenderGodraysFalloffMultiplier->setEnabled(volumetric_enabled);
		}

		//BD - Tone Mapping
		//=================
		LLRect tone_rect = mDisplayTabs[10]->calcScreenRect();
		if (scroll_rect.overlaps(deferred_rect))
		{
			//BD - Tone Mapping
			bool tone_enabled = (exoPostProcess::sExodusRenderToneMapping && deferred_enabled);
			bool custom_enabled = (mExodusRenderToneMappingTech->getValue().asInteger() == 3 && tone_enabled);
			mExodusRenderToneMappingTech->setEnabled(deferred_enabled);
			mExodusRenderToneExposure->setEnabled(tone_enabled);
			mExodusRenderToneAdvOptA[0]->setEnabled(custom_enabled);
			mExodusRenderToneAdvOptA[1]->setEnabled(custom_enabled);
			mExodusRenderToneAdvOptA[2]->setEnabled(custom_enabled);

			mExodusRenderToneAdvOptB[0]->setEnabled(custom_enabled);
			mExodusRenderToneAdvOptB[1]->setEnabled(custom_enabled);
			mExodusRenderToneAdvOptB[2]->setEnabled(custom_enabled);

			mExodusRenderToneAdvOptC[0]->setEnabled(custom_enabled);
			mExodusRenderToneAdvOptC[1]->setEnabled(custom_enabled);

			//BD - Color Correction
			bool color_enabled = (mExodusRenderColorGradeTech->getValue().asInteger() == 0 && deferred_enabled);
			mExodusRenderColorGradeTech->setEnabled(deferred_enabled);
			mExodusRenderGamma[0]->setEnabled(color_enabled);
			mExodusRenderGamma[1]->setEnabled(color_enabled);
			mExodusRenderGamma[2]->setEnabled(color_enabled);

			mExodusRenderExposure[0]->setEnabled(color_enabled);
			mExodusRenderExposure[1]->setEnabled(color_enabled);
			mExodusRenderExposure[2]->setEnabled(color_enabled);

			mExodusRenderOffset[0]->setEnabled(color_enabled);
			mExodusRenderOffset[1]->setEnabled(color_enabled);
			mExodusRenderOffset[2]->setEnabled(color_enabled);
		}
	}

	if (mTabContainer->getCurrentPanelIndex() == 9)
	{
		BOOL has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected() != NULL);
		gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);

		has_first_selected = (getChildRef<LLScrollListCtrl>("enabled_popups").getFirstSelected() != NULL);
		gSavedSettings.setBOOL("FirstSelectedEnabledPopups", has_first_selected);
	}
}

//BD - Refresh all controls
void LLFloaterPreference::refreshGraphicControls()
{
	LLVector3 vec3 = gSavedSettings.getVector3("RenderGlowLumWeights");
	mRenderGlowLumWeights[0]->setValue(vec3.mV[VX]);
	mRenderGlowLumWeights[1]->setValue(vec3.mV[VY]);
	mRenderGlowLumWeights[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("RenderGlowWarmthWeights");
	mRenderGlowWarmthWeights[0]->setValue(vec3.mV[VX]);
	mRenderGlowWarmthWeights[1]->setValue(vec3.mV[VY]);
	mRenderGlowWarmthWeights[2]->setValue(vec3.mV[VZ]);

	LLVector4 vec4 = gSavedSettings.getVector4("RenderShadowDistance");
	mRenderShadowDistance[0]->setValue(vec4.mV[VX]);
	mRenderShadowDistance[1]->setValue(vec4.mV[VY]);
	mRenderShadowDistance[2]->setValue(vec4.mV[VZ]);
	mRenderShadowDistance[3]->setValue(vec4.mV[VW]);
	vec4 = gSavedSettings.getVector4("RenderShadowResolution");
	mRenderShadowResolution[0]->setValue(vec4.mV[VX]);
	mRenderShadowResolution[1]->setValue(vec4.mV[VY]);
	mRenderShadowResolution[2]->setValue(vec4.mV[VZ]);
	mRenderShadowResolution[3]->setValue(vec4.mV[VW]);

	LLVector2 vec2 = gSavedSettings.getVector2("RenderProjectorShadowResolution");
	mRenderProjectorShadowResolution[0]->setValue(vec2.mV[VX]);
	mRenderProjectorShadowResolution[1]->setValue(vec2.mV[VY]);

	vec3 = gSavedSettings.getVector3("ExodusRenderToneAdvOptA");
	mExodusRenderToneAdvOptA[0]->setValue(vec3.mV[VX]);
	mExodusRenderToneAdvOptA[1]->setValue(vec3.mV[VY]);
	mExodusRenderToneAdvOptA[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("ExodusRenderToneAdvOptB");
	mExodusRenderToneAdvOptB[0]->setValue(vec3.mV[VX]);
	mExodusRenderToneAdvOptB[1]->setValue(vec3.mV[VY]);
	mExodusRenderToneAdvOptB[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("ExodusRenderToneAdvOptC");
	mExodusRenderToneAdvOptC[0]->setValue(vec3.mV[VX]);
	mExodusRenderToneAdvOptC[1]->setValue(vec3.mV[VY]);

	vec3 = gSavedSettings.getVector3("ExodusRenderGamma");
	mExodusRenderGamma[0]->setValue(vec3.mV[VX]);
	mExodusRenderGamma[1]->setValue(vec3.mV[VY]);
	mExodusRenderGamma[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("ExodusRenderExposure");
	mExodusRenderExposure[0]->setValue(vec3.mV[VX]);
	mExodusRenderExposure[1]->setValue(vec3.mV[VY]);
	mExodusRenderExposure[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("ExodusRenderOffset");
	mExodusRenderOffset[0]->setValue(vec3.mV[VX]);
	mExodusRenderOffset[1]->setValue(vec3.mV[VY]);
	mExodusRenderOffset[2]->setValue(vec3.mV[VZ]);
	vec3 = gSavedSettings.getVector3("ExodusRenderVignette");
	mExodusRenderVignette[0]->setValue(vec3.mV[VX]);
	mExodusRenderVignette[1]->setValue(vec3.mV[VY]);
	mExodusRenderVignette[2]->setValue(vec3.mV[VZ]);

	//BD - Anything that triggers this should also refresh the sidebar.
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		gSideBar->refreshGraphicControls();
	}
}

//BD - Warning System
void LLFloaterPreference::refreshWarnings()
{
	//BD - Viewer Options
	mWarning0->setVisible(gSavedSettings.getF32("UIScaleFactor") != 1.0);
	mWarning1->setVisible(gSavedSettings.getF32("FontScreenDPI") != 96.0);
	S32 max_vram = gGLManager.mVRAM;
	S32 tex_mem = gSavedSettings.getS32("TextureMemory");
	S32 sys_mem = gSavedSettings.getS32("SystemMemory");
	mWarning2->setVisible((tex_mem < 368) || (sys_mem < 512)
						|| (tex_mem > 1024) || (sys_mem > 1024)
						|| (tex_mem + sys_mem) > (max_vram * 0.8));

	//BD - Quality Options
	mWarning3->setVisible(LLVOVolume::sLODFactor > 2.0);

	//BD - Rendering Options
	mWarning4->setVisible(gPipeline.RenderFarClip > 128);
	mWarning5->setVisible(gSavedSettings.getU32("RenderAvatarMaxNonImpostors") > 15);
	mWarning6->setVisible(gSavedSettings.getF32("RenderAutoMuteSurfaceAreaLimit") > 256.f);
	mWarning7->setVisible(gSavedSettings.getU32("RenderAvatarMaxComplexity") > 250000);
	mWarning8->setVisible(gSavedSettings.getF32("RenderAutoHideSurfaceAreaLimit") > 256.f);

	//BD - Windlight Options
	mWarning9->setVisible(gSavedSettings.getS32("RenderReflectionRes") > 768);
	mWarning10->setVisible(gSavedSettings.getU32("WLSkyDetail") > 128);

	//BD - Deferred Rendering Options
	mWarning11->setVisible(gPipeline.RenderShadowResolution.mV[VX] > 2048
															|| gPipeline.RenderShadowResolution.mV[VY] > 2048
															|| gPipeline.RenderShadowResolution.mV[VZ] > 1024
															|| gPipeline.RenderShadowResolution.mV[VW] > 1024);
	mWarning12->setVisible(gPipeline.RenderProjectorShadowResolution.mV[VX] > 2048
																|| gPipeline.RenderProjectorShadowResolution.mV[VY] > 2048);

	//BD - Motion Blur Options
	mWarning13->setVisible(gSavedSettings.getU32("RenderMotionBlurStrength") < 80);

	//BD - Volumetric Lighting Options
	mWarning14->setVisible(gSavedSettings.getU32("RenderGodraysResolution") > 32);
}

//BD - Memory Allocation
void LLFloaterPreference::refreshMemoryControls()
{
	U32Megabytes bound_mem = LLViewerTexture::sMaxBoundTextureMemory;
	U32Megabytes total_mem = LLViewerTexture::sMaxDesiredTextureMem;
	S32 max_vram = gGLManager.mVRAM;
	S32 used_vram = 0;
	S32 avail_vram;
	S32 max_mem;
	F32 percent;

	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &avail_vram);
	used_vram = max_vram - (avail_vram / 1024);

	//BD - Limit our slider max further on how much is actually still available.
	max_mem = max_vram - bound_mem.value() + total_mem.value();

	//BD - Cap out at the highest possible stable value we tested.
	max_mem = llclamp(max_mem, 128, 3984);

	//BD - Don't update max values when the widget is selected, we make entering values impossible otherwise.
	if (!mSystemMemory->hasFocus() && !mSceneMemory->hasFocus())
	{
		mSystemMemory->setMaxValue(max_mem);
		mSceneMemory->setMaxValue(max_mem);
	}

	percent = llclamp(((F32)used_vram / (F32)max_vram) * 100.f, 0.f, 100.f);
	mProgressBar->setValue(percent);
	mGPUMemoryLabel->setTextArg("[USED_MEM]", llformat("%5d", used_vram));
	mGPUMemoryLabel->setTextArg("[MAX_MEM]", llformat("%5d", max_vram));
}

//BD - Unlimited Camera Presets
void LLFloaterPreference::onCameraArray(LLUICtrl* ctrl, const LLSD& param)
{
	std::string name = gSavedSettings.getString("CameraPresetName");
	LLVector3 vec3 = gAgentCamera.mCameraOffsetInitial[name];

	if (param.asString() == "X")
	{
		vec3[VX] = ctrl->getValue().asReal();
	}
	else if (param.asString() == "Y")
	{
		vec3[VY] = ctrl->getValue().asReal();
	}
	else
	{
		vec3[VZ] = ctrl->getValue().asReal();
	}

	gAgentCamera.mCameraOffsetInitial[name] = vec3;
	onAddCameraPreset();
}

void LLFloaterPreference::onFocusArray(LLUICtrl* ctrl, const LLSD& param)
{
	std::string name = gSavedSettings.getString("CameraPresetName");
	LLVector3d vec3 = gAgentCamera.mFocusOffsetInitial[name];

	if (param.asString() == "X")
	{
		vec3[VX] = ctrl->getValue().asReal();
	}
	else if (param.asString() == "Y")
	{
		vec3[VY] = ctrl->getValue().asReal();
	}
	else
	{
		vec3[VZ] = ctrl->getValue().asReal();
	}

	gAgentCamera.mFocusOffsetInitial[name] = vec3;
	onAddCameraPreset();
}

void LLFloaterPreference::onAddCameraPreset(bool refresh)
{
	//BD - First and foremost before we do anything, check if the folder exists.
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "camera");
	if (!gDirUtilp->fileExists(pathname))
	{
		LL_WARNS("Camera") << "Couldn't find folder: " << pathname << " - creating one." << LL_ENDL;
		LLFile::mkdir(pathname);
	}

	//BD - Get the name and make sure its not empty.
	std::string name = getChild<LLComboBox>("camera_preset_name")->getValue().asString();
	if (name.empty())
	{
		LL_WARNS("Camera") << "No name given for camera preset." << LL_ENDL;
		return;
	}

	LLSD record;
	llofstream file;

	if (refresh)
	{
		//BD - Creates a basic empty camera preset for us to edit.
		record["camera_offset"] = LLVector3(-3.f, 0.f, 0.f).getValue();
		record["focus_offset"] = LLVector3d(0.3f, 0.f, 0.75f).getValue();
	}
	else
	{
		//BD - We're editing an already existing.
		LLVector3 camera = gAgentCamera.mCameraOffsetInitial[name];
		LLVector3d focus = gAgentCamera.mFocusOffsetInitial[name];
		record["camera_offset"] = camera.getValue();
		record["focus_offset"] = focus.getValue();
	}

	//BD - Hide custom presets of RLVa View, RLVa will be using this.
	if (name == "RLVa View")
	{
		record["hidden"] = true;
	}

	//BD - Write the camera preset and save it into our folder.
	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "camera" , gDragonLibrary.escapeString(name) + ".xml");
	file.open(full_path.c_str());
	LLSDSerialize::toXML(record, file);
	file.close();

	if (refresh)
	{
		//BD - Switch to the newly created camera preset so we can start editing it right away.
		gAgentCamera.switchCameraPreset(name);

		//BD - Refresh our presets list.
		refreshPresets();

		//BD - Keep the controls in sync.
		refreshCameraControls();
	}
}

void LLFloaterPreference::onRemoveCameraPreset()
{
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "camera");
	std::string name = gSavedSettings.getString("CameraPresetName");

	if (gDirUtilp->deleteFilesInDir(pathname, LLURI::escape(name) + ".xml") < 1)
	{
		LL_WARNS("Camera") << "Cannot remove camera preset file: " << name << LL_ENDL;
	}

	gAgentCamera.initializeCameraPresets();

	//BD - Refresh.
	refreshPresets();
}

void LLFloaterPreference::refreshPresets()
{
	LLComboBox* combo = getChild<LLComboBox>("camera_preset_name");
	combo->removeall();

	//BD - Look through our defaults first.
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
		if (preset["hidden"].isDefined())
		{
			continue;
		}

		combo->add(name);
	}

	//BD - Add a separator.
	combo->addSeparator(ADD_BOTTOM);

	//BD - Go through all custom presets.
	dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "camera");
	LLDirIterator dir_iter_user(dir, "*.xml");
	while (dir_iter_user.next(file))
	{
		std::string path = gDirUtilp->add(dir, file);
		std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

		//BD - Don't add if we've already established it as default. It's name is already
		//     in the list.
		if (!combo->itemExists(name))
		{
			combo->add(name);
		}
	}
}

void LLFloaterPreference::refreshCameraControls()
{
	std::string preset_name = gSavedSettings.getString("CameraPresetName");
	LLVector3 vec3 = gAgentCamera.mCameraOffsetInitial[preset_name];
	LLVector3d vec3d = gAgentCamera.mFocusOffsetInitial[preset_name];

	getChild<LLUICtrl>("CameraOffset_X")->setValue(vec3.mV[VX]);
	getChild<LLUICtrl>("CameraOffset_Y")->setValue(vec3.mV[VY]);
	getChild<LLUICtrl>("CameraOffset_Z")->setValue(vec3.mV[VZ]);
	getChild<LLUICtrl>("FocusOffset_X")->setValue(vec3d.mdV[VX]);
	getChild<LLUICtrl>("FocusOffset_Y")->setValue(vec3d.mdV[VY]);
	getChild<LLUICtrl>("FocusOffset_Z")->setValue(vec3d.mdV[VZ]);

	//BD - Disable the delete button when we have a default preset selected.
	//     We will instead use the default buttons which essentially do the same
	//     but make more sense label wise for the user.
	if (preset_name == "Front View" || preset_name == "Group View"
		|| preset_name == "Left Shoulder View" || preset_name == "Right Shoulder View"
		|| preset_name == "Rear View" || preset_name == "RLVa View"
		|| preset_name == "Mouselook" || preset_name == "Top View")
	{
		getChild<LLButton>("DeleteCameraPreset")->setEnabled(FALSE);
	}
	else
	{
		getChild<LLButton>("DeleteCameraPreset")->setEnabled(TRUE);
	}
}

void LLFloaterPreference::onChangeCameraPreset()
{
	std::string name = getChild<LLComboBox>("camera_preset_name")->getValue().asString();

	//BD - Don't switch to the Mouselook preset.
	if (name != "Mouselook")
	{
		//BD - Switch to the selected camera preset so we can our changes live.
		gAgentCamera.switchCameraPreset(name);
	}
	else
	{
		//BD - Since we don't switch to the preset we need to set the debug manually.
		gSavedSettings.setString("CameraPresetName", name);
	}

	//BD - Keep the controls in sync.
	refreshCameraControls();
}

void LLFloaterPreference::onCameraPresetReset(const LLSD& param)
{
	//BD - If we are using a modified default preset, delete it and reload the default
	//     settings for it.
	std::string preset_name = gSavedSettings.getString("CameraPresetName");
	if (preset_name == "Front View" || preset_name == "Group View"
		|| preset_name == "Left Shoulder View" || preset_name == "Right Shoulder View"
		|| preset_name == "Rear View" || preset_name == "RLVa View"
		|| preset_name == "Mouselook" || preset_name == "Top View")
	{
		//BD - Remove the currently selected preset.
		onRemoveCameraPreset();

		//BD - Don't switch to the Mouselook preset.
		if (preset_name != "Mouselook")
		{
			//BD - Reload the preset to load the default version of it.
			gAgentCamera.switchCameraPreset(preset_name);
		}
	}
	else
	{
		//BD - In case we are defaulting a non default preset, reset to a default rear
		//     preset just like if we created a new one.
		gAgentCamera.mCameraOffsetInitial[preset_name] = LLVector3(-3.f, 0.f, 0.f);
		gAgentCamera.mFocusOffsetInitial[preset_name] = LLVector3d(0.3f, 0.f, 0.75f);

		//BD - Overwrite it with the new values.
		onAddCameraPreset();
	}

	refreshCameraControls();
}

void LLFloaterPreference::draw()
{
	if (mUpdateTimer.getElapsedTimeF32() > 1.f)
	{
		mUpdateTimer.reset();

		refreshEverything();
	}

//	//BD - Avatar Rendering Settings
	if ((mTabContainer->getCurrentPanelIndex() == 2
		&& mAvatarSettingsList->isEmpty()) || mNeedsUpdate)
	{
		updateList();
		mNeedsUpdate = false;
	}
	
	LLFloater::draw();
}

void LLFloaterPreference::saveSettings()
{
	child_list_t::const_iterator iter = mTabContainer->getChildList()->begin();
	child_list_t::const_iterator end = mTabContainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->saveSettings();
	}
}	

void LLFloaterPreference::apply()
{
	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );
	
	if (sSkin != gSavedSettings.getString("SkinCurrent"))
	{
		LLNotificationsUtil::add("ChangeSkin");
		refreshSkin(this);
	}
	// Call apply() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = mTabContainer->getChildList()->begin();
		iter != mTabContainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->apply();
	}

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);
	
	if (hasChild("web_proxy_enabled", TRUE) &&hasChild("web_proxy_editor", TRUE) && hasChild("web_proxy_port", TRUE))
	{
		bool proxy_enable = getChild<LLUICtrl>("web_proxy_enabled")->getValue();
		std::string proxy_address = getChild<LLUICtrl>("web_proxy_editor")->getValue();
		int proxy_port = getChild<LLUICtrl>("web_proxy_port")->getValue();
		LLViewerMedia::setProxyConfig(proxy_enable, proxy_address, proxy_port);
	}
	
	if (mGotPersonalInfo)
	{ 
		bool new_im_via_email = getChild<LLUICtrl>("send_im_to_email")->getValue().asBoolean();
		bool new_hide_online = getChild<LLUICtrl>("online_visibility")->getValue().asBoolean();		
	
		if ((new_im_via_email != mOriginalIMViaEmail)
			||(new_hide_online != mOriginalHideOnlineStatus))
		{
			// This hack is because we are representing several different 	 
			// possible strings with a single checkbox. Since most users 	 
			// can only select between 2 values, we represent it as a 	 
			// checkbox. This breaks down a little bit for liaisons, but 	 
			// works out in the end. 	 
			if (new_hide_online != mOriginalHideOnlineStatus)
			{
				if (new_hide_online) mDirectoryVisibility = VISIBILITY_HIDDEN;
				else mDirectoryVisibility = VISIBILITY_DEFAULT;
			 //Update showonline value, otherwise multiple applys won't work
				mOriginalHideOnlineStatus = new_hide_online;
			}
			gAgent.sendAgentUpdateUserInfo(new_im_via_email,mDirectoryVisibility);
		}
	}

	saveAvatarProperties();
}

void LLFloaterPreference::cancel()
{
	// Call cancel() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = mTabContainer->getChildList()->begin();
		iter != mTabContainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->cancel();
	}
	// hide joystick pref floater
	LLFloaterReg::hideInstance("pref_joystick");

	// hide translation settings floater
	LLFloaterReg::hideInstance("prefs_translation");
	
	// hide autoreplace settings floater
	LLFloaterReg::hideInstance("prefs_autoreplace");
	
	// hide spellchecker settings folder
	LLFloaterReg::hideInstance("prefs_spellchecker");

	// reverts any changes to current skin
	gSavedSettings.setString("SkinCurrent", sSkin);



	LLFloaterPreferenceProxy * advanced_proxy_settings = LLFloaterReg::findTypedInstance<LLFloaterPreferenceProxy>("prefs_proxy");
	if (advanced_proxy_settings)
	{
		advanced_proxy_settings->cancel();
	}
	//Need to reload the navmesh if the pathing console is up
	LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
	if ( !pathfindingConsoleHandle.isDead() )
	{
		LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
		pPathfindingConsole->onRegionBoundaryCross();
	}
}

void LLFloaterPreference::onOpen(const LLSD& key)
{

	// this variable and if that follows it are used to properly handle do not disturb mode response message
	static bool initialized = FALSE;
	// if user is logged in and we haven't initialized do not disturb mode response yet, do it
	if (!initialized && LLStartUp::getStartupState() == STATE_STARTED)
	{
		// Special approach is used for do not disturb response localization, because "DoNotDisturbModeResponse" is
		// in non-localizable xml, and also because it may be changed by user and in this case it shouldn't be localized.
		// To keep track of whether do not disturb response is default or changed by user additional setting DoNotDisturbResponseChanged
		// was added into per account settings.

		// initialization should happen once,so setting variable to TRUE
		initialized = TRUE;
		// this connection is needed to properly set "DoNotDisturbResponseChanged" setting when user makes changes in
		// do not disturb response message.
		gSavedPerAccountSettings.getControl("DoNotDisturbModeResponse")->getSignal()->connect(boost::bind(&LLFloaterPreference::onDoNotDisturbResponseChanged, this));
	}
	gAgent.sendAgentUserInfoRequest();

	/////////////////////////// From LLPanelGeneral //////////////////////////
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose_maturity =
		gAgent.getID().notNull() &&
		(gAgent.isMature() || gAgent.isGodlike());
	
	//BD
	LLRadioGroup* maturity_radio = getChild<LLRadioGroup>("maturity_desired_radio");
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest( gAgent.getID() );
	if (can_choose_maturity)
	{		
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlikeWithoutAdminMenuFakery())
		{
			//BD - We're going to disable the adult radio button
			maturity_radio->setIndexEnabled(2, FALSE);
		}
	}

//	//BD - Unlimited Camera Presets
	getChild<LLComboBox>("camera_preset_name")->setValue(gSavedSettings.getString("CameraPresetName"));

	maturity_radio->setEnabled(can_choose_maturity);

	// Forget previous language changes.
	mLanguageChanged = false;

	onChangeModelFolder();
	onChangeTextureFolder();
	onChangeSoundFolder();
	onChangeAnimationFolder();
	
	// Enabled/disabled popups, might have been changed by user actions
	// while preferences floater was closed.
	buildPopupLists();

	//get the options that were checked
	onNotificationsChange("FriendIMOptions");
	onNotificationsChange("NonFriendIMOptions");
	onNotificationsChange("ConferenceIMOptions");
	onNotificationsChange("GroupChatOptions");
	onNotificationsChange("NearbyChatOptions");
	onNotificationsChange("ObjectIMOptions");

	//BD
	//LLPanelLogin::setAlwaysRefresh(true);
	refresh();

	//BD
	refreshGraphicControls();
	refreshEnabledGraphics();
	toggleTabs();

//	//BD - Avatar Rendering Settings
	//     Only update this when absolutely necessary, this causes a huge 
	//     framedrop for up to a second or two.
	if ((mTabContainer->getCurrentPanelIndex() == 2
		&& mAvatarSettingsList->isEmpty()) || mNeedsUpdate)
	{
		updateList();
		mNeedsUpdate = false;
	}

//	//BD - Unlimited Camera Presets
	refreshPresets();
	refreshCameraControls();

	// Make sure the current state of prefs are saved away when
	// when the floater is opened.  That will make cancel do its
	// job
	saveSettings();

	// Make sure there is a default preference file
	LLPresetsManager::getInstance()->createMissingDefault();

	bool started = (LLStartUp::getStartupState() == STATE_STARTED);
	mLoadBtn->setEnabled(started);
	mSaveBtn->setEnabled(started);
	mDeleteBtn->setEnabled(started);

	collectSearchableItems();
	if (!mFilterEdit->getText().empty())
	{
		mFilterEdit->setText(LLStringExplicit(""));
		onUpdateFilterTerm(true);
	}
}

void LLFloaterPreference::onAvatarImpostorsEnable()
{
	//refreshEnabledGraphics();
}

//static
void LLFloaterPreference::initDoNotDisturbResponse()
	{
		if (!gSavedPerAccountSettings.getBOOL("DoNotDisturbResponseChanged"))
		{
			//LLTrans::getString("DoNotDisturbModeResponseDefault") is used here for localization (EXT-5885)
			gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
		}
	}

//static 
void LLFloaterPreference::updateShowFavoritesCheckbox(bool val)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->getChild<LLUICtrl>("favorites_on_login_check")->setValue(val);
	}	
}

void LLFloaterPreference::setHardwareDefaults()
{
	LLFeatureManager::getInstance()->applyRecommendedSettings();

	gSavedSettings.setString("PresetGraphicActive", "");
	LLPresetsManager::getInstance()->triggerChangeSignal();

	child_list_t::const_iterator iter = mTabContainer->getChildList()->begin();
	child_list_t::const_iterator end = mTabContainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
		{
			panel->setHardwareDefaults();
		}
	}
}

void LLFloaterPreference::getControlNames(std::vector<std::string>& names)
{
	LLView* view = findChild<LLView>("display");
	//BD
	if (view)
	{
		std::list<LLView*> stack;
		stack.push_back(view);
		while (!stack.empty())
		{
			// Process view on top of the stack
			LLView* curview = stack.front();
			stack.pop_front();

			LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
			if (ctrl)
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					std::string control_name = control->getName();
					if (std::find(names.begin(), names.end(), control_name) == names.end())
					{
						names.push_back(control_name);
					}
				}
			}

			for (child_list_t::const_iterator iter = curview->getChildList()->begin();
				iter != curview->getChildList()->end(); ++iter)
			{
				stack.push_back(*iter);
			}
		}
	}
}

//virtual
void LLFloaterPreference::onClose(bool app_quitting)
{
	gSavedSettings.setS32("LastPrefTab", mTabContainer->getCurrentPanelIndex());

	//BD
	if (!app_quitting)
	{
		//BD - when closing this window, turn of visiblity control so that 
		//     next time preferences is opened we don't suspend voice
		if (gSavedSettings.getBOOL("ShowDeviceSettings"))
		{
			gSavedSettings.setBOOL("ShowDeviceSettings", FALSE);
			inputOutput();
		}
	}
}

//BD
// static
void LLFloaterPreference::onBtnOK()
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (canClose())
	{
		saveSettings();
		apply();

		//Conversation transcript and log path changed so reload conversations based on new location
		if (mPriorInstantMessageLogPath.length())
		{
			if (moveTranscriptsAndLog())
			{
				//When floaters are empty but have a chat history files, reload chat history into them
				LLFloaterIMSessionTab::reloadEmptyFloaters();
			}
			//Couldn't move files so restore the old path and show a notification
			else
			{
				gSavedPerAccountSettings.setString("InstantMessageLogPath", mPriorInstantMessageLogPath);
				LLNotificationsUtil::add("PreferenceChatPathChanged");
			}
			mPriorInstantMessageLogPath.clear();
		}

		LLUIColorTable::instance().saveUserSettings();
		gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);

		//Only save once logged in and loaded per account settings
		if (mGotPersonalInfo)
		{
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		}

		//BD
		closeFloater(false);
	}
	else
	{
		// Show beep, pop up dialog, etc.
		LL_INFOS() << "Can't close preferences!" << LL_ENDL;
	}

	//Need to reload the navmesh if the pathing console is up
	LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
	if (!pathfindingConsoleHandle.isDead())
	{
		LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
		pPathfindingConsole->onRegionBoundaryCross();
	}

}

//BD
// static 
void LLFloaterPreference::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
		refresh();
	}
	cancel();
	//BD
	closeFloater();
}

// static 
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email, bool is_verified_email)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
        instance->setPersonalInfo(visibility, im_via_email, is_verified_email);
	}
}

void LLFloaterPreference::refreshEnabledGraphics()
{
	//BD - If we detect an Intel GPU, display a warning that this will negatively impact
	//     performance and not all features might be usable depending on the GPU.
	bool is_good_gpu = (gGLManager.mIsNVIDIA || gGLManager.mIsATI);

	getChild<LLUICtrl>("warning_multi_panel")->setVisible(!is_good_gpu);
}

void LLFloaterPreference::onClickClearCache()
{
	LLNotificationsUtil::add("ConfirmClearCache", LLSD(), LLSD(), callback_clear_cache);
}

void LLFloaterPreference::onClickBrowserClearCache()
{
	LLNotificationsUtil::add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
}

// Called when user changes language via the combobox.
void LLFloaterPreference::onLanguageChange()
{
	// Let the user know that the change will only take effect after restart.
	// Do it only once so that we're not too irritating.
	if (!mLanguageChanged)
	{
		LLNotificationsUtil::add("ChangeLanguage");
		mLanguageChanged = true;
	}
}

void LLFloaterPreference::onNotificationsChange(const std::string& OptionName)
{
	//BD
	mNotificationOptions[OptionName] = getChild<LLRadioGroup>(OptionName)->getSelectedValue();

	bool show_notifications_alert = true;
	for (notifications_map::iterator it_notification = mNotificationOptions.begin(); it_notification != mNotificationOptions.end(); it_notification++)
	{
		if(it_notification->second != "No action")
		{
			show_notifications_alert = false;
			break;
		}
	}

	//BD - We might want to add this later again.
	//getChild<LLTextBox>("notifications_alert")->setVisible(show_notifications_alert);
}

void LLFloaterPreference::onNameTagOpacityChange(const LLSD& newvalue)
{
	LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>("background");
	if (color_swatch)
	{
		LLColor4 new_color = color_swatch->get();
		color_swatch->set( new_color.setAlpha(newvalue.asReal()) );
	}
}

void LLFloaterPreference::onClickSetCache()
{
	std::string cur_name(gSavedSettings.getString("CacheLocation"));
	
	std::string proposed_name(cur_name);

	(new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeCachePath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name)
{
	std::string dir_name = filenames[0];
	if (!dir_name.empty() && dir_name != proposed_name)
	{
		std::string new_top_folder(gDirUtilp->getBaseFileName(dir_name));
		LLNotificationsUtil::add("CacheWillBeMoved");
		gSavedSettings.setString("NewCacheLocation", dir_name);
		gSavedSettings.setString("NewCacheLocationTopFolder", new_top_folder);
	}
	else
	{
		std::string cache_location = gDirUtilp->getCacheDir();
		gSavedSettings.setString("CacheLocation", cache_location);
		std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
		gSavedSettings.setString("CacheLocationTopFolder", top_folder);
	}
}

void LLFloaterPreference::onClickResetCache()
{
	if (gDirUtilp->getCacheDir(false) == gDirUtilp->getCacheDir(true))
	{
		// The cache location was already the default.
		return;
	}
	gSavedSettings.setString("NewCacheLocation", "");
	gSavedSettings.setString("NewCacheLocationTopFolder", "");
	LLNotificationsUtil::add("CacheWillBeMoved");
	std::string cache_location = gDirUtilp->getCacheDir(false);
	gSavedSettings.setString("CacheLocation", cache_location);
	std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
	gSavedSettings.setString("CacheLocationTopFolder", top_folder);
}

void LLFloaterPreference::onClickSkin(LLUICtrl* ctrl, const LLSD& userdata)
{
	gSavedSettings.setString("SkinCurrent", userdata.asString());
	ctrl->setValue(userdata.asString());
}

void LLFloaterPreference::onSelectSkin()
{
	std::string skin_selection = getChild<LLRadioGroup>("skin_selection")->getValue().asString();
	gSavedSettings.setString("SkinCurrent", skin_selection);
}

void LLFloaterPreference::refreshSkin(void* data)
{
	LLPanel*self = (LLPanel*)data;
	sSkin = gSavedSettings.getString("SkinCurrent");
	self->getChild<LLRadioGroup>("skin_selection", true)->setValue(sSkin);
}

void LLFloaterPreference::buildPopupLists()
{
	LLScrollListCtrl& disabled_popups =
		getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups =
		getChildRef<LLScrollListCtrl>("enabled_popups");
	
	disabled_popups.deleteAllItems();
	enabled_popups.deleteAllItems();
	
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		LLNotificationTemplatePtr templatep = iter->second;
		LLNotificationFormPtr formp = templatep->mForm;
		
		LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
		if (ignore == LLNotificationForm::IGNORE_NO)
			continue;
		
		LLSD row;
		row["columns"][0]["value"] = formp->getIgnoreMessage();
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		row["columns"][0]["width"] = 400;
		
		LLScrollListItem* item = NULL;
		
		bool show_popup = !formp->getIgnored();
		if (!show_popup)
		{
			if (ignore == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
			{
				LLSD last_response = LLUI::sSettingGroups["config"]->getLLSD("Default" + templatep->mName);
				if (!last_response.isUndefined())
				{
					for (LLSD::map_const_iterator it = last_response.beginMap();
						 it != last_response.endMap();
						 ++it)
					{
						if (it->second.asBoolean())
						{
							row["columns"][1]["value"] = formp->getElement(it->first)["ignore"].asString();
							break;
						}
					}
				}
				row["columns"][1]["font"] = "SANSSERIF_SMALL";
				row["columns"][1]["width"] = 360;
			}
			item = disabled_popups.addElement(row);
		}
		else
		{
			item = enabled_popups.addElement(row);
		}
		
		if (item)
		{
			item->setUserdata((void*)&iter->first);
		}
	}
}

void LLFloaterPreference::refreshEnabledState()
{

	// Cannot have floater active until caps have been received
	getChild<LLButton>("default_creation_permissions")->setEnabled(LLStartUp::getStartupState() < STATE_STARTED ? false : true);

// [RLVa:KB] - Checked: 2013-05-11 (RLVa-1.4.9)
	if (rlv_handler_t::isEnabled())
	{
		getChild<LLUICtrl>("do_not_disturb_response")->setEnabled(!RlvActions::hasBehaviour(RLV_BHVR_SENDIM));
	}
// [/RLVa:KB]

	getChildView("block_list")->setEnabled(LLLoginInstance::getInstance()->authSuccess());
}

void LLFloaterPreference::refresh()
{
	LLPanel::refresh();
}

void LLFloaterPreference::onClickSetKey()
{
	LLVoiceSetKeyDialog* dialog = LLFloaterReg::showTypedInstance<LLVoiceSetKeyDialog>("voice_set_key", LLSD(), TRUE);
	if (dialog)
	{
		dialog->setParent(this);
	}
}

void LLFloaterPreference::setKey(KEY key)
{
	getChild<LLUICtrl>("modifier_combo")->setValue(LLKeyboard::stringFromKey(key));
	// update the control right away since we no longer wait for apply
	getChild<LLUICtrl>("modifier_combo")->onCommit();
}

void LLFloaterPreference::onClickSetMiddleMouse()
{
	LLUICtrl* p2t_line_editor = getChild<LLUICtrl>("modifier_combo");

	// update the control right away since we no longer wait for apply
	p2t_line_editor->setControlValue(MIDDLE_MOUSE_CV);

	//push2talk button "middle mouse" control value is in English, need to localize it for presentation
	LLPanel* advanced_preferences = dynamic_cast<LLPanel*>(p2t_line_editor->getParent());
	if (advanced_preferences)
	{
		p2t_line_editor->setValue(advanced_preferences->getString("middle_mouse"));
	}
}

//BD
void LLFloaterPreference::onClickSetNone()
{
	LLUICtrl* p2t_line_editor = getChild<LLUICtrl>("modifier_combo");

	// update the control right away since we no longer wait for apply
	p2t_line_editor->setControlValue("");

	//push2talk control value is in English, need to localize it for presentation
	LLPanel* advanced_preferences = dynamic_cast<LLPanel*>(p2t_line_editor->getParent());
	if (advanced_preferences)
	{
		p2t_line_editor->setValue(advanced_preferences->getString("none"));
	}
}

void LLFloaterPreference::onClickSetSounds()
{
	// Disable Enable gesture sounds checkbox if the master sound is disabled 
	// or if sound effects are disabled.
	getChild<LLCheckBoxCtrl>("gesture_audio_play_btn")->setEnabled(!gSavedSettings.getBOOL("MuteSounds"));
}

/*
void LLFloaterPreference::onClickSkipDialogs()
{
	LLNotificationsUtil::add("SkipShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_skip_dialogs, _1, _2, this));
}

void LLFloaterPreference::onClickResetDialogs()
{
	LLNotificationsUtil::add("ResetShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_reset_dialogs, _1, _2, this));
}
 */

void LLFloaterPreference::onClickEnablePopup()
{	
	LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");
	
	std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		std::string notification_name = templatep->mName;
		LLUI::sSettingGroups["ignores"]->setBOOL(notification_name, TRUE);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::onClickDisablePopup()
{	
	LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");
	
	std::vector<LLScrollListItem*> items = enabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		templatep->mForm->setIgnored(true);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::resetAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(false);
		}
	}
}

void LLFloaterPreference::setAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(true);
		}
	}
}

void LLFloaterPreference::onClickLogPath()
{
	std::string proposed_name(gSavedPerAccountSettings.getString("InstantMessageLogPath"));	 
	mPriorInstantMessageLogPath.clear();
	

	(new LLDirPickerThread(boost::bind(&LLFloaterPreference::changeLogPath, this, _1, _2), proposed_name))->getFile();
}

void LLFloaterPreference::changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name)
{
	//Path changed
	if (proposed_name != filenames[0])
	{
		gSavedPerAccountSettings.setString("InstantMessageLogPath", filenames[0]);
		mPriorInstantMessageLogPath = proposed_name;

		// enable/disable 'Delete transcripts button
		updateDeleteTranscriptsButton();
	}
}

bool LLFloaterPreference::moveTranscriptsAndLog()
{
	std::string instantMessageLogPath(gSavedPerAccountSettings.getString("InstantMessageLogPath"));
	std::string chatLogPath = gDirUtilp->add(instantMessageLogPath, gDirUtilp->getUserName());

	bool madeDirectory = false;

	//Does the directory really exist, if not then make it
	if(!LLFile::isdir(chatLogPath))
	{
		//mkdir success is defined as zero
		if(LLFile::mkdir(chatLogPath) != 0)
		{
			return false;
		}
		madeDirectory = true;
	}
	
	std::string originalConversationLogDir = LLConversationLog::instance().getFileName();
	std::string targetConversationLogDir = gDirUtilp->add(chatLogPath, "conversation.log");
	//Try to move the conversation log
	if(!LLConversationLog::instance().moveLog(originalConversationLogDir, targetConversationLogDir))
	{
		//Couldn't move the log and created a new directory so remove the new directory
		if(madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}
		return false;
	}

	//Attempt to move transcripts
	std::vector<std::string> listOfTranscripts;
	std::vector<std::string> listOfFilesMoved;

	LLLogChat::getListOfTranscriptFiles(listOfTranscripts);

	if(!LLLogChat::moveTranscripts(gDirUtilp->getChatLogsDir(), 
									instantMessageLogPath, 
									listOfTranscripts,
									listOfFilesMoved))
	{
		//Couldn't move all the transcripts so restore those that moved back to their old location
		LLLogChat::moveTranscripts(instantMessageLogPath, 
			gDirUtilp->getChatLogsDir(), 
			listOfFilesMoved);

		//Move the conversation log back
		LLConversationLog::instance().moveLog(targetConversationLogDir, originalConversationLogDir);

		if(madeDirectory)
		{
			LLFile::rmdir(chatLogPath);
		}

		return false;
	}

	gDirUtilp->setChatLogsDir(instantMessageLogPath);
	gDirUtilp->updatePerAccountChatLogsDir();

	return true;
}

void LLFloaterPreference::setPersonalInfo(const std::string& visibility, bool im_via_email, bool is_verified_email)
{
	mGotPersonalInfo = true;
	mOriginalIMViaEmail = im_via_email;
	mDirectoryVisibility = visibility;
	
	if (visibility == VISIBILITY_DEFAULT)
	{
		mOriginalHideOnlineStatus = false;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else if (visibility == VISIBILITY_HIDDEN)
	{
		mOriginalHideOnlineStatus = true;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else
	{
		mOriginalHideOnlineStatus = true;
	}
	
	getChild<LLUICtrl>("online_searchresults")->setEnabled(TRUE);
	getChildView("friends_online_notify_checkbox")->setEnabled(TRUE);
	getChild<LLUICtrl>("online_visibility")->setValue(mOriginalHideOnlineStatus); 	 
	getChild<LLUICtrl>("online_visibility")->setLabelArg("[DIR_VIS]", mDirectoryVisibility);
	getChildView("send_im_to_email")->setEnabled(is_verified_email);

    std::string tooltip;
    if (!is_verified_email)
        tooltip = getString("email_unverified_tooltip");

    getChildView("send_im_to_email")->setToolTip(tooltip);

    // *TODO: Show or hide verify email text here based on is_verified_email
    getChild<LLUICtrl>("send_im_to_email")->setValue(im_via_email);
	getChildView("favorites_on_login_check")->setEnabled(TRUE);
	getChildView("log_path_button")->setEnabled(TRUE);
	getChildView("chat_font_size")->setEnabled(TRUE);
	getChildView("conversation_log_radio")->setEnabled(TRUE);
}


void LLFloaterPreference::refreshUI()
{
	refresh();
}

bool LLFloaterPreference::loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map)
{
    LLXMLNodePtr root;

    if (!LLXMLNode::parseFile(filename, root, NULL))
    {
        LL_WARNS() << "Unable to parse file " << filename << LL_ENDL;
        return false;
    }

    if (!root->hasName("labels"))
    {
        LL_WARNS() << filename << " is not a valid definition file" << LL_ENDL;
        return false;
    }

    LabelTable params;
    LLXUIParser parser;
    parser.readXUI(root, params, filename);

    if (params.validateBlock())
    {
        for (LLInitParam::ParamIterator<LabelDef>::const_iterator it = params.labels.begin();
            it != params.labels.end();
            ++it)
        {
            LabelDef label_entry = *it;
            label_map[label_entry.name] = label_entry.value;
        }
    }
    else
    {
        LL_WARNS() << filename << " failed to load" << LL_ENDL;
        return false;
    }

    return true;
}

std::string get_category_path(LLUUID cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (cat->getParentUUID().notNull())
	{
		return get_category_path(cat->getParentUUID()) + " > " + cat->getName();
	}
	else
	{
		return cat->getName();
	}
}

std::string get_category_path(LLFolderType::EType cat_type)
{
	LLUUID cat_id = gInventory.findUserDefinedCategoryUUIDForType(cat_type);
	return get_category_path(cat_id);
}

void LLFloaterPreference::onChangeModelFolder()
{
	if (gInventory.isInventoryUsable())
	{
		getChild<LLLineEditor>("upload_models")->setText(get_category_path(LLFolderType::FT_OBJECT));
	}
}

void LLFloaterPreference::onChangeTextureFolder()
{
	if (gInventory.isInventoryUsable())
	{
		getChild<LLLineEditor>("upload_textures")->setText(get_category_path(LLFolderType::FT_TEXTURE));
	}
}

void LLFloaterPreference::onChangeSoundFolder()
{
	if (gInventory.isInventoryUsable())
	{
		getChild<LLLineEditor>("upload_sounds")->setText(get_category_path(LLFolderType::FT_SOUND));
	}
}

void LLFloaterPreference::onChangeAnimationFolder()
{
	if (gInventory.isInventoryUsable())
	{
		getChild<LLLineEditor>("upload_animation")->setText(get_category_path(LLFolderType::FT_ANIMATION));
	}
}

// FIXME: this will stop you from spawning the sidetray from preferences dialog on login screen
// but the UI for this will still be enabled
void LLFloaterPreference::onClickBlockList()
{
	LLFloaterSidePanelContainer::showPanel("people", "panel_people",
		LLSD().with("people_panel_tab_name", "blocked_panel"));
}

void LLFloaterPreference::onClickProxySettings()
{
	LLFloaterReg::showInstance("prefs_proxy");
}

void LLFloaterPreference::onClickTranslationSettings()
{
	LLFloaterReg::showInstance("prefs_translation");
}

void LLFloaterPreference::onClickAutoReplace()
{
	LLFloaterReg::showInstance("prefs_autoreplace");
}

void LLFloaterPreference::onClickSpellChecker()
{
    LLFloaterReg::showInstance("prefs_spellchecker");
}

void LLFloaterPreference::onClickPermsDefault()
{
	LLFloaterReg::showInstance("perms_default");
}

void LLFloaterPreference::onDeleteTranscripts()
{
	LLSD args;
	args["FOLDER"] = gDirUtilp->getUserName();

	LLNotificationsUtil::add("PreferenceChatDeleteTranscripts", args, LLSD(), boost::bind(&LLFloaterPreference::onDeleteTranscriptsResponse, this, _1, _2));
}

void LLFloaterPreference::onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response)
{
	if (0 == LLNotificationsUtil::getSelectedOption(notification, response))
	{
		LLLogChat::deleteTranscripts();
		updateDeleteTranscriptsButton();
	}
}

void LLFloaterPreference::onLogChatHistorySaved()
{
	LLButton * delete_transcripts_buttonp = getChild<LLButton>("delete_transcripts");

	if (!delete_transcripts_buttonp->getEnabled())
	{
		delete_transcripts_buttonp->setEnabled(true);
	}
}

void LLFloaterPreference::applyUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLUIColorTable::instance().setColor(param.asString(), LLColor4(ctrl->getValue()));
}

void LLFloaterPreference::getUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLColorSwatchCtrl* color_swatch = (LLColorSwatchCtrl*) ctrl;
	color_swatch->setOriginal(LLUIColorTable::instance().getColor(param.asString()));
}

void LLFloaterPreference::setCacheLocation(const LLStringExplicit& location)
{
	LLUICtrl* cache_location_editor = getChild<LLUICtrl>("cache_location");
	cache_location_editor->setValue(location);
	cache_location_editor->setToolTip(location);
}

void LLFloaterPreference::selectPanel(const LLSD& name)
{
	LLPanel * panel = mTabContainer->getPanelByName(name);
	if (NULL != panel)
	{
		mTabContainer->selectTabPanel(panel);
	}
}

void LLFloaterPreference::selectPrivacyPanel()
{
	selectPanel("im");
}

void LLFloaterPreference::selectChatPanel()
{
	selectPanel("chat");
}

void LLFloaterPreference::changed()
{
	getChild<LLButton>("clear_log")->setEnabled(LLConversationLog::instance().getConversations().size() > 0);

	// set 'enable' property for 'Delete transcripts...' button
	updateDeleteTranscriptsButton();

}

//BD - Quick Graphics Presets
void LLFloaterPreference::deletePreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	std::string name = combo->getSimple();

	if (!LLPresetsManager::getInstance()->deletePreset(subdirectory, name)
		&& name != "Default")
	{
		LLSD args;
		args["NAME"] = name;
		LLNotificationsUtil::add("PresetNotDeleted", args);
	}
}

void LLFloaterPreference::savePreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	std::string name = combo->getSimple();

	if (!LLPresetsManager::getInstance()->savePreset(subdirectory, name))
	{
		LLSD args;
		args["NAME"] = name;
		LLNotificationsUtil::add("PresetNotSaved", args);
	}
	//BD - We should probably show some sort of notification regardless to inform
	//     the user that he successfully saved a preset.
}

void LLFloaterPreference::loadPreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	std::string name = combo->getSimple();

	LLPresetsManager::getInstance()->loadPreset(subdirectory, name);
	toggleTabs();
	refreshGraphicControls();
	//BD - Machinima Sidebar.
	//     Refresh it's controls too if its open.
	if (gSideBar && gSideBar->isInVisibleChain())
	{
		gSideBar->refreshGraphicControls();
	}
}

void LLFloaterPreference::onPresetsListChange()
{
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	LLButton* delete_btn = getChild<LLButton>("delete");

	EDefaultOptions option = DEFAULT_TOP;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox("graphic", combo, option);

	delete_btn->setEnabled(0 != combo->getItemCount());
}

/*void LLFloaterPreference::saveGraphicsPreset(std::string& preset)
{
	mSavedGraphicsPreset = preset;
}*/

//------------------------------Updater---------------------------------------

static bool handleBandwidthChanged(const LLSD& newvalue)
{
	gViewerThrottle.setMaxBandwidth((F32) newvalue.asReal());
	return true;
}

class LLPanelPreference::Updater : public LLEventTimer
{

public:

	typedef boost::function<bool(const LLSD&)> callback_t;

	Updater(callback_t cb, F32 period)
	:LLEventTimer(period),
	 mCallback(cb)
	{
		mEventTimer.stop();
	}

	virtual ~Updater(){}

	void update(const LLSD& new_value)
	{
		mNewValue = new_value;
		mEventTimer.start();
	}

protected:

	BOOL tick()
	{
		mCallback(mNewValue);
		mEventTimer.stop();

		return FALSE;
	}

private:

	LLSD mNewValue;
	callback_t mCallback;
};
//----------------------------------------------------------------------------
static LLPanelInjector<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel(),
  mBandWidthUpdater(NULL)
{
	mCommitCallbackRegistrar.add("Pref.setControlFalse",	boost::bind(&LLPanelPreference::setControlFalse,this, _2));
	mCommitCallbackRegistrar.add("Pref.updateMediaAutoPlayCheckbox",	boost::bind(&LLPanelPreference::updateMediaAutoPlayCheckbox, this, _1));
}

//virtual
BOOL LLPanelPreference::postBuild()
{
	////////////////////// PanelGeneral ///////////////////
	if (hasChild("display_names_check", TRUE))
	{
		BOOL use_people_api = gSavedSettings.getBOOL("UsePeopleAPI");
		LLCheckBoxCtrl* ctrl_display_name = getChild<LLCheckBoxCtrl>("display_names_check");
		ctrl_display_name->setEnabled(use_people_api);
		if (!use_people_api)
		{
			ctrl_display_name->setValue(FALSE);
		}
	}

	////////////////////// PanelVoice ///////////////////
	if (hasChild("voice_unavailable", TRUE))
	{
		BOOL voice_disabled = gSavedSettings.getBOOL("CmdLineDisableVoice");
		getChildView("voice_unavailable")->setVisible( voice_disabled);
		getChildView("enable_voice_check")->setVisible( !voice_disabled);
	}
	
	//////////////////////PanelSkins ///////////////////
	
	if (hasChild("skin_selection", TRUE))
	{
		LLFloaterPreference::refreshSkin(this);

		// if skin is set to a skin that no longer exists (silver) set back to default
		if (getChild<LLRadioGroup>("skin_selection")->getSelectedIndex() < 0)
		{
			gSavedSettings.setString("SkinCurrent", "default");
			LLFloaterPreference::refreshSkin(this);
		}

	}

	//////////////////////PanelPrivacy ///////////////////
	if (hasChild("media_enabled", TRUE))
	{
		bool media_enabled = gSavedSettings.getBOOL("AudioStreamingMedia");
		
		getChild<LLCheckBoxCtrl>("media_enabled")->set(media_enabled);
		getChild<LLCheckBoxCtrl>("autoplay_enabled")->setEnabled(media_enabled);
	}
	if (hasChild("music_enabled", TRUE))
	{
		getChild<LLCheckBoxCtrl>("music_enabled")->set(gSavedSettings.getBOOL("AudioStreamingMusic"));
	}
	if (hasChild("voice_call_friends_only_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("voice_call_friends_only_check")->setCommitCallback(boost::bind(&showFriendsOnlyWarning, _1, _2));
	}
	if (hasChild("allow_multiple_viewer_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("allow_multiple_viewer_check")->setCommitCallback(boost::bind(&showMultipleViewersWarning, _1, _2));
	}
	if (hasChild("favorites_on_login_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setCommitCallback(boost::bind(&handleFavoritesOnLoginChanged, _1, _2));
	}
	if (hasChild("mute_chb_label", TRUE))
	{
		getChild<LLTextBox>("mute_chb_label")->setShowCursorHand(false);
		getChild<LLTextBox>("mute_chb_label")->setSoundFlags(LLView::MOUSE_UP);
		getChild<LLTextBox>("mute_chb_label")->setClickedCallback(boost::bind(&toggleMuteWhenMinimized));
	}

	//////////////////////PanelAdvanced ///////////////////
	if (hasChild("modifier_combo", TRUE))
	{
		//localizing if push2talk button is set to middle mouse
		if (MIDDLE_MOUSE_CV == getChild<LLUICtrl>("modifier_combo")->getValue().asString())
		{
			getChild<LLUICtrl>("modifier_combo")->setValue(getString("middle_mouse"));
		}
	}

	//////////////////////PanelSetup ///////////////////
	if (hasChild("max_bandwidth"), TRUE)
	{
		mBandWidthUpdater = new LLPanelPreference::Updater(boost::bind(&handleBandwidthChanged, _1), BANDWIDTH_UPDATER_TIMEOUT);
		gSavedSettings.getControl("ThrottleBandwidthKBPS")->getSignal()->connect(boost::bind(&LLPanelPreference::Updater::update, mBandWidthUpdater, _2));
	}

//	//BD - Quick Graphics Presets
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");

	EDefaultOptions option = DEFAULT_TOP;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox("graphic", combo, option);

#ifdef EXTERNAL_TOS
	LLRadioGroup* ext_browser_settings = getChild<LLRadioGroup>("preferred_browser_behavior");
	if (ext_browser_settings)
	{
		// turn off ability to set external/internal browser
		ext_browser_settings->setSelectedByValue(LLWeb::BROWSER_EXTERNAL_ONLY, true);
		ext_browser_settings->setEnabled(false);
	}
#endif

	apply();
	return true;
}

LLPanelPreference::~LLPanelPreference()
{
	if (mBandWidthUpdater)
	{
		delete mBandWidthUpdater;
	}
}
void LLPanelPreference::apply()
{
	// no-op
}

void LLPanelPreference::saveSettings()
{
	// Save the value of all controls in the hierarchy
	mSavedValues.clear();
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLColorSwatchCtrl* color_swatch = dynamic_cast<LLColorSwatchCtrl *>(curview);
		if (color_swatch)
		{
			mSavedColors[color_swatch->getName()] = color_swatch->get();
		}
		else
		{
			LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
			if (ctrl)
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					mSavedValues[control] = control->getValue();
				}
			}
		}
			
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}

void LLPanelPreference::showMultipleViewersWarning(LLUICtrl* checkbox, const LLSD& value)
{
    if (checkbox && checkbox->getValue())
    {
        LLNotificationsUtil::add("AllowMultipleViewers");
    }
}

void LLPanelPreference::showFriendsOnlyWarning(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox && checkbox->getValue())
	{
		LLNotificationsUtil::add("FriendsAndGroupsOnly");
	}
}

void LLPanelPreference::handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox)
	{
		LLFavoritesOrderStorage::instance().showFavoritesOnLoginChanged(checkbox->getValue().asBoolean());
		if(checkbox->getValue())
		{
			LLNotificationsUtil::add("FavoritesOnLogin");
		}
	}
}

void LLPanelPreference::toggleMuteWhenMinimized()
{
	std::string mute("MuteWhenMinimized");
	gSavedSettings.setBOOL(mute, !gSavedSettings.getBOOL(mute));
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->getChild<LLCheckBoxCtrl>("mute_when_minimized")->setBtnFocus();
	}
}

void LLPanelPreference::cancel()
{
	for (control_values_map_t::iterator iter =  mSavedValues.begin();
		 iter !=  mSavedValues.end(); ++iter)
	{
		LLControlVariable* control = iter->first;
		LLSD ctrl_value = iter->second;

		if((control->getName() == "InstantMessageLogPath") && (ctrl_value.asString() == ""))
		{
			continue;
		}

		control->set(ctrl_value);
	}

	for (string_color_map_t::iterator iter = mSavedColors.begin();
		 iter != mSavedColors.end(); ++iter)
	{
		LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>(iter->first);
		if (color_swatch)
		{
			color_swatch->set(iter->second);
			color_swatch->onCommit();
		}
	}
}

void LLPanelPreference::setControlFalse(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	LLControlVariable* control = findControl(control_name);
	
	if (control)
		control->set(LLSD(FALSE));
}

void LLPanelPreference::updateMediaAutoPlayCheckbox(LLUICtrl* ctrl)
{
	std::string name = ctrl->getName();

	// Disable "Allow Media to auto play" only when both
	// "Streaming Music" and "Media" are unchecked. STORM-513.
	if ((name == "enable_music") || (name == "enable_media"))
	{
		bool music_enabled = getChild<LLCheckBoxCtrl>("enable_music")->get();
		bool media_enabled = getChild<LLCheckBoxCtrl>("enable_media")->get();

		getChild<LLCheckBoxCtrl>("media_auto_play_btn")->setEnabled(music_enabled || media_enabled);
	}
}

void LLPanelPreference::setHardwareDefaults()
{
}

class LLPanelPreferencePrivacy : public LLPanelPreference
{
public:
	LLPanelPreferencePrivacy()
	{
		mAccountIndependentSettings.push_back("VoiceCallsFriendsOnly");
		mAccountIndependentSettings.push_back("AutoDisengageMic");
	}

	/*virtual*/ void saveSettings()
	{
		LLPanelPreference::saveSettings();

		// Don't save (=erase from the saved values map) per-account privacy settings
		// if we're not logged in, otherwise they will be reset to defaults on log off.
		if (LLStartUp::getStartupState() != STATE_STARTED)
		{
			// Erase only common settings, assuming there are no color settings on Privacy page.
			for (control_values_map_t::iterator it = mSavedValues.begin(); it != mSavedValues.end(); )
			{
				const std::string setting = it->first->getName();
				if (find(mAccountIndependentSettings.begin(),
					mAccountIndependentSettings.end(), setting) == mAccountIndependentSettings.end())
				{
					mSavedValues.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}
	}

private:
	std::list<std::string> mAccountIndependentSettings;
};

static LLPanelInjector<LLPanelPreferenceGraphics> t_pref_graph("panel_preference_graphics");
static LLPanelInjector<LLPanelPreferencePrivacy> t_pref_privacy("panel_preference_privacy");

BOOL LLPanelPreferenceGraphics::postBuild()
{
	resetDirtyChilds();
	setPresetText();

	LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
    presetsMgr->setPresetListChangeCallback(boost::bind(&LLPanelPreferenceGraphics::onPresetsListChange, this));
    presetsMgr->createMissingDefault(); // a no-op after the first time, but that's ok
    
	return LLPanelPreference::postBuild();
}

void LLPanelPreferenceGraphics::draw()
{
	setPresetText();
	LLPanelPreference::draw();
}

void LLPanelPreferenceGraphics::onPresetsListChange()
{
	resetDirtyChilds();
	setPresetText();

	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance && !gSavedSettings.getString("PresetGraphicActive").empty())
	{
		instance->saveSettings(); //make cancel work correctly after changing the preset
	}
	else
	{
		std::string dummy;
		//instance->saveGraphicsPreset(dummy);
	}

}

void LLPanelPreferenceGraphics::setPresetText()
{
//	//BD - Quick Graphics Presets
	//LLTextBox* preset_text = getChild<LLTextBox>("preset_text");

	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");

	/*if (!preset_graphic_active.empty() && preset_graphic_active != preset_text->getText())
	{
		LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
		if (instance)
		{
			instance->saveGraphicsPreset(preset_graphic_active);
		}
	}*/

	if (hasDirtyChilds() && !preset_graphic_active.empty())
	{
		gSavedSettings.setString("PresetGraphicActive", "");
		preset_graphic_active.clear();
		// This doesn't seem to cause an infinite recursion.  This trigger is needed to cause the pulldown
		// panel to update.
		LLPresetsManager::getInstance()->triggerChangeSignal();
	}

//	//BD - Quick Graphics Presets
	/*if (!preset_graphic_active.empty())
	{
		if (preset_graphic_active == PRESETS_DEFAULT)
		{
			preset_graphic_active = LLTrans::getString(PRESETS_DEFAULT);
		}
		preset_text->setText(preset_graphic_active);
	}
	else
	{
		preset_text->setText(LLTrans::getString("none_paren_cap"));
	}

	preset_text->resetDirty();*/
}

bool LLPanelPreferenceGraphics::hasDirtyChilds()
{
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			if (ctrl->isDirty())
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					std::string control_name = control->getName();
					if (!control_name.empty())
					{
						return true;
					}
				}
			}
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}

	return false;
}

void LLPanelPreferenceGraphics::resetDirtyChilds()
{
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			ctrl->resetDirty();
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}
void LLPanelPreferenceGraphics::cancel()
{
	//BD
	resetDirtyChilds();
	LLPanelPreference::cancel();
}
void LLPanelPreferenceGraphics::saveSettings()
{
	resetDirtyChilds();
	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
	if (preset_graphic_active.empty())
	{
		LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
		if (instance)
		{
			//don't restore previous preset after closing Preferences
			//instance->saveGraphicsPreset(preset_graphic_active);
		}
	}
	LLPanelPreference::saveSettings();
}
void LLPanelPreferenceGraphics::setHardwareDefaults()
{
	resetDirtyChilds();
	//BD
	LLPanelPreference::setHardwareDefaults();
}

//BD - Avatar Render Settings
void LLFloaterPreference::onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
	LLNameListCtrl* list = dynamic_cast<LLNameListCtrl*>(ctrl);
	if (!list) return;
	list->selectItemAt(x, y, MASK_NONE);
	uuid_vec_t selected_uuids;

	if (list->getCurrentID().notNull())
	{
		selected_uuids.push_back(list->getCurrentID());
		mContextMenu->show(ctrl, selected_uuids, x, y);
	}
}

void LLFloaterPreference::updateList()
{
	mAvatarSettingsList->deleteAllItems();
	LLAvatarName av_name;
	LLNameListCtrl::NameItem item_params;
	for (std::map<LLUUID, S32>::iterator iter = LLRenderMuteList::getInstance()->sVisuallyMuteSettingsMap.begin(); iter != LLRenderMuteList::getInstance()->sVisuallyMuteSettingsMap.end(); iter++)
	{
		item_params.value = iter->first;
		LLAvatarNameCache::get(iter->first, &av_name);
		if (!isHiddenRow(av_name.getCompleteName()))
		{
			item_params.columns.add().value(av_name.getCompleteName()).column("name");
			std::string setting = getString(iter->second == 1 ? "av_never_render" : "av_always_render");
			item_params.columns.add().value(setting).column("setting");
			std::string timestamp = createTimestamp(LLRenderMuteList::getInstance()->getVisualMuteDate(iter->first));
			item_params.columns.add().value(timestamp).column("timestamp");
			mAvatarSettingsList->addNameItemRow(item_params);
		}
	}
}

void LLFloaterPreference::onFilterEdit(const std::string& search_string)
{
	std::string filter_upper = search_string;
	LLStringUtil::toUpper(filter_upper);
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;
		mNeedsUpdate = true;
	}
}

bool LLFloaterPreference::isHiddenRow(const std::string& av_name)
{
	if (mNameFilter.empty()) return false;
	std::string upper_name = av_name;
	LLStringUtil::toUpper(upper_name);
	return std::string::npos == upper_name.find(mNameFilter);
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
	LLViewerObject *obj = gObjectList.findObject(id);
	while (obj && obj->isAttachment())
	{
		obj = (LLViewerObject *)obj->getParent();
	}

	if (obj && obj->isAvatar())
	{
		return (LLVOAvatar*)obj;
	}
	else
	{
		return NULL;
	}
}


void LLFloaterPreference::onCustomAction(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();

	S32 new_setting = 0;
	if ("default" == command_name)
	{
		new_setting = S32(LLVOAvatar::AV_RENDER_NORMALLY);
	}
	else if ("never" == command_name)
	{
		new_setting = S32(LLVOAvatar::AV_DO_NOT_RENDER);
	}
	else if ("always" == command_name)
	{
		new_setting = S32(LLVOAvatar::AV_ALWAYS_RENDER);
	}

	setAvatarRenderSetting(getRenderSettingUUIDs(), new_setting);
}


bool LLFloaterPreference::isActionChecked(const LLSD& userdata, const LLUUID& av_id)
{
	const std::string command_name = userdata.asString();

	S32 visual_setting = LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(av_id);
	if ("default" == command_name)
	{
		return (visual_setting == S32(LLVOAvatar::AV_RENDER_NORMALLY));
	}
	else if ("never" == command_name)
	{
		return (visual_setting == S32(LLVOAvatar::AV_DO_NOT_RENDER));
	}
	else if ("always" == command_name)
	{
		return (visual_setting == S32(LLVOAvatar::AV_ALWAYS_RENDER));
	}
	return false;
}

void LLFloaterPreference::setNeedsUpdate()
{
	LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
	if (!instance) return;
	instance->mNeedsUpdate = true;
}

void LLFloaterPreference::onClickAdd(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	S32 visual_setting = 0;
	if ("never" == command_name)
	{
		visual_setting = S32(LLVOAvatar::AV_DO_NOT_RENDER);
	}
	else if ("always" == command_name)
	{
		visual_setting = S32(LLVOAvatar::AV_ALWAYS_RENDER);
	}

	LLView * button = findChild<LLButton>("plus_btn", TRUE);
	LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker * picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterPreference::callbackAvatarPicked, this, _1, visual_setting),
		FALSE, TRUE, FALSE, root_floater->getName(), button);

	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
}

uuid_vec_t LLFloaterPreference::getRenderSettingUUIDs()
{
	//BD - Allow mass changing.
	uuid_vec_t av_ids;
	std::vector<LLScrollListItem*> selected_items = mAvatarSettingsList->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = selected_items.begin();
		iter != selected_items.end(); ++iter)
	{
		LLScrollListItem* item = (*iter);
		if (item)
		{
			av_ids.push_back(item->getUUID());
		}
	}

	return av_ids;
}

void LLFloaterPreference::callbackAvatarPicked(const uuid_vec_t& ids, S32 visual_setting)
{
	if (ids.empty()) return;
	setAvatarRenderSetting(ids, visual_setting);
}

void LLFloaterPreference::setAvatarRenderSetting(const uuid_vec_t& av_ids, S32 new_setting)
{
	//BD - Allow mass changing.
	if (!av_ids.empty())
	{
		for (uuid_vec_t::const_iterator iter = av_ids.begin();
			iter != av_ids.end(); ++iter)
		{
			const LLUUID av_id = (*iter);
			LLVOAvatar *avatarp = find_avatar(av_id);
			if (avatarp)
			{
				avatarp->setVisualMuteSettings(LLVOAvatar::VisualMuteSettings(new_setting));
			}
			else
			{
				LLRenderMuteList::getInstance()->saveVisualMuteSetting(av_id, new_setting);
			}
		}
		//BD
		updateList();
	}
}

BOOL LLFloaterPreference::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;

	if (KEY_DELETE == key)
	{
		//BD - Allow mass changing.
		setAvatarRenderSetting(getRenderSettingUUIDs(), (S32)LLVOAvatar::AV_RENDER_NORMALLY);
		handled = TRUE;
	}
	return handled;
}

std::string LLFloaterPreference::createTimestamp(S32 datetime)
{
	std::string timeStr;
	LLSD substitution;
	substitution["datetime"] = datetime;

	timeStr = "[" + LLTrans::getString("TimeMonth") + "]/["
		+ LLTrans::getString("TimeDay") + "]/["
		+ LLTrans::getString("TimeYear") + "]";

	LLStringUtil::format(timeStr, substitution);
	return timeStr;
}


LLFloaterPreferenceProxy::LLFloaterPreferenceProxy(const LLSD& key)
	: LLFloater(key),
	  mSocksSettingsDirty(false)
{
	mCommitCallbackRegistrar.add("Proxy.OK",                boost::bind(&LLFloaterPreferenceProxy::onBtnOk, this));
	mCommitCallbackRegistrar.add("Proxy.Cancel",            boost::bind(&LLFloaterPreferenceProxy::onBtnCancel, this));
	mCommitCallbackRegistrar.add("Proxy.Change",            boost::bind(&LLFloaterPreferenceProxy::onChangeSocksSettings, this));
}

LLFloaterPreferenceProxy::~LLFloaterPreferenceProxy()
{
}

BOOL LLFloaterPreferenceProxy::postBuild()
{
	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (!socksAuth)
	{
		return FALSE;
	}
	if (socksAuth->getSelectedValue().asString() == "None")
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(false);
		getChild<LLLineEditor>("socks5_password")->setEnabled(false);
	}
	else
	{
		// Populate the SOCKS 5 credential fields with protected values.
		LLPointer<LLCredential> socks_cred = gSecAPIHandler->loadCredential("SOCKS5");
		getChild<LLLineEditor>("socks5_username")->setValue(socks_cred->getIdentifier()["username"].asString());
		getChild<LLLineEditor>("socks5_password")->setValue(socks_cred->getAuthenticator()["creds"].asString());
	}

	return TRUE;
}

void LLFloaterPreferenceProxy::onOpen(const LLSD& key)
{
	saveSettings();
}

void LLFloaterPreferenceProxy::onClose(bool app_quitting)
{
	if(app_quitting)
	{
		cancel();
	}

	if (mSocksSettingsDirty)
	{

		// If the user plays with the Socks proxy settings after login, it's only fair we let them know
		// it will not be updated until next restart.
		if (LLStartUp::getStartupState()>STATE_LOGIN_WAIT)
		{
			LLNotifications::instance().add("ChangeProxySettings", LLSD(), LLSD());
			mSocksSettingsDirty = false; // we have notified the user now be quiet again
		}
	}
}

void LLFloaterPreferenceProxy::saveSettings()
{
	// Save the value of all controls in the hierarchy
	mSavedValues.clear();
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			LLControlVariable* control = ctrl->getControlVariable();
			if (control)
			{
				mSavedValues[control] = control->getValue();
			}
		}

		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
				iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}
}

void LLFloaterPreferenceProxy::onBtnOk()
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	// Save SOCKS proxy credentials securely if password auth is enabled
	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (socksAuth->getSelectedValue().asString() == "UserPass")
	{
		LLSD socks_id = LLSD::emptyMap();
		socks_id["type"] = "SOCKS5";
		socks_id["username"] = getChild<LLLineEditor>("socks5_username")->getValue().asString();

		LLSD socks_authenticator = LLSD::emptyMap();
		socks_authenticator["type"] = "SOCKS5";
		socks_authenticator["creds"] = getChild<LLLineEditor>("socks5_password")->getValue().asString();

		// Using "SOCKS5" as the "grid" argument since the same proxy
		// settings will be used for all grids and because there is no
		// way to specify the type of credential.
		LLPointer<LLCredential> socks_cred = gSecAPIHandler->createCredential("SOCKS5", socks_id, socks_authenticator);
		gSecAPIHandler->saveCredential(socks_cred, true);
	}
	else
	{
		// Clear SOCKS5 credentials since they are no longer needed.
		LLPointer<LLCredential> socks_cred = new LLCredential("SOCKS5");
		gSecAPIHandler->deleteCredential(socks_cred);
	}

	closeFloater(false);
}

void LLFloaterPreferenceProxy::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
		refresh();
	}

	cancel();
}

void LLFloaterPreferenceProxy::onClickCloseBtn(bool app_quitting)
{
	cancel();
}

void LLFloaterPreferenceProxy::cancel()
{

	for (control_values_map_t::iterator iter =  mSavedValues.begin();
			iter !=  mSavedValues.end(); ++iter)
	{
		LLControlVariable* control = iter->first;
		LLSD ctrl_value = iter->second;
		control->set(ctrl_value);
	}
	mSocksSettingsDirty = false;
	closeFloater();
}

void LLFloaterPreferenceProxy::onChangeSocksSettings() 
{
	mSocksSettingsDirty = true;

	LLRadioGroup* socksAuth = getChild<LLRadioGroup>("socks5_auth_type");
	if (socksAuth->getSelectedValue().asString() == "None")
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(false);
		getChild<LLLineEditor>("socks5_password")->setEnabled(false);
	}
	else
	{
		getChild<LLLineEditor>("socks5_username")->setEnabled(true);
		getChild<LLLineEditor>("socks5_password")->setEnabled(true);
	}

	// Check for invalid states for the other HTTP proxy radio
	LLRadioGroup* otherHttpProxy = getChild<LLRadioGroup>("other_http_proxy_type");
	if ((otherHttpProxy->getSelectedValue().asString() == "Socks" &&
			getChild<LLCheckBoxCtrl>("socks_proxy_enabled")->get() == FALSE )||(
					otherHttpProxy->getSelectedValue().asString() == "Web" &&
					getChild<LLCheckBoxCtrl>("web_proxy_enabled")->get() == FALSE ) )
	{
		otherHttpProxy->selectFirstItem();
	}

}

//BD - Revert to Default
void LLFloaterPreference::resetToDefault(LLUICtrl* ctrl)
{
	if (gDragonLibrary.resetToDefault(ctrl))
	{
		refreshGraphicControls();
	}
}

void LLFloaterPreference::onUpdateFilterTerm(bool force)
{
	LLWString seachValue = utf8str_to_wstring( mFilterEdit->getValue() );
	LLWStringUtil::toLower( seachValue );

	if( !mSearchData || (mSearchData->mLastFilter == seachValue && !force))
		return;

	mSearchData->mLastFilter = seachValue;

	if( !mSearchData->mRootTab )
		return;

	mSearchData->mRootTab->hightlightAndHide( seachValue );
	LLTabContainer *pRoot = getChild< LLTabContainer >( "pref core" );
	if( pRoot )
		pRoot->selectFirstTab();
}

void collectChildren( LLView const *aView, ll::prefs::PanelDataPtr aParentPanel, ll::prefs::TabContainerDataPtr aParentTabContainer )
{
	if( !aView )
		return;

	llassert_always( aParentPanel || aParentTabContainer );

	LLView::child_list_const_iter_t itr = aView->beginChild();
	LLView::child_list_const_iter_t itrEnd = aView->endChild();

	while( itr != itrEnd )
	{
		LLView *pView = *itr;
		ll::prefs::PanelDataPtr pCurPanelData = aParentPanel;
		ll::prefs::TabContainerDataPtr pCurTabContainer = aParentTabContainer;
		if( !pView )
			continue;
		LLPanel const *pPanel = dynamic_cast< LLPanel const *>( pView );
		LLTabContainer const *pTabContainer = dynamic_cast< LLTabContainer const *>( pView );
		ll::ui::SearchableControl const *pSCtrl = dynamic_cast< ll::ui::SearchableControl const *>( pView );

		if( pTabContainer )
		{
			pCurPanelData.reset();

			pCurTabContainer = ll::prefs::TabContainerDataPtr( new ll::prefs::TabContainerData );
			pCurTabContainer->mTabContainer = const_cast< LLTabContainer *>( pTabContainer );
			pCurTabContainer->mLabel = pTabContainer->getLabel();
			pCurTabContainer->mPanel = 0;

			if( aParentPanel )
				aParentPanel->mChildPanel.push_back( pCurTabContainer );
			if( aParentTabContainer )
				aParentTabContainer->mChildPanel.push_back( pCurTabContainer );
		}
		else if( pPanel )
		{
			pCurTabContainer.reset();

			pCurPanelData = ll::prefs::PanelDataPtr( new ll::prefs::PanelData );
			pCurPanelData->mPanel = pPanel;
			pCurPanelData->mLabel = pPanel->getLabel();

			llassert_always( aParentPanel || aParentTabContainer );

			if( aParentTabContainer )
				aParentTabContainer->mChildPanel.push_back( pCurPanelData );
			else if( aParentPanel )
				aParentPanel->mChildPanel.push_back( pCurPanelData );
		}
		else if( pSCtrl && pSCtrl->getSearchText().size() )
		{
			ll::prefs::SearchableItemPtr item = ll::prefs::SearchableItemPtr( new ll::prefs::SearchableItem() );
			item->mView = pView;
			item->mCtrl = pSCtrl;

			item->mLabel = utf8str_to_wstring( pSCtrl->getSearchText() );
			LLWStringUtil::toLower( item->mLabel );

			llassert_always( aParentPanel || aParentTabContainer );

			if( aParentPanel )
				aParentPanel->mChildren.push_back( item );
			if( aParentTabContainer )
				aParentTabContainer->mChildren.push_back( item );
		}
		collectChildren( pView, pCurPanelData, pCurTabContainer );
		++itr;
	}
}

void LLFloaterPreference::collectSearchableItems()
{
	mSearchData.reset( nullptr );
	LLTabContainer *pRoot = getChild< LLTabContainer >( "pref core" );
	if( mFilterEdit && pRoot )
	{
		mSearchData.reset(new ll::prefs::SearchData() );

		ll::prefs::TabContainerDataPtr pRootTabcontainer = ll::prefs::TabContainerDataPtr( new ll::prefs::TabContainerData );
		pRootTabcontainer->mTabContainer = pRoot;
		pRootTabcontainer->mLabel = pRoot->getLabel();
		mSearchData->mRootTab = pRootTabcontainer;

		collectChildren( this, ll::prefs::PanelDataPtr(), pRootTabcontainer );
	}
}