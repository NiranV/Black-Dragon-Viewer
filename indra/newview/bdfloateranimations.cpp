/** 
 * 
 * Copyright (C) 2017, NiranV Dean
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

#include "bdfloateranimations.h"
#include "lluictrlfactory.h"
#include "llworld.h"
#include "llagent.h"
#include "llcharacter.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llsdserialize.h"
#include "lldiriterator.h"

// viewer includes
#include "llfloaterpreference.h"

#include "bdposingmotion.h"

BDFloaterAnimations::BDFloaterAnimations(const LLSD& key)
	:	LLFloater(key)
{
	//BD - This is the global command from which on we decide what to do. We had them all separately before.
	//     "Stop" - Stops an animation complete, essentially just reset animation.
	//     "Freeze" - Pauses an animation just like if we set its playback speed to 0.0.
	//     "Set" - Changes the animation playback speed to the desired value.
	//     "Restart" - Restarts the animation, could be used as animation sync.
	//     "Save" - Save a motion to be copied to other avatars.
	//     "Load" - Copy a motion to a selected avatar and make it play it.
	//     "Remove" - Remove all selected motions from our list.
	//     "Create" - Create a new motion via a given UUID.
	mCommitCallbackRegistrar.add("Motion.Command", boost::bind(&BDFloaterAnimations::onMotionCommand, this, _1, _2));

	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Motion.Refresh", boost::bind(&BDFloaterAnimations::onMotionRefresh, this));

	//BD - Save our current pose into a XML file to import it later or use it for creating an animation.
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterAnimations::onClickPoseSave, this));
	//BD - Start our custom pose.
	mCommitCallbackRegistrar.add("Pose.Start", boost::bind(&BDFloaterAnimations::onPoseStart, this));
	//BD - Load the current pose and export all its values into the UI so we can alter them.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterAnimations::onPoseLoad, this, ""));
	//BD - Delete the currently selected Pose.
	mCommitCallbackRegistrar.add("Pose.Delete", boost::bind(&BDFloaterAnimations::onPoseDelete, this));
	//BD - Change a pose's blend type and time.
	mCommitCallbackRegistrar.add("Pose.Set", boost::bind(&BDFloaterAnimations::onPoseSet, this, _1, _2));

	//BD - Change a bone's rotation.
	mCommitCallbackRegistrar.add("Joint.Set", boost::bind(&BDFloaterAnimations::onJointSet, this, _1, _2));
	//BD - Change a bone's position (specifically the mPelvis bone).
	mCommitCallbackRegistrar.add("Joint.PosSet", boost::bind(&BDFloaterAnimations::onJointPosSet, this, _1, _2));
	//BD - Add or remove a joint state to or from the pose (enable/disable our overrides).
	mCommitCallbackRegistrar.add("Joint.ChangeState", boost::bind(&BDFloaterAnimations::onJointChangeState, this));

	//BD - Add a new entry to the animation creator.
	mCommitCallbackRegistrar.add("Anim.Add", boost::bind(&BDFloaterAnimations::onAnimAdd, this));
	//BD - Remove an entry in the animation creator.
	mCommitCallbackRegistrar.add("Anim.Delete", boost::bind(&BDFloaterAnimations::onAnimDelete, this));
	//BD - Save the currently build list as animation.
	mCommitCallbackRegistrar.add("Anim.Save", boost::bind(&BDFloaterAnimations::onAnimSave, this));
	//BD - Play the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Play", boost::bind(&BDFloaterAnimations::onAnimPlay, this));
	//BD - Stop the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Stop", boost::bind(&BDFloaterAnimations::onAnimStop, this));

//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX", boost::bind(&LLFloaterPreference::onCommitX, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY", boost::bind(&LLFloaterPreference::onCommitY, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ", boost::bind(&LLFloaterPreference::onCommitZ, _1, _2));

//	//BD - Revert to Default
	mCommitCallbackRegistrar.add("Pref.Default", boost::bind(&BDFloaterAnimations::resetToDefault, this, _1));
}

BDFloaterAnimations::~BDFloaterAnimations()
{
}

BOOL BDFloaterAnimations::postBuild()
{
	//BD - Motions
	mAvatarScroll = getChild<LLScrollListCtrl>("other_avatars_scroll");
	mMotionScroll = getChild<LLScrollListCtrl>("motions_scroll");

	//BD - Posing
	mPoseScroll = this->getChild<LLScrollListCtrl>("poses_scroll", true);
	mPoseScroll->setCommitOnSelectionChange(TRUE);
	mPoseScroll->setCommitCallback(boost::bind(&BDFloaterAnimations::onPoseControlsRefresh, this));
	mPoseScroll->setDoubleClickCallback(boost::bind(&BDFloaterAnimations::onPoseLoad, this, ""));
	mJointsScroll = this->getChild<LLScrollListCtrl>("joints_scroll", true);
	mJointsScroll->setCommitOnSelectionChange(TRUE);
	mJointsScroll->setCommitCallback(boost::bind(&BDFloaterAnimations::onJointControlsRefresh, this));
	mJointsScroll->setDoubleClickCallback(boost::bind(&BDFloaterAnimations::onJointChangeState, this));

	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimScrollIndex = 0;
	return TRUE;
}

void BDFloaterAnimations::draw()
{
	if (mAnimPlayTimer.getStarted() &&
		mAnimPlayTimer.getElapsedTimeF32() > mExpiryTime)
	{
		mAnimPlayTimer.stop();
		LL_INFOS("Posing") << "Stopping the timer temporarily. We are at: " << mAnimScrollIndex << LL_ENDL;
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
			LL_INFOS("Posing") << "Continueing the timer." << LL_ENDL;
		}
	}

	LLFloater::draw();
}

//BD - Revert to Default
void BDFloaterAnimations::resetToDefault(LLUICtrl* ctrl)
{
	ctrl->getControlVariable()->resetToDefault(true);
	onJointRefresh();
}

void BDFloaterAnimations::onOpen(const LLSD& key)
{
	onMotionRefresh();
	onJointRefresh();
	onPoseRefresh();
}

void BDFloaterAnimations::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mAvatarScroll->clearRows();
	mMotionScroll->clearRows();
	mJointsScroll->clearRows();
}

////////////////////////////////
//BD - Motions
////////////////////////////////
void BDFloaterAnimations::onMotionRefresh()
{
	LLAvatarTracker& at = LLAvatarTracker::instance();
	//BD - First lets go through all things that need updating, if any.
	std::vector<LLScrollListItem*> items = mAvatarScroll->getAllData();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			//BD - Flag all items for removal by default.
			item->setFlagged(TRUE);

			//BD - Now lets check our list against all current avatars.
			for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
				iter != LLCharacter::sInstances.end(); ++iter)
			{
				LLCharacter* character = (*iter);
				//BD - Check if the character is valid and if its the same.
				if (character && character == item->getUserdata())
				{
					//BD - Remove the removal flag, we're updating it.
					item->setFlagged(FALSE);

					//BD - When we refresh it might happen that we don't have a name for someone
					//     yet, when this happens the list entry won't be purged and rebuild as
					//     it will be updated with this part, so we have to update the name in
					//     case it was still being resolved last time we refreshed and created the
					//     initial list entry. This prevents the name from missing forever.
					if (item->getColumn(0)->getValue().asString().empty())
					{
						LLAvatarName av_name;
						LLAvatarNameCache::get(character->getID(), &av_name);
						item->getColumn(0)->setValue(av_name.asLLSD());
					}

					//BD - Show if we got the permission to animate them and save their motions.
					//     Todo: Don't do anything if permissions didn't change.
					std::string str = "-";
					const LLRelationship* relation = at.getBuddyInfo(character->getID());
					if (relation)
					{
						str = relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "Yes" : "No";
					}
					else
					{
						str = character->getID() == gAgentID ? "Yes" : "-";
					}
					item->getColumn(3)->setValue(str);

					F32 value = character->getMotionController().getTimeFactor();
					item->getColumn(2)->setValue(value);
					break;
				}
			}
		}
	}

	//BD - Now safely delete all items so we can start adding the missing ones.
	mAvatarScroll->deleteFlaggedItems();

	//BD - Now lets do it the other way around and look for new things to add.
	items = mAvatarScroll->getAllData();
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLCharacter* character = (*iter);
		if (character)
		{
			LLUUID uuid = character->getID();
			bool skip = false;

			for (std::vector<LLScrollListItem*>::iterator it = items.begin();
				it != items.end(); ++it)
			{
				if ((*it)->getColumn(1)->getValue().asString() == uuid.asString())
				{
					skip = true;
					break;
				}
			}

			if (skip)
			{
				continue;
			}

			F32 value = character->getMotionController().getTimeFactor();
			LLAvatarName av_name;
			LLAvatarNameCache::get(uuid, &av_name);
			const LLRelationship* relation = at.getBuddyInfo(uuid);

			std::string str = "-";
			if (relation)
			{
				str = relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "Yes" : "No";
			}
			else
			{
				str = uuid == gAgentID ? "Yes" : "-";
			}

			LLSD row;
			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = av_name.getDisplayName();
			row["columns"][1]["column"] = "uuid";
			row["columns"][1]["value"] = uuid.asString();
			row["columns"][2]["column"] = "value";
			row["columns"][2]["value"] = value;
			//BD - Show if we got the permission to animate them and save their motions.
			row["columns"][3]["column"] = "permission";
			row["columns"][3]["value"] = str;
			LLScrollListItem* element = mAvatarScroll->addElement(row);
			element->setUserdata(character);
		}
	}
}

void BDFloaterAnimations::onMotionCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		//BD - First lets get all selected characters, we'll need them for all
		//     the things we're going to do.
		std::vector<LLScrollListItem*> items = mAvatarScroll->getAllSelected();
		for (std::vector<LLScrollListItem*>::iterator item = items.begin();
			item != items.end(); ++item)
		{
			LLScrollListItem* element = (*item);
			LLCharacter* character = (LLCharacter*)element->getUserdata();
			
			//BD - Check if our selected characters are still on the SIM.
			bool is_valid = false;
			for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
				iter != LLCharacter::sInstances.end(); ++iter)
			{
				LLCharacter* character2 = (*iter);
				if (character == character2)
				{
					is_valid = true;
					break;
				}
			}

			if (character && is_valid)
			{
				if (param.asString() == "Freeze")
				{
					if (character->getMotionController().isPaused())
					{
						character->getMotionController().unpauseAllMotions();
					}
					else
					{
						mAvatarPauseHandles.push_back(character->requestPause());
					}
				}
				else if (param.asString() == "Set")
				{
					character->getMotionController().setTimeFactor(getChild<LLUICtrl>("anim_factor")->getValue().asReal());
					onMotionRefresh();
				}
				else if (param.asString() == "Restart")
				{
					LLMotionController::motion_list_t motions = character->getMotionController().getActiveMotions();
					for (LLMotionController::motion_list_t::iterator it = motions.begin();
						it != motions.end(); ++it)
					{
						LLMotion* motion = *it;
						if (motion)
						{
							LLUUID motion_id = motion->getID();
							character->stopMotion(motion_id, 1);
							character->startMotion(motion_id, 0.0f);
						}
					}
				}
				else if (param.asString() == "Stop")
				{
					//BD - Special case to prevent the posing animation from becoming stuck.
					if (character->getID() == gAgentID)
					{
						gAgentAvatarp->stopMotion(ANIM_BD_POSING_MOTION);
						gAgent.clearPosing();
					}

					character->deactivateAllMotions();
				}
				else if (param.asString() == "Remove")
				{
					mMotionScroll->deleteSelectedItems();
				}
				else if (param.asString() == "Save")
				{
					LLAvatarTracker& at = LLAvatarTracker::instance();
					//BD - Only allow saving motions whose object mod rights we have.
					const LLRelationship* relation = at.getBuddyInfo(character->getID());
					if (relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS)
						|| character->getID() == gAgentID)
					{
						LLMotionController::motion_list_t motions = character->getMotionController().getActiveMotions();
						for (LLMotionController::motion_list_t::iterator it = motions.begin();
							it != motions.end(); ++it)
						{
							LLMotion* motion = *it;
							if (motion)
							{
								LLUUID motion_id = motion->getID();
								LLSD row;
								LLAvatarName av_name;
								LLAvatarNameCache::get(character->getID(), &av_name);
								row["columns"][0]["column"] = "name";
								row["columns"][0]["value"] = av_name.getDisplayName();
								row["columns"][1]["column"] = "uuid";
								row["columns"][1]["value"] = motion_id.asString();
								row["columns"][2]["column"] = "priority";
								row["columns"][2]["value"] = motion->getPriority();
								row["columns"][3]["column"] = "duration";
								row["columns"][3]["value"] = motion->getDuration();
								row["columns"][4]["column"] = "blend type";
								row["columns"][4]["value"] = motion->getBlendType();
								row["columns"][5]["column"] = "loop";
								row["columns"][5]["value"] = motion->getLoop();
								if (motion->getName().empty() &&
									motion->getDuration() > 0.0f)
								{
									mMotionScroll->addElement(row);
								}
							}
						}
					}
				}
				else if (param.asString() == "Load")
				{
					//BD - Only allow animating people whose object mod rights we have.
					//const LLRelationship* relation = at.getBuddyInfo(character->getID());
					//if (relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS)
					//	|| character->getID() == gAgentID)
					//if (character->getID() == gAgentID)
					//{
						
						//BD - Only allow animating ourselves, as per Oz Linden.
						LLScrollListItem* item = mMotionScroll->getFirstSelected();
						if (item)
						{
							LLUUID motion_id = item->getColumn(1)->getValue().asUUID();
							if (!motion_id.isNull())
							{
								gAgentAvatarp->createMotion(motion_id);
								gAgentAvatarp->stopMotion(motion_id, 1);
								gAgentAvatarp->startMotion(motion_id, 0.0f);
							}
						}
					//}
				}
				else if (param.asString() == "Create")
				{
					LLUUID motion_id = getChild<LLUICtrl>("motion_uuid")->getValue().asUUID();
					if (!motion_id.isNull())
					{
						LLSD row;
						LLAvatarName av_name;
						LLAvatarNameCache::get(gAgentID, &av_name);
						row["columns"][0]["column"] = "name";
						row["columns"][0]["value"] = av_name.getDisplayName();
						row["columns"][1]["column"] = "uuid";
						row["columns"][1]["value"] = motion_id.asString();
						mMotionScroll->addElement(row);
					}
				}
			}
		}
	}
}

////////////////////////////////
//BD - Poses
////////////////////////////////
void BDFloaterAnimations::onPoseRefresh()
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

void BDFloaterAnimations::onPoseControlsRefresh()
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("interp_time")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("interp_type")->setValue(item->getColumn(2)->getValue());
		getChild<LLUICtrl>("interp_time")->setEnabled(true);
		getChild<LLUICtrl>("interp_type")->setEnabled(true);
	}
	else
	{
		getChild<LLUICtrl>("interp_time")->setEnabled(false);
		getChild<LLUICtrl>("interp_type")->setEnabled(false);
	}
}

void BDFloaterAnimations::onClickPoseSave()
{
	//BD - Values don't matter when not editing.
	onPoseSave(2, 0.1f, false);
}

BOOL BDFloaterAnimations::onPoseSave(S32 type, F32 time, bool editing)
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
			//BD - Ohoh, don't write bogus bones into the file, when we load them
			//     we'll somehow end up "injecting" them into the chain which will
			//     break said bone until relog. "mHindLimb4Right" Does this a lot.
			//     No idea if it's a bug with something i did or simply a broken bone.
			//     I have to assume that it's a bone breaking upon loading somehow
			//     as the floater only writes into the file what is given by the Viewer
			//     so the Viewer has to have a broken bone and give its broken name to us
			//     before we can even write a broken name. Now how does "mHindLimb4Right"
			//     end up being broken in the first place?
			if (joint && gAgentAvatarp->getJoint(joint->getName()))
			{
				record[line]["bone"] = joint->getName();
				vec3.mV[VX] = (*item)->getColumn(1)->getValue().asReal();
				vec3.mV[VY] = (*item)->getColumn(2)->getValue().asReal();
				vec3.mV[VZ] = (*item)->getColumn(3)->getValue().asReal();
				record[line]["rotation"] = vec3.getValue();

				//BD - Pelvis is a special case, add position values too.
				if (joint->getName() == "mPelvis")
				{
					vec3.mV[VX] = (*item)->getColumn(4)->getValue().asReal();
					vec3.mV[VY] = (*item)->getColumn(5)->getValue().asReal();
					vec3.mV[VZ] = (*item)->getColumn(6)->getValue().asReal();
					record[line]["position"] = vec3.getValue();
				}

				//BD - Save the enabled state per preset so we can switch bones on and off
				//     on demand inbetween poses additionally to globally.
				LLMotion* motion = gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
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

BOOL BDFloaterAnimations::onPoseLoad(const LLSD& name)
{
	//BOOL ret = TRUE;
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
			quat.setEulerAngles(vec3.mV[VX], vec3.mV[VZ], vec3.mV[VY]);
			joint->setLastRotation(joint->getRotation());
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

void BDFloaterAnimations::onPoseStart()
{
	LLMotion* pose_motion = gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (!pose_motion)
	{
		gAgent.setPosing();
		gAgent.stopFidget();
		gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
	}
	else if (pose_motion->isStopped())
	{
		gAgent.setPosing();
		gAgent.stopFidget();
		gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		gAgent.clearPosing();
		gAgentAvatarp->stopMotion(ANIM_BD_POSING_MOTION);
	}
	//BD - Wipe the joint list.
	onJointRefresh();
}

void BDFloaterAnimations::onPoseDelete()
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

void BDFloaterAnimations::onPoseSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		S32 type;
		F32 time;
		if (param.asString() == "time")
		{
			LLScrollListCell* column = item->getColumn(2);
			time = getChild<LLUICtrl>("interp_time")->getValue().asReal();
			type = column->getValue().asInteger();
			column->setValue(time);
		}
		else // if (param.asString() == "type")
		{
			LLScrollListCell* column = item->getColumn(1);
			time = column->getValue().asReal();
			type = getChild<LLUICtrl>("interp_type")->getValue().asInteger();
			column->setValue(type);
		}
		onPoseSave(type, time, true);
	}
}


////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterAnimations::onJointRefresh()
{
	mJointsScroll->clearRows();
	S32 i = 0;
	for (;; i++)
	{
		LLJoint* joint = gAgentAvatarp->getCharacterJoint(i);
		if (joint)
		{
			LLVector3 vec3;
			LLSD row;
			std::string format = llformat("%%.%df", 3);

			//BD - When posing get the target values otherwise we end up getting the in-interpolation values.
			if (gAgent.getPosing())
			{
				joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
			}
			else
			{
				joint->getRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
			}
			row["columns"][0]["column"] = "joint";
			row["columns"][0]["value"] = joint->getName();
			row["columns"][1]["column"] = "x";
			row["columns"][1]["value"] = llformat(format.c_str(), vec3.mV[VX]);
			row["columns"][2]["column"] = "y";
			row["columns"][2]["value"] = llformat(format.c_str(), vec3.mV[VY]);
			row["columns"][3]["column"] = "z";
			row["columns"][3]["value"] = llformat(format.c_str(), vec3.mV[VZ]);

			//BD - Special case for mPelvis as it has position information too.
			if (joint->getName() == "mPelvis")
			{
				if (gAgent.getPosing())
				{
					vec3 = joint->getTargetPosition();
				}
				else
				{
					vec3 = joint->getPosition();
				}
				row["columns"][4]["column"] = "pos_x";
				row["columns"][4]["value"] = llformat(format.c_str(), vec3.mV[VX]);
				row["columns"][5]["column"] = "pos_y";
				row["columns"][5]["value"] = llformat(format.c_str(), vec3.mV[VY]);
				row["columns"][6]["column"] = "pos_z";
				row["columns"][6]["value"] = llformat(format.c_str(), vec3.mV[VZ]);
			}

			LLScrollListItem* item = mJointsScroll->addElement(row);
			item->setUserdata(joint);

			LLMotion* motion = gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						((LLScrollListText*)item->getColumn(0))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
		else
		{
			break;
		}
	}
	onJointControlsRefresh();
}

void BDFloaterAnimations::onJointControlsRefresh()
{
	bool is_pelvis = false;
	bool is_posing = gAgent.getPosing();
	LLScrollListItem* item = mJointsScroll->getFirstSelected();
	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		getChild<LLSliderCtrl>("Rotation_X")->setValue(item->getColumn(1)->getValue());
		getChild<LLSliderCtrl>("Rotation_Y")->setValue(item->getColumn(2)->getValue());
		getChild<LLSliderCtrl>("Rotation_Z")->setValue(item->getColumn(3)->getValue());
		if (joint && joint->getName() == "mPelvis")
		{
			is_pelvis = true;
			getChild<LLSliderCtrl>("Position_X")->setValue(item->getColumn(4)->getValue());
			getChild<LLSliderCtrl>("Position_Y")->setValue(item->getColumn(5)->getValue());
			getChild<LLSliderCtrl>("Position_Z")->setValue(item->getColumn(6)->getValue());
		}

		BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
		if (motion)
		{
			LLPose* pose = motion->getPose();
			if (pose)
			{
				LLPointer<LLJointState> joint_state = pose->findJointState(joint);
				getChild<LLButton>("toggle_bone")->setValue(joint_state.notNull());
			}
		}
	}

	getChild<LLButton>("activate")->setValue(gAgent.getPosing());
	getChild<LLSliderCtrl>("Rotation_X")->setEnabled(item && is_posing);
	getChild<LLSliderCtrl>("Rotation_Y")->setEnabled(item && is_posing);
	getChild<LLSliderCtrl>("Rotation_Z")->setEnabled(item && is_posing);
	getChild<LLSliderCtrl>("Position_X")->setEnabled(is_pelvis && is_posing);
	getChild<LLSliderCtrl>("Position_Y")->setEnabled(is_pelvis && is_posing);
	getChild<LLSliderCtrl>("Position_Z")->setEnabled(is_pelvis && is_posing);
}

void BDFloaterAnimations::onJointSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLJoint* joint = (LLJoint*)mJointsScroll->getFirstSelected()->getUserdata();
	if (joint)
	{
		F32 val = ctrl->getValue().asReal();
		LLVector3 vec3;
		LLQuaternion quat;
		LLScrollListCell* column_1 = mJointsScroll->getFirstSelected()->getColumn(1);
		LLScrollListCell* column_2 = mJointsScroll->getFirstSelected()->getColumn(2);
		LLScrollListCell* column_3 = mJointsScroll->getFirstSelected()->getColumn(3);

		vec3.mV[VX] = column_1->getValue().asReal();
		vec3.mV[VY] = column_2->getValue().asReal();
		vec3.mV[VZ] = column_3->getValue().asReal();

		if (param.asString() == "x")
		{
			vec3.mV[VX] = val;
			column_1->setValue(ll_round(vec3.mV[VX], 0.001f));
		}
		else if (param.asString() == "y")
		{
			vec3.mV[VY] = val;
			column_2->setValue(ll_round(vec3.mV[VY], 0.001f));
		}
		else
		{
			vec3.mV[VZ] = val;
			column_3->setValue(ll_round(vec3.mV[VZ], 0.001f));
		}

		//BD - While editing rotations, make sure we use a bit of linear interpolation to make movements smoother.
		LLMotion* motion = gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
		if (motion)
		{
			//BD - If we don't use our default, set it once.
			if (motion->getInterpolationTime() != 0.2
				|| motion->getInterpolationType() != 2)
			{
				motion->setInterpolationTime(0.2f);
				motion->setInterpolationType(2);
			}
		}
		llassert(!vec3.isFinite());
		quat.setEulerAngles(vec3.mV[VX], vec3.mV[VZ], vec3.mV[VY]);
		joint->setTargetRotation(quat);
	}
}

void BDFloaterAnimations::onJointPosSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLJoint* joint = (LLJoint*)mJointsScroll->getFirstSelected()->getUserdata();
	if (joint && joint->getName() == "mPelvis")
	{
		F32 val = ctrl->getValue().asReal();
		LLVector3 vec3;
		LLScrollListCell* column_4 = mJointsScroll->getFirstSelected()->getColumn(4);
		LLScrollListCell* column_5 = mJointsScroll->getFirstSelected()->getColumn(5);
		LLScrollListCell* column_6 = mJointsScroll->getFirstSelected()->getColumn(6);

		vec3.mV[VX] = column_4->getValue().asReal();
		vec3.mV[VY] = column_5->getValue().asReal();
		vec3.mV[VZ] = column_6->getValue().asReal();

		//BD - Really?
		std::string format = llformat("%%.%df", 3);
		if (param.asString() == "x")
		{
			vec3.mV[VX] = val;
			column_4->setValue(llformat(format.c_str(), vec3.mV[VX]));
		}
		else if (param.asString() == "y")
		{
			vec3.mV[VY] = val;
			column_5->setValue(llformat(format.c_str(), vec3.mV[VY]));
		}
		else
		{
			vec3.mV[VZ] = val;
			column_6->setValue(llformat(format.c_str(), vec3.mV[VZ]));
		}
		llassert(!vec3.isFinite());
		joint->setTargetPosition(vec3);
	}
}

void BDFloaterAnimations::onJointChangeState()
{
	LLJoint* joint = (LLJoint*)mJointsScroll->getFirstSelected()->getUserdata();
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
					((LLScrollListText*)mJointsScroll->getFirstSelected()->getColumn(0))->setFontStyle(LLFontGL::NORMAL);
				}
				else
				{
					motion->addJointToState(joint);
					((LLScrollListText*)mJointsScroll->getFirstSelected()->getColumn(0))->setFontStyle(LLFontGL::BOLD);
				}
			}
		}
	}
	onJointControlsRefresh();
}


////////////////////////////////
//BD - Animations
////////////////////////////////
void BDFloaterAnimations::onAnimAdd()
{
	//BD - This is going to be a really dirty way of inserting elements inbetween others
	//     until i can figure out a better way.
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 index = 0;
	std::vector<LLScrollListItem*> old_items = mAnimEditorScroll->getAllData();
	std::vector<LLScrollListItem*> new_items;
	LLScrollListItem* new_item;
	LLSD row;

	LLSD choice = getChild<LLComboBox>("anim_choice_combo")->getValue();
	F32 time = getChild<LLLineEditor>("anim_time")->getValue().asReal();
	if (choice.asString() == "Repeat")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Repeat";
	}
	else if (choice.asString() == "Wait")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Wait";
		row["columns"][1]["column"] = "time";
		row["columns"][1]["value"] = time;
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
	}

	new_item = mAnimEditorScroll->addElement(row);

	//BD - Let's go through all entries in the list and copy them into a new
	//     list in our new desired order, flag the old ones for removal while
	//     we do this.
	for (std::vector<LLScrollListItem*>::iterator it = old_items.begin();
		it != old_items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			item->setFlagged(true);
			new_items.push_back(item);
		}

		if (index == selected_index)
		{
			new_item->setFlagged(true);
			new_items.push_back(new_item);
		}
		index++;
	}

	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (std::vector<LLScrollListItem*>::iterator it = new_items.begin();
		it != new_items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
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
				mAnimEditorScroll->addElement(row);
			}
		}
	}

	//BD - Delete all flagged items now and we'll end up with a new list order.
	mAnimEditorScroll->deleteFlaggedItems();

	//BD - Select the last selected entry and make it appear as nothing happened.
	if (selected_index >= 0)
	{
		mAnimEditorScroll->selectNthItem(selected_index);
	}
}

void BDFloaterAnimations::onAnimDelete()
{
	mAnimEditorScroll->deleteSelectedItems();
}

void BDFloaterAnimations::onAnimSave()
{

}

void BDFloaterAnimations::onAnimSet()
{
	F32 value = getChild<LLUICtrl>("anim_time")->getValue().asReal();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	LLScrollListCell* column = item->getColumn(1);
	column->setValue(value);
}

void BDFloaterAnimations::onAnimPlay()
{
	mExpiryTime = 0.0f;
	mAnimPlayTimer.start();
}

void BDFloaterAnimations::onAnimStop()
{
	mAnimPlayTimer.stop();
}