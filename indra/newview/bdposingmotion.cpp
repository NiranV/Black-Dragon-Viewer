/** 
 * @file BDPosingMotion.cpp
 * @brief Implementation of BDPosingMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llviewerprecompiledheaders.h"

#include "bdposingmotion.h"

#include "llcharacter.h"
#include "llhandmotion.h"

//BD
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
	//BD - Use slight linear interpolation by default.
	mInterpolationTime = 0.15f;
	mInterpolationType = 1;

	for (S32 i = 0; i <= 133; i++)
	{
		mJointState[i] = new LLJointState;
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

	for (S32 i = 0;;i++)
	{
		mTargetJoint = mCharacter->getCharacterJoint(i);
		if (mTargetJoint)
		{
			mJointState[i]->setJoint(mTargetJoint);
			//BD - Special case for pelvis as we're going to rotate and reposition it.
			if (mTargetJoint->getName() == "mPelvis")
			{
				mJointState[i]->setUsage(LLJointState::POS | LLJointState::ROT);
			}
			else
			{
				mJointState[i]->setUsage(LLJointState::ROT);
			}
			addJointState(mJointState[i]);
		}
		else
		{
			break;
		}
	}

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL BDPosingMotion::onActivate()
{
	S32 i = 0;
	for (;; i++)
	{
		if (mJointState[i].notNull())
		{
			mJointState[i]->getJoint()->setTargetRotation(mJointState[i]->getJoint()->getRotation());
			if (mJointState[i]->getJoint()->getName() == "mPelvis")
			{
				mJointState[i]->getJoint()->setTargetPosition(mJointState[i]->getJoint()->getPosition());
			}
		}
		else
		{
			break;
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

	S32 i = 0;
	for (;;i++)
	{
		if (mJointState[i].notNull())
		{
			target_quat = mJointState[i]->getJoint()->getTargetRotation();
			joint_quat = mJointState[i]->getJoint()->getRotation();
			last_quat = mJointState[i]->getJoint()->getLastRotation();
			bool is_pelvis = (mJointState[i]->getJoint()->getName() == "mPelvis");

			//BD - Merge these two together?
			perc = llclamp(mInterpolationTimer.getElapsedTimeF32() / mInterpolationTime, 0.0f, 1.0f);
			if (is_pelvis)
			{
				joint_pos = mJointState[i]->getJoint()->getPosition();
				target_pos = mJointState[i]->getJoint()->getTargetPosition();
				last_pos = mJointState[i]->getJoint()->getLastPosition();
				if (target_pos != joint_pos)
				{
					if (mInterpolationType == 2)
					{
						//BD - Do spherical linear interpolation.
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

					llassert(joint_pos.isFinite());
					mJointState[i]->setPosition(joint_pos);
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

				llassert(joint_quat.isFinite());
				mJointState[i]->setRotation(joint_quat);
			}
		}
		else
		{
			break;
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
	for (S32 i = 0;; i++)
	{
		mTargetJoint = mCharacter->getCharacterJoint(i);
		if (mTargetJoint)
		{
			if (mTargetJoint->getName() == joint->getName())
			{
				mJointState[i]->setJoint(mTargetJoint);
				//BD - Special case for pelvis as we're going to rotate and reposition it.
				if (mTargetJoint->getName() == "mPelvis")
				{
					mJointState[i]->setUsage(LLJointState::POS | LLJointState::ROT);
				}
				else
				{
					mJointState[i]->setUsage(LLJointState::ROT);
				}
				addJointState(mJointState[i]);
			}
		}
		else
		{
			break;
		}
	}
}

// End

