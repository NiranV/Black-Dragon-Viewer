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
#include "llcallingcard.h"


BDFloaterAnimations::BDFloaterAnimations(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Stops an animation complete, essentially just reset animation.
	mCommitCallbackRegistrar.add("Anim.Stop", boost::bind(&BDFloaterAnimations::onAnimStop, this));
	//BD - Pauses an animation just like if we set its playback speed to 0.0.
	mCommitCallbackRegistrar.add("Anim.Freeze", boost::bind(&BDFloaterAnimations::onAnimFreeze, this));
	//BD - Changes the animation playback speed to the desired value.
	mCommitCallbackRegistrar.add("Anim.Set", boost::bind(&BDFloaterAnimations::onAnimSet, this));
	//BD - Restarts the animation, could be used as animation sync.
	mCommitCallbackRegistrar.add("Anim.Restart", boost::bind(&BDFloaterAnimations::onAnimRestart, this));
	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Anim.Refresh", boost::bind(&BDFloaterAnimations::onRefresh, this));

	//BD - Save a motion to be copied to other avatars.
	mCommitCallbackRegistrar.add("Anim.Save", boost::bind(&BDFloaterAnimations::onSave, this));
	//BD - Copy a motion to a selected avatar and make it play it.
	mCommitCallbackRegistrar.add("Anim.Load", boost::bind(&BDFloaterAnimations::onLoad, this));
	//BD - Remove all selected motions from our list.
	mCommitCallbackRegistrar.add("Anim.Remove", boost::bind(&BDFloaterAnimations::onRemove, this));
	//BD - Create a new motion via a given UUID.
	mCommitCallbackRegistrar.add("Anim.Create", boost::bind(&BDFloaterAnimations::onCreate, this));

	//BD - Save the current pose to our list.
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterAnimations::onPoseSave, this));
	//BD - Load the current pose onto the currently selected avatars.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterAnimations::onPoseLoad, this));
}

BDFloaterAnimations::~BDFloaterAnimations()
{
}

BOOL BDFloaterAnimations::postBuild()
{
	mAvatarScroll = getChild<LLScrollListCtrl>("other_avatars_scroll");
	mMotionScroll = getChild<LLScrollListCtrl>("motions_scroll");
	mPoseScroll = getChild<LLScrollListCtrl>("poses_scroll");
	mJointsScroll = getChild<LLScrollListCtrl>("joints_scroll");
	return TRUE;
}

void BDFloaterAnimations::draw()
{
	//BD - Maybe use this instead.
	/*S32 selected = mAvatarScroll->getAllSelected().size();
	if (selected != mSelectedAmount)
	{
		mSelectedAmount = selected;
		getSelected();
	}*/

	LLFloater::draw();
}

void BDFloaterAnimations::onOpen(const LLSD& key)
{
	onRefresh();
}

void BDFloaterAnimations::onClose(bool app_quitting)
{
	if (app_quitting)
	{
		mAvatarScroll->clearRows();
		mMotionScroll->clearRows();
	}
	mSelectedCharacters.clear();
	mSelectedAmount = 0;
}

void BDFloaterAnimations::onRefresh()
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
			item->setFlagged(TRUE);
			for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
				iter != LLCharacter::sInstances.end(); ++iter)
			{
				LLCharacter* character = (*iter);
				if (character)
				{
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

					if (item->getColumn(1)->getValue().asUUID() == character->getID())
					{
						item->setFlagged(FALSE);

						F32 value = character->getMotionController().getTimeFactor();
						item->getColumn(2)->setValue(value);
						break;
					}
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
			mAvatarScroll->addElement(row);
		}
	}
}

void BDFloaterAnimations::getSelected()
{
	mSelectedCharacters.clear();
	std::vector<LLScrollListItem*> items = mAvatarScroll->getAllSelected();
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		for (std::vector<LLScrollListItem*>::iterator item = items.begin();
			item != items.end(); ++item)
		{
			if ((*item)->getColumn(1)->getValue().asString() == (*iter)->getID().asString())
			{
				mSelectedCharacters.push_back(*iter);
				break;
			}
		}
	}
}

//BD - Combine all below to one big onCommand function?
void BDFloaterAnimations::onAnimStop()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = *iter;
		if (character)
		{
			character->deactivateAllMotions();
		}
	}
}

void BDFloaterAnimations::onAnimFreeze()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = (*iter);
		if (character)
		{
			if (character->areAnimationsPaused())
			{
				mAvatarPauseHandles.clear();
			}
			else
			{
				mAvatarPauseHandles.push_back(character->requestPause());
			}
		}
	}
}

void BDFloaterAnimations::onAnimSet()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = *iter;
		if (character)
		{
			character->getMotionController().setTimeFactor(getChild<LLUICtrl>("anim_factor")->getValue().asReal());
		}
	}
	onRefresh();
}

void BDFloaterAnimations::onAnimRestart()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = *iter;
		if (character)
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
	}
}

void BDFloaterAnimations::onSave()
{
	LLAvatarTracker& at = LLAvatarTracker::instance();
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = (*iter);
		if (character)
		{
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
	}
}

void BDFloaterAnimations::onLoad()
{
	//LLAvatarTracker& at = LLAvatarTracker::instance();
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = (*iter);
		if (character)
		{
			//BD - Only allow animating people whose object mod rights we have.
			//const LLRelationship* relation = at.getBuddyInfo(character->getID());
			//if (relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS)
			//	|| character->getID() == gAgentID)

			//BD - Only allow animating ourselves, as per Oz Linden.
			if (character->getID() == gAgentID)
			{
				LLScrollListItem* item = mMotionScroll->getFirstSelected();
				if (item)
				{
					LLUUID motion_id = item->getColumn(1)->getValue().asUUID();
					if (!motion_id.isNull())
					{
						character->createMotion(motion_id);
						character->stopMotion(motion_id, 1);
						character->startMotion(motion_id, 0.0f);
					}
				}
			}
		}
	}
}

void BDFloaterAnimations::onPoseSave()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = *iter;
		if (character)
		{
			LLMotionController::motion_list_t motions = character->getMotionController().getActiveMotions();
			for (LLMotionController::motion_list_t::iterator it = motions.begin();
				it != motions.end(); ++it)
			{
				LLMotion* motion = *it;
				if (motion)
				{
					LLPose* pose = motion->getPose();
					while (pose->getNextJointState() != NULL)
					{
						LLJointState* joint_state = pose->getFirstJointState();
						LLJoint* joint = joint_state->getJoint();
						LLSD row;
						row["columns"][0]["column"] = "joint";
						row["columns"][0]["value"] = joint->getName();
						row["columns"][1]["column"] = "pos";
						row["columns"][1]["value"] = joint->getPosition().getValue();
						row["columns"][2]["column"] = "rot";
						row["columns"][2]["value"] = joint->getRotation().packToVector3().getValue();
						row["columns"][3]["column"] = "scale";
						row["columns"][3]["value"] = joint->getScale().getValue();
						row["columns"][4]["column"] = "parent";
						row["columns"][4]["value"] = joint->getParent()->getName();
						row["columns"][5]["column"] = "skin";
						row["columns"][5]["value"] = joint->getSkinOffset().getValue();
						row["columns"][6]["column"] = "support";
						row["columns"][6]["value"] = joint->getSupport();
						//row["columns"][5]["column"] = "x";
						//row["columns"][5]["value"] = joint->getSkinOffset().mV[VX];
						//row["columns"][6]["column"] = "y";
						//row["columns"][6]["value"] = joint->getSkinOffset().mV[VY];
						//row["columns"][7]["column"] = "z";
						//row["columns"][7]["value"] = joint->getSkinOffset().mV[VZ];
						mJointsScroll->addElement(row);
					}
				}
			}
		}
	}
}

void BDFloaterAnimations::onPoseLoad()
{
	getSelected();
	for (std::vector<LLCharacter*>::iterator iter = mSelectedCharacters.begin();
		iter != mSelectedCharacters.end(); ++iter)
	{
		LLCharacter* character = *iter;
		if (character)
		{
			LLScrollListItem* joint_item = mJointsScroll->getFirstSelected();
			if (joint_item)
			{
				
				LLMotion* motion = character->getMotionController().findMotion(joint_item->getColumn(1)->getValue().asUUID());
				//LLMotionController* motion_cstr;
				LLPose* pose = motion->getPose();
				LLJointState* joint_state = pose->findJointState(joint_item->getColumn(0)->getValue());
				LLJoint* joint = joint_state->getJoint();
				//LLJoint* parent;
				LLVector3 vec3;
				LLQuaternion quaternion;

				//BD - Name & Parent
				//joint->setup(joint_item->getColumn(0)->getValue().asString());
				//joint->setName(item->getColumn(0)->getValue().asString());
				//BD - Position
				vec3.setValue(joint_item->getColumn(1)->getValue());
				joint->setPosition(vec3);
				//BD - Rotation
				vec3.setValue(joint_item->getColumn(2)->getValue());
				quaternion.unpackFromVector3(vec3);
				joint->setRotation(quaternion);
				//BD - Scale
				vec3.setValue(joint_item->getColumn(3)->getValue());
				joint->setScale(vec3);
				//BD - Skin Offset
				vec3.setValue(joint_item->getColumn(5)->getValue());
				joint->setSkinOffset(vec3);
				//BD - Support
				joint->setSupport(joint_item->getColumn(6)->getValue());

				//joint_state->setJoint(joint);
				//pose->addJointState(joint_state);
				//motion->setPose(*pose);
				//character->getJoint();
				//motion->addJointState(joint_state);
				//character->
				//motion_cstr->

				/*LLUUID motion_id = motion->getID();;
				if (!motion_id.isNull())
				{
					character->createMotion(motion_id);
					character->stopMotion(motion_id, 1);
					character->startMotion(motion_id, 0.0f);
				}*/
			}
		}
	}
}

void BDFloaterAnimations::onCreate()
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

void BDFloaterAnimations::onRemove()
{
	mMotionScroll->deleteSelectedItems();
}