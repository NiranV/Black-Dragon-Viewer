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

#include "llagent.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llviewercontrol.h"

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
	LLPanel::draw(); 
}

//BD
void BDStatus::setSitting(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("sitting_layout")->setVisible(toggle);
}

void BDStatus::setFlying(bool toggle)
{
	if(gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("flying_layout")->setVisible(toggle);
}

void BDStatus::setRunning(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("running_layout")->setVisible(toggle);
}

void BDStatus::setCrouching(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("crouching_layout")->setVisible(toggle);
}

void BDStatus::setFreeDoF(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("free_dof_layout")->setVisible(toggle);
}

void BDStatus::setLockedDoF(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("locked_dof_layout")->setVisible(toggle);
}

void BDStatus::setWorldFrozen(bool toggle)
{
	if (gDragonStatus)
		gDragonStatus->findChild<LLLayoutPanel>("world_frozen_layout")->setVisible(toggle);
}

void BDStatus::setPosing(bool toggle)
{
	gDragonStatus->findChild<LLLayoutPanel>("posing_layout")->setVisible(toggle);
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

	mSittingButton->setCommitCallback(boost::bind(&BDStatus::onSittingButtonClick, this));
	mFlyingButton->setCommitCallback(boost::bind(&BDStatus::onFlyingButtonClick, this));
	mRunningButton->setCommitCallback(boost::bind(&BDStatus::onRunningButtonClick, this));
	mCrouchingButton->setCommitCallback(boost::bind(&BDStatus::onCrouchingButtonClick, this));
	mFreeDoFButton->setCommitCallback(boost::bind(&BDStatus::onFreeDoFButtonClick, this));
	mLockedDoFButton->setCommitCallback(boost::bind(&BDStatus::onLockedDoFButtonClick, this));
	mWorldFrozenButton->setCommitCallback(boost::bind(&BDStatus::onWorldFrozenButtonClick, this));
	mPosingButton->setCommitCallback(boost::bind(&BDStatus::onPosingButtonClick, this));

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
	gPipeline.CameraDoFLocked = FALSE;

	setFocus(FALSE);
}

void BDStatus::onLockedDoFButtonClick()
{
	gPipeline.CameraFreeDoFFocus = FALSE;

	setFocus(FALSE);
}

void BDStatus::onWorldFrozenButtonClick()
{
	gSavedSettings.setBOOL("UseFreezeWorld", FALSE);

	setFocus(FALSE);
}

void BDStatus::onPosingButtonClick()
{
	gAgent.clearPosing();

	setFocus(FALSE);
}
