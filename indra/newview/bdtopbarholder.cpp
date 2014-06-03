/** 
* @file nvtopbarholder.cpp
* @brief LLTopBar class implementation
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

#include "llviewerprecompiledheaders.h"

#include "bdtopbarholder.h"

// viewer includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llframetimer.h"

// system includes
#include <iomanip>

LLTopBar *gTopBar = NULL;

LLTopBar::LLTopBar(const LLRect& rect)
:	LLPanel()
{
	buildFromFile("panel_top_bar.xml");
}

LLTopBar::~LLTopBar()
{
}

// virtual
void LLTopBar::draw()
{
	//NV - Get the time in seconds after which the topbar should hide.
	//F32 hide_time = 10.f;

	//NV - If time runs out, hide everything.
	/*if(mHoverTimer.getElapsedTimeF32() > hide_time)
	{
		hideTopbar();
	}*/
	
	//NV - If we disable the feature, unhide everything, including
	//     the Favoritesbar if we had it enabled previously, but dont
	//     do it endlessly otherwise we cannot enable the Favoritebar!
	/*if(!gSavedSettings.getBOOL("AutohideTopbar") && !mStop)
	{
		unhideTopbar();
	}*/
	mShown = gSavedSettings.getBOOL("HideTopbar");
	LLPanel::draw();
}

BOOL LLTopBar::postBuild()
{
	//NV - Make sure we keep the topbar from doing anything bad before
	//     we havnt enabled or disabled the autohiding feature not even
	//     once yet.
	//mStop = true;

	mShown = gSavedSettings.getBOOL("HideTopbar");
	return TRUE;
}

void LLTopBar::onAutohideChange()
{
	//NV - Whenever the "AutohideTopbar" Debug is changed we will be
	//     send here to handle that change properly.

	//NV - We just enabled AutohideTopbar and were send here, in this
	//     case AutohideTopbar should be on when we arrive.
	/*if(gSavedSettings.getBOOL("AutohideTopbar"))
	{
		//NV - Does "PrevFavbarUsed" exist? If not, create it.
		if(!gSavedPerAccountSettings.controlExists("PrevFavbarUsed"))
		{
			gSavedPerAccountSettings.declareBOOL("PrevFavbarUsed", 1, 
			"Used for determining if the User had the Favoritesbar active before enabling AutohideTopbar", 1);
		}
		//NV - Now save our current Favoritebar status it into the 
		//     (previously created) debug.
		gSavedPerAccountSettings.setBOOL("PrevFavbarUsed", gSavedSettings.getBOOL("ShowNavbarFavoritesPanel"));

		//NV - Make sure the Hovertimer is reset and started.
		mHoverTimer.reset();
		mHoverTimer.start();
	}
	else
	{
		//NV - If we get here, that means that AutohideTopbar debug was 
		//     changed and we just turned it off. In this case, set the
		//     previous value to our current, either making the Favoritesbar
		//     unhide again or stay hidden, depending on what the user had before
		//     he enabled this autohide function.
		gSavedPerAccountSettings.setBOOL("ShowNavbarFavoritesPanel", gSavedSettings.getBOOL("PrevFavbarUsed"));

		//NV - Make sure the Hovetimer is reset and stopped.
		mHoverTimer.reset();
		mHoverTimer.stop();
	}*/
}

void LLTopBar::hideTopbar()
{
	gSavedSettings.setBOOL("HideTopbar", true);
	//gSavedSettings.setBOOL("ShowNavbarFavoritesPanel", false);
	//mStop = false;
	//mShown = true;
}

void LLTopBar::unhideTopbar()
{
	//gSavedSettings.setBOOL("ShowNavbarFavoritesPanel", gSavedPerAccountSettings.getBOOL("PrevFavbarUsed"));
	gSavedSettings.setBOOL("HideTopbar", false);
	//mStop = true;
	//mShown = false;
}

void LLTopBar::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseEnter(x, y, mask);

	//if(gSavedSettings.getBOOL("AutohideTopbar"))
	//{
		//NV - We entered the Topbar region, reset our timer and stop
		//     it to prevent it from running out while our mouse is
		//     still in the Topbar region.
		//mHoverTimer.reset();
		//mHoverTimer.pause();

		//NV - Unhide everything.
		//unhideTopbar();
	//}
}

void LLTopBar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseLeave(x, y, mask);
	//if(gSavedSettings.getBOOL("AutohideTopbar"))
	//{
		//NV - We left the Topbar region again, resume the timer.
		//mHoverTimer.unpause();
	//}
} 

BOOL LLTopBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL ret;
	if(mShown)
	{
		unhideTopbar();
		ret = TRUE;
	}
	else
	{
		ret = FALSE;
	}
	return ret;
}