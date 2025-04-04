/**
 * @file llpopupview.h
 * @brief Holds transient popups
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

#ifndef LL_LLPOPUPVIEW_H
#define LL_LLPOPUPVIEW_H

#include "llpanel.h"

class LLPopupView : public LLPanel
{
public:
    LLPopupView(const Params& p = LLPanel::Params());
    ~LLPopupView();

    /*virtual*/ void draw();
    /*virtual*/ bool handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleMiddleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleMiddleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleRightMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleHover(S32 x, S32 y, MASK mask);
    //BD - UI Improvements
	/*virtual*/ bool handleScrollWheel(S32 x, S32 y, S32 clicks, MASK mask);
	/*virtual*/ bool handleToolTip(S32 x, S32 y, MASK mask);

    void addPopup(LLView* popup);
    void removePopup(LLView* popup);
    void clearPopups();

    typedef std::list<LLHandle<LLView> > popup_list_t;
    popup_list_t getCurrentPopups() { return mPopups; }

private:
    bool handleMouseEvent(boost::function<bool(LLView*, S32, S32)>, boost::function<bool(LLView*)>, S32 x, S32 y, bool close_popups);
    popup_list_t mPopups;
};
#endif //LL_LLROOTVIEW_H
