/** 
 * @file bdtopbarholder.h
 * @brief LLTopBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_NVTOPBARHOLDER_H
#define LL_NVTOPBARHOLDER_H

#include "llpanel.h"

class LLUICtrl;

class LLTopBar
:	public LLPanel
{
public:
	LLTopBar(const LLRect& rect);
	/*virtual*/ ~LLTopBar();
	
	/*virtual*/ void draw();

	/*virtual*/ BOOL postBuild();

	void refreshGraphicControls();

	/*
//	//BD - Vector4
	void onCommitVec4X(LLUICtrl* ctrl, const LLSD& param);
	void onCommitVec4Y(LLUICtrl* ctrl, const LLSD& param);
	void onCommitVec4Z(LLUICtrl* ctrl, const LLSD& param);
	void onCommitVec4W(LLUICtrl* ctrl, const LLSD& param);

//	//BD - Debug Arrays
	void onCommitX(LLUICtrl* ctrl, const LLSD& param);
	void onCommitY(LLUICtrl* ctrl, const LLSD& param);
	void onCommitZ(LLUICtrl* ctrl, const LLSD& param);
	void onCommitXd(LLUICtrl* ctrl, const LLSD& param);
	void onCommitYd(LLUICtrl* ctrl, const LLSD& param);
	void onCommitZd(LLUICtrl* ctrl, const LLSD& param);*/

	void onMouseLeave(S32 x, S32 y, MASK mask);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);
};

extern LLTopBar *gTopBar;

#endif
