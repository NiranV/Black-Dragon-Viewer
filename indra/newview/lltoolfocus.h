/**
 * @file lltoolfocus.h
 * @brief A tool to set the build focus point.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLTOOLFOCUS_H
#define LL_LLTOOLFOCUS_H

#include "lltool.h"

class LLPickInfo;

class LLToolCamera
:   public LLTool, public LLSingleton<LLToolCamera>
{
    LLSINGLETON(LLToolCamera);
    virtual ~LLToolCamera();
public:

    virtual bool    handleMouseDown(S32 x, S32 y, MASK mask) override;
    virtual bool    handleMouseUp(S32 x, S32 y, MASK mask) override;
    virtual bool    handleHover(S32 x, S32 y, MASK mask) override;
//	//BD - Right Click Steering
	virtual bool	handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	virtual bool	handleRightMouseUp(S32 x, S32 y, MASK mask) override;

    virtual void    onMouseCaptureLost() override;

    virtual void    handleSelect() override;
    virtual void    handleDeselect() override;

    virtual LLTool* getOverrideTool(MASK mask) override { return NULL; }

    void setClickPickPending() { mClickPickPending = true; }
    static void pickCallback(const LLPickInfo& pick_info);
    bool mouseSteerMode() { return mMouseSteering; }

//	//BD - Right Click Steering
	bool	mRightMouse;

protected:
    // called from handleMouseUp and onMouseCaptureLost to "let go"
    // of the mouse and make it visible JC
    void releaseMouse();

protected:
    S32     mAccumX;
    S32     mAccumY;
    S32     mMouseDownX;
    S32     mMouseDownY;
    bool    mOutsideSlopX;
    bool    mOutsideSlopY;
    bool    mValidClickPoint;
    bool    mClickPickPending;
    bool    mValidSelection;
    bool    mMouseSteering;
    S32     mMouseUpX;  // needed for releaseMouse()
    S32     mMouseUpY;
    MASK    mMouseUpMask;
//	//BD - Right Click Steering
	bool	mOutsideSlopRightX;
	bool	mOutsideSlopRightY;
	S32		mMouseRightUpX;
	S32		mMouseRightUpY;
};


extern bool gCameraBtnOrbit;
extern bool gCameraBtnPan;
extern bool gCameraBtnZoom;

#endif
