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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llviewerprecompiledheaders.h"

#include "bdposingmotion.h"
#include "llcharacter.h"
#include "llviewercontrol.h"

//-----------------------------------------------------------------------------
// BDPosingMotion()
// Class Constructor
//-----------------------------------------------------------------------------
BDPosingMotion::BDPosingMotion(const LLUUID &id) :
LLMotion(id),
	mCharacter(NULL),
	mTargetJoint(NULL)
{
	mName = "custom_pose";
	//BD - Use slight spherical linear interpolation by default.
	mInterpolationTime = 0.25f;
	mInterpolationType = 2;

	for (auto& entry : mJointState)
	{
		entry = new LLJointState;
	}
}


//-----------------------------------------------------------------------------
// ~BDPosingMotion()
// Class Destructor
//-----------------------------------------------------------------------------
BDPosingMotion::~BDPosingMotion()
{
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus BDPosingMotion::onInitialize(LLCharacter *character)
{
	// save character for future use
	mCharacter = character;

	for (S32 i = 0; (mTargetJoint = mCharacter->getCharacterJoint(i)); ++i)
	{
		mJointState[i]->setJoint(mTargetJoint);
		//BD - Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (mTargetJoint->mHasPosition)
		{
			mJointState[i]->setUsage(LLJointState::POS | LLJointState::ROT);
		}
		else
		{
			mJointState[i]->setUsage(LLJointState::ROT);
		}
		addJointState(mJointState[i]);
	}

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL BDPosingMotion::onActivate()
{
	for (auto joint_state : mJointState)
	{
		LLJoint* joint = joint_state->getJoint();
		if (joint)
		{
			joint->setTargetRotation(joint->getRotation());
			//BD - Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			if (joint->mHasPosition)
			{
				joint->setTargetPosition(joint->getPosition());
			}
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL BDPosingMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLQuaternion target_quat;
	LLQuaternion joint_quat;
	LLQuaternion last_quat;
	LLVector3 joint_pos;
	LLVector3 last_pos;
	LLVector3 target_pos;
	F32 perc = 0.0f;

	for (auto joint_state : mJointState)
	{
		LLJoint* joint = joint_state->getJoint();
		if (joint)
		{
			target_quat = joint->getTargetRotation();
			joint_quat = joint->getRotation();
			last_quat = joint->getLastRotation();

			//BD - Merge these two together?
			perc = llclamp(mInterpolationTimer.getElapsedTimeF32() / mInterpolationTime, 0.0f, 1.0f);
			//BD - Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			if (joint->mHasPosition)
			{
				joint_pos = joint->getPosition();
				target_pos = joint->getTargetPosition();
				last_pos = joint->getLastPosition();
				if (target_pos != joint_pos)
				{
					if (mInterpolationType == 2)
					{
						//BD - Do spherical linear interpolation.
						//     We emulate the spherical linear interpolation here because
						//     slerp() does not support LLVector3. mInterpolationTime is always
						//     in a range between 0.00 and 1.00 which makes it perfect to use
						//     as percentage directly.
						//     We use the current joint position rather than the original like
						//     in linear interpolation to take a fraction of the fraction, this
						//     re-creates spherical linear interpolation's behavior.
						joint_pos = lerp(joint_pos, target_pos, mInterpolationTime);
					}
					else
					{
						if (perc >= 1.0f)
						{
							//BD - Can be used to do no interpolation too.
							joint_pos = target_pos;
							last_pos = joint_pos;
						}
						else
						{
							//BD - Do linear interpolation.
							joint_pos = lerp(last_pos, target_pos, perc);
						}
					}
					joint_state->setPosition(joint_pos);
				}
			}

			if (target_quat != joint_quat)
			{
				if (mInterpolationType == 2)
				{
					//BD - Do spherical linear interpolation.
					joint_quat = slerp(mInterpolationTime, joint_quat, target_quat);
				}
				else
				{
					if (perc >= 1.0f)
					{
						//BD - Can be used to do no interpolation too.
						joint_quat = target_quat;
						last_quat = joint_quat;
					}
					else
					{
						//BD - Do linear interpolation.
						joint_quat = lerp(perc, last_quat, target_quat);
					}
				}
				joint_state->setRotation(joint_quat);
			}
		}
	}

	if (perc >= 1.0f && mInterpolationTimer.getStarted()
		&& (last_quat == joint_quat
		&& (last_pos == joint_pos)))
	{
		mInterpolationTimer.stop();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onDeactivate()
//-----------------------------------------------------------------------------
void BDPosingMotion::onDeactivate()
{
}

//-----------------------------------------------------------------------------
// addJointState()
//-----------------------------------------------------------------------------
void BDPosingMotion::addJointToState(LLJoint *joint)
{
	//BD - It is safe to assume that whatever is passed here must be at least some kind of valid
	//     joint, all code calling this function has null checks in place.
	S32 i = joint->getJointNum();

	//BD - Don't add collision volumes and attachment bones.
	if (i >= 134)
		return;

	mJointState[i]->setJoint(joint);
	//BD - Bones that can support position
	//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
	if (joint->mHasPosition)
	{
		mJointState[i]->setUsage(LLJointState::POS | LLJointState::ROT);
	}
	else
	{
		mJointState[i]->setUsage(LLJointState::ROT);
	}
	addJointState(mJointState[i]);
}
// End

