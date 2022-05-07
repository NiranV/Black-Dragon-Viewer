/** 
 * @file llfloaterwindowsize.cpp
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
 
#include "llviewerprecompiledheaders.h"

#include "llfloaterwindowsize.h"

// Viewer includes
#include "llviewerwindow.h"

// Linden library includes
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llregex.h"
#include "lluictrl.h"

//BD
#include "llspinctrl.h"
#include "llviewercontrol.h"

// Extract from strings of the form "<width> x <height>", e.g. "640 x 480".
bool extractWindowSizeFromString(const std::string& instr, U32 *width, U32 *height)
{
	boost::cmatch what;
	// matches (any number)(any non-number)(any number)
	const boost::regex expression("([0-9]+)[^0-9]+([0-9]+)");
	if (ll_regex_match(instr.c_str(), what, expression))
	{
		*width = atoi(what[1].first);
		*height = atoi(what[2].first);
		return true;
	}
	
	*width = 0;
	*height = 0;
	return false;
}


LLFloaterWindowSize::LLFloaterWindowSize(const LLSD& key) 
:       LLFloater(key)
{}

LLFloaterWindowSize::~LLFloaterWindowSize()
{}

BOOL LLFloaterWindowSize::postBuild()
{
	//BD
    center();
    getChild<LLUICtrl>("set_btn")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClickSet, this));
    getChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClickCancel, this));
    setDefaultBtn("set_btn");

	mWindowWidth = getChild<LLSpinCtrl>("window_x");
	mWindowHeight = getChild<LLSpinCtrl>("window_y");

	//Resolution template buttons
	getChild<LLButton>("1024 x 768")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick768, this));
	getChild<LLButton>("1280 x 720")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick720, this));
	getChild<LLButton>("1440 x 900")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick900, this));
	getChild<LLButton>("1540 x 960")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick960, this));

	initWindowSizeControls();

    return TRUE;
}

void LLFloaterWindowSize::initWindowSizeControls()
{
	//BD
    // Look to see if current window size matches existing window sizes, if so then
    // just set the selection value...
    S32 height = gViewerWindow->getWindowHeightRaw();
    S32 width = gViewerWindow->getWindowWidthRaw();
	//BD - Ugh... make the spinners show the correct value on first creation
	height = height - 38;
	width = width - 16;

	mWindowWidth->setValue(width);
	mWindowHeight->setValue(height);
}

void LLFloaterWindowSize::onClickSet()
{
	//BD
	S32 width = mWindowWidth->getValue().asInteger();
	S32 height = mWindowHeight->getValue().asInteger();

	LLViewerWindow::movieSize(width , height);
}

void LLFloaterWindowSize::onClickCancel()
{
	closeFloater();
}

//BD - TODO: Find a better way to do this
void LLFloaterWindowSize::onClick768()
{
	mWindowWidth->setValue(1024);
	mWindowHeight->setValue(768);
	onClickSet();
}
void LLFloaterWindowSize::onClick720()
{
	mWindowWidth->setValue(1280);
	mWindowHeight->setValue(720);
	onClickSet();
}
void LLFloaterWindowSize::onClick900()
{
	mWindowWidth->setValue(1440);
	mWindowHeight->setValue(900);
	onClickSet();
}
void LLFloaterWindowSize::onClick960()
{
	mWindowWidth->setValue(1540);
	mWindowHeight->setValue(960);
	onClickSet();
}