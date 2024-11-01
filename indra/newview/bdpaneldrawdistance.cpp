/** 
 * @file bdpaneldrawdistance.cpp
 * @author NiranV Dean
 * @brief A panel showing the draw distance pulldown
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "bdpaneldrawdistance.h"

// Viewer libs
#include "llstatusbar.h"

// Linden libs
#include "llsliderctrl.h"

/* static */ const F32 BDPanelDrawDistance::sAutoCloseFadeStartTimeSec = 4.0f;
/* static */ const F32 BDPanelDrawDistance::sAutoCloseTotalTimeSec = 5.0f;

///----------------------------------------------------------------------------
/// Class BDPanelDrawDistance
///----------------------------------------------------------------------------

// Default constructor
BDPanelDrawDistance::BDPanelDrawDistance()
{
	mHoverTimer.stop();

	buildFromFile( "panel_draw_distance.xml");
}

bool BDPanelDrawDistance::postBuild()
{
	return LLPanel::postBuild();
}

/*virtual*/
void BDPanelDrawDistance::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}

/*virtual*/
void BDPanelDrawDistance::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
void BDPanelDrawDistance::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void BDPanelDrawDistance::onVisibilityChange ( bool new_visibility )
{
	if (new_visibility)	
	{
		mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
	}
	else
	{
		mHoverTimer.stop();

	}
}

//virtual
void BDPanelDrawDistance::draw()
{
	F32 alpha = mHoverTimer.getStarted() 
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), sAutoCloseFadeStartTimeSec, sAutoCloseTotalTimeSec, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	LLPanel::draw();

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}
}

