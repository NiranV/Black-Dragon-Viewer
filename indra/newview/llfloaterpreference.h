/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llconversationlog.h"
#include "llfiltereditor.h"
#include "lllistcontextmenu.h"
#include "llmutelist.h"
#include "llsearchableui.h"

class LLConversationLogObserver;
class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSD;
class LLTextBox;
class LLNameListCtrl;
class LLProgressBar;
class LLTabContainer;
class LLSearchableUI;
class LLSearchableControl;

typedef std::map<std::string, std::string> notifications_map;

//BD
class LLPanelVoiceDeviceSettings : public LLPanel
{
public:
	LLPanelVoiceDeviceSettings();
	~LLPanelVoiceDeviceSettings();

	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	void apply();
	void cancel();
	void refresh();
	void initialize();
	void cleanup();

	/*virtual*/ void onVisibilityChange ( BOOL new_visibility );

	void setUseTuningMode(bool use) { mUseTuningMode = use; };
	
protected:
	std::string getLocalizedDeviceName(const std::string& en_dev_name);

	void onCommitInputDevice();
	void onCommitOutputDevice();

	F32 mMicVolume;
	std::string mInputDevice;
	std::string mOutputDevice;
	class LLComboBox		*mCtrlInputDevices;
	class LLComboBox		*mCtrlOutputDevices;
	BOOL mDevicesUpdated;
	bool mUseTuningMode;
	std::map<std::string, std::string> mLocalizedDeviceNames;
};
// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver, public LLConversationLogObserver
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);
	/*virtual*/ void changed();
	/*virtual*/ void changed(const LLUUID& session_id, U32 mask) {};

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email, bool is_verified_email);
	
	// translate user's do not disturb response message according to current locale if message is default, otherwise do nothing
	static void initDoNotDisturbResponse();

	// update Show Favorites checkbox
	static void updateShowFavoritesCheckbox(bool val);

	void processProperties( void* pData, EAvatarProcessorType type );
	void processProfileProperties(const LLAvatarData* pAvatarData );
	void storeAvatarProperties( const LLAvatarData* pAvatarData );
	void saveAvatarProperties( void );
	void selectPrivacyPanel();
	void selectChatPanel();
	void getControlNames(std::vector<std::string>& names);

protected:	
	//BD
	void		onBtnOK();
	void		onBtnCancel();

	void		onClickClearCache();			// Clear viewer texture cache, vfs, and VO cache on next startup
	void		onClickBrowserClearCache();		// Clear web history and caches as well as viewer caches above
	void		onLanguageChange();
	void		onNotificationsChange(const std::string& OptionName);
	void		onNameTagOpacityChange(const LLSD& newvalue);

	// set value of "DoNotDisturbResponseChanged" in account settings depending on whether do not disturb response
	// string differs from default after user changes.
	void onDoNotDisturbResponseChanged();
	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	// callback for defaults
	void setHardwareDefaults();

	// callback for when client turns on impostors
	void onAvatarImpostorsEnable();
	
public:
	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.	
	void saveSettings();

	void setCacheLocation(const LLStringExplicit& location);

	void onClickSetCache();
	void changeCachePath(const std::vector<std::string>& filenames, std::string proposed_name);
	void onClickResetCache();
	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
	void onSelectSkin();
	void onClickSetKey();
	void setKey(KEY key);
	void onClickSetMiddleMouse();
	void onClickSetSounds();
	void onClickEnablePopup();
	void onClickDisablePopup();	
	void resetAllIgnored();
	void setAllIgnored();
	void onClickLogPath();
	void changeLogPath(const std::vector<std::string>& filenames, std::string proposed_name);
	bool moveTranscriptsAndLog();
	void enableHistory();
	void setPersonalInfo(const std::string& visibility, bool im_via_email, bool is_verified_email);
	void refreshEnabledState();
	void refresh();	// Refresh enable/disable
	
	void refreshUI();

	void onCommitParcelMediaAutoPlayEnable();
	void onCommitMediaEnabled();
	void onCommitMusicEnabled();
	void applyResolution();
	void onChangeModelFolder();
	void onChangeTextureFolder();
	void onChangeSoundFolder();
	void onChangeAnimationFolder();
	void onClickBlockList();
	void onClickProxySettings();
	void onClickTranslationSettings();
	void onClickPermsDefault();
	void onClickAutoReplace();
	void onClickSpellChecker();
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);
	void onLogChatHistorySaved();	
	void buildPopupLists();
	static void refreshSkin(void* data);
	void selectPanel(const LLSD& name);

//	//BD - Memory Allocation
	void refreshMemoryControls();

//	//BD - Unlimited Camera Presets
	void onAddCameraPreset(bool refresh = false);
	void onRemoveCameraPreset();
	void onChangeCameraPreset();
	void onCameraPresetReset(const LLSD& param);
	void onCameraArray(LLUICtrl* ctrl, const LLSD& param);
	void onFocusArray(LLUICtrl* ctrl, const LLSD& param);
	void refreshPresets();
	void refreshCameraControls();

//	//BD - Warning System
	void refreshWarnings();

//	//BD - Expandable Tabs
	void toggleTabs();

//	//BD - Revert to Default
	void resetToDefault(LLUICtrl* ctrl);

//	//BD - Custom Keyboard Layout
	void onExportControls();
	void onUnbindControls();
	void onDefaultControls();
	void refreshKeys();
	void onAddBind(KEY key, MASK mask, std::string action);
	void onRemoveBind();
	void onReplaceBind(KEY key, MASK mask);

	void onListClickAction();

	void onClickSetAnyKey();

	//BD - Toggle Input/Output Panel.
	void inputOutput();

//	//BD - Refresh Display Settings
	void refreshEverything();

//	//BD - Refresh all controls
	void refreshGraphicControls();

//	//BD - Quick Graphics Presets
	void deletePreset(const LLSD& user_data);
	void savePreset(const LLSD& user_data);
	void loadPreset(const LLSD& user_data);

//	//BD - Avatar Rendering Settings
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);

	//BD
	void onClickSetNone();
	void onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y);
	void refreshEnabledGraphics();


	void updateList();
	void onFilterEdit(const std::string& search_string);
	bool isActionChecked(const LLSD& userdata, const LLUUID& av_id);
	void onClickAdd(const LLSD& userdata);
	//BD - Allow mass changing.
	void setAvatarRenderSetting(const uuid_vec_t& av_ids, S32 new_setting);
	void onCustomAction(const LLSD& userdata);
	uuid_vec_t getRenderSettingUUIDs();

	std::string createTimestamp(S32 datetime);

	static void setNeedsUpdate();

	//void saveGraphicsPreset(std::string& preset);

private:

//	//BD - Quick Graphics Presets
	void onPresetsListChange();

	void onDeleteTranscripts();
	void onDeleteTranscriptsResponse(const LLSD& notification, const LLSD& response);
	void updateDeleteTranscriptsButton();

	static bool loadFromFilename(const std::string& filename, std::map<std::string, std::string> &label_map);
	
//	//BD - Expandable Tabs
	S32 mModifier;

	static std::string sSkin;
	notifications_map mNotificationOptions;
	bool mClickActionDirty; ///< Set to true when the click/double-click options get changed by user.
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	bool mLanguageChanged;
	bool mAvatarDataInitialized;
	std::string mPriorInstantMessageLogPath;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
	
	LLAvatarData mAvatarProperties;
	LOG_CLASS(LLFloaterPreference);

	LLFilterEditor *mFilterEdit;
	std::unique_ptr<LLSearchableUI::LLTabData> mSearchData;

	void onUpdateFilterTerm( bool force = false );
	void collectSearchableItems();

	bool mFilterCleared;
	
//	//BD - Avatar Render Settings
	bool isHiddenRow(const std::string& av_name);
	void callbackAvatarPicked(const uuid_vec_t& ids, S32 visual_setting);

	bool mNeedsUpdate;
	LLListContextMenu* mContextMenu;
	LLNameListCtrl* mAvatarSettingsList;

	std::string mNameFilter;

//	//BD - Custom Keyboard Layout
	LLScrollListCtrl* mBindModeList;

//	//BD - Warning System
	LLUICtrl* mWarning0;
	LLUICtrl* mWarning1;
	LLUICtrl* mWarning2;
	LLUICtrl* mWarning3;
	LLUICtrl* mWarning4;
	LLUICtrl* mWarning5;
	LLUICtrl* mWarning6;
	LLUICtrl* mWarning7;
	LLUICtrl* mWarning8;
	LLUICtrl* mWarning9;
	LLUICtrl* mWarning10;
	LLUICtrl* mWarning11;
	LLUICtrl* mWarning12;
	LLUICtrl* mWarning13;
	LLUICtrl* mWarning14;

//	//BD - Memory Allocation
	LLSliderCtrl*	mSystemMemory;
	LLSliderCtrl*	mSceneMemory;
	LLProgressBar*	mProgressBar;
	LLTextBox*		mGPUMemoryLabel;

	//BD
	LLFrameTimer	mUpdateTimer;
	LLTabContainer* mTabContainer;
	LLLayoutStack*	mGFXStack;

	LLButton*		mSaveBtn;
	LLButton*		mLoadBtn;
	LLButton*		mDeleteBtn;

	LLUICtrl*	mRenderSpotLightReflections;
	LLUICtrl*	mRenderSpotLightImages;
	LLUICtrl*	mRenderShadowAutomaticDistance;
	LLUICtrl*	mRenderShadowBlurSize;
	LLUICtrl*	mRenderSSRResolution;
	LLUICtrl*	mRenderSSRBrightness;
	LLUICtrl*	mRenderDepthOfFieldHighQuality;
	LLUICtrl*	mCameraFOV;
	LLUICtrl*	mCameraFNum;
	LLUICtrl*	mCameraFocal;
	LLUICtrl*	mCameraCoF;
	LLUICtrl*	mCameraFocusTrans;
	LLUICtrl*	mCameraDoFRes;
	LLUICtrl*	mRenderSSAOBlurSize;
	LLUICtrl*	mSSAOEffect;
	LLUICtrl*	mSSAOScale;
	LLUICtrl*	mSSAOMaxScale;
	LLUICtrl*	mSSAOFactor;
	LLUICtrl*	mRenderRiggedMotionBlurQuality;
	LLUICtrl*	mMotionBlurQuality;
	LLUICtrl*	mRenderGodrays;
	LLUICtrl*	mRenderGodraysDirectional;
	LLUICtrl*	mRenderGodraysResolution;
	LLUICtrl*	mRenderGodraysMultiplier;
	LLUICtrl*	mRenderGodraysFalloffMultiplier;
	LLUICtrl*	mExodusRenderToneExposure;
	LLUICtrl*	mExodusRenderColorGradeTech;
	LLUICtrl*	mExodusRenderToneMappingTech;

	std::array<LLView*, 12> mDisplayTabs;

	std::array<LLUICtrl*, 3> mRenderGlowLumWeights;
	std::array<LLUICtrl*, 3> mRenderGlowWarmthWeights;
	std::array<LLUICtrl*, 3> mExodusRenderToneAdvOptA;
	std::array<LLUICtrl*, 3> mExodusRenderToneAdvOptB;
	std::array<LLUICtrl*, 3> mExodusRenderGamma;
	std::array<LLUICtrl*, 3> mExodusRenderExposure;
	std::array<LLUICtrl*, 3> mExodusRenderOffset;
	std::array<LLUICtrl*, 3> mExodusRenderVignette;

	std::array<LLUICtrl*, 4> mRenderShadowDistance;
	std::array<LLUICtrl*, 4> mRenderShadowResolution;

	std::array<LLUICtrl*, 2> mRenderProjectorShadowResolution;
	std::array<LLUICtrl*, 2> mExodusRenderToneAdvOptC;
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ BOOL postBuild();
	
	virtual ~LLPanelPreference();

	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);
	virtual void setHardwareDefaults();

	// Disables "Allow Media to auto play" check box only when both
	// "Streaming Music" and "Media" are unchecked. Otherwise enables it.
	void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();

//	//BD - Quick Graphics Presets
	void deletePreset(const LLSD& user_data);
	void savePreset(const LLSD& user_data);
	void loadPreset(const LLSD& user_data);

	class Updater;

protected:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

private:
	//for "Only friends and groups can call or IM me"
	static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
    //for  "Allow Multiple Viewers"
    static void showMultipleViewersWarning(LLUICtrl*, const LLSD&);
	//for "Show my Favorite Landmarks at Login"
	static void handleFavoritesOnLoginChanged(LLUICtrl* checkbox, const LLSD& value);

	static void toggleMuteWhenMinimized();
	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;

	Updater* mBandWidthUpdater;
	LOG_CLASS(LLPanelPreference);
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
	BOOL postBuild();
	void draw();
	void cancel();
	void saveSettings();
	void setHardwareDefaults();
	void setPresetText();

	static const std::string getPresetsPath();

protected:
	bool hasDirtyChilds();
	//BD
	void resetDirtyChilds();

private:

	void onPresetsListChange();
	LOG_CLASS(LLPanelPreferenceGraphics);
};

class LLFloaterPreferenceProxy : public LLFloater
{
public: 
	LLFloaterPreferenceProxy(const LLSD& key);
	~LLFloaterPreferenceProxy();

	/// show off our menu
	static void show();
	void cancel();
	
protected:
	BOOL postBuild();
	void onOpen(const LLSD& key);
	void onClose(bool app_quitting);
	void saveSettings();
	void onBtnOk();
	void onBtnCancel();
	void onClickCloseBtn(bool app_quitting = false);

	void onChangeSocksSettings();

private:
	
	bool mSocksSettingsDirty;
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;
	LOG_CLASS(LLFloaterPreferenceProxy);
};


#endif  // LL_LLPREFERENCEFLOATER_H
