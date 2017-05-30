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
#include "llcriticaldamp.h"

//BD
#include "llviewercontrol.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

S32 BDPosingMotion::sHandPose = LLHandMotion::HAND_POSE_RELAXED_R;
S32 BDPosingMotion::sHandPosePriority = 3;

//-----------------------------------------------------------------------------
// BDPosingMotion()
// Class Constructor
//-----------------------------------------------------------------------------
BDPosingMotion::BDPosingMotion(const LLUUID &id) :
LLMotion(id),
	mCharacter(NULL),
	mHeadJoint(NULL),
	mNeckJoint(NULL),
	mChestJoint(NULL),
	mTorsoJoint(NULL),
	mShoulderLeftJoint(NULL),
	mElbowLeftJoint(NULL),
	mWristLeftJoint(NULL),
	mShoulderRightJoint(NULL),
	mElbowRightJoint(NULL),
	mWristRightJoint(NULL),
	mPelvisJoint(NULL),
	mHipLeftJoint(NULL),
	mKneeLeftJoint(NULL),
	mAnkleLeftJoint(NULL),
	mHipRightJoint(NULL),
	mKneeRightJoint(NULL),
	mAnkleRightJoint(NULL)
{
	mName = "custom_pose";

	mHeadState = new LLJointState;
	mNeckState = new LLJointState;
	mChestState = new LLJointState;
	mTorsoState = new LLJointState;
	mShoulderLeftState = new LLJointState;
	mElbowLeftState = new LLJointState;
	mWristLeftState = new LLJointState;
	mShoulderRightState = new LLJointState;
	mElbowRightState = new LLJointState;
	mWristRightState = new LLJointState;
	mPelvisState = new LLJointState;
	mHipLeftState = new LLJointState;
	mKneeLeftState = new LLJointState;
	mAnkleLeftState = new LLJointState;
	mHipRightState = new LLJointState;
	mKneeRightState = new LLJointState;
	mAnkleRightState = new LLJointState;

	//BD - Bento support
	//     I need to rebuild this entire system or this will quickly end in massive chaos
	//     and millions of debugs.
	mHandThumb1LeftState = new LLJointState;
	mHandThumb2LeftState = new LLJointState;
	mHandThumb3LeftState = new LLJointState;

	mHandIndex1LeftState = new LLJointState;
	mHandIndex2LeftState = new LLJointState;
	mHandIndex3LeftState = new LLJointState;

	mHandMiddle1LeftState = new LLJointState;
	mHandMiddle2LeftState = new LLJointState;
	mHandMiddle3LeftState = new LLJointState;

	mHandRing1LeftState = new LLJointState;
	mHandRing2LeftState = new LLJointState;
	mHandRing3LeftState = new LLJointState;

	mHandPinky1LeftState = new LLJointState;
	mHandPinky2LeftState = new LLJointState;
	mHandPinky3LeftState = new LLJointState;

	mHandThumb1RightState = new LLJointState;
	mHandThumb2RightState = new LLJointState;
	mHandThumb3RightState = new LLJointState;

	mHandIndex1RightState = new LLJointState;
	mHandIndex2RightState = new LLJointState;
	mHandIndex3RightState = new LLJointState;

	mHandMiddle1RightState = new LLJointState;
	mHandMiddle2RightState = new LLJointState;
	mHandMiddle3RightState = new LLJointState;

	mHandRing1RightState = new LLJointState;
	mHandRing2RightState = new LLJointState;
	mHandRing3RightState = new LLJointState;

	mHandPinky1LeftState = new LLJointState;
	mHandPinky2LeftState = new LLJointState;
	mHandPinky3LeftState = new LLJointState;

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

	mHeadJoint = mCharacter->getJoint("mHead");
	mNeckJoint = mCharacter->getJoint("mNeck");
	mChestJoint = mCharacter->getJoint("mChest");
	mTorsoJoint = mCharacter->getJoint("mTorso");
	mShoulderLeftJoint = mCharacter->getJoint("mShoulderLeft");
	mElbowLeftJoint = mCharacter->getJoint("mElbowLeft");
	mWristLeftJoint = mCharacter->getJoint("mWristLeft");
	mShoulderRightJoint = mCharacter->getJoint("mShoulderRight");
	mElbowRightJoint = mCharacter->getJoint("mElbowRight");
	mWristRightJoint = mCharacter->getJoint("mWristRight");
	mPelvisJoint = mCharacter->getJoint("mPelvis");
	mHipLeftJoint = mCharacter->getJoint("mHipLeft");
	mKneeLeftJoint = mCharacter->getJoint("mKneeLeft");
	mAnkleLeftJoint = mCharacter->getJoint("mAnkleLeft");
	mHipRightJoint = mCharacter->getJoint("mHipRight");
	mKneeRightJoint = mCharacter->getJoint("mKneeRight");
	mAnkleRightJoint = mCharacter->getJoint("mAnkleRight");

	// make sure character skeleton is copacetic
	//BD
	if (!mHeadJoint
	||	!mNeckJoint
	||	!mChestJoint
	||	!mTorsoJoint
	||	!mShoulderLeftJoint
	||	!mElbowLeftJoint
	||	!mWristLeftJoint
	||	!mShoulderRightJoint
	||	!mElbowRightJoint
	||	!mWristRightJoint
	||	!mPelvisJoint
	||	!mHipLeftJoint
	||	!mKneeLeftJoint
	||	!mAnkleLeftJoint
	||	!mHipRightJoint
	||	!mKneeRightJoint
	||	!mAnkleRightJoint)
	{
		LL_WARNS() << "Invalid skeleton for editing motion!" << LL_ENDL;
		return STATUS_FAILURE;
	}

	mHeadState->setJoint(mHeadJoint);
	mNeckState->setJoint(mNeckJoint);
	mChestState->setJoint(mChestJoint);
	mTorsoState->setJoint(mTorsoJoint);
	mShoulderLeftState->setJoint(mShoulderLeftJoint);
	mElbowLeftState->setJoint(mElbowLeftJoint);
	mWristLeftState->setJoint(mWristLeftJoint);
	mShoulderRightState->setJoint(mShoulderRightJoint);
	mElbowRightState->setJoint(mElbowRightJoint);
	mWristRightState->setJoint(mWristRightJoint);
	mPelvisState->setJoint(mPelvisJoint);
	mHipLeftState->setJoint(mHipLeftJoint);
	mKneeLeftState->setJoint(mKneeLeftJoint);
	mAnkleLeftState->setJoint(mAnkleLeftJoint);
	mHipRightState->setJoint(mHipRightJoint);
	mKneeRightState->setJoint(mKneeRightJoint);
	mAnkleRightState->setJoint(mAnkleRightJoint);

	// add joint states to the pose
	mHeadState->setUsage(LLJointState::ROT);
	mNeckState->setUsage(LLJointState::ROT);
	mChestState->setUsage(LLJointState::ROT);
	mTorsoState->setUsage(LLJointState::ROT);
	mShoulderLeftState->setUsage(LLJointState::ROT);
	mElbowLeftState->setUsage(LLJointState::ROT);
	mWristLeftState->setUsage(LLJointState::ROT);
	mShoulderRightState->setUsage(LLJointState::ROT);
	mElbowRightState->setUsage(LLJointState::ROT);
	mWristRightState->setUsage(LLJointState::ROT);
	mPelvisState->setUsage(LLJointState::ROT);
	mHipLeftState->setUsage(LLJointState::ROT);
	mKneeLeftState->setUsage(LLJointState::ROT);
	mAnkleLeftState->setUsage(LLJointState::ROT);
	mHipRightState->setUsage(LLJointState::ROT);
	mKneeRightState->setUsage(LLJointState::ROT);
	mAnkleRightState->setUsage(LLJointState::ROT);

	addJointState(mHeadState);
	addJointState(mNeckState);
	addJointState(mChestState);
	addJointState(mTorsoState);
	addJointState(mShoulderLeftState);
	addJointState(mElbowLeftState);
	addJointState(mWristLeftState);
	addJointState(mShoulderRightState);
	addJointState(mElbowRightState);
	addJointState(mWristRightState);
	addJointState(mPelvisState);
	addJointState(mHipLeftState);
	addJointState(mKneeLeftState);
	addJointState(mAnkleLeftState);
	addJointState(mHipRightState);
	addJointState(mKneeRightState);
	addJointState(mAnkleRightState);

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL BDPosingMotion::onActivate()
{
	return TRUE;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL BDPosingMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLQuaternion vec2quat;

	LLVector3 vec3 = gSavedSettings.getVector3("PosingHeadJoint");
	vec2quat.unpackFromVector3(vec3); 
	mHeadState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingNeckJoint");
	vec2quat.unpackFromVector3(vec3);
	mNeckState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingChestJoint");
	vec2quat.unpackFromVector3(vec3);
	mChestState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingTorsoJoint");
	vec2quat.unpackFromVector3(vec3);
	mTorsoState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingShoulderLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mShoulderLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingElbowLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mElbowLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingWristLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mWristLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingShoulderRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mShoulderRightState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingElbowRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mElbowRightState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingWristRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mWristRightState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingPelvisJoint");
	vec2quat.unpackFromVector3(vec3);
	mPelvisState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingHipLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mHipLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingKneeLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mKneeLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingAnkleLeftJoint");
	vec2quat.unpackFromVector3(vec3);
	mAnkleLeftState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingHipRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mHipRightState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingKneeRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mKneeRightState->setRotation(vec2quat);

	vec3 = gSavedSettings.getVector3("PosingAnkleRightJoint");
	vec2quat.unpackFromVector3(vec3);
	mAnkleRightState->setRotation(vec2quat);
	return TRUE;
}

//-----------------------------------------------------------------------------
// BDPosingMotion::onDeactivate()
//-----------------------------------------------------------------------------
void BDPosingMotion::onDeactivate()
{
}


// End

