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
#include "lldiriterator.h"
#include "llfloaterpreference.h"
#include "llkeyframemotion.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerjointattachment.h"
#include "llviewerjoint.h"
#include "llvoavatarself.h"

#include "bdposingmotion.h"


BDFloaterPoser::BDFloaterPoser(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Save our current pose into a XML file to import it later or use it for creating an animation.
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterPoser::onClickPoseSave, this));
	//BD - Start our custom pose.
	mCommitCallbackRegistrar.add("Pose.Start", boost::bind(&BDFloaterPoser::onPoseStart, this));
	//BD - Load the current pose and export all its values into the UI so we can alter them.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterPoser::onPoseLoad, this, ""));
	//BD - Delete the currently selected Pose.
	mCommitCallbackRegistrar.add("Pose.Delete", boost::bind(&BDFloaterPoser::onPoseDelete, this));
	//BD - Change a pose's blend type and time.
	mCommitCallbackRegistrar.add("Pose.Set", boost::bind(&BDFloaterPoser::onPoseSet, this, _1, _2));
	//BD - Extend or collapse the floater's pose list.
	mCommitCallbackRegistrar.add("Pose.Layout", boost::bind(&BDFloaterPoser::onUpdateLayout, this));

	//BD - Change a bone's rotation.
	mCommitCallbackRegistrar.add("Joint.Set", boost::bind(&BDFloaterPoser::onJointSet, this, _1, _2));
	//BD - Change a bone's position.
	mCommitCallbackRegistrar.add("Joint.PosSet", boost::bind(&BDFloaterPoser::onJointPosSet, this, _1, _2));
	//BD - Change a bone's scale.
	//mCommitCallbackRegistrar.add("Joint.SetScale", boost::bind(&BDFloaterPoser::onJointScaleSet, this, _1, _2));
	//BD - Add or remove a joint state to or from the pose (enable/disable our overrides).
	mCommitCallbackRegistrar.add("Joint.ChangeState", boost::bind(&BDFloaterPoser::onJointChangeState, this));
	//BD - Reset all selected bone rotations and positions.
	mCommitCallbackRegistrar.add("Joint.ResetJointFull", boost::bind(&BDFloaterPoser::onJointRotPosReset, this));
	//BD - Reset all selected bone rotations back to 0,0,0.
	mCommitCallbackRegistrar.add("Joint.ResetJointRotation", boost::bind(&BDFloaterPoser::onJointRotationReset, this));
	//BD - Reset all selected bones positions back to their default.
	mCommitCallbackRegistrar.add("Joint.ResetJointPosition", boost::bind(&BDFloaterPoser::onJointPositionReset, this));

	//BD - Add a new entry to the animation creator.
	mCommitCallbackRegistrar.add("Anim.Add", boost::bind(&BDFloaterPoser::onAnimAdd, this, _2));
	//BD - Move the selected entry one row up.
	mCommitCallbackRegistrar.add("Anim.Move", boost::bind(&BDFloaterPoser::onAnimMove, this, _2));
	//BD - Remove an entry in the animation creator.
	mCommitCallbackRegistrar.add("Anim.Delete", boost::bind(&BDFloaterPoser::onAnimDelete, this));
	//BD - Save the currently build list as animation.
	mCommitCallbackRegistrar.add("Anim.Save", boost::bind(&BDFloaterPoser::onAnimSave, this));
	//BD - Play the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Play", boost::bind(&BDFloaterPoser::onAnimPlay, this));
	//BD - Stop the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Stop", boost::bind(&BDFloaterPoser::onAnimStop, this));
	//BD - Change the value for a wait entry.
	mCommitCallbackRegistrar.add("Anim.Set", boost::bind(&BDFloaterPoser::onAnimSet, this));
}

BDFloaterPoser::~BDFloaterPoser()
{
}

BOOL BDFloaterPoser::postBuild()
{
	//BD - Posing
	mJointsScroll = this->getChild<LLScrollListCtrl>("joints_scroll", true);
	mJointsScroll->setCommitOnSelectionChange(TRUE);
	mJointsScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));
	mJointsScroll->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onJointChangeState, this));

	mPoseScroll = this->getChild<LLScrollListCtrl>("poses_scroll", true);
	mPoseScroll->setCommitOnSelectionChange(TRUE);
	mPoseScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onPoseControlsRefresh, this));
	mPoseScroll->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onPoseLoad, this, ""));

	mRotationSliders = { { getChild<LLUICtrl>("Rotation_X"), getChild<LLUICtrl>("Rotation_Y"), getChild<LLUICtrl>("Rotation_Z") } };
	mPositionSliders = { { getChild<LLSliderCtrl>("Position_X"), getChild<LLSliderCtrl>("Position_Y"), getChild<LLSliderCtrl>("Position_Z") } };

	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimEditorScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAnimControlsRefresh, this));
	mAnimScrollIndex = 0;

	return TRUE;
}

void BDFloaterPoser::draw()
{
	//BD - This only works while the window is visible, hiding the UI will stop it.
	//     This part here is used to make the animator work, whenever we hit "Start" we
	//     start the mAnimPlayTimer with a expiry time of 0.0 to fire off the first action
	//     immediately after hitting "Start", this ensures we don't delay any actions.
	if (mAnimPlayTimer.getStarted() &&
		mAnimPlayTimer.getElapsedTimeF32() > mExpiryTime)
	{
		//BD - Stop the timer, we're going to reconfigure and restart it when we're done.
		mAnimPlayTimer.stop();
		if (mAnimEditorScroll->getItemCount() != 0)
		{
			mAnimEditorScroll->selectNthItem(mAnimScrollIndex);
			LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
			if (item)
			{
				//BD - We can't use Wait or Restart as label, need to fix this.
				//     TODO: Allow these to be translated and read the "type" from hidden values.
				std::string label = item->getColumn(0)->getValue().asString();
				if (label == "Wait")
				{
					//BD - Do nothing?
					mExpiryTime = item->getColumn(1)->getValue().asReal();
					++mAnimScrollIndex;
				}
				else if (label == "Repeat")
				{
					mAnimScrollIndex = 0;
					mExpiryTime = 0.0f;
				}
				else
				{
					mExpiryTime = 0.0f;
					onPoseLoad(label);
					++mAnimScrollIndex;
				}
			}
		}

		//BD - As long as we are not at the end, start the timer again, automatically
		//     resetting the counter in the process.
		if (mAnimEditorScroll->getItemCount() != mAnimScrollIndex)
		{
			mAnimPlayTimer.start();
		}
	}

	LLFloater::draw();
}

void BDFloaterPoser::onOpen(const LLSD& key)
{
	onJointRefresh();
	onPoseRefresh();
	onUpdateLayout();
}

void BDFloaterPoser::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mJointsScroll->clearRows();
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
		std::string name = LLURI::unescape(gDirUtilp->getBaseFileName(path, true));

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

		if (!infile.eof())
		{
			row["columns"][1]["column"] = "time";
			row["columns"][1]["value"] = data["time"];
			row["columns"][2]["column"] = "type";
			row["columns"][2]["value"] = data["type"];
		}
		mPoseScroll->addElement(row);
	}
	onJointControlsRefresh();
}

void BDFloaterPoser::onClickPoseSave()
{
	//BD - Values don't matter when not editing.
	onPoseSave(2, 0.1f, false);
}

BOOL BDFloaterPoser::onPoseSave(S32 type, F32 time, bool editing)
{
	//BD - First and foremost before we do anything, check if the folder exists.
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");
	if (!gDirUtilp->fileExists(pathname))
	{
		LL_WARNS("Posing") << "Couldn't find folder: " << pathname << " - creating one." << LL_ENDL;
		LLFile::mkdir(pathname);
	}

	std::string filename;
	if (editing)
	{
		LLScrollListItem* item = mPoseScroll->getFirstSelected();
		if (item)
		{
			filename = item->getColumn(0)->getValue().asString();
		}
	}
	else
	{
		filename = getChild<LLUICtrl>("pose_name")->getValue().asString();
	}

	if (filename.empty())
	{
		return FALSE;
	}

	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_POSES, filename + ".xml");
	LLSD record;
	S32 line = 0;

	if (editing)
	{
		llifstream infile;

		infile.open(full_path);
		if (!infile.is_open())
		{
			LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
			return FALSE;
		}

		LLSD old_record;
		//BD - Read the pose and save it into an LLSD so we can rewrite it later.
		while (!infile.eof())
		{
			if (LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(old_record, infile))
			{
				LL_WARNS("Posing") << "Failed to parse while rewrtiting file: " << filename << LL_ENDL;
				return FALSE;
			}

			if (line != 0)
			{
				record[line] = old_record;
			}
			++line;
		}

		//BD - Change the header here.
		record[0]["type"] = type;
		//BD - If we are using spherical linear interpolation we need to clamp the values 
		//     between 0.f and 1.f otherwise unexpected things might happen.
		if (type == 2)
		{
			time = llclamp(time, 0.0f, 1.0f);
		}
		record[0]["time"] = time;

		infile.close();
	}
	else
	{
		//BD - Create the header first.
		S32 type = getChild<LLUICtrl>("interpolation_type")->getValue();
		F32 time = getChild<LLUICtrl>("interpolation_time")->getValue().asReal();
		//BD - If we are using spherical linear interpolation we need to clamp the values 
		//     between 0.f and 1.f otherwise unexpected things might happen.
		if (type == 2)
		{
			time = llclamp(time, 0.0f, 1.0f);
		}
		record[line]["time"] = time;
		record[line]["type"] = type;
		line++;

		//BD - Now create the rest.
		mJointsScroll->selectAll();
		for (LLScrollListItem* element : mJointsScroll->getAllSelected())
		{
			LLVector3 vec3;
			LLJoint* joint = (LLJoint*)element->getUserdata();
			if (joint)
			{
				record[line]["bone"] = joint->getName();
				joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
				record[line]["rotation"] = vec3.getValue();

				//BD - We could just check whether position information is available since only joints
				//     which can have their position changed will have position information but we
				//     want this to be a minefield for crashes.
				//     Bones that can support position
				//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
				if (joint->mHasPosition)
				{
					vec3.mV[VX] = element->getColumn(COL_POS_X)->getValue().asReal();
					vec3.mV[VY] = element->getColumn(COL_POS_Y)->getValue().asReal();
					vec3.mV[VZ] = element->getColumn(COL_POS_Z)->getValue().asReal();
					record[line]["position"] = vec3.getValue();
				}

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
							record[line]["enabled"] = true;
						}
						else
						{
							record[line]["enabled"] = false;
						}
					}
				}
				++line;
			}
		}

		mJointsScroll->deselectAllItems();
	}
	
	llofstream file;
	file.open(full_path.c_str());
	//BD - Now lets actually write the file, whether it is writing a new one
	//     or just rewriting the previous one with a new header.
	for (LLSD cur_line : llsd::inArray(record))
	{
		LLSDSerialize::toXML(cur_line, file);
	}
	file.close();
	onPoseRefresh();
	return TRUE;
}

BOOL BDFloaterPoser::onPoseLoad(const LLSD& name)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	std::string filename;
	if (!name.asString().empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, name.asString() + ".xml");
	}
	else if (item)
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, item->getColumn(0)->getValue().asString() + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return FALSE;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return FALSE;
		}

		//BD - Not sure how to read the exact line out of a XML file, so we're just going
		//     by the amount of tags here, since the header has only 3 it's a good indicator
		//     if it's the correct line we're in.
		BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
		if (count == 3)
		{
			if (motion)
			{
				F32 time = pose["time"].asReal();
				S32 type = pose["type"].asInteger();
				motion->setInterpolationType(type);
				motion->setInterpolationTime(time);
				motion->startInterpolationTimer();
			}
		}

		LLJoint* joint = gAgentAvatarp->getJoint(pose["bone"].asString());
		if (joint)
		{
			if (motion)
			{
				LLPose* mpose = motion->getPose();
				if (mpose)
				{
					//BD - Fail safe, assume that a bone is always enabled in case we
					//     load a pose that was created prior to including the enabled
					//     state or for whatever reason end up not having an enabled state
					//     written into the file.
					bool state_enabled = true;

					//BD - Check whether the joint state of the current joint has any enabled
					//     status saved into the pose file or not.
					if (pose["enabled"].isDefined())
					{
						state_enabled = pose["enabled"].asBoolean();
					}

					//BD - Add the joint state but only if it's not active yet.
					//     Same goes for removing it, don't remove it if it doesn't exist.
					LLPointer<LLJointState> joint_state = mpose->findJointState(joint);
					if (!joint_state && state_enabled)
					{
						motion->addJointToState(joint);
					}
					else if (joint_state && !state_enabled)
					{
						motion->removeJointState(joint_state);
					}
				}
			}

			LLVector3 vec3;
			LLQuaternion quat;
			LLQuaternion new_quat = joint->getRotation();

			joint->setLastRotation(joint->getRotation());
			vec3.setValue(pose["rotation"]);
			quat.setEulerAngles(vec3.mV[VX], vec3.mV[VZ], vec3.mV[VY]);
			joint->setTargetRotation(quat);

			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			if (joint->mHasPosition)
			{
				if (pose["position"].isDefined())
				{
					vec3.setValue(pose["position"]);
					joint->setLastPosition(joint->getPosition());
					joint->setTargetPosition(vec3);
				}
			}
		}
	}
	infile.close();
	onJointRefresh();
	return TRUE;
}

void BDFloaterPoser::onPoseStart()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (!motion || motion->isStopped())
	{
		gAgent.setPosing();
		gAgent.stopFidget();
		gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		//BD - Reset our skeleton to bring our bones back into proper position including avatar deforms.
		gAgentAvatarp->resetSkeleton(false);
	}
	//BD - Wipe the joint list.
	onJointRefresh();

	onPoseControlsRefresh();
}

void BDFloaterPoser::onPoseDelete()
{
	for (LLScrollListItem* item : mPoseScroll->getAllSelected())
	{
		if (item)
		{
			std::string filename = item->getColumn(0)->getValue().asString();
			std::string dirname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");

			if (gDirUtilp->deleteFilesInDir(dirname, LLURI::escape(filename) + ".xml") < 1)
			{
				LL_WARNS("Posing") << "Cannot remove file: " << filename << LL_ENDL;
			}
		}
	}
	onPoseRefresh();
}

//BD - We use this function to edit an already saved pose's interpolation time or type.
//     Both are combined into one function.
void BDFloaterPoser::onPoseSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		S32 type;
		F32 time;
		//BD - We either edit the interpolation time here or if we don't we automatically assume 
		//     we are trying to edit the interpolation type.
		if (param.asString() == "time")
		{
			//BD - Get the new interpolation time from the time widget and read the type from
			//     the list as we are not going to change it anyway.
			time = getChild<LLUICtrl>("interp_time")->getValue().asReal();
			type = item->getColumn(2)->getValue().asInteger();
		}
		else
		{
			//BD - Do the opposite here.
			time = item->getColumn(1)->getValue().asReal();
			type = getChild<LLUICtrl>("interp_type")->getValue().asInteger();
		}
		onPoseSave(type, time, true);
	}
}

void BDFloaterPoser::onPoseControlsRefresh()
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("interp_time")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("interp_type")->setValue(item->getColumn(2)->getValue());
	}
	getChild<LLUICtrl>("interp_time")->setEnabled(bool(item));
	getChild<LLUICtrl>("interp_type")->setEnabled(bool(item));
	getChild<LLUICtrl>("delete_poses")->setEnabled(bool(item));
	getChild<LLUICtrl>("add_entry")->setEnabled(bool(item));
	getChild<LLUICtrl>("load_poses")->setEnabled(bool(item));
}

////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterPoser::onJointRefresh()
{
	bool is_posing = gAgent.getPosing();
	mJointsScroll->clearRows();
	LLJoint* joint;
	for (S32 i = 0; (joint = gAgentAvatarp->getCharacterJoint(i)); ++i)
	{
		LLSD row;
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
			std::string name = joint->getName();
			row["columns"][COL_ICON]["column"] = "icon";
			row["columns"][COL_ICON]["type"] = "icon";
			row["columns"][COL_ICON]["value"] = getString("icon_category");
			row["columns"][COL_NAME]["column"] = "joint";
			row["columns"][COL_NAME]["value"] = getString("title_" + name);
			mJointsScroll->addElement(row);
		}

		LLVector3 vec3;
		//BD - When posing get the target values otherwise we end up getting the in-interpolation values.
		if (is_posing)
		{
			joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
		}
		else
		{
			joint->getRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
		}

		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = joint->getName();
		row["columns"][COL_ROT_X]["column"] = "x";
		row["columns"][COL_ROT_X]["value"] = ll_round(vec3.mV[VX], 0.001f);
		row["columns"][COL_ROT_Y]["column"] = "y";
		row["columns"][COL_ROT_Y]["value"] = ll_round(vec3.mV[VY], 0.001f);
		row["columns"][COL_ROT_Z]["column"] = "z";
		row["columns"][COL_ROT_Z]["value"] = ll_round(vec3.mV[VZ], 0.001f);

		//BD - We could just check whether position information is available since only joints
		//     which can have their position changed will have position information but we
		//     want this to be a minefield for crashes.
		//     Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (joint->mHasPosition)
		{
			if (is_posing)
			{
				vec3 = joint->getTargetPosition();
			}
			else
			{
				vec3 = joint->getPosition();
			}
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(vec3.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(vec3.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(vec3.mV[VZ], 0.001f);
		}

		LLScrollListItem* item = mJointsScroll->addElement(row);
		item->setUserdata(joint);

		//BD - We need to check if we are posing or not, simply set all bones to deactivated
		//     when we are not posed otherwise they will remain on "enabled" state. This behavior
		//     could be confusing to the user, this is due to how animations work.
		if (is_posing)
		{
			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
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
	onJointControlsRefresh();
}

void BDFloaterPoser::onJointControlsRefresh()
{
	bool can_position = false;
	bool is_pelvis = false;
	bool is_posing = gAgent.getPosing();
	LLScrollListItem* item = mJointsScroll->getFirstSelected();
	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			mRotationSliders[VX]->setValue(item->getColumn(COL_ROT_X)->getValue());
			mRotationSliders[VY]->setValue(item->getColumn(COL_ROT_Y)->getValue());
			mRotationSliders[VZ]->setValue(item->getColumn(COL_ROT_Z)->getValue());
			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			if (joint->mHasPosition)
			{
				can_position = true;
				is_pelvis = (joint->mJointNum == 0);

				mPositionSliders[VX]->setValue(item->getColumn(COL_POS_X)->getValue());
				mPositionSliders[VY]->setValue(item->getColumn(COL_POS_Y)->getValue());
				mPositionSliders[VZ]->setValue(item->getColumn(COL_POS_Z)->getValue());
			}
			else
			{
				//BD - If we didn't select a positionable bone kill all values. We might
				//     end up with numbers that are too big for the min/max values when
				//     changed below.
				mPositionSliders[VX]->setValue(0.000f);
				mPositionSliders[VY]->setValue(0.000f);
				mPositionSliders[VZ]->setValue(0.000f);
			}

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

	getChild<LLButton>("toggle_bone")->setEnabled(item && is_posing);
	//getChild<LLButton>("reset_bone")->setEnabled(item && is_posing);
	getChild<LLButton>("reset_bone_rot")->setEnabled(item && is_posing);
	getChild<LLButton>("reset_bone_pos")->setEnabled(item && is_posing);
	getChild<LLButton>("activate")->setValue(is_posing);
	getChild<LLUICtrl>("pose_name")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_time")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_type")->setEnabled(is_posing);
	getChild<LLUICtrl>("save_poses")->setEnabled(is_posing);

	mRotationSliders[VX]->setEnabled(item && is_posing);
	mRotationSliders[VY]->setEnabled(item && is_posing);
	mRotationSliders[VZ]->setEnabled(item && is_posing);

	mPositionSliders[VX]->setEnabled(can_position);
	mPositionSliders[VY]->setEnabled(can_position);
	mPositionSliders[VZ]->setEnabled(can_position);

	F32 max_val = is_pelvis ? 5.f : 1.0f;
	mPositionSliders[VX]->setMaxValue(max_val);
	mPositionSliders[VY]->setMaxValue(max_val);
	mPositionSliders[VZ]->setMaxValue(max_val);
	mPositionSliders[VX]->setMinValue(-max_val);
	mPositionSliders[VY]->setMinValue(-max_val);
	mPositionSliders[VZ]->setMinValue(-max_val);
}

void BDFloaterPoser::onJointSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mJointsScroll->getFirstSelected();
	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			//BD - Neat yet quick and direct way of rotating our bones.
			//     No more need to include bone rotation orders.
			F32 val = ctrl->getValue().asReal();
			S32 axis = param.asInteger();
			LLScrollListCell* cell[3] = { item->getColumn(COL_ROT_X), item->getColumn(COL_ROT_Y), item->getColumn(COL_ROT_Z) };
			LLQuaternion rot_quat = joint->getTargetRotation();
			LLMatrix3 rot_mat;
			F32 old_value;
			LLVector3 vec3;

			old_value = cell[axis]->getValue().asReal();
			cell[axis]->setValue(ll_round(val, 0.001f));
			val -= old_value;
			vec3.mV[axis] = val;
			rot_mat = LLMatrix3(vec3.mV[VX], vec3.mV[VY], vec3.mV[VZ]);
			rot_quat = LLQuaternion(rot_mat)*rot_quat;
			joint->setTargetRotation(rot_quat);
		}
	}
}

void BDFloaterPoser::onJointPosSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mJointsScroll->getFirstSelected();
	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			if (joint->mHasPosition)
			{
				F32 val = ctrl->getValue().asReal();
				LLScrollListCell* cell[3] = { item->getColumn(COL_POS_X), item->getColumn(COL_POS_Y), item->getColumn(COL_POS_Z) };
				LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
					F32(cell[VY]->getValue().asReal()),
					F32(cell[VZ]->getValue().asReal()) };

				S32 dir = param.asInteger();
				vec3.mV[dir] = val;
				cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
				joint->setTargetPosition(vec3);
			}
		}
	}
}

void BDFloaterPoser::onJointChangeState()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		for (LLScrollListItem* item : mJointsScroll->getAllSelected())
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
void BDFloaterPoser::onJointRotPosReset()
{
	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	//BD - We use this bool to determine whether or not we'll be in need for a full skeleton
	//     reset and to prevent checking for it every single time.
	bool needs_reset = false;

	for (LLScrollListItem* item : mJointsScroll->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLQuaternion quat;
				LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_X);

				F32 round_val = ll_round(0, 0.001f);
				col_rot_x->setValue(round_val);
				col_rot_y->setValue(round_val);
				col_rot_z->setValue(round_val);

				quat.setEulerAngles(0, 0, 0);
				joint->setTargetRotation(quat);

				S32 i = joint->mJointNum;
				//BD - We could just check whether position information is available since only joints
				//     which can have their position changed will have position information but we
				//     want this to be a minefield for crashes.
				//     Bones that can support position
				//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
				if (joint->mHasPosition)
				{
					//BD - We only check once and do a full reset.
					if (!needs_reset && i != 0)
					{
						//BD - Kill all interpolations before we do this.
						motion->setInterpolationTime(1.0f);
						motion->setInterpolationType(2);

						needs_reset = true;
						//BD - To selectively reset our bones into their proper position (including attachment
						//     overrides we need to reset the skeleton completely and then reapply all our
						//     position overrides minus the ones we are resetting.
						gAgentAvatarp->resetSkeleton(false);

						//BD - Restart the poser, reset skeleton killed it.
						gAgent.setPosing();
						gAgent.stopFidget();
						gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);

						//BD - Kill all interpolations before we do this.
						motion->setInterpolationTime(0.25f);
						motion->setInterpolationType(2);
					}

					LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
					LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
					LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
					//BD - mPelvis is a special case.
					bool is_pelvis = (i == 0);

					//BD - The reason we don't use the default position for mPelvis is because it 
					//     will make the pelvis float under certain circumstances with certain meshes
					//     attached.
					LLVector3 pos = joint->getPosition();
					col_pos_x->setValue(is_pelvis ? round_val : pos.mV[VX]);
					col_pos_y->setValue(is_pelvis ? round_val : pos.mV[VY]);
					col_pos_z->setValue(is_pelvis ? round_val : pos.mV[VZ]);
					joint->setTargetPosition(is_pelvis ? LLVector3::zero : pos);
				}
			}
		}
	}

	//BD - Now that we've reset all position overrides we need to reapply all our overrides
	//     minus those we want to reset.
	if (needs_reset)
	{
		onJointPositionReset();
	}

	onJointControlsRefresh();
}

//BD - Used to reset rotations only.
void BDFloaterPoser::onJointRotationReset()
{
	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	for (LLScrollListItem* item : mJointsScroll->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLQuaternion quat;
				LLScrollListCell* col_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_z = item->getColumn(COL_ROT_Z);

				//F32 round_val = ll_round(0, 0.001f);
				col_x->setValue(0.000f);
				col_y->setValue(0.000f);
				col_z->setValue(0.000f);

				quat.setEulerAngles(0, 0, 0);
				joint->setTargetRotation(quat);
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
	//BD - When resetting positions, we don't use interpolation for now, it looks stupid.
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	bool has_reset = false;
	for (LLScrollListItem* item : mJointsScroll->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				S32 i = joint->mJointNum;
				//BD - We could just check whether position information is available since only joints
				//     which can have their position changed will have position information but we
				//     want this to be a minefield for crashes.
				//     Bones that can support position
				//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
				if (joint->mHasPosition)
				{
					//BD - We only check once and do a full reset.
					if (!has_reset && i != 0)
					{
						has_reset = true;
						//BD - To selectively reset our bones into their proper position (including attachment
						//     overrides we need to reset the skeleton and then reapply all our
						//     position overrides minus the ones we are resetting.
						gAgentAvatarp->clearAttachmentOverrides();
						gAgentAvatarp->rebuildAttachmentOverrides();
					}

					LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
					LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
					LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
					//BD - mPelvis is a special case.
					bool is_pelvis = (i == 0);

					//BD - The reason we don't use the default position for mPelvis is because it 
					//     will make the pelvis float under certain circumstances with certain meshes
					//     attached.
					LLVector3 pos = joint->getPosition();
					col_pos_x->setValue(is_pelvis ? 0.000f : pos.mV[VX]);
					col_pos_y->setValue(is_pelvis ? 0.000f : pos.mV[VY]);
					col_pos_z->setValue(is_pelvis ? 0.000f : pos.mV[VZ]);
					joint->setTargetPosition(is_pelvis ? LLVector3::zero : pos);
					//BD - Skip the animation.
					joint->setPosition(is_pelvis ? LLVector3::zero : pos);
				}
			}
		}
	}

	//BD - Now that we've reset all position overrides we need to reapply all our overrides
	//     minus those we want to reset.
	if (has_reset)
	{
		afterJointPositionReset();
	}

	onJointControlsRefresh();
}

void BDFloaterPoser::afterJointPositionReset()
{
	//BD - Iterate through all joint entries and reapply all our bone overrides again.
	for (LLScrollListItem* item : mJointsScroll->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLVector3 rot;
				rot.mV[VX] = item->getColumn(COL_ROT_X)->getValue().asReal();
				rot.mV[VY] = item->getColumn(COL_ROT_Y)->getValue().asReal();
				rot.mV[VZ] = item->getColumn(COL_ROT_Z)->getValue().asReal();

				LLQuaternion quat;
				joint->setLastRotation(joint->getRotation());
				quat.setEulerAngles(rot.mV[VX], rot.mV[VZ], rot.mV[VY]);
				joint->setTargetRotation(quat);

				//BD - We could just check whether position information is available since only joints
				//     which can have their position changed will have position information but we
				//     want this to be a minefield for crashes.
				//     Bones that can support position
				//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
				if (joint->mHasPosition)
				{
					LLVector3 pos;
					pos.mV[VX] = item->getColumn(COL_POS_X)->getValue().asReal();
					pos.mV[VY] = item->getColumn(COL_POS_Y)->getValue().asReal();
					pos.mV[VZ] = item->getColumn(COL_POS_Z)->getValue().asReal();

					//BD - This should reapply all still alive bone overrides we've not yet reset.
					joint->setTargetPosition(pos);
				}
			}
		}
	}
}

/*void BDFloaterPoser::onJointStateCheck()
{
	bool all_enabled = true;
	//BD - We check if all bones are enabled or not and pause or unpause depending on it.
	S32 i = 0;
	for (;; i++)
	{
		LLJoint* joint = gAgentAvatarp->getCharacterJoint(i);
		if (joint)
		{
			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (!joint_state)
					{
						//BD - One joint state was not enabled, break out and unpause all motions.
						all_enabled = false;
						gAgentAvatarp->getMotionController().unpauseAllMotions();
						break;
					}
				}
			}
		}
		else
		{
			break;
		}
	}

	if (all_enabled)
	{
		//BD - We ran through with no bone disabled. Pause all motions.
		gAgentAvatarp->getMotionController().pauseAllMotions();

		gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
	}
}*/


////////////////////////////////
//BD - Animations
////////////////////////////////
void BDFloaterPoser::onAnimAdd(const LLSD& param)
{
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	std::vector<LLScrollListItem*> items = mAnimEditorScroll->getAllData();
	LLScrollListItem* new_item;
	LLSD row;

	if (param.asString() == "Repeat")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Repeat";
	}
	else if (param.asString() == "Wait")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Wait";
		row["columns"][1]["column"] = "time";
		row["columns"][1]["value"] = 1.f;
	}
	else
	{
		LLScrollListItem* pose = mPoseScroll->getFirstSelected();
		if (pose)
		{
			//BD - Replace with list fill.
			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = pose->getColumn(0)->getValue().asString();
		}
		else
		{
			//BD - We should return here since when we dont add anything we don't need to do the rest.
			//     This MIGHT actually cause the issues users are seeing that i cannot reproduce.
			return;
		}
	}

	new_item = mAnimEditorScroll->addElement(row);

	if (!new_item)
	{
		LL_WARNS("Animator") << "Item we wrote is invalid, abort." << LL_ENDL;
		return;
	}

	++selected_index;
	items.insert(items.begin() + selected_index, new_item);
	onAnimListWrite(items);

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(selected_index);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimListWrite(std::vector<LLScrollListItem*> item_list)
{
	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (LLScrollListItem* item : item_list)
	{
		if (item)
		{
			item->setFlagged(true);

			LLSD row;
			LLScrollListCell* column = item->getColumn(0);
			if (column)
			{
				std::string value_str = column->getValue().asString();
				row["columns"][0]["column"] = "name";
				row["columns"][0]["value"] = value_str;
				if (value_str == "Wait")
				{
					row["columns"][1]["column"] = "time";
					row["columns"][1]["value"] = item->getColumn(1)->getValue().asString();
				}
				LLScrollListItem* new_item = mAnimEditorScroll->addElement(row);
				new_item->setFlagged(false);
			}
		}
	}

	//BD - Delete all flagged items now and we'll end up with a new list order.
	mAnimEditorScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();
}

void BDFloaterPoser::onAnimMove(const LLSD& param)
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 new_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 old_index = new_index;

	//BD - Don't allow moving down when we are already at the bottom, this would
	//     create another entry.
	//     Don't allow going up when we are on the first entry already either, this
	//     would select everything in the list.
	//     Don't allow moving if there's nothing to move. (Crashfix)
	if (((new_index + 1) >= item_count && param.asString() == "Down")
		|| (new_index == 0 && param.asString() == "Up")
		|| item_count == 0)
	{
		return;
	}

	LLScrollListItem* cur_item = mAnimEditorScroll->getFirstSelected();

	//BD - Don't allow moving if we don't have anything selected either. (Crashfix)
	if (!cur_item)
	{
		return;
	}

	//BD - Move up, otherwise move the entry down. No other option.
	if (param.asString() == "Up")
	{
		--new_index;
	}
	else
	{
		++new_index;
	}

	cur_item = mAnimEditorScroll->getFirstSelected();
	std::vector<LLScrollListItem*> items = mAnimEditorScroll->getAllData();

	items.erase(items.begin() + old_index);
	items.insert(items.begin() + new_index, cur_item);
	onAnimListWrite(items);

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(new_index);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimDelete()
{
	mAnimEditorScroll->deleteSelectedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();

	//BD - Update our controls when we delete items. Most of them should
	//     disable now that nothing is selected anymore.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimSave()
{
	//BD - Work in Progress exporter.
	//LLKeyframeMotion* motion = (LLKeyframeMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	//LLKeyframeMotion::JointMotionList* jointmotion_list;
	//jointmotion_list = LLKeyframeDataCache::getKeyframeData(ANIM_BD_POSING_MOTION);
	//typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;
	//LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->getJointMotion(0);
	//LLKeyframeMotion::RotationCurve rot_courve = joint_motion->mRotationCurve;
	//LLKeyframeMotion::RotationKey rot_key = rot_courve.mLoopInKey;
	//LLQuaternion rotation = rot_key.mRotation;
	//F32 time = rot_key.mTime;
}

void BDFloaterPoser::onAnimSet()
{
	F32 value = getChild<LLUICtrl>("anim_time")->getValue().asReal();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		LLScrollListCell* column = item->getColumn(1);
		if (item->getColumn(0)->getValue().asString() == "Wait")
		{
			column->setValue(value);
		}
	}
}

void BDFloaterPoser::onAnimPlay()
{
	//BD - We start the animator frametimer here and set the expiry time to 0.0
	//     to force the animator to start immediately when hitting the "Start" button.
	mExpiryTime = 0.0f;
	mAnimPlayTimer.start();
}

void BDFloaterPoser::onAnimStop()
{
	//BD - We only need to stop it here because the .start function resets the timer automatically.
	mAnimPlayTimer.stop();
}

void BDFloaterPoser::onAnimControlsRefresh()
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 index = mAnimEditorScroll->getFirstSelectedIndex();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item && item->getColumn(0)->getValue().asString() == "Wait")
	{
		getChild<LLUICtrl>("anim_time")->setEnabled(true);
	}
	else
	{
		getChild<LLUICtrl>("anim_time")->setEnabled(false);
	}
	getChild<LLUICtrl>("delete_entry")->setEnabled(bool(item));
	getChild<LLUICtrl>("move_up")->setEnabled(item && index != 0);
	getChild<LLUICtrl>("move_down")->setEnabled(item && !((index + 1) >= item_count));
}


////////////////////////////////
//BD - Misc Functions
////////////////////////////////

void BDFloaterPoser::onUpdateLayout()
{
	if (!this->isMinimized())
	{
		bool animator_expanded = getChild<LLButton>("animator")->getValue();
		bool poses_expanded = getChild<LLButton>("extend")->getValue() || animator_expanded;
		getChild<LLUICtrl>("poses_layout")->setVisible(poses_expanded);
		getChild<LLUICtrl>("animator_layout")->setVisible(animator_expanded);
		getChild<LLUICtrl>("poser_layout")->setVisible(!animator_expanded);

		S32 collapsed_width = getChild<LLPanel>("min_panel")->getRect().getWidth();
		S32 expanded_width = getChild<LLPanel>("max_panel")->getRect().getWidth();
		S32 floater_width = (poses_expanded && !animator_expanded) ? expanded_width : collapsed_width;
		this->reshape(floater_width, this->getRect().getHeight());
	}
}
