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
#include "lldiriterator.h"
#include "llkeyframemotion.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerjointattachment.h"
#include "llviewerjoint.h"
#include "llvoavatarself.h"

//BD - Animesh Support
#include "llcontrolavatar.h"

//BD - Black Dragon specifics
#include "bdanimator.h"
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

	//BD - Test.
	//mCommitCallbackRegistrar.add("Anim.Edit", boost::bind(&BDFloaterPoser::onAnimEdit, this, _1, _2));
	//mCommitCallbackRegistrar.add("Anim.SetValue", boost::bind(&BDFloaterPoser::onAnimSetValue, this, _1, _2));
}

BDFloaterPoser::~BDFloaterPoser()
{
}

BOOL BDFloaterPoser::postBuild()
{
	//BD - Posing
	mJointScrolls = { { this->getChild<LLScrollListCtrl>("joints_scroll", true),
						this->getChild<LLScrollListCtrl>("cv_scroll", true),
						this->getChild<LLScrollListCtrl>("attach_scroll", true) } };

	mJointScrolls[JOINTS]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[JOINTS]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));
	mJointScrolls[JOINTS]->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onJointChangeState, this));

	//BD - Collision Volumes
	mJointScrolls[COLLISION_VOLUMES]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[COLLISION_VOLUMES]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	//BD - Attachment Bones
	mJointScrolls[ATTACHMENT_BONES]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[ATTACHMENT_BONES]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	mPoseScroll = this->getChild<LLScrollListCtrl>("poses_scroll", true);
	mPoseScroll->setCommitOnSelectionChange(TRUE);
	mPoseScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onPoseControlsRefresh, this));
	mPoseScroll->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onPoseLoad, this, ""));

	mRotationSliders = { { getChild<LLUICtrl>("Rotation_X"), getChild<LLUICtrl>("Rotation_Y"), getChild<LLUICtrl>("Rotation_Z") } };
	mPositionSliders = { { getChild<LLSliderCtrl>("Position_X"), getChild<LLSliderCtrl>("Position_Y"), getChild<LLSliderCtrl>("Position_Z") } };
	mScaleSliders = { { getChild<LLSliderCtrl>("Scale_X"), getChild<LLSliderCtrl>("Scale_Y"), getChild<LLSliderCtrl>("Scale_Z") } };

	mJointTabs = getChild<LLTabContainer>("joints_tabs");
	mJointTabs->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	//BD - Animesh
	mAvatarScroll = this->getChild<LLScrollListCtrl>("avatar_scroll", true);
	mAvatarScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAvatarsSelect, this));

	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimEditorScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAnimControlsRefresh, this));

	//BD - Misc
	mDelayRefresh = false;

	//BD - Experimental
	/*mTimeSlider = getChild<LLMultiSliderCtrl>("time_slider");
	mKeySlider = getChild<LLMultiSliderCtrl>("key_slider");

	mTimeSlider->addSlider();
	addSliderKey(0.f, BDPoseKey(std::string("Default")));
	mTimeSlider->setCommitCallback(boost::bind(&BDFloaterPoser::onTimeSliderMoved, this));
	mKeySlider->setCommitCallback(boost::bind(&BDFloaterPoser::onKeyTimeMoved, this));
	getChild<LLButton>("add_key")->setClickedCallback(boost::bind(&BDFloaterPoser::onAddKey, this));
	getChild<LLButton>("delete_key")->setClickedCallback(boost::bind(&BDFloaterPoser::onDeleteKey, this));*/

	return TRUE;
}

void BDFloaterPoser::draw()
{
	if (gDragonAnimator.getIsPlaying())
	{
		LLScrollListItem* item = mAvatarScroll->getFirstSelected();
		if (item)
		{
			LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
			if (avatar || !avatar->isDead())
			{
				S32 current_index = avatar->getCurrentActionIndex();
				if (mAnimEditorScroll->getItemCount() != 0)
				{
					mAnimEditorScroll->selectNthItem(current_index);
				}
			}
		}
	}

	//BD - Retry until we are fully loaded, then grab our default values, this prevents
	//     us from using the Poser too early and grabbing in-loading values.
	if (mDelayRefresh)
	{
		if (gAgentAvatarp->isFullyLoaded())
		{
			onCollectDefaults();
			mDelayRefresh = false;
		}
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

	onAvatarsRefresh();
	onJointRefresh();
	onPoseRefresh();
	onUpdateLayout();
}

void BDFloaterPoser::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mJointScrolls[JOINTS]->clearRows();
	mJointScrolls[COLLISION_VOLUMES]->clearRows();
	mJointScrolls[ATTACHMENT_BONES]->clearRows();
	mAvatarScroll->clearRows();
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
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item)
	{
		LL_WARNS("Posing") << "No avatar selected." << LL_ENDL;
		return FALSE;
	}

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead())
	{
		LL_WARNS("Posing") << "Couldn't find avatar, dead?" << LL_ENDL;
		return FALSE;
	}

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

	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_POSES, escapeString(filename) + ".xml");
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
			if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(old_record, infile))
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

		LLScrollListItem* item = mPoseScroll->getFirstSelected();
		if (item)
		{
			item->getColumn(1)->setValue(time);
			item->getColumn(2)->setValue(type);
		}
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
		++line;

		//BD - Now create the rest.
		for (S32 it = 0; it < 3; ++it)
		{
			for (LLScrollListItem* element : mJointScrolls[it]->getAllData())
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
					//     as well as all attachment bones and collision volumes.
					if (joint->mHasPosition || it > JOINTS)
					{
						vec3.mV[VX] = element->getColumn(COL_POS_X)->getValue().asReal();
						vec3.mV[VY] = element->getColumn(COL_POS_Y)->getValue().asReal();
						vec3.mV[VZ] = element->getColumn(COL_POS_Z)->getValue().asReal();
						record[line]["position"] = vec3.getValue();
					}

					vec3.mV[VX] = element->getColumn(COL_SCALE_X)->getValue().asReal();
					vec3.mV[VY] = element->getColumn(COL_SCALE_Y)->getValue().asReal();
					vec3.mV[VZ] = element->getColumn(COL_SCALE_Z)->getValue().asReal();
					record[line]["scale"] = vec3.getValue();

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
									record[line]["enabled"] = true;
								}
								else
								{
									record[line]["enabled"] = false;
								}
							}
						}
					}
					++line;
				}
			}
		}
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

	if (!editing)
	{
		onPoseRefresh();
	}

	return TRUE;
}

BOOL BDFloaterPoser::onPoseLoad(const LLSD& name)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (!item) return FALSE;

	std::string pose_name;
	if (!name.asString().empty())
	{
		pose_name = name.asString();
	}
	else
	{
		pose_name = item->getColumn(0)->getValue().asString();
	}

	gDragonAnimator.loadPose(pose_name);
	onJointRefresh();
	return TRUE;
}

void BDFloaterPoser::onPoseStart()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (!motion || motion->isStopped())
	{
		avatar->setPosing();
		if (avatar->isSelf())
		{
			gAgent.stopFidget();
		}
		avatar->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		//BD - Reset everything, all rotations, positions and scales of all bones.
		onJointRotPosScaleReset();

		//BD - Clear posing when we're done now that we've safely endangered getting spaghetified.
		avatar->clearPosing();
		avatar->stopMotion(ANIM_BD_POSING_MOTION);
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
	bool is_playing = gDragonAnimator.getIsPlaying();
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("interp_time")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("interp_type")->setValue(item->getColumn(2)->getValue());
	}
	getChild<LLUICtrl>("interp_time")->setEnabled(bool(item));
	getChild<LLUICtrl>("interp_type")->setEnabled(bool(item));
	getChild<LLUICtrl>("delete_poses")->setEnabled(bool(item));
	getChild<LLUICtrl>("add_entry")->setEnabled(!is_playing && item);
	getChild<LLUICtrl>("load_poses")->setEnabled(bool(item));
}

////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterPoser::onJointRefresh()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

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
		const std::string name = joint->getName();
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
			row["columns"][COL_NAME]["value"] = getString("title_" + name);
			LLScrollListItem* element = mJointScrolls[JOINTS]->addElement(row);
			element->setEnabled(FALSE);
		}

		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = name;

		if (is_posing)
		{
			//BD - Bone Positions
			joint->getTargetRotation().getEulerAngles(&rot.mV[VX], &rot.mV[VZ], &rot.mV[VY]);
			row["columns"][COL_ROT_X]["column"] = "x";
			row["columns"][COL_ROT_X]["value"] = ll_round(rot.mV[VX], 0.001f);
			row["columns"][COL_ROT_Y]["column"] = "y";
			row["columns"][COL_ROT_Y]["value"] = ll_round(rot.mV[VY], 0.001f);
			row["columns"][COL_ROT_Z]["column"] = "z";
			row["columns"][COL_ROT_Z]["value"] = ll_round(rot.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		//BD - We could just check whether position information is available since only joints
		//     which can have their position changed will have position information but we
		//     want this to be a minefield for crashes.
		//     Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (joint->mHasPosition)
		{
			if (is_posing)
			{
				pos = joint->getTargetPosition();
				row["columns"][COL_POS_X]["column"] = "pos_x";
				row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
				row["columns"][COL_POS_Y]["column"] = "pos_y";
				row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
				row["columns"][COL_POS_Z]["column"] = "pos_z";
				row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);
			}
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
	for (std::vector<std::string>::iterator name_iter = cv_names.begin();
		name_iter != cv_names.end(); ++name_iter)
	{
		const std::string name = *name_iter;
		LLJoint* joint = avatar->getJoint(name);
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

		LLScrollListItem* item = mJointScrolls[COLLISION_VOLUMES]->addElement(row);
		item->setUserdata(joint);
	}

	//BD - Attachment Bones
	for (std::vector<std::string>::iterator name_iter = attach_names.begin();
		name_iter != attach_names.end(); ++name_iter)
	{
		const std::string name = *name_iter;
		LLJoint* joint = avatar->getJoint(name);
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

	bool can_position = false;
	bool is_pelvis = false;
	bool is_posing = (avatar->isFullyLoaded() && avatar->getPosing());
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			if (index == 0)
			{
				mRotationSliders[VX]->setValue(item->getColumn(COL_ROT_X)->getValue());
				mRotationSliders[VY]->setValue(item->getColumn(COL_ROT_Y)->getValue());
				mRotationSliders[VZ]->setValue(item->getColumn(COL_ROT_Z)->getValue());
			}

			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			//     as well as all attachment bones and collision volumes.
			if (joint->mHasPosition || index > JOINTS)
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
	getChild<LLButton>("reset_bone_rot")->setEnabled(item && is_posing && index == JOINTS);
	getChild<LLButton>("reset_bone_pos")->setEnabled(item && is_posing);
	getChild<LLButton>("reset_bone_scale")->setEnabled(item && is_posing);
	getChild<LLButton>("activate")->setValue(is_posing);
	getChild<LLUICtrl>("pose_name")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_time")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_type")->setEnabled(is_posing);
	getChild<LLUICtrl>("save_poses")->setEnabled(is_posing);

	mRotationSliders[VX]->setEnabled(item && is_posing && index == JOINTS);
	mRotationSliders[VY]->setEnabled(item && is_posing && index == JOINTS);
	mRotationSliders[VZ]->setEnabled(item && is_posing && index == JOINTS);

	mPositionSliders[VX]->setEnabled(is_posing && can_position);
	mPositionSliders[VY]->setEnabled(is_posing && can_position);
	mPositionSliders[VZ]->setEnabled(is_posing && can_position);

	mScaleSliders[VX]->setEnabled(item && is_posing);
	mScaleSliders[VY]->setEnabled(item && is_posing);
	mScaleSliders[VZ]->setEnabled(item && is_posing);

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
	//BD - Rotations are only supported by joints so far, everything
	//     else snaps back instantly.
	LLScrollListItem* item = mJointScrolls[JOINTS]->getFirstSelected();
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
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

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
			//     as well as all attachment bones and collision volumes.
			if (joint->mHasPosition || index > JOINTS)
			{
				F32 val = ctrl->getValue().asReal();
				LLScrollListCell* cell[3] = { item->getColumn(COL_POS_X), item->getColumn(COL_POS_Y), item->getColumn(COL_POS_Z) };
				LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
								   F32(cell[VY]->getValue().asReal()),
								   F32(cell[VZ]->getValue().asReal()) };

				S32 dir = param.asInteger();
				vec3.mV[dir] = val;
				cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
				(index > JOINTS) ? joint->setPosition(vec3) : joint->setTargetPosition(vec3);
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
			F32 val = ctrl->getValue().asReal();
			LLScrollListCell* cell[3] = { item->getColumn(COL_SCALE_X), item->getColumn(COL_SCALE_Y), item->getColumn(COL_SCALE_Z) };
			LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
							   F32(cell[VY]->getValue().asReal()),
							   F32(cell[VZ]->getValue().asReal()) };

			S32 dir = param.asInteger();
			vec3.mV[dir] = val;
			cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
			joint->setScale(vec3);
		}
	}
}

void BDFloaterPoser::onJointChangeState()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		for (LLScrollListItem* item : mJointScrolls[JOINTS]->getAllSelected())
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
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
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

	for (S32 it = 0; it < 3; ++it)
	{
		//BD - We use this bool to determine whether or not we'll be in need for a full skeleton
		//     reset and to prevent checking for it every single time.
		for (LLScrollListItem* item : mJointScrolls[it]->getAllData())
		{
			if (item)
			{
				LLJoint* joint = (LLJoint*)item->getUserdata();
				if (joint)
				{
					//BD - Resetting rotations first if there are any.
					if (it == JOINTS)
					{
						LLQuaternion quat;
						LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
						LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_X);
						LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_X);

						col_rot_x->setValue(0.000f);
						col_rot_y->setValue(0.000f);
						col_rot_z->setValue(0.000f);

						quat.setEulerAngles(0, 0, 0);
						joint->setTargetRotation(quat);
					}

					//BD - Resetting positions next.
					//BD - We could just check whether position information is available since only joints
					//     which can have their position changed will have position information but we
					//     want this to be a minefield for crashes.
					//     Bones that can support position
					//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
					//     as well as all attachment bones and collision volumes.
					if (joint->mHasPosition || it > JOINTS)
					{
						LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
						LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
						LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
						LLVector3 pos = mDefaultPositions[joint->getName()];

						col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
						col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
						col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
						if (it == JOINTS)
						{
							joint->setTargetPosition(pos);
						}
						//BD - Skip the animation.
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
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Used to reset rotations only.
void BDFloaterPoser::onJointRotationReset()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We do support resetting bone rotations for everyone however.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
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

	for (LLScrollListItem* item : mJointScrolls[JOINTS]->getAllSelected())
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
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones positions for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - When resetting positions, we don't use interpolation for now, it looks stupid.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	//BD - We use this bool to prevent going through attachment override reset every single time.
	//bool has_reset = false;
	for (LLScrollListItem* item : mJointScrolls[index]->getAllSelected())
	{
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
				//     as well as all attachment bones and collision volumes.
				if (joint->mHasPosition || index > JOINTS)
				{
					LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
					LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
					LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
					LLVector3 pos = mDefaultPositions[joint->getName()];

					col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
					col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
					col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
					//BD - Attachment bones and collision volumes don't have target positions.
					//     They don't animate and they are not even included in our posing animation.
					if (index == JOINTS)
					{
						joint->setTargetPosition(pos);
					}
					else
					{
						//BD - Skip the animation.
						joint->setPosition(pos);
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
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones scales for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - Clear all attachment bone scale changes we've done, they are not automatically
	//     reverted.
	for (LLScrollListItem* item : mJointScrolls[index]->getAllSelected())
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

				col_scale_x->setValue(ll_round(scale.mV[VX], 0.001f));
				col_scale_y->setValue(ll_round(scale.mV[VY], 0.001f));
				col_scale_z->setValue(ll_round(scale.mV[VZ], 0.001f));
				joint->setScale(scale);
			}
		}
	}
	onJointControlsRefresh();
}

//BD - This is used to collect all default values at the beginning to revert to later on.
void BDFloaterPoser::onCollectDefaults()
{
	LLVector3 rot;
	LLVector3 pos;
	LLVector3 scale;
	LLJoint* joint;

	//BD - Getting collision volumes and attachment points.
	std::vector<std::string> cv_names;
	std::vector<std::string> attach_names;
	gAgentAvatarp->getSortedJointNames(1, cv_names);
	gAgentAvatarp->getSortedJointNames(2, attach_names);

	for (S32 i = 0; (joint = gAgentAvatarp->getCharacterJoint(i)); ++i)
	{
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLSD row;
		const std::string name = joint->getName();

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		scale = joint->getScale();
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));

		//BD - We could just check whether position information is available since only joints
		//     which can have their position changed will have position information but we
		//     want this to be a minefield for crashes.
		//     Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (joint->mHasPosition)
		{
			//BD - We always get the values but we don't write them out as they are not relevant for the
			//     user yet but we need them to establish default values we revert to later on.
			pos = joint->getPosition();
			mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		}
	}

	//BD - Collision Volumes
	for (std::vector<std::string>::iterator name_iter = cv_names.begin();
		name_iter != cv_names.end(); ++name_iter)
	{
		const std::string name = *name_iter;
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		pos = joint->getPosition();
		scale = joint->getScale();
		mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));
	}

	//BD - Attachment Bones
	for (std::vector<std::string>::iterator name_iter = attach_names.begin();
		name_iter != attach_names.end(); ++name_iter)
	{
		const std::string name = *name_iter;
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		pos = joint->getPosition();
		scale = joint->getScale();

		mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));
	}
}

////////////////////////////////
//BD - Animations
////////////////////////////////
void BDFloaterPoser::onAnimAdd(const LLSD& param)
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying()) return;

	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	LLScrollListItem* pose = mPoseScroll->getFirstSelected();
	std::string name = param.asString();
	S32 location = mAnimEditorScroll->getFirstSelectedIndex() + 1;
	F32 time = 0.f;
	BD_EActionType type;
	if (param.asString() == "Repeat")
	{
		type = BD_EActionType::REPEAT;
	}
	else if (param.asString() == "Wait")
	{
		type = BD_EActionType::WAIT;
		time = 1.f;
	}
	else
	{
		name = pose->getColumn(0)->getValue().asString();
		type = BD_EActionType::POSE;
	}
	//BD - We'll be adding the avatar into a list in here to keep track of all
	//     avatars we've added actions for.
	gDragonAnimator.onAddAction(avatar, name, type, time, location);
	onAnimListWrite();

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(location);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimListWrite()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	mAnimEditorScroll->clearRows();
	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (Action action : avatar->mAnimatorActions)
	{
		LLSD row;
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = action.mPoseName;
		if (action.mType == BD_EActionType::WAIT)
		{
			row["columns"][1]["column"] = "time";
			row["columns"][1]["value"] = action.mTime;
			row["columns"][2]["column"] = "type";
			row["columns"][2]["value"] = action.mType;
		}
		mAnimEditorScroll->addElement(row);
	}

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();
}

void BDFloaterPoser::onAnimMove(const LLSD& param)
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying())	return;

	//BD - Don't allow moving if we don't have anything selected either. (Crashfix)
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

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

	//BD - Move up, otherwise move the entry down. No other option.
	if (param.asString() == "Up")
	{
		--new_index;
	}
	else
	{
		++new_index;
	}

	Action action = avatar->mAnimatorActions[old_index];
	gDragonAnimator.onDeleteAction(avatar, old_index);
	gDragonAnimator.onAddAction(avatar, action, new_index);
	onAnimListWrite();

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(new_index);

	//BD - Update our controls when we move items, the move and delete buttons
	//     should enable now that we might have selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimDelete()
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying())	return;

	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::vector<LLScrollListItem*> items = mAnimEditorScroll->getAllSelected();
	while (!items.empty())
	{
		//BD - We'll delete the avatar from our list if necessary when no more actions
		//     are saved in a given avatar.
		gDragonAnimator.onDeleteAction(avatar, mAnimEditorScroll->getFirstSelectedIndex());
		items.erase(items.begin());
	}
	onAnimListWrite();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();

	//BD - Update our controls when we delete items. Most of them should
	//     disable now that nothing is selected anymore.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimSave()
{
	LLKeyframeMotion* temp_motion = NULL;
	LLAssetID mMotionID;
	LLTransactionID	mTransactionID;

	//BD - To make this work we'll first need a unique UUID for this animation.
	mTransactionID.generate();
	mMotionID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
	temp_motion->create(mMotionID);

	//BD - Work in Progress exporter.
	//LLKeyframeMotion* motion = (LLKeyframeMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	//typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;

	//LLKeyframeMotion::JointMotionList* jointmotion_list;
	//jointmotion_list = LLKeyframeDataCache::getKeyframeData(ANIM_BD_POSING_MOTION);


	//LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->getJointMotion(0);
	//LLKeyframeMotion::RotationCurve rot_courve = joint_motion->mRotationCurve;
	//LLKeyframeMotion::RotationKey rot_key = rot_courve.mLoopInKey;
	//LLQuaternion rotation = rot_key.mRotation;
	//F32 time = rot_key.mTime;
	//LLDataPackerBinaryBuffer* dp;
	//BOOL success = TRUE;

	//BD - Lets collect everything we will need.
	std::string motion_name = getChild<LLUICtrl>("set_name")->getValue().asString();
	//U16 motion_major_version = KEYFRAME_MOTION_VERSION;
	//U16 motion_sub_version = KEYFRAME_MOTION_SUBVERSION;
	//S32 base_priority = getChild<LLUICtrl>("set_priority")->getValue().asInteger();
	LLJoint::JointPriority base_priority = LLJoint::HIGHEST_PRIORITY;
	F32 duration = getChild<LLUICtrl>("set_duration")->getValue().asReal();
	std::string face_emote_name = getChild<LLUICtrl>("set_face_emote")->getValue();
	F32 loop_in_time = getChild<LLUICtrl>("set_loop_in")->getValue().asReal();
	F32 loop_out_time = getChild<LLUICtrl>("set_loop_out")->getValue().asReal();
	S32 loop = getChild<LLUICtrl>("set_loop")->getValue().asBoolean();
	F32 ease_in_time = getChild<LLUICtrl>("set_ease_in")->getValue().asReal();
	F32 ease_out_time = getChild<LLUICtrl>("set_ease_out")->getValue().asReal();
	//U32 hand_pose = getChild<LLUICtrl>("set_hand_pose")->getValue().asInteger();
	//U32 joint_motion_count = getChild<LLUICtrl>("set_active_motions")->getValue().asInteger();
	LLUUID emote_name = getChild<LLUICtrl>("set_emote")->getValue();

	LLKeyframeMotion::JointMotionList* jointmotion_list = new LLKeyframeMotion::JointMotionList;

	//BD - Reserve for all 134 joints.
	jointmotion_list->mJointMotionArray.reserve(134);

	LLJoint* joint;
	for (S32 i = 0; (joint = gAgentAvatarp->getCharacterJoint(i)); ++i)
	{
		if (!joint)
			continue;

		//BD - Let's build joint motions
		LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->mJointMotionArray[i];

		//BD - We start with the joint name;
		joint_motion->mJointName = joint->getName();

		//BD - Add the priority
		joint_motion->mPriority = LLJoint::HIGHEST_PRIORITY;

		//BD - Define the bone usage
		S32 usage = 0;
		if (joint->mHasPosition)
		{
			usage = usage & LLJointState::POS;
		}
		usage = usage & LLJointState::ROT;
		usage = usage & LLJointState::POS;
		joint_motion->mUsage = usage;

		//BD - And finally the fun part.
		//LLKeyframeMotion::RotationCurve* rot_courve;
		//LLKeyframeMotion::PositionCurve* pos_courve;
		//LLKeyframeMotion::ScaleCurve* scale_courve;

		//LLKeyframeMotion::RotationKey* rot_key;
		//rot_key->mRotation = joint->getRotation();
		//rot_key->mTime = 0.0f;		
		//rot_courve->mKeys[0.0f].mRotation = joint->getRotation();
		//rot_courve->mKeys[0.0f].mTime = 0.0f;
		joint_motion->mRotationCurve.mKeys[0.0f].mRotation = joint->getRotation();
		joint_motion->mRotationCurve.mKeys[0.0f].mTime = 0.0f;

		joint_motion->mPositionCurve.mKeys[0.0f].mPosition = joint->getPosition();
		joint_motion->mPositionCurve.mKeys[0.0f].mTime = 0.0f;

		joint_motion->mScaleCurve.mKeys[0.0f].mScale = joint->getScale();
		joint_motion->mScaleCurve.mKeys[0.0f].mTime = 0.0f;

		//BD - Add the joint motion to our array.
		jointmotion_list->mJointMotionArray[i] = joint_motion;
	}
	LLKeyframeDataCache::addKeyframeData(mMotionID, jointmotion_list);

	temp_motion->setPriority(base_priority);
	temp_motion->setEaseIn(ease_in_time);
	temp_motion->setEaseOut(ease_out_time);
	temp_motion->setEmote(emote_name);
	//jointmotion_list->mHandPose = hand_pose;
	temp_motion->setLoop(loop);
	temp_motion->setLoopIn(loop_in_time);
	temp_motion->setLoopOut(loop_out_time);

	if (!jointmotion_list)
		return;

	jointmotion_list->mBasePriority = base_priority;
	jointmotion_list->mDuration = duration;
	jointmotion_list->mEaseInDuration = ease_in_time;
	jointmotion_list->mEaseOutDuration = ease_out_time;
	jointmotion_list->mEmoteName = face_emote_name;
	//jointmotion_list->mHandPose = hand_pose;
	jointmotion_list->mLoop = loop;
	jointmotion_list->mLoopInPoint = loop_in_time;
	jointmotion_list->mLoopOutPoint = loop_out_time;
	jointmotion_list->mMaxPriority = base_priority;

	temp_motion->dumpToFile(motion_name);
}

void BDFloaterPoser::onAnimSet()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	F32 value = getChild<LLUICtrl>("anim_time")->getValue().asReal();
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	Action action = avatar->mAnimatorActions[selected_index];
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		if (action.mType == BD_EActionType::WAIT)
		{
			avatar->mAnimatorActions[mAnimEditorScroll->getFirstSelectedIndex()].mTime = value;
			LLScrollListCell* column = item->getColumn(1);
			column->setValue(value);
		}
	}
}

void BDFloaterPoser::onAnimPlay()
{
	//BD - We start the animator frametimer here and set the expiry time to 0.0
	//     to force the animator to start immediately when hitting the "Start" button.
	gDragonAnimator.startPlayback();

	//BD - Update our controls when we start the animator. Most of them should
	//     disable now.
	onAnimControlsRefresh();
	onPoseControlsRefresh();
}

void BDFloaterPoser::onAnimStop()
{
	//BD - We only need to stop it here because the .start function resets the timer 
	//     automatically.
	gDragonAnimator.stopPlayback();

	//BD - Update our controls when we stop the animator. Most of them should
	//     enable again.
	onAnimControlsRefresh();
	onPoseControlsRefresh();
}

void BDFloaterPoser::onAnimControlsRefresh()
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 index = mAnimEditorScroll->getFirstSelectedIndex();

	bool is_playing = gDragonAnimator.getIsPlaying();
	bool selected = index != -1;
	bool is_wait = false;

	//BD - Don't.
	if (!is_playing)
	{
		LLScrollListItem* item = mAvatarScroll->getFirstSelected();
		if (item)
		{
			LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
			if (avatar || !avatar->isDead())
			{
				//BD - Crashfix.
				if (selected)
				{
					is_wait = avatar->mAnimatorActions[index].mType == BD_EActionType::WAIT;
				}
			}
		}
	}

	getChild<LLUICtrl>("anim_time")->setEnabled(!is_playing && selected && is_wait);
	getChild<LLUICtrl>("delete_entry")->setEnabled(!is_playing && selected);
	getChild<LLUICtrl>("move_up")->setEnabled(!is_playing && selected && index != 0);
	getChild<LLUICtrl>("move_down")->setEnabled(!is_playing && selected && !((index + 1) >= item_count));
	getChild<LLUICtrl>("add_repeat")->setEnabled(!is_playing);
	getChild<LLUICtrl>("add_wait")->setEnabled(!is_playing);
	getChild<LLUICtrl>("play_anim")->setEnabled(!is_playing && (item_count > 0));
	getChild<LLUICtrl>("stop_anim")->setEnabled(is_playing);
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

		if (animator_expanded)
		{
			//BD - Refresh our animation list
			onAnimListWrite();
		}
	}
}

std::string BDFloaterPoser::escapeString(const std::string& str)
{
	//BD - Don't use LLURI::escape() because it doesn't encode '-' characters
	//     which may break handling of some poses.
	char* curl_str = curl_escape(str.c_str(), str.size());
	std::string escaped_str(curl_str);
	curl_free(curl_str);

	return escaped_str;
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
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAvatarsRefresh()
{
	bool skip_creation = false;
	//BD - Flag all items first, we're going to unflag them when they are valid.
	for (LLScrollListItem* item : mAvatarScroll->getAllData())
	{
		if (item)
		{
			//BD - Automatically flag all animesh, they might have vanished.
			//     Our own avatar is never flagged, it does not need to be removed as it remains
			//     valid throughout an entire session once created.
			if (item->getColumn(3)->getValue().asBoolean())
			{
				item->setFlagged(TRUE);
			}
			else
			{
				skip_creation = true;
			}
		}
	}

	//BD - Add our own avatar first at all times, only if we haven't already.
	if (!skip_creation)
	{
		for (LLCharacter* character : LLCharacter::sInstances)
		{
			LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(character);
			if (avatar && avatar->isSelf())
			{
				LLSD own_row;
				LLAvatarName av_name;
				LLAvatarNameCache::get(gAgent.getID(), &av_name);
				own_row["columns"][0]["column"] = "icon";
				own_row["columns"][0]["type"] = "icon";
				own_row["columns"][0]["value"] = getString("icon_category");
				own_row["columns"][1]["column"] = "name";
				own_row["columns"][1]["value"] = av_name.getDisplayName();
				own_row["columns"][2]["column"] = "uuid";
				own_row["columns"][2]["value"] = gAgent.getID();
				own_row["columns"][3]["column"] = "control_avatar";
				own_row["columns"][3]["value"] = false;
				LLScrollListItem* item = mAvatarScroll->addElement(own_row);
				item->setUserdata(avatar);

				//BD - We're just here to find outselves, break out immediately when we are done.
				break;
			}
		}
	}

	bool create_new = true;
	//BD - Animesh Support
	//     Search through all control avatars.
	for (LLCharacter* character : LLControlAvatar::sInstances)
	{
		LLControlAvatar* avatar = dynamic_cast<LLControlAvatar*>(character);
		if (avatar)
		{
			LLUUID uuid = avatar->getID();
			for (LLScrollListItem* item : mAvatarScroll->getAllData())
			{
				if (item)
				{
					if (avatar == item->getUserdata())
					{
						//BD - Avatar is still valid unflag it from removal.
						item->setFlagged(FALSE);
						create_new = false;
						break;
					}
				}
			}

			if (create_new)
			{
				//BD - Avatar was not listed yet, create a new entry.
				LLSD row;
				row["columns"][0]["column"] = "icon";
				row["columns"][0]["type"] = "icon";
				row["columns"][0]["value"] = getString("icon_object");
				row["columns"][1]["column"] = "name";
				row["columns"][1]["value"] = avatar->getFullname();
				row["columns"][2]["column"] = "uuid";
				row["columns"][2]["value"] = avatar->getID();
				row["columns"][3]["column"] = "control_avatar";
				row["columns"][3]["value"] = false;
				LLScrollListItem* item = mAvatarScroll->addElement(row);
				item->setUserdata(avatar);
			}
		}
	}
}



////////////////////////////////
//BD - Experimental Functions
////////////////////////////////

/*void BDFloaterPoser::onAnimEdit(LLUICtrl* ctrl, const LLSD& param)
{
	getChild<LLMultiSliderCtrl>("key_slider")->clear();
	LLUUID id = LLUUID("cd9b0386-b26d-e860-0114-d879ee12a777");

	//LLKeyframeMotion* motion = (LLKeyframeMotion*)gAgentAvatarp->findMotion(id);
	typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;

	LLKeyframeMotion::JointMotionList* jointmotion_list;
	jointmotion_list = LLKeyframeDataCache::getKeyframeData(id);
	S32 i = 0;

	if (!jointmotion_list)
	{
		return;
	}

	LLScrollListItem* item = mJointScrolls[JOINTS]->getLastSelectedItem();
	if (!item)
	{
		return;
	}

	LLJoint* joint = (LLJoint*)item->getUserdata();
	i = joint->mJointNum;

	LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->getJointMotion(i);
	LLKeyframeMotion::RotationCurve rot_courve = joint_motion->mRotationCurve;
	LLKeyframeMotion::RotationKey rot_key;
	LLQuaternion rotation;
	F32 time;
	LLKeyframeMotion::RotationCurve::key_map_t keys = rot_courve.mKeys;

	for (LLKeyframeMotion::RotationCurve::key_map_t::const_iterator iter = keys.begin();
		iter != keys.end(); ++iter)
	{
		time = iter->first;
		rot_key = iter->second;

		//const std::string& sldr_name = getChild<LLMultiSliderCtrl>("key_slider")->addSlider(time);
		getChild<LLMultiSliderCtrl>("key_slider")->addSlider(time);
		//getChild<LLMultiSliderCtrl>("key_slider")->getSliderValue();
	}
}

void BDFloaterPoser::onAddKey()
{
	//S32 max_sliders = 60;

	//if ((S32)mSliderToKey.size() >= max_sliders)
	//{
	//	LLSD args;
	//	args["MAX"] = max_sliders;
	//	//LLNotificationsUtil::add("DayCycleTooManyKeyframes", args, LLSD(), LLNotificationFunctorRegistry::instance().DONOTHING);
	//	return;
	//}

	// add the slider key
	std::string key_val = mPoseScroll->getFirstSelected()->getColumn(0)->getValue().asString();
	BDPoseKey pose_params(key_val);

	F32 time = mTimeSlider->getCurSliderValue();
	addSliderKey(time, pose_params);
}

void BDFloaterPoser::addSliderKey(F32 time, BDPoseKey keyframe)
{
	// make a slider
	const std::string& sldr_name = mKeySlider->addSlider(time);
	if (sldr_name.empty())
	{
		return;
	}

	// set the key
	SliderKey newKey(keyframe, mKeySlider->getCurSliderValue());

	// add to map
	mSliderToKey.insert(std::pair<std::string, SliderKey>(sldr_name, newKey));
}

void BDFloaterPoser::onTimeSliderMoved()
{
}

void BDFloaterPoser::onKeyTimeMoved()
{
	if (mKeySlider->getValue().size() == 0)
	{
		return;
	}

	// make sure we have a slider
	const std::string& cur_sldr = mKeySlider->getCurSlider();
	if (cur_sldr == "")
	{
		return;
	}

	F32 time24 = mKeySlider->getCurSliderValue();

	// check to see if a key exists
	BDPoseKey key = mSliderToKey[cur_sldr].keyframe;
	//LL_DEBUGS() << "Setting key time: " << time24 << LL_ENDL;
	mSliderToKey[cur_sldr].time = time24;

	// if it exists, turn on check box
	//mSkyPresetsCombo->selectByValue(key.toStringVal());

	//mTimeCtrl->setTime24(time24);
}

void BDFloaterPoser::onKeyTimeChanged()
{
	// if no keys, skipped
	if (mSliderToKey.size() == 0)
	{
		return;
	}

	F32 time24 = getChild<LLTimeCtrl>("time_control")->getTime24();

	const std::string& cur_sldr = mKeySlider->getCurSlider();
	mKeySlider->setCurSliderValue(time24, TRUE);
	F32 time = mKeySlider->getCurSliderValue() / 60;

	// now set the key's time in the sliderToKey map
	//LL_DEBUGS() << "Setting key time: " << time << LL_ENDL;
	mSliderToKey[cur_sldr].time = time;

	//applyTrack();
}

void BDFloaterPoser::onDeleteKey()
{
	if (mSliderToKey.size() == 0)
	{
		return;
	}

	// delete from map
	const std::string& sldr_name = mKeySlider->getCurSlider();
	std::map<std::string, SliderKey>::iterator mIt = mSliderToKey.find(sldr_name);
	mSliderToKey.erase(mIt);

	mKeySlider->deleteCurSlider();

	if (mSliderToKey.size() == 0)
	{
		return;
	}

	const std::string& name = mKeySlider->getCurSlider();
	//mSkyPresetsCombo->selectByValue(mSliderToKey[name].keyframe.toStringVal());
	F32 time24 = mSliderToKey[name].time;

	getChild<LLTimeCtrl>("time_control")->setTime24(time24);
}

void BDFloaterPoser::onAnimSetValue(LLUICtrl* ctrl, const LLSD& param)
{
	F32 val = ctrl->getValue().asReal();
	mKeySlider->setValue(val);
}*/
