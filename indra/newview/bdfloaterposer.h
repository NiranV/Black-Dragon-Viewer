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


#ifndef BD_FLOATER_POSER_H
#define BD_FLOATER_POSER_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llcharacter.h"
#include "llvoavatar.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "lltabcontainer.h"
#include "llframetimer.h"
#include "llkeyframemotion.h"

class BDFloaterPoser :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterPoser(const LLSD& key);
	/*virtual*/	~BDFloaterPoser();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();

	//BD - Motions
	void onMotionRefresh();
	void onMotionCommand(LLUICtrl* ctrl, const LLSD& param);

	//BD - Posing
	void onClickPoseSave();
	BOOL onPoseSave(S32 type, F32 time, bool editing);
	BOOL onPoseLoad(const LLSD& name);
	void onPoseStart(LLVOAvatar* character);
	void onPoseDelete();
	void onPoseRefresh();
	void onPoseSet(LLUICtrl* ctrl, const LLSD& param);
	void onPoseControlsRefresh();

	void onAvatarTabsRefresh();
	void onAvatarControlsRefresh();

	void onDoNothing();

	//BD - Joints
	void onJointRefresh(LLVOAvatar* character);
	void onJointSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointSetFinal(LLUICtrl* ctrl, const LLSD& param);
	void onJointPosSet(LLUICtrl* ctrl, const LLSD& param);
	void onJointChangeState(LLVOAvatar* character);
	void onJointControlsRefresh(LLVOAvatar* character);
	void onJointReset(LLVOAvatar* character);

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

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	LLScrollListCtrl*				mAvatarScroll;
	LLScrollListCtrl*				mMotionScroll;

	uuid_vec_t						mIDs;
	std::vector<LLAnimPauseRequest>	mAvatarPauseHandles;

	//BD - Posing
	LLScrollListCtrl*				mPoseScroll;
	LLScrollListCtrl*				mJointsScroll;

	LLTabContainer*					mAvatarTabs;

	LLScrollListCtrl*				mUserDataScroll;

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
