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


#ifndef BD_ANIMATOR_H
#define BD_ANIMATOR_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llsliderctrl.h"
#include "llmultisliderctrl.h"
#include "lltimectrl.h"
#include "llkeyframemotion.h"
#include "llframetimer.h"

class BDAnimator
{
public:
	enum EActionType
	{
		WAIT = 0,
		REPEAT = 1,
		POSE = 2
	};

	class Action
	{
	public:

		std::string		mPoseName;
		EActionType		mType;
		F32				mTime;
	};

	BDAnimator();
	/*virtual*/	~BDAnimator();

	void			onAddAction(LLScrollListItem* item, S32 location);
	void			onAddAction(std::string name, EActionType type, F32 time, S32 location);
	void			onAddAction(Action action, S32 location);
	void			onDeleteAction(S32 i);

	BOOL			loadPose(const LLSD& name);
	LLSD			returnPose(const LLSD& name);

	void			update();
	void			startPlayback();
	void			stopPlayback();
	void			clearAnimList() { mAnimatorActions.clear(); }

	bool			getIsPlaying() { return mPlaying; }
	S32				getCurrentActionIndex() { return mCurrentAction; }

	std::vector<Action>		mAnimatorActions;
	//BD - Animesh Support
	LLVOAvatar*				mTargetAvatar;
private:
	bool					mPlaying;

	LLFrameTimer			mAnimPlayTimer;
	F32						mExpiryTime;
	S32						mCurrentAction;
};

extern BDAnimator gDragonAnimator;

#endif
