/** 
 * @file lltoolfocus.cpp
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

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolfocus.h" 

// Library includes
#include "v3math.h"
#include "llfontgl.h"
#include "llui.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "lltooltip.h"
#include "llhudmanager.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llmorphview.h"
#include "llfloaterreg.h"
#include "llfloatercamera.h"
#include "llmenugl.h"
#include "lltoolpie.h"

// Globals
BOOL gCameraBtnZoom = TRUE;
BOOL gCameraBtnOrbit = FALSE;
BOOL gCameraBtnPan = FALSE;

const S32 SLOP_RANGE = 4;
const S32 SLOP_RANGE_RIGHT = 24;

//
// Camera - shared functionality
//

LLToolCamera::LLToolCamera()
:	LLTool(std::string("Camera")),
	mAccumX(0),
	mAccumY(0),
	mMouseDownX(0),
	mMouseDownY(0),
	mOutsideSlopX(FALSE),
	mOutsideSlopY(FALSE),
	mOutsideSlopRightX(FALSE),
	mOutsideSlopRightY(FALSE),
	mValidClickPoint(FALSE),
	mValidSelection(FALSE),
	mMouseSteering(FALSE),
	mRightMouse(FALSE),
	mMouseUpX(0),
	mMouseUpY(0),
	mMouseRightUpX(0),
	mMouseRightUpY(0),
	mMouseUpMask(MASK_NONE)
{ }


LLToolCamera::~LLToolCamera()
{ }

// virtual
void LLToolCamera::handleSelect()
{
	if (gFloaterTools)
	{
		gFloaterTools->setStatusText("camera");
		// in case we start from tools floater, we count any selection as valid
		mValidSelection = gFloaterTools->getVisible();
	}
}

// virtual
void LLToolCamera::handleDeselect()
{
//	gAgent.setLookingAtAvatar(FALSE);

	// Make sure that temporary selection won't pass anywhere except pie tool.
	MASK override_mask = gKeyboard ? gKeyboard->currentMask(TRUE) : 0;
	if (!mValidSelection && (override_mask != MASK_NONE || (gFloaterTools && gFloaterTools->getVisible())))
	{
		LLMenuGL::sMenuContainer->hideMenus();
		LLSelectMgr::getInstance()->validateSelection();
	}
}

BOOL LLToolCamera::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Ensure a mouseup
	setMouseCapture(TRUE);

	// call the base class to propogate info to sim
	LLTool::handleMouseDown(x, y, mask);

	mAccumX = 0;
	mAccumY = 0;

	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;

	mValidClickPoint = FALSE;

	// If mouse capture gets ripped away, claim we moused up
	// at the point we moused down. JC
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	gViewerWindow->hideCursor();

	gViewerWindow->pickAsync(x, y, mask, pickCallback, /*BOOL pick_transparent*/ FALSE, /*BOOL pick_rigged*/ FALSE, /*BOOL pick_unselectable*/ TRUE);

	return TRUE;
}

//BD - Hold Right-Mouse-Button to turn
BOOL LLToolCamera::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// Ensure a mouseup
	setMouseCapture(TRUE);

	mAccumX = 0;
	mAccumY = 0;

	// If mouse capture gets ripped away, claim we moused up
	// at the point we moused down. JC
	mMouseRightUpX = x;
	mMouseRightUpY = y;
	mMouseUpMask = mask;

	mOutsideSlopRightX = FALSE;
	mOutsideSlopRightY = FALSE;

	mRightMouse = TRUE;

	return TRUE;
}

void LLToolCamera::pickCallback(const LLPickInfo& pick_info)
{
	if (!LLToolCamera::getInstance()->hasMouseCapture())
	{
		return;
	}

	LLToolCamera::getInstance()->mMouseDownX = pick_info.mMousePt.mX;
	LLToolCamera::getInstance()->mMouseDownY = pick_info.mMousePt.mY;

	gViewerWindow->moveCursorToCenter();

	// Potentially recenter if click outside rectangle
	LLViewerObject* hit_obj = pick_info.getObject();

	// Check for hit the sky, or some other invalid point
	if (!hit_obj && pick_info.mPosGlobal.isExactlyZero())
	{
		LLToolCamera::getInstance()->mValidClickPoint = FALSE;
		return;
	}

	// check for hud attachments
	if (hit_obj && hit_obj->isHUDAttachment())
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		if (!selection->getObjectCount() || selection->getSelectType() != SELECT_TYPE_HUD)
		{
			LLToolCamera::getInstance()->mValidClickPoint = FALSE;
			return;
		}
	}

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
	{
		BOOL good_customize_avatar_hit = FALSE;
		if( hit_obj )
		{
			if (isAgentAvatarValid() && (hit_obj == gAgentAvatarp))
			{
				// It's you
				good_customize_avatar_hit = TRUE;
			}
			else if (hit_obj->isAttachment() && hit_obj->permYouOwner())
			{
				// It's an attachment that you're wearing
				good_customize_avatar_hit = TRUE;
			}
		}

		if( !good_customize_avatar_hit )
		{
			LLToolCamera::getInstance()->mValidClickPoint = FALSE;
			return;
		}

		if( gMorphView )
		{
			gMorphView->setCameraDrivenByKeys( FALSE );
		}
	}
	//RN: check to see if this is mouse-driving as opposed to ALT-zoom or Focus tool
	else if (pick_info.mKeyMask & MASK_ALT || 
			(LLToolMgr::getInstance()->getCurrentTool()->getName() == "Camera")) 
	{
		LLViewerObject* hit_obj = pick_info.getObject();
		if (hit_obj)
		{
			// ...clicked on a world object, so focus at its position
			if (!hit_obj->isHUDAttachment())
			{
				gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
				gAgentCamera.setFocusGlobal(pick_info);
			}
		}
		else if (!pick_info.mPosGlobal.isExactlyZero())
		{
			// Hit the ground
			gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
			gAgentCamera.setFocusGlobal(pick_info);
		}

		if (!(pick_info.mKeyMask & MASK_ALT) &&
			gAgentCamera.cameraThirdPerson() &&
			gViewerWindow->getLeftMouseDown() && 
			!gSavedSettings.getBOOL("FreezeTime") &&
			(hit_obj == gAgentAvatarp || 
			 (hit_obj && hit_obj->isAttachment() && LLVOAvatar::findAvatarFromAttachment(hit_obj)->isSelf())))
		{
			LLToolCamera::getInstance()->mMouseSteering = TRUE;
		}

	}

	LLToolCamera::getInstance()->mValidClickPoint = TRUE;

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
	{
		gAgentCamera.setFocusOnAvatar(FALSE, FALSE);
		
		LLVector3d cam_pos = gAgentCamera.getCameraPositionGlobal();

		gAgentCamera.setCameraPosAndFocusGlobal( cam_pos, pick_info.mPosGlobal, pick_info.mObjectID);
	}
}


// "Let go" of the mouse, for example on mouse up or when
// we lose mouse capture.  This ensures that cursor becomes visible
// if a modal dialog pops up during Alt-Zoom. JC
void LLToolCamera::releaseMouse()
{
	// Need to tell the sim that the mouse button is up, since this
	// tool is no longer working and cursor is visible (despite actual
	// mouse button status).
	LLTool::handleMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);

	gViewerWindow->showCursor();

	//for the situation when left click was performed on the Agent
	if (!LLFloaterCamera::inFreeCameraMode())
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}

	mMouseSteering = FALSE;
	mValidClickPoint = FALSE;
	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;
}


BOOL LLToolCamera::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// Claim that we're mousing up somewhere
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	if (hasMouseCapture())
	{
		if (mValidClickPoint)
		{
			if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() )
			{
				LLCoordGL mouse_pos;
				LLVector3 focus_pos = gAgent.getPosAgentFromGlobal(gAgentCamera.getFocusGlobal());
				BOOL success = LLViewerCamera::getInstance()->projectPosAgentToScreen(focus_pos, mouse_pos);
				if (success)
				{
					LLUI::setMousePositionScreen(mouse_pos.mX, mouse_pos.mY);
				}
			}
			else if (mMouseSteering)
			{
				LLUI::setMousePositionScreen(mMouseDownX, mMouseDownY);
			}
			else
			{
				gViewerWindow->moveCursorToCenter();
			}
		}
		else
		{
			// not a valid zoomable object
			LLUI::setMousePositionScreen(mMouseDownX, mMouseDownY);
		}

		// calls releaseMouse() internally
		setMouseCapture(FALSE);
	}
	else
	{
		releaseMouse();
	}

	return TRUE;
}

//BD - Hold Right-Mouse-Button to turn
BOOL LLToolCamera::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	gViewerWindow->showCursor();
	mRightMouse = FALSE;

	if (hasMouseCapture())
	{
		// calls releaseMouse() internally
		setMouseCapture(FALSE);
	}
	else
	{
		releaseMouse();
	}

//	//BD - Intercept here if we reached the threshold or if we are in
	//     Appearance Mode or if we are in any sort of camera movement 
	//     mode, this prevents crashing.
	if (mOutsideSlopRightX || mOutsideSlopRightY ||
		CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode() ||
		LLToolMgr::getInstance()->getCurrentTool() == LLToolCamera::getInstance())
	{
		return TRUE;
	}

//	//BD - Calculate the correct onscreen position and throw both
	//	   RightMouseDown and RightMouseUp to prevent pie menus from
	//	   staying in their initial state until clicking somewhere
	LLCoordGL pos;
	pos.mX = x * gViewerWindow->getDisplayScale().mV[VX];
	pos.mY = y * gViewerWindow->getDisplayScale().mV[VY];
	gViewerWindow->handleAnyMouseClick(gViewerWindow->getWindow(), pos, mask, LLMouseHandler::CLICK_RIGHT, TRUE);
	return gViewerWindow->handleAnyMouseClick(gViewerWindow->getWindow(), pos, mask, LLMouseHandler::CLICK_RIGHT, FALSE);
}


BOOL LLToolCamera::handleHover(S32 x, S32 y, MASK mask)
{
	S32 dx = gViewerWindow->getCurrentMouseDX();
	S32 dy = gViewerWindow->getCurrentMouseDY();
	
	if (hasMouseCapture() && mValidClickPoint || 
		(gSavedSettings.getBOOL("EnableThirdPersonSteering") &&
		!gAgentCamera.cameraMouselook()) ||
		mRightMouse)
	{
		mAccumX += llabs(dx);
		mAccumY += llabs(dy);

		if (!mRightMouse)
		{
			if (mAccumX >= SLOP_RANGE)
			{
				mOutsideSlopX = TRUE;
			}

			if (mAccumY >= SLOP_RANGE)
			{
				mOutsideSlopY = TRUE;
			}
		}
		else
		{
			if (mAccumX >= SLOP_RANGE_RIGHT)
			{
				mOutsideSlopRightX = TRUE;
			}

			if (mAccumY >= SLOP_RANGE_RIGHT)
			{
				mOutsideSlopRightY = TRUE;
			}
		}
	}

	if (mOutsideSlopX || mOutsideSlopY)
	{
		if (!mValidClickPoint && !gSavedSettings.getBOOL("EnableThirdPersonSteering"))
		{
			LL_DEBUGS("UserInput") << "hover handled by LLToolFocus [invalid point]" << LL_ENDL;
			gViewerWindow->setCursor(UI_CURSOR_NO);
			gViewerWindow->showCursor();
			return TRUE;
		}

		if (gCameraBtnOrbit ||
			mask == MASK_ORBIT || 
			mask == (MASK_ALT | MASK_ORBIT) ||
			(gSavedSettings.getBOOL("EnableThirdPersonSteering") &&
			!gAgentCamera.cameraMouselook()))
		{
			// Orbit tool
			if (hasMouseCapture())
			{
				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				if (dy != 0)
				{
					gAgentCamera.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
				}

				gViewerWindow->moveCursorToCenter();
			}
			LL_DEBUGS("UserInput") << "hover handled by LLToolFocus [active]" << LL_ENDL;
		}
		else if (	gCameraBtnPan ||
					mask == MASK_PAN ||
					mask == (MASK_PAN | MASK_ALT) )
		{
			// Pan tool
			if (hasMouseCapture())
			{
				LLVector3d camera_to_focus = gAgentCamera.getCameraPositionGlobal();
				camera_to_focus -= gAgentCamera.getFocusGlobal();
				F32 dist = (F32) camera_to_focus.normVec();

				// Fudge factor for pan
				F32 meters_per_pixel = 3.f * dist / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraPanLeft( dx * meters_per_pixel );
				}

				if (dy != 0)
				{
					gAgentCamera.cameraPanUp( -dy * meters_per_pixel );
				}

				gViewerWindow->moveCursorToCenter();
			}
			LL_DEBUGS("UserInput") << "hover handled by LLToolPan" << LL_ENDL;
		}
		else if (gCameraBtnZoom)
		{
			// Zoom tool
			if (hasMouseCapture())
			{

				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWorldViewWidthScaled();

				if (dx != 0)
				{
					gAgentCamera.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				const F32 IN_FACTOR = 0.99f;

				if (dy != 0 && mOutsideSlopY )
				{
					if (mMouseSteering)
					{
						gAgentCamera.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
					}
					else
					{
						gAgentCamera.cameraZoomIn( pow( IN_FACTOR, dy ) );
					}
				}

				gViewerWindow->moveCursorToCenter();
			}

			LL_DEBUGS("UserInput") << "hover handled by LLToolZoom" << LL_ENDL;		
		}
	}
	else if (mRightMouse && mOutsideSlopRightX || mRightMouse && mOutsideSlopRightY)
	{
//		//BD - Handle right click threshold breaks as a seperate camera tool to
		//	   prevent any unintentional left-click blocking or interceptions
		if (hasMouseCapture())
		{
			const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWorldViewWidthScaled();

			if (dx != 0)
			{
				gAgentCamera.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
			}

			if (dy != 0)
			{
				gAgentCamera.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
			}
			gViewerWindow->hideCursor();
			LLUI::setMousePositionScreen(mMouseRightUpX, mMouseRightUpY);
		}
	}

	if((gSavedSettings.getBOOL("EnableThirdPersonSteering") &&
		!gAgentCamera.cameraMouselook()) ||
		mRightMouse)
	{
		gViewerWindow->setCursor(UI_CURSOR_ARROW);
	}
	else if (gCameraBtnOrbit ||
		mask == MASK_ORBIT || 
		mask == (MASK_ALT | MASK_ORBIT))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (	gCameraBtnPan ||
				mask == MASK_PAN ||
				mask == (MASK_PAN | MASK_ALT))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}
	
	
	return TRUE;
}


void LLToolCamera::onMouseCaptureLost()
{
	releaseMouse();
}
