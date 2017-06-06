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

// viewer includes
#include "llfloaterpreference.h"


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

	//BD - WIP
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterAnimations::onPoseSave, this));
	//BD - Load the current pose and export all its values into the UI so we can alter them.
	mCommitCallbackRegistrar.add("Pose.Copy", boost::bind(&BDFloaterAnimations::onPoseSave, this));
	//BD - Start our custom pose.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterAnimations::onPoseLoad, this));

//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX", boost::bind(&LLFloaterPreference::onCommitX, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY", boost::bind(&LLFloaterPreference::onCommitY, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ", boost::bind(&LLFloaterPreference::onCommitZ, _1, _2));

	mCommitCallbackRegistrar.add("Pose.Set", boost::bind(&BDFloaterAnimations::onPoseSet, this, _1, _2));

//	//BD - Revert to Default
	mCommitCallbackRegistrar.add("Pref.Default", boost::bind(&BDFloaterAnimations::resetToDefault, this, _1));
}

BDFloaterAnimations::~BDFloaterAnimations()
{
}

BOOL BDFloaterAnimations::postBuild()
{
	mAvatarScroll = getChild<LLScrollListCtrl>("other_avatars_scroll");
	mMotionScroll = getChild<LLScrollListCtrl>("motions_scroll");

	//BD - Posing
	//mPoseScroll = getChild<LLScrollListCtrl>("poses_scroll");
	mJointsScroll = this->getChild<LLScrollListCtrl>("joints_scroll", true);
	mJointsScroll->setDoubleClickCallback(boost::bind(&BDFloaterAnimations::onBonesClick, this));
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

//BD - Revert to Default
void BDFloaterAnimations::resetToDefault(LLUICtrl* ctrl)
{
	ctrl->getControlVariable()->resetToDefault(true);
	onRefreshPoseControls();
}

void BDFloaterAnimations::onOpen(const LLSD& key)
{
	onRefresh();
	onRefreshPoseControls();
	onBoneRefresh();
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

void BDFloaterAnimations::onRefreshPoseControls()
{
	LLJoint* joint = gAgentAvatarp->getJoint(mTargetName);
	if (!mTargetName.empty())
	{
		getChild<LLSliderCtrl>("Rotation_X")->setValue(joint->getRotation().packToVector3().mV[VX]);
		getChild<LLSliderCtrl>("Rotation_Y")->setValue(joint->getRotation().packToVector3().mV[VY]);
		getChild<LLSliderCtrl>("Rotation_Z")->setValue(joint->getRotation().packToVector3().mV[VZ]);
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
	/*getSelected();
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

			if (character->getID() == gAgentID)
			{*/
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
	//		}
	//	}
	//}
}

void BDFloaterAnimations::onBoneRefresh()
{
	mJointsScroll->clearRows();
	S32 i = 0;
	for (;;i++)
	{
		LLJoint* joint = gAgentAvatarp->getCharacterJoint(i);
		if (joint)
		{
			LLVector3 vec3;
			if (!joint->getRotation().isFinite())
			{
				vec3.zeroVec();
				LL_INFOS() << "Not finite, defaulting to: " << vec3 << LL_ENDL;
			}
			else
			{
				vec3 = joint->getRotation().packToVector3();
				LL_INFOS() << "Finite, take: " << vec3 << LL_ENDL;
			}
			LLSD row;
			row["columns"][0]["column"] = "joint";
			row["columns"][0]["value"] = joint->getName();
			row["columns"][1]["column"] = "x";
			row["columns"][1]["value"] = vec3.mV[VX];
			row["columns"][2]["column"] = "y";
			row["columns"][2]["value"] = vec3.mV[VY];
			row["columns"][3]["column"] = "z";
			row["columns"][3]["value"] = vec3.mV[VZ];
			mJointsScroll->addElement(row);
		}
		else
		{
			break;
		}
	}
}

void BDFloaterAnimations::onPoseSave()
{
	mJointsScroll->selectAll();
	LLMotion* motion = gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		LLPose* pose = motion->getPose();
		while (true)
		{
			LLJointState* joint_state = pose->getNextJointState();
			if (joint_state == NULL)
			{
				break;
			}

			LLJoint* joint = joint_state->getJoint();
			std::vector<LLScrollListItem*> items = mJointsScroll->getAllSelected();
			for (std::vector<LLScrollListItem*>::iterator item = items.begin();
				item != items.end(); ++item)
			{
				if ((*item)->getColumn(0)->getValue().asString() == joint->getName())
				{
					LLScrollListCell* cell = (*item)->getColumn(1);
					cell->setValue(joint->getRotation().packToVector3().getValue());
				}
			}
		}
	}
	mJointsScroll->deselectAllItems();
	onBoneRefresh();
}

void BDFloaterAnimations::onPoseCopy()
{
	LLMotionController::motion_list_t motions = gAgentAvatarp->getMotionController().getActiveMotions();
	for (LLMotionController::motion_list_t::iterator it = motions.begin();
		it != motions.end(); ++it)
	{
		LLMotion* motion = *it;
		if (motion)
		{
			LLPose* pose = motion->getPose();
			while (pose->getNextJointState() != NULL)
			{
				//BD - WIP.
			}
		}
	}
}

void BDFloaterAnimations::onPoseLoad()
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

void BDFloaterAnimations::onPoseSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLJoint* joint = gAgentAvatarp->getJoint(mTargetName);
	if (joint)
	{
		F32 val = ctrl->getValue().asReal();
		LLVector3 vec3 = joint->getRotation().packToVector3();
		LLQuaternion quat;

		if (param.asString() == "x")
		{
			vec3.mV[VX] = val;
		}
		else if (param.asString() == "y")
		{
			vec3.mV[VY] = val;
		}
		else
		{
			vec3.mV[VZ] = val;
		}

		quat.unpackFromVector3(vec3);
		joint->setRotation(quat);

		mJointsScroll->getFirstSelected()->getColumn(0)->getValue();
		LLScrollListCell* column_1 = mJointsScroll->getFirstSelected()->getColumn(1);
		LLScrollListCell* column_2 = mJointsScroll->getFirstSelected()->getColumn(2);
		LLScrollListCell* column_3 = mJointsScroll->getFirstSelected()->getColumn(3);
		column_1->setValue(vec3.mV[VX]);
		column_2->setValue(vec3.mV[VZ]);
		column_3->setValue(vec3.mV[VY]);
	}
}

void BDFloaterAnimations::onBonesClick()
{
	mTargetName = mJointsScroll->getFirstSelected()->getColumn(0)->getValue();
	getChild<LLSliderCtrl>("Rotation_X")->setEnabled(true);
	getChild<LLSliderCtrl>("Rotation_X")->setValue(mJointsScroll->getFirstSelected()->getColumn(1)->getValue());
	getChild<LLSliderCtrl>("Rotation_Y")->setEnabled(true);
	getChild<LLSliderCtrl>("Rotation_Y")->setValue(mJointsScroll->getFirstSelected()->getColumn(2)->getValue());
	getChild<LLSliderCtrl>("Rotation_Z")->setEnabled(true);
	getChild<LLSliderCtrl>("Rotation_Z")->setValue(mJointsScroll->getFirstSelected()->getColumn(3)->getValue());
}