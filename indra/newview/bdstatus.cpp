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

#include "bdstatus.h"

#include "bdposingmotion.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llvoavatarself.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"

#include "rlvactions.h"

BDStatus *gDragonStatus = NULL;

BDStatus::BDStatus(const LLRect& rect)
	: LLPanel()
{
	buildFromFile("panel_statuses.xml");
}

BDStatus::~BDStatus()
{
}

void BDStatus::draw()
{
	bool camera_roll = gAgentCamera.mCameraRollAngle != 0;
	bool sitting = gAgent.isSitting();
	bool flycam = gJoystick->getOverrideCamera();

	if (!mFlycam && flycam)
		setFlycam(true);
	else if (mFlycam && !flycam)
		setFlycam(false);

	if (!mSitting && sitting)
		setSitting(true);
	else if(mSitting && !sitting)
		setSitting(false);

	if (!mRoll && camera_roll)
		setCameraRoll(true);
	else if (mRoll && !camera_roll)
		setCameraRoll(false);

	LLPanel::draw(); 
}

//BD
void BDStatus::setSitting(bool toggle)
{
	if (gDragonStatus)
	{
		gDragonStatus->mSittingLayout->setVisible(toggle);
		gDragonStatus->mSitting = toggle;
	}
}

void BDStatus::setFlying(bool toggle)
{
	if(gDragonStatus)
		gDragonStatus->mFlyingLayout->setVisible(toggle);
}

void BDStatus::setRunning(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->mRunningLayout->setVisible(toggle);
}

void BDStatus::setCrouching(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->mCrouchingLayout->setVisible(toggle);
}

void BDStatus::setFreeDoF(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->mFreeDoFLayout->setVisible(toggle);
}

void BDStatus::setLockedDoF(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->mLockedDoFLayout->setVisible(toggle);
}

void BDStatus::setWorldFrozen(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->mWorldFrozenLayout->setVisible(toggle);
}

void BDStatus::setPosing(bool toggle)
{
	gDragonStatus->mPosingLayout->setVisible(toggle);
}

void BDStatus::setCameraRoll(bool toggle)
{
	gDragonStatus->mCameraRollLayout->setVisible(toggle);
}

void BDStatus::setCameraLock(bool toggle)
{
	gDragonStatus->mCameraLockLayout->setVisible(toggle);
}

void BDStatus::setFlycam(bool toggle)
{
	if (gDragonStatus)
	{
		gDragonStatus->mFlycamLayout->setVisible(toggle);
		gDragonStatus->mFlycam = toggle;
	}
}


BOOL BDStatus::postBuild()
{
	mSittingButton = getChild<LLButton>("sitting_btn");
	mFlyingButton = getChild<LLButton>("flying_btn");
	mRunningButton = getChild<LLButton>("running_btn");
	mCrouchingButton = getChild<LLButton>("crouching_btn");
	mFreeDoFButton = getChild<LLButton>("free_dof_btn");
	mLockedDoFButton = getChild<LLButton>("locked_dof_btn");
	mWorldFrozenButton = getChild<LLButton>("world_frozen_btn");
	mPosingButton = getChild<LLButton>("posing_btn");
	mCameraRollButton = getChild<LLButton>("camera_roll_btn");
	mCameraLockButton = getChild<LLButton>("camera_lock_btn");
	mFlycamButton = getChild<LLButton>("flycam_btn");

	mSittingLayout = getChild<LLLayoutPanel>("sitting_layout");
	mFlyingLayout = getChild<LLLayoutPanel>("flying_layout");
	mRunningLayout = getChild<LLLayoutPanel>("running_layout");
	mCrouchingLayout = getChild<LLLayoutPanel>("crouching_layout");
	mFreeDoFLayout = getChild<LLLayoutPanel>("free_dof_layout");
	mLockedDoFLayout = getChild<LLLayoutPanel>("locked_dof_layout");
	mWorldFrozenLayout = getChild<LLLayoutPanel>("world_frozen_layout");
	mPosingLayout = getChild<LLLayoutPanel>("posing_layout");
	mCameraRollLayout = getChild<LLLayoutPanel>("camera_roll_layout");
	mCameraLockLayout = getChild<LLLayoutPanel>("camera_lock_layout");
	mFlycamLayout = getChild<LLLayoutPanel>("flycam_layout");

	mSittingButton->setCommitCallback(boost::bind(&BDStatus::onSittingButtonClick, this));
	mFlyingButton->setCommitCallback(boost::bind(&BDStatus::onFlyingButtonClick, this));
	mRunningButton->setCommitCallback(boost::bind(&BDStatus::onRunningButtonClick, this));
	mCrouchingButton->setCommitCallback(boost::bind(&BDStatus::onCrouchingButtonClick, this));
	mFreeDoFButton->setCommitCallback(boost::bind(&BDStatus::onFreeDoFButtonClick, this));
	mLockedDoFButton->setCommitCallback(boost::bind(&BDStatus::onLockedDoFButtonClick, this));
	mWorldFrozenButton->setCommitCallback(boost::bind(&BDStatus::onWorldFrozenButtonClick, this));
	mPosingButton->setCommitCallback(boost::bind(&BDStatus::onPosingButtonClick, this));
	mCameraRollButton->setCommitCallback(boost::bind(&BDStatus::onCameraRollButtonClick, this));
	mCameraLockButton->setCommitCallback(boost::bind(&BDStatus::onCameraLockButtonClick, this));
	mFlycamButton->setCommitCallback(boost::bind(&BDStatus::onFlycamButtonClick, this));

	return TRUE;
}

void BDStatus::onSittingButtonClick()
{
	// [RLVa:KB] - Checked: 2010-03-07 (RLVa-1.2.0c) | Added: RLVa-1.2.0a
	if ((!RlvActions::isRlvEnabled()) || (RlvActions::canStand()))
	{
		LLSelectMgr::getInstance()->deselectAllForStandingUp();
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
	}
	// [/RLVa:KB]
	//	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	//	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);

	setFocus(FALSE);
}

void BDStatus::onFlyingButtonClick()
{
	gAgent.setFlying(false);

	setFocus(FALSE);
}

void BDStatus::onRunningButtonClick()
{
	gAgent.clearAlwaysRun();

	setFocus(FALSE);
}

void BDStatus::onCrouchingButtonClick()
{
	gAgent.setCrouching(false);

	setFocus(FALSE);
}

void BDStatus::onFreeDoFButtonClick()
{
	gSavedSettings.setBOOL("CameraFreeDoFFocus", false);
	setFreeDoF(false);
	setFocus(FALSE);
}

void BDStatus::onLockedDoFButtonClick()
{
	gSavedSettings.setBOOL("CameraDoFLocked", false);
	setLockedDoF(false);
	setFocus(FALSE);
}

void BDStatus::onWorldFrozenButtonClick()
{
	gSavedSettings.setBOOL("UseFreezeWorld", FALSE);

	setFocus(FALSE);
}

void BDStatus::onPosingButtonClick()
{
	gAgentAvatarp->stopMotion(ANIM_BD_POSING_MOTION);
	gAgent.clearPosing();
	setPosing(false);

	setFocus(FALSE);
}

void BDStatus::onCameraRollButtonClick()
{
	gAgentCamera.mCameraRollAngle = 0;
	setCameraRoll(false);

	setFocus(FALSE);
}

void BDStatus::onCameraLockButtonClick()
{
	gAgentCamera.setCameraLocked(false);
	setCameraLock(false);

	setFocus(FALSE);
}

void BDStatus::onFlycamButtonClick()
{
	gJoystick->setOverrideCamera(false);
	setFlycam(false);

	setFocus(FALSE);
}
