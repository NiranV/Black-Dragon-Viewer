/**
 * @file llpanelprofilepicks.cpp
 * @brief LLPanelProfilePicks and related class implementations
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelprofilepicks.h"

#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcommandhandler.h"
#include "lldispatcher.h"
#include "llfloaterreg.h"
#include "llfloaterprofile.h"
#include "llfloaterworldmap.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewergenericmessage.h" // send_generic_message
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

static LLPanelInjector<LLPanelProfilePick> t_panel_profile_pick("panel_profile_pick");

//-----------------------------------------------------------------------------
// LLPanelProfilePick
//-----------------------------------------------------------------------------

LLPanelProfilePick::LLPanelProfilePick()
 : LLRemoteParcelInfoObserver()
 , mSnapshotCtrl(NULL)
 , mPickId(LLUUID::null)
 , mParcelId(LLUUID::null)
 , mRequestedId(LLUUID::null)
 , mLocationChanged(false)
 , mNewPick(false)
 , mCurrentPickDescription("")
 , mIsEditing(false)
 , mSelfProfile(false)
 , mLoaded(false)
{
}

//static
LLPanelProfilePick* LLPanelProfilePick::create()
{
    LLPanelProfilePick* panel = new LLPanelProfilePick();
    panel->buildFromFile("panel_profile_pick.xml");
    return panel;
}

LLPanelProfilePick::~LLPanelProfilePick()
{
    if (mParcelId.notNull())
    {
        LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
    }
}

void LLPanelProfilePick::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.isNull())
    {
        return;
    }

	mAvatarId = avatar_id;
	mSelfProfile = gAgent.getID() == mAvatarId;

    // creating new Pick
	if (getPickId().isNull() && mSelfProfile)
    {
        mNewPick = true;

        setPosGlobal(gAgent.getPositionGlobal());

        LLUUID parcel_id = LLUUID::null, snapshot_id = LLUUID::null;
        std::string pick_name, pick_desc, region_name;

        LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
        if (parcel)
        {
            parcel_id = parcel->getID();
            pick_name = parcel->getName();
            pick_desc = parcel->getDesc();
            snapshot_id = parcel->getSnapshotID();
        }

        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            region_name = region->getName();
        }

        setParcelID(parcel_id);
        setPickName(pick_name.empty() ? region_name : pick_name);
        setPickDesc(pick_desc);
        setSnapshotId(snapshot_id);
        setPickLocation(createLocationText(getLocationNotice(), pick_name, region_name, getPosGlobal()));
    }

	enableSaveButton(getPickId().isNull() && mSelfProfile);

    resetDirty();

	if (mSelfProfile)
    {
        mPickName->setEnabled(TRUE);
        mPickDescription->setEnabled(TRUE);
        mSetCurrentLocationButton->setVisible(TRUE);
    }
    else
    {
        mSnapshotCtrl->setEnabled(FALSE);
    }
}

BOOL LLPanelProfilePick::postBuild()
{
    mPickName = getChild<LLLineEditor>("pick_name");
    mPickDescription = getChild<LLTextEditor>("pick_desc");
    mSaveButton = getChild<LLButton>("save_changes_btn");
    mSetCurrentLocationButton = getChild<LLButton>("set_to_curr_location_btn");

    mSnapshotCtrl = getChild<LLTextureCtrl>("pick_snapshot");
    mSnapshotCtrl->setCommitCallback(boost::bind(&LLPanelProfilePick::onSnapshotChanged, this));

    childSetAction("teleport_btn", boost::bind(&LLPanelProfilePick::onClickTeleport, this));
    childSetAction("show_on_map_btn", boost::bind(&LLPanelProfilePick::onClickMap, this));

    mSaveButton->setCommitCallback(boost::bind(&LLPanelProfilePick::onClickSave, this));
    mSetCurrentLocationButton->setCommitCallback(boost::bind(&LLPanelProfilePick::onClickSetLocation, this));

    mPickName->setKeystrokeCallback(boost::bind(&LLPanelProfilePick::onPickChanged, this, _1), NULL);
    mPickName->setEnabled(FALSE);

    mPickDescription->setKeystrokeCallback(boost::bind(&LLPanelProfilePick::onPickChanged, this, _1));
    mPickDescription->setFocusReceivedCallback(boost::bind(&LLPanelProfilePick::onDescriptionFocusReceived, this));

    getChild<LLUICtrl>("pick_location")->setEnabled(FALSE);

    return TRUE;
}

void LLPanelProfilePick::onDescriptionFocusReceived()
{
    if (!mIsEditing && mSelfProfile)
    {
        mIsEditing = true;
        mPickDescription->setParseHTML(false);
        setPickDesc(mCurrentPickDescription);
    }
}

void LLPanelProfilePick::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_PICK_INFO != type)
    {
        return;
    }

    LLPickData* pick_info = static_cast<LLPickData*>(data);
    if (!pick_info
        || pick_info->creator_id != mAvatarId
        || pick_info->pick_id != getPickId())
    {
        return;
    }

    mIsEditing = false;
    mPickDescription->setParseHTML(true);
    mParcelId = pick_info->parcel_id;
    setSnapshotId(pick_info->snapshot_id);
    if (!mSelfProfile)
    {
        mSnapshotCtrl->setEnabled(FALSE);
    }
    setPickName(pick_info->name);
    setPickDesc(pick_info->desc);
    setPosGlobal(pick_info->pos_global);
    mCurrentPickDescription = pick_info->desc;

    // Send remote parcel info request to get parcel name and sim (region) name.
    sendParcelInfoRequest();

    // *NOTE dzaporozhan
    // We want to keep listening to APT_PICK_INFO because user may
    // edit the Pick and we have to update Pick info panel.
    // revomeObserver is called from onClickBack
}

void LLPanelProfilePick::apply()
{
    if ((mNewPick || mLoaded) && isDirty())
    {
        sendUpdate();
    }
}

void LLPanelProfilePick::setSnapshotId(const LLUUID& id)
{
    mSnapshotCtrl->setImageAssetID(id);
    mSnapshotCtrl->setValid(TRUE);
}

void LLPanelProfilePick::setPickName(const std::string& name)
{
    mPickName->setValue(name);
}

const std::string LLPanelProfilePick::getPickName()
{
    return mPickName->getValue().asString();
}

void LLPanelProfilePick::setPickDesc(const std::string& desc)
{
    mPickDescription->setValue(desc);
}

void LLPanelProfilePick::setPickLocation(const std::string& location)
{
    getChild<LLUICtrl>("pick_location")->setValue(location);
}

void LLPanelProfilePick::onClickMap()
{
    LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelProfilePick::onClickTeleport()
{
    if (!getPosGlobal().isExactlyZero())
    {
        gAgent.teleportViaLocation(getPosGlobal());
        LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    }
}

void LLPanelProfilePick::enableSaveButton(BOOL enable)
{
    mSaveButton->setEnabled(enable);
    mSaveButton->setVisible(enable);
}

void LLPanelProfilePick::onSnapshotChanged()
{
    enableSaveButton(TRUE);
}

void LLPanelProfilePick::onPickChanged(LLUICtrl* ctrl)
{
    if (ctrl && ctrl == mPickName)
    {
        updateTabLabel(mPickName->getText());
    }

    enableSaveButton(isDirty());
}

void LLPanelProfilePick::resetDirty()
{
    LLPanel::resetDirty();

    mPickName->resetDirty();
    mPickDescription->resetDirty();
    mSnapshotCtrl->resetDirty();
    mLocationChanged = false;
}

BOOL LLPanelProfilePick::isDirty() const
{
    if (mNewPick
        || LLPanel::isDirty()
        || mLocationChanged
        || mSnapshotCtrl->isDirty()
        || mPickName->isDirty()
        || mPickDescription->isDirty())
    {
        return TRUE;
    }
    return FALSE;
}

void LLPanelProfilePick::onClickSetLocation()
{
    // Save location for later use.
    setPosGlobal(gAgent.getPositionGlobal());

    std::string parcel_name, region_name;

    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (parcel)
    {
        mParcelId = parcel->getID();
        parcel_name = parcel->getName();
    }

    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        region_name = region->getName();
    }

    setPickLocation(createLocationText(getLocationNotice(), parcel_name, region_name, getPosGlobal()));

    mLocationChanged = true;
    enableSaveButton(TRUE);
}

void LLPanelProfilePick::onClickSave()
{
    sendUpdate();

    mLocationChanged = false;
}

std::string LLPanelProfilePick::getLocationNotice()
{
    static const std::string notice = getString("location_notice");
    return notice;
}

void LLPanelProfilePick::sendParcelInfoRequest()
{
    if (mParcelId != mRequestedId)
    {
        if (mRequestedId.notNull())
        {
            LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mRequestedId, this);
        }
        LLRemoteParcelInfoProcessor::getInstance()->addObserver(mParcelId, this);
        LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(mParcelId);

        mRequestedId = mParcelId;
    }
}

void LLPanelProfilePick::processParcelInfo(const LLParcelData& parcel_data)
{
    setPickLocation(createLocationText(LLStringUtil::null, parcel_data.name, parcel_data.sim_name, getPosGlobal()));

    // We have received parcel info for the requested ID so clear it now.
    mRequestedId.setNull();

    if (mParcelId.notNull())
    {
        LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
    }
}

void LLPanelProfilePick::sendUpdate()
{
    LLPickData pick_data;

    // If we don't have a pick id yet, we'll need to generate one,
    // otherwise we'll keep overwriting pick_id 00000 in the database.
    if (getPickId().isNull())
    {
        getPickId().generate();
    }

    pick_data.agent_id = gAgentID;
    pick_data.session_id = gAgent.getSessionID();
    pick_data.pick_id = getPickId();
    pick_data.creator_id = gAgentID;;

    //legacy var  need to be deleted
    pick_data.top_pick = FALSE;
    pick_data.parcel_id = mParcelId;
    pick_data.name = getPickName();
    pick_data.desc = mPickDescription->getValue().asString();
    pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
    pick_data.pos_global = getPosGlobal();
    pick_data.sort_order = 0;
    pick_data.enabled = TRUE;

    LLAvatarPropertiesProcessor::getInstance()->sendPickInfoUpdate(&pick_data);

    if(mNewPick)
    {
        // Assume a successful create pick operation, make new number of picks
        // available immediately. Actual number of picks will be requested in
        // LLAvatarPropertiesProcessor::sendPickInfoUpdate and updated upon server respond.
        LLAgentPicksInfo::getInstance()->incrementNumberOfPicks();
    }
}

// static
std::string LLPanelProfilePick::createLocationText(const std::string& owner_name, const std::string& original_name, const std::string& sim_name, const LLVector3d& pos_global)
{
    std::string location_text(owner_name);
    if (!original_name.empty())
    {
        if (!location_text.empty())
        {
            location_text.append(", ");
        }
        location_text.append(original_name);

    }

    if (!sim_name.empty())
    {
        if (!location_text.empty())
        {
            location_text.append(", ");
        }
        location_text.append(sim_name);
    }

    if (!location_text.empty())
    {
        location_text.append(" ");
    }

    if (!pos_global.isNull())
    {
        S32 region_x = ll_round((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
        S32 region_y = ll_round((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
        S32 region_z = ll_round((F32)pos_global.mdV[VZ]);
        location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));
    }
    return location_text;
}

void LLPanelProfilePick::updateTabLabel(const std::string& title)
{
    setLabel(title);
    LLTabContainer* parent = dynamic_cast<LLTabContainer*>(getParent());
    if (parent)
    {
        parent->setCurrentTabName(title);
    }
}

