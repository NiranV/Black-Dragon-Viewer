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

#include "lluictrlfactory.h"
#include "llagent.h"
#include "lldiriterator.h"
#include "llkeyframemotion.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerjointattachment.h"
#include "llviewerjoint.h"
#include "llvoavatarself.h"

#include "bdfloaterposer.h"
#include "bdanimator.h"
#include "bdposingmotion.h"

BDAnimator gDragonAnimator;


BDAnimator::BDAnimator() :
	mPlaying(false),
	mExpiryTime(0.0f),
	mCurrentAction(0)
{
}

BDAnimator::~BDAnimator()
{
}

void BDAnimator::update()
{
	if (!mPlaying)
	{
		return;
	}

	if (mAnimatorActions.empty())
	{
		mAnimPlayTimer.stop();
		return;
	}


	if (mAnimPlayTimer.getStarted() &&
		mAnimPlayTimer.getElapsedTimeF32() > mExpiryTime)
	{
		Action action = mAnimatorActions[mCurrentAction];
		//BD - Stop the timer, we're going to reconfigure and restart it when we're done.
		mAnimPlayTimer.stop();

		EActionType type = action.mType;
		std::string name = action.mPoseName;

		//BD - We can't use Wait or Restart as label, need to fix this.
		if (type == WAIT)
		{
			//BD - Do nothing?
			mExpiryTime = action.mTime;
			++mCurrentAction;
		}
		else if (type == REPEAT)
		{
			mCurrentAction = 0;
			mExpiryTime = 0.0f;
		}
		else
		{
			mExpiryTime = 0.0f;
			loadPose(name);
			++mCurrentAction;
		}

		//BD - As long as we are not at the end, start the timer again, automatically
		//     resetting the counter in the process.
		if (mAnimatorActions.size() != mCurrentAction)
		{
			mAnimPlayTimer.start();
		}
	}
}

void BDAnimator::onAddAction(LLScrollListItem* item, S32 location)
{
	if (item)
	{
		Action action;
		S32 type = item->getColumn(2)->getValue().asInteger();
		if (type == 0)
		{
			action.mType = WAIT;
		}
		else if (type == 1)
		{
			action.mType = REPEAT;
		}
		else
		{
			action.mType = POSE;
		}
		action.mPoseName = item->getColumn(0)->getValue().asString();
		action.mTime = item->getColumn(1)->getValue().asReal();
		mAnimatorActions.push_back(action);
	}
}

void BDAnimator::onAddAction(std::string name, EActionType type, F32 time, S32 location)
{
	Action action;
	action.mType = type;
	action.mPoseName = name;
	action.mTime = time;
	if (mAnimatorActions.size() == 0)
	{
		mAnimatorActions.push_back(action);
	}
	else
	{
		if (location <= mAnimatorActions.size())
		{
			mAnimatorActions.insert(mAnimatorActions.begin() + location, action);
		}
	}
}

void BDAnimator::onAddAction(Action action, S32 location)
{
	mAnimatorActions.insert(mAnimatorActions.begin() + location, action);
}


void BDAnimator::onDeleteAction(S32 location)
{
	if (!mPlaying)
	{
		mAnimatorActions.erase(mAnimatorActions.begin() + location);
	}
}

void BDAnimator::startPlayback()
{
	mAnimPlayTimer.start();
	mExpiryTime = 0.0f;
	mCurrentAction = 0;
	mPlaying = true;
}

void BDAnimator::stopPlayback()
{
	mAnimPlayTimer.stop();
	mPlaying = false;
}

BOOL BDAnimator::loadPose(const LLSD& name)
{
	if (!mTargetAvatar || mTargetAvatar->isDead())
	{
		LL_WARNS("Posing") << "Couldn't find avatar, dead?" << LL_ENDL;
		return FALSE;
	}

	std::string filename;
	if (!name.asString().empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, BDFloaterPoser::escapeString(name.asString()) + ".xml");
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
		BDPosingMotion* motion = (BDPosingMotion*)mTargetAvatar->findMotion(ANIM_BD_POSING_MOTION);
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

		LLJoint* joint = mTargetAvatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			//BD - Don't try to add/remove joint states for anything but our default bones.
			if (motion && joint->getJointNum() < 134)
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
			if (pose["rotation"].isDefined())
			{
				LLQuaternion quat;
				LLQuaternion new_quat = joint->getRotation();

				joint->setLastRotation(new_quat);
				vec3.setValue(pose["rotation"]);
				quat.setEulerAngles(vec3.mV[VX], vec3.mV[VZ], vec3.mV[VY]);
				joint->setTargetRotation(quat);
			}

			//BD - Position information is only ever written when it is actually safe to do.
			//     It's safe to assume that IF information is available it's safe to apply.
			if (pose["position"].isDefined())
			{
				vec3.setValue(pose["position"]);
				joint->setLastPosition(joint->getPosition());
				joint->setTargetPosition(vec3);
			}

			//BD - Bone Scales
			if (pose["scale"].isDefined())
			{
				vec3.setValue(pose["scale"]);
				joint->setScale(vec3);
			}
		}
	}
	infile.close();
	return TRUE;
}