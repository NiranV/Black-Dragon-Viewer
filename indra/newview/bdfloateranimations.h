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

class BDFloaterAnimations :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterAnimations(const LLSD& key);
	/*virtual*/	~BDFloaterAnimations();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();

	void getSelected();

	void onAnimStop();
	void onAnimFreeze();
	void onAnimSet();
	void onAnimRestart();

	void onRefresh();

	void onSave();
	void onLoad();
	void onRemove();
	void onCreate();

	//BD - Posing
	void onPoseSet(LLUICtrl* ctrl, const LLSD& param);
	void onRefreshPoseControls();
	void onBoneRefresh();
	void onBonesClick();

	void onPoseSave();
	void onPoseLoad();
	void onPoseCopy();

	void resetToDefault(LLUICtrl* ctrl);

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	LLScrollListCtrl*				mAvatarScroll;
	LLScrollListCtrl*				mMotionScroll;

	S32								mSelectedAmount;

	uuid_vec_t						mIDs;
	std::vector<LLAnimPauseRequest>	mAvatarPauseHandles;
	std::vector<LLCharacter*>		mSelectedCharacters;

	//BD - Posing
	//LLScrollListCtrl*				mPoseScroll;
	LLScrollListCtrl*				mJointsScroll;
	std::string						mTargetName;
};

#endif
