/** 
 * @file llfloateroutfitsnapshot.cpp
 * @brief Snapshot preview window for saving as an outfit thumbnail in visual outfit gallery
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
#include "llfloateroutfitsnapshot.h"

#include "llagent.h"
#include "llfloaterreg.h"
#include "llimagefiltersmanager.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llpostcard.h"
#include "llresmgr.h"		// LLLocale
#include "llsdserialize.h"
#include "llsidetraypanelcontainer.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
LLOutfitSnapshotFloaterView* gOutfitSnapshotFloaterView = NULL;

const S32 OUTFIT_SNAPSHOT_WIDTH = 256;
const S32 OUTFIT_SNAPSHOT_HEIGHT = 256;

static LLDefaultChildRegistry::Register<LLOutfitSnapshotFloaterView> r("snapshot_outfit_floater_view");

///----------------------------------------------------------------------------
/// Class LLFloaterOutfitSnapshot::Impl
///----------------------------------------------------------------------------

// virtual
LLPanelSnapshot* LLFloaterOutfitSnapshot::getActivePanel(bool ok_if_not_found)
{
    LLPanel* panel = getChild<LLPanel>("panel_outfit_snapshot_inventory");
    LLPanelSnapshot* active_panel = dynamic_cast<LLPanelSnapshot*>(panel);
    if (!ok_if_not_found)
    {
        llassert_always(active_panel != NULL);
    }
    return active_panel;
}

// virtual
LLSnapshotModel::ESnapshotFormat LLFloaterOutfitSnapshot::getImageFormat()
{
    return LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
}

// virtual
LLSnapshotModel::ESnapshotLayerType LLFloaterOutfitSnapshot::getLayerType()
{
    return LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// FIXME: The above comment seems obsolete now.
// virtual
void LLFloaterOutfitSnapshot::updateControls(LLFloaterSnapshot* floater)
{
    LLSnapshotModel::ESnapshotType shot_type = getActiveSnapshotType();
    LLSnapshotModel::ESnapshotFormat shot_format = (LLSnapshotModel::ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
    LLSnapshotModel::ESnapshotLayerType layer_type = getLayerType();

    LLSnapshotLivePreview* previewp = getPreviewView();
    BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

    // *TODO: Separate maximum size for Web images from postcards
    // _LL_DEBUGS() << "Is snapshot up-to-date? " << got_snap << LL_ENDL;

    LLLocale locale(LLLocale::USER_LOCALE);
    std::string bytes_string;
    if (got_snap)
    {
        LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10);
    }

    // Update displayed image resolution.
    LLTextBox* image_res_tb = floater->getChild<LLTextBox>("image_res_text");
    image_res_tb->setVisible(got_snap);
    if (got_snap)
    {
        image_res_tb->setTextArg("[WIDTH]", llformat("%d", previewp->getEncodedImageWidth()));
        image_res_tb->setTextArg("[HEIGHT]", llformat("%d", previewp->getEncodedImageHeight()));
    }

    floater->getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : floater->getString("unknown"));
    floater->getChild<LLUICtrl>("file_size_label")->setColor(LLUIColorTable::instance().getColor("LabelTextColor"));

    updateResolution(floater);

    if (previewp)
    {
        previewp->setSnapshotType(shot_type);
        previewp->setSnapshotFormat(shot_format);
        previewp->setSnapshotBufferType(layer_type);
    }

    LLPanelSnapshot* current_panel = getActivePanel(floater);
    if (current_panel)
    {
        LLSD info;
        info["have-snapshot"] = got_snap;
        current_panel->updateControls(info);
    }
    // _LL_DEBUGS() << "finished updating controls" << LL_ENDL;
}

// virtual
std::string LLFloaterOutfitSnapshot::getSnapshotPanelPrefix()
{
    return "panel_outfit_snapshot_";
}

// Show/hide upload status message.
// virtual
void LLFloaterOutfitSnapshot::setFinished(bool finished, bool ok, const std::string& msg)
{
	LLFloaterSnapshot* snapshot_floater = LLFloaterSnapshot::findInstance();
	snapshot_floater->setSuccessLabelPanelVisible(finished && ok);
	snapshot_floater->setFailureLabelPanelVisible(finished && !ok);

    if (finished)
    {
        LLUICtrl* finished_lbl = snapshot_floater->getChild<LLUICtrl>(ok ? "succeeded_lbl" : "failed_lbl");
        std::string result_text = snapshot_floater->getString(msg + "_" + (ok ? "succeeded_str" : "failed_str"));
        finished_lbl->setValue(result_text);

        LLPanel* snapshot_panel = snapshot_floater->getChild<LLPanel>("panel_outfit_snapshot_inventory");
        snapshot_panel->onOpen(LLSD());
    }
}

void LLFloaterOutfitSnapshot::updateResolution(void* data)
{
    LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;

    if (!view)
    {
        llassert(view);
        return;
    }

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

///----------------------------------------------------------------------------
/// Class LLFloaterOutfitSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterOutfitSnapshot::LLFloaterOutfitSnapshot(const LLSD& key)
: LLFloaterSnapshot(key),
mOutfitGallery(NULL)
{
}

LLFloaterOutfitSnapshot::~LLFloaterOutfitSnapshot()
{
}

// virtual
BOOL LLFloaterOutfitSnapshot::postBuild()
{
    mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	mRefreshBtn->setCommitCallback(boost::bind(&LLFloaterSnapshot::onClickNewSnapshot, this));
    mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
    mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
    mFailureLblPanel = getChild<LLUICtrl>("failed_panel");

    getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));
	getChild<LLUICtrl>("ui_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onUpdateSnapshotAndControls, this));
    getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));
	getChild<LLUICtrl>("hud_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onUpdateSnapshotAndControls, this));
	getChild<LLUICtrl>("freeze_world_check")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onCommitFreezeWorld, this, _1));
    getChild<LLButton>("retract_btn")->setCommitCallback(boost::bind(&LLFloaterOutfitSnapshot::onExtendFloater, this));
    getChild<LLButton>("extend_btn")->setCommitCallback(boost::bind(&LLFloaterOutfitSnapshot::onExtendFloater, this));

    // Filters
    LLComboBox* filterbox = getChild<LLComboBox>("filters_combobox");
    std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
    for (U32 i = 0; i < filter_list.size(); i++)
    {
        filterbox->add(filter_list[i]);
    }
	getChild<LLButton>("filters_combobox")->setCommitCallback(boost::bind(&LLFloaterOutfitSnapshot::onClickFilter, this, _1));

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

    mPreviewHandle = previewp->getHandle();
    previewp->setContainer(this);
    updateControls(this);
    setAdvanced(gSavedSettings.getBOOL("AdvanceOutfitSnapshot"));

    previewp->mKeepAspectRatio = FALSE;
    previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

    return TRUE;
}

// virtual
void LLFloaterOutfitSnapshot::onOpen(const LLSD& key)
{
    LLSnapshotLivePreview* preview = getPreviewView();
    if (preview)
    {
        // _LL_DEBUGS() << "opened, updating snapshot" << LL_ENDL;
        preview->updateSnapshot(TRUE);
    }
    focusFirstItem(FALSE);
    gSnapshotFloaterView->setEnabled(TRUE);
    gSnapshotFloaterView->setVisible(TRUE);
    gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

    updateControls(this);
    setAdvanced(gSavedSettings.getBOOL("AdvanceOutfitSnapshot"));

    LLPanel* snapshot_panel = getChild<LLPanel>("panel_outfit_snapshot_inventory");
    snapshot_panel->onOpen(LLSD());
    //postPanelSwitch();

}

void LLFloaterOutfitSnapshot::onExtendFloater()
{
	setAdvanced(gSavedSettings.getBOOL("AdvanceOutfitSnapshot"));
}

// static 
void LLFloaterOutfitSnapshot::update()
{
    LLFloaterOutfitSnapshot* inst = findInstance();
    if (inst != NULL)
    {
        inst->updateLivePreview();
    }
}


// static
LLFloaterOutfitSnapshot* LLFloaterOutfitSnapshot::findInstance()
{
    return LLFloaterReg::findTypedInstance<LLFloaterOutfitSnapshot>("outfit_snapshot");
}

// static
LLFloaterOutfitSnapshot* LLFloaterOutfitSnapshot::getInstance()
{
    return LLFloaterReg::getTypedInstance<LLFloaterOutfitSnapshot>("outfit_snapshot");
}

// virtual
void LLFloaterOutfitSnapshot::saveTexture()
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
/// Class LLOutfitSnapshotFloaterView
///----------------------------------------------------------------------------

LLOutfitSnapshotFloaterView::LLOutfitSnapshotFloaterView(const Params& p) : LLFloaterView(p)
{
}

LLOutfitSnapshotFloaterView::~LLOutfitSnapshotFloaterView()
{
}
