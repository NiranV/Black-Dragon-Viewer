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


#ifndef BD_FLOATER_COMPLEXITY_H
#define BD_FLOATER_COMPLEXITY_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llcharacter.h"
#include "llvovolume.h"
#include "llsafehandle.h"
#include "llselectmgr.h"

class BDFloaterComplexity :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterComplexity(const LLSD& key);
	/*virtual*/	~BDFloaterComplexity();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();

	//BD - Shameless copy from bdfloateranimations.cpp
	void onAvatarsRefresh();

	//BD - Complexity
	void calcARC();
	void checkObject(LLVOVolume* vovolume, LLVOVolume::texture_cost_t &textures,
					U32 &volume_cost, U32 &base_cost, U64 &total_triangles, U64 &total_vertices);
	void rateAvatarGroup(std::string type, F32 rating);
	void onSelectEntry();
	void onSelectAttachment();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	//BD - Complexity
	LLScrollListCtrl*				mARCScroll;
	LLScrollListCtrl*				mAvatarScroll;
	LLTextBox*						mTotalVerticeCount;
	LLTextBox*						mTotalTriangleCount;
	LLTextBox*						mTotalCost;
	LLTextBox*						mTextureCost;

	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

#endif
