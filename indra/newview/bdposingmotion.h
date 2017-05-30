/** 
 * @file BDPosingMotion.h
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

#ifndef LL_BDPOSINGMOTION_H
#define LL_BDPOSINGMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lljointsolverrp3.h"
#include "v3dmath.h"

#define EDITING_EASEIN_DURATION	0.0f
#define EDITING_EASEOUT_DURATION 0.5f
#define POSING_PRIORITY LLJoint::HIGHEST_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_EDITING 500.f

//-----------------------------------------------------------------------------
// class BDPosingMotion
//-----------------------------------------------------------------------------
class BDPosingMotion :
	public LLMotion
{
public:
	// Constructor
	BDPosingMotion(const LLUUID &id);

	// Destructor
	virtual ~BDPosingMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new BDPosingMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual BOOL getLoop() { return TRUE; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return EDITING_EASEIN_DURATION; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return EDITING_EASEOUT_DURATION; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return POSING_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EDITING; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);

	// called when a motion is activated
	// must return TRUE to indicate success, or else
	// it will be deactivated
	virtual BOOL onActivate();

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask);

	// called when a motion is deactivated
	virtual void onDeactivate();

public:
	//-------------------------------------------------------------------------
	// Joint States
	//-------------------------------------------------------------------------
	LLCharacter			*mCharacter;

	LLPointer<LLJointState> mHeadState;
	LLPointer<LLJointState> mNeckState;
	LLPointer<LLJointState> mChestState;
	LLPointer<LLJointState> mTorsoState;
	LLPointer<LLJointState> mPelvisState;

	LLPointer<LLJointState> mShoulderLeftState;
	LLPointer<LLJointState> mElbowLeftState;
	LLPointer<LLJointState> mWristLeftState;
	
	LLPointer<LLJointState> mShoulderRightState;
	LLPointer<LLJointState> mElbowRightState;
	LLPointer<LLJointState> mWristRightState;

	LLPointer<LLJointState> mHipLeftState;
	LLPointer<LLJointState> mKneeLeftState;
	LLPointer<LLJointState> mAnkleLeftState;

	LLPointer<LLJointState> mHipRightState;
	LLPointer<LLJointState> mKneeRightState;
	LLPointer<LLJointState> mAnkleRightState;

	//BD - Bento
	//

	LLPointer<LLJointState> mHandThumb1LeftState;
	LLPointer<LLJointState> mHandThumb2LeftState;
	LLPointer<LLJointState> mHandThumb3LeftState;

	LLPointer<LLJointState> mHandIndex1LeftState;
	LLPointer<LLJointState> mHandIndex2LeftState;
	LLPointer<LLJointState> mHandIndex3LeftState;

	LLPointer<LLJointState> mHandMiddle1LeftState;
	LLPointer<LLJointState> mHandMiddle2LeftState;
	LLPointer<LLJointState> mHandMiddle3LeftState;

	LLPointer<LLJointState> mHandRing1LeftState;
	LLPointer<LLJointState> mHandRing2LeftState;
	LLPointer<LLJointState> mHandRing3LeftState;

	LLPointer<LLJointState> mHandPinky1LeftState;
	LLPointer<LLJointState> mHandPinky2LeftState;
	LLPointer<LLJointState> mHandPinky3LeftState;

	LLPointer<LLJointState> mHandThumb1RightState;
	LLPointer<LLJointState> mHandThumb2RightState;
	LLPointer<LLJointState> mHandThumb3RightState;

	LLPointer<LLJointState> mHandIndex1RightState;
	LLPointer<LLJointState> mHandIndex2RightState;
	LLPointer<LLJointState> mHandIndex3RightState;

	LLPointer<LLJointState> mHandMiddle1RightState;
	LLPointer<LLJointState> mHandMiddle2RightState;
	LLPointer<LLJointState> mHandMiddle3RightState;

	LLPointer<LLJointState> mHandRing1RightState;
	LLPointer<LLJointState> mHandRing2RightState;
	LLPointer<LLJointState> mHandRing3RightState;

	LLPointer<LLJointState> mHandPinky1RightState;
	LLPointer<LLJointState> mHandPinky2RightState;
	LLPointer<LLJointState> mHandPinky3RightState;

	//-------------------------------------------------------------------------
	// Joints
	//-------------------------------------------------------------------------
	//BD - Core Body
	LLJoint*			mHeadJoint;			// head
	LLJoint*			mNeckJoint;			// neck
	LLJoint*			mChestJoint;		// upper body
	LLJoint*			mTorsoJoint;		// upper body
	LLJoint*			mPelvisJoint;		// lower body

	//BD - Upper Body Limbs
	LLJoint*			mShoulderLeftJoint;	// upper arm
	LLJoint*			mElbowLeftJoint;	// lower arm
	LLJoint*			mWristLeftJoint;	// hand

	LLJoint*			mShoulderRightJoint;// upper arm
	LLJoint*			mElbowRightJoint;	// lower arm
	LLJoint*			mWristRightJoint;	// hand

	//BD - Lower Body Limbs
	LLJoint*			mHipLeftJoint;		// upper leg
	LLJoint*			mKneeLeftJoint;		// lower leg
	LLJoint*			mAnkleLeftJoint;	// foot

	LLJoint*			mHipRightJoint;		// upper leg
	LLJoint*			mKneeRightJoint;	// lower leg
	LLJoint*			mAnkleRightJoint;	// foot

	//BD - Bento
	//

	LLJoint* mHandThumb1LeftJoint;
	LLJoint* mHandThumb2LeftJoint;
	LLJoint* mHandThumb3LeftJoint;

	LLJoint* mHandIndex1LeftJoint;
	LLJoint* mHandIndex2LeftJoint;
	LLJoint* mHandIndex3LeftJoint;

	LLJoint* mHandMiddle1LeftJoint;
	LLJoint* mHandMiddle2LeftJoint;
	LLJoint* mHandMiddle3LeftJoint;

	LLJoint* mHandRing1LeftJoint;
	LLJoint* mHandRing2LeftJoint;
	LLJoint* mHandRing3LeftJoint;

	LLJoint* mHandPinky1LeftJoint;
	LLJoint* mHandPinky2LeftJoint;
	LLJoint* mHandPinky3LeftJoint;

	LLJoint* mHandThumb1RightJoint;
	LLJoint* mHandThumb2RightJoint;
	LLJoint* mHandThumb3RightJoint;

	LLJoint* mHandIndex1RightJoint;
	LLJoint* mHandIndex2RightJoint;
	LLJoint* mHandIndex3RightJoint;

	LLJoint* mHandMiddle1RightJoint;
	LLJoint* mHandMiddle2RightJoint;
	LLJoint* mHandMiddle3RightJoint;

	LLJoint* mHandRing1RightJoint;
	LLJoint* mHandRing2RightJoint;
	LLJoint* mHandRing3RightJoint;

	LLJoint* mHandPinky1RightJoint;
	LLJoint* mHandPinky2RightJoint;
	LLJoint* mHandPinky3RightJoint;

	LLJoint				mTarget;

	static S32			sHandPose;
	static S32			sHandPosePriority;
	LLVector3			mLastSelectPt;
};

#endif // LL_LLKEYFRAMEMOTION_H

