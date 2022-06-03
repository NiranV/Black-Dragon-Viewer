/** 
 * @file llfloatersnapshot.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"

#include "llfloaterbigpreview.h"
#include "llfloaterreg.h"
#include "llimagefiltersmanager.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llresmgr.h"		// LLLocale
#include "llsdserialize.h"
#include "llsidetraypanelcontainer.h"
#include "llsnapshotlivepreview.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llwebprofile.h"
#include "llscrolllistctrl.h"

#include "llagentbenefits.h"
#include "lltrans.h"
#include "llagentui.h"
#include "llsliderctrl.h"
#include "llnotificationsutil.h"
#include "lltexteditor.h"
#include "llagent.h"
#include "llstatusbar.h"
#include "llviewerregion.h"
#include "lloutfitgallery.h"

//BD
#include "pipeline.h"

#include <boost/regex.hpp>

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
LLSnapshotFloaterView* gSnapshotFloaterView = NULL;

const S32 MAX_TEXTURE_SIZE = 1024 ; //max upload texture size 1024 * 1024

const S32 OUTFIT_SNAPSHOT_WIDTH = 256;
const S32 OUTFIT_SNAPSHOT_HEIGHT = 256;

static LLDefaultChildRegistry::Register<LLSnapshotFloaterView> r("snapshot_floater_view");

S32 power_of_two(S32 sz, S32 upper)
{
	S32 res = upper;
	while (upper >= sz)
	{
		res = upper;
		upper >>= 1;
	}
	return res;
}

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot(const LLSD& key)
	: LLFloater(key),
	mAvatarPauseHandles(),
	mLastToolset(NULL),
	mAspectRatioCheckOff(false),
	mNeedRefresh(false),
	mStatus(STATUS_READY),
	mBigPreviewFloater(NULL),
	mRefreshBtn(NULL),
	mRefreshLabel(NULL),
	mSucceessLblPanel(NULL),
	mFailureLblPanel(NULL),
	mOutfitGallery(NULL),
	//BD
	mSnapshotAutoscale(false)
{
	//BD - Profile
	mCommitCallbackRegistrar.add("Snapshot.SendProfile", boost::bind(&LLFloaterSnapshot::sendProfile, this));
	//BD - Local
	mLocalFormat = gSavedSettings.getS32("SnapshotFormat");
	mCommitCallbackRegistrar.add("Snapshot.SaveLocal", boost::bind(&LLFloaterSnapshot::saveLocal, this,_1));
	//BD - Inventory
	mCommitCallbackRegistrar.add("Snapshot.SaveTexture", boost::bind(&LLFloaterSnapshot::onInventorySend, this));
	//BD - Outfit
	mCommitCallbackRegistrar.add("Inventory.SaveOutfitPhoto", boost::bind(&LLFloaterSnapshot::onInventorySend, this));

	//BD - Options
	mCommitCallbackRegistrar.add("Snapshot.SaveToProfile", boost::bind(&LLFloaterSnapshot::onSaveToProfile, this));
	mCommitCallbackRegistrar.add("Snapshot.SaveToInventory", boost::bind(&LLFloaterSnapshot::onSaveToInventory, this));
	mCommitCallbackRegistrar.add("Snapshot.SaveToComputer", boost::bind(&LLFloaterSnapshot::onSaveToComputer, this));

	mCommitCallbackRegistrar.add("Snapshot.Save", boost::bind(&LLFloaterSnapshot::onSnapshotSave, this));
	mCommitCallbackRegistrar.add("Snapshot.Cancel", boost::bind(&LLFloaterSnapshot::onSnapshotCancel, this));

	mCommitCallbackRegistrar.add("Snapshot.ResolutionCombo", boost::bind(&LLFloaterSnapshot::updateResolution, this, _1, true));
	mCommitCallbackRegistrar.add("Snapshot.Format", boost::bind(&LLFloaterSnapshot::onFormatComboCommit, this, _1));
	mCommitCallbackRegistrar.add("Snapshot.CustomResolution", boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mCommitCallbackRegistrar.add("Snapshot.CustomResolution", boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mCommitCallbackRegistrar.add("Snapshot.AspectRatio", boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));
	mCommitCallbackRegistrar.add("Snapshot.ImageQuality", boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));

	mCommitCallbackRegistrar.add("SocialSharing.BigPreview", boost::bind(&LLFloaterSnapshot::onClickBigPreview, this));
}

LLFloaterSnapshot::~LLFloaterSnapshot()
{
	if (mPreviewHandle.get()) mPreviewHandle.get()->die();

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(mLastToolset);
	}

	mAvatarPauseHandles.clear();
}

// virtual
BOOL LLFloaterSnapshot::postBuild()
{
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	mRefreshBtn->setCommitCallback(boost::bind(&LLFloaterSnapshot::onClickNewSnapshot, this));
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");
	mSnapshotOptionsPanel = getChild<LLSideTrayPanelContainer>("panel_container");

	mSizeComboCtrl = getChild<LLComboBox>("local_size_combo");
	mSizeComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateResolution, this, _1, true));
	mFormatComboCtrl = getChild<LLComboBox>("local_format_combo");
	mFormatComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onFormatComboCommit, this, _1));
	mWidthSpinnerCtrl = getChild<LLSpinCtrl>("local_snapshot_width");
	mWidthSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mHeightSpinnerCtrl = getChild<LLSpinCtrl>("local_snapshot_height");
	mHeightSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mKeepAspectCheckCtrl = getChild<LLUICtrl>("local_keep_aspect_check");
	mKeepAspectCheckCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));
	mImageQualitySliderCtrl = getChild<LLSliderCtrl>("image_quality_slider");
	mSaveBtn = getChild<LLUICtrl>("save_btn");
	mCancelBtn = getChild<LLUICtrl>("cancel_btn");

	getChild<LLUICtrl>("ui_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onUpdateSnapshotAndControls, this));
	getChild<LLUICtrl>("hud_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onUpdateSnapshotAndControls, this));
	getChild<LLUICtrl>("layer_types")->setValue("colors");
	getChild<LLUICtrl>("layer_types")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCommitLayerTypes, this, _1));
	getChild<LLUICtrl>("layer_types")->setEnabled(FALSE);
	//BD
	getChild<LLUICtrl>("freeze_world_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCommitFreezeWorld, this, _1));
	getChild<LLUICtrl>("autoscale_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onUpdateSnapshotAndControls, this));
	getChild<LLUICtrl>("unlock_resolutions")->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateControls, this));
	getChild<LLButton>("retract_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));
	getChild<LLButton>("extend_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));

	// Pre-select "Current Window" resolution.
	getChild<LLComboBox>("profile_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("texture_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("local_format_combo")->selectNthItem(0);
	setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));

	//BD - Local
	//getChild<LLUICtrl>("local_format_combo")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onFormatComboCommit, this, _1));

	//BD - Inventory
	getChild<LLSpinCtrl>("inventory_snapshot_width")->setAllowEdit(FALSE);
	getChild<LLSpinCtrl>("inventory_snapshot_height")->setAllowEdit(FALSE);
	getChild<LLUICtrl>("texture_size_combo")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onResolutionCommit, this, _1));

	// Filters
	LLComboBox* filterbox = getChild<LLComboBox>("filters_combobox");
	std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
	for (U32 i = 0; i < filter_list.size(); i++)
	{
		filterbox->add(filter_list[i]);
	}
	filterbox->setCommitCallback(boost::bind(&LLFloaterSnapshot::onClickFilter, this, _1));

	LLWebProfile::setImageUploadResultCallback(boost::bind(&LLFloaterSnapshot::onSnapshotUploadFinished, this, _1));


	// create preview window
	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	LLView* parent_view = gSnapshotFloaterView->getParent();
	parent_view->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	parent_view->addChild(previewp);
	parent_view->addChild(gSnapshotFloaterView);

	//move snapshot floater to special purpose snapshotfloaterview
	gFloaterView->removeChild(this);
	gSnapshotFloaterView->addChild(this);

	
	//BD
	mSnapshotFreezeWorld = false;
	mSnapshotAutoscale = false;

	getBigPreview();
	mPreviewHandle = previewp->getHandle();
	previewp->setContainer(this);
	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());
	updateControls();

	//BD - Exception for local, select custom by default, current window is the same anyway.
	LLComboBox* combo = getChild<LLComboBox>("local_size_combo");
	combo->selectNthItem(combo->getItemCount() - 1);

	return TRUE;
}

// virtual
void LLFloaterSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	LLFloater::draw();

	if (previewp && !isMinimized() && mThumbnailPlaceholder->getVisible())
	{
		if (previewp->getThumbnailImage())
		{
			bool working = getStatus() == LLFloaterSnapshot::STATUS_WORKING;
			const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
			S32 thumbnail_w = previewp->getEncodedImageWidth();
			S32 thumbnail_h = previewp->getEncodedImageHeight();

			F32 ratio = llmax((F32)(thumbnail_w) / (F32)(thumbnail_rect.getWidth()), (F32)(thumbnail_h) / (F32)(thumbnail_rect.getHeight()));
			thumbnail_w = (S32)((F32)(thumbnail_w) / ratio);
			thumbnail_h = (S32)((F32)(thumbnail_h) / ratio);

			// calc preview offset within the preview rect
			const S32 local_offset_x = (thumbnail_rect.getWidth() - thumbnail_w) / 2;
			const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2; // preview y pos within the preview rect

			// calc preview offset within the floater rect
			S32 offset_x = thumbnail_rect.mLeft + local_offset_x;
			S32 offset_y = thumbnail_rect.mBottom + local_offset_y;

			gGL.matrixMode(LLRender::MM_MODELVIEW);
			// Apply floater transparency to the texture unless the floater is focused.
			F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
			LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
			gl_draw_scaled_image(offset_x, offset_y,
				thumbnail_w, thumbnail_h,
				previewp->getBigThumbnailImage(), color % alpha);

			//previewp->drawPreviewRect(offset_x, offset_y) ;

			gGL.pushUIMatrix();
			LLUI::translate((F32)thumbnail_rect.mLeft, (F32)thumbnail_rect.mBottom);
			mThumbnailPlaceholder->draw();
			gGL.popUIMatrix();
		}
	}
	updateLayout();
}

//virtual
void LLFloaterSnapshot::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = getPreviewView();
	if (preview)
	{
		// _LL_DEBUGS() << "opened, updating snapshot" << LL_ENDL;
		preview->setAllowFullScreenPreview(TRUE);
		preview->updateSnapshot(TRUE);
	}
	focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

	//BD
	if (mSnapshotFreezeWorld)
	{
		gSavedSettings.setBOOL("UseFreezeWorld", true);
	}

	updateControls();
	setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
	//BD - Autoscale Rendering
	gSavedSettings.setBOOL("RenderSnapshotAutoAdjustMultiplier", mSnapshotAutoscale);


	// Initialize default tab.
	getChild<LLSideTrayPanelContainer>("panel_container")->getCurrentPanel()->onOpen(LLSD());

	//BD
	S32 old_format = gSavedSettings.getS32("SnapshotFormat");
	S32 new_format = (S32)getImageFormat();

	setCtrlsEnabled(true);

	// Switching panels will likely change image format.
	// Not updating preview right away may lead to errors,
	// e.g. attempt to send a large BMP image by email.
	if (old_format != new_format)
	{
		onImageFormatChange();
	}

	//BD - Local
	if (gSavedSettings.getS32("SnapshotFormat") != mLocalFormat)
	{
		getChild<LLComboBox>("local_format_combo")->selectNthItem(mLocalFormat);
	}

	//BD - Options
	updateUploadCost();
}

void LLFloaterSnapshot::onExtendFloater()
{
	setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
}

//virtual
void LLFloaterSnapshot::onClose(bool app_quitting)
{
	getParent()->setMouseOpaque(FALSE);

	//unfreeze everything, hide fullscreen preview
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setAllowFullScreenPreview(FALSE);
		previewp->setVisible(FALSE);
		previewp->setEnabled(FALSE);
	}

	//BD - Autoscale Rendering
	//     When closing the window, automatically disable the autoscale option or we
	//     get temporarily stuck with it until we disable it again and this might cause
	//     confusion for the user who doesn't know what he might have missed.
	mSnapshotAutoscale = gPipeline.RenderSnapshotAutoAdjustMultiplier;
	gSavedSettings.setBOOL("RenderSnapshotAutoAdjustMultiplier", false);

	//BD
	if (mSnapshotFreezeWorld)
	{
		gSavedSettings.setBOOL("UseFreezeWorld", false);
	}

	if (mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(mLastToolset);
	}

	if (mSnapshotOptionsPanel && !gSavedSettings.getBOOL("RememberSnapshotMode"))
	{
		mSnapshotOptionsPanel->openPreviousPanel();
		mSnapshotOptionsPanel->getCurrentPanel()->onOpen(LLSD());
	}
	setStatus(LLFloaterSnapshot::STATUS_READY, true);
}

//virtual 
void LLFloaterSnapshot::updateLayout()
{
	LLUICtrl* thumbnail_placeholder = getChild<LLUICtrl>("thumbnail_placeholder");
	F32 aspect_ratio = mWidthSpinnerCtrl->getValue().asReal() / mHeightSpinnerCtrl->getValue().asReal();
	//BD - Automatically calculate the size of our snapshot window to enlarge
	//     the snapshot preview to its maximum size, this is especially helpfull
	//     for pretty much every aspect ratio other than 1:1.
	F32 panel_width = 400.f * llmin(aspect_ratio, gViewerWindow->getWorldViewAspectRatio());

	//BD - Make sure we clamp at 700 here because 700 would be for 16:9 which we
	//     consider the maximum. Everything bigger will be clamped and will have
	//     a slightly smaller preview window which most likely won't fill up the
	//     whole snapshot floater as it should.
	if (panel_width > 700.f)
	{
		panel_width = 700.f;
	}

	S32 floater_width = 224.f;
	if (mAdvanced)
	{
		floater_width = floater_width + panel_width;
	}

	thumbnail_placeholder->setVisible(mAdvanced);
	thumbnail_placeholder->reshape(panel_width, thumbnail_placeholder->getRect().getHeight());
	getChild<LLUICtrl>("file_size_label")->setVisible(mAdvanced);
	if (!isMinimized())
    if (floaterp->hasChild("360_label", TRUE))
    { 
        floaterp->getChild<LLUICtrl>("360_label")->setVisible(mAdvanced);
    }
	if(!floaterp->isMinimized())
	{
		reshape(floater_width, getRect().getHeight());
	}
}


// virtual
LLPanel* LLFloaterSnapshot::getActivePanel(bool ok_if_not_found)
{
	LLPanel* panel = mSnapshotOptionsPanel->getCurrentPanel();
	if (panel->getName() == "panel_snapshot_options")
	{
		return NULL;
	}
	return panel;
}

// virtual
S32 LLFloaterSnapshot::getActivePanelIndex(bool ok_if_not_found)
{
	S32 idx = mSnapshotOptionsPanel->getCurrentPanelIndex();
	return idx;
}

// virtual
LLSnapshotModel::ESnapshotType LLFloaterSnapshot::getActiveSnapshotType()
{
	LLSnapshotModel::ESnapshotType type = LLSnapshotModel::SNAPSHOT_WEB;
	switch (getActivePanelIndex())
	{
	//BD - Outfit
	case 4:
	//BD - Inventory
	case 2:
		type = LLSnapshotModel::SNAPSHOT_TEXTURE;
		break;
	//BD - Local
	case 3:
		type = LLSnapshotModel::SNAPSHOT_LOCAL;
		break;
	//BD - Options
	case 0:
	//BD - Profile
	case 1:
	default:
		type = LLSnapshotModel::SNAPSHOT_WEB;
		break;
	}
	return type;
}

// virtual
LLSnapshotModel::ESnapshotFormat LLFloaterSnapshot::getImageFormat()
{
	LLSnapshotModel::ESnapshotFormat format = LLSnapshotModel::SNAPSHOT_FORMAT_PNG;

	S32 idx = getActivePanelIndex();
	if (idx == 2)
	{
		//BD - Inventory
		format = LLSnapshotModel::SNAPSHOT_FORMAT_JPEG;
	}
	else if (idx == 3)
	{
		//BD - Local
		std::string id = mFormatComboCtrl->getValue().asString();
		if (id == "PNG")
			format = LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
		else if (id == "JPEG")
			format = LLSnapshotModel::SNAPSHOT_FORMAT_JPEG;
		else if (id == "BMP")
			format = LLSnapshotModel::SNAPSHOT_FORMAT_BMP;
	}
	else
	{
		format = LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
	}
	return format;
}

LLSnapshotLivePreview* LLFloaterSnapshot::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

// virtual
LLSnapshotModel::ESnapshotLayerType LLFloaterSnapshot::getLayerType()
{
	LLSnapshotModel::ESnapshotLayerType type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
	LLSD value = getChild<LLUICtrl>("layer_types")->getValue();
	const std::string id = value.asString();
	if (id == "colors")
		type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
	else if (id == "depth")
		type = LLSnapshotModel::SNAPSHOT_TYPE_DEPTH;
	return type;
}

void LLFloaterSnapshot::setResolution(const std::string& comboname)
{
	LLComboBox* combo = getChild<LLComboBox>(comboname);
		combo->setVisible(TRUE);
	updateResolution(combo, FALSE); // to sync spinners with combo
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// FIXME: The above comment seems obsolete now.
// virtual
void LLFloaterSnapshot::updateControls()
{
	LLSnapshotModel::ESnapshotType shot_type = getActiveSnapshotType();
	LLSnapshotModel::ESnapshotFormat shot_format = (LLSnapshotModel::ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
	LLSnapshotModel::ESnapshotLayerType layer_type = getLayerType();

	getChildView("layer_types")->setEnabled(shot_type == LLSnapshotModel::SNAPSHOT_LOCAL);

	if(getActivePanel())
	{
		// Initialize spinners.
		if (mWidthSpinnerCtrl->getValue().asInteger() == 0)
		{
			S32 w = gViewerWindow->getWindowWidthRaw();
			// _LL_DEBUGS() << "Initializing width spinner (" << width_ctrl->getName() << "): " << w << LL_ENDL;
			mWidthSpinnerCtrl->setValue(w);
			if (getActiveSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
			{
				mWidthSpinnerCtrl->setIncrement(w >> 1);
			}
		}
		if (mHeightSpinnerCtrl->getValue().asInteger() == 0)
		{
			S32 h = gViewerWindow->getWindowHeightRaw();
			// _LL_DEBUGS() << "Initializing height spinner (" << height_ctrl->getName() << "): " << h << LL_ENDL;
			mHeightSpinnerCtrl->setValue(h);
			if (getActiveSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
			{
				mHeightSpinnerCtrl->setIncrement(h >> 1);
			}
		}

		//BD
		if (getActivePanelIndex() == 4)
		{
			//BD
			bool unlock = gSavedSettings.getBOOL("SnapshotResolutionUnlock");
			mSizeComboCtrl->getItemByLabel("6144x3160 (6K)")->setEnabled(unlock);
			mSizeComboCtrl->getItemByLabel("7680x4320 (8K)")->setEnabled(unlock);
			mSizeComboCtrl->getItemByLabel("11520x6480 (12K)")->setEnabled(unlock);

			// Clamp snapshot resolution to window size when showing UI or HUD in snapshot.
			if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
			{
				S32 width = gViewerWindow->getWindowWidthRaw();
				S32 height = gViewerWindow->getWindowHeightRaw();

				mWidthSpinnerCtrl->setMaxValue(width);

				mHeightSpinnerCtrl->setMaxValue(height);

				if (mWidthSpinnerCtrl->getValue().asInteger() > width)
				{
					mWidthSpinnerCtrl->forceSetValue(width);
				}
				if (mHeightSpinnerCtrl->getValue().asInteger() > height)
				{
					mHeightSpinnerCtrl->forceSetValue(height);
				}
			}
			else
			{
				//BD
				mWidthSpinnerCtrl->setMaxValue(unlock ? 11520 : 3840);
				mHeightSpinnerCtrl->setMaxValue(unlock ? 11520 : 3840);
			}
		}

		mFormatComboCtrl->selectNthItem((S32)shot_format);
		const bool show_quality_ctrls = (shot_format == LLSnapshotModel::SNAPSHOT_FORMAT_JPEG);
		mImageQualitySliderCtrl->setVisible(show_quality_ctrls);
		getActivePanel()->getChild<LLUICtrl>("image_quality_level")->setVisible(show_quality_ctrls);
		mImageQualitySliderCtrl->setValue(gSavedSettings.getS32("SnapshotQuality"));
		updateImageQualityLevel();
	}
		
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

	// *TODO: Separate maximum size for Web images from postcards
	// _LL_DEBUGS() << "Is snapshot up-to-date? " << got_snap << LL_ENDL;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_snap)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}

	// Update displayed image resolution.
	LLTextBox* file_info_label = getChild<LLTextBox>("file_size_label");
	file_info_label->setTextArg("[WIDTH]", got_snap ? llformat("%d", previewp->getEncodedImageWidth()) : getString("unknown"));
	file_info_label->setTextArg("[HEIGHT]", got_snap ? llformat("%d", previewp->getEncodedImageHeight()) : getString("unknown"));
	file_info_label->setTextArg("[SIZE]", got_snap ? bytes_string : getString("unknown"));
	file_info_label->setColor(LLUIColorTable::instance().getColor( "LabelTextColor" ));

	// Update the width and height spinners based on the corresponding resolution combos. (?)
	switch(shot_type)
	{
	  case LLSnapshotModel::SNAPSHOT_WEB:
		layer_type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
		getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution("profile_size_combo");
		break;
	  case LLSnapshotModel::SNAPSHOT_TEXTURE:
		layer_type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
		getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution("texture_size_combo");
		break;
	  case  LLSnapshotModel::SNAPSHOT_LOCAL:
		setResolution("local_size_combo");
		break;
	  default:
		break;
	}

	if (previewp)
	{
		previewp->setSnapshotType(shot_type);
		previewp->setSnapshotFormat(shot_format);
		previewp->setSnapshotBufferType(layer_type);
	}

	LLPanel* current_panel = getActivePanel();
	if (current_panel)
	{
		LLSD info;
		info["have-snapshot"] = got_snap;
		mSaveBtn->setEnabled(got_snap);
	}
}

//virtual
void LLFloaterSnapshot::setStatus(EStatus status, bool ok, const std::string& msg)
{
	switch (status)
	{
	case STATUS_READY:
		setWorking(false);
		setFinished(false);
		break;
	case STATUS_WORKING:
		setWorking(true);
		setFinished(false);
		break;
	case STATUS_FINISHED:
		setWorking(false);
		setFinished(true, ok, msg);
		break;
	}

	mStatus = status;
}

// virtual
void LLFloaterSnapshot::setNeedRefresh(bool need)
{
	setRefreshLabelVisible(need);
	mNeedRefresh = need;
}

// static
void LLFloaterSnapshot::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		setStatus(STATUS_READY);
		// _LL_DEBUGS() << "updating snapshot" << LL_ENDL;
		previewp->mForceUpdateSnapshot = TRUE;
	}
}

// static
void LLFloaterSnapshot::onClickFilter(LLUICtrl *ctrl)
{
	updateControls();
	LLSnapshotLivePreview* previewp = getPreviewView();
    if (previewp)
    {
        // Note : index 0 of the filter drop down is assumed to be "No filter" in whichever locale
        LLComboBox* filterbox = static_cast<LLComboBox *>(getChild<LLComboBox>("filters_combobox"));
        std::string filter_name = (filterbox->getCurrentIndex() ? filterbox->getSimple() : "");
        previewp->setFilter(filter_name);
        previewp->updateSnapshot(TRUE);
    }
}

// static
void LLFloaterSnapshot::onUpdateSnapshotAndControls()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->updateSnapshot(TRUE);
	}
	updateControls();
}



void LLFloaterSnapshot::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	BOOL checked = ctrl->getValue().asBoolean();

	LLPanel* active_panel = getActivePanel();
	if (checked && active_panel)
	{
		mSizeComboCtrl->setCurrentByIndex(mSizeComboCtrl->getItemCount() - 1); // "custom" is always the last index
	}

	LLSnapshotLivePreview* previewp = getPreviewView() ;
	if(previewp)
	{
		previewp->mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;

		S32 w, h ;
		previewp->getSize(w, h) ;
		updateSpinners(previewp, w, h, TRUE); // may change w and h

		// _LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
		previewp->setSize(w, h) ;
		previewp->updateSnapshot(TRUE);
	}
}

//BD
// static
void LLFloaterSnapshot::onCommitFreezeWorld(LLUICtrl* ctrl)
{
	mSnapshotFreezeWorld = ctrl->getValue().asBoolean();
}


// static
void LLFloaterSnapshot::checkAspectRatio(S32 index)
{
	LLSnapshotLivePreview *previewp = getPreviewView();

	// Don't round texture sizes; textures are commonly stretched in world, profiles, etc and need to be "squashed" during upload, not cropped here
	if (LLSnapshotModel::SNAPSHOT_TEXTURE == getActiveSnapshotType())
	{
		previewp->mKeepAspectRatio = FALSE ;
		return ;
	}

	BOOL keep_aspect = FALSE, enable_cb = FALSE;

	if (0 == index) // current window size
	{
		enable_cb = FALSE;
		keep_aspect = TRUE;
	}
	else if (-1 == index) // custom
	{
		enable_cb = TRUE;
		keep_aspect = gSavedSettings.getBOOL("KeepAspectForSnapshot");
	}
	else // predefined resolution
	{
		enable_cb = FALSE;
		keep_aspect = FALSE;
	}

	mAspectRatioCheckOff = !enable_cb;

	if (previewp)
	{
		previewp->mKeepAspectRatio = keep_aspect;
	}
}

// Show/hide upload progress indicators.
void LLFloaterSnapshot::setWorking(bool working)
{
	LLUICtrl* working_lbl = getChild<LLUICtrl>("working_lbl");
	working_lbl->setVisible(working);
	getChild<LLUICtrl>("working_indicator")->setVisible(working);

	if (working)
	{
		const std::string panel_name = getActivePanel(false)->getName();
		const std::string prefix = panel_name.substr(getSnapshotPanelPrefix().size());
		std::string progress_text = getString(prefix + "_" + "progress_str");
		working_lbl->setValue(progress_text);
	}

	// All controls should be disabled while posting.
	setCtrlsEnabled(!working);
}

//virtual
std::string LLFloaterSnapshot::getSnapshotPanelPrefix()
{
	return "panel_snapshot_";
}

// Show/hide upload status message.
// virtual
void LLFloaterSnapshot::setFinished(bool finished, bool ok, const std::string& msg)
{
	setSuccessLabelPanelVisible(finished && ok);
	setFailureLabelPanelVisible(finished && !ok);

	if (finished)
	{
		LLUICtrl* finished_lbl = getChild<LLUICtrl>(ok ? "succeeded_lbl" : "failed_lbl");
		std::string result_text = getString(msg + "_" + (ok ? "succeeded_str" : "failed_str"));
		finished_lbl->setValue(result_text);

		if (!gSavedSettings.getBOOL("RememberSnapshotMode"))
		{
			LLSideTrayPanelContainer* panel_container = getChild<LLSideTrayPanelContainer>("panel_container");
			panel_container->openPreviousPanel();
			panel_container->getCurrentPanel()->onOpen(LLSD());
		}
	}
}

// Apply a new resolution selected from the given combobox.
void LLFloaterSnapshot::updateResolution(LLUICtrl* ctrl, BOOL do_update)
{
	S32 idx = getActivePanelIndex();
	if (idx != 5)
	{
		LLComboBox* combobox = (LLComboBox*)ctrl;
		if (!combobox)
		{
			llassert(combobox);
			return;
		}

		std::string sdstring = combobox->getSelectedValue();
		LLSD sdres;
		std::stringstream sstream(sdstring);
		LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());

		S32 width = sdres[0];
		S32 height = sdres[1];

		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp && combobox->getCurrentIndex() >= 0)
		{
			S32 original_width = 0, original_height = 0;
			previewp->getSize(original_width, original_height);

			if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
			{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
				width = llmin(width, gViewerWindow->getWindowWidthRaw());
				height = llmin(height, gViewerWindow->getWindowHeightRaw());
			}

			if (width == 0 || height == 0)
			{
				// take resolution from current window size
				// _LL_DEBUGS() << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << LL_ENDL;
				previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
			}
			else if (width == -1 || height == -1)
			{
				// load last custom value
				S32 new_width = 0, new_height = 0;
				LLPanel* spanel = getActivePanel();
				if (spanel)
				{
					// _LL_DEBUGS() << "Loading typed res from panel " << spanel->getName() << LL_ENDL;
					new_width = mWidthSpinnerCtrl->getValue().asInteger();
					new_height = mHeightSpinnerCtrl->getValue().asInteger();

					// Limit custom size for inventory snapshots to 512x512 px.
					if (getActiveSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
					{
						new_width = llmin(new_width, MAX_TEXTURE_SIZE);
						new_height = llmin(new_height, MAX_TEXTURE_SIZE);
					}
				}
				else
				{
					/*// _LL_DEBUGS() << "No custom res chosen, setting preview res from window: "
						<< gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << LL_ENDL;*/
					new_width = gViewerWindow->getWindowWidthRaw();
					new_height = gViewerWindow->getWindowHeightRaw();
				}

				llassert(new_width > 0 && new_height > 0);
				previewp->setSize(new_width, new_height);
			}
			else
			{
				// use the resolution from the selected pre-canned drop-down choice
				// _LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
				previewp->setSize(width, height);
			}

			checkAspectRatio(width);

			previewp->getSize(width, height);

			//BD - Autoscale Rendering
			if (gPipeline.RenderSnapshotAutoAdjustMultiplier)
			{
				F32 window_height = gViewerWindow->getWindowHeightRaw();
				F32 multiplier = (F32)height / window_height;
				gSavedSettings.setF32("RenderSnapshotMultiplier", multiplier);
			}

			// We use the height spinner here because we come here via the aspect ratio
			// checkbox as well and we want height always changing to width by default.
			// If we use the width spinner we would change width according to height by
			// default, that is not what we want.
			updateSpinners(previewp, width, height, !mHeightSpinnerCtrl->isDirty()); // may change width and height

			if (mWidthSpinnerCtrl->getValue().asInteger() != width || mHeightSpinnerCtrl->getValue().asInteger() != height)
			{
				mWidthSpinnerCtrl->setValue(width);
				mHeightSpinnerCtrl->setValue(height);
				if (getActiveSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
				{
					mWidthSpinnerCtrl->setIncrement(width >> 1);
					mHeightSpinnerCtrl->setIncrement(height >> 1);
				}
			}

			if (original_width != width || original_height != height)
			{
				previewp->setSize(width, height);

				// hide old preview as the aspect ratio could be wrong
				// _LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
				// Don't update immediately, give window chance to redraw
				getPreviewView()->updateSnapshot(TRUE, FALSE, 1.f);
				if (do_update)
				{
					// _LL_DEBUGS() << "Will update controls" << LL_ENDL;
					updateControls();
				}
			}
		}
	}
	//BD - Special case, outfit snapshot does not have a combobox.
	else
	{
		S32 width = OUTFIT_SNAPSHOT_WIDTH;
		S32 height = OUTFIT_SNAPSHOT_HEIGHT;

		LLSnapshotLivePreview* previewp = getPreviewView();
		if (previewp)
		{
			S32 original_width = 0, original_height = 0;
			previewp->getSize(original_width, original_height);

			if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
			{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
				width = llmin(width, gViewerWindow->getWindowWidthRaw());
				height = llmin(height, gViewerWindow->getWindowHeightRaw());
			}


			llassert(width > 0 && height > 0);

			// use the resolution from the selected pre-canned drop-down choice
			// _LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
			previewp->setSize(width, height);

			if (original_width != width || original_height != height)
			{
				// hide old preview as the aspect ratio could be wrong
				// _LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
				previewp->updateSnapshot(TRUE);
			}
		}
	}
}

// static
void LLFloaterSnapshot::onCommitLayerTypes(LLUICtrl* ctrl)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setSnapshotBufferType((LLSnapshotModel::ESnapshotLayerType)combobox->getCurrentIndex());
	}
	previewp->updateSnapshot(TRUE, TRUE);
}

void LLFloaterSnapshot::onImageQualityChange(S32 quality_val)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
}

void LLFloaterSnapshot::onImageFormatChange()
{
	// _LL_DEBUGS() << "image format changed, updating snapshot" << LL_ENDL;
	getPreviewView()->updateSnapshot(TRUE);
	updateControls();
}

// Sets the named size combo to "custom" mode.
void LLFloaterSnapshot::comboSetCustom(const std::string& comboname)
{
	LLComboBox* combo = getChild<LLComboBox>(comboname);
	combo->setCurrentByIndex(combo->getItemCount() - 1); // "custom" is always the last index
	checkAspectRatio(-1); // -1 means custom
}

// Update supplied width and height according to the constrain proportions flag; limit them by max_val.
BOOL LLFloaterSnapshot::checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value)
{
	S32 w = width ;
	S32 h = height ;

	if(previewp && previewp->mKeepAspectRatio)
	{
		if(gViewerWindow->getWindowWidthRaw() < 1 || gViewerWindow->getWindowHeightRaw() < 1)
		{
			return FALSE ;
		}

		//aspect ratio of the current window
		F32 aspect_ratio = (F32)gViewerWindow->getWindowWidthRaw() / gViewerWindow->getWindowHeightRaw() ;

		//change another value proportionally
		if(isWidthChanged)
		{
			height = ll_round(width / aspect_ratio) ;
		}
		else
		{
			width = ll_round(height * aspect_ratio) ;
		}

		//bound w/h by the max_value
		if(width > max_value || height > max_value)
		{
			if(width > height)
			{
				width = max_value ;
				height = (S32)(width / aspect_ratio) ;
			}
			else
			{
				height = max_value ;
				width = (S32)(height * aspect_ratio) ;
			}
		}
	}

	return (w != width || h != height) ;
}

void LLFloaterSnapshot::updateSpinners(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed)
{
	mWidthSpinnerCtrl->resetDirty();
	mHeightSpinnerCtrl->resetDirty();
	if (checkImageSize(previewp, width, height, is_width_changed, previewp->getMaxImageSize()))
	{
		mWidthSpinnerCtrl->forceSetValue(width);
		mHeightSpinnerCtrl->forceSetValue(height);
		if (getActiveSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
		{
			mWidthSpinnerCtrl->setIncrement(width >> 1);
			mHeightSpinnerCtrl->setIncrement(height >> 1);
		}
	}
}

void LLFloaterSnapshot::applyCustomResolution(S32 w, S32 h)
{
	// _LL_DEBUGS() << "applyCustomResolution(" << w << ", " << h << ")" << LL_ENDL;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		S32 curw,curh;
		previewp->getSize(curw, curh);

		if (w != curw || h != curh)
		{
			//if to upload a snapshot, process spinner input in a special way.
			previewp->setMaxImageSize((S32)mWidthSpinnerCtrl->getMaxValue()) ;

			previewp->setSize(w,h);
			comboSetCustom("profile_size_combo");
			comboSetCustom("postcard_size_combo");
			comboSetCustom("texture_size_combo");
			comboSetCustom("local_size_combo");
			// _LL_DEBUGS() << "applied custom resolution, updating thumbnail" << LL_ENDL;
			previewp->updateSnapshot(TRUE);
		}
	}
}

// static
void LLFloaterSnapshot::onSnapshotUploadFinished(bool status)
{
	setStatus(STATUS_FINISHED, status, "profile");
}

void LLFloaterSnapshot::onClickBigPreview()
{
    // Toggle the preview
    if (isPreviewVisible())
    {
		if (impl->mPreviewHandle.get()) impl->mPreviewHandle.get()->die();
        LLFloaterReg::hideInstance("big_preview");

    }
    else
    {
        attachPreview();
        LLFloaterReg::showInstance("big_preview");
    }

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl->mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl->mLastToolset);
	}

	delete impl;
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot(const LLSD& key)
    : LLFloaterSnapshotBase(key)
{
	impl = new Impl(this);
}

LLFloaterSnapshot::~LLFloaterSnapshot()
{
}

// virtual
BOOL LLFloaterSnapshot::postBuild()
{
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", ImplBase::onClickNewSnapshot, this);
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");

	childSetCommitCallback("ui_check", ImplBase::onClickUICheck, this);
	getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));

	childSetCommitCallback("hud_check", ImplBase::onClickHUDCheck, this);
	getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	((Impl*)impl)->setAspectRatioCheckboxValue(this, gSavedSettings.getBOOL("KeepAspectForSnapshot"));

	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	getChild<LLUICtrl>("layer_types")->setValue("colors");
	getChildView("layer_types")->setEnabled(FALSE);

	getChild<LLUICtrl>("freeze_frame_check")->setValue(gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", ImplBase::onCommitFreezeFrame, this);

	getChild<LLUICtrl>("auto_snapshot_check")->setValue(gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", ImplBase::onClickAutoSnap, this);

    getChild<LLButton>("retract_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));
    getChild<LLButton>("extend_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));

    getChild<LLTextBox>("360_label")->setSoundFlags(LLView::MOUSE_UP);
    getChild<LLTextBox>("360_label")->setShowCursorHand(false);
    getChild<LLTextBox>("360_label")->setClickedCallback(boost::bind(&LLFloaterSnapshot::on360Snapshot, this));

	// Filters
	LLComboBox* filterbox = getChild<LLComboBox>("filters_combobox");
	std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
	for (U32 i = 0; i < filter_list.size(); i++)
	{
		filterbox->add(filter_list[i]);
	}
	childSetCommitCallback("filters_combobox", ImplBase::onClickFilter, this);
    
	LLWebProfile::setImageUploadResultCallback(boost::bind(&Impl::onSnapshotUploadFinished, this, _1));
	LLPostCard::setPostResultCallback(boost::bind(&Impl::onSendingPostcardFinished, this, _1));

	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

	// create preview window
	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	LLView* parent_view = gSnapshotFloaterView->getParent();
	
	parent_view->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	parent_view->addChild(previewp);
	parent_view->addChild(gSnapshotFloaterView);
	
	//move snapshot floater to special purpose snapshotfloaterview
	gFloaterView->removeChild(this);
	gSnapshotFloaterView->addChild(this);

	// Pre-select "Current Window" resolution.
	getChild<LLComboBox>("profile_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("postcard_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("texture_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("local_size_combo")->selectNthItem(8);
	getChild<LLComboBox>("local_format_combo")->selectNthItem(0);

	impl->mPreviewHandle = previewp->getHandle();
    previewp->setContainer(this);
	impl->updateControls(this);
	impl->setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
	impl->updateLayout(this);
	

	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

	return TRUE;
>>>>>>> 630c4427447471a5a9e30b097789948cd236196b
}

bool LLFloaterSnapshot::isPreviewVisible()
{
	return (mBigPreviewFloater && mBigPreviewFloater->getVisible());
}

void LLFloaterSnapshot::attachPreview()
{
    if (mBigPreviewFloater)
    {
        LLSnapshotLivePreview* previewp = getPreviewView();
        mBigPreviewFloater->setPreview(previewp);
        mBigPreviewFloater->setFloaterOwner(this);
    }
}

void LLFloaterSnapshot::getBigPreview()
{
	mBigPreviewFloater = dynamic_cast<LLFloaterBigPreview*>(LLFloaterReg::getInstance("big_preview"));
}

void LLFloaterSnapshot::on360Snapshot()
{
    LLFloaterReg::showInstance("360capture");
    closeFloater();
}

//virtual
void LLFloaterSnapshotBase::onClose(bool app_quitting)
{
	getParent()->setMouseOpaque(FALSE);

	//unfreeze everything, hide fullscreen preview
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setAllowFullScreenPreview(FALSE);
		previewp->setVisible(FALSE);
		previewp->setEnabled(FALSE);
	}

	gSavedSettings.setBOOL("FreezeTime", FALSE);
	impl->mAvatarPauseHandles.clear();

	if (impl->mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl->mLastToolset);
	}
}

// virtual
S32 LLFloaterSnapshot::notify(const LLSD& info)
{
	if (info.has("snapshot-updating"))
	{
		// Disable the send/post/save buttons until snapshot is ready.
		updateControls();
		return 1;
	}

	if (info.has("snapshot-updated"))
	{
		// Enable the send/post/save buttons.
		updateControls();
		// We've just done refresh.
		setNeedRefresh(false);

		// The refresh button is initially hidden. We show it after the first update,
		// i.e. when preview appears.
		if (!mRefreshBtn->getVisible())
		{
			mRefreshBtn->setVisible(true);
		}
		return 1;
	}
    
	return 0;
}

BOOL LLFloaterSnapshot::isWaitingState()
{
	return (getStatus() == STATUS_WORKING);
}

BOOL LLFloaterSnapshot::updatePreviewList(bool initialized)
{
	if (!initialized)
		return FALSE;

	BOOL changed = FALSE;
	// _LL_DEBUGS() << "npreviews: " << LLSnapshotLivePreview::sList.size() << LL_ENDL;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
	return changed;
}


void LLFloaterSnapshot::updateLivePreview()
{
	if (updatePreviewList(true))
	{
		// _LL_DEBUGS() << "changed" << LL_ENDL;
		updateControls();
	}
}

//static 
void LLFloaterSnapshot::update()
{
	LLFloaterSnapshot* inst = findInstance();
	if (inst != NULL)
	{
		inst->updateLivePreview();
	}
	else
	{
		updatePreviewList(false);
	}
}

// virtual
void LLFloaterSnapshot::saveTexture()
{
	// _LL_DEBUGS() << "saveTexture" << LL_ENDL;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return;
	}

	BOOL is_outfit_snapshot = getOutfitID().notNull();
	if (is_outfit_snapshot && mOutfitGallery)
	{
		mOutfitGallery->onBeforeOutfitSnapshotSave();
	}
	previewp->saveTexture(is_outfit_snapshot, is_outfit_snapshot ? getOutfitID().asString() : "");
	if (is_outfit_snapshot && mOutfitGallery)
	{
		mOutfitGallery->onAfterOutfitSnapshotSave();
	}
}

void LLFloaterSnapshot::saveLocal(const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb)
{
	// _LL_DEBUGS() << "saveLocal" << LL_ENDL;
	LLSnapshotLivePreview* previewp = getPreviewView();
	llassert(previewp != NULL);
	if (previewp)
	{
		previewp->saveLocal(success_cb, failure_cb);
	}
}

void LLFloaterSnapshot::postSave()
{
	updateControls();
	setStatus(STATUS_WORKING);
}

void LLFloaterSnapshot::saveTextureFailed()
{
    updateControls();
    setStatus(STATUS_FINISHED, false, "inventory");
}

LLPointer<LLImageFormatted> LLFloaterSnapshot::getImageData()
{
	// FIXME: May not work for textures.

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return NULL;
	}

	LLPointer<LLImageFormatted> img = previewp->getFormattedImage();
	if (!img.get())
	{
		LL_WARNS() << "Empty snapshot image data" << LL_ENDL;
		llassert(img.get() != NULL);
	}

	return img;
}

const LLVector3d& LLFloaterSnapshot::getPosTakenGlobal()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return LLVector3d::zero;
	}

	return previewp->getPosTakenGlobal();
}

// static
void LLFloaterSnapshot::setAgentEmail(const std::string& email)
{
	LLFloaterSnapshot* snapshot_floater = LLFloaterSnapshot::findInstance();
	if (snapshot_floater)
	{
		snapshot_floater->notify(LLSD().with("agent-email", email));
	}
}

void LLFloaterSnapshot::enableControls(BOOL enable)
{
	setCtrlsEnabled(enable);
}

void LLFloaterSnapshot::updateImageQualityLevel()
{
	S32 quality_val = llfloor((F32)mImageQualitySliderCtrl->getValue().asReal());

	std::string quality_lvl;

	if (quality_val < 20)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_very_low");
	}
	else if (quality_val < 40)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_low");
	}
	else if (quality_val < 60)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_medium");
	}
	else if (quality_val < 80)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_high");
	}
	else
	{
		quality_lvl = LLTrans::getString("snapshot_quality_very_high");
	}

	getChild<LLTextBox>("image_quality_level")->setTextArg("[QLVL]", quality_lvl);
}

void LLFloaterSnapshot::onCustomResolutionCommit()
{
	S32 width = mWidthSpinnerCtrl->getValue().asInteger();
	S32 height = mHeightSpinnerCtrl->getValue().asInteger();
	LLPanel* panel = getActivePanel();
	if (panel->getName() == "panel_snapshot_inventory")
	{
		width = power_of_two(width, MAX_TEXTURE_SIZE);
		mWidthSpinnerCtrl->setIncrement(width >> 1);
		mWidthSpinnerCtrl->forceSetValue(width);
		height = power_of_two(height, MAX_TEXTURE_SIZE);
		mHeightSpinnerCtrl->setIncrement(height >> 1);
		mHeightSpinnerCtrl->forceSetValue(height);
	}
	applyCustomResolution(width, height);
}

void LLFloaterSnapshot::updateUploadCost()
{
	S32 upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
	getChild<LLUICtrl>("save_to_inventory_btn")->setLabelArg("[AMOUNT]", llformat("%d", upload_cost));

	//BD - Outfit
	mSnapshotOptionsPanel->getChild<LLUICtrl>("outfit_hint_lbl", TRUE)->setTextArg("[UPLOAD_COST]", llformat("%d", upload_cost));
}

void LLFloaterSnapshot::openPanel(const std::string& panel_name)
{
	if (!mSnapshotOptionsPanel)
	{
		LL_WARNS() << "Cannot find panel container" << LL_ENDL;
		return;
	}
	mSnapshotOptionsPanel->openPanel(panel_name);
	mSnapshotOptionsPanel->getCurrentPanel()->onOpen(LLSD());
	setStatus(STATUS_READY);
}

void LLFloaterSnapshot::onSaveToProfile()
{
	openPanel("panel_snapshot_profile");

	mSizeComboCtrl = getActivePanel()->getChild<LLComboBox>("profile_size_combo");
	mWidthSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("profile_snapshot_width");
	mHeightSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("profile_snapshot_height");
	mKeepAspectCheckCtrl = getActivePanel()->getChild<LLUICtrl>("profile_keep_aspect_check");
	mSaveBtn = getActivePanel()->getChild<LLUICtrl>("post_btn");
	mCancelBtn = getActivePanel()->getChild<LLUICtrl>("cancel_btn");

	mSaveBtn->setLabelArg("[UPLOAD_COST]", std::to_string(LLAgentBenefitsMgr::current().getTextureUploadCost()));
	mSizeComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateResolution, this, _1, true));
	mFormatComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateControls, this));
	mWidthSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mHeightSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mKeepAspectCheckCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));

	updateControls();
}

void LLFloaterSnapshot::onSaveToInventory()
{
	openPanel("panel_snapshot_inventory");

	mSizeComboCtrl = getActivePanel()->getChild<LLComboBox>("texture_size_combo");
	mWidthSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("inventory_snapshot_width");
	mHeightSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("inventory_snapshot_height");
	mKeepAspectCheckCtrl = getActivePanel()->getChild<LLUICtrl>("inventory_keep_aspect_check");
	mSaveBtn = getActivePanel()->getChild<LLUICtrl>("save_btn");
	mCancelBtn = getActivePanel()->getChild<LLUICtrl>("cancel_btn");

	mSaveBtn->setLabelArg("[UPLOAD_COST]", std::to_string(LLAgentBenefitsMgr::current().getTextureUploadCost()));
	mSizeComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateResolution, this, _1, true));
	mFormatComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateControls, this));
	mWidthSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mHeightSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mKeepAspectCheckCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));

	updateControls();
}

void LLFloaterSnapshot::onSaveToComputer()
{
	openPanel("panel_snapshot_local");

	mSizeComboCtrl = getActivePanel()->getChild<LLComboBox>("local_size_combo");
	mFormatComboCtrl = getActivePanel()->getChild<LLComboBox>("local_format_combo");
	mWidthSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("local_snapshot_width");
	mHeightSpinnerCtrl = getActivePanel()->getChild<LLSpinCtrl>("local_snapshot_height");
	mKeepAspectCheckCtrl = getActivePanel()->getChild<LLUICtrl>("local_keep_aspect_check");
	mImageQualitySliderCtrl = getActivePanel()->getChild<LLSliderCtrl>("image_quality_slider");
	mSaveBtn = getActivePanel()->getChild<LLUICtrl>("save_btn");
	mCancelBtn = getActivePanel()->getChild<LLUICtrl>("cancel_btn");

	if(gSavedSettings.getS32("LastSnapshotToDiskWidth") != 0)
		mWidthSpinnerCtrl->setValue(gSavedSettings.getS32("LastSnapshotToDiskWidth"));
	if (gSavedSettings.getS32("LastSnapshotToDiskHeight") != 0 && !mKeepAspectCheckCtrl->getValue())
		mHeightSpinnerCtrl->setValue(gSavedSettings.getS32("LastSnapshotToDiskHeight"));

	mSaveBtn->setLabelArg("[UPLOAD_COST]", std::to_string(LLAgentBenefitsMgr::current().getTextureUploadCost()));
	mSizeComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateResolution, this, _1, true));
	mFormatComboCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::updateControls, this));
	mWidthSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mHeightSpinnerCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCustomResolutionCommit, this));
	mKeepAspectCheckCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onKeepAspectRatioCommit, this, _1));
	mImageQualitySliderCtrl->setCommitCallback(boost::bind(&LLFloaterSnapshot::onQualitySliderCommit, this, _1));

	updateControls();
}

void LLFloaterSnapshot::onResolutionCommit(LLUICtrl* ctrl)
{
	BOOL current_window_selected = (mSizeComboCtrl->getCurrentIndex() == 3);
	mWidthSpinnerCtrl->setVisible(!current_window_selected);
	mHeightSpinnerCtrl->setVisible(!current_window_selected);
}

void LLFloaterSnapshot::onInventorySend()
{
	S32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
	if (can_afford_transaction(expected_upload_cost))
	{
		saveTexture();
		postSave();
	}
	else
	{
		LLSD args;
		args["COST"] = llformat("%d", expected_upload_cost);
		LLNotificationsUtil::add("ErrorPhotoCannotAfford", args);
		saveTextureFailed();
	}
}

void LLFloaterSnapshot::onQualitySliderCommit(LLUICtrl* ctrl)
{
	//updateImageQualityLevel();

	S32 quality_val = llfloor(ctrl->getValue().asReal());
	onImageQualityChange(quality_val);
}

void LLFloaterSnapshot::sendProfile()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	LLWebProfile::uploadImage(getImageData(), caption, add_location);
	postSave();
}

void LLFloaterSnapshot::onFormatComboCommit(LLUICtrl* ctrl)
{
	mLocalFormat = getImageFormat();
	// will call updateControls()
	gSavedSettings.setS32("SnapshotFormat", getImageFormat());
	onImageFormatChange();
}

void LLFloaterSnapshot::saveLocal(LLUICtrl* ctrl)
{
	if (ctrl->getValue().asString() == "save as")
	{
		gViewerWindow->resetSnapshotLoc();
	}

	setStatus(LLFloaterSnapshot::STATUS_WORKING, true);
	saveLocal((boost::bind(&LLFloaterSnapshot::saveLocalFinished, this)), (boost::bind(&LLFloaterSnapshot::saveLocalFailed, this)));
}

void LLFloaterSnapshot::saveLocalFinished()
{
	postSave();
	if(!gSavedSettings.getBOOL("UploadLocalSnapshots"))
		setStatus(LLFloaterSnapshot::STATUS_FINISHED, true, "local");
	else
		onInventorySend();
}

void LLFloaterSnapshot::saveLocalFailed()
{
	setStatus(LLFloaterSnapshot::STATUS_FINISHED, false, "local");
}


void LLFloaterSnapshot::onSnapshotSave()
{
	setStatus(LLFloaterSnapshot::STATUS_FINISHED);
}

void LLFloaterSnapshot::onSnapshotCancel()
{
	if (mSnapshotOptionsPanel)
	{
		mSnapshotOptionsPanel->openPreviousPanel();
		mSnapshotOptionsPanel->getCurrentPanel()->onOpen(LLSD());
	}
	setStatus(LLFloaterSnapshot::STATUS_READY, true);
}

// static
LLFloaterSnapshot* LLFloaterSnapshot::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
}

// static
LLFloaterSnapshot* LLFloaterSnapshot::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterSnapshot>("snapshot");
}


void LLFloaterSnapshot::updateOutfitResolution()
{
	S32 width = OUTFIT_SNAPSHOT_WIDTH;
	S32 height = OUTFIT_SNAPSHOT_HEIGHT;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		S32 original_width = 0, original_height = 0;
		previewp->getSize(original_width, original_height);

		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
			width = llmin(width, gViewerWindow->getWindowWidthRaw());
			height = llmin(height, gViewerWindow->getWindowHeightRaw());
		}


		llassert(width > 0 && height > 0);

		// use the resolution from the selected pre-canned drop-down choice
		// _LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
		previewp->setSize(width, height);

		if (original_width != width || original_height != height)
		{
			// hide old preview as the aspect ratio could be wrong
			// _LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
			previewp->updateSnapshot(TRUE);
		}
	}
}

// virtual
void LLFloaterSnapshot::saveOutfitTexture()
{
	// _LL_DEBUGS() << "saveTexture" << LL_ENDL;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return;
	}

	if (mOutfitGallery)
	{
		mOutfitGallery->onBeforeOutfitSnapshotSave();
	}
	previewp->saveTexture(TRUE, getOutfitID().asString());
	if (mOutfitGallery)
	{
		mOutfitGallery->onAfterOutfitSnapshotSave();
	}
	closeFloater();
}



///----------------------------------------------------------------------------
/// Class LLSnapshotFloaterView
///----------------------------------------------------------------------------

LLSnapshotFloaterView::LLSnapshotFloaterView (const Params& p) : LLFloaterView (p)
{
}

LLSnapshotFloaterView::~LLSnapshotFloaterView()
{
}

// virtual
BOOL LLSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if (called_from_parent)
	{
		// pass all keystrokes down
		LLFloaterView::handleKey(key, mask, called_from_parent);
	}
	else
	{
		// bounce keystrokes back down
		LLFloaterView::handleKey(key, mask, TRUE);
	}
	return TRUE;
}




