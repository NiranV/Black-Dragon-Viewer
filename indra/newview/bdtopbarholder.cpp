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
#include "llfloaterpreference.h"

// system includes
#include <iomanip>

LLTopBar *gTopBar = NULL;

LLTopBar::LLTopBar(const LLRect& rect)
:	LLPanel()
{
//	//BD - Vector4
	mCommitCallbackRegistrar.add("Pref.ArrayVec4X", boost::bind(&LLFloaterPreference::onCommitVec4X, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Y", boost::bind(&LLFloaterPreference::onCommitVec4Y, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4Z", boost::bind(&LLFloaterPreference::onCommitVec4Z, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayVec4W", boost::bind(&LLFloaterPreference::onCommitVec4W, _1, _2));

//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX", boost::bind(&LLFloaterPreference::onCommitX, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY", boost::bind(&LLFloaterPreference::onCommitY, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ", boost::bind(&LLFloaterPreference::onCommitZ, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayXD", boost::bind(&LLFloaterPreference::onCommitXd, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayYD", boost::bind(&LLFloaterPreference::onCommitYd, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZD", boost::bind(&LLFloaterPreference::onCommitZd, _1, _2));

	buildFromFile("panel_machinima.xml");
}

LLTopBar::~LLTopBar()
{
}

// virtual
void LLTopBar::draw()
{
	LLPanel::draw();
}

BOOL LLTopBar::postBuild()
{
	refreshGraphicControls();
	return TRUE;
}

/*
//BD - Vector4
void LLFloaterPreference::onCommitVec4W(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitVec4W(ctrl, param);
}

void LLFloaterPreference::onCommitVec4X(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitVec4X(ctrl, param);
}

void LLFloaterPreference::onCommitVec4Y(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitVec4Y(ctrl, param);
}

void LLFloaterPreference::onCommitVec4Z(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitVec4Z(ctrl, param);
}

//BD - Array Debugs
void LLFloaterPreference::onCommitX(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitX(ctrl, param);
}

void LLFloaterPreference::onCommitY(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitX(ctrl, param);
}

void LLFloaterPreference::onCommitZ(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitX(ctrl, param);
}

void LLFloaterPreference::onCommitXd(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitXd(ctrl, param);
}

void LLFloaterPreference::onCommitYd(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitYd(ctrl, param);
}

void LLFloaterPreference::onCommitZd(LLUICtrl* ctrl, const LLSD& param)
{
	LLFloaterPreference::onCommitZd(ctrl, param);
}*/

//BD - Refresh all controls
void LLTopBar::refreshGraphicControls()
{
	getChild<LLUICtrl>("RenderShadowResolution_X")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VX]);
	getChild<LLUICtrl>("RenderShadowResolution_Y")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VY]);
	getChild<LLUICtrl>("RenderShadowResolution_Z")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VZ]);
	getChild<LLUICtrl>("RenderShadowResolution_W")->setValue(gSavedSettings.getVector4("RenderShadowResolution").mV[VW]);

	getChild<LLUICtrl>("ExodusRenderVignette_X")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VX]);
	getChild<LLUICtrl>("ExodusRenderVignette_Y")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VY]);
	getChild<LLUICtrl>("ExodusRenderVignette_Z")->setValue(gSavedSettings.getVector3("ExodusRenderVignette").mV[VZ]);
}

void LLTopBar::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseEnter(x, y, mask);
}

void LLTopBar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseLeave(x, y, mask);
} 

BOOL LLTopBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x, y, mask);
}