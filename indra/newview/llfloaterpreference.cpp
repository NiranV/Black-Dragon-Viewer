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
#include "llviewerkeyboard.h"
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

#include "lllogininstance.h"        // to check if logged in yet
#include "llsdserialize.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"
#include "llpresetsmanager.h"
#include "llfeaturemanager.h"
#include "llviewertexturelist.h"


const F32 BANDWIDTH_UPDATER_TIMEOUT = 0.5f;
char const* const VISIBILITY_DEFAULT = "default";
char const* const VISIBILITY_HIDDEN = "hidden";

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
	LLSliderCtrl* volume_slider = getChild<LLSliderCtrl>("mic_volume_slider");
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

	getChild<LLSliderCtrl>("mic_volume_slider")->setEnabled(device_settings_available);

	if(!device_settings_available)
	{
		// The combo boxes are disabled, since we can't get the device settings from the daemon just now.
		// Put the currently set default (ONLY) in the box, and select it.
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(mInputDevice), mInputDevice, ADD_BOTTOM);
			mCtrlInputDevices->setValue(mInputDevice);
		}
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(mOutputDevice), mOutputDevice, ADD_BOTTOM);
			mCtrlOutputDevices->setValue(mOutputDevice);
		}
		mDevicesUpdated = FALSE;
	}
	else if (!mDevicesUpdated)
	{
		LLVoiceDeviceList::const_iterator iter;
		
		if(mCtrlInputDevices)
		{
			mCtrlInputDevices->removeall();
			mCtrlInputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for(iter=LLVoiceClient::getInstance()->getCaptureDevices().begin(); 
				iter != LLVoiceClient::getInstance()->getCaptureDevices().end();
				iter++)
			{
				mCtrlInputDevices->add(getLocalizedDeviceName(*iter), *iter, ADD_BOTTOM);
			}

			// Fix invalid input audio device preference.
			if (!mCtrlInputDevices->setSelectedByValue(mInputDevice, TRUE))
			{
				mCtrlInputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceInputAudioDevice", DEFAULT_DEVICE);
				mInputDevice = DEFAULT_DEVICE;
			}
		}
		
		if(mCtrlOutputDevices)
		{
			mCtrlOutputDevices->removeall();
			mCtrlOutputDevices->add(getLocalizedDeviceName(DEFAULT_DEVICE), DEFAULT_DEVICE, ADD_BOTTOM);

			for(iter= LLVoiceClient::getInstance()->getRenderDevices().begin(); 
				iter !=  LLVoiceClient::getInstance()->getRenderDevices().end(); iter++)
			{
				mCtrlOutputDevices->add(getLocalizedDeviceName(*iter), *iter, ADD_BOTTOM);
			}

			// Fix invalid output audio device preference.
			if (!mCtrlOutputDevices->setSelectedByValue(mOutputDevice, TRUE))
			{
				mCtrlOutputDevices->setValue(DEFAULT_DEVICE);
				gSavedSettings.setString("VoiceOutputAudioDevice", DEFAULT_DEVICE);
				mOutputDevice = DEFAULT_DEVICE;
			}
		}
		mDevicesUpdated = TRUE;
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

//control value for middle mouse as talk2push button
const static std::string MIDDLE_MOUSE_CV = "MiddleMouse";

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
	void setFunction(LLUICtrl* ctrl) { mCtrl = ctrl; }
	void setMode(S32 mode) { mMode = mode; }

	BOOL handleKeyHere(KEY key, MASK mask);
	void onCancel();
	void onBind();
	void onMasks();
	void onOpen(const LLSD& key);

	LLUICtrl* mCtrl;
	S32 mMode;
	MASK mMask;
	KEY mKey;
private:
	LLFloaterPreference* mParent;
};

LLSetKeyDialog::LLSetKeyDialog(const LLSD& key)
	: LLModalDialog(key),
	mParent(NULL),
	mCtrl(NULL),
	mMode(NULL),
	mMask(MASK_NONE),
	mKey(NULL)
{
	mCommitCallbackRegistrar.add("Set.Masks", boost::bind(&LLSetKeyDialog::onMasks, this));
	mCommitCallbackRegistrar.add("Set.Bind", boost::bind(&LLSetKeyDialog::onBind, this));
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
	getChild<LLUICtrl>("FocusButton")->setFocus(TRUE);
	gFocusMgr.setKeystrokesOnly(TRUE);
}

void LLSetKeyDialog::onCancel()
{
	this->closeFloater();
}

void LLSetKeyDialog::onBind()
{
	if (mParent && mKey != NULL)
	{
		mParent->onBindKey(mKey, mMask, mCtrl, mMode);
	}
	this->closeFloater();
}


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

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false),
	mLanguageChanged(false),
	mAvatarDataInitialized(false),
	mClickActionDirty(false)
{
	LLConversationLog::instance().addObserver(this);

	//Build Floater is now Called from 	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	
	static bool registered_voice_dialog = false;
	if (!registered_voice_dialog)
	{
		LLFloaterReg::add("voice_set_key", "floater_select_key_voice.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLVoiceSetKeyDialog>);
		registered_voice_dialog = true;
	}

	//BD - Custom Keyboard Layout
	static bool registered_dialog = false;
	if (!registered_dialog)
	{
		LLFloaterReg::add("set_any_key", "floater_select_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLSetKeyDialog>);
		registered_dialog = true;
	}
	
	mCommitCallbackRegistrar.add("Pref.Apply",					boost::bind(&LLFloaterPreference::onBtnApply, this));
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
	mCommitCallbackRegistrar.add("Pref.HardwareSettings",		boost::bind(&LLFloaterPreference::onOpenHardwareSettings, this));
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",		boost::bind(&LLFloaterPreference::setHardwareDefaults, this));
	mCommitCallbackRegistrar.add("Pref.UpdateSliderText",		boost::bind(&LLFloaterPreference::refreshUI,this));
	mCommitCallbackRegistrar.add("Pref.applyUIColor",			boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.getUIColor",				boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.BlockList",				boost::bind(&LLFloaterPreference::onClickBlockList, this));
	mCommitCallbackRegistrar.add("Pref.Proxy",					boost::bind(&LLFloaterPreference::onClickProxySettings, this));
	mCommitCallbackRegistrar.add("Pref.TranslationSettings",	boost::bind(&LLFloaterPreference::onClickTranslationSettings, this));
	mCommitCallbackRegistrar.add("Pref.AutoReplace",            boost::bind(&LLFloaterPreference::onClickAutoReplace, this));
	mCommitCallbackRegistrar.add("Pref.PermsDefault",           boost::bind(&LLFloaterPreference::onClickPermsDefault, this));
	mCommitCallbackRegistrar.add("Pref.SpellChecker",           boost::bind(&LLFloaterPreference::onClickSpellChecker, this));

//	//BD - Custom Keyboard Layout
	mCommitCallbackRegistrar.add("Pref.BindKey",                boost::bind(&LLFloaterPreference::onClickSetAnyKey, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.UnbindKey",				boost::bind(&LLFloaterPreference::onUnbindKey, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ExportControls",         boost::bind(&LLFloaterPreference::onExportControls, this));
	mCommitCallbackRegistrar.add("Pref.UnbindAll",				boost::bind(&LLFloaterPreference::onUnbindControls, this));
	mCommitCallbackRegistrar.add("Pref.DefaultControls",		boost::bind(&LLFloaterPreference::onDefaultControls, this));

//	//BD - Expandable Tabs
	mCommitCallbackRegistrar.add("Pref.Tab",					boost::bind(&LLFloaterPreference::onTab, this, _1, _2));

//	//BD - Vector4
	mCommitCallbackRegistrar.add("Pref.ArrayVec4X",				boost::bind(&LLFloaterPreference::onCommitVec4X, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Y",				boost::bind(&LLFloaterPreference::onCommitVec4Y, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Z",				boost::bind(&LLFloaterPreference::onCommitVec4Z, this, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4W",				boost::bind(&LLFloaterPreference::onCommitVec4W, this, _1, _2));

//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX",					boost::bind(&LLFloaterPreference::onCommitX, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY",					boost::bind(&LLFloaterPreference::onCommitY, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ",					boost::bind(&LLFloaterPreference::onCommitZ, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayXD",				boost::bind(&LLFloaterPreference::onCommitXd, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayYD",				boost::bind(&LLFloaterPreference::onCommitYd, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZD",				boost::bind(&LLFloaterPreference::onCommitZd, this,_1, _2));

//	//BD - Revert to Default
	mCommitCallbackRegistrar.add("Pref.Default",				boost::bind(&LLFloaterPreference::resetToDefault, this,_1));

//	//BD - Input/Output resizer
	mCommitCallbackRegistrar.add("Pref.InputOutput",			boost::bind(&LLFloaterPreference::inputOutput, this));

//	//BD - Catznip's Borderless Window Mode
	mCommitCallbackRegistrar.add("Pref.FullscreenWindow",		boost::bind(&LLFloaterPreference::toggleFullscreenWindow, this));

	sSkin = gSavedSettings.getString("SkinCurrent");

	gSavedSettings.getControl("NameTagShowUsernames")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("NameTagShowFriends")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("UseDisplayNames")->getCommitSignal()->connect(boost::bind(&handleDisplayNamesOptionChanged,  _2));
	
	gSavedSettings.getControl("AppearanceCameraMovement")->getCommitSignal()->connect(boost::bind(&handleAppearanceCameraMovementChanged,  _2));

	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );

	mCommitCallbackRegistrar.add("Pref.ClearLog",				boost::bind(&LLConversationLog::onClearLog, &LLConversationLog::instance()));
	mCommitCallbackRegistrar.add("Pref.DeleteTranscripts",      boost::bind(&LLFloaterPreference::onDeleteTranscripts, this));
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

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();

	getChild<LLUICtrl>("cache_location")->setEnabled(FALSE); // make it read-only but selectable (STORM-227)
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);

	getChild<LLUICtrl>("log_path_string")->setEnabled(FALSE); // make it read-only but selectable

	getChild<LLComboBox>("language_combobox")->setCommitCallback(boost::bind(&LLFloaterPreference::onLanguageChange, this));

	getChild<LLComboBox>("FriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"FriendIMOptions"));
	getChild<LLComboBox>("NonFriendIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NonFriendIMOptions"));
	getChild<LLComboBox>("ConferenceIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ConferenceIMOptions"));
	getChild<LLComboBox>("GroupChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"GroupChatOptions"));
	getChild<LLComboBox>("NearbyChatOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"NearbyChatOptions"));
	getChild<LLComboBox>("ObjectIMOptions")->setCommitCallback(boost::bind(&LLFloaterPreference::onNotificationsChange, this,"ObjectIMOptions"));

	// if floater is opened before login set default localized do not disturb message
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("DoNotDisturbModeResponse", LLTrans::getString("DoNotDisturbModeResponseDefault"));
	}

	// set 'enable' property for 'Clear log...' button
	changed();

	LLLogChat::setSaveHistorySignal(boost::bind(&LLFloaterPreference::onLogChatHistorySaved, this));

//	//BD - Refresh our controls at the start
	refreshGraphicControls();
	refreshCameraControls();
	refreshKeys();

	toggleTabs();
	if (!gSavedSettings.getBOOL("RememberPreferencesTabs"))
	{
		gSavedSettings.setBOOL("PrefsViewerVisible", false);
		gSavedSettings.setBOOL("PrefsBasicVisible", false);
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
	// clean up user data
	LLComboBox* ctrl_window_size = getChild<LLComboBox>("windowsize combo");
	for (S32 i = 0; i < ctrl_window_size->getItemCount(); i++)
	{
		ctrl_window_size->setCurrentByIndex(i);
	}

	LLConversationLog::instance().removeObserver(this);
}

//BD - Vector4
void LLFloaterPreference::onCommitVec4W(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector4 value = gSavedSettings.getVector4(param.asString());
	value.mV[VW] = ctrl->getValue().asReal();
	gSavedSettings.setVector4(param.asString(), value);
}

void LLFloaterPreference::onCommitVec4X(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector4 value = gSavedSettings.getVector4(param.asString());
	value.mV[VX] = ctrl->getValue().asReal();
	gSavedSettings.setVector4(param.asString(), value);
}

void LLFloaterPreference::onCommitVec4Y(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector4 value = gSavedSettings.getVector4(param.asString());
	value.mV[VY] = ctrl->getValue().asReal();
	gSavedSettings.setVector4(param.asString(), value);
}

void LLFloaterPreference::onCommitVec4Z(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector4 value = gSavedSettings.getVector4(param.asString());
	value.mV[VZ] = ctrl->getValue().asReal();
	gSavedSettings.setVector4( param.asString() , value);
}

//BD - Array Debugs
void LLFloaterPreference::onCommitX(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3 value = gSavedSettings.getVector3(param.asString());
	value.mV[VX] = ctrl->getValue().asReal();
	gSavedSettings.setVector3(param.asString(), value);
}

void LLFloaterPreference::onCommitY(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3 value = gSavedSettings.getVector3(param.asString());
	value.mV[VY] = ctrl->getValue().asReal();
	gSavedSettings.setVector3(param.asString(), value);
}

void LLFloaterPreference::onCommitZ(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3 value = gSavedSettings.getVector3(param.asString());
	value.mV[VZ] = ctrl->getValue().asReal();
	gSavedSettings.setVector3(param.asString(), value);
}

void LLFloaterPreference::onCommitXd(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3d value = gSavedSettings.getVector3d(param.asString());
	value.mdV[VX] = ctrl->getValue().asReal();
	gSavedSettings.setVector3d( param.asString() , value);
}

void LLFloaterPreference::onCommitYd(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3d value = gSavedSettings.getVector3d(param.asString());
	value.mdV[VY] = ctrl->getValue().asReal();
	gSavedSettings.setVector3d( param.asString() , value);
}

void LLFloaterPreference::onCommitZd(LLUICtrl* ctrl, const LLSD& param)
{
	LLVector3d value = gSavedSettings.getVector3d(param.asString());
	value.mdV[VZ] = ctrl->getValue().asReal();
	gSavedSettings.setVector3d( param.asString() , value);
}

//BD - Revert to Default
void LLFloaterPreference::resetToDefault(LLUICtrl* ctrl)
{
	ctrl->getControlVariable()->resetToDefault(true);
	refreshGraphicControls();
}

//BD - Custom Keyboard Layout
void LLFloaterPreference::onBindKey(KEY key, MASK mask, LLUICtrl* ctrl, const LLSD& param)
{
	S32 mode;
	LLSD settings;
	llifstream infile;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");
	bool bound = false;

	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Settings") << "Cannot find file " << filename << " to load." << LL_ENDL;
		return;
	}

	//BD - We need to unbind all keys, to ensure that everything is empty and properly rebound,
	//     this prevents a whole bunch of issues but makes it a tedious work to fix if something breaks.
	gViewerKeyboard.unbindAllKeys(true);
	//LL_INFOS("Settings") << "Writing bound controls to " << filename << LL_ENDL;
	while (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(settings, infile))
	{
		mode = settings["mode"].asInteger();
		if (!bound && settings["slot"].asInteger() == param.asInteger())
		{
			gViewerKeyboard.bindKey(mode, key, mask, settings["function"].asString());
			//LL_INFOS() << "Rebound: " << key << +"(" + gKeyboard->stringFromKey(key) << ") + " 
			//	<< mask << +"(" << gKeyboard->stringFromMask(mask) << ") to "
			//	<< settings["function"].asString() << " in mode "
			//	<< settings["mode"].asInteger() << LL_ENDL;
			bound = true;
		}
		else
		{
			MASK old_mask = MASK_NONE;
			KEY old_key = NULL;
			LLKeyboard::keyFromString(settings["key"], &old_key);
			LLKeyboard::maskFromString(settings["mask"], &old_mask);
			//LL_INFOS() << "Kept: " << old_key << +"(" + settings["key"].asString() << ") + "
			//	<< mask << +"(" << settings["mask"].asString() << ") to " 
			//	<< settings["function"].asString() << " in mode "
			//	<< settings["mode"].asInteger() << LL_ENDL;
			gViewerKeyboard.bindKey(mode, old_key, old_mask, settings["function"].asString());
		}
	}
	infile.close();
	//BD - We can now safely rewrite the entire file now that we filled all bindings in.
	onExportControls();
}

void LLFloaterPreference::onUnbindKey(LLUICtrl* ctrl, const LLSD& param)
{
	onBindKey(NULL, MASK_NONE, ctrl, param);
}

void LLFloaterPreference::onExportControls()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");
	//LL_INFOS("Settings") << "Exporting controls to " << filename << LL_ENDL;
	gViewerKeyboard.exportBindingsXML(filename);
	refreshKeys();
}

void LLFloaterPreference::onUnbindControls()
{
	if (gViewerKeyboard.unbindAllKeys(false))
	{
		onExportControls();
	}
}

void LLFloaterPreference::onDefaultControls()
{
	gViewerKeyboard.unbindAllKeys(true);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "controls.xml");
	LL_INFOS("Settings") << "Loading default controls file from " << filename << LL_ENDL;
	if (gViewerKeyboard.loadBindingsSettings(filename))
	{
		onExportControls();
	}
}

void LLFloaterPreference::refreshKeys()
{
	LLSD settings;
	llifstream infile;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "controls.xml");

	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Settings") << "Cannot find file " << filename << " to load." << LL_ENDL;
		return;
	}

	LLUICtrl* ctrl;
	while (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(settings, infile))
	{
		ctrl = getChild<LLButton>("label_slot_" + settings["slot"].asString());
		if (ctrl)
		{
			MASK mask;
			std::string mask_string = "";
			gKeyboard->maskFromString(settings["mask"].asString(), &mask);
			ctrl->setLabelArg("[KEY]", settings["key"].asString());
			
			if (mask != MASK_NONE)
			{
				mask_string = " + [" + gKeyboard->stringFromMask(mask, true) + "]";
				ctrl->setLabelArg("[MASK]", mask_string);
			}
			else
			{
				ctrl->setLabelArg("[MASK]", mask_string);
			}
		}
	}
	infile.close();
}

//BD - Expandable Tabs
void LLFloaterPreference::onTab(LLUICtrl* ctrl, const LLSD& param)
{
	getChild<LLLayoutPanel>(param.asString())->setVisible(ctrl->getValue());
	LLRect rect = getChild<LLPanel>("gfx_scroll_panel")->getRect();
	S32 modifier = getChild<LLLayoutPanel>(param.asString())->getRect().getHeight();
	modifier -= 5;
	if (ctrl->getValue().asBoolean())
	{
		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), rect.getHeight() + modifier);
		getChild<LLLayoutStack>("gfx_stack")->translate(0, modifier);
	}
	else
	{
		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), rect.getHeight() - modifier);
		getChild<LLLayoutStack>("gfx_stack")->translate(0, -modifier);
	}
	getChild<LLPanel>("gfx_scroll_panel")->setRect(rect);
}

//BD - Tab Toggles
void LLFloaterPreference::toggleTabs()
{
	LLRect rect = getChild<LLPanel>("gfx_scroll_panel")->getRect();
	S32 modifier = 0;
	if (gSavedSettings.getBOOL("PrefsViewerVisible"))
		modifier += (getChild<LLLayoutPanel>("viewer_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsBasicVisible"))
		modifier += (getChild<LLLayoutPanel>("basic_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsLoDVisible"))
		modifier += (getChild<LLLayoutPanel>("lod_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsPerformanceVisible"))
		modifier += (getChild<LLLayoutPanel>("performance_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsVertexVisible"))
		modifier += (getChild<LLLayoutPanel>("vertex_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsDeferredVisible"))
		modifier += (getChild<LLLayoutPanel>("deferred_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsDoFVisible"))
		modifier += (getChild<LLLayoutPanel>("dof_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsAOVisible"))
		modifier += (getChild<LLLayoutPanel>("ao_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsMotionBlurVisible"))
		modifier += (getChild<LLLayoutPanel>("motionblur_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsGodraysVisible"))
		modifier += (getChild<LLLayoutPanel>("godrays_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsPostVisible"))
		modifier += (getChild<LLLayoutPanel>("post_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsToneMappingVisible"))
		modifier += (getChild<LLLayoutPanel>("tonemapping_layout_panel")->getRect().getHeight() - 5);
	if (gSavedSettings.getBOOL("PrefsVignetteVisible"))
		modifier += (getChild<LLLayoutPanel>("vignette_layout_panel")->getRect().getHeight() - 5);

	rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), (rect.getHeight() + modifier));
	getChild<LLPanel>("gfx_scroll_panel")->setRect(rect);
	getChild<LLLayoutStack>("gfx_stack")->translate(0, modifier);
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

//BD - Catznip's Borderless Window Mode
void LLFloaterPreference::toggleFullscreenWindow()
{
	if ((gViewerWindow) && (gViewerWindow->canFullscreenWindow()))
		gViewerWindow->setFullscreenWindow(!gViewerWindow->getFullscreenWindow());
}

//BD - Refresh all controls
void LLFloaterPreference::refreshGraphicControls()
{
	getChild<LLUICtrl>("RenderGlowLumWeights_X")->setValue(gSavedSettings.getVector3("RenderGlowLumWeights").mV[VX]);
	getChild<LLUICtrl>("RenderGlowLumWeights_Y")->setValue(gSavedSettings.getVector3("RenderGlowLumWeights").mV[VY]);
	getChild<LLUICtrl>("RenderGlowLumWeights_Z")->setValue(gSavedSettings.getVector3("RenderGlowLumWeights").mV[VZ]);
	getChild<LLUICtrl>("RenderGlowWarmthWeights_X")->setValue(gSavedSettings.getVector3("RenderGlowWarmthWeights").mV[VX]);
	getChild<LLUICtrl>("RenderGlowWarmthWeights_Y")->setValue(gSavedSettings.getVector3("RenderGlowWarmthWeights").mV[VY]);
	getChild<LLUICtrl>("RenderGlowWarmthWeights_Z")->setValue(gSavedSettings.getVector3("RenderGlowWarmthWeights").mV[VZ]);

	getChild<LLUICtrl>("RenderShadowResolution_X")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VX]);
	getChild<LLUICtrl>("RenderShadowResolution_Y")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VY]);
	getChild<LLUICtrl>("RenderShadowResolution_Z")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VZ]);
	getChild<LLUICtrl>("RenderShadowResolution_W")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VW]);

	getChild<LLUICtrl>("RenderProjectorShadowResolution_X")->setValue(gSavedSettings.getVector3("RenderProjectorShadowResolution").mV[VX]);
	getChild<LLUICtrl>("RenderProjectorShadowResolution_Y")->setValue(gSavedSettings.getVector3("RenderProjectorShadowResolution").mV[VY]);
	getChild<LLUICtrl>("RenderProjectorShadowResolution_Z")->setValue(gSavedSettings.getVector3("RenderProjectorShadowResolution").mV[VZ]);

	getChild<LLUICtrl>("ExodusRenderToneAdvOptA_X")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptA").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptA_Y")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptA").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptA_Z")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptA").mV[VZ]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptB_X")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptB").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptB_Y")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptB").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptB_Z")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptB").mV[VZ]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptC_X")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptC").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderToneAdvOptC_Y")->setValue(gSavedSettings.getVector3("ExodusRenderToneAdvOptC").mV[VY]);

	getChild<LLUICtrl>("ExodusRenderGamma_X")->setValue(gSavedSettings.getVector3("ExodusRenderGamma").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderGamma_Y")->setValue(gSavedSettings.getVector3("ExodusRenderGamma").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderGamma_Z")->setValue(gSavedSettings.getVector3("ExodusRenderGamma").mV[VZ]);
	getChild<LLUICtrl>("ExodusRenderExposure_X")->setValue(gSavedSettings.getVector3("ExodusRenderExposure").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderExposure_Y")->setValue(gSavedSettings.getVector3("ExodusRenderExposure").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderExposure_Z")->setValue(gSavedSettings.getVector3("ExodusRenderExposure").mV[VZ]);
	getChild<LLUICtrl>("ExodusRenderOffset_X")->setValue(gSavedSettings.getVector3("ExodusRenderOffset").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderOffset_Y")->setValue(gSavedSettings.getVector3("ExodusRenderOffset").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderOffset_Z")->setValue(gSavedSettings.getVector3("ExodusRenderOffset").mV[VZ]);
	getChild<LLUICtrl>("ExodusRenderVignette_X")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderVignette_Y")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderVignette_Z")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VZ]);
}

void LLFloaterPreference::refreshCameraControls()
{
	getChild<LLUICtrl>("FocusOffsetFrontView_X")->setValue(gSavedSettings.getVector3d("FocusOffsetFrontView").mdV[VX]);
	getChild<LLUICtrl>("FocusOffsetFrontView_Y")->setValue(gSavedSettings.getVector3d("FocusOffsetFrontView").mdV[VY]);
	getChild<LLUICtrl>("FocusOffsetFrontView_Z")->setValue(gSavedSettings.getVector3d("FocusOffsetFrontView").mdV[VZ]);
	getChild<LLUICtrl>("FocusOffsetGroupView_X")->setValue(gSavedSettings.getVector3d("FocusOffsetGroupView").mdV[VX]);
	getChild<LLUICtrl>("FocusOffsetGroupView_Y")->setValue(gSavedSettings.getVector3d("FocusOffsetGroupView").mdV[VY]);
	getChild<LLUICtrl>("FocusOffsetGroupView_Z")->setValue(gSavedSettings.getVector3d("FocusOffsetGroupView").mdV[VZ]);
	getChild<LLUICtrl>("FocusOffsetRearView_X")->setValue(gSavedSettings.getVector3d("FocusOffsetRearView").mdV[VX]);
	getChild<LLUICtrl>("FocusOffsetRearView_Y")->setValue(gSavedSettings.getVector3d("FocusOffsetRearView").mdV[VY]);
	getChild<LLUICtrl>("FocusOffsetRearView_Z")->setValue(gSavedSettings.getVector3d("FocusOffsetRearView").mdV[VZ]);
	getChild<LLUICtrl>("FocusOffsetLeftShoulderView_X")->setValue(gSavedSettings.getVector3d("FocusOffsetLeftShoulderView").mdV[VX]);
	getChild<LLUICtrl>("FocusOffsetLeftShoulderView_Y")->setValue(gSavedSettings.getVector3d("FocusOffsetLeftShoulderView").mdV[VY]);
	getChild<LLUICtrl>("FocusOffsetLeftShoulderView_Z")->setValue(gSavedSettings.getVector3d("FocusOffsetLeftShoulderView").mdV[VZ]);
	getChild<LLUICtrl>("FocusOffsetRightShoulderView_X")->setValue(gSavedSettings.getVector3d("FocusOffsetRightShoulderView").mdV[VX]);
	getChild<LLUICtrl>("FocusOffsetRightShoulderView_Y")->setValue(gSavedSettings.getVector3d("FocusOffsetRightShoulderView").mdV[VY]);
	getChild<LLUICtrl>("FocusOffsetRightShoulderView_Z")->setValue(gSavedSettings.getVector3d("FocusOffsetRightShoulderView").mdV[VZ]);

	getChild<LLUICtrl>("CameraOffsetFrontView_X")->setValue(gSavedSettings.getVector3("CameraOffsetFrontView").mV[VX]);
	getChild<LLUICtrl>("CameraOffsetFrontView_Y")->setValue(gSavedSettings.getVector3("CameraOffsetFrontView").mV[VY]);
	getChild<LLUICtrl>("CameraOffsetFrontView_Z")->setValue(gSavedSettings.getVector3("CameraOffsetFrontView").mV[VZ]);
	getChild<LLUICtrl>("CameraOffsetGroupView_X")->setValue(gSavedSettings.getVector3("CameraOffsetGroupView").mV[VX]);
	getChild<LLUICtrl>("CameraOffsetGroupView_Y")->setValue(gSavedSettings.getVector3("CameraOffsetGroupView").mV[VY]);
	getChild<LLUICtrl>("CameraOffsetGroupView_Z")->setValue(gSavedSettings.getVector3("CameraOffsetGroupView").mV[VZ]);
	getChild<LLUICtrl>("CameraOffsetRearView_X")->setValue(gSavedSettings.getVector3("CameraOffsetRearView").mV[VX]);
	getChild<LLUICtrl>("CameraOffsetRearView_Y")->setValue(gSavedSettings.getVector3("CameraOffsetRearView").mV[VY]);
	getChild<LLUICtrl>("CameraOffsetRearView_Z")->setValue(gSavedSettings.getVector3("CameraOffsetRearView").mV[VZ]);
	getChild<LLUICtrl>("CameraOffsetLeftShoulderView_X")->setValue(gSavedSettings.getVector3("CameraOffsetLeftShoulderView").mV[VX]);
	getChild<LLUICtrl>("CameraOffsetLeftShoulderView_Y")->setValue(gSavedSettings.getVector3("CameraOffsetLeftShoulderView").mV[VY]);
	getChild<LLUICtrl>("CameraOffsetLeftShoulderView_Z")->setValue(gSavedSettings.getVector3("CameraOffsetLeftShoulderView").mV[VZ]);
	getChild<LLUICtrl>("CameraOffsetRightShoulderView_X")->setValue(gSavedSettings.getVector3("CameraOffsetRightShoulderView").mV[VX]);
	getChild<LLUICtrl>("CameraOffsetRightShoulderView_Y")->setValue(gSavedSettings.getVector3("CameraOffsetRightShoulderView").mV[VY]);
	getChild<LLUICtrl>("CameraOffsetRightShoulderView_Z")->setValue(gSavedSettings.getVector3("CameraOffsetRightShoulderView").mV[VZ]);
}

//BD - Warning system
void LLFloaterPreference::refreshWarnings()
{
	//BD - Viewer Options
	getChild<LLUICtrl>("warning_ui_size")->setVisible(gSavedSettings.getF32("UIScaleFactor") != 1.0);
	getChild<LLUICtrl>("warning_font_dpi")->setVisible(gSavedSettings.getF32("FontScreenDPI") != 96.0);
	getChild<LLUICtrl>("warning_texture_memory")->setVisible(gSavedSettings.getU32("TextureMemory") > 768
														|| ((gSavedSettings.getU32("TextureMemory") 
														+ gSavedSettings.getU32("SystemMemory")) > 768
														&& gSavedSettings.getBOOL("CustomSystemMemory")));

	//BD - Basic Options
	getChild<LLUICtrl>("warning_texture_compression")->setVisible(gSavedSettings.getBOOL("RenderCompressTextures"));
	getChild<LLUICtrl>("warning_draw_distance")->setVisible(gPipeline.RenderFarClip > 128);

	//BD - LOD Options
	getChild<LLUICtrl>("warning_dynamic_lod")->setVisible(!gPipeline.sDynamicLOD);
	getChild<LLUICtrl>("warning_object_lod")->setVisible(LLVOVolume::sLODFactor > 2.0);

	//BD - Performance Options
	getChild<LLUICtrl>("warning_object_occlusion")->setVisible(gPipeline.RenderDeferred && gPipeline.sUseOcclusion);
	getChild<LLUICtrl>("warning_avatars_visible")->setVisible(gSavedSettings.getU32("RenderAvatarMaxNonImpostors") > 15);
	getChild<LLUICtrl>("warning_derender_kb")->setVisible(gSavedSettings.getU32("RenderAutoMuteByteLimit") > 12000000);
	getChild<LLUICtrl>("warning_derender_m2")->setVisible(gSavedSettings.getU32("RenderAutoMuteSurfaceAreaLimit") > 200);
	getChild<LLUICtrl>("warning_derender_ar")->setVisible(gSavedSettings.getU32("RenderAvatarMaxComplexity") > 120000);
	getChild<LLUICtrl>("warning_derender_surface")->setVisible(gSavedSettings.getU32("RenderAutoHideSurfaceAreaLimit") > 200);

	//BD - Windlight Options
	getChild<LLUICtrl>("warning_reflection_quality")->setVisible(gSavedSettings.getU32("RenderReflectionRes") > 768);
	getChild<LLUICtrl>("warning_sky_quality")->setVisible(gSavedSettings.getU32("WLSkyDetail") > 128);

	//BD - Deferred Rendering Options
	getChild<LLUICtrl>("warning_fxaa")->setVisible(gPipeline.RenderFSAASamples == 0);
	getChild<LLUICtrl>("warning_shadow_resolution")->setVisible(gPipeline.RenderShadowResolution.mV[VX] > 2048 
															|| gPipeline.RenderShadowResolution.mV[VY] > 2048
															|| gPipeline.RenderShadowResolution.mV[VZ] > 2048
															|| gPipeline.RenderShadowResolution.mV[VW] > 2048);
	getChild<LLUICtrl>("warning_projector_resolution")->setVisible(gPipeline.RenderProjectorShadowResolution.mV[VX] > 2048
																|| gPipeline.RenderProjectorShadowResolution.mV[VY] > 2048);

	//BD - SSR Options
	getChild<LLUICtrl>("warning_ssr")->setVisible(gSavedSettings.getBOOL("RenderScreenSpaceReflections"));

	//BD - Motion Blur Options
	getChild<LLUICtrl>("warning_blur_quality")->setVisible(gSavedSettings.getU32("RenderMotionBlurStrength") < 100);

	//BD - Volumetric Lighting Options
	getChild<LLUICtrl>("warning_light_resolution")->setVisible(gSavedSettings.getU32("RenderGodraysResolution") > 32);
}

void LLFloaterPreference::draw()
{
	BOOL has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);
	
	has_first_selected = (getChildRef<LLScrollListCtrl>("enabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedEnabledPopups", has_first_selected);

	refreshWarnings();
	
	LLFloater::draw();
}

void LLFloaterPreference::saveSettings()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
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
	
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (sSkin != gSavedSettings.getString("SkinCurrent"))
	{
		LLNotificationsUtil::add("ChangeSkin");
		refreshSkin(this);
	}
	// Call apply() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		 iter != tabcontainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->apply();
	}
	
	gViewerWindow->requestResolutionUpdate(); // for UIScaleFactor

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);
	
	LLViewerMedia::setCookiesEnabled(getChild<LLUICtrl>("cookies_enabled")->getValue());
	
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
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	// Call cancel() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		iter != tabcontainer->getChildList()->end(); ++iter)
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
	
	LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest( gAgent.getID() );
	if (can_choose_maturity)
	{		
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlikeWithoutAdminMenuFakery())
		{
			// we're going to remove the adult entry from the combo
			LLScrollListCtrl* maturity_list = maturity_combo->findChild<LLScrollListCtrl>("ComboBox");
			if (maturity_list)
			{
				maturity_list->deleteItems(LLSD(SIM_ACCESS_ADULT));
			}
		}
		getChildView("maturity_desired_combobox")->setEnabled( true);
	}
	else
	{
		getChildView("maturity_desired_combobox")->setEnabled( false);
	}

	// Forget previous language changes.
	mLanguageChanged = false;
	
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

	//LLPanelLogin::setAlwaysRefresh(true);
	refresh();

	refreshGraphicControls();
	refreshCameraControls();

	// Make sure the current state of prefs are saved away when
	// when the floater is opened.  That will make cancel do its
	// job
	saveSettings();

	// Make sure there is a default preference file
	LLPresetsManager::getInstance()->createMissingDefault();

	bool started = (LLStartUp::getStartupState() == STATE_STARTED);

	LLButton* load_btn = findChild<LLButton>("PrefLoadButton");
	LLButton* save_btn = findChild<LLButton>("PrefSaveButton");
	LLButton* delete_btn = findChild<LLButton>("PrefDeleteButton");

	load_btn->setEnabled(started);
	save_btn->setEnabled(started);
	delete_btn->setEnabled(started);
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

	// reset indirects before refresh because we may have changed what they control
	setIndirectControls();

	refreshEnabledGraphics();

	gSavedSettings.setString("PresetGraphicActive", "");
	LLPresetsManager::getInstance()->triggerChangeSignal();

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->setHardwareDefaults();
	}
}

void LLFloaterPreference::getControlNames(std::vector<std::string>& names)
{
	LLView* view = findChild<LLView>("display");
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
	gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
	//LLPanelLogin::setAlwaysRefresh(false);
	if (!app_quitting)
	{
		cancel();
	}

	// when closing this window, turn of visiblity control so that 
	// next time preferences is opened we don't suspend voice
	if (gSavedSettings.getBOOL("ShowDeviceSettings"))
	{
		gSavedSettings.setBOOL("ShowDeviceSettings", FALSE);
		inputOutput();
	}
}

void LLFloaterPreference::onOpenHardwareSettings()
{
	LLFloater* floater = LLFloaterReg::showInstance("prefs_hardware_settings");
	addDependentFloater(floater, FALSE);
}
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
		closeFloater(false);

		//Conversation transcript and log path changed so reload conversations based on new location
		if(mPriorInstantMessageLogPath.length())
		{
			if(moveTranscriptsAndLog())
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
		if(mGotPersonalInfo)
		{
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
	}
	}
	else
	{
		// Show beep, pop up dialog, etc.
		LL_INFOS() << "Can't close preferences!" << LL_ENDL;
	}

	//LLPanelLogin::updateLocationSelectorsVisibility();	
	//Need to reload the navmesh if the pathing console is up
	LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
	if ( !pathfindingConsoleHandle.isDead() )
	{
		LLFloaterPathfindingConsole* pPathfindingConsole = pathfindingConsoleHandle.get();
		pPathfindingConsole->onRegionBoundaryCross();
	}
	
}

// static 
void LLFloaterPreference::onBtnApply( )
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	apply();
	saveSettings();

	//LLPanelLogin::updateLocationSelectorsVisibility();
}

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
	closeFloater();
}

// static 
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->setPersonalInfo(visibility, im_via_email);	
	}
}


void LLFloaterPreference::refreshEnabledGraphics()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if (instance)
	{
		instance->refresh();
		//instance->refreshEnabledState();
	}
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
	mNotificationOptions[OptionName] = getChild<LLComboBox>(OptionName)->getSelectedItemLabel();

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
//	std::string cur_top_folder(gDirUtilp->getBaseFileName(cur_name));
	
	std::string proposed_name(cur_name);

	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
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
	LLComboBox* ctrl_reflections = getChild<LLComboBox>("Reflections");
	
// [RLVa:KB] - Checked: 2013-05-11 (RLVa-1.4.9)
	if (rlv_handler_t::isEnabled())
	{
		getChild<LLUICtrl>("do_not_disturb_response")->setEnabled(!RlvActions::hasBehaviour(RLV_BHVR_SENDIM));
	}
// [/RLVa:KB]

	// Reflections
	BOOL reflections = gSavedSettings.getBOOL("RenderDeferred") 
		&& gGLManager.mHasCubeMap
		&& LLCubeMap::sUseCubeMaps;
	ctrl_reflections->setEnabled(reflections);
	
	// Bump & Shiny	
	//LLCheckBoxCtrl* bumpshiny_ctrl = getChild<LLCheckBoxCtrl>("BumpShiny");
	//bool bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump");
	//bumpshiny_ctrl->setEnabled(bumpshiny ? TRUE : FALSE);
	
	// Avatar Mode
	// Enable Avatar Shaders
	//LLCheckBoxCtrl* ctrl_avatar_vp = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	// Avatar Render Mode
	//LLCheckBoxCtrl* ctrl_avatar_cloth = getChild<LLCheckBoxCtrl>("AvatarCloth");
	
	//bool avatar_vp_enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred");
	//if (LLViewerShaderMgr::sInitialized)
	//{
	//	S32 max_avatar_shader = LLViewerShaderMgr::instance()->mMaxAvatarShaderLevel;
	//	avatar_vp_enabled = (max_avatar_shader > 0) ? TRUE : FALSE;
	//}

	//ctrl_avatar_vp->setEnabled(avatar_vp_enabled);
	
	// Vertex Shaders
	// Global Shader Enable
	//LLCheckBoxCtrl* ctrl_shader_enable   = getChild<LLCheckBoxCtrl>("BasicShaders");
	
//	ctrl_shader_enable->setEnabled(LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"));
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a) | Modified: RLVa-0.2.0a
	// "Basic Shaders" can't be disabled - but can be enabled - under @setenv=n
	//bool fCtrlShaderEnable = LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable");
	//ctrl_shader_enable->setEnabled(
	//	fCtrlShaderEnable && ((!gRlvHandler.hasBehaviour(RLV_BHVR_SETENV)) || (!gSavedSettings.getBOOL("VertexShaderEnable"))) );
// [/RLVa:KB]

	//BOOL shaders = ctrl_shader_enable->get();
	
	// WindLight
	//LLCheckBoxCtrl* ctrl_wind_light = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	
	// *HACK just checks to see if we can use shaders... 
	// maybe some cards that use shaders, but don't support windlight
//	ctrl_wind_light->setEnabled(ctrl_shader_enable->getEnabled() && shaders);
// [RLVa:KB] - Checked: 2010-03-18 (RLVa-1.2.0a) | Modified: RLVa-0.2.0a
	// "Atmospheric Shaders" can't be disabled - but can be enabled - under @setenv=n
	//bool fCtrlWindLightEnable = fCtrlShaderEnable && shaders;
	//ctrl_wind_light->setEnabled(
	//	fCtrlWindLightEnable && ((!gRlvHandler.hasBehaviour(RLV_BHVR_SETENV)) || (!gSavedSettings.getBOOL("WindLightUseAtmosShaders"))) );
// [/RLVa:KB]

	//Deferred/SSAO/Shadows
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("Deferred");

	
	BOOL enabled = LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
						//((bumpshiny_ctrl && bumpshiny_ctrl->get()) ? TRUE : FALSE) &&
						//shaders && 
						gGLManager.mHasFramebufferObject;

	ctrl_deferred->setEnabled(enabled);

	LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");
	LLCheckBoxCtrl* ctrl_dof = getChild<LLCheckBoxCtrl>("UseDoF");
	LLComboBox* ctrl_shadow = getChild<LLComboBox>("ShadowDetail");

	// note, okay here to get from ctrl_deferred as it's twin, ctrl_deferred2 will alway match it
	enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO") && (ctrl_deferred->get() ? TRUE : FALSE);
	
	ctrl_deferred->set(gSavedSettings.getBOOL("RenderDeferred"));

	ctrl_ssao->setEnabled(enabled);
	ctrl_dof->setEnabled(enabled);

	enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail");

	ctrl_shadow->setEnabled(enabled);
	

	// now turn off any features that are unavailable
	disableUnavailableSettings();

	getChildView("block_list")->setEnabled(LLLoginInstance::getInstance()->authSuccess());

	//BD - We might want to add this later.
	// Cannot have floater active until caps have been received
	//getChild<LLButton>("default_creation_permissions")->setEnabled(LLStartUp::getStartupState() < STATE_STARTED ? false : true);
}

// static
void LLFloaterPreference::setIndirectControls()
{
	/*
	* We have controls that have an indirect relationship between the control
	* values and adjacent text and the underlying setting they influence.
	* In each case, the control and its associated setting are named Indirect<something>
	* This method interrogates the controlled setting and establishes the
	* appropriate value for the indirect control. It must be called whenever the
	* underlying setting may have changed other than through the indirect control,
	* such as when the 'Reset all to recommended settings' button is used...
	*/
	setIndirectMaxNonImpostors();
	setIndirectMaxArc();
}

// static
void LLFloaterPreference::setIndirectMaxNonImpostors()
{
	U32 max_non_impostors = gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
	// for this one, we just need to make zero, which means off, the max value of the slider
	U32 indirect_max_non_impostors = (0 == max_non_impostors) ? LLVOAvatar::IMPOSTORS_OFF : max_non_impostors;
	gSavedSettings.setU32("IndirectMaxNonImpostors", indirect_max_non_impostors);
}

void LLFloaterPreference::setIndirectMaxArc()
{
	U32 max_arc = gSavedSettings.getU32("RenderAvatarMaxComplexity");
	U32 indirect_max_arc;
	if (0 == max_arc)
	{
		// the off position is all the way to the right, so set to control max
		indirect_max_arc = INDIRECT_MAX_ARC_OFF;
	}
	else
	{
		// This is the inverse of the calculation in updateMaxComplexity
		indirect_max_arc = (U32)((log(max_arc) - MIN_ARC_LOG) / ARC_LIMIT_MAP_SCALE) + MIN_INDIRECT_ARC_LIMIT;
	}
	gSavedSettings.setU32("IndirectMaxComplexity", indirect_max_arc);
}

void LLFloaterPreference::disableUnavailableSettings()
{	
	/*LLComboBox* ctrl_reflections   = getChild<LLComboBox>("Reflections");
	LLCheckBoxCtrl* ctrl_avatar_vp     = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	LLCheckBoxCtrl* ctrl_avatar_cloth  = getChild<LLCheckBoxCtrl>("AvatarCloth");
	LLCheckBoxCtrl* ctrl_shader_enable = getChild<LLCheckBoxCtrl>("BasicShaders");
	LLCheckBoxCtrl* ctrl_wind_light    = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLCheckBoxCtrl* ctrl_avatar_impostors = getChild<LLCheckBoxCtrl>("AvatarImpostors");
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");
	LLComboBox* ctrl_shadows = getChild<LLComboBox>("ShadowDetail");
	LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");
	LLCheckBoxCtrl* ctrl_dof = getChild<LLCheckBoxCtrl>("UseDoF");*/

	//BD - We really shouldn't disable everything twice. We might use this later tho for
	//     features that should be disabled if one or another option is missing or for
	//     options that need not just one but 2 or more previous features.
	// if vertex shaders off, disable all shader related products
	/*if (!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"))
	{
		ctrl_shader_enable->setEnabled(FALSE);
		ctrl_shader_enable->setValue(FALSE);
		
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);
		
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(0);
		
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled windlight
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders"))
	{
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);

		//deferred needs windlight, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled deferred
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") ||
		!gGLManager.mHasFramebufferObject)
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled deferred SSAO
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO"))
	{
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);
	}
	
	// disabled deferred shadows
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail"))
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
	}

	// disabled reflections
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderReflectionDetail"))
	{
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(FALSE);
	}
	
	// disabled av
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarVP"))
	{
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		//deferred needs AvatarVP, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_dof->setEnabled(FALSE);
		ctrl_dof->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled cloth
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarCloth"))
	{
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);
	}

	// disabled impostors
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderUseImpostors"))
	{
		ctrl_avatar_impostors->setEnabled(FALSE);
		ctrl_avatar_impostors->setValue(FALSE);
	}*/
}

void LLFloaterPreference::refresh()
{
	LLPanel::refresh();

	//BD - We might want to use this for later warnings of too high graphic options.
	//updateSliderText(getChild<LLSliderCtrl>("ObjectMeshDetail",		true), getChild<LLTextBox>("ObjectMeshDetailText",		true));
	
	//refreshEnabledState();
}

//BD - We might want to use this for later warnings in preferences
/*void LLFloaterPreference::onCommitWindowedMode()
{
	refresh();
}*/

//BD - Set Key dialog
void LLFloaterPreference::onClickSetAnyKey(LLUICtrl* ctrl, const LLSD& param)
{
	LLSetKeyDialog* dialog = LLFloaterReg::showTypedInstance<LLSetKeyDialog>("set_any_key", LLSD(), TRUE);
	if (dialog)
	{
		dialog->setParent(this);
		dialog->setFunction(ctrl);
		dialog->setMode(param.asInteger());
	}
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
		//gSavedSettings.setWarning(templatep->mName, TRUE);
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
	
	LLDirPicker& picker = LLDirPicker::instance();
	//Launches a directory picker and waits for feedback
	if (!picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	//Gets the path from the directory picker
	std::string dir_name = picker.getDirName();

	//Path changed
	if(proposed_name != dir_name)
	{
	gSavedPerAccountSettings.setString("InstantMessageLogPath", dir_name);
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

void LLFloaterPreference::setPersonalInfo(const std::string& visibility, bool im_via_email)
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
	getChildView("send_im_to_email")->setEnabled(TRUE);
	getChild<LLUICtrl>("send_im_to_email")->setValue(im_via_email);
	getChildView("favorites_on_login_check")->setEnabled(TRUE);
	getChildView("log_path_button")->setEnabled(TRUE);
	getChildView("chat_font_size")->setEnabled(TRUE);
}


void LLFloaterPreference::refreshUI()
{
	refresh();
}

/*void LLFloaterPreference::updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box)
{
	if (text_box == NULL || ctrl== NULL)
		return;
	
	// get range and points when text should change
	F32 value = (F32)ctrl->getValue().asReal();
	F32 min = ctrl->getMinValue();
	F32 max = ctrl->getMaxValue();
	F32 range = max - min;
	llassert(range > 0);
	F32 midPoint = min + range / 3.0f;
	F32 highPoint = min + (2.0f * range / 3.0f);
	
	// choose the right text
	if (value < midPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityLow"));
	} 
	else if (value < highPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityMid"));
	}
	else
	{
		text_box->setText(LLTrans::getString("GraphicsQualityHigh"));
	}
}*/

void LLFloaterPreference::updateMaxNonImpostors()
{
	// Called when the IndirectMaxNonImpostors control changes
	// Responsible for fixing the slider label (IndirectMaxNonImpostorsText) and setting RenderAvatarMaxNonImpostors
	LLSliderCtrl* ctrl = getChild<LLSliderCtrl>("IndirectMaxNonImpostors", true);
	U32 value = ctrl->getValue().asInteger();

	if (0 == value || LLVOAvatar::IMPOSTORS_OFF <= value)
	{
		value = 0;
	}
	gSavedSettings.setU32("RenderAvatarMaxNonImpostors", value);
	LLVOAvatar::updateImpostorRendering(value); // make it effective immediately
	setMaxNonImpostorsText(value, getChild<LLTextBox>("IndirectMaxNonImpostorsText"));
}

void LLFloaterPreference::setMaxNonImpostorsText(U32 value, LLTextBox* text_box)
{
	if (0 == value)
	{
		text_box->setText(LLTrans::getString("no_limit"));
	}
	else
	{
		text_box->setText(llformat("%d", value));
	}
}

void LLFloaterPreference::updateMaxComplexity()
{
	// Called when the IndirectMaxComplexity control changes
	// Responsible for fixing the slider label (IndirectMaxComplexityText) and setting RenderAvatarMaxComplexity
	LLSliderCtrl* ctrl = getChild<LLSliderCtrl>("IndirectMaxComplexity");
	U32 indirect_value = ctrl->getValue().asInteger();
	U32 max_arc;

	if (INDIRECT_MAX_ARC_OFF == indirect_value)
	{
		// The 'off' position is when the slider is all the way to the right, 
		// which is a value of INDIRECT_MAX_ARC_OFF,
		// so it is necessary to set max_arc to 0 disable muted avatars.
		max_arc = 0;
	}
	else
	{
		// if this is changed, the inverse calculation in setIndirectMaxArc
		// must be changed to match
		max_arc = (U32)exp(MIN_ARC_LOG + (ARC_LIMIT_MAP_SCALE * (indirect_value - MIN_INDIRECT_ARC_LIMIT)));
	}

	gSavedSettings.setU32("RenderAvatarMaxComplexity", (U32)max_arc);
	setMaxComplexityText(max_arc, getChild<LLTextBox>("IndirectMaxComplexityText"));
}

void LLFloaterPreference::setMaxComplexityText(U32 value, LLTextBox* text_box)
{
	if (0 == value)
	{
		text_box->setText(LLTrans::getString("no_limit"));
	}
	else
	{
		text_box->setText(llformat("%d", value));
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
	LLTabContainer * tab_containerp = getChild<LLTabContainer>("pref core");
	LLPanel * panel = tab_containerp->getPanelByName(name);
	if (NULL != panel)
	{
		tab_containerp->selectTabPanel(panel);
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
	mCommitCallbackRegistrar.add("Pref.PrefDelete", boost::bind(&LLPanelPreference::deletePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefSave", boost::bind(&LLPanelPreference::savePreset, this, _2));
	mCommitCallbackRegistrar.add("Pref.PrefLoad", boost::bind(&LLPanelPreference::loadPreset, this, _2));
	LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLPanelPreference::onPresetsListChange, this));
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
	if (hasChild("favorites_on_login_check", TRUE))
	{
		getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setCommitCallback(boost::bind(&handleFavoritesOnLoginChanged, _1, _2));
		//bool show_favorites_at_login = LLPanelLogin::getShowFavorites();
		//getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setValue(show_favorites_at_login);
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

void LLPanelPreference::deletePreset(const LLSD& user_data)
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

void LLPanelPreference::savePreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	std::string name = combo->getSimple();

	if (!LLPresetsManager::getInstance()->savePreset(subdirectory, name)
		&& name != "Default")
	{
		LLSD args;
		args["NAME"] = name;
		LLNotificationsUtil::add("PresetNotSaved", args);
	}
}

void LLPanelPreference::loadPreset(const LLSD& user_data)
{
	std::string subdirectory = user_data.asString();
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	std::string name = combo->getSimple();

	LLPresetsManager::getInstance()->loadPreset(subdirectory, name);
}

void LLPanelPreference::onPresetsListChange()
{
	LLComboBox* combo = getChild<LLComboBox>("preset_combo");
	LLButton* delete_btn = getChild<LLButton>("delete");

	EDefaultOptions option = DEFAULT_TOP;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox("graphic", combo, option);

	delete_btn->setEnabled(0 != combo->getItemCount());
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
	return LLPanelPreference::postBuild();
}

void LLPanelPreferenceGraphics::draw()
{
	setPresetText();
	LLPanelPreference::draw();
}

void LLPanelPreferenceGraphics::onPresetsListChange()
{
	setPresetText();
}

void LLPanelPreferenceGraphics::setPresetText()
{
	LLTextBox* preset_text = getChild<LLTextBox>("preset_text");

	std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");

	if (hasDirtyChilds() && !preset_graphic_active.empty())
	{
		gSavedSettings.setString("PresetGraphicActive", "");
		preset_graphic_active.clear();
		// This doesn't seem to cause an infinite recursion.  This trigger is needed to cause the pulldown
		// panel to update.
		LLPresetsManager::getInstance()->triggerChangeSignal();
	}

	if (!preset_graphic_active.empty())
	{
		preset_text->setText(preset_graphic_active);
	}
	else
	{
		preset_text->setText(LLTrans::getString("none_paren_cap"));
	}

	preset_text->resetDirty();
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
				return true;
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
void LLPanelPreferenceGraphics::apply()
{
	resetDirtyChilds();
	LLPanelPreference::apply();
}
void LLPanelPreferenceGraphics::cancel()
{
	resetDirtyChilds();
	LLPanelPreference::cancel();
}
void LLPanelPreferenceGraphics::saveSettings()
{
	resetDirtyChilds();
	LLPanelPreference::saveSettings();
}
void LLPanelPreferenceGraphics::setHardwareDefaults()
{
	resetDirtyChilds();
	LLPanelPreference::setHardwareDefaults();
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

