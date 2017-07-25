/**
*
* Copyright (C) 2017, NiranV Dean
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


#ifndef BD_FLOATER_ANIMATIONS_H
#define BD_FLOATER_ANIMATIONS_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llcharacter.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "llframetimer.h"
#include "llkeyframemotion.h"

class BDFloaterAnimations :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterAnimations(const LLSD& key);
	/*virtual*/	~BDFloaterAnimations();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();

	void resetToDefault(LLUICtrl* ctrl);

	//BD - Motions
	void onMotionRefresh();
	void onMotionCommand(LLUICtrl* ctrl, const LLSD& param);

	//BD - Posing
	void onClickPoseSave();
	BOOL onPoseSave(S32 type, F32 time, bool editing);
	BOOL onPoseLoad(const LLSD& name);
	void onPoseStart();
	void onPoseDelete();
	void onPoseRefresh();
	void onPoseSet(LLUICtrl* ctrl, const LLSD& param);
	void onPoseControlsRefresh();

	//BD - Joints
	void onJointRefresh();
	void onJointSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointPosSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointControlsRefresh();

	//BD - Animating
	void onAnimAdd();
	void onAnimDelete();
	void onAnimSave();
	void onAnimSet();
	void onAnimPlay();
	void onAnimStop();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	LLScrollListCtrl*				mAvatarScroll;
	LLScrollListCtrl*				mMotionScroll;

	uuid_vec_t						mIDs;
	std::vector<LLAnimPauseRequest>	mAvatarPauseHandles;

	//BD - Posing
	LLScrollListCtrl*				mPoseScroll;
	LLScrollListCtrl*				mJointsScroll;

	//BD - Animations
	LLScrollListCtrl*				mAnimEditorScroll;
	LLFrameTimer					mAnimPlayTimer;
	F32								mExpiryTime;
	S32								mAnimScrollIndex;
};

#endif
