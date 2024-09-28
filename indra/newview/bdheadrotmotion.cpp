/** 
 * @file BDHeadRotMotion.cpp
 * @brief Implementation of BDHeadRotMotion class.
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
#include "llviewerprecompiledheaders.h"

#include "bdheadrotmotion.h"
#include "llcharacter.h"
#include "llrand.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"

//BD
#include "llviewercontrol.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 TORSO_LAG	= 0.35f;	// torso rotation factor
const F32 NECK_LAG = 0.5f;		// neck rotation factor
const F32 HEAD_LOOKAT_LAG_HALF_LIFE	= 0.15f;		// half-life of lookat targeting for head
const F32 TORSO_LOOKAT_LAG_HALF_LIFE	= 0.27f;		// half-life of lookat targeting for torso
const F32 MIN_HEAD_LOOKAT_DISTANCE = 0.3f;	// minimum distance from head before we turn to look at it
const F32 EYE_JITTER_MIN_TIME = 0.3f; // min amount of time between eye "jitter" motions
const F32 EYE_JITTER_MAX_TIME = 2.5f; // max amount of time between eye "jitter" motions
const F32 EYE_JITTER_MAX_YAW = 0.08f; // max yaw of eye jitter motion
const F32 EYE_JITTER_MAX_PITCH = 0.015f; // max pitch of eye jitter motion
const F32 EYE_LOOK_AWAY_MIN_TIME = 5.f; // min amount of time between eye "look away" motions
const F32 EYE_LOOK_AWAY_MAX_TIME = 15.f; // max amount of time between eye "look away" motions
const F32 EYE_LOOK_BACK_MIN_TIME = 1.f; // min amount of time before looking back after looking away
const F32 EYE_LOOK_BACK_MAX_TIME = 5.f; // max amount of time before looking back after looking away
const F32 EYE_LOOK_AWAY_MAX_YAW = 0.15f; // max yaw of eye look away motion
const F32 EYE_LOOK_AWAY_MAX_PITCH = 0.12f; // max pitch of look away motion

const F32 EYE_BLINK_MIN_TIME = 0.5f; // minimum amount of time between blinks
const F32 EYE_BLINK_MAX_TIME = 8.f;	// maximum amount of time between blinks
const F32 EYE_BLINK_CLOSE_TIME = 0.03f; // how long the eye stays closed in a blink
const F32 EYE_BLINK_SPEED = 0.015f;		// seconds it takes for a eye open/close movement
const F32 EYE_BLINK_TIME_DELTA = 0.005f; // time between one eye starting a blink and the other following

//-----------------------------------------------------------------------------
// BDHeadRotMotion()
// Class Constructor
//-----------------------------------------------------------------------------
BDHeadRotMotion::BDHeadRotMotion(const LLUUID &id) : 
	LLMotion(id),
	mCharacter(nullptr),
	mTorsoJoint(nullptr),
	mHeadJoint(nullptr),
    mPelvisJoint(nullptr),
    mRootJoint(nullptr)
{
	mName = "head_rot";

	mTorsoState = new LLJointState;
	mNeckState = new LLJointState;
	mHeadState = new LLJointState;

	mHeadConstrains = (S32)gSavedSettings.getF32("YawFromMousePosition");
}


//-----------------------------------------------------------------------------
// ~BDHeadRotMotion()
// Class Destructor
//-----------------------------------------------------------------------------
BDHeadRotMotion::~BDHeadRotMotion()
{
}

//-----------------------------------------------------------------------------
// BDHeadRotMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus BDHeadRotMotion::onInitialize(LLCharacter *character)
{
	if (!character)
		return STATUS_FAILURE;
	mCharacter = character;

	mPelvisJoint = character->getJoint("mPelvis");
	if ( ! mPelvisJoint )
	{
		LL_INFOS() << getName() << ": Can't get pelvis joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mRootJoint = character->getJoint("mRoot");
	if ( ! mRootJoint )
	{
		LL_INFOS() << getName() << ": Can't get root joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mTorsoJoint = character->getJoint("mTorso");
	if ( ! mTorsoJoint )
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mHeadJoint = character->getJoint("mHead");
	if ( ! mHeadJoint )
	{
		LL_INFOS() << getName() << ": Can't get head joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mTorsoState->setJoint( character->getJoint("mTorso") );
	if ( ! mTorsoState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mNeckState->setJoint( character->getJoint("mNeck") );
	if ( ! mNeckState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get neck joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mHeadState->setJoint( character->getJoint("mHead") );
	if ( ! mHeadState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get head joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mTorsoState->setUsage(LLJointState::ROT);
	mNeckState->setUsage(LLJointState::ROT);
	mHeadState->setUsage(LLJointState::ROT);

	addJointState( mTorsoState );
	addJointState( mNeckState );
	addJointState( mHeadState );

	mLastHeadRot.loadIdentity();

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// BDHeadRotMotion::onActivate()
//-----------------------------------------------------------------------------
bool BDHeadRotMotion::onActivate()
{
	return true;
}


//-----------------------------------------------------------------------------
// BDHeadRotMotion::onUpdate()
//-----------------------------------------------------------------------------
bool BDHeadRotMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLQuaternion	targetHeadRotWorld;
	LLQuaternion	currentRootRotWorld = mRootJoint->getWorldRotation();
	LLQuaternion	currentInvRootRotWorld = ~currentRootRotWorld;

	F32 head_slerp_amt = LLSmoothInterpolation::getInterpolant(HEAD_LOOKAT_LAG_HALF_LIFE);
	//F32 torso_slerp_amt = LLSmoothInterpolation::getInterpolant(TORSO_LOOKAT_LAG_HALF_LIFE);

	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");

	if (targetPos)
	{
		LLVector3 headLookAt = *targetPos;

//		LL_INFOS() << "Look At: " << headLookAt + mHeadJoint->getWorldPosition() << LL_ENDL;

		F32 lookatDistance = headLookAt.normVec();

		if (lookatDistance < MIN_HEAD_LOOKAT_DISTANCE)
		{
			targetHeadRotWorld = mPelvisJoint->getWorldRotation();
		}
		else
		{
			LLVector3 root_up = LLVector3(0.f, 0.f, 1.f) * currentRootRotWorld;
			LLVector3 left(root_up % headLookAt);
			// if look_at has zero length, fail
			// if look_at and skyward are parallel, fail
			//
			// Test both of these conditions with a cross product.

			if (left.magVecSquared() < 0.15f)
			{
				LLVector3 root_at = LLVector3(1.f, 0.f, 0.f) * currentRootRotWorld;
				root_at.mV[VZ] = 0.f;
				root_at.normVec();

				headLookAt = lerp(headLookAt, root_at, 0.4f);
				headLookAt.normVec();

				left = root_up % headLookAt;
			}
			
			// Make sure look_at and skyward and not parallel
			// and neither are zero length
			LLVector3 up(headLookAt % left);

			targetHeadRotWorld = LLQuaternion(headLookAt, left, up);
		}
	}
	else
	{
		targetHeadRotWorld = currentRootRotWorld;
	}

	LLQuaternion head_rot_local = targetHeadRotWorld * currentInvRootRotWorld;
	//BD
	head_rot_local.constrain(mHeadConstrains * DEG_TO_RAD);

	// set final torso rotation
	// Set torso target rotation such that it lags behind the head rotation
	// by a fixed amount.
	//LLQuaternion torso_rot_local = nlerp(TORSO_LAG, LLQuaternion::DEFAULT, head_rot_local );
	//mTorsoState->setRotation( nlerp(torso_slerp_amt, mTorsoState->getRotation(), torso_rot_local) );

	head_rot_local = nlerp(head_slerp_amt, mLastHeadRot, head_rot_local);
	mLastHeadRot = head_rot_local;
	// Set the head rotation.
	if (mNeckState->getJoint() && mNeckState->getJoint()->getParent())
	{
		//LLQuaternion torsoRotLocal = mNeckState->getJoint()->getParent()->getWorldRotation() * currentInvRootRotWorld;
		//head_rot_local = head_rot_local * ~torsoRotLocal;
		mNeckState->setRotation(nlerp(NECK_LAG, LLQuaternion::DEFAULT, head_rot_local));
		mHeadState->setRotation(nlerp(1.f - NECK_LAG, LLQuaternion::DEFAULT, head_rot_local));
	}

	return true;
}


//-----------------------------------------------------------------------------
// BDHeadRotMotion::onDeactivate()
//-----------------------------------------------------------------------------
void BDHeadRotMotion::onDeactivate()
{
}

// End

