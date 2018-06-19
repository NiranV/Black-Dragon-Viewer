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
	//BD - Change a bone's position (specifically the mPelvis bone).
	mCommitCallbackRegistrar.add("Joint.PosSet", boost::bind(&BDFloaterPoser::onJointPosSet, this, _1, _2));
	//BD - Add or remove a joint state to or from the pose (enable/disable our overrides).
	mCommitCallbackRegistrar.add("Joint.ChangeState", boost::bind(&BDFloaterPoser::onJointChangeState, this));
	//BD - Reset all selected bones back to 0,0,0.
	mCommitCallbackRegistrar.add("Joint.ResetJoint", boost::bind(&BDFloaterPoser::onJointReset, this));

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

	mRotX = getChild<LLUICtrl>("Rotation_X");
	mRotY = getChild<LLUICtrl>("Rotation_Y");
	mRotZ = getChild<LLUICtrl>("Rotation_Z");

	mRotXBig = getChild<LLUICtrl>("Rotation_X_Big");
	mRotYBig = getChild<LLUICtrl>("Rotation_Y_Big");
	mRotZBig = getChild<LLUICtrl>("Rotation_Z_Big");

	mPosX = getChild<LLUICtrl>("Position_X");
	mPosY = getChild<LLUICtrl>("Position_Y");
	mPosZ = getChild<LLUICtrl>("Position_Z");

	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimEditorScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAnimControlsRefresh, this));
	mAnimScrollIndex = 0;

	return TRUE;
}

void BDFloaterPoser::draw()
{
	//BD - This only works while the window is visible, hiding the UI will stop it.
	if (mAnimPlayTimer.getStarted() &&
		mAnimPlayTimer.getElapsedTimeF32() > mExpiryTime)
	{
		mAnimPlayTimer.stop();
		if (mAnimEditorScroll->getItemCount() != 0)
		{
			mAnimEditorScroll->selectNthItem(mAnimScrollIndex);
			LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
			if (item)
			{
				//BD - We can't use Wait or Restart as label, need to fix this.
				std::string label = item->getColumn(0)->getValue().asString();
				if (label == "Wait")
				{
					//BD - Do nothing?
					mExpiryTime = item->getColumn(1)->getValue().asReal();
					mAnimScrollIndex++;
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
					mAnimScrollIndex++;
				}
			}
		}

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
	std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses", "");

	LLDirIterator dir_iter(dir, "*.xml");
	bool found = true;
	while (found)
	{
		std::string file;
		found = dir_iter.next(file);

		if (found)
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
			if (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(data, infile))
			{
				row["columns"][1]["column"] = "time";
				row["columns"][1]["value"] = data["time"];
				row["columns"][2]["column"] = "type";
				row["columns"][2]["value"] = data["type"];
			}
			infile.close();
			mPoseScroll->addElement(row);
		}
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
			filename = item->getColumn(0)->getValue();
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

	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses", filename + ".xml");
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
		while (!infile.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(old_record, infile))
		{
			if (line != 0)
			{
				record[line]["bone"] = old_record["bone"];
				record[line]["rotation"] = old_record["rotation"];
				if (old_record["bone"].asString() == "mPelvis")
				{
					record[line]["position"] = old_record["position"];
				}
			}
			line++;
		}

		//BD - Change the header here.
		record[0]["type"] = type;
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
		if (type == 2)
		{
			time = llclamp(time, 0.0f, 1.0f);
		}
		record[line]["time"] = time;
		record[line]["type"] = type;
		line++;

		//BD - Now create the rest.
		mJointsScroll->selectAll();
		std::vector<LLScrollListItem*> items = mJointsScroll->getAllSelected();
		for (std::vector<LLScrollListItem*>::iterator item = items.begin();
			item != items.end(); ++item)
		{
			LLVector3 vec3;
			LLScrollListItem* element = *item;
			LLJoint* joint = (LLJoint*)element->getUserdata();
			if (joint)
			{
				record[line]["bone"] = joint->getName();
				joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
				record[line]["rotation"] = vec3.getValue();

				//BD - Pelvis is a special case, add position values too.
				if (joint->getName() == "mPelvis")
				{
					vec3.mV[VX] = (*item)->getColumn(5)->getValue().asReal();
					vec3.mV[VY] = (*item)->getColumn(6)->getValue().asReal();
					vec3.mV[VZ] = (*item)->getColumn(7)->getValue().asReal();
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
							record[line]["enabled"] = TRUE;
						}
						else
						{
							record[line]["enabled"] = FALSE;
						}
					}
				}

				line++;
			}
		}

		mJointsScroll->deselectAllItems();
	}
	
	llofstream file;
	file.open(full_path.c_str());
	//BD - Now lets actually write the file, whether it is writing a new one
	//     or just rewriting the previous one with a new header.
	for (S32 cur_line = 0; cur_line < line; cur_line++)
	{
		LLSDSerialize::toXML(record[cur_line], file);
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
		filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses", name.asString() + ".xml");
	}
	else if (item)
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses", item->getColumn(0)->getValue().asString() + ".xml");
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
			LL_WARNS("Posing") << "Failed to parse file" << filename << LL_ENDL;
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
			LLVector3 vec3;
			LLQuaternion quat;

			vec3.setValue(pose["rotation"]);
			joint->setLastRotation(joint->getRotation());
			quat.setEulerAngles(vec3.mV[VX], vec3.mV[VZ], vec3.mV[VY]);
			joint->setTargetRotation(quat);

			if (joint->getName() == "mPelvis")
			{
				vec3.setValue(pose["position"]);
				joint->setLastPosition(joint->getPosition());
				joint->setTargetPosition(vec3);
			}

			if (motion)
			{
				LLPose* mpose = motion->getPose();
				if (mpose)
				{
					LLPointer<LLJointState> joint_state = mpose->findJointState(joint);
					std::string state_string = pose["enabled"];
					if (!state_string.empty())
					{
						bool state_enabled = pose["enabled"].asBoolean();
						if (!joint_state && state_enabled)
						{
							motion->addJointToState(joint);
						}
						else if (joint_state && !state_enabled)
						{
							motion->removeJointState(joint_state);
						}
					}
					else
					{
						//BD - Fail safe, assume that a bone is always enabled in case we
						//     load a pose that was created prior to including the enabled
						//     state.
						if (!joint_state)
						{
							motion->addJointToState(joint);
						}
					}
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
		//BD - Pause all other motions and prevent them from interrupting us even
		//     if they technically shouldn't be able to anyway.
		//     This might not be necessary anymore.
		//gAgentAvatarp->getMotionController().pauseAllMotions();

		gAgent.setPosing();
		gAgent.stopFidget();
		gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		//BD - Reset all bones to make sure that those that are not going to be animated
		//     after posing are back in their correct default state.
		mJointsScroll->selectAll();
		onJointReset();
		mJointsScroll->deselectAllItems();

		//BD - Unpause all other motions that we paused to prevent them overriding
		//     our Poser.
		//     This might not be necessary anymore.
		//gAgentAvatarp->getMotionController().unpauseAllMotions();
		gAgent.clearPosing();
		gAgentAvatarp->stopMotion(ANIM_BD_POSING_MOTION);
	}
	//BD - Wipe the joint list.
	onJointRefresh();

	onPoseControlsRefresh();
}

void BDFloaterPoser::onPoseDelete()
{
	std::vector<LLScrollListItem*> items = mPoseScroll->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin();
		iter != items.end(); ++iter)
	{
		LLScrollListItem* item = (*iter);
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

void BDFloaterPoser::onPoseSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		S32 type;
		F32 time;
		//BD - Is this actually required since we save the pose anyway?
		if (param.asString() == "time")
		{
			LLScrollListCell* column = item->getColumn(2);
			time = getChild<LLUICtrl>("interp_time")->getValue().asReal();
			type = column->getValue().asInteger();
			column->setValue(time);
		}
		else
		{
			LLScrollListCell* column = item->getColumn(1);
			time = column->getValue().asReal();
			type = getChild<LLUICtrl>("interp_type")->getValue().asInteger();
			column->setValue(type);
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
	getChild<LLUICtrl>("interp_time")->setEnabled(item ? true : false);
	getChild<LLUICtrl>("interp_type")->setEnabled(item ? true : false);
	getChild<LLUICtrl>("delete_poses")->setEnabled(item ? true : false);
	getChild<LLUICtrl>("load_poses")->setEnabled(item ? true : false);
}

////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterPoser::onJointRefresh()
{
	bool is_posing = gAgent.getPosing();
	mJointsScroll->clearRows();
	S32 i = 0;
	for (;; i++)
	{
		LLJoint* joint = gAgentAvatarp->getCharacterJoint(i);
		if (joint)
		{
			LLSD row;
			//BD - Show some categories to make it a bit easier finding out which
			//     bone belongs where and what they might be for those who can't use
			//     bone names.
			std::string name = joint->getName();
			if (name == "mPelvis" ||
				name == "mHead" ||
				name == "mCollarLeft" ||
				name == "mCollarRight" ||
				name == "mWingsRoot" ||
				name == "mHipLeft" ||
				name == "mHipRight" ||
				name == "mTail1" ||
				name == "mGroin")
			{
				row["columns"][0]["column"] = "icon";
				row["columns"][0]["type"] = "icon";
				row["columns"][0]["value"] = getString("icon_category");
				row["columns"][1]["column"] = "joint";
				row["columns"][1]["value"] = getString("title_" + name);
				LLScrollListItem* item = mJointsScroll->addElement(row);
				((LLScrollListText*)item->getColumn(1))->setFontStyle(LLFontGL::BOLD);
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

			row["columns"][0]["column"] = "icon";
			row["columns"][0]["type"] = "icon";
			row["columns"][0]["value"] = getString("icon_bone");
			row["columns"][1]["column"] = "joint";
			row["columns"][1]["value"] = joint->getName();
			row["columns"][2]["column"] = "x";
			row["columns"][2]["value"] = ll_round(vec3.mV[VX], 0.001f);
			row["columns"][3]["column"] = "y";
			row["columns"][3]["value"] = ll_round(vec3.mV[VY], 0.001f);
			row["columns"][4]["column"] = "z";
			row["columns"][4]["value"] = ll_round(vec3.mV[VZ], 0.001f);

			//BD - Special case for mPelvis as it has position information too.
			if (name == "mPelvis")
			{
				if (is_posing)
				{
					vec3 = joint->getTargetPosition();
				}
				else
				{
					vec3 = joint->getPosition();
				}
				row["columns"][5]["column"] = "pos_x";
				row["columns"][5]["value"] = ll_round(vec3.mV[VX], 0.001f);
				row["columns"][6]["column"] = "pos_y";
				row["columns"][6]["value"] = ll_round(vec3.mV[VY], 0.001f);
				row["columns"][7]["column"] = "pos_z";
				row["columns"][7]["value"] = ll_round(vec3.mV[VZ], 0.001f);
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
						LLPointer<LLJointState> joint_state = pose->findJointState(joint);
						if (joint_state)
						{
							((LLScrollListText*)item->getColumn(1))->setFontStyle(LLFontGL::BOLD);
						}
					}
				}
			}
			else
			{
				((LLScrollListText*)item->getColumn(1))->setFontStyle(LLFontGL::NORMAL);
			}
		}
		else
		{
			break;
		}
	}
	onJointControlsRefresh();
}

void BDFloaterPoser::onJointControlsRefresh()
{
	bool is_pelvis = false;
	bool is_posing = gAgent.getPosing();
	LLScrollListItem* item = mJointsScroll->getFirstSelected();
	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			mRotX->setValue(item->getColumn(2)->getValue());
			mRotY->setValue(item->getColumn(3)->getValue());
			mRotZ->setValue(item->getColumn(4)->getValue());
			mRotXBig->setValue(item->getColumn(2)->getValue());
			mRotYBig->setValue(item->getColumn(3)->getValue());
			mRotZBig->setValue(item->getColumn(4)->getValue());
			if (joint && joint->getName() == "mPelvis")
			{
				is_pelvis = true;
				mPosX->setValue(item->getColumn(5)->getValue());
				mPosY->setValue(item->getColumn(6)->getValue());
				mPosZ->setValue(item->getColumn(7)->getValue());
			}

			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				//BD - If we don't use our default, set it once.
				if (motion->getInterpolationTime() != 0.1
					|| motion->getInterpolationType() != 2)
				{
					motion->setInterpolationTime(0.25f);
					motion->setInterpolationType(2);
				}

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
	getChild<LLButton>("reset_bone")->setEnabled(item && is_posing);
	getChild<LLButton>("activate")->setValue(is_posing);
	getChild<LLUICtrl>("pose_name")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_time")->setEnabled(is_posing);
	getChild<LLUICtrl>("interpolation_type")->setEnabled(is_posing);
	getChild<LLUICtrl>("save_poses")->setEnabled(is_posing);

	mRotX->setEnabled(item && is_posing);
	mRotY->setEnabled(item && is_posing);
	mRotZ->setEnabled(item && is_posing);
	mRotXBig->setEnabled(item && is_posing);
	mRotYBig->setEnabled(item && is_posing);
	mRotZBig->setEnabled(item && is_posing);

	mRotXBig->setVisible(!is_pelvis);
	mRotYBig->setVisible(!is_pelvis);
	mRotZBig->setVisible(!is_pelvis);
	mRotX->setVisible(is_pelvis);
	mRotY->setVisible(is_pelvis);
	mRotZ->setVisible(is_pelvis);

	mPosX->setVisible(is_pelvis);
	mPosY->setVisible(is_pelvis);
	mPosZ->setVisible(is_pelvis);
}

void BDFloaterPoser::onJointSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLJoint* joint = (LLJoint*)mJointsScroll->getFirstSelected()->getUserdata();
	if (joint)
	{
		//BD - Neat yet quick and direct way of rotating our bones.
		//     No more need to include bone rotation orders.
		F32 val = ctrl->getValue().asReal();
		LLScrollListCell* column_1 = mJointsScroll->getFirstSelected()->getColumn(2);
		LLScrollListCell* column_2 = mJointsScroll->getFirstSelected()->getColumn(3);
		LLScrollListCell* column_3 = mJointsScroll->getFirstSelected()->getColumn(4);
		LLQuaternion rot_quat = joint->getTargetRotation();
		LLMatrix3 rot_mat;
		F32 old_value;

		if (param.asString() == "x")
		{
			old_value = column_1->getValue().asReal();
			column_1->setValue(ll_round(val, 0.001f));
			val -= old_value;
			rot_mat = LLMatrix3(val, 0.f, 0.f);
		}
		else if (param.asString() == "y")
		{
			old_value = column_2->getValue().asReal();
			column_2->setValue(ll_round(val, 0.001f));
			val -= old_value;
			rot_mat = LLMatrix3(0.f, val, 0.f);
		}
		else
		{
			old_value = column_3->getValue().asReal();
			column_3->setValue(ll_round(val, 0.001f));
			val -= old_value;
			rot_mat = LLMatrix3(0.f, 0.f, val);
		}
		rot_quat = LLQuaternion(rot_mat)*rot_quat;
		joint->setTargetRotation(rot_quat);
	}
}

void BDFloaterPoser::onJointPosSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLJoint* joint = (LLJoint*)mJointsScroll->getFirstSelected()->getUserdata();
	if (joint && joint->getName() == "mPelvis")
	{
		F32 val = ctrl->getValue().asReal();
		LLVector3 vec3;
		LLScrollListCell* column_4 = mJointsScroll->getFirstSelected()->getColumn(5);
		LLScrollListCell* column_5 = mJointsScroll->getFirstSelected()->getColumn(6);
		LLScrollListCell* column_6 = mJointsScroll->getFirstSelected()->getColumn(7);

		vec3.mV[VX] = column_4->getValue().asReal();
		vec3.mV[VY] = column_5->getValue().asReal();
		vec3.mV[VZ] = column_6->getValue().asReal();

		if (param.asString() == "x")
		{
			vec3.mV[VX] = val;
			column_4->setValue(ll_round(vec3.mV[VX], 0.001f));
		}
		else if (param.asString() == "y")
		{
			vec3.mV[VY] = val;
			column_5->setValue(ll_round(vec3.mV[VY], 0.001f));
		}
		else
		{
			vec3.mV[VZ] = val;
			column_6->setValue(ll_round(vec3.mV[VZ], 0.001f));
		}
		llassert(!vec3.isFinite());
		joint->setTargetPosition(vec3);
	}
}

void BDFloaterPoser::onJointChangeState()
{
	std::vector<LLScrollListItem*> items = mJointsScroll->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						motion->removeJointState(joint_state);
						((LLScrollListText*)item->getColumn(1))->setFontStyle(LLFontGL::NORMAL);
					}
					else
					{
						motion->addJointToState(joint);
						((LLScrollListText*)item->getColumn(1))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
	}
	onJointControlsRefresh();
}

void BDFloaterPoser::onJointReset()
{
	std::vector<LLScrollListItem*> items = mJointsScroll->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			LLQuaternion quat;
			LLScrollListCell* column_1 = item->getColumn(2);
			LLScrollListCell* column_2 = item->getColumn(3);
			LLScrollListCell* column_3 = item->getColumn(4);

			F32 round_val = ll_round(0, 0.001f);
			column_1->setValue(round_val);
			column_2->setValue(round_val);
			column_3->setValue(round_val);
			
			//BD - While editing rotations, make sure we use a bit of linear interpolation to make movements smoother.
			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				//BD - If we don't use our default, set it once.
				if (motion->getInterpolationTime() != 0.1
					|| motion->getInterpolationType() != 2)
				{
					motion->setInterpolationTime(0.25f);
					motion->setInterpolationType(2);
				}
			}

			quat.setEulerAngles(0,0,0);
			joint->setTargetRotation(quat);

			//BD - The reason we don't use the default position here is because it will
			//     make the pelvis float under certain circumstances with certain meshes
			//     attached, simply revert to 0,0,0. Better safe than sorry.
			if (joint->getName() == "mPelvis")
			{
				LLScrollListCell* column_4 = item->getColumn(5);
				LLScrollListCell* column_5 = item->getColumn(6);
				LLScrollListCell* column_6 = item->getColumn(7);

				column_4->setValue(round_val);
				column_5->setValue(round_val);
				column_6->setValue(round_val);

				joint->setTargetPosition(LLVector3::zero);
			}
		}
	}

	onJointControlsRefresh();
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
	//BD - This is going to be a really dirty way of inserting elements inbetween others
	//     until i can figure out a better way.
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 index = 0;
	std::vector<LLScrollListItem*> old_items = mAnimEditorScroll->getAllData();
	std::vector<LLScrollListItem*> new_items;
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
			LL_WARNS("Animator") << "No scroll list item was selected or found, return." << LL_ENDL;
			return;
		}
	}

	new_item = mAnimEditorScroll->addElement(row);

	if (!new_item)
	{
		LL_WARNS("Animator") << "Item we wrote is invalid, abort." << LL_ENDL;
		new_item->setFlagged(true);
		mAnimEditorScroll->deleteFlaggedItems();
		return;
	}

	//BD - We can skip this on first add.
	if (selected_index != -1)
	{
		//BD - Let's go through all entries in the list and copy them into a new
		//     list in our new desired order, flag the old ones for removal while
		//     we do this.
		for (std::vector<LLScrollListItem*>::iterator it = old_items.begin();
			it != old_items.end(); ++it)
		{
			LLScrollListItem* item = (*it);
			if (item)
			{
				new_items.push_back(item);
			}

			if (index == selected_index)
			{
				new_items.push_back(new_item);
			}
			index++;
		}

		onAnimListWrite(new_items);
	}

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(selected_index + 1);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimListWrite(std::vector<LLScrollListItem*> item_list)
{
	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (std::vector<LLScrollListItem*>::iterator it = item_list.begin();
		it != item_list.end(); ++it)
	{
		LLScrollListItem* item = (*it);
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
		else
		{
			LL_WARNS("Animator") << "Invalid element or nothing in list." << LL_ENDL;
		}
	}

	//BD - Delete all flagged items now and we'll end up with a new list order.
	mAnimEditorScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();
}

void BDFloaterPoser::onAnimMove(const LLSD& param)
{
	std::vector<LLScrollListItem*> old_items = mAnimEditorScroll->getAllData();
	std::vector<LLScrollListItem*> new_items;
	S32 new_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 index = 0;
	bool skip = false;

	//BD - Don't allow moving down when we are already at the bottom, this would
	//     create another entry.
	//     Don't allow going up when we are on the first entry already either, this
	//     would select everything in the list.
	//     Don't allow moving if there's nothing to move. (Crashfix)
	if (((new_index + 1) >= old_items.size() && param.asString() == "Down")
		|| (new_index == 0 && param.asString() == "Up")
		|| old_items.empty())
	{
		LL_WARNS("Animator") << "We are already at the top/bottom!" << LL_ENDL;
		return;
	}

	LLScrollListItem* cur_item = mAnimEditorScroll->getFirstSelected();

	//BD - Don't allow moving if we don't have anything selected either. (Crashfix)
	if (!cur_item)
	{
		LL_WARNS("Animator") << "Nothing selected to move, abort." << LL_ENDL;
		return;
	}

	//BD - Move up, otherwise move the entry down. No other option.
	if (param.asString() == "Up")
	{
		new_index--;
	}
	else
	{
		mAnimEditorScroll->selectNthItem(new_index + 1);
	}

	cur_item = mAnimEditorScroll->getFirstSelected();
	cur_item->setFlagged(true);

	//BD - Let's go through all entries in the list and copy them into a new
	//     list in our new desired order, flag the old ones for removal while
	//     we do this.
	//     For both up and down this works exactly the same, we made sure of that.
	for (std::vector<LLScrollListItem*>::iterator it = old_items.begin();
		it != old_items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			item->setFlagged(true);
			if (!skip)
			{
				if (index == new_index)
				{
					new_items.push_back(cur_item);
					skip = true;
				}

				new_items.push_back(item);
			}
			else
			{
				skip = false;
			}
		}
		index++;
	}

	onAnimListWrite(new_items);

	if (param.asString() == "Up")
	{
		mAnimEditorScroll->selectNthItem(new_index);
	}
	else
	{
		mAnimEditorScroll->selectNthItem(new_index + 1);
	}
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
	mExpiryTime = 0.0f;
	mAnimPlayTimer.start();
}

void BDFloaterPoser::onAnimStop()
{
	mAnimPlayTimer.stop();
}

void BDFloaterPoser::onAnimControlsRefresh()
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 index = mAnimEditorScroll->getFirstSelectedIndex();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("load_poses")->setEnabled(true);
		getChild<LLUICtrl>("delete_poses")->setEnabled(true);
		getChild<LLUICtrl>("move_up")->setEnabled(index != 0);
		getChild<LLUICtrl>("move_down")->setEnabled(!((index + 1) >= item_count));

		if (item->getColumn(0)->getValue().asString() == "Wait")
		{
			getChild<LLUICtrl>("anim_time")->setEnabled(true);
		}
		else
		{
			getChild<LLUICtrl>("anim_time")->setEnabled(false);
		}
	}
	else
	{
		getChild<LLUICtrl>("load_poses")->setEnabled(false);
		getChild<LLUICtrl>("delete_poses")->setEnabled(false);
		getChild<LLUICtrl>("anim_time")->setEnabled(false);
		getChild<LLUICtrl>("move_up")->setEnabled(false);
		getChild<LLUICtrl>("move_down")->setEnabled(false);
	}
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
		S32 floater_width = (poses_expanded && !animator_expanded) ? 745.f : 517.f;
		this->reshape(floater_width, this->getRect().getHeight());
	}
}