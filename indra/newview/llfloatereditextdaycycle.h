/**
 * @file llfloatereditextdaycycle.h
 * @brief Floater to create or edit a day cycle
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLFLOATEREDITEXTDAYCYCLE_H
#define LL_LLFLOATEREDITEXTDAYCYCLE_H

#include "llfloater.h"
#include "llsettingsdaycycle.h"
#include <boost/signals2.hpp>

#include "llenvironment.h"

class LLCheckBoxCtrl;
class LLComboBox;
class LLFlyoutComboBtnCtrl;
class LLLineEditor;
class LLMultiSliderCtrl;
class LLTextBox;
class LLTimeCtrl;
class LLTabContainer;

class LLInventoryItem;
class LLDaySettingCopiedCallback;

typedef std::shared_ptr<LLSettingsBase> LLSettingsBasePtr_t;

/**
 * Floater for creating or editing a day cycle.
 */
class LLFloaterEditExtDayCycle : public LLFloater
{
    LOG_CLASS(LLFloaterEditExtDayCycle);

    friend class LLDaySettingCopiedCallback;

public:
    static const std::string    KEY_INVENTORY_ID;
    static const std::string    KEY_EDIT_CONTEXT;
    static const std::string    KEY_DAY_LENGTH;
    static const std::string    KEY_CANMOD;

    static const std::string    VALUE_CONTEXT_INVENTORY;
    static const std::string    VALUE_CONTEXT_PARCEL;
    static const std::string    VALUE_CONTEXT_REGION;

    enum edit_context_t {
        CONTEXT_UNKNOWN,
        CONTEXT_INVENTORY,
        CONTEXT_PARCEL,
        CONTEXT_REGION
    };

    typedef boost::signals2::signal<void(LLSettingsDay::ptr_t)> edit_commit_signal_t;
    typedef boost::signals2::connection connection_t;

                                LLFloaterEditExtDayCycle(const LLSD &key);
    virtual                     ~LLFloaterEditExtDayCycle();

    virtual bool                postBuild() override;
    virtual void                onOpen(const LLSD& key) override;
    virtual void                onClose(bool app_quitting) override;
    //virtual void                onFocusReceived() override;
    //virtual void                onFocusLost() override;
    virtual void                onVisibilityChange(bool new_visibility) override;

    connection_t                setEditCommitSignal(edit_commit_signal_t::slot_type cb);

    virtual void                refresh() override;

    void                        setEditDayCycle(const LLSettingsDay::ptr_t &pday);
    void                        setEditDefaultDayCycle();
    std::string                 getEditName() const;
    void                        setEditName(const std::string &name);
    LLUUID                      getEditingAssetId() { return mEditDay ? mEditDay->getAssetId() : LLUUID::null; }
    LLUUID                      getEditingInventoryId() { return mInventoryId; }

    virtual LLSettingsBase::ptr_t getEditSettings()   const { return mEditDay; }

    bool                        handleKeyUp(KEY key, MASK mask, bool called_from_parent) override;

protected:
    virtual void                setEditSettingsAndUpdate(const LLSettingsBase::ptr_t& settings);

private:
    typedef std::function<void()> on_confirm_fn;
    F32 getCurrentFrame() const;

    // flyout response/click
    void                        onButtonApply(LLUICtrl *ctrl);
    virtual void                onClickCloseBtn(bool app_quitting = false) override;
    void                        onButtonImport();
    void                        onButtonLoadFrame();
    void                        onAddFrame();
    void                        onRemoveFrame();
    void                        onCloneTrack();
    void                        onLoadTrack();
    void                        onClearTrack();
    void                        onNameKeystroke();
    void                        onTrackSelectionCallback(const LLSD& user_data);
    void                        onPlayActionCallback(const LLSD& user_data);
    //BD
    void                        onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsDay::ptr_t &day);
    // time slider clicked
    void                        onTimeSliderCallback();
    // a frame moved or frame selection changed
    void                        onFrameSliderCallback(const LLSD &);
    void                        onFrameSliderDoubleClick(S32 x, S32 y, MASK mask);
    void                        onFrameSliderMouseDown(S32 x, S32 y, MASK mask);
    void                        onFrameSliderMouseUp(S32 x, S32 y, MASK mask);

    void                        onPanelDirtyFlagChanged(bool);

    void                        checkAndConfirmSettingsLoss(on_confirm_fn cb);

    void                        cloneTrack(U32 source_index, U32 dest_index);
    void                        cloneTrack(const LLSettingsDay::ptr_t &source_day, U32 source_index, U32 dest_index);
    void                        selectTrack(U32 track_index, bool force = false);
    void                        selectFrame(F32 frame, F32 slop_factor);
    void                        clearTabs();
    void                        updateTabs();
    void                        updateWaterTabs(const LLSettingsWaterPtr_t &p_water);
    void                        updateSkyTabs(const LLSettingsSkyPtr_t &p_sky);
    void                        updateButtons();
    void                        updateLabels();
    void                        updateSlider(); //generate sliders from current track
    void                        updateTimeAndLabel();
    void                        addSliderFrame(F32 frame, const LLSettingsBase::ptr_t &setting, bool update_ui = true);
    void                        removeCurrentSliderFrame();
    void                        removeSliderFrame(F32 frame);

    void                        loadInventoryItem(const LLUUID  &inventoryId, bool can_trans = true);
    void                        onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status);

    void                        doImportFromDisk();
    void                        loadSettingFromFile(const std::vector<std::string>& filenames);
    void                        doApplyCreateNewInventory(const LLSettingsDay::ptr_t &day, std::string settings_name);
    void                        doApplyUpdateInventory(const LLSettingsDay::ptr_t &day);
    void                        doApplyEnvironment(const std::string &where, const LLSettingsDay::ptr_t &day);
    void                        doApplyCommit(LLSettingsDay::ptr_t day);
    void                        onInventoryCreated(LLUUID asset_id, LLUUID inventory_id);
    void                        onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results);
    void                        onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results);

    void                        doOpenTrackFloater(const LLSD &args);
    void                        doCloseTrackFloater(bool quitting = false);
    void                        onPickerCommitTrackId(U32 track_id);

    void                        doOpenInventoryFloater(LLSettingsType::type_e type, LLUUID curritem);
    void                        doCloseInventoryFloater(bool quitting = false);
    void                        onPickerCommitSetting(LLUUID item_id, S32 track);
    void                        onAssetLoadedForInsertion(LLUUID item_id,
                                                          LLUUID asset_id,
                                                          LLSettingsBase::ptr_t settings,
                                                          S32 status,
                                                          S32 source_track,
                                                          S32 dest_track,
                                                          LLSettingsBase::TrackPosition dest_frame);

    bool                        canUseInventory() const;
    bool                        canApplyRegion() const;
    bool                        canApplyParcel() const;

    void                        updateEditEnvironment();
    void                        synchronizeTabs();
    void                        reblendSettings();

    void                        setTabsData(LLTabContainer * tabcontainer, const LLSettingsBase::ptr_t &settings, bool editable);

    // play functions
    void                        startPlay();
    void                        stopPlay();
    static void                 onIdlePlay(void *);

    bool                        getIsDirty() const  { return mIsDirty; }
    void                        setDirtyFlag()      { mIsDirty = true; }
    virtual void                clearDirtyFlag();

    bool                        isRemovingFrameAllowed();
    bool                        isAddingFrameAllowed();

    void                        showHDRNotification(const LLSettingsDay::ptr_t &pday);

    LLSettingsDay::ptr_t        mEditDay; // edited copy
    LLSettingsDay::Seconds      mDayLength;
    U32                         mCurrentTrack;
    std::string                 mLastFrameSlider;
    bool                        mShiftCopyEnabled;

	LLUUID                      mExpectingAssetId;

    LLLineEditor*               mNameEditor;
    LLButton*                   mAddFrameButton;
    LLButton*                   mDeleteFrameButton;
    LLButton*                   mImportButton;
    LLButton*                   mLoadFrame;
    LLButton *                  mCloneTrack;
    LLButton *                  mLoadTrack;
    LLButton *                  mClearTrack;
    LLMultiSliderCtrl*          mTimeSlider;
    LLMultiSliderCtrl*          mFramesSlider;
    LLTextBox*                  mCurrentTimeLabel;
    LLUUID                      mInventoryId;
    LLInventoryItem *           mInventoryItem;
    LLFlyoutComboBtnCtrl *      mFlyoutControl;

	LLTabContainer*				mSkyTabs;
	LLTabContainer*				mWaterTabs;

    LLHandle<LLFloater>         mInventoryFloater;
    LLHandle<LLFloater>         mTrackFloater;

    LLTrackBlenderLoopingManual::ptr_t  mSkyBlender;
    LLTrackBlenderLoopingManual::ptr_t  mWaterBlender;
    LLSettingsSky::ptr_t        mScratchSky;
    LLSettingsWater::ptr_t      mScratchWater;
	//BD
	LLSettingsDay::ptr_t		mScratchDay;

    LLSettingsBase::ptr_t       mCurrentEdit;
    LLSettingsSky::ptr_t        mEditSky;
    LLSettingsWater::ptr_t      mEditWater;

    LLFrameTimer                mPlayTimer;
    F32                         mPlayStartFrame; // an env frame
    bool                        mIsPlaying;
    bool                        mIsDirty;
    bool                        mCanCopy;
    bool                        mCanMod;
    bool                        mCanTrans;
    bool                        mCanSave;

    edit_commit_signal_t        mCommitSignal;

    edit_context_t              mEditContext;

    // For map of sliders to parameters
    class FrameData
    {
    public:
        FrameData() : mFrame(0) {};
        FrameData(F32 frame, LLSettingsBase::ptr_t settings) : mFrame(frame), pSettings(settings) {};
        F32 mFrame;
        LLSettingsBase::ptr_t pSettings;
    };
    typedef std::map<std::string, FrameData> keymap_t;
    keymap_t mSliderKeyMap; //slider's keys vs old_frames&settings, shadows mFramesSlider
};

#endif // LL_LLFloaterEditExtDayCycle_H
