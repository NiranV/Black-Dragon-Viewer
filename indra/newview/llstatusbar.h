/**
 * @file llstatusbar.h
 * @brief LLStatusBar class definition
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

#ifndef LL_LLSTATUSBAR_H
#define LL_LLSTATUSBAR_H

#include "llpanel.h"
#include "llsearchableui.h"

// "Constants" loaded from settings.xml at start time
extern S32 STATUS_BAR_HEIGHT;

class LLButton;
class LLLineEditor;
class LLMessageSystem;
class LLTextBox;
class LLTextEditor;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLStatGraph;
class LLPanelPresetsPulldown;
class LLPanelVolumePulldown;
class LLPanelNearByMedia;

class LLIconCtrl;
class LLSearchEditor;
class LLSearchableUI;
//BD - Quick Draw Distance Slider
class BDPanelDrawDistance;


class LLStatusBar
:   public LLPanel
{
public:
    LLStatusBar(const LLRect& rect );
    /*virtual*/ ~LLStatusBar();

    /*virtual*/ void draw();

    /*virtual*/ bool handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool postBuild();

	void		setHealth(S32 percent);

	//BD
	void		setMediaEnabled(bool enabled) { mMediaEnabled = enabled; }
	void		setShowNetStats(bool enabled);

	void setLandCredit(S32 credit);
	void setLandCommitted(S32 committed);

    void        refresh();
    void setVisibleForMouselook(bool visible);
        // some elements should hide in mouselook

	// ACCESSORS
	S32			getHealth() const;

    bool isUserTiered() const;
    S32 getSquareMetersCredit() const;
    S32 getSquareMetersCommitted() const;
    S32 getSquareMetersLeft() const;

    LLPanelNearByMedia* getNearbyMediaPanel() { return mPanelNearByMedia; }

private:
	
	void onVolumeChanged(const LLSD& newvalue);
    void onVoiceChanged(const LLSD& newvalue);

	void onMouseEnterPresets();
	void onMouseEnterVolume();
	void onMouseEnterNearbyMedia();
//	//BD - Quick Draw Distance Slider
	void onMouseEnterDrawDistance();

//	//BD - Statusbar Framerate Count
	void onToggleFPSType();

	static void onClickMediaToggle(void* data);

private:
	LLTextBox	*mTextTime;
//	//BD - Statusbar Framerate Count
	LLTextBox	*mFPSText;
	LLFrameTimer	mFPSUpdateTimer;
	LLFrameTimer	mFPSHistoryTimer;
	S32			mFPSType;
	LLVector2	mFPSHistory;

	LLStatGraph *mSGBandwidth;
	LLStatGraph *mSGPacketLoss;

	LLIconCtrl  *mIconPresets;
	LLButton	*mBtnVolume;
	LLTextBox	*mBoxBalance;
	LLButton	*mMediaToggle;
	LLFrameTimer	mClockUpdateTimer;
//	//BD - Quick Draw Distance Slider
	LLIconCtrl	*mDrawDistance;

	S32				mHealth;
	S32				mSquareMetersCredit;
	S32				mSquareMetersCommitted;
	LLFrameTimer*	mHealthTimer;
	LLPanelPresetsPulldown* mPanelPresetsPulldown;
	LLPanelVolumePulldown* mPanelVolumePulldown;
	LLPanelNearByMedia*	mPanelNearByMedia;
//	//BD - Quick Draw Distance Slider
	BDPanelDrawDistance* mPanelDrawDistance;

	bool	mMediaEnabled;
	bool	mShowNetStats;
};

// *HACK: Status bar owns your cached money balance. JC
bool can_afford_transaction(S32 cost);

extern LLStatusBar *gStatusBar;

#endif
