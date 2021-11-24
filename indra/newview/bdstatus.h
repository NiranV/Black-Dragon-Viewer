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


#ifndef BD_STATUS_H
#define BD_STATUS_H

#include <string>

#include "llsliderctrl.h"
#include "llbutton.h"
#include "lllayoutstack.h"

class LLComboBox;

class BDStatus
: public LLPanel
{
public:
	BDStatus(const LLRect& rect);
	/*virtual*/ ~BDStatus();

	/*typedef enum status_mode
	{
		NONE = 0,
		SITTING = 1,
		FLYING = 2,
		RUNNING = 4,
		CROUCHING = 8,
		FREE_DOF = 16,
		LOCKED_DOF = 32,
		WORLD_FROZEN = 64,
		POSING = 128
	} EStatusMode;*/

	static void setSitting(bool toggle);
	static void setFlying(bool toggle);
	static void setRunning(bool toggle);
	static void setCrouching(bool toggle);
	static void setFreeDoF(bool toggle);
	static void setLockedDoF(bool toggle);
	static void setWorldFrozen(bool toggle);
	static void setPosing(bool toggle);
	static void setCameraRoll(bool toggle);
	static void setCameraLock(bool toggle);
	static void setFlycam(bool toggle);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();


private:
	void onSittingButtonClick();
	void onFlyingButtonClick();
	void onRunningButtonClick();
	void onCrouchingButtonClick();
	void onFreeDoFButtonClick();
	void onLockedDoFButtonClick();
	void onWorldFrozenButtonClick();
	void onPosingButtonClick();
	void onCameraRollButtonClick();
	void onCameraLockButtonClick();
	void onFlycamButtonClick();

	LLButton* mSittingButton;
	LLButton* mFlyingButton;
	LLButton* mRunningButton;
	LLButton* mCrouchingButton;
	LLButton* mFreeDoFButton;
	LLButton* mLockedDoFButton;
	LLButton* mWorldFrozenButton;
	LLButton* mPosingButton;
	LLButton* mCameraRollButton;
	LLButton* mCameraLockButton;
	LLButton* mFlycamButton;

	LLLayoutPanel* mSittingLayout;
	LLLayoutPanel* mFlyingLayout;
	LLLayoutPanel* mRunningLayout;
	LLLayoutPanel* mCrouchingLayout;
	LLLayoutPanel* mFreeDoFLayout;
	LLLayoutPanel* mLockedDoFLayout;
	LLLayoutPanel* mWorldFrozenLayout;
	LLLayoutPanel* mPosingLayout;
	LLLayoutPanel* mCameraRollLayout;
	LLLayoutPanel* mCameraLockLayout;
	LLLayoutPanel* mFlycamLayout;

	bool mSitting;
	bool mRoll;
	bool mFlycam;
};

extern BDStatus *gDragonStatus;

#endif
