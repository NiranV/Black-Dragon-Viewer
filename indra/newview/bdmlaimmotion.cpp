/** 
 * @file llheadrotmotion.cpp
 * @brief Implementation of BDMLAimMotion class.
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

#include "bdmlaimmotion.h"
#include "llcharacter.h"
#include "llrand.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"

//BD
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llagentcamera.h"
#include "llagent.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 TORSO_LAG	= 0.5f;	// torso rotation factor
const F32 TORSO_LOOKAT_LAG_HALF_LIFE	= 0.015f;		// half-life of lookat targeting for torso
const F32 MIN_HEAD_LOOKAT_DISTANCE = 0.1f;	// minimum distance from head before we turn to look at it

//-----------------------------------------------------------------------------
// BDMLAimMotion()
// Class Constructor
//-----------------------------------------------------------------------------
BDMLAimMotion::BDMLAimMotion(const LLUUID &id) : 
	LLMotion(id),
	mCharacter(NULL),
	mTorsoJoint(NULL),
	mChestJoint(NULL)
{
	mName = "ml_aim";

	mTorsoState = new LLJointState;
	mChestState = new LLJointState;

	mHeadConstrains = (S32)gSavedSettings.getF32("YawFromMousePosition");
}


//-----------------------------------------------------------------------------
// ~BDMLAimMotion()
// Class Destructor
//-----------------------------------------------------------------------------
BDMLAimMotion::~BDMLAimMotion()
{
}

//-----------------------------------------------------------------------------
// BDMLAimMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus BDMLAimMotion::onInitialize(LLCharacter *character)
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

	mChestJoint = character->getJoint("mChest");
	if (!mChestJoint)
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mTorsoState->setJoint( character->getJoint("mTorso") );
	if ( ! mTorsoState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mChestState->setJoint(character->getJoint("mChest"));
	if (!mChestState->getJoint())
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}

	mTorsoState->setUsage(LLJointState::ROT);
	mChestState->setUsage(LLJointState::ROT);

	addJointState( mTorsoState );
	addJointState( mChestState );

	mLastHeadRot.loadIdentity();

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// BDMLAimMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL BDMLAimMotion::onActivate()
{
	return TRUE;
}


//-----------------------------------------------------------------------------
// BDMLAimMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL BDMLAimMotion::onUpdate(F32 time, U8* joint_mask)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
	LLQuaternion	targetHeadRotWorld;
	LLQuaternion	currentRootRotWorld = mPelvisJoint->getWorldRotation();
	LLQuaternion	currentInvRootRotWorld = ~currentRootRotWorld;

	LLViewerCamera* viewer_cam = LLViewerCamera::getInstance();

	F32 head_slerp_amt = LLSmoothInterpolation::getInterpolant(TORSO_LOOKAT_LAG_HALF_LIFE);

	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");

	if (targetPos)
	{
		LLVector3 headLookAt = *targetPos;

		F32 lookatDistance = headLookAt.normVec();

		if (lookatDistance < MIN_HEAD_LOOKAT_DISTANCE)
		{
			targetHeadRotWorld = mPelvisJoint->getWorldRotation();
		}
		else
		{
			LLVector3 root_up = LLVector3(0.f, 0.f, 1.f);
			LLVector3 left(root_up % headLookAt);
			// if look_at has zero length, fail
			// if look_at and skyward are parallel, fail
			//
			// Test both of these conditions with a cross product.

			if (left.magVecSquared() < 0.05f)
			{
				LLVector3 root_at = LLVector3(1.f, 0.f, 0.f);
				root_at.mV[VZ] = 0.f;
				root_at.normVec();

				headLookAt = lerp(headLookAt, root_at, 0.1f);
				headLookAt.normVec();

				left = root_up % headLookAt;
			}
			
			// Make sure look_at and skyward and not parallel
			// and neither are zero length
			LLVector3 up(headLookAt % left);

			if (gAgentCamera.cameraMouselook())
			{
				//BD - For first person (your own avatar only)
				targetHeadRotWorld = LLQuaternion(headLookAt, viewer_cam->getLeftAxis(), viewer_cam->getUpAxis());
			}
			else
			{
				//BD - For third person and for other avatars.
				targetHeadRotWorld = LLQuaternion(headLookAt, left, up);
			}
			
		}
	}
	else
	{
		targetHeadRotWorld = currentRootRotWorld;
	}

	LLQuaternion head_rot_local = targetHeadRotWorld * currentInvRootRotWorld;
	LLQuaternion quat_rot;

	head_rot_local = nlerp(head_slerp_amt, mLastHeadRot, head_rot_local);
	mLastHeadRot = head_rot_local;

	/*LLQuaternion torso_rot_local = nlerp(TORSO_LAG, LLQuaternion::DEFAULT, head_rot_local);
	mTorsoState->setRotation(nlerp(head_slerp_amt / viewer_cam->getAverageAngularSpeed(), mTorsoState->getRotation(), torso_rot_local));

	LLQuaternion torsoRotLocal = mTorsoState->getJoint()->getWorldRotation() * currentInvRootRotWorld;
	head_rot_local = head_rot_local * ~torsoRotLocal;*/

	if (mChestState->getJoint())
	{
		mChestState->setRotation(head_rot_local);
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// BDMLAimMotion::onDeactivate()
//-----------------------------------------------------------------------------
void BDMLAimMotion::onDeactivate()
{
}

// End

