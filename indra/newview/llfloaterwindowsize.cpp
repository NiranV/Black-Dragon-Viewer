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
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrl.h"
#include "llviewercontrol.h"

// System libraries
#include <boost/regex.hpp>

// Extract from strings of the form "<width> x <height>", e.g. "640 x 480".
bool extractWindowSizeFromString(const std::string& instr, U32 *width, U32 *height)
{
    boost::cmatch what;
    // matches (any number)(any non-number)(any number)
    const boost::regex expression("([0-9]+)[^0-9]+([0-9]+)");
    if (boost::regex_match(instr.c_str(), what, expression))
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
    center();
    initWindowSizeControls();
    getChild<LLUICtrl>("set_btn")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClickSet, this));
    getChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClickCancel, this));
    setDefaultBtn("set_btn");

	//Resolution template buttons
	getChild<LLButton>("800 x 600")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick600, this));
	getChild<LLButton>("1024 x 768")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick768, this));
	getChild<LLButton>("1280 x 720")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick720, this));
	getChild<LLButton>("1440 x 900")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick900, this));
	getChild<LLButton>("1540 x 960")->setCommitCallback(boost::bind(&LLFloaterWindowSize::onClick960, this));

    return TRUE;
}

void LLFloaterWindowSize::initWindowSizeControls()
{
    LLComboBox* ctrl_window_size = getChild<LLComboBox>("window_size_combo");
        
    // Look to see if current window size matches existing window sizes, if so then
    // just set the selection value...
    U32 height = gViewerWindow->getWindowHeightRaw();
    U32 width = gViewerWindow->getWindowWidthRaw();
	// ugh... make the spinners show the correct value on first creation
	height = height - 38;
	width = width - 16;
    for (S32 i=0; i < ctrl_window_size->getItemCount(); i++)
    {
        U32 height_test = 0;
        U32 width_test = 0;
        ctrl_window_size->setCurrentByIndex(i);
        std::string resolution = ctrl_window_size->getValue().asString();
        if (extractWindowSizeFromString(resolution, &width_test, &height_test))
        {
            if ((height_test == height) && (width_test == width))
            {
				return;
            }
        }
    }
    // ...otherwise, add a new entry with the current window height/width.
    LLUIString resolution_label = getString("resolution_format");
    resolution_label.setArg("[RES_X]", llformat("%d", width));
    resolution_label.setArg("[RES_Y]", llformat("%d", height));
    ctrl_window_size->add(resolution_label, ADD_TOP);
    ctrl_window_size->setCurrentByIndex(0);
}

void LLFloaterWindowSize::onClickSet()
{
	LLSpinCtrl* window_size_x = getChild<LLSpinCtrl>("window_x");
	LLSpinCtrl* window_size_y = getChild<LLSpinCtrl>("window_y");
	S32 width = window_size_x->getValue();
	S32 height = window_size_y->getValue();

	LLViewerWindow::movieSize(width , height);
}

void LLFloaterWindowSize::onClickCancel()
{
	closeFloater();
}

//TODO: find a better way to do this
void LLFloaterWindowSize::onClick600()
{
    gSavedSettings.setS32("WindowWidth",800);
	gSavedSettings.setS32("WindowHeight",600);
	onClickSet();
}
void LLFloaterWindowSize::onClick768()
{
    gSavedSettings.setS32("WindowWidth",1024);
	gSavedSettings.setS32("WindowHeight",768);
	onClickSet();
}
void LLFloaterWindowSize::onClick720()
{
    gSavedSettings.setS32("WindowWidth",1280);
	gSavedSettings.setS32("WindowHeight",720);
	onClickSet();
}
void LLFloaterWindowSize::onClick900()
{
    gSavedSettings.setS32("WindowWidth",1440);
	gSavedSettings.setS32("WindowHeight",900);
	onClickSet();
}
void LLFloaterWindowSize::onClick960()
{
    gSavedSettings.setS32("WindowWidth",1540);
	gSavedSettings.setS32("WindowHeight",960);
	onClickSet();
}