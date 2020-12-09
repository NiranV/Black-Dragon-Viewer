/**
 * @file llprogressview.h
 * @brief LLProgressView class definition
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

#ifndef LL_LLPROGRESSVIEW_H
#define LL_LLPROGRESSVIEW_H

#include "llpanel.h"
#include "llframetimer.h"
#include "llevents.h"
//BD - TODO: Remove?
#include <boost/concept_check.hpp>

//BD
#include "lltextbox.h"

class LLImageRaw;
class LLButton;
class LLProgressBar;

class LLProgressView : 
	//BD
	public LLPanel

{
public:
	LLProgressView();
	virtual ~LLProgressView();
	
	BOOL postBuild();

	/*virtual*/ void draw();
	// ## Zi: Fade teleport screens
	//void drawStartTexture(F32 alpha);

	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);

	//BD
	void setProgressVisible(BOOL visible, BOOL logout = false);

	void setPercent(const F32 percent);

	//BD
	// Set a random Tip every X seconds
	void setTip();
	void setText(const std::string& string);
	void setMessage(const std::string& string);
	
	// turns on (under certain circumstances) the into video after login
	//void revealIntroPanel();

	void fade(BOOL in);		// ## Zi: Fade teleport screens

	void setStartupComplete();

	void setCancelButtonVisible(BOOL b, const std::string& label);

	static void onCancelButtonClicked( void* );
	bool onAlertModal(const LLSD& sd);

	//BD
	LLUICtrl* mMessageText;
	LLUICtrl* mStateText;
	LLUICtrl* mStatusText;
	LLTextBox* mPercentText;

	// note - this is not just hiding the intro panel - it also hides the parent panel
	// and is used when the intro is finished and we want to show the world
	void removeIntroPanel();

protected:
	LLProgressBar* mProgressBar;
	F32 mPercentDone;
	LLButton*	mCancelBtn;
	LLFrameTimer mFadeToWorldTimer;
	LLFrameTimer mFadeFromLoginTimer;
	//BD
	LLFrameTimer mTipCycleTimer;
	//std::string mMessage;

	LLRect mOutlineRect;

	bool mMouseDownInActiveArea;
	bool mStartupComplete;

	// The LLEventStream mUpdateEvents depends upon this class being a singleton
	// to avoid pump name conflicts.
	static LLProgressView* sInstance;
	LLEventStream mUpdateEvents; 

	bool handleUpdate(const LLSD& event_data);
};

#endif // LL_LLPROGRESSVIEW_H
