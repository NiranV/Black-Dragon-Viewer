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


#ifndef BD_FLOATER_POSER_H
#define BD_FLOATER_POSER_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llkeyframemotion.h"
#include "llframetimer.h"

class BDFloaterPoser :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterPoser(const LLSD& key);
	/*virtual*/	~BDFloaterPoser();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

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
	void onJointSetFinal(LLUICtrl* ctrl, const LLSD& param);
	void onJointPosSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointChangeState();
	void onJointControlsRefresh();
	void onJointReset();
	//void onJointStateCheck();

	//BD - Animating
	void onAnimAdd(const LLSD& param);
	void onAnimListWrite(std::vector<LLScrollListItem*> item_list);
	void onAnimMove(const LLSD& param);
	void onAnimDelete();
	void onAnimSave();
	void onAnimSet();
	void onAnimPlay();
	void onAnimStop();
	void onAnimControlsRefresh();

	//BD - Misc
	void onUpdateLayout();

	//BD - Posing
	LLScrollListCtrl*				mPoseScroll;
	LLScrollListCtrl*				mJointsScroll;

	LLUICtrl*					mRotX;
	LLUICtrl*					mRotY;
	LLUICtrl*					mRotZ;
	LLUICtrl*					mRotXBig;
	LLUICtrl*					mRotYBig;
	LLUICtrl*					mRotZBig;
	LLUICtrl*					mPosX;
	LLUICtrl*					mPosY;
	LLUICtrl*					mPosZ;

	//BD - Animations
	LLScrollListCtrl*				mAnimEditorScroll;
	LLFrameTimer					mAnimPlayTimer;
	F32								mExpiryTime;
	S32								mAnimScrollIndex;
};

#endif
