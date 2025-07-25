/**
*
* Copyright (C) 2018, NiranV Dean
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
*/

#include "llviewerprecompiledheaders.h"

#include "bdfloaterposer.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llavataractions.h"
#include "llclipboard.h"
#include "lldatapacker.h"
#include "lldiriterator.h"
#include "llfilepicker.h"
#include "llfilesystem.h"
#include "llkeyframemotion.h"
#include "llnotificationsutil.h"
#include "llmenugl.h"
#include "llmenubutton.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerjointattachment.h"
#include "llviewerjoint.h"
#include "llvoavatarself.h"
#include "llwindowwin32.h"
#include "pipeline.h"
#include "lltoolcomp.h"
#include "llviewercontrol.h"
#include "llagentui.h"
#include "llinstantmessage.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include "llviewerobjectlist.h"
#include "lldrawpoolavatar.h"

//BD - Animesh Support
#include "llcontrolavatar.h"

//BD - Black Dragon specifics
#include "bdanimator.h"
#include "bdfunctions.h"
#include "bdposingmotion.h"
#include "bdstatus.h"


//BD - TODO:
//1.   Now that Collision Volumes and Attachment Bones rotations, positions and scales are all working
//     they all need to be made compatible with several of the functions that were all locked behind
//     normal joints.
//     Flipping, Mirroring, Symmetrizing, Copying, Pasting, Recapturing to name a few.
//2.   Collision Volumes and Attachment Bones also need to be moved over to the new combined information
//     display and they need to be reconstructed and updated too.    

BDFloaterPoser::BDFloaterPoser(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Save our current pose as XML or ANIM file to be used or uploaded later.
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterPoser::onClickPoseSave, this, _2));
	//BD - Start our custom pose.
	mCommitCallbackRegistrar.add("Pose.Start", boost::bind(&BDFloaterPoser::onPoseStart, this));
	//BD - Load the current pose and export all its values into the UI so we can alter them.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterPoser::onPoseLoad, this));
	//BD - Delete the currently selected Pose.
	mCommitCallbackRegistrar.add("Pose.Delete", boost::bind(&BDFloaterPoser::onPoseDelete, this));
	//BD - Extend or collapse the floater's pose list.
	mCommitCallbackRegistrar.add("Pose.Layout", boost::bind(&BDFloaterPoser::onUpdateLayout, this));
	//BD - Include some menu interactions. Sadly necessary.
	mCommitCallbackRegistrar.add("Pose.Menu", boost::bind(&BDFloaterPoser::onPoseMenuAction, this, _2));

	//BD - Change a bone's rotation.
	mCommitCallbackRegistrar.add("Joint.Set", boost::bind(&BDFloaterPoser::onJointSet, this, _1, _2));
	//BD - Change a bone's position.
	mCommitCallbackRegistrar.add("Joint.PosSet", boost::bind(&BDFloaterPoser::onJointPosSet, this, _1, _2));
	//BD - Change a bone's scale.
	mCommitCallbackRegistrar.add("Joint.SetScale", boost::bind(&BDFloaterPoser::onJointScaleSet, this, _1, _2));
	//BD - Add or remove a joint state to or from the pose (enable/disable our overrides).
	mCommitCallbackRegistrar.add("Joint.ChangeState", boost::bind(&BDFloaterPoser::onJointChangeState, this));
	//BD - Reset all selected bone rotations and positions.
	mCommitCallbackRegistrar.add("Joint.ResetJointFull", boost::bind(&BDFloaterPoser::onJointRotPosScaleReset, this));
	//BD - Reset all selected bone rotations back to 0,0,0.
	mCommitCallbackRegistrar.add("Joint.ResetJointRotation", boost::bind(&BDFloaterPoser::onJointRotationReset, this));
	//BD - Reset all selected bones positions back to their default.
	mCommitCallbackRegistrar.add("Joint.ResetJointPosition", boost::bind(&BDFloaterPoser::onJointPositionReset, this));
	//BD - Reset all selected bones scales back to their default.
	mCommitCallbackRegistrar.add("Joint.ResetJointScale", boost::bind(&BDFloaterPoser::onJointScaleReset, this));
	//BD - Reset all selected bone rotations back to the initial rotation.
	mCommitCallbackRegistrar.add("Joint.RevertJointRotation", boost::bind(&BDFloaterPoser::onJointRotationRevert, this));
	//BD - Recapture all bones either all or just disabled ones.
	mCommitCallbackRegistrar.add("Joint.Recapture", boost::bind(&BDFloaterPoser::onJointRecapture, this));
	//BD - Mirror the current bone's rotation to match what the other body side's rotation should be.
	mCommitCallbackRegistrar.add("Joint.Mirror", boost::bind(&BDFloaterPoser::onJointMirror, this));
	//BD - Copy and mirror the other body side's bone rotation.
	mCommitCallbackRegistrar.add("Joint.SymmetrizeFrom", boost::bind(&BDFloaterPoser::onJointSymmetrize, this, true));
	//BD - Copy and mirror the other body side's bone rotation.
	mCommitCallbackRegistrar.add("Joint.SymmetrizeTo", boost::bind(&BDFloaterPoser::onJointSymmetrize, this, false));
    //BD - Copy bone transformations.
    mCommitCallbackRegistrar.add("Joint.CopyTransforms", boost::bind(&BDFloaterPoser::onJointCopyTransforms, this));

	//BD - Toggle Mirror Mode on/off.
	mCommitCallbackRegistrar.add("Joint.ToggleMirror", boost::bind(&BDFloaterPoser::toggleMirrorMode, this, _1));
	//BD - Toggle Easy Rotation on/off.
	mCommitCallbackRegistrar.add("Joint.EasyRotations", boost::bind(&BDFloaterPoser::toggleEasyRotations, this, _1));
	//BD - Flip pose (mirror).
	mCommitCallbackRegistrar.add("Joint.FlipPose", boost::bind(&BDFloaterPoser::onFlipPose, this));
	//BD - Symmetrize both sides of the opposite body.
	mCommitCallbackRegistrar.add("Joint.SymmetrizePose", boost::bind(&BDFloaterPoser::onPoseSymmetrize, this, _2));

    //BD - Toggle the visual rotation widgets.
    mCommitCallbackRegistrar.add("Joint.ToggleWidgets", boost::bind(&BDFloaterPoser::toggleWidgets, this));

	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Poser.RefreshAvatars", boost::bind(&BDFloaterPoser::onAvatarsRefresh, this));

    //BD - Request Permission to pose someone.
    mCommitCallbackRegistrar.add("Poser.RequestPermission", boost::bind(&BDFloaterPoser::onRequestPermission, this));

    //BD - Request Permission to pose someone.
    mCommitCallbackRegistrar.add("Poser.RequestSyncing", boost::bind(&BDFloaterPoser::onRequestSyncing, this));

    //BD - Synchronize the current pose.
    //mCommitCallbackRegistrar.add("Poser.SyncPose", boost::bind(&BDFloaterPoser::onSyncPose, this));
}

BDFloaterPoser::~BDFloaterPoser()
{
}

bool BDFloaterPoser::postBuild()
{
	//BD - Posing
	mJointScrolls = { { this->getChild<LLScrollListCtrl>("joints_scroll", true),
						this->getChild<LLScrollListCtrl>("cv_scroll", true),
						this->getChild<LLScrollListCtrl>("attach_scroll", true) } };


    for (S32 it = 0; it < 3; ++it)
    {
        mJointScrolls[it]->setCommitOnSelectionChange(TRUE);
        mJointScrolls[it]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));
    }
    mJointScrolls[JOINTS]->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onJointChangeState, this));

	mPoseScroll = this->getChild<LLScrollListCtrl>("poses_scroll", true);
	mPoseScroll->setCommitOnSelectionChange(TRUE);
	mPoseScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onPoseControlsRefresh, this));
	mPoseScroll->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onPoseLoad, this));

	mRotationSliders = { { getChild<LLUICtrl>("Rotation_X"), getChild<LLUICtrl>("Rotation_Y"), getChild<LLUICtrl>("Rotation_Z") } };
	mPositionSliders = { { getChild<LLSliderCtrl>("Position_X"), getChild<LLSliderCtrl>("Position_Y"), getChild<LLSliderCtrl>("Position_Z") } };
	mScaleSliders = { { getChild<LLSliderCtrl>("Scale_X"), getChild<LLSliderCtrl>("Scale_Y"), getChild<LLSliderCtrl>("Scale_Z") } };

	mJointTabs = getChild<LLTabContainer>("joints_tabs");
	mJointTabs->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	mModifierTabs = getChild<LLTabContainer>("modifier_tabs");
    mModifierTabs->setCommitCallback(boost::bind(&BDFloaterPoser::onModifierTabSwitch, this));

	//BD - Animesh
	mAvatarScroll = this->getChild<LLScrollListCtrl>("avatar_scroll", true);
	mAvatarScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAvatarsSelect, this));

	//BD - Misc
	mDelayRefresh = false;

	mMirrorMode = false;
	mEasyRotations = true;

	mStartPosingBtn = getChild<LLButton>("activate");
	mLoadPosesBtn = getChild<LLMenuButton>("load_poses");
	mSavePosesBtn = getChild<LLButton>("save_poses");

	//BD - Poser Menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar pose_reg;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	pose_reg.add("Pose.Menu", boost::bind(&BDFloaterPoser::onPoseLoadSelective, this, _2));
	LLToggleableMenu* btn_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_poser_poses_btn.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(btn_menu)
		mLoadPosesBtn->setMenu(btn_menu, LLMenuButton::MP_BOTTOM_LEFT);

	LLContextMenu *context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_poser_poses.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (context_menu)
	{
		mPoseScroll->setContextMenu(context_menu);
	}

	//BD - Poser Right Click Menu
	pose_reg.add("Joints.Menu", boost::bind(&BDFloaterPoser::onJointContextMenuAction, this, _2));
	enable_registrar.add("Joints.OnEnable", boost::bind(&BDFloaterPoser::onJointContextMenuEnable, this, _2));
	LLContextMenu* joint_menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_poser_joints.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mJointScrolls[JOINTS]->setContextMenu(joint_menu);

    //BD - Copy/Paste Button Menus
    LLToggleableMenu* copy_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_poser_copy_btn.xml",
        gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if (copy_menu)
        getChild<LLMenuButton>("copy")->setMenu(copy_menu, LLMenuButton::MP_BOTTOM_LEFT);
    LLToggleableMenu* paste_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_poser_paste_btn.xml",
        gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if (paste_menu)
        getChild<LLMenuButton>("paste")->setMenu(paste_menu, LLMenuButton::MP_BOTTOM_LEFT);

    mSyncTimer.start();
    mSyncList.clear();

	return true;
}

void BDFloaterPoser::draw()
{
    //BD - If the posing status changes from the outside (such as via the status button)
    //     then refresh the UI to reflect that.
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (av_item)
    {
        LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
        if (avatar)
        {
            if (avatar->getIsInSync())
            {
                //BD - Synchronize any changes to all "subscribed" avatars.
                if (mSyncTimer.getElapsedTimeF32() > 10.f)
                {
                    if (avatar->getNeedsFullSync())
                    {
                        onSyncPose(avatar->getID());
                        avatar->setNeedsFullSync(false);
                    }
                    else
                    {
                        onSyncBones();
                    }
                    mSyncTimer.reset();
                }
            }

            if ((mStartPosingBtn->getValue() && !avatar->mIsPosing))
            {
                mStartPosingBtn->setValue(false);
                onJointRefresh();
                onPoseControlsRefresh();
            }
        }
    }

    //BD - Synchronize any changes to all "subscribed" avatars.
    if (mSyncTimer.getElapsedTimeF32() > 1.5f)
    {
        for (LLCharacter* character : LLCharacter::sInstances)
        {
            if (LLVOAvatar* avatar = (LLVOAvatar*)character)
            {
                if (avatar)
                {
                    if (avatar->getIsInSync())
                    {
                        if (avatar->getNeedsFullSync())
                        {
                            onSyncPose(avatar->getID());
                            avatar->setNeedsFullSync(false);
                        }
                        else
                        {
                            onSyncBones();
                        }
                    }
                }
            }
        }

        //BD - Re-enable the sync button after we actually synchronized, this should
        //     put a short cooldown on full resyncs.
        getChild<LLUICtrl>("sync")->setEnabled(true);
        mSyncTimer.reset();
    }

	LLFloater::draw();
}

void BDFloaterPoser::onOpen(const LLSD& key)
{
	//BD - Check whether we should delay the default value collection or fire it immediately.
	mDelayRefresh = !gAgentAvatarp->isFullyLoaded();
	if (!mDelayRefresh)
	{
		onCollectDefaults();
	}

	//BD - We first fill the avatar list because the creation controls require it.
	onAvatarsRefresh();
	onJointRefresh();
	onPoseRefresh();
	onUpdateLayout();
}

void BDFloaterPoser::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
    for (S32 it = 0; it < 3; ++it)
	    mJointScrolls[it]->clearRows();

	mAvatarScroll->clearRows();

    //BD - Beq's Visual Posing
    if (mLastToolset)
    {
        LLToolMgr::getInstance()->setCurrentToolset(mLastToolset);
    }
    FSToolCompPose::getInstance()->setAvatar(nullptr);
    BDToolCompPoseTranslate::getInstance()->setAvatar(nullptr);
}

void BDFloaterPoser::onFocusReceived()
{
    //BD - Check all Avatars and if we still have their permission.
    onAvatarsRefresh();

    //BD - Refresh all our controls after to disable the Posing button and show the request button.
    onJointControlsRefresh();
}

////////////////////////////////
//BD - Poses
////////////////////////////////
void BDFloaterPoser::onPoseRefresh()
{
	mPoseScroll->clearRows();
	std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_POSES, "");
	std::string file;
	LLDirIterator dir_iter(dir, "*.xml");
	while (dir_iter.next(file))
	{
		std::string path = gDirUtilp->add(dir, file);
		std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

		LLSD row;
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = name;

		llifstream infile;
		infile.open(path);
		if (!infile.is_open())
		{
			LL_WARNS("Posing") << "Skipping: Cannot read file in: " << path << LL_ENDL;
			continue;
		}

		LLSD data;
		if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(data, infile))
		{
			LL_WARNS("Posing") << "Skipping: Failed to parse pose file: " << path << LL_ENDL;
			continue;
		}
		mPoseScroll->addElement(row);
	}
	onJointControlsRefresh();
}

bool BDFloaterPoser::onClickPoseSave(const LLSD& param)
{
	//BD - Values don't matter when not editing.
	if (onPoseSave())
	{
		LLNotificationsUtil::add("PoserExportXMLSuccess");
		return true;
	}
	return false;
}

bool BDFloaterPoser::onPoseSave()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item)
	{
		LL_WARNS("Posing") << "No avatar selected." << LL_ENDL;
		return false;
	}

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead())
	{
		LL_WARNS("Posing") << "Couldn't find avatar, dead?" << LL_ENDL;
		return false;
	}

	//BD - First and foremost before we do anything, check if the folder exists.
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");
	if (!gDirUtilp->fileExists(pathname))
	{
		LL_WARNS("Posing") << "Couldn't find folder: " << pathname << " - creating one." << LL_ENDL;
		LLFile::mkdir(pathname);
	}

	std::string filename = getChild<LLUICtrl>("pose_name")->getValue().asString();
	if (filename.empty())
		return false;

	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(filename) + ".xml");
	LLSD record;

	//BD - Create the header first.
	S32 version = 3;
	record["version"]["value"] = version;

	//BD - Now create the rest.
	for (S32 it = 0; it < 3; ++it)
	{
		for (auto element : mJointScrolls[it]->getAllData())
		{
			LLVector3 vec3;
			LLJoint* joint = (LLJoint*)element->getUserdata();
			if (joint)
			{
				std::string bone_name = joint->getName();
				record[bone_name] = joint->getName();
                if (joint->getJointNum() >= 134)
                    joint->getRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
                else
                    joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
				record[bone_name]["rotation"] = vec3.getValue();

				//BD - All bones support positions now.
				vec3 = it > JOINTS ? joint->getPosition() : joint->getTargetPosition();
				record[bone_name]["position"] = vec3.getValue();

				vec3 = joint->getScale();
				record[bone_name]["scale"] = vec3.getValue();

				if (it == JOINTS)
				{
					//BD - Save the enabled state per preset so we can switch bones on and off
					//     on demand inbetween poses additionally to globally.
					BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
					if (motion)
					{
						LLPose* pose = motion->getPose();
						if (pose)
						{
							if (pose->findJointState(joint))
							{
								record[bone_name]["enabled"] = true;
							}
							else
							{
								record[bone_name]["enabled"] = false;
							}
						}
					}
				}
			}
		}
	}
	
	llofstream file;
	file.open(full_path.c_str());
	//BD - Now lets actually write the file, whether it is writing a new one
	//     or just rewriting the previous one with a new header.
	LLSDSerialize::toPrettyXML(record, file);
	file.close();

	onPoseRefresh();

	//BD - Flash the poses button to give the user a visual cue where it went.
	getChild<LLButton>("extend")->setFlashing(true, true);
	return true;
}

void BDFloaterPoser::onPoseLoad()
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (!item) return;

	std::string pose_name = item->getColumn(0)->getValue().asString();

	gDragonAnimator.loadPose(pose_name);
	onJointRefresh();
}

void BDFloaterPoser::onPoseLoadSelective(const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (!item) return;

	std::string pose_name = item->getColumn(0)->getValue().asString();

	S32 load_type = 0;
	if (param.asString() == "rotation")
		load_type |= ROTATIONS;
	else if (param.asString() == "position")
		load_type |= POSITIONS;
	else if (param.asString() == "scale")
		load_type |= SCALES;
	else if (param.asString() == "rot_pos")
		load_type |= ROTATIONS | POSITIONS;
	else if (param.asString() == "rot_scale")
		load_type |= ROTATIONS | SCALES;
	else if (param.asString() == "pos_scale")
		load_type |= POSITIONS | SCALES;
	else if (param.asString() == "all")
		load_type |= ROTATIONS | POSITIONS | SCALES;

	gDragonAnimator.loadPose(pose_name, load_type);
	onJointRefresh();
}

void BDFloaterPoser::onPoseStart()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

    bool is_poseable = (avatar->isSelf() || gDragonLibrary.checkKonamiCode() || avatar->isControlAvatar()) ? true : avatar->getIsPoseable();
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if ((!motion || motion->isStopped()) && is_poseable)
	{
		avatar->setPosing();
		if (avatar->isSelf())
		{
			//BD - Grab our current defaults to revert to them when stopping the Poser.
			if(gAgentAvatarp->isFullyLoaded())
				onCollectDefaults();

			gAgent.stopFidget();
			gDragonStatus->setPosing(true);

            //BD - Sync our initial pose to everyone.
            onSyncPose();
		}
		avatar->startDefaultMotions();
		avatar->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		//BD - Reset everything, all rotations, positions and scales of all bones.
		onJointRotPosScaleReset();

		//BD - Clear posing when we're done now that we've safely endangered getting spaghetified.
		avatar->clearPosing();
		avatar->stopMotion(ANIM_BD_POSING_MOTION);

		if (avatar->isSelf())
		{
			gDragonStatus->setPosing(false);
		}

        if (mLastToolset)
        {
            LLToolMgr::getInstance()->setCurrentToolset(mLastToolset);
        }
	}
	//BD - Wipe the joint list.
	onJointRefresh();

	onPoseControlsRefresh();
}

void BDFloaterPoser::onPoseDelete()
{
	for (auto item : mPoseScroll->getAllSelected())
	{
		std::string filename = item->getColumn(0)->getValue().asString();
		std::string dirname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");

		if (gDirUtilp->deleteFilesInDir(dirname, gDragonLibrary.escapeString(filename) + ".xml") < 1)
		{
			LL_WARNS("Posing") << "Cannot remove file: " << filename << LL_ENDL;
		}
	}
	onPoseRefresh();
}

void BDFloaterPoser::onPoseControlsRefresh()
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	getChild<LLUICtrl>("delete_poses")->setEnabled(bool(item));
	mLoadPosesBtn->setEnabled(bool(item));
}

void BDFloaterPoser::onPoseMenuAction(const LLSD& param)
{
	onPoseLoadSelective(param);
}

////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterPoser::onJointRefresh()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	//BD - Getting collision volumes and attachment points.
	std::vector<std::string> cv_names, attach_names;
	avatar->getSortedJointNames(1, cv_names);
	avatar->getSortedJointNames(2, attach_names);

	bool is_posing = avatar->getPosing();
	mJointScrolls[JOINTS]->clearRows();
	mJointScrolls[COLLISION_VOLUMES]->clearRows();
	mJointScrolls[ATTACHMENT_BONES]->clearRows();

	LLVector3 rot;
	LLVector3 pos;
	LLVector3 scale;
	LLJoint* joint;
	for (S32 i = 0; (joint = avatar->getCharacterJoint(i)); ++i)
	{
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLSD row;
		const std::string joint_name = joint->getName();
		//BD - Show some categories to make it a bit easier finding out which
		//     bone belongs where and what they might be for those who can't use
		//     bone names.
		if (joint->mJointNum == 0 ||	//mPelvis
			joint->mJointNum == 8 ||	//mHead
			joint->mJointNum == 58 ||	//mCollarLeft
			joint->mJointNum == 77 ||	//mCollarRight
			joint->mJointNum == 96 ||	//mWingsRoot
			joint->mJointNum == 107 ||	//mHipRight
			joint->mJointNum == 112 ||	//mHipLeft
			joint->mJointNum == 117 ||	//mTail1
			joint->mJointNum == 123)	//mGroin
		{
			row["columns"][COL_ICON]["column"] = "icon";
			row["columns"][COL_ICON]["type"] = "icon";
			row["columns"][COL_ICON]["value"] = getString("icon_category");
			row["columns"][COL_NAME]["column"] = "joint";
			row["columns"][COL_NAME]["value"] = getString("title_" + joint_name);
			LLScrollListItem* element = mJointScrolls[JOINTS]->addElement(row);
			element->setEnabled(FALSE);
		}

		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = joint_name;

		if (is_posing)
		{
			//BD - Bone Rotations
			joint->getTargetRotation().getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
			row["columns"][COL_ROT_X]["column"] = "x";
			row["columns"][COL_ROT_X]["value"] = ll_round(rot.mV[VX], 0.001f);
			row["columns"][COL_ROT_Y]["column"] = "y";
			row["columns"][COL_ROT_Y]["value"] = ll_round(rot.mV[VY], 0.001f);
			row["columns"][COL_ROT_Z]["column"] = "z";
			row["columns"][COL_ROT_Z]["value"] = ll_round(rot.mV[VZ], 0.001f);
            rot.mV[VX] = ll_round(rot.mV[VX], 0.001f);
            rot.mV[VY] = ll_round(rot.mV[VY], 0.001f);
            rot.mV[VZ] = ll_round(rot.mV[VZ], 0.001f);
            row["columns"][COL_VISUAL_ROT]["column"] = "vis_rot";
            row["columns"][COL_VISUAL_ROT]["type"] = "vector";
            row["columns"][COL_VISUAL_ROT]["value"] = rot.getValue();

			//BD - All bones support positions now.
			pos = joint->getTargetPosition();
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);
            pos.mV[VX] = ll_round(pos.mV[VX], 0.001f);
            pos.mV[VY] = ll_round(pos.mV[VY], 0.001f);
            pos.mV[VZ] = ll_round(pos.mV[VZ], 0.001f);
            row["columns"][COL_VISUAL_POS]["column"] = "vis_pos";
            row["columns"][COL_VISUAL_POS]["type"] = "vector";
            row["columns"][COL_VISUAL_POS]["value"] = pos.getValue();

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
            scale.mV[VX] = ll_round(scale.mV[VX], 0.001f);
            scale.mV[VY] = ll_round(scale.mV[VY], 0.001f);
            scale.mV[VZ] = ll_round(scale.mV[VZ], 0.001f);
            row["columns"][COL_VISUAL_SCALE]["column"] = "vis_scale";
            row["columns"][COL_VISUAL_SCALE]["type"] = "vector";
            row["columns"][COL_VISUAL_SCALE]["value"] = scale.getValue();
		}

		LLScrollListItem* item = mJointScrolls[JOINTS]->addElement(row);
		item->setUserdata(joint);

		//BD - We need to check if we are posing or not, simply set all bones to deactivated
		//     when we are not posed otherwise they will remain on "enabled" state. This behavior
		//     could be confusing to the user, this is due to how animations work.
		if (is_posing)
		{
			BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					// BD - We do check here for the joint_state because otherwise we end up with the toggle
					//      state not appearing properly toggled/untoggled when we first refresh after firing
					//      up the poser. At the same time this is used to refresh all bone states when we
					//      load a pose.
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
		else
		{
			((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::NORMAL);
		}
	}

	//BD - We add an empty line into both lists and include an icon to automatically resize
	//     the list row heights to sync up with the height in our joint list. We remove it
	//     immediately after anyway.
	LLSD test_row;
	test_row["columns"][COL_ICON]["column"] = "icon";
	test_row["columns"][COL_ICON]["type"] = "icon";
	test_row["columns"][COL_ICON]["value"] = getString("icon_category");
	mJointScrolls[COLLISION_VOLUMES]->addElement(test_row);
	mJointScrolls[COLLISION_VOLUMES]->deleteSingleItem(0);
	mJointScrolls[ATTACHMENT_BONES]->addElement(test_row);
	mJointScrolls[ATTACHMENT_BONES]->deleteSingleItem(0);

	//BD - Collision Volumes
	for (auto name : cv_names)
	{
		joint = avatar->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint) continue;

		LLSD row;
		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = name;

		if (is_posing)
		{
			//BD - Bone Rotations
			//     It's stupid but we have to define the empty columns here and fill them with
			//     nothing otherwise the addRow() function is assuming that the sometimes missing
			//     rotation columns have an "empty" name and thus creates faulty extra columns.
			row["columns"][COL_ROT_X]["column"] = "x";
			row["columns"][COL_ROT_X]["value"] = "";
			row["columns"][COL_ROT_Y]["column"] = "y";
			row["columns"][COL_ROT_Y]["value"] = "";
			row["columns"][COL_ROT_Z]["column"] = "z";
			row["columns"][COL_ROT_Z]["value"] = "";

			//BD - Bone Positions
			pos = joint->getPosition();
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		LLScrollListItem* new_item = mJointScrolls[COLLISION_VOLUMES]->addElement(row);
		new_item->setUserdata(joint);
	}

	//BD - Attachment Bones
	for (auto name : attach_names)
	{
		joint = avatar->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint) continue;

		LLSD row;
		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = name;

		if (is_posing)
		{
			//BD - Bone Rotations
			//     It's stupid but we have to define the empty columns here and fill them with
			//     nothing otherwise the addRow() function is assuming that the sometimes missing
			//     rotation columns have an "empty" name and thus creates faulty extra columns.
			row["columns"][COL_ROT_X]["column"] = "x";
			row["columns"][COL_ROT_X]["value"] = "";
			row["columns"][COL_ROT_Y]["column"] = "y";
			row["columns"][COL_ROT_Y]["value"] = "";
			row["columns"][COL_ROT_Z]["column"] = "z";
			row["columns"][COL_ROT_Z]["value"] = "";

			//BD - Bone Positions
			pos = joint->getPosition();
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		LLScrollListItem* item = mJointScrolls[ATTACHMENT_BONES]->addElement(row);
		item->setUserdata(joint);
	}

	onJointControlsRefresh();
}

void BDFloaterPoser::onJointControlsRefresh()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	bool is_pelvis = false;
	bool is_posing = (avatar->isFullyLoaded() && avatar->getPosing());
    bool is_visual_posing = (is_posing && gSavedSettings.getBOOL("ShowVisualPosingTools"));
    bool is_poseable = (avatar->isSelf() || gDragonLibrary.checkKonamiCode() || avatar->isControlAvatar()) ? true : avatar->getIsPoseable();
	S32 index = mJointTabs->getCurrentPanelIndex();
    S32 curr_idx = mModifierTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
            if (is_visual_posing)
            {
                //BD - Beq's Visual Posing
                if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset
                    && LLToolMgr::getInstance()->getCurrentToolset() != gPoserToolset)
                {
                    mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
                    LLToolMgr::getInstance()->setCurrentToolset(gPoserToolset);
                }

                if (curr_idx == 0)
                {
                    LLToolMgr::getInstance()->getCurrentToolset()->selectTool(FSToolCompPose::getInstance());
                    FSToolCompPose::getInstance()->setAvatar(avatar);
                    FSToolCompPose::getInstance()->setJoint(joint);
                }
                if (curr_idx == 1)
                {
                    LLToolMgr::getInstance()->getCurrentToolset()->selectTool(BDToolCompPoseTranslate::getInstance());
                    BDToolCompPoseTranslate::getInstance()->setAvatar(avatar);
                    BDToolCompPoseTranslate::getInstance()->setJoint(joint);
                }
            }

			if (index == 0)
			{
				mRotationSliders[VX]->setValue(item->getColumn(COL_ROT_X)->getValue());
				mRotationSliders[VY]->setValue(item->getColumn(COL_ROT_Y)->getValue());
				mRotationSliders[VZ]->setValue(item->getColumn(COL_ROT_Z)->getValue());
			}

			//BD - All bones support positions now.
			is_pelvis = (joint->mJointNum == 0);

			mPositionSliders[VX]->setValue(item->getColumn(COL_POS_X)->getValue());
			mPositionSliders[VY]->setValue(item->getColumn(COL_POS_Y)->getValue());
			mPositionSliders[VZ]->setValue(item->getColumn(COL_POS_Z)->getValue());

			//BD - Bone Scales
			mScaleSliders[VX]->setValue(item->getColumn(COL_SCALE_X)->getValue());
			mScaleSliders[VY]->setValue(item->getColumn(COL_SCALE_Y)->getValue());
			mScaleSliders[VZ]->setValue(item->getColumn(COL_SCALE_Z)->getValue());

			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				//BD - If we don't use our default spherical interpolation, set it once.
				motion->setInterpolationTime(0.25f);
				motion->setInterpolationType(2);

				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					getChild<LLButton>("toggle_bone")->setValue(joint_state.notNull());
				}
			}
		}
	}

	getChild<LLButton>("toggle_bone")->setEnabled(item && is_posing && index == JOINTS);
	mStartPosingBtn->setValue(is_posing);
    mStartPosingBtn->setEnabled(avatar->isSelf() ? avatar->isFullyLoaded() : true);
    mStartPosingBtn->setVisible(is_poseable);
    getChild<LLUICtrl>("request_pose")->setVisible(!is_poseable);
	getChild<LLUICtrl>("pose_name")->setEnabled(is_posing);
	getChild<LLUICtrl>("save_poses")->setEnabled(is_posing);
	getChild<LLUICtrl>("joints_tabs")->setEnabled(is_posing);
    getChild<LLUICtrl>("sym_to")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("sym_from")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("symmetrize")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("mirror")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("recapture")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("copy")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("paste")->setEnabled(item && is_posing);
    getChild<LLUICtrl>("reset")->setEnabled(item && is_posing);

	//BD - Enable position tabs whenever positions are available, scales are always enabled
	//     unless we are editing attachment bones, rotations on the other hand are only
	//     enabled when editing joints.
	mModifierTabs->setEnabled(item && is_posing);

	F32 max_val = is_pelvis ? 20.f : 1.0f;
	mPositionSliders[VX]->setMaxValue(max_val);
	mPositionSliders[VY]->setMaxValue(max_val);
	mPositionSliders[VZ]->setMaxValue(max_val);
	mPositionSliders[VX]->setMinValue(-max_val);
	mPositionSliders[VY]->setMinValue(-max_val);
	mPositionSliders[VZ]->setMinValue(-max_val);

	//BD - Change our animator's target, make sure it is always up-to-date.
	gDragonAnimator.mTargetAvatar = avatar;
}

void BDFloaterPoser::onJointSet(LLUICtrl* ctrl, const LLSD& param)
{
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (!av_item) return;

    LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
    if (!avatar || avatar->isDead()) return;

    S32 index = mJointTabs->getCurrentPanelIndex();
    LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();
	if (!item)
		return;

	LLJoint* joint = (LLJoint*)item->getUserdata();
	if (!joint)
		return;

	//BD - Neat yet quick and direct way of rotating our bones.
	//     No more need to include bone rotation orders.
	F32 val = (F32)ctrl->getValue().asReal();
	S32 axis = param.asInteger();
	LLScrollListCell* cell[3] = { item->getColumn(COL_ROT_X), item->getColumn(COL_ROT_Y), item->getColumn(COL_ROT_Z) };
	LLQuaternion rot_quat = joint->getTargetRotation();
	LLMatrix3 rot_mat;
	F32 old_value;
	F32 new_value;
	LLVector3 vec3;

	old_value = (F32)cell[axis]->getValue().asReal();
	cell[axis]->setValue(ll_round(val, 0.001f));
	new_value = val - old_value;
	vec3.mV[axis] = new_value;
	rot_mat = LLMatrix3(vec3.mV[VX], vec3.mV[VY], vec3.mV[VZ]);
	rot_quat = LLQuaternion(rot_mat)*rot_quat;

    joint->setTargetRotation(rot_quat);
    //BD - Collision Volumes and Attachment Points need this to work.
    //     Any bone past 133 is assumed to be not a normal joint anymore.
    if (joint->getJointNum() >= 134)
    {
        joint->setRotation(rot_quat);
    }

    //BD - Reconstruct the entire rotation vector so we can properly display.
    vec3.mV[0] = (F32)cell[0]->getValue().asReal();
    vec3.mV[1] = (F32)cell[1]->getValue().asReal();
    vec3.mV[2] = (F32)cell[2]->getValue().asReal();
    item->getColumn(COL_VISUAL_ROT)->setValue(vec3.getValue());

	if (!mEasyRotations)
	{
		rot_quat.getEulerAngles(&vec3.mV[VX], &vec3.mV[VY], &vec3.mV[VZ]);
		S32 i = 0;
		while (i < 3)
		{
			if (i != axis)
			{
				cell[i]->setValue(ll_round(vec3.mV[i], 0.001f));
				mRotationSliders[i]->setValue(item->getColumn(i + 2)->getValue());
			}
			++i;
		}
	}

    //BD - Sync our bone changes.
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
        {
            if (c_avatar && c_avatar->getIsInSync())
            {
                const LLUUID& id = c_avatar->getID();
                mSyncList[id] = joint->getJointNum();
                bool found = false;
                for (const auto& item : mSyncLists)
                {
                    if (item[1].asInteger() == joint->getJointNum())
                        found = true;
                }

                if (!found)
                {
                    LLSD sync_info;
                    sync_info[0] = id;
                    sync_info[1] = joint->getJointNum();
                    mSyncLists.push_back(sync_info);
                }

                //BD - Reset the sync timer so it doesn't sync while we're dragging
                //     need a better way to handle this.
                mSyncTimer.reset();
            }
        }
    }
	
	//BD - If we are in Mirror mode, try to find the opposite bone of our currently
	//     selected one, for now this simply means we take the name and replace "Left"
	//     with "Right" and vise versa since all bones are conveniently that way.
	//     TODO: Do this when creating the joint list so we don't try to find the joint
	//     over and over again.
	if (mMirrorMode)
	{
		std::string mirror_joint_name = joint->getName();

        //BD - Attempt to find the "right" version of this bone first, we assume we always
                //     end up with the "left" version of a bone first.
        S32 idx = static_cast<S32>(joint->getName().find("Left"));
        if (idx != -1)
            mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
        else
        {
            //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
            idx = static_cast<S32>(joint->getName().find("L "));
            if (idx != -1)
                mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
        }
        //BD - Attempt to find the "right" version of this bone first, this is necessary
        //     because there are a couple bones starting with the "right" bone.
        idx = static_cast<S32>(joint->getName().find("Right"));
        if (idx != -1)
            mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
        else
        {
            //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
            idx = static_cast<S32>(joint->getName().find("R "));
            if (idx != -1)
                mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
        }

        LLJoint* mirror_joint = nullptr;
        if (mirror_joint_name != joint->getName())
            mirror_joint = avatar->mRoot->findJoint(mirror_joint_name);

		if (mirror_joint)
		{
			//BD - For the opposite joint we invert X and Z axis, everything else is directly applied
			//     exactly like we do it in our currently selected joint.
			if (axis != 1)
				val = -val;

			LLQuaternion inv_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
			mirror_joint->setTargetRotation(inv_quat);
            //BD - Collision Volumes and Attachment Points need this to work.
            //     Any bone past 133 is assumed to be not a normal joint anymore.
            if (joint->getJointNum() >= 134)
            {
                mirror_joint->setRotation(inv_quat);
            }

			//BD - We also need to find the opposite joint's list entry and change its values to reflect
			//     the new ones, doing this here is still better than causing a complete refresh.
			LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
			if (item2)
			{
                LLVector3 mirror_rot;
                inv_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
                item2->getColumn(COL_VISUAL_ROT)->setValue(mirror_rot.getValue());
			}

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                {
                    if (c_avatar && c_avatar->getIsInSync())
                    {
                        const LLUUID& id = c_avatar->getID();
                        mSyncList[id] = mirror_joint->getJointNum();

                        bool found = false;
                        for (const auto& item : mSyncLists)
                        {
                            if (item[1].asInteger() == joint->getJointNum())
                                found = true;
                        }

                        if (!found)
                        {
                            LLSD sync_info;
                            sync_info[0] = id;
                            sync_info[1] = joint->getJointNum();
                            mSyncLists.push_back(sync_info);
                        }

                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
		}
	}
}

void BDFloaterPoser::onJointPosSet(LLUICtrl* ctrl, const LLSD& param)
{
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			//BD - All bones support positions now.
			F32 val = (F32)ctrl->getValue().asReal();
			LLScrollListCell* cell[3] = { item->getColumn(COL_POS_X), item->getColumn(COL_POS_Y), item->getColumn(COL_POS_Z) };
			LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
								F32(cell[VY]->getValue().asReal()),
								F32(cell[VZ]->getValue().asReal()) };

			S32 dir = param.asInteger();
			vec3.mV[dir] = val;
			cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
			joint->setTargetPosition(vec3);
            //BD - Collision Volumes and Attachment Points need this to work.
            //     Any bone past 133 is assumed to be not a normal joint anymore.
            if (joint->getJointNum() >= 134)
            {
                joint->setPosition(vec3);
            }

            item->getColumn(COL_VISUAL_POS)->setValue(vec3.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
		}
	}
}

void BDFloaterPoser::onJointScaleSet(LLUICtrl* ctrl, const LLSD& param)
{
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			F32 val = (F32)ctrl->getValue().asReal();
			LLScrollListCell* cell[3] = { item->getColumn(COL_SCALE_X), item->getColumn(COL_SCALE_Y), item->getColumn(COL_SCALE_Z) };
			LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
							   F32(cell[VY]->getValue().asReal()),
							   F32(cell[VZ]->getValue().asReal()) };

			S32 dir = param.asInteger();
			vec3.mV[dir] = val;
			cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
			joint->setScale(vec3);

            item->getColumn(COL_VISUAL_SCALE)->setValue(vec3.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
		}
	}
}

void BDFloaterPoser::onJointChangeState()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		for (auto item : mJointScrolls[JOINTS]->getAllSelected())
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						motion->removeJointState(joint_state);
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::NORMAL);
					}
					else
					{
						motion->addJointToState(joint);
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
	}
	onJointControlsRefresh();
}

//BD - We use this to reset everything at once.
void BDFloaterPoser::onJointRotPosScaleReset()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	//BD - We don't support resetting bones for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

    S32 index = mJointTabs->getCurrentPanelIndex();

	for (auto item : mJointScrolls[index]->getAllData())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				//BD - Resetting rotations first.
				LLQuaternion quat;
				LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

				col_rot_x->setValue(0.000f);
				col_rot_y->setValue(0.000f);
				col_rot_z->setValue(0.000f);

				quat.setEulerAngles(0, 0, 0);
				joint->setTargetRotation(quat);
                //BD - Collision Volumes and Attachment Points need this to work.
                //     Any bone past 133 is assumed to be not a normal joint anymore.
                if (joint->getJointNum() >= 134)
                {
                    joint->setRotation(quat);
                }

				//BD - Resetting positions next.
				//     All bones support positions now.
				LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
				LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
				LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
				LLVector3 pos = mDefaultPositions[joint->getName()];

				col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
				col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
				col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
				joint->setTargetPosition(pos);
                //BD - Collision Volumes and Attachment Points need this to work.
                //     Any bone past 133 is assumed to be not a normal joint anymore.
                if (joint->getJointNum() >= 134)
                {
                    joint->setPosition(pos);
                }

				//BD - Resetting scales last.
				LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
				LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
				LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
				LLVector3 scale = mDefaultScales[joint->getName()];

				col_scale_x->setValue(ll_round(scale.mV[VX], 0.001f));
				col_scale_y->setValue(ll_round(scale.mV[VY], 0.001f));
				col_scale_z->setValue(ll_round(scale.mV[VZ], 0.001f));
				joint->setScale(scale);

                item->getColumn(COL_VISUAL_ROT)->setValue(LLVector3::zero.getValue());
                item->getColumn(COL_VISUAL_POS)->setValue(pos.getValue());
                item->getColumn(COL_VISUAL_SCALE)->setValue(scale.getValue());
			}
		}
	}

    //BD - Sync our reset pose to everyone.
    onSyncPose();

	onJointControlsRefresh();
}

//BD - Used to reset rotations only.
void BDFloaterPoser::onJointRotationReset()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	//BD - We do support resetting bone rotations for everyone however.
	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto new_item : mJointScrolls[index]->getAllSelected())
    {
        if (new_item)
        {
            LLJoint* joint = (LLJoint*)new_item->getUserdata();
            if (joint)
            {
                LLQuaternion quat;
                LLScrollListCell* col_x = new_item->getColumn(COL_ROT_X);
                LLScrollListCell* col_y = new_item->getColumn(COL_ROT_Y);
                LLScrollListCell* col_z = new_item->getColumn(COL_ROT_Z);

                col_x->setValue(0.000f);
                col_y->setValue(0.000f);
                col_z->setValue(0.000f);

                quat.setEulerAngles(0, 0, 0);
                joint->setTargetRotation(quat);
                //BD - Collision Volumes and Attachment Points need this to work.
                //     Any bone past 133 is assumed to be not a normal joint anymore.
                if (joint->getJointNum() >= 134)
                {
                    joint->setRotation(quat);
                }

                //BD - Sync our bone changes.
                for (LLCharacter* character : LLCharacter::sInstances)
                {
                    if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                    {
                        if (c_avatar && c_avatar->getIsInSync())
                        {
                            const LLUUID& id = c_avatar->getID();
                            mSyncList[id] = joint->getJointNum();
                            LLSD sync_info;
                            sync_info[0] = id;
                            sync_info[1] = joint->getJointNum();
                            mSyncLists.push_back(sync_info);
                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                            //     need a better way to handle this.
                            mSyncTimer.reset();
                        }
                    }
                }

                //BD - If we are in Mirror mode, try to find the opposite bone of our currently
                //     selected one, for now this simply means we take the name and replace "Left"
                //     with "Right" and vise versa since all bones are conveniently that way.
                //     TODO: Do this when creating the joint list so we don't try to find the joint
                //     over and over again.
                if (mMirrorMode)
                {
                    std::string mirror_joint_name = joint->getName();

                    //BD - Attempt to find the "right" version of this bone first, we assume we always
                    //     end up with the "left" version of a bone first.
                    S32 idx = static_cast<S32>(joint->getName().find("Left"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("L "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
                    }
                    //BD - Attempt to find the "right" version of this bone first, this is necessary
                    //     because there are a couple bones starting with the "right" bone.
                    idx = static_cast<S32>(joint->getName().find("Right"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("R "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
                    }

                    LLJoint* mirror_joint = nullptr;
                    if (mirror_joint_name != joint->getName())
                        mirror_joint = avatar->mRoot->findJoint(mirror_joint_name);

                    if (mirror_joint)
                    {
                        //BD - We also need to find the opposite joint's list entry and change its values to reflect
                        //     the new ones, doing this here is still better than causing a complete refresh.
                        LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
                        if (item2)
                        {
                            col_x = item2->getColumn(COL_ROT_X);
                            col_y = item2->getColumn(COL_ROT_Y);
                            col_z = item2->getColumn(COL_ROT_Z);

                            col_x->setValue(0.000f);
                            col_y->setValue(0.000f);
                            col_z->setValue(0.000f);

                            mirror_joint->setTargetRotation(quat);
                            //BD - Collision Volumes and Attachment Points need this to work.
                            //     Any bone past 133 is assumed to be not a normal joint anymore.
                            if (joint->getJointNum() >= 134)
                            {
                                mirror_joint->setRotation(quat);
                            }

                            item2->getColumn(COL_VISUAL_ROT)->setValue(LLVector3::zero.getValue());

                            //BD - Sync our bone changes.
                            for (LLCharacter* character : LLCharacter::sInstances)
                            {
                                if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                                {
                                    if (c_avatar && c_avatar->getIsInSync())
                                    {
                                        const LLUUID& id = c_avatar->getID();
                                        mSyncList[id] = mirror_joint->getJointNum();
                                        LLSD sync_info;
                                        sync_info[0] = id;
                                        sync_info[1] = mirror_joint->getJointNum();
                                        mSyncLists.push_back(sync_info);
                                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                                        //     need a better way to handle this.
                                        mSyncTimer.reset();
                                    }
                                }
                            }
                        }
                    }
                }
                new_item->getColumn(COL_VISUAL_ROT)->setValue(LLVector3::zero.getValue());
            }
        }
    }

	onJointControlsRefresh();
}

//BD - Used to reset positions, this is very tricky hence why it was separated.
//     It causes the avatar to flinch for a second which doesn't look as nice as resetting
//     rotations does.
void BDFloaterPoser::onJointPositionReset()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	//BD - We don't support resetting bones positions for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - When resetting positions, we don't use interpolation for now, it looks stupid.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        if (item)
        {
            LLJoint* joint = (LLJoint*)item->getUserdata();
            if (joint)
            {
                LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
                LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
                LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
                LLVector3 pos = mDefaultPositions[joint->getName()];
                pos.mV[VX] = ll_round(pos.mV[VX], 0.001f);
                pos.mV[VY] = ll_round(pos.mV[VY], 0.001f);
                pos.mV[VZ] = ll_round(pos.mV[VZ], 0.001f);

                col_pos_x->setValue(pos.mV[VX]);
                col_pos_y->setValue(pos.mV[VY]);
                col_pos_z->setValue(pos.mV[VZ]);
                joint->setTargetPosition(pos);
                //BD - Collision Volumes and Attachment Points need this to work.
                //     Any bone past 133 is assumed to be not a normal joint anymore.
                if (joint->getJointNum() >= 134)
                {
                    joint->setPosition(pos);
                }

                item->getColumn(COL_VISUAL_POS)->setValue(pos.getValue());

                //BD - Sync our bone changes.
                for (LLCharacter* character : LLCharacter::sInstances)
                {
                    if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                    {
                        if (c_avatar && c_avatar->getIsInSync())
                        {
                            const LLUUID& id = c_avatar->getID();
                            mSyncList[id] = joint->getJointNum();
                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                            //     need a better way to handle this.
                            mSyncTimer.reset();
                        }
                    }
                }
            }
        }
    }

	onJointControlsRefresh();
}

//BD - Used to reset scales only.
void BDFloaterPoser::onJointScaleReset()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	//BD - We don't support resetting bones scales for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - Clear all attachment bone scale changes we've done, they are not automatically
	//     reverted.
	for (auto item : mJointScrolls[index]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
				LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
				LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
				LLVector3 scale = mDefaultScales[joint->getName()];
                scale.mV[VX] = ll_round(scale.mV[VX], 0.001f);
                scale.mV[VY] = ll_round(scale.mV[VY], 0.001f);
                scale.mV[VZ] = ll_round(scale.mV[VZ], 0.001f);

				col_scale_x->setValue(scale.mV[VX]);
				col_scale_y->setValue(scale.mV[VY]);
				col_scale_z->setValue(scale.mV[VZ]);
				joint->setScale(scale);

                item->getColumn(COL_VISUAL_SCALE)->setValue(scale.getValue());

                //BD - Sync our bone changes.
                for (LLCharacter* character : LLCharacter::sInstances)
                {
                    if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                    {
                        if (c_avatar && c_avatar->getIsInSync())
                        {
                            const LLUUID& id = c_avatar->getID();
                            mSyncList[id] = joint->getJointNum();
                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                            //     need a better way to handle this.
                            mSyncTimer.reset();
                        }
                    }
                }
			}
		}
	}
	onJointControlsRefresh();
}

//BD - Used to revert rotations only.
void BDFloaterPoser::onJointRotationRevert()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	//BD - We do support reverting bone rotations for everyone however.
	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

    S32 index = mJointTabs->getCurrentPanelIndex();

	for (auto item : mJointScrolls[index]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				//BD - Reverting rotations first if there are any.
				LLQuaternion quat = mDefaultRotations[joint->getName()];
				LLVector3 rot;
				quat.getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
				LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

				col_rot_x->setValue(rot.mV[VX]);
				col_rot_y->setValue(rot.mV[VY]);
				col_rot_z->setValue(rot.mV[VZ]);

				joint->setTargetRotation(quat);
                //BD - Collision Volumes and Attachment Points need this to work.
                //     Any bone past 133 is assumed to be not a normal joint anymore.
                if (joint->getJointNum() >= 134)
                {
                    joint->setRotation(quat);
                }

                //BD - Sync our bone changes.
                for (LLCharacter* character : LLCharacter::sInstances)
                {
                    if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                    {
                        if (avatar && avatar->getIsInSync())
                        {
                            const LLUUID& id = avatar->getID();
                            mSyncList[id] = joint->getJointNum();
                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                            //     need a better way to handle this.
                            mSyncTimer.reset();
                        }
                    }
                }

				//BD - If we are in Mirror mode, try to find the opposite bone of our currently
				//     selected one, for now this simply means we take the name and replace "Left"
				//     with "Right" and vise versa since all bones are conveniently that way.
				//     TODO: Do this when creating the joint list so we don't try to find the joint
				//     over and over again.
				if (mMirrorMode)
				{
					std::string mirror_joint_name = joint->getName();

                    //BD - Attempt to find the "right" version of this bone first, we assume we always
                    //     end up with the "left" version of a bone first.
                    S32 idx = static_cast<S32>(joint->getName().find("Left"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("L "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
                    }
                    //BD - Attempt to find the "right" version of this bone first, this is necessary
                    //     because there are a couple bones starting with the "right" bone.
                    idx = static_cast<S32>(joint->getName().find("Right"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("R "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
                    }

                    LLJoint* mirror_joint = nullptr;
                    if (mirror_joint_name != joint->getName())
                        mirror_joint = avatar->mRoot->findJoint(mirror_joint_name);

					if (mirror_joint)
					{
						//BD - We also need to find the opposite joint's list entry and change its values to reflect
						//     the new ones, doing this here is still better than causing a complete refresh.
						LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
						if (item2)
						{
							col_rot_x = item2->getColumn(COL_ROT_X);
							col_rot_y = item2->getColumn(COL_ROT_Y);
							col_rot_z = item2->getColumn(COL_ROT_Z);

							col_rot_x->setValue(rot.mV[VX]);
							col_rot_y->setValue(rot.mV[VY]);
							col_rot_z->setValue(rot.mV[VZ]);

							mirror_joint->setTargetRotation(quat);
                            //BD - Collision Volumes and Attachment Points need this to work.
                            //     Any bone past 133 is assumed to be not a normal joint anymore.
                            if (joint->getJointNum() >= 134)
                            {
                                mirror_joint->setRotation(quat);
                            }

                            //BD - Sync our bone changes.
                            for (LLCharacter* character : LLCharacter::sInstances)
                            {
                                if (LLVOAvatar* c_avatar = (LLVOAvatar*)character)
                                {
                                    if (c_avatar && c_avatar->getIsInSync())
                                    {
                                        const LLUUID& id = c_avatar->getID();
                                        mSyncList[id] = mirror_joint->getJointNum();
                                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                                        //     need a better way to handle this.
                                        mSyncTimer.reset();
                                    }
                                }
                            }
						}
					}
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Flip our pose (mirror it)
void BDFloaterPoser::onFlipPose()
{
	LLVOAvatar* avatar = gDragonAnimator.mTargetAvatar;
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	LLJoint* joint = nullptr;
	bool flipped[512] = { false };

    for (S32 it = 0; it < 3; ++it)
    {
        for (auto item : mJointScrolls[it]->getAllData())
        {
            LLJoint* joint = (LLJoint*)item->getUserdata();
            if (joint)
            {
                S32 i = joint->getJointNum();
                //BD - Skip if we already flipped this bone.
                if (flipped[i]) continue;

                //BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
                if (!joint)	continue;

                LLVector3 rot, mirror_rot;
                LLQuaternion rot_quat, mirror_rot_quat;
                std::string joint_name = joint->getName();
                std::string mirror_joint_name = joint->getName();

                //BD - Attempt to find the "right" version of this bone first, we assume we always
                //     end up with the "left" version of a bone first.
                S32 idx = static_cast<S32>(joint->getName().find("Left"));
                if (idx != -1)
                    mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
                else
                {
                    //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                    idx = static_cast<S32>(joint->getName().find("L "));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
                }
                //BD - Attempt to find the "right" version of this bone first, this is necessary
                //     because there are a couple bones starting with the "right" bone.
                idx = static_cast<S32>(joint->getName().find("Right"));
                if (idx != -1)
                    mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
                else
                {
                    //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                    idx = static_cast<S32>(joint->getName().find("R "));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
                }

                LLJoint* mirror_joint = nullptr;
                if (mirror_joint_name != joint->getName())
                    mirror_joint = avatar->mRoot->findJoint(mirror_joint_name);

                //BD - Collect the joint and mirror joint entries and their cells, we need them later.
                LLScrollListItem* item1 = mJointScrolls[it]->getItemByLabel(joint_name, FALSE, COL_NAME);
                LLScrollListItem* item2 = nullptr;

                //BD - Get the rotation of our current bone and that of the mirror bone (if available).
                //     Flip our current bone's rotation and apply it to the mirror bone (if available).
                //     Flip the mirror bone's rotation (if available) and apply it to our current bone.
                //     If the mirror bone does not exist, flip the current bone rotation and use that.
                rot_quat = joint->getTargetRotation();
                LLQuaternion inv_rot_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
                inv_rot_quat.getEulerAngles(&rot[VX], &rot[VY], &rot[VZ]);

                if (mirror_joint)
                {
                    mirror_rot_quat = mirror_joint->getTargetRotation();
                    LLQuaternion inv_mirror_rot_quat = LLQuaternion(-mirror_rot_quat.mQ[VX], mirror_rot_quat.mQ[VY], -mirror_rot_quat.mQ[VZ], mirror_rot_quat.mQ[VW]);
                    inv_mirror_rot_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
                    mirror_joint->setTargetRotation(inv_rot_quat);
                    joint->setTargetRotation(inv_mirror_rot_quat);
                    //BD - Collision Volumes and Attachment Points need this to work.
                    //     Any bone past 133 is assumed to be not a normal joint anymore.
                    if (joint->getJointNum() >= 134)
                    {
                        mirror_joint->setRotation(inv_rot_quat);
                        joint->setRotation(inv_mirror_rot_quat);
                    }

                    item2 = mJointScrolls[it]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);

                    //BD - Make sure we flag this bone as flipped so we skip it next time we iterate over it.
                    flipped[mirror_joint->getJointNum()] = true;
                }
                else
                {
                    joint->setTargetRotation(inv_rot_quat);
                    if (joint->getJointNum() >= 134)
                    {
                        joint->setRotation(inv_rot_quat);
                    }
                }

                S32 axis = 0;
                while (axis <= 2)
                {
                    //BD - Now flip the list entry values.
                    if (item1)
                    {
                        if (mirror_joint)
                            item1->getColumn(axis + 2)->setValue(ll_round(mirror_rot[axis], 0.001f));
                        else
                            item1->getColumn(axis + 2)->setValue(ll_round(rot[axis], 0.001f));
                    }

                    //BD - Now flip the mirror joint list entry values.
                    if (item2)
                        item2->getColumn(axis + 2)->setValue(ll_round(rot[axis], 0.001f));

                    ++axis;
                }
                flipped[i] = true;
            }
        }
    }

    //BD - Sync our new pose to everyone.
    onSyncPose();
}

//BD - Copy and mirror one side's joints to the other (symmetrize the pose).
void BDFloaterPoser::onPoseSymmetrize(const LLSD& param)
{
	LLVOAvatar* avatar = gDragonAnimator.mTargetAvatar;
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	LLJoint* joint = nullptr;
	bool flipped[512] = { false };
	bool flipLeft = false;
	if (param.asInteger() == 0)
	{
		flipLeft = true;
	}

    for (S32 it = 0; it < 3; ++it)
    {
        for (auto item : mJointScrolls[it]->getAllData())
        {
            LLJoint* joint = (LLJoint*)item->getUserdata();
            if (joint)
            {
                S32 i = joint->getJointNum();
                //BD - Skip if we already flipped this bone.
                if (flipped[i]) continue;

                //BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
                if (!joint)	continue;

                LLVector3 rot, mirror_rot;
                LLQuaternion rot_quat, mirror_rot_quat;
                std::string joint_name = joint->getName();
                std::string mirror_joint_name = joint->getName();
                if (!flipLeft)
                {
                    //BD - Attempt to find the "right" version of this bone first, we assume we always
                    //     end up with the "left" version of a bone first.
                    S32 idx = static_cast<S32>(joint->getName().find("Left"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("L "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
                    }
                }
                else
                {
                    //BD - Attempt to find the "right" version of this bone first, this is necessary
                    //     because there are a couple bones starting with the "right" bone.
                    S32 idx = static_cast<S32>(joint->getName().find("Right"));
                    if (idx != -1)
                        mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
                    else
                    {
                        //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                        idx = static_cast<S32>(joint->getName().find("R "));
                        if (idx != -1)
                            mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
                    }
                }

                LLJoint* mirror_joint = nullptr;
                if (mirror_joint_name != joint->getName())
                    mirror_joint = avatar->mRoot->findJoint(mirror_joint_name);

                //BD - Collect the joint and mirror joint entries and their cells, we need them later.
                LLScrollListItem* item2 = nullptr;

                //BD - Get the rotation of our current bone and that of the mirror bone (if available).
                //     Flip our current bone's rotation and apply it to the mirror bone (if available).
                //     Flip the mirror bone's rotation (if available) and apply it to our current bone.
                //     If the mirror bone does not exist, flip the current bone rotation and use that.
                rot_quat = joint->getTargetRotation();
                LLQuaternion inv_rot_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
                inv_rot_quat.getEulerAngles(&rot[VX], &rot[VY], &rot[VZ]);

                if (mirror_joint)
                {
                    mirror_joint->setTargetRotation(inv_rot_quat);
                    if (joint->getJointNum() >= 134)
                    {
                        mirror_joint->setRotation(inv_rot_quat);
                    }

                    item2 = mJointScrolls[it]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);

                    //BD - Make sure we flag this bone as flipped so we skip it next time we iterate over it.
                    flipped[mirror_joint->getJointNum()] = true;
                }

                S32 axis = 0;
                while (axis <= 2)
                {
                    //BD - Now flip the mirror joint list entry values.
                    if (item2)
                        item2->getColumn(axis + 2)->setValue(ll_round(rot[axis], 0.001f));

                    ++axis;
                }
                flipped[i] = true;
            }
        }
    }

    //BD - Sync our initial pose to everyone.
    onSyncPose();
}

//BD - Recapture the current joint's values.
void BDFloaterPoser::onJointsRecapture()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	LLQuaternion rot;
	LLVector3 pos;

	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		LLPose* pose = motion->getPose();
		if (pose)
		{
            for (S32 it = 0; it < 3; ++it)
            {
                for (auto item : mJointScrolls[it]->getAllData())
                {
                    if (item)
                    {
                        LLJoint* joint = (LLJoint*)item->getUserdata();
                        if (joint)
                        {
                            // BD - Check for the joint state (whether a bone is enabled or not)
                            //      If not proceed.
                            LLPointer<LLJointState> joint_state = pose->findJointState(joint);
                            if (!joint_state)
                            {
                                //BD - First gather the current rotation and position.
                                rot = joint->getRotation();
                                pos = joint->getPosition();

                                //BD - Now, re-add the joint state and enable changing the pose.
                                motion->addJointToState(joint);
                                ((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);

                                //BD - Apply the newly collected rotation and position to the pose.
                                joint->setTargetRotation(rot);
                                joint->setTargetPosition(pos);
                                if (joint->getJointNum() >= 134)
                                {
                                    joint->setRotation(rot);
                                    joint->setPosition(pos);
                                }

                                //BD - Sync our bone changes.
                                for (LLCharacter* character : LLCharacter::sInstances)
                                {
                                    if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                                    {
                                        if (avatar && avatar->getIsInSync())
                                        {
                                            const LLUUID& id = avatar->getID();
                                            mSyncList[id] = joint->getJointNum();
                                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                                            //     need a better way to handle this.
                                            mSyncTimer.reset();
                                        }
                                    }
                                }

                                //BD - Get all columns and fill in the new values.
                                LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
                                LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
                                LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

                                LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
                                LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
                                LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);

                                LLVector3 euler_rot;
                                rot.getEulerAngles(&euler_rot.mV[VX], &euler_rot.mV[VY], &euler_rot.mV[VZ]);

                                euler_rot.mV[VX] = ll_round(euler_rot.mV[VX], 0.001f);
                                euler_rot.mV[VY] = ll_round(euler_rot.mV[VY], 0.001f);
                                euler_rot.mV[VZ] = ll_round(euler_rot.mV[VZ], 0.001f);

                                pos.mV[VX] = ll_round(pos.mV[VX], 0.001f);
                                pos.mV[VY] = ll_round(pos.mV[VY], 0.001f);
                                pos.mV[VZ] = ll_round(pos.mV[VZ], 0.001f);

                                col_rot_x->setValue(euler_rot.mV[VX]);
                                col_rot_y->setValue(euler_rot.mV[VY]);
                                col_rot_z->setValue(euler_rot.mV[VZ]);

                                col_pos_x->setValue(pos.mV[VX]);
                                col_pos_y->setValue(pos.mV[VY]);
                                col_pos_z->setValue(pos.mV[VZ]);

                                item->getColumn(COL_VISUAL_ROT)->setValue(euler_rot.getValue());
                                item->getColumn(COL_VISUAL_POS)->setValue(pos.getValue());
                            }
                        }
                    }
                }
            }
		}
	}
}

//BD - Modified version to recapture only the selected bone(s).
void BDFloaterPoser::onJointRecapture()
{
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (!av_item) return;

    LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
    if (!avatar || avatar->isDead()) return;

    if (!(avatar->getRegion() == gAgent.getRegion())) return;

    LLQuaternion rot;
    LLVector3 pos;

    BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
    if (motion)
    {
        S32 index = mJointTabs->getCurrentPanelIndex();

        for (auto item : mJointScrolls[index]->getAllSelected())
        {
            LLJoint* joint = (LLJoint*)item->getUserdata();
            if (joint)
            {
                //BD - First gather the current rotation and position.
                rot = joint->getRotation();
                pos = joint->getPosition();

                //BD - Apply the newly collected rotation and position to the pose.
                joint->setTargetRotation(rot);
                joint->setTargetPosition(pos);
                if (joint->getJointNum() >= 134)
                {
                    joint->setRotation(rot);
                    joint->setPosition(pos);
                }

                //BD - Sync our bone changes.
                for (LLCharacter* character : LLCharacter::sInstances)
                {
                    if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                    {
                        if (avatar && avatar->getIsInSync())
                        {
                            const LLUUID& id = avatar->getID();
                            mSyncList[id] = joint->getJointNum();
                            //BD - Reset the sync timer so it doesn't sync while we're dragging
                            //     need a better way to handle this.
                            mSyncTimer.reset();
                        }
                    }
                }

                //BD - Get all columns and fill in the new values.
                LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
                LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
                LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

                LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
                LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
                LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);

                LLVector3 euler_rot;
                rot.getEulerAngles(&euler_rot.mV[VX], &euler_rot.mV[VY], &euler_rot.mV[VZ]);

                euler_rot.mV[VX] = ll_round(euler_rot.mV[VX], 0.001f);
                euler_rot.mV[VY] = ll_round(euler_rot.mV[VY], 0.001f);
                euler_rot.mV[VZ] = ll_round(euler_rot.mV[VZ], 0.001f);

                pos.mV[VX] = ll_round(pos.mV[VX], 0.001f);
                pos.mV[VY] = ll_round(pos.mV[VY], 0.001f);
                pos.mV[VZ] = ll_round(pos.mV[VZ], 0.001f);

                col_rot_x->setValue(euler_rot.mV[VX]);
                col_rot_y->setValue(euler_rot.mV[VY]);
                col_rot_z->setValue(euler_rot.mV[VZ]);

                col_pos_x->setValue(pos.mV[VX]);
                col_pos_y->setValue(pos.mV[VY]);
                col_pos_z->setValue(pos.mV[VZ]);

                item->getColumn(COL_VISUAL_ROT)->setValue(euler_rot.getValue());
                item->getColumn(COL_VISUAL_POS)->setValue(pos.getValue());
            }
        }
    }
}

//BD - Poser Utility Functions
void BDFloaterPoser::onJointPasteRotation()
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
            LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
            LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

            LLVector3 euler_rot;
            LLQuaternion rot = (LLQuaternion)mClipboard["rot"];

            joint->setTargetRotation(rot);
            if (joint->getJointNum() >= 134)
            {
                joint->setRotation(rot);
            }

            rot.getEulerAngles(&euler_rot.mV[VX], &euler_rot.mV[VY], &euler_rot.mV[VZ]);
            euler_rot.mV[VX] = ll_round(euler_rot.mV[VX], 0.001f);
            euler_rot.mV[VY] = ll_round(euler_rot.mV[VY], 0.001f);
            euler_rot.mV[VZ] = ll_round(euler_rot.mV[VZ], 0.001f);

            col_rot_x->setValue(euler_rot.mV[VX]);
            col_rot_y->setValue(euler_rot.mV[VY]);
            col_rot_z->setValue(euler_rot.mV[VZ]);

            item->getColumn(COL_VISUAL_ROT)->setValue(euler_rot.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
        }
    }
}

void BDFloaterPoser::onJointPastePosition()
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
            LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
            LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);

            LLVector3 pos = (LLVector3)mClipboard["pos"];

            joint->setTargetPosition(pos);
            if (joint->getJointNum() >= 134)
            {
                joint->setPosition(pos);
            }

            pos.mV[VX] = ll_round(pos.mV[VX], 0.001f);
            pos.mV[VY] = ll_round(pos.mV[VY], 0.001f);
            pos.mV[VZ] = ll_round(pos.mV[VZ], 0.001f);

            col_pos_x->setValue(pos.mV[VX]);
            col_pos_y->setValue(pos.mV[VY]);
            col_pos_z->setValue(pos.mV[VZ]);

            item->getColumn(COL_VISUAL_POS)->setValue(pos.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
        }
    }
}

void BDFloaterPoser::onJointPasteScale()
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
            LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
            LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
            LLVector3 scale = (LLVector3)mClipboard["scale"];

            joint->setScale(scale);

            scale.mV[VX] = ll_round(scale.mV[VX], 0.001f);
            scale.mV[VY] = ll_round(scale.mV[VY], 0.001f);
            scale.mV[VZ] = ll_round(scale.mV[VZ], 0.001f);

            col_scale_x->setValue(scale.mV[VX]);
            col_scale_y->setValue(scale.mV[VY]);
            col_scale_z->setValue(scale.mV[VZ]);

            item->getColumn(COL_VISUAL_SCALE)->setValue(scale.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
        }
    }
}

void BDFloaterPoser::onJointMirror()
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            LLVector3 euler_rot;
            LLQuaternion rot_quat = joint->getTargetRotation();

            //BD - Simply mirror the current bone's rotation like we'd do if we pressed the mirror
            //     button without a mirror bone available.
            LLQuaternion inv_rot_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
            inv_rot_quat.getEulerAngles(&euler_rot[VX], &euler_rot[VY], &euler_rot[VZ]);
            joint->setTargetRotation(inv_rot_quat);
            if (joint->getJointNum() >= 134)
            {
                joint->setRotation(inv_rot_quat);
            }

            LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
            LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
            LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

            euler_rot.mV[VX] = ll_round(euler_rot.mV[VX], 0.001f);
            euler_rot.mV[VY] = ll_round(euler_rot.mV[VY], 0.001f);
            euler_rot.mV[VZ] = ll_round(euler_rot.mV[VZ], 0.001f);

            col_rot_x->setValue(euler_rot.mV[VX]);
            col_rot_y->setValue(euler_rot.mV[VY]);
            col_rot_z->setValue(euler_rot.mV[VZ]);

            item->getColumn(COL_VISUAL_ROT)->setValue(euler_rot.getValue());

            //BD - Sync our bone changes.
            for (LLCharacter* character : LLCharacter::sInstances)
            {
                if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                {
                    if (avatar && avatar->getIsInSync())
                    {
                        const LLUUID& id = avatar->getID();
                        mSyncList[id] = joint->getJointNum();
                        //BD - Reset the sync timer so it doesn't sync while we're dragging
                        //     need a better way to handle this.
                        mSyncTimer.reset();
                    }
                }
            }
        }
    }
}

void BDFloaterPoser::onJointSymmetrize(bool from)
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    for (auto item : mJointScrolls[index]->getAllSelected())
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            std::string joint_name = joint->getName();
            std::string mirror_joint_name = joint->getName();

            //BD - Attempt to find the "right" version of this bone first, we assume we always
                //     end up with the "left" version of a bone first.
            S32 idx = static_cast<S32>(joint->getName().find("Left"));
            if (idx != -1)
                mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
            else
            {
                //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                idx = static_cast<S32>(joint->getName().find("L "));
                if (idx != -1)
                    mirror_joint_name.replace(idx, mirror_joint_name.length(), "R ");
            }
            //BD - Attempt to find the "right" version of this bone first, this is necessary
            //     because there are a couple bones starting with the "right" bone.
            idx = static_cast<S32>(joint->getName().find("Right"));
            if (idx != -1)
                mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");
            else
            {
                //BD - We failed, try again, Attachment Bones use both "L " as well as "Left"
                idx = static_cast<S32>(joint->getName().find("R "));
                if (idx != -1)
                    mirror_joint_name.replace(idx, mirror_joint_name.length(), "L ");
            }

            LLJoint* mirror_joint = nullptr;
            if (mirror_joint_name != joint->getName())
                mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);

            //BD - Get the rotation of the mirror bone (if available).
            //     Flip the mirror bone's rotation (if available) and apply it to our current bone.
            //     ELSE
            //     Flip the our bone's rotation and apply it to the mirror bone (if available).
            if (mirror_joint)
            {
                LLVector3 mirror_rot;
                LLQuaternion mirror_rot_quat;
                LLScrollListItem* item2 = mJointScrolls[index]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
                if (from)
                {
                    mirror_rot_quat = mirror_joint->getTargetRotation();
                    LLQuaternion inv_mirror_rot_quat = LLQuaternion(-mirror_rot_quat.mQ[VX], mirror_rot_quat.mQ[VY], -mirror_rot_quat.mQ[VZ], mirror_rot_quat.mQ[VW]);
                    inv_mirror_rot_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
                    joint->setTargetRotation(inv_mirror_rot_quat);
                    if (joint->getJointNum() >= 134)
                    {
                        joint->setRotation(inv_mirror_rot_quat);
                    }

                    LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
                    LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
                    LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

                    mirror_rot.mV[VX] = ll_round(mirror_rot.mV[VX], 0.001f);
                    mirror_rot.mV[VY] = ll_round(mirror_rot.mV[VY], 0.001f);
                    mirror_rot.mV[VZ] = ll_round(mirror_rot.mV[VZ], 0.001f);

                    col_rot_x->setValue(mirror_rot.mV[VX]);
                    col_rot_y->setValue(mirror_rot.mV[VY]);
                    col_rot_z->setValue(mirror_rot.mV[VZ]);

                    item2->getColumn(COL_VISUAL_ROT)->setValue(mirror_rot.getValue());

                    //BD - Sync our bone changes.
                    for (LLCharacter* character : LLCharacter::sInstances)
                    {
                        if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                        {
                            if (avatar && avatar->getIsInSync())
                            {
                                const LLUUID& id = avatar->getID();
                                mSyncList[id] = joint->getJointNum();
                                //BD - Reset the sync timer so it doesn't sync while we're dragging
                                //     need a better way to handle this.
                                mSyncTimer.reset();
                            }
                        }
                    }
                }
                else
                {
                    mirror_rot_quat = joint->getTargetRotation();
                    LLQuaternion inv_mirror_rot_quat = LLQuaternion(-mirror_rot_quat.mQ[VX], mirror_rot_quat.mQ[VY], -mirror_rot_quat.mQ[VZ], mirror_rot_quat.mQ[VW]);
                    inv_mirror_rot_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
                    mirror_joint->setTargetRotation(inv_mirror_rot_quat);
                    if (mirror_joint->getJointNum() >= 134)
                    {
                        mirror_joint->setRotation(inv_mirror_rot_quat);
                    }

                    LLScrollListCell* col_rot_x = item2->getColumn(COL_ROT_X);
                    LLScrollListCell* col_rot_y = item2->getColumn(COL_ROT_Y);
                    LLScrollListCell* col_rot_z = item2->getColumn(COL_ROT_Z);

                    mirror_rot.mV[VX] = ll_round(mirror_rot.mV[VX], 0.001f);
                    mirror_rot.mV[VY] = ll_round(mirror_rot.mV[VY], 0.001f);
                    mirror_rot.mV[VZ] = ll_round(mirror_rot.mV[VZ], 0.001f);

                    col_rot_x->setValue(mirror_rot.mV[VX]);
                    col_rot_y->setValue(mirror_rot.mV[VY]);
                    col_rot_z->setValue(mirror_rot.mV[VZ]);

                    item2->getColumn(COL_VISUAL_ROT)->setValue(mirror_rot.getValue());

                    //BD - Sync our bone changes.
                    for (LLCharacter* character : LLCharacter::sInstances)
                    {
                        if (LLVOAvatar* avatar = (LLVOAvatar*)character)
                        {
                            if (avatar && avatar->getIsInSync())
                            {
                                const LLUUID& id = avatar->getID();
                                mSyncList[id] = mirror_joint->getJointNum();
                                //BD - Reset the sync timer so it doesn't sync while we're dragging
                                //     need a better way to handle this.
                                mSyncTimer.reset();
                            }
                        }
                    }
                }
            }
        }
    }
}

void BDFloaterPoser::onJointCopyTransforms()
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();
    if (item)
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            mClipboard["rot"] = joint->getTargetRotation().getValue();
            mClipboard["pos"] = joint->getTargetPosition().getValue();
            mClipboard["scale"] = joint->getScale().getValue();
            LL_INFOS("Posing") << "Copied all transforms " << LL_ENDL;
        }
    }
}

bool BDFloaterPoser::onJointContextMenuEnable(const LLSD& param)
{
	std::string action = param.asString();
	if (action == "clipboard")
	{
		return mClipboard.has("rot");
	}
	if (action == "enable_bone")
	{
		LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
		if (!av_item) return false;

		LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
		if (!avatar || avatar->isDead()) return false;

		LLScrollListItem* item = mJointScrolls[JOINTS]->getFirstSelected();
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					return joint_state;
				}
			}
		}
	}
	return false;
}

void BDFloaterPoser::onJointContextMenuAction(const LLSD& param)
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string action = param.asString();
	if (action == "copy_transforms")
	{
		onJointCopyTransforms();
	}
	else if (action == "paste_rot")
	{
		onJointPasteRotation();
	}
	else if (action == "paste_pos")
	{
		onJointPastePosition();
	}
	else if (action == "paste_scale")
	{
		onJointPasteScale();
	}
	else if (action == "paste_rot_pos")
	{
		onJointPasteRotation();
		onJointPastePosition();
	}
	else if (action == "paste_rot_scale")
	{
		onJointPasteRotation();
		onJointPasteScale();
	}
	else if (action == "paste_pos_scale")
	{
		onJointPastePosition();
		onJointPasteScale();
	}
	else if (action == "paste_all")
	{
		onJointPasteRotation();
		onJointPastePosition();
		onJointPasteScale();
	}
	else if (action == "symmetrize_from")
	{
		onJointSymmetrize(true);
	}
	else if (action == "symmetrize_to")
	{
		onJointSymmetrize(false);
	}
	else if (action == "mirror")
	{
		onJointMirror();
	}
	else if (action == "recapture")
	{
		onJointRecapture();
	}
	else if (action == "enable_bone")
	{
		onJointChangeState();
	}
	else if (action == "enable_override")
	{

	}
	else if (action == "enable_offset")
	{

	}
	else if (action == "reset_rot")
	{
		onJointRotationReset();
        onSyncPose();
	}
	else if (action == "reset_pos")
	{
		onJointPositionReset();
        onSyncPose();
	}
	else if (action == "reset_scale")
	{
		onJointScaleReset();
        onSyncPose();
	}
	else if (action == "reset_all")
	{
		//BD - We do all 3 here because the combined function resets all bones regardless of
		//     our selection, these only reset the selected ones.
		onJointRotationReset();
		onJointPositionReset();
		onJointScaleReset();
	}
}

//BD - This is used to collect all default values at the beginning to revert to later on.
void BDFloaterPoser::onCollectDefaults()
{
	LLQuaternion rot;
	LLVector3 pos;
	LLVector3 scale;
	LLJoint* joint;

	//BD - Getting collision volumes and attachment points.
	std::vector<std::string> joint_names, cv_names, attach_names;
	gAgentAvatarp->getSortedJointNames(0, joint_names);
	gAgentAvatarp->getSortedJointNames(1, cv_names);
	gAgentAvatarp->getSortedJointNames(2, attach_names);

	mDefaultRotations.clear();
	mDefaultScales.clear();
	mDefaultPositions.clear();

	for (auto name : joint_names)
	{
		joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLSD row;

		rot = joint->getTargetRotation();
		mDefaultRotations.insert(std::pair<std::string, LLQuaternion>(name, rot));

		scale = joint->getScale();
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));

		pos = joint->getPosition();
		mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
	}

	//BD - Collision Volumes
	for (auto name : cv_names)
	{
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

        rot = joint->getRotation();
        mDefaultRotations.insert(std::pair<std::string, LLQuaternion>(name, rot));

        scale = joint->getScale();
        mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));

        pos = joint->getPosition();
        mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
	}

	//BD - Attachment Bones
	for (auto name : attach_names)
	{
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

        rot = joint->getRotation();
        mDefaultRotations.insert(std::pair<std::string, LLQuaternion>(name, rot));

        //BD - We always get the values but we don't write them out as they are not relevant for the
        //     user yet but we need them to establish default values we revert to later on.
        scale = joint->getScale();
        mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));

        //BD - All bones support positions now.
        //     We always get the values but we don't write them out as they are not relevant for the
        //     user yet but we need them to establish default values we revert to later on.
        pos = joint->getPosition();
        mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
	}
}

////////////////////////////////
//BD - Misc Functions
////////////////////////////////

void BDFloaterPoser::loadPoseRotations(std::string name, LLVector3 *rotations)
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			if (pose["rotation"].isDefined())
			{
				rotations[joint_num].setValue(pose["rotation"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::loadPosePositions(std::string name, LLVector3 *positions)
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			//BD - Position information is only ever written when it is actually safe to do.
			//     It's safe to assume that IF information is available it's safe to apply.
			if (pose["position"].isDefined())
			{
				positions[joint_num].setValue(pose["position"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::loadPoseScales(std::string name, LLVector3 *scales)
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			//BD - Bone Scales
			if (pose["scale"].isDefined())
			{
				scales[joint_num].setValue(pose["scale"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::onUpdateLayout()
{
	if (!this->isMinimized())
	{
		bool poses_expanded = getChild<LLButton>("extend")->getValue();
		getChild<LLUICtrl>("poses_layout")->setVisible(poses_expanded);

		S32 collapsed_width = getChild<LLPanel>("min_panel")->getRect().getWidth();
		S32 expanded_width = getChild<LLPanel>("max_panel")->getRect().getWidth();
		S32 floater_width = poses_expanded ? expanded_width : collapsed_width;
		this->reshape(floater_width, this->getRect().getHeight());
	}
}

//BD - Animesh
void BDFloaterPoser::onAvatarsSelect()
{
	//BD - Whenever we select an avatar in the list, check if the selected Avatar is still
	//     valid and/or if new avatars have become valid for posing.
	onAvatarsRefresh();

	//BD - Now that we selected an avatar we can refresh the joint list to have all bones
	//     mapped to that avatar so we can immediately start posing them or continue doing so.
	//     This will automatically invoke a onJointControlsRefresh()
	onJointRefresh();

	//BD - Now that we support animating multiple avatars we also need to refresh all controls
	//     and animation/pose lists for them when we switch to make it as easy as possible to
	//     quickly switch back and forth and make edits.
	onUpdateLayout();

	onPoseControlsRefresh();

	//BD - Disable the Start Posing button if we haven't loaded yet.
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (av_item)
	{
		LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
        if (avatar)
        {
            //BD - Flash the Start button but only if the avatar isn't already posing.
            if(!avatar->mIsPosing)
                mStartPosingBtn->setFlashing(true, true);
        }
	}
}

void BDFloaterPoser::onAvatarsRefresh()
{
	//BD - Flag all items first, we're going to unflag them when they are valid.
	for (LLScrollListItem* av_item : mAvatarScroll->getAllData())
	{
		if (av_item)
		{
            av_item->setFlagged(true);
		}
	}

    bool create_new = true;
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        create_new = true;
        if (LLVOAvatar* avatar = (LLVOAvatar*)character)
        {
            bool is_poseable = (avatar->isSelf() || gDragonLibrary.checkKonamiCode() || avatar->isControlAvatar()) ? true : avatar->getIsPoseable();
            for (LLScrollListItem* av_item : mAvatarScroll->getAllData())
            {
                if (av_item)
                {
                    if (avatar == (LLVOAvatar*)av_item->getUserdata())
                    {
                        //BD - Avatar is still valid unflag it from removal.
                        av_item->setFlagged(false);

                        //BD - Check if our permission to pose has changed.
                        av_item->getColumn(3)->setValue(is_poseable ? "☑" : "☐");

                        //BD - Avatar is being posed but is no longer allowed to be posed.
                        if (avatar->mIsPosing && !is_poseable)
                        {
                            //BD - If we have the avatar currently selected simply toggle as if we clicked stop.
                            if (av_item->getSelected())
                            {
                                onPoseStart();
                            }
                            else
                            {
                                //BD - Else stop posing manually.
                                avatar->setIsPoseable(false);
                                avatar->clearPosing();
                                avatar->stopMotion(ANIM_BD_POSING_MOTION);

                                //BD - Refresh all controls.
                                onPoseControlsRefresh();
                            }
                        }

                        create_new = false;
                    }
                }
            }

            if (create_new)
            {
                LLUUID uuid = avatar->getID();
                LLAvatarName av_name;
                LLAvatarNameCache::get(uuid, &av_name);

                LLSD row;
                row["columns"][0]["column"] = "icon";
                row["columns"][0]["type"] = "icon";
                if (!avatar->isControlAvatar())
                    row["columns"][0]["value"] = getString("icon_avatar");
                else
                    row["columns"][0]["value"] = getString("icon_object");
                row["columns"][1]["column"] = "name";
                if (!avatar->isControlAvatar())
                    row["columns"][1]["value"] = av_name.getDisplayName();
                else
                    row["columns"][1]["value"] = avatar->getFullname();
                row["columns"][2]["column"] = "uuid";
                row["columns"][2]["value"] = avatar->getID();
                row["columns"][3]["column"] = "control_avatar";
                row["columns"][3]["value"] = !avatar->isControlAvatar();
                row["columns"][4]["column"] = "is_poseable";
                row["columns"][4]["value"] = is_poseable ? "☑" : "☐";
                LLScrollListItem* new_item = mAvatarScroll->addElement(row);
                new_item->setUserdata(avatar);
                new_item->setFlagged(false);
            }
        }
    }

	//BD - Now safely delete all items so we can start adding the missing ones.
	mAvatarScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAvatarScroll->updateLayout();
}

//BD - Beq's Visual Posing
void BDFloaterPoser::selectJointByName(const std::string& jointName)
{
    S32 index = mJointTabs->getCurrentPanelIndex();

    std::vector<LLScrollListItem*> items = mJointScrolls[index]->getAllData();
    for (auto item : items)
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            if (joint->getName() == jointName)
            {
                mJointScrolls[index]->selectNthItem(mJointScrolls[index]->getItemIndex(item));
                mJointScrolls[index]->scrollToShowSelected();
                return; // Exit the loop once we've found and selected the joint
            }
        }
    }
    LL_WARNS() << "Joint not found: " << jointName << LL_ENDL;
}

LLVector3 BDFloaterPoser::getJointRot(const std::string& jointName)
{
    S32 index = mJointTabs->getCurrentPanelIndex();
    LLVector3 vec3;
    //BD - Rotations are only supported by joints so far, everything
    //     else snaps back instantly.
    LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();
    if (!item)
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (!joint)
        {
            vec3[VX] = (F32)item->getColumn(COL_ROT_X)->getValue().asReal();
            vec3[VY] = (F32)item->getColumn(COL_ROT_Y)->getValue().asReal();
            vec3[VZ] = (F32)item->getColumn(COL_ROT_Z)->getValue().asReal();
        }
    }
    return vec3;
}

void BDFloaterPoser::toggleWidgets()
{
    bool visual_posing = gSavedSettings.getBOOL("ShowVisualPosingTools");
    if (visual_posing)
    {
        LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
        if (!av_item) return;

        LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
        if (!avatar || avatar->isDead()) return;

        bool is_posing = (avatar->isFullyLoaded() && avatar->getPosing());
        bool is_visual_posing = (is_posing && gSavedSettings.getBOOL("ShowVisualPosingTools"));
        S32 index = mJointTabs->getCurrentPanelIndex();
        LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

        if (item)
        {
            LLJoint* joint = (LLJoint*)item->getUserdata();
            if (joint)
            {
                //BD - Beq's Visual Posing
                if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
                {
                    mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
                }
                LLToolMgr::getInstance()->setCurrentToolset(gPoserToolset);
                LLToolMgr::getInstance()->getCurrentToolset()->selectTool(FSToolCompPose::getInstance());
                FSToolCompPose::getInstance()->setAvatar(avatar);
                FSToolCompPose::getInstance()->setJoint(joint);
            }
        }
    }
    else
    {
        if (mLastToolset)
        {
            LLToolMgr::getInstance()->setCurrentToolset(mLastToolset);
        }
        FSToolCompPose::getInstance()->setAvatar(nullptr);
    }
}

void BDFloaterPoser::onModifierTabSwitch()
{
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (!av_item) return;

    LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
    if (!avatar || avatar->isDead()) return;

    bool is_pelvis = false;
    bool is_posing = (avatar->isFullyLoaded() && avatar->getPosing());
    bool is_visual_posing = (is_posing && gSavedSettings.getBOOL("ShowVisualPosingTools"));
    S32 index = mJointTabs->getCurrentPanelIndex();
    S32 curr_idx = mModifierTabs->getCurrentPanelIndex();
    LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

    if (item)
    {
        LLJoint* joint = (LLJoint*)item->getUserdata();
        if (joint)
        {
            if (is_visual_posing)
            {
                //BD - Beq's Visual Posing
                if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset
                    && LLToolMgr::getInstance()->getCurrentToolset() != gPoserToolset)
                {
                    mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
                    LLToolMgr::getInstance()->setCurrentToolset(gPoserToolset);
                }

                if (curr_idx == 0)
                {
                    LLToolMgr::getInstance()->getCurrentToolset()->selectTool(FSToolCompPose::getInstance());
                    FSToolCompPose::getInstance()->setAvatar(avatar);
                    FSToolCompPose::getInstance()->setJoint(joint);
                }
                if (curr_idx == 1)
                {
                    LLToolMgr::getInstance()->getCurrentToolset()->selectTool(BDToolCompPoseTranslate::getInstance());
                    BDToolCompPoseTranslate::getInstance()->setAvatar(avatar);
                    BDToolCompPoseTranslate::getInstance()->setJoint(joint);
                }
            }
        }
    }
}

void BDFloaterPoser::onRequestPermission()
{
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (!av_item) return;

    LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
    if (!avatar || avatar->isDead()) return;

    LLAvatarActions::posingRequest(avatar->getID());
}

void BDFloaterPoser::onRequestSyncing()
{
    LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
    if (!av_item) return;

    LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
    if (!avatar || avatar->isDead()) return;

    //BD - If we are already syncing to that person, issue a full resync and put
    //     the button on cooldown, otherwise request sync permission.
    if (avatar->getIsInSync())
    {
        avatar->setNeedsFullSync(true);
        getChild<LLUICtrl>("sync")->setEnabled(false);
        mSyncTimer.reset();
    }
    else
    {
        LLAvatarActions::poseSyncingRequest(avatar->getID());
    }
}

void BDFloaterPoser::onSyncPose()
{
    //BD - Sync our entire pose to everyone.
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        if (LLVOAvatar* avatar = (LLVOAvatar*)character)
        {
            if (avatar && avatar->getIsInSync())
            {
                avatar->setNeedsFullSync(true);
                mSyncTimer.reset();
            }
        }
    }
}

void BDFloaterPoser::onSyncPose(const LLUUID& id)
{
    S32 max_it = llceil((F32)gAgentAvatarp->getSkeletonJointCount() / 15.f);
    for (S32 i = 0; i < max_it; i++)
    {
        std::string message = ";PoserSync";
        for (S32 it = 0; it < 15; it++)
        {
            //BD - Getting collision volumes and attachment points.
            S32 bone_idx = (i * 15) + it;
            LLJoint* joint;
            joint = gAgentAvatarp->getCharacterJoint(bone_idx);
            {
                //BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
                if (!joint)	continue;

                LLVector3 rot;
                LLVector3 pos;
                LLVector3 scale;

                message.append("\n");
                //BD - Add bone index
                message.append((LLSD)bone_idx);
                message.append(";");

                //BD - Add rotation
                joint->getTargetRotation().getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
                message.append((LLSD)ll_round(rot.mV[VX], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(rot.mV[VY], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(rot.mV[VZ], 0.001f));
                message.append(";");

                //BD - Add position
                pos = joint->getTargetPosition();
                message.append(pos.getValue());
                message.append((LLSD)ll_round(pos.mV[VX], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(pos.mV[VY], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(pos.mV[VZ], 0.001f));
                message.append(";");

                //BD - Add position
                scale = joint->getScale();
                message.append((LLSD)ll_round(scale.mV[VX], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(scale.mV[VY], 0.001f));
                message.append(",");
                message.append((LLSD)ll_round(scale.mV[VZ], 0.001f));
                message.append(scale.getValue());
                message.append(";");
            }
        }
        sendSyncPackage(message, id);
    }
}

void BDFloaterPoser::onSyncBones()
{
    for (const auto& item : mSyncLists)
    {
        LL_INFOS("Posing") << "Syncing " << item[0] << " our bone: " << item[1] << LL_ENDL;
    }

    std::map<LLUUID, S32>::iterator itor = mSyncList.begin();
    while (itor != mSyncList.end())
    {
        LLUUID id = (*itor).first;
        S32 bone_to_sync = (*itor).second;

        LLJoint* joint;
        joint = gAgentAvatarp->getCharacterJoint(bone_to_sync);
        {
            //BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
            if (!joint)	continue;

            LLVector3 rot;
            LLVector3 pos;
            LLVector3 scale;

            std::string message = ";PoserSync";
            message.append("\n");
            //BD - Add bone index
            message.append((LLSD)bone_to_sync);
            message.append(";");

            //BD - Add rotation
            joint->getTargetRotation().getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
            message.append((LLSD)ll_round(rot.mV[VX], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(rot.mV[VY], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(rot.mV[VZ], 0.001f));
            message.append(";");

            //BD - Add position
            pos = joint->getTargetPosition();
            message.append(pos.getValue());
            message.append((LLSD)ll_round(pos.mV[VX], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(pos.mV[VY], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(pos.mV[VZ], 0.001f));
            message.append(";");

            //BD - Add position
            scale = joint->getScale();
            message.append((LLSD)ll_round(scale.mV[VX], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(scale.mV[VY], 0.001f));
            message.append(",");
            message.append((LLSD)ll_round(scale.mV[VZ], 0.001f));
            message.append(scale.getValue());
            message.append(";");

            sendSyncPackage(message, id);
        }

        mSyncList.erase(itor++);
    }
}

void BDFloaterPoser::sendSyncPackage(std::string message, const LLUUID& id)
{
    if (id.isNull()) return;

    LLMessageSystem* msg = gMessageSystem;

    msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_MessageBlock);
    msg->addBOOLFast(_PREHASH_FromGroup, false);
    msg->addUUIDFast(_PREHASH_ToAgentID, id);
    msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
    msg->addU8Fast(_PREHASH_Dialog, IM_NOTHING_SPECIAL);
    msg->addUUIDFast(_PREHASH_ID, id);
    msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

    std::string name;
    LLAgentUI::buildFullname(name);

    msg->addStringFast(_PREHASH_FromAgentName, name);
    msg->addStringFast(_PREHASH_Message, message);
    msg->addU32Fast(_PREHASH_ParentEstateID, 0);
    msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
    msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());

    gMessageSystem->addBinaryDataFast(
        _PREHASH_BinaryBucket,
        EMPTY_BINARY_BUCKET,
        EMPTY_BINARY_BUCKET_SIZE);

    gAgent.sendReliableMessage();
}

// static
void BDFloaterPoser::unpackSyncPackage(std::string message, const LLUUID& id)
{
    //BD - Get the right avatar before we do anything.
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        if (LLVOAvatar* avatar = (LLVOAvatar*)character)
        {
            if (avatar->getID() == id)
            {
                //BD - Make sure the person is being posed.
                if (!avatar->getPosing())
                {
                    avatar->setPosing();
                    avatar->startDefaultMotions();
                    avatar->startMotion(ANIM_BD_POSING_MOTION);
                }

                std::map<S32, S32> bones;
                std::map<S32, LLVector3> rot;
                std::map<S32, LLVector3> pos;
                std::map<S32, LLVector3> scale;

                //BD - First we cull the header.
                size_t sub_idx = message.find_first_of("\n");
                message.erase(0, sub_idx+1);

                S32 bone_idx = 0;
                //BD - Iterate through every line, culling the line out of the remaining
                //     text until nothing is left.
                while (!message.empty())
                {
                    size_t token_length = message.find_first_of(";");
                    //BD - Get bone number
                    std::string token = message.substr(0, token_length);
                    bones[bone_idx] = stoi(token);
                    message.erase(0, token_length + 1);
                    size_t vector_length = message.find_first_of(";");
                    std::string vector_token = message.substr(0, vector_length);
                    message.erase(0, vector_length + 1);
                    LL_INFOS("Posing") << message << LL_ENDL;
                    LLVector3 vec3;
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[0] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[1] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    vec3.mV[2] = stof(vector_token);
                    rot[bone_idx] = vec3;

                    vector_length = message.find_first_of(";");
                    vector_token = message.substr(0, vector_length);
                    message.erase(0, vector_length + 1);
                    LL_INFOS("Posing") << message << LL_ENDL;
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[0] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[1] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    vec3.mV[2] = stof(vector_token);
                    pos[bone_idx] = vec3;

                    vector_length = message.find_first_of(";");
                    vector_token = message.substr(0, vector_length);
                    message.erase(0, vector_length + 1);
                    LL_INFOS("Posing") << message << LL_ENDL;
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[0] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    token_length = vector_token.find_first_of(",");
                    vec3.mV[1] = stof(vector_token.substr(0, token_length));
                    vector_token.erase(0, token_length + 1);
                    vec3.mV[2] = stof(vector_token);
                    scale[bone_idx] = vec3;

                    bone_idx++;
                }

                for (S32 i = 0; i < bone_idx; i++)
                {
                    S32 bone_to_sync = bones[i];
                    LLVector3 vec_rot = rot[i];
                    LLVector3 vec_pos = pos[i];
                    LLVector3 vec_scale = scale[i];

                    LLQuaternion rot_quat;
                    rot_quat.setEulerAngles(vec_rot.mV[VX], vec_rot.mV[VY], vec_rot.mV[VZ]);

                    LLJoint* joint = avatar->getCharacterJoint(bone_to_sync);
                    if (joint)
                    {
                        joint->setTargetRotation(rot_quat);
                        joint->setTargetPosition(vec_pos);
                        joint->setScale(vec_scale);
                    }
                }
            }
        }
    }
}

//Poser Sync Breakdown:
// (Header)
//;PoserSync (10 bytes)
// (Body)
// # = Bone Number (1 - 3 bytes, assuming 3 bytes at all times)
// X.XXX = X vector (5 bytes)
// Y.YYY = Y vector (5 bytes)
// Z.ZZZ = Z vector (5 bytes)
// \n = New line (1 byte)
// (Examples)
// (Singular Targeted Sync) (19 bytes)
//<X.XXX,Y.YYY,Z.ZZZ>
// (Singular Full Sync) (64 bytes)
//#,<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>\n
// (Pose Sync) (15 bones per message) (960 bytes) + (10 bytes)
//;PoserSync (10 bytes)
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
//#;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>;<X.XXX,Y.YYY,Z.ZZZ>
// A full pose sync would require 9 messages for the main bones
// and another few for attachment bones and collision volumes.
