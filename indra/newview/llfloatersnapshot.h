/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc.
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

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llagent.h"
#include "llfloater.h"
#include "llviewertexture.h"
#include "llsnapshotmodel.h"

class LLSpinCtrl;
class LLComboBox;
class LLSnapshotLivePreview;
class LLToolset;
class LLFloaterBigPreview;
class LLSliderCtrl;
class LLSideTrayPanelContainer;
class LLOutfitGallery;

class LLFloaterSnapshot: public LLFloater
{
    LOG_CLASS(LLFloaterSnapshot);

public:
	LLFloaterSnapshot(const LLSD& key);
	/*virtual*/ ~LLFloaterSnapshot();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ S32 notify(const LLSD& info);
	static void update();

	static LLFloaterSnapshot* getInstance();
	static LLFloaterSnapshot* findInstance();

	typedef enum e_status
	{
		STATUS_READY,
		STATUS_WORKING,
		STATUS_FINISHED
	} EStatus;

	// Gets
	// TODO: create a snapshot model instead
	//virtual void postPanelSwitch();
	LLPointer<LLImageFormatted> getImageData();
	LLSnapshotLivePreview* getPreviewView();
	const LLVector3d& getPosTakenGlobal();
	LLPanel* getActivePanel(bool ok_if_not_found = true);
	S32 getActivePanelIndex(bool ok_if_not_found = true);
	LLSnapshotModel::ESnapshotType getActiveSnapshotType();
	LLSnapshotModel::ESnapshotFormat getImageFormat();
	std::string getSnapshotPanelPrefix();
	LLSnapshotModel::ESnapshotLayerType getLayerType();
	const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }
	LLUUID getOutfitID() { return mOutfitID; }

	void setOutfitID(LLUUID id) { mOutfitID = id; }
	void setGallery(LLOutfitGallery* gallery) { mOutfitGallery = gallery; }
	
	void onClickBigPreview();
	void onClickNewSnapshot();
	void onClickFilter(LLUICtrl *ctrl);
	void onUpdateSnapshotAndControls();
	void onCommitFreezeWorld(LLUICtrl* ctrl);
	void onCustomResolutionCommit();
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);
	void onImageQualityChange(S32 quality_val);
	void onImageFormatChange();
	void onCommitLayerTypes(LLUICtrl* ctrl);
	void onFormatComboCommit(LLUICtrl* ctrl);
	void onQualitySliderCommit(LLUICtrl* ctrl);
	void onResolutionCommit(LLUICtrl* ctrl);

	void setResolution(const std::string& comboname);
	void comboSetCustom(const std::string& comboname);
	void checkAspectRatio(S32 index);
	void applyCustomResolution(S32 w, S32 h);

	// Updating
	void updateLayout();
	void updateControls();
	void enableControls(BOOL enable);
	void updateSpinners(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed);
	void updateResolution(LLUICtrl* ctrl, BOOL do_update = TRUE);
	void updateOutfitResolution();
	void updateLivePreview();
	void updateUploadCost();
	void onExtendFloater();

	void setAdvanced(bool advanced) { mAdvanced = advanced; }
	static void setAgentEmail(const std::string& email);
	BOOL checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value);

	// Panels
	void openPanel(const std::string& panel_name);
	void onSaveToProfile();
	void onSaveToInventory();
	void onSaveToComputer();

	// Status
	void setStatus(EStatus status, bool ok = true, const std::string& msg = LLStringUtil::null);
	EStatus getStatus() const { return mStatus; }
	void setWorking(bool working);
	void setFinished(bool finished, bool ok = true, const std::string& msg = LLStringUtil::null);
	void setNeedRefresh(bool need);

	// Preview
	static BOOL updatePreviewList(bool initialized);
	void getBigPreview();
	void attachPreview();
	bool isPreviewVisible();

	// Labels
	void setRefreshLabelVisible(bool value) { mRefreshLabel->setVisible(value); }
	void setSuccessLabelPanelVisible(bool value) { mSucceessLblPanel->setVisible(value); }
	void setFailureLabelPanelVisible(bool value) { mFailureLblPanel->setVisible(value); }
	
	// Saving
	void onSnapshotUploadFinished(bool status);
	typedef boost::signals2::signal<void(void)> snapshot_saved_signal_t;
	void saveLocal(const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb);

	void saveTextureFailed();
	void onInventorySend();
	void saveLocalFinished();
	void saveLocalFailed();

	void onSnapshotCancel();
	void onSnapshotSave();
	void sendProfile();
	void saveTexture();
	void saveOutfitTexture();
	void saveLocal(LLUICtrl* ctrl);
	void postSave();


	LLComboBox* mSizeComboCtrl;
	LLComboBox* mFormatComboCtrl;
	LLSpinCtrl* mWidthSpinnerCtrl;
	LLSpinCtrl* mHeightSpinnerCtrl;
	LLUICtrl* mKeepAspectCheckCtrl;
	LLUICtrl* mSaveBtn;
	LLUICtrl* mCancelBtn;
	LLSliderCtrl* mImageQualitySliderCtrl;

	void onExtendFloater();
    void on360Snapshot();

	LLSideTrayPanelContainer* mSnapshotOptionsPanel;
	LLFloaterBigPreview* mBigPreviewFloater;
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;
	EStatus mStatus;
	LLToolset*	mLastToolset;
	LLHandle<LLView> mPreviewHandle;

	S32 mLocalFormat;

	bool mAspectRatioCheckOff;
	bool mNeedRefresh;
	bool mAdvanced;
	bool mSnapshotFreezeWorld;
	bool mSnapshotAutoscale;
	BOOL isWaitingState();

protected:

	void updateImageQualityLevel();

	LLUICtrl* mThumbnailPlaceholder;
	LLUICtrl *mRefreshBtn, *mRefreshLabel;
	LLUICtrl *mSucceessLblPanel, *mFailureLblPanel;

private:

	LLUUID mOutfitID;
	LLOutfitGallery* mOutfitGallery;

};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLFloaterView::Params>
	{
	};

protected:
	LLSnapshotFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;

#endif // LL_LLFLOATERSNAPSHOT_H
