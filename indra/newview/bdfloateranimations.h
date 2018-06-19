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


#ifndef BD_FLOATER_ANIMATIONS_H
#define BD_FLOATER_ANIMATIONS_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llcharacter.h"

class BDFloaterAnimations :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterAnimations(const LLSD& key);
	/*virtual*/	~BDFloaterAnimations();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	//BD - Motions
	void onMotionRefresh();
	void onMotionCommand(LLUICtrl* ctrl, const LLSD& param);

	LLScrollListCtrl*				mAvatarScroll;
	LLScrollListCtrl*				mMotionScroll;

	uuid_vec_t						mIDs;
	std::vector<LLAnimPauseRequest>	mAvatarPauseHandles;
};

#endif
