/** 
* @file bdsidebar.cpp
* @brief LLSideBar class implementation
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

#include "bdsidebar.h"

// viewer includes
#include "llfloaterpreference.h"
#include "llviewercamera.h"
#include "pipeline.h"

#include "llsliderctrl.h"

// system includes
#include <iomanip>

LLSideBar *gSideBar = NULL;

LLSideBar::LLSideBar(const LLRect& rect)
:	LLPanel()
{
//	//BD - Array Debugs
	mCommitCallbackRegistrar.add("Pref.ArrayX", boost::bind(&LLFloaterPreference::onCommitX, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayY", boost::bind(&LLFloaterPreference::onCommitY, _1, _2));
	mCommitCallbackRegistrar.add("Pref.ArrayZ", boost::bind(&LLFloaterPreference::onCommitZ, _1, _2));
//	//BD - Vector4
	mCommitCallbackRegistrar.add("Pref.ArrayW", boost::bind(&LLFloaterPreference::onCommitW, _1, _2));

	buildFromFile("panel_machinima.xml");
}

LLSideBar::~LLSideBar()
{
}

// virtual
void LLSideBar::draw()
{
	if (gSideBar->getParentByType<LLLayoutPanel>()->getVisibleAmount() > 0.01f)
	{
		mShadowDistY->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
		mShadowDistZ->setEnabled(!gPipeline.RenderShadowAutomaticDistance);
		mShadowDistW->setEnabled(!gPipeline.RenderShadowAutomaticDistance);

		//refreshGraphicControls();
		LLPanel::draw();
	}
}

BOOL LLSideBar::postBuild()
{
	mShadowResX = getChild<LLUICtrl>("RenderShadowResolution_X");
	mShadowResY = getChild<LLUICtrl>("RenderShadowResolution_Y");
	mShadowResZ = getChild<LLUICtrl>("RenderShadowResolution_Z");
	mShadowResW = getChild<LLUICtrl>("RenderShadowResolution_W");

	mShadowDistX = getChild<LLUICtrl>("RenderShadowDistance_X");
	mShadowDistY = getChild<LLUICtrl>("RenderShadowDistance_Y");
	mShadowDistZ = getChild<LLUICtrl>("RenderShadowDistance_Z");
	mShadowDistW = getChild<LLUICtrl>("RenderShadowDistance_W");

	mProjectorResX = getChild<LLUICtrl>("RenderProjectorShadowResolution_X");
	mProjectorResY = getChild<LLUICtrl>("RenderProjectorShadowResolution_Y");

	mVignetteX = getChild<LLUICtrl>("ExodusRenderVignette_X");
	mVignetteY = getChild<LLUICtrl>("ExodusRenderVignette_Y");
	mVignetteZ = getChild<LLUICtrl>("ExodusRenderVignette_Z");

	mCameraAngle = getChild<LLSliderCtrl>("CameraAngle");
	return TRUE;
}

//BD - Refresh all controls
void LLSideBar::refreshGraphicControls()
{
	LLVector4 vec4 = gSavedSettings.getVector4("RenderShadowResolution");
	mShadowResX->setValue(vec4.mV[VX]);
	mShadowResY->setValue(vec4.mV[VY]);
	mShadowResZ->setValue(vec4.mV[VZ]);
	mShadowResW->setValue(vec4.mV[VW]);

	vec4 = gSavedSettings.getVector4("RenderShadowDistance");
	mShadowDistX->setValue(vec4.mV[VX]);
	mShadowDistY->setValue(vec4.mV[VY]);
	mShadowDistZ->setValue(vec4.mV[VZ]);
	mShadowDistW->setValue(vec4.mV[VW]);

	LLVector2 vec2 = gSavedSettings.getVector2("RenderProjectorShadowResolution");
	mProjectorResX->setValue(vec2.mV[VX]);
	mProjectorResY->setValue(vec2.mV[VY]);

	LLVector3 vec3 = gSavedSettings.getVector3("ExodusRenderVignette");
	mVignetteX->setValue(vec3.mV[VX]);
	mVignetteY->setValue(vec3.mV[VY]);
	mVignetteZ->setValue(vec3.mV[VZ]);

	LLViewerCamera* viewer_camera = LLViewerCamera::getInstance();
	mCameraAngle->setMaxValue(viewer_camera->getMaxView());
	mCameraAngle->setMinValue(viewer_camera->getMinView());
}

void LLSideBar::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseEnter(x, y, mask);
}

void LLSideBar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLPanel::onMouseLeave(x, y, mask);
} 

BOOL LLSideBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x, y, mask);
}

void LLSideBar::setVisibleForMouselook(bool visible)
{
	//	//BD - Hide UI In Mouselook
	if (gSavedSettings.getBOOL("AllowUIHidingInML"))
	{
		this->setVisible(visible);
	}
}