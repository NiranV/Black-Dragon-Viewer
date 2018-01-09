/** 
 * @file bdsidebar.h
 * @brief LLSideBar class definition
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

#ifndef LL_BDSIDEBAR_H
#define LL_BDSIDEBAR_H

#include "llpanel.h"
#include "lluictrl.h"
#include "llsliderctrl.h"

class LLUICtrl;
class LLSliderCtrl;

class LLSideBar
:	public LLPanel
{
public:
	LLSideBar(const LLRect& rect);
	/*virtual*/ ~LLSideBar();
	
	/*virtual*/ void draw();

	/*virtual*/ BOOL postBuild();

	void refreshGraphicControls();
	void setVisibleForMouselook(bool visible);
private:

	void onMouseLeave(S32 x, S32 y, MASK mask);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);

	LLUICtrl*			mShadowResX;
	LLUICtrl*			mShadowResY;
	LLUICtrl*			mShadowResZ;
	LLUICtrl*			mShadowResW;

	LLUICtrl*			mShadowDistX;
	LLUICtrl*			mShadowDistY;
	LLUICtrl*			mShadowDistZ;
	LLUICtrl*			mShadowDistW;

	LLUICtrl*			mProjectorResX;
	LLUICtrl*			mProjectorResY;

	LLUICtrl*			mVignetteX;
	LLUICtrl*			mVignetteY;
	LLUICtrl*			mVignetteZ;

	LLSliderCtrl*		mCameraAngle;
};

extern LLSideBar *gSideBar;

#endif
