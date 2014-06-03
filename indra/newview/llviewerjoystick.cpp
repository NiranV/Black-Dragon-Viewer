/** 
 * @file llviewerjoystick.cpp
 * @brief Joystick / NDOF device functionality.
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

#include "llviewerjoystick.h"

#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llappviewer.h"
#include "llkeyboard.h"
#include "lltoolmgr.h"
#include "llselectmgr.h"
#include "llviewermenu.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfocusmgr.h"


// ----------------------------------------------------------------------------
// Constants

#define  X_I	1
#define  Y_I	2
#define  Z_I	0
#define RX_I	4
#define RY_I	5
#define RZ_I	3

F32  LLViewerJoystick::sLastDelta[] = {0,0,0,0,0,0,0};
F32  LLViewerJoystick::sDelta[] = {0,0,0,0,0,0,0};

// These constants specify the maximum absolute value coming in from the device.
// HACK ALERT! the value of MAX_JOYSTICK_INPUT_VALUE is not arbitrary as it 
// should be.  It has to be equal to 3000 because the SpaceNavigator on Windows
// refuses to respond to the DirectInput SetProperty call; it always returns 
// values in the [-3000, 3000] range.
#define MAX_SPACENAVIGATOR_INPUT  3000.0f
#define MAX_JOYSTICK_INPUT_VALUE  MAX_SPACENAVIGATOR_INPUT

// -----------------------------------------------------------------------------
void LLViewerJoystick::updateEnabled(bool autoenable)
{
	if (mDriverState == JDS_UNINITIALIZED)
	{
		gSavedSettings.setBOOL("JoystickEnabled", FALSE );
	}
	else
	{
		if (isLikeSpaceNavigator() && autoenable)
		{
			gSavedSettings.setBOOL("JoystickEnabled", TRUE );
		}
	}
	if (!gSavedSettings.getBOOL("JoystickEnabled"))
	{
		mOverrideCamera = FALSE;
	}
}

void LLViewerJoystick::setOverrideCamera(bool val)
{
	if (!gSavedSettings.getBOOL("JoystickEnabled"))
	{
		mOverrideCamera = FALSE;
	}
	else
	{
		mOverrideCamera = val;
	}

	if (mOverrideCamera)
	{
		gAgentCamera.changeCameraToDefault();
	}
}

// -----------------------------------------------------------------------------
#if LIB_NDOF
NDOF_HotPlugResult LLViewerJoystick::HotPlugAddCallback(NDOF_Device *dev)
{
	NDOF_HotPlugResult res = NDOF_DISCARD_HOTPLUGGED;
	LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
	if (joystick->mDriverState == JDS_UNINITIALIZED)
	{
        LL_INFOS() << "HotPlugAddCallback: will use device:" << LL_ENDL;
		ndof_dump(dev);
		joystick->mNdofDev = dev;
        joystick->mDriverState = JDS_INITIALIZED;
        res = NDOF_KEEP_HOTPLUGGED;
	}
	joystick->updateEnabled(true);
    return res;
}
#endif

// -----------------------------------------------------------------------------
#if LIB_NDOF
void LLViewerJoystick::HotPlugRemovalCallback(NDOF_Device *dev)
{
	LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
	if (joystick->mNdofDev == dev)
	{
        LL_INFOS() << "HotPlugRemovalCallback: joystick->mNdofDev=" 
				<< joystick->mNdofDev << "; removed device:" << LL_ENDL;
		ndof_dump(dev);
		joystick->mDriverState = JDS_UNINITIALIZED;
	}
	joystick->updateEnabled(true);
}
#endif

// -----------------------------------------------------------------------------
LLViewerJoystick::LLViewerJoystick()
:	mDriverState(JDS_UNINITIALIZED),
	mNdofDev(NULL),
	mResetFlag(false),
	mCameraUpdated(true),
	mOverrideCamera(false),
	mJoystickRun(0)
{
	for (int i = 0; i < 6; i++)
	{
		mAxes[i] = sDelta[i] = sLastDelta[i] = 0.0f;
	}
	
	memset(mBtn, 0, sizeof(mBtn));

	// factor in bandwidth? bandwidth = gViewerStats->mKBitStat
	mPerfScale = 4000.f / gSysCPU.getMHz(); // hmm.  why?
}

// -----------------------------------------------------------------------------
LLViewerJoystick::~LLViewerJoystick()
{
	if (mDriverState == JDS_INITIALIZED)
	{
		terminate();
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::init(bool autoenable)
{
#if LIB_NDOF
	static bool libinit = false;
	mDriverState = JDS_INITIALIZING;

	if (libinit == false)
	{
		// Note: The HotPlug callbacks are not actually getting called on Windows
		if (ndof_libinit(HotPlugAddCallback, 
						 HotPlugRemovalCallback, 
						 NULL))
		{
			mDriverState = JDS_UNINITIALIZED;
		}
		else
		{
			// NB: ndof_libinit succeeds when there's no device
			libinit = true;

			// allocate memory once for an eventual device
			mNdofDev = ndof_create();
		}
	}

	if (libinit)
	{
		if (mNdofDev)
		{
			// Different joysticks will return different ranges of raw values.
			// Since we want to handle every device in the same uniform way, 
			// we initialize the mNdofDev struct and we set the range 
			// of values we would like to receive. 
			// 
			// HACK: On Windows, libndofdev passes our range to DI with a 
			// SetProperty call. This works but with one notable exception, the
			// SpaceNavigator, who doesn't seem to care about the SetProperty
			// call. In theory, we should handle this case inside libndofdev. 
			// However, the range we're setting here is arbitrary anyway, 
			// so let's just use the SpaceNavigator range for our purposes. 
			mNdofDev->axes_min = (long)-MAX_JOYSTICK_INPUT_VALUE;
			mNdofDev->axes_max = (long)+MAX_JOYSTICK_INPUT_VALUE;

			// libndofdev could be used to return deltas.  Here we choose to
			// just have the absolute values instead.
			mNdofDev->absolute = 1;

			// init & use the first suitable NDOF device found on the USB chain
			if (ndof_init_first(mNdofDev, NULL))
			{
				mDriverState = JDS_UNINITIALIZED;
				LL_WARNS() << "ndof_init_first FAILED" << LL_ENDL;
			}
			else
			{
				mDriverState = JDS_INITIALIZED;
			}
		}
		else
		{
			mDriverState = JDS_UNINITIALIZED;
		}
	}

	// Autoenable the joystick for recognized devices if nothing was connected previously
	if (!autoenable)
	{
		autoenable = gSavedSettings.getString("JoystickInitialized").empty() ? true : false;
	}
	updateEnabled(autoenable);
	
	if (mDriverState == JDS_INITIALIZED)
	{
		// A Joystick device is plugged in
		if (isLikeSpaceNavigator())
		{
			// It's a space navigator, we have defaults for it.
			if (gSavedSettings.getString("JoystickInitialized") != "SpaceNavigator")
			{
				// Only set the defaults if we haven't already (in case they were overridden)
				setSNDefaults();
				gSavedSettings.setString("JoystickInitialized", "SpaceNavigator");
			}
		}
		else
		{
			// It's not a Space Navigator, no problem, use Xbox360 defaults.
			if (gSavedSettings.getString("JoystickInitialized") != "UnknownDevice")
			{
//				//BD - Let's put our Xbox360 defaults here because it is most likely a controller
				//     atleast similar to that of the Xbox360.
				setXboxDefaults();
				gSavedSettings.setString("JoystickInitialized", "UnknownDevice");
			}
		}
	}
	else
	{
		// No device connected, don't change any settings
	}
	
	LL_INFOS() << "ndof: mDriverState=" << mDriverState << "; mNdofDev=" 
			<< mNdofDev << "; libinit=" << libinit << LL_ENDL;
#endif
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::terminate()
{
#if LIB_NDOF

	ndof_libcleanup();
	LL_INFOS() << "Terminated connection with NDOF device." << LL_ENDL;
	mDriverState = JDS_UNINITIALIZED;
#endif
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::updateStatus()
{
#if LIB_NDOF

	ndof_update(mNdofDev);

	for (int i=0; i<6; i++)
	{
		mAxes[i] = (F32) mNdofDev->axes[i] / mNdofDev->axes_max;
	}

	for (int i=0; i<16; i++)
	{
		mBtn[i] = mNdofDev->buttons[i];
	}
	
#endif
}

// -----------------------------------------------------------------------------
F32 LLViewerJoystick::getJoystickAxis(U32 axis) const
{
	if (axis < 6)
	{
		return mAxes[axis];
	}
	return 0.f;
}

// -----------------------------------------------------------------------------
U32 LLViewerJoystick::getJoystickButton(U32 button) const
{
	if (button < 16)
	{
		return mBtn[button];
	}
	return 0;
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::handleRun(F32 inc)
{
	// Decide whether to walk or run by applying a threshold, with slight
	// hysteresis to avoid oscillating between the two with input spikes.
	// Analog speed control would be better, but not likely any time soon.
	if (inc > gSavedSettings.getF32("JoystickRunThreshold"))
	{
		if (1 == mJoystickRun)
		{
			++mJoystickRun;
//			gAgent.setRunning();
//			gAgent.sendWalkRun(gAgent.getRunning());
// [RLVa:KB] - Checked: 2011-05-11 (RLVa-1.3.0i) | Added: RLVa-1.3.0i
			gAgent.setTempRun();
// [/RLVa:KB]
		}
		else if (0 == mJoystickRun)
		{
			// hysteresis - respond NEXT frame
			++mJoystickRun;
		}
	}
	else
	{
		if (mJoystickRun > 0)
		{
			--mJoystickRun;
			if (0 == mJoystickRun)
			{
//				gAgent.clearRunning();
//				gAgent.sendWalkRun(gAgent.getRunning());
// [RLVa:KB] - Checked: 2011-05-11 (RLVa-1.3.0i) | Added: RLVa-1.3.0i
				gAgent.clearTempRun();
// [/RLVa:KB]
			}
		}
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentJump()
{
    gAgent.moveUp(1, false);
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentSlide(F32 inc)
{
	if (inc < 0.f)
	{
		gAgent.moveLeft(1, false);
	}
	else if (inc > 0.f)
	{
		gAgent.moveLeft(-1, false);
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentPush(F32 inc)
{
	if (inc < 0.f)                            // forward
	{
		gAgent.moveAt(1, false);
	}
	else if (inc > 0.f)                       // backward
	{
		gAgent.moveAt(-1, false);
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentFly(F32 inc)
{
	if (inc < 0.f)
	{
		if (! (gAgent.getFlying() ||
		       !gAgent.canFly() ||
		       gAgent.upGrabbed() ||
		       !gSavedSettings.getBOOL("AutomaticFly")) )
		{
			gAgent.setFlying(true);
		}
		gAgent.moveUp(1, false);
	}
	else if (inc > 0.f)
	{
		// crouch
		gAgent.moveUp(-1, false);
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentPitch(F32 pitch_inc)
{
	if (pitch_inc < 0)
	{
		gAgent.setControlFlags(AGENT_CONTROL_PITCH_POS);
	}
	else if (pitch_inc > 0)
	{
		gAgent.setControlFlags(AGENT_CONTROL_PITCH_NEG);
	}
	
	gAgent.pitch(-pitch_inc);
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentYaw(F32 yaw_inc)
{	
	// Cannot steer some vehicles in mouselook if the script grabs the controls
	if (gAgentCamera.cameraMouselook() && !gSavedSettings.getBOOL("JoystickMouselookYaw"))
	{
		gAgent.rotate(-yaw_inc, gAgent.getReferenceUpVector());
	}
	else
	{
		if (yaw_inc < 0)
		{
			gAgent.setControlFlags(AGENT_CONTROL_YAW_POS);
		}
		else if (yaw_inc > 0)
		{
			gAgent.setControlFlags(AGENT_CONTROL_YAW_NEG);
		}

		gAgent.yaw(-yaw_inc);
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::resetDeltas(S32 axis[])
{
	for (U32 i = 0; i < 6; i++)
	{
		sLastDelta[i] = -mAxes[axis[i]];
		sDelta[i] = 0.f;
	}

	sLastDelta[6] = sDelta[6] = 0.f;
	mResetFlag = false;
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::moveObjects(bool reset)
{
	static bool toggle_send_to_sim = false;

	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickBuildEnabled"))
	{
		return;
	}

	S32 axis[] = 
	{
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5"),
	};

	if (reset || mResetFlag)
	{
		resetDeltas(axis);
		return;
	}

	F32 axis_scale[] =
	{
		gSavedSettings.getF32("BuildAxisScale0"),
		gSavedSettings.getF32("BuildAxisScale1"),
		gSavedSettings.getF32("BuildAxisScale2"),
		gSavedSettings.getF32("BuildAxisScale3"),
		gSavedSettings.getF32("BuildAxisScale4"),
		gSavedSettings.getF32("BuildAxisScale5"),
	};

	F32 dead_zone[] =
	{
		gSavedSettings.getF32("BuildAxisDeadZone0"),
		gSavedSettings.getF32("BuildAxisDeadZone1"),
		gSavedSettings.getF32("BuildAxisDeadZone2"),
		gSavedSettings.getF32("BuildAxisDeadZone3"),
		gSavedSettings.getF32("BuildAxisDeadZone4"),
		gSavedSettings.getF32("BuildAxisDeadZone5"),
	};

	F32 cur_delta[6];
	F32 time = gFrameIntervalSeconds.value();

	// avoid making ridicously big movements if there's a big drop in fps 
	if (time > .2f)
	{
		time = .2f;
	}

	// max feather is 32
	F32 feather = gSavedSettings.getF32("BuildFeathering"); 
	bool is_zero = true, absolute = gSavedSettings.getBOOL("Cursor3D");
	
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[axis[i]];
		F32 tmp = cur_delta[i];
		if (absolute)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;
		is_zero = is_zero && (cur_delta[i] == 0.f);
			
		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}
		cur_delta[i] *= axis_scale[i];
		
		if (!absolute)
		{
			cur_delta[i] *= time;
		}

		sDelta[i] = sDelta[i] + (cur_delta[i]-sDelta[i])*time*feather;
	}

//	//BD - Remappable joystick controls
	{
		//BD - Zoom in
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomIn")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(
				LLViewerCamera::getInstance()->getDefaultFOV() * 0.99);
		}

//		//BD - Zoom Out
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomOut")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(
				LLViewerCamera::getInstance()->getDefaultFOV() * 1.01);
		}

//		//BD - Default Zoom
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomDefault")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(DEFAULT_FIELD_OF_VIEW);
		}

//		//BD - Roll Left
		if (mBtn[gSavedSettings.getS32("JoystickButtonRollLeft")] == 1)
		{
			sDelta[3] -= 0.0015f * axis_scale[3];
		}

//		//BD - Roll Right
		if (mBtn[gSavedSettings.getS32("JoystickButtonRollRight")] == 1)
		{
			sDelta[3] += 0.0015f * axis_scale[3];
		}

//		//BD - Jump / Y Up
		if (mBtn[gSavedSettings.getS32("JoystickButtonJump")] == 1)
		{
			sDelta[2] += 0.001f * axis_scale[2]; 
		}

//		//BD - Crouch / Y Down
		if (mBtn[gSavedSettings.getS32("JoystickButtonCrouch")] == 1)
		{
			sDelta[2] -= 0.001f * axis_scale[2]; 
		}
	}

	U32 upd_type = UPD_NONE;
	LLVector3 v;
    
	if (!is_zero)
	{
		// Clear AFK state if moved beyond the deadzone
		if (gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
		{
			gAgent.clearAFK();
		}
		
		if (sDelta[0] || sDelta[1] || sDelta[2])
		{
			upd_type |= UPD_POSITION;
			v.setVec(sDelta[0], sDelta[1], sDelta[2]);
		}
		
		if (sDelta[3] || sDelta[4] || sDelta[5])
		{
			upd_type |= UPD_ROTATION;
		}
				
		// the selection update could fail, so we won't send 
		if (LLSelectMgr::getInstance()->selectionMove(v, sDelta[3],sDelta[4],sDelta[5], upd_type))
		{
			toggle_send_to_sim = true;
		}
	}
	else if (toggle_send_to_sim)
	{
		LLSelectMgr::getInstance()->sendSelectionMove();
		toggle_send_to_sim = false;
	}
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::moveAvatar(bool reset)
{
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickAvatarEnabled"))
	{
		return;
	}

	S32 axis[] = 
	{
		// [1 0 2 4  3  5]
		// [Z X Y RZ RX RY]
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5")
	};

	if (reset || mResetFlag)
	{
		resetDeltas(axis);
		if (reset)
		{
			// Note: moving the agent triggers agent camera mode;
			//  don't do this every time we set mResetFlag (e.g. because we gained focus)
			gAgent.moveAt(0, true);
		}
		return;
	}

	bool is_zero = true;
	static bool button_held = false;
	static bool w_button_held = false;
	static bool m_button_held = false;

//	//BD - Remappable joystick controls
	{
//		//BD - Flying
		if (mBtn[gSavedSettings.getS32("JoystickButtonFly")] == 1 && !button_held)
		{
			button_held = true;
			if(gAgent.getFlying())
			{
				gAgent.setFlying(FALSE);
			}
			else
			{
				gAgent.setFlying(TRUE);
			}
		}
		else if (mBtn[gSavedSettings.getS32("JoystickButtonFly")] == 0 && button_held)
		{
			button_held = false;
		}

//		//BD - Jumping
		if (mBtn[gSavedSettings.getS32("JoystickButtonJump")] == 1)
		{
			agentJump();
		}

//		//BD - Crouching
		if (mBtn[gSavedSettings.getS32("JoystickButtonCrouch")] == 1)
		{
			gAgent.moveUp(-1, false);
		}

//		//BD - Walking/Running toggle
		if (mBtn[gSavedSettings.getS32("JoystickButtonRunToggle")] == 1 && !w_button_held)
		{
			w_button_held = true;
			if(gAgent.getAlwaysRun())
			{
				gAgent.clearAlwaysRun();
			}
			else
			{
				gAgent.setAlwaysRun();
			}
		}
		else if (mBtn[gSavedSettings.getS32("JoystickButtonRunToggle")] == 0 && w_button_held)
		{
			w_button_held = false;
		}

//		//BD - Zoom in
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomIn")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(
				LLViewerCamera::getInstance()->getDefaultFOV() * 0.99);
		}

//		//BD - Zoom Out
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomOut")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(
				LLViewerCamera::getInstance()->getDefaultFOV() * 1.01);
		}

//		//BD - Default Zoom
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomDefault")] == 1)
		{
			LLViewerCamera::getInstance()->setDefaultFOV(DEFAULT_FIELD_OF_VIEW);
		}

//		//BD - Toggle Mouselook
		if (mBtn[gSavedSettings.getS32("JoystickButtonMouselook")] == 1 && !m_button_held)
		{
			m_button_held = true;
			if(gAgentCamera.cameraMouselook())
			{
				gAgentCamera.changeCameraToDefault();
			}
			else
			{
				gAgentCamera.changeCameraToMouselook();
			}
		}
		else if (mBtn[gSavedSettings.getS32("JoystickButtonMouselook")] == 0 && m_button_held)
		{
			m_button_held = false;
		}
	}

	F32 axis_scale[] =
	{
		gSavedSettings.getF32("AvatarAxisScale0"),
		gSavedSettings.getF32("AvatarAxisScale1"),
		gSavedSettings.getF32("AvatarAxisScale2"),
		gSavedSettings.getF32("AvatarAxisScale3"),
		gSavedSettings.getF32("AvatarAxisScale4"),
		gSavedSettings.getF32("AvatarAxisScale5")
	};

	F32 dead_zone[] =
	{
		gSavedSettings.getF32("AvatarAxisDeadZone0"),
		gSavedSettings.getF32("AvatarAxisDeadZone1"),
		gSavedSettings.getF32("AvatarAxisDeadZone2"),
		gSavedSettings.getF32("AvatarAxisDeadZone3"),
		gSavedSettings.getF32("AvatarAxisDeadZone4"),
		gSavedSettings.getF32("AvatarAxisDeadZone5")
	};

	// time interval in seconds between this frame and the previous
	F32 time = gFrameIntervalSeconds.value();

	// avoid making ridicously big movements if there's a big drop in fps 
	if (time > .2f)
	{
		time = .2f;
	}

	// note: max feather is 32.0
	F32 feather = gSavedSettings.getF32("AvatarFeathering"); 
	
	F32 cur_delta[6];
	F32 val, dom_mov = 0.f;
	U32 dom_axis = Z_I;
#if LIB_NDOF
    bool absolute = (gSavedSettings.getBOOL("Cursor3D") && mNdofDev->absolute);
#else
    bool absolute = false;
#endif
	// remove dead zones and determine biggest movement on the joystick 
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[axis[i]];
		if (absolute)
		{
			F32 tmp = cur_delta[i];
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
			sLastDelta[i] = tmp;
		}

		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}

		// we don't care about Roll (RZ) and Z is calculated after the loop
        if (i != Z_I && i != RZ_I)
		{
			// find out the axis with the biggest joystick motion
			val = fabs(cur_delta[i]);
			if (val > dom_mov)
			{
				dom_axis = i;
				dom_mov = val;
			}
		}
		
		is_zero = is_zero && (cur_delta[i] == 0.f);
	}

	if (!is_zero)
	{
		// Clear AFK state if moved beyond the deadzone
		if (gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
		{
			gAgent.clearAFK();
		}
		
		setCameraNeedsUpdate(true);
	}

	// forward|backward movements overrule the real dominant movement if 
	// they're bigger than its 20%. This is what you want 'cos moving forward
	// is what you do most. We also added a special (even more lenient) case 
	// for RX|RY to allow walking while pitching and turning
	if (fabs(cur_delta[Z_I]) > .2f * dom_mov
	    || ((dom_axis == RX_I || dom_axis == RY_I) 
		&& fabs(cur_delta[Z_I]) > .05f * dom_mov))
	{
		dom_axis = Z_I;
	}

	sDelta[X_I] = -cur_delta[X_I] * axis_scale[X_I];
	sDelta[Y_I] = -cur_delta[Y_I] * axis_scale[Y_I];
	sDelta[Z_I] = -cur_delta[Z_I] * axis_scale[Z_I];
	cur_delta[RX_I] *= -axis_scale[RX_I] * mPerfScale;
	cur_delta[RY_I] *= -axis_scale[RY_I] * mPerfScale;
		
	if (!absolute)
	{
		cur_delta[RX_I] *= time;
		cur_delta[RY_I] *= time;
	}
	sDelta[RX_I] += (cur_delta[RX_I] - sDelta[RX_I]) * time * feather;
	sDelta[RY_I] += (cur_delta[RY_I] - sDelta[RY_I]) * time * feather;
	
	handleRun((F32) sqrt(sDelta[Z_I]*sDelta[Z_I] + sDelta[X_I]*sDelta[X_I]));
	
//	//BD - Use raw deltas, do not add any stupid limitations or extra dead zones
	//     otherwise alot controllers will cry and camera movement will bug out
	//     or be completely ignored on some controllers. Especially fixes Xbox 360
	//     controller avatar movement.
	agentSlide(sDelta[X_I]);		// move sideways
	agentFly(sDelta[Y_I]);			// up/down & crouch
	agentPush(sDelta[Z_I]);			// forward/back
	agentPitch(sDelta[RX_I]);		// pitch
	agentYaw(sDelta[RY_I]);			// turn
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::moveFlycam(bool reset)
{
	static LLQuaternion 		sFlycamRotation;
	static LLVector3    		sFlycamPosition;
	static F32          		sFlycamZoom;
	
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickFlycamEnabled"))
	{
		return;
	}

	S32 axis[] = 
	{
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5"),
		gSavedSettings.getS32("JoystickAxis6")
	};

	bool in_build_mode = LLToolMgr::getInstance()->inBuildMode();
	if (reset || mResetFlag)
	{
		sFlycamPosition = LLViewerCamera::getInstance()->getOrigin();
		sFlycamRotation = LLViewerCamera::getInstance()->getQuaternion();
		sFlycamZoom = LLViewerCamera::getInstance()->getView();
		
		resetDeltas(axis);

		return;
	}

	F32 axis_scale[] =
	{
		gSavedSettings.getF32("FlycamAxisScale0"),
		gSavedSettings.getF32("FlycamAxisScale1"),
		gSavedSettings.getF32("FlycamAxisScale2"),
		gSavedSettings.getF32("FlycamAxisScale3"),
		gSavedSettings.getF32("FlycamAxisScale4"),
		gSavedSettings.getF32("FlycamAxisScale5"),
		gSavedSettings.getF32("FlycamAxisScale6")
	};

	F32 dead_zone[] =
	{
		gSavedSettings.getF32("FlycamAxisDeadZone0"),
		gSavedSettings.getF32("FlycamAxisDeadZone1"),
		gSavedSettings.getF32("FlycamAxisDeadZone2"),
		gSavedSettings.getF32("FlycamAxisDeadZone3"),
		gSavedSettings.getF32("FlycamAxisDeadZone4"),
		gSavedSettings.getF32("FlycamAxisDeadZone5"),
		gSavedSettings.getF32("FlycamAxisDeadZone6")
	};

	F32 time = gFrameIntervalSeconds.value();

	// avoid making ridiculously big movements if there's a big drop in fps 
	if (time > .2f)
	{
		time = .2f;
	}

	F32 cur_delta[7];
	F32 feather = gSavedSettings.getF32("FlycamFeathering");
	bool absolute = gSavedSettings.getBOOL("Cursor3D");
	bool is_zero = true;

	for (U32 i = 0; i < 7; i++)
	{
		cur_delta[i] = -getJoystickAxis(axis[i]);


		F32 tmp = cur_delta[i];
		if (absolute)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;

		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}

		// We may want to scale camera movements up or down in build mode.
		// NOTE: this needs to remain after the deadzone calculation, otherwise
		// we have issues with flycam "jumping" when the build dialog is opened/closed  -Nyx
		if (in_build_mode)
		{
			if (i == X_I || i == Y_I || i == Z_I)
			{
				static LLCachedControl<F32> build_mode_scale(gSavedSettings,"FlycamBuildModeScale", 1.0);
				cur_delta[i] *= build_mode_scale;
			}
		}

		cur_delta[i] *= axis_scale[i];
		
		if (!absolute)
		{
			cur_delta[i] *= time;
		}

		sDelta[i] = sDelta[i] + (cur_delta[i]-sDelta[i])*time*feather;

		is_zero = is_zero && (cur_delta[i] == 0.f);

	}
	
	// Clear AFK state if moved beyond the deadzone
	if (!is_zero && gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}

//	//BD - Remappable joystick controls
	//     We need to cheat alot here because buttons only have 1/0, so we just
	//     give 1 the maximum value of a default setup and multiply it with the
	//     axis scales
	{
//		//BD - Roll Left
		if (mBtn[gSavedSettings.getS32("JoystickButtonRollLeft")] == 1)
		{
			sDelta[3] -= 0.0015f * axis_scale[3];
		}

//		//BD - Roll Right
		if (mBtn[gSavedSettings.getS32("JoystickButtonRollRight")] == 1)
		{
			sDelta[3] += 0.0015f * axis_scale[3];
		}

//		//BD - Zoom in
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomIn")] == 1)
		{
			sDelta[6] += 0.001f * axis_scale[6];
		}

//		//BD - Zoom Out
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomOut")] == 1)
		{
			sDelta[6] -= 0.001f * axis_scale[6];
		}

//		//BD - Jump / Y Up
		if (mBtn[gSavedSettings.getS32("JoystickButtonJump")] == 1)
		{
			sDelta[2] += 0.001f * axis_scale[2]; 
		}

//		//BD - Crouch / Y Down
		if (mBtn[gSavedSettings.getS32("JoystickButtonCrouch")] == 1)
		{
			sDelta[2] -= 0.001f * axis_scale[2]; 
		}

//		//BD - Default Zoom
		if (mBtn[gSavedSettings.getS32("JoystickButtonZoomDefault")] == 1)
		{
			sFlycamZoom = DEFAULT_FIELD_OF_VIEW;
		}
	}

	sFlycamPosition += LLVector3(sDelta) * sFlycamRotation;

	LLMatrix3 rot_mat(sDelta[3], sDelta[4], sDelta[5]);
	sFlycamRotation = LLQuaternion(rot_mat)*sFlycamRotation;

	if (gSavedSettings.getBOOL("AutoLeveling"))
	{
		LLMatrix3 level(sFlycamRotation);

		LLVector3 x = LLVector3(level.mMatrix[0]);
		LLVector3 y = LLVector3(level.mMatrix[1]);
		LLVector3 z = LLVector3(level.mMatrix[2]);

		y.mV[2] = 0.f;
		y.normVec();

		level.setRows(x,y,z);
		level.orthogonalize();
				
		LLQuaternion quat(level);
		sFlycamRotation = nlerp(llmin(feather*time,1.f), sFlycamRotation, quat);
	}

	if (gSavedSettings.getBOOL("ZoomDirect"))
	{
		sFlycamZoom = sLastDelta[6]*axis_scale[6]+dead_zone[6];
	}
	else
	{
		sFlycamZoom += sDelta[6];
	}

	LLMatrix3 mat(sFlycamRotation);

	LLViewerCamera::getInstance()->setView(sFlycamZoom);
	LLViewerCamera::getInstance()->setOrigin(sFlycamPosition);
	LLViewerCamera::getInstance()->mXAxis = LLVector3(mat.mMatrix[0]);
	LLViewerCamera::getInstance()->mYAxis = LLVector3(mat.mMatrix[1]);
	LLViewerCamera::getInstance()->mZAxis = LLVector3(mat.mMatrix[2]);
}

// -----------------------------------------------------------------------------
bool LLViewerJoystick::toggleFlycam()
{
	if (!gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickFlycamEnabled"))
	{
		mOverrideCamera = false;
		return false;
	}

	if (!mOverrideCamera)
	{
		gAgentCamera.changeCameraToDefault();
	}

	if (gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}
	
	mOverrideCamera = !mOverrideCamera;
	if (mOverrideCamera)
	{
		moveFlycam(true);
		
	}
	else 
	{
		// Exiting from the flycam mode: since we are going to keep the flycam POV for
		// the main camera until the avatar moves, we need to track this situation.
		setCameraNeedsUpdate(false);
		setNeedsReset(true);
	}
	return true;
}

void LLViewerJoystick::scanJoystick()
{
	if (mDriverState != JDS_INITIALIZED || !gSavedSettings.getBOOL("JoystickEnabled"))
	{
		return;
	}

#if LL_WINDOWS
	// On windows, the flycam is updated syncronously with a timer, so there is
	// no need to update the status of the joystick here.
	if (!mOverrideCamera)
#endif
	updateStatus();

	// App focus check Needs to happen AFTER updateStatus in case the joystick
	// is not centred when the app loses focus.
	if (!gFocusMgr.getAppHasFocus())
	{
		return;
	}

	static long toggle_flycam = 0;

//	//BD - Toggle flycam
	if (mBtn[gSavedSettings.getS32("JoystickButtonFlycam")] == 1)
	{
		if (mBtn[gSavedSettings.getS32("JoystickButtonFlycam")] != toggle_flycam)
		{
			toggle_flycam = toggleFlycam() ? 1 : 0;
		}
	}
	else
	{
		toggle_flycam = 0;
	}
	
	if (!mOverrideCamera && !(LLToolMgr::getInstance()->inBuildMode() && gSavedSettings.getBOOL("JoystickBuildEnabled")))
	{
		moveAvatar();
	}
}

// -----------------------------------------------------------------------------
std::string LLViewerJoystick::getDescription()
{
	std::string res;
#if LIB_NDOF
	if (mDriverState == JDS_INITIALIZED && mNdofDev)
	{
		res = ll_safe_string(mNdofDev->product);
	}
#endif
	return res;
}

bool LLViewerJoystick::isLikeSpaceNavigator() const
{
#if LIB_NDOF	
	return (isJoystickInitialized() 
			&& (strncmp(mNdofDev->product, "SpaceNavigator", 14) == 0
				|| strncmp(mNdofDev->product, "SpaceExplorer", 13) == 0
				|| strncmp(mNdofDev->product, "SpaceTraveler", 13) == 0
				|| strncmp(mNdofDev->product, "SpacePilot", 10) == 0));
#else
	return false;
#endif
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::setSNDefaults()
{
#if LL_DARWIN || LL_LINUX
	const float platformScale = 20.f;
	const float platformScaleAvXZ = 1.f;
	// The SpaceNavigator doesn't act as a 3D cursor on OS X / Linux. 
	const bool is_3d_cursor = false;
#else
	const float platformScale = 1.f;
	const float platformScaleAvXZ = 2.f;
	const bool is_3d_cursor = true;
#endif
	
	//gViewerWindow->alertXml("CacheWillClear");
	LL_INFOS() << "restoring SpaceNavigator defaults..." << LL_ENDL;
	
	gSavedSettings.setS32("JoystickAxis0", 1); // z (at)
	gSavedSettings.setS32("JoystickAxis1", 0); // x (slide)
	gSavedSettings.setS32("JoystickAxis2", 2); // y (up)
	gSavedSettings.setS32("JoystickAxis3", 4); // pitch
	gSavedSettings.setS32("JoystickAxis4", 3); // roll 
	gSavedSettings.setS32("JoystickAxis5", 5); // yaw
	gSavedSettings.setS32("JoystickAxis6", -1);

	gSavedSettings.setS32("JoystickButtonJump", -1);
	gSavedSettings.setS32("JoystickButtonCrouch", -1);
	gSavedSettings.setS32("JoystickButtonFly", -1);
	gSavedSettings.setS32("JoystickButtonRunToggle", -1);
	gSavedSettings.setS32("JoystickButtonMouselook", -1);
	gSavedSettings.setS32("JoystickButtonZoomDefault", -1);
	gSavedSettings.setS32("JoystickButtonFlycam", 0);
	gSavedSettings.setS32("JoystickButtonZoomOut", -1);
	gSavedSettings.setS32("JoystickButtonZoomIn", -1);
	gSavedSettings.setS32("JoystickButtonRollLeft", -1);
	gSavedSettings.setS32("JoystickButtonRollRight", -1);
	
	gSavedSettings.setBOOL("Cursor3D", is_3d_cursor);
	gSavedSettings.setBOOL("AutoLeveling", true);
	gSavedSettings.setBOOL("ZoomDirect", false);
	
	gSavedSettings.setF32("AvatarAxisScale0", 1.f * platformScaleAvXZ);
	gSavedSettings.setF32("AvatarAxisScale1", 1.f * platformScaleAvXZ);
	gSavedSettings.setF32("AvatarAxisScale2", 1.f);
	gSavedSettings.setF32("AvatarAxisScale4", .1f * platformScale);
	gSavedSettings.setF32("AvatarAxisScale5", .1f * platformScale);
	gSavedSettings.setF32("AvatarAxisScale3", 0.f * platformScale);
	gSavedSettings.setF32("BuildAxisScale1", .3f * platformScale);
	gSavedSettings.setF32("BuildAxisScale2", .3f * platformScale);
	gSavedSettings.setF32("BuildAxisScale0", .3f * platformScale);
	gSavedSettings.setF32("BuildAxisScale4", .3f * platformScale);
	gSavedSettings.setF32("BuildAxisScale5", .3f * platformScale);
	gSavedSettings.setF32("BuildAxisScale3", .3f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale1", 2.f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale2", 2.f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale0", 2.1f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale4", .1f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale5", .15f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale3", 0.f * platformScale);
	gSavedSettings.setF32("FlycamAxisScale6", 0.f * platformScale);
	
	gSavedSettings.setF32("AvatarAxisDeadZone0", .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone1", .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone2", .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone3", 1.f);
	gSavedSettings.setF32("AvatarAxisDeadZone4", .02f);
	gSavedSettings.setF32("AvatarAxisDeadZone5", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone0", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone1", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone2", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone3", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone4", .01f);
	gSavedSettings.setF32("BuildAxisDeadZone5", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone0", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone1", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone2", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone3", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone4", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone5", .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone6", 1.f);
	
	gSavedSettings.setF32("AvatarFeathering", 6.f);
	gSavedSettings.setF32("BuildFeathering", 12.f);
	gSavedSettings.setF32("FlycamFeathering", 5.f);
}

void LLViewerJoystick::setXboxDefaults()
{
	LL_INFOS() << "restoring Xbox360 Controller defaults..." << LL_ENDL;
	
	gSavedSettings.setS32("JoystickAxis0", 1); // z (at)
	gSavedSettings.setS32("JoystickAxis1", 0); // x (slide)
	gSavedSettings.setS32("JoystickAxis2", -1); // y (up)
	gSavedSettings.setS32("JoystickAxis3", 2); // pitch
	gSavedSettings.setS32("JoystickAxis4", 4); // roll 
	gSavedSettings.setS32("JoystickAxis5", 3); // yaw
	gSavedSettings.setS32("JoystickAxis6", -1);

	gSavedSettings.setS32("JoystickButtonJump", 0);
	gSavedSettings.setS32("JoystickButtonCrouch", 1);
	gSavedSettings.setS32("JoystickButtonFly", 2);
	gSavedSettings.setS32("JoystickButtonRunToggle", 8);
	gSavedSettings.setS32("JoystickButtonMouselook", 9);
	gSavedSettings.setS32("JoystickButtonZoomDefault", 6);
	gSavedSettings.setS32("JoystickButtonFlycam", 7);
	gSavedSettings.setS32("JoystickButtonZoomOut", 5);
	gSavedSettings.setS32("JoystickButtonZoomIn", 4);
	gSavedSettings.setS32("JoystickButtonRollLeft", -1);
	gSavedSettings.setS32("JoystickButtonRollRight", -1);
	
	gSavedSettings.setBOOL("Cursor3D", false);
	gSavedSettings.setBOOL("AutoLeveling", false);
	gSavedSettings.setBOOL("ZoomDirect", false);
	
	gSavedSettings.setF32("AvatarAxisScale0", .2f);
	gSavedSettings.setF32("AvatarAxisScale1", .2f);
	gSavedSettings.setF32("AvatarAxisScale2", .1f);
	gSavedSettings.setF32("AvatarAxisScale4", 1.0f);
	gSavedSettings.setF32("AvatarAxisScale5", 2.4f);
	gSavedSettings.setF32("AvatarAxisScale3", 2.0f);
	gSavedSettings.setF32("BuildAxisScale1", 2.f);
	gSavedSettings.setF32("BuildAxisScale2", 2.f);
	gSavedSettings.setF32("BuildAxisScale0", 2.f);
	gSavedSettings.setF32("BuildAxisScale4", 1.f);
	gSavedSettings.setF32("BuildAxisScale5", 1.f);
	gSavedSettings.setF32("BuildAxisScale3", 1.f);
	gSavedSettings.setF32("FlycamAxisScale1", 2.f);
	gSavedSettings.setF32("FlycamAxisScale2", 2.f);
	gSavedSettings.setF32("FlycamAxisScale0", 2.f);
	gSavedSettings.setF32("FlycamAxisScale4", 1.f);
	gSavedSettings.setF32("FlycamAxisScale5", 1.5f);
	gSavedSettings.setF32("FlycamAxisScale3", 1.f);
	gSavedSettings.setF32("FlycamAxisScale6", 1.f);
	
	gSavedSettings.setF32("AvatarAxisDeadZone0", .4f);
	gSavedSettings.setF32("AvatarAxisDeadZone1", .4f);
	gSavedSettings.setF32("AvatarAxisDeadZone2", .2f);
	gSavedSettings.setF32("AvatarAxisDeadZone3", .25f);
	gSavedSettings.setF32("AvatarAxisDeadZone4", .25f);
	gSavedSettings.setF32("AvatarAxisDeadZone5", .25f);
	gSavedSettings.setF32("BuildAxisDeadZone0", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone1", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone2", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone3", .1f);
	gSavedSettings.setF32("BuildAxisDeadZone4", .1f);
	gSavedSettings.setF32("BuildAxisDeadZone5", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone0", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone1", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone2", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone3", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone4", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone5", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone6", .1f);
	
	gSavedSettings.setF32("AvatarFeathering", 7.150f);
	gSavedSettings.setF32("BuildFeathering", 3.f);
	gSavedSettings.setF32("FlycamFeathering", 1.f);
}
