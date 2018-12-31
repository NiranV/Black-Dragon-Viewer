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

//BD
#include "lldefs.h"


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
	if (!mJoystickEnabled)
	{
		mOverrideCamera = FALSE;
	}
}

void LLViewerJoystick::setOverrideCamera(bool val)
{
	if (!mJoystickEnabled)
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
	refreshEverything();

	for (int i = 0; i < 6; i++)
	{
		mAxes[i] = sDelta[i] = sLastDelta[i] = 0.0f;
	}
	
	memset(mBtn, 0, sizeof(mBtn));
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
	
	//BD - Tell us the name of the product beforehand so in future we can add more.
	LL_INFOS() << "Product Name=" << mNdofDev->product << LL_ENDL;

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
//			//BD - Xbox360 Controller Support
			// It's not a Space Navigator, no problem, use Xbox360 defaults.
			if (gSavedSettings.getString("JoystickInitialized") != "Xbox360Controller")
			{
//				//BD - Let's put our Xbox360 defaults here because it is most likely a controller
				//     atleast similar to that of the Xbox360.
				setXboxDefaults();
				gSavedSettings.setString("JoystickInitialized", "Xbox360Controller");
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
void LLViewerJoystick::handleRun(F32 inc)
{
	// Decide whether to walk or run by applying a threshold, with slight
	// hysteresis to avoid oscillating between the two with input spikes.
	// Analog speed control would be better, but not likely any time soon.
	if (inc > mJoystickRunThreshold)
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
//	//BD - Xbox360 Controller Support
    gAgent.moveUp(1, false);
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::agentSlide(F32 inc)
{
	if (inc < 0.f)
	{
//		//BD - Xbox360 Controller Support
		gAgent.moveLeft(1, false);
	}
	else if (inc > 0.f)
	{
//		//BD - Xbox360 Controller Support
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
			   !mAutomaticFly))
		{
			gAgent.setFlying(true);
		}
//		//BD - Xbox360 Controller Support
		gAgent.moveUp(1, false);
	}
	else if (inc > 0.f)
	{
		// crouch
//		//BD - Xbox360 Controller Support
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
	if (gAgentCamera.cameraMouselook() && !mJoystickMouselookYaw)
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
		|| !mJoystickEnabled || !mJoystickBuildEnabled)
	{
		return;
	}

	if (reset || mResetFlag)
	{
		resetDeltas(mMappedAxes);
		return;
	}

	F32 cur_delta[6];
	F32 time = gFrameIntervalSeconds.value();

	//BD - Avoid making ridicously big movements if there's a big drop in fps 
	time = llclamp(time, 0.125f, 0.2f);

	// max feather is 32
	bool is_zero = true;
	
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[mMappedAxes[i]];

//		//BD - Remappable Joystick Controls
		cur_delta[3] -= (F32)mBtn[mMappedButtons[ROLL_LEFT]];
		cur_delta[3] += (F32)mBtn[mMappedButtons[ROLL_RIGHT]];
		cur_delta[2] += (F32)mBtn[mMappedButtons[JUMP]];
		cur_delta[2] -= (F32)mBtn[mMappedButtons[CROUCH]];

//		//BD - Invertable Pitch Controls
		if (mJoystickInvertPitch)
			cur_delta[4] *= -1.f;

		F32 tmp = cur_delta[i];
		if (mCursor3D)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;
		is_zero = is_zero && (cur_delta[i] == 0.f);
			
		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-mAxesDeadzones[i+6], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i] + mAxesDeadzones[i+6], 0.f);
		}
		cur_delta[i] *= mAxesScalings[i+6];
		
		if (!mCursor3D)
		{
			cur_delta[i] *= time;
		}

		sDelta[i] = sDelta[i] + (cur_delta[i] - sDelta[i]) * time * mBuildFeathering;
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
		|| !mJoystickEnabled || !mJoystickAvatarEnabled)
	{
		return;
	}

	if (reset || mResetFlag)
	{
		resetDeltas(mMappedAxes);
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
	//BD
	static bool w_button_held = false;
	static bool m_button_held = false;

//	//BD - Remappable Joystick Controls
	if (mBtn[mMappedButtons[FLY]] && !button_held)
	{
		button_held = true;
		if (gAgent.getFlying())
		{
			gAgent.setFlying(FALSE);
		}
		else
		{
			gAgent.setFlying(TRUE);
		}
	}
	else if (!mBtn[mMappedButtons[FLY]] && button_held)
	{
		button_held = false;
	}

	if (mBtn[mMappedButtons[TOGGLE_RUN]] && !w_button_held)
	{
		w_button_held = true;
		if (gAgent.getAlwaysRun())
		{
			gAgent.clearAlwaysRun();
		}
		else
		{
			gAgent.setAlwaysRun();
		}
	}
	else if (!mBtn[mMappedButtons[TOGGLE_RUN]] && w_button_held)
	{
		w_button_held = false;
	}

	if (mBtn[mMappedButtons[MOUSELOOK]] && !m_button_held)
	{
		m_button_held = true;
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToDefault();
		}
		else
		{
			gAgentCamera.changeCameraToMouselook();
		}
	}
	else if (!mBtn[mMappedButtons[MOUSELOOK]] && m_button_held)
	{
		m_button_held = false;
	}

	// time interval in seconds between this frame and the previous
	F32 time = gFrameIntervalSeconds.value();

	//BD - Avoid making ridicously big movements if there's a big drop in fps 
	time = llclamp(time, 0.125f, 0.2f);

	// note: max feather is 32.0
	
	F32 cur_delta[6];
	F32 val, dom_mov = 0.f;
	U32 dom_axis = Z_I;
#if LIB_NDOF
	bool absolute = (mCursor3D && mNdofDev->absolute);
#else
    bool absolute = false;
#endif
	// remove dead zones and determine biggest movement on the joystick 
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[mMappedAxes[i]];

//		//BD - Remappable Joystick Controls
		cur_delta[2] += (F32)mBtn[mMappedButtons[JUMP]];
		cur_delta[2] -= (F32)mBtn[mMappedButtons[CROUCH]];

		if (absolute)
		{
			F32 tmp = cur_delta[i];
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
			sLastDelta[i] = tmp;
		}

		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i] - mAxesDeadzones[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i] + mAxesDeadzones[i], 0.f);
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

//	//BD - Invertable Pitch Controls
	if (!mJoystickInvertPitch)
		cur_delta[RX_I] *= -1.f;

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

	sDelta[X_I] = -cur_delta[X_I] * mAxesScalings[X_I];
	sDelta[Y_I] = -cur_delta[Y_I] * mAxesScalings[Y_I];
	sDelta[Z_I] = -cur_delta[Z_I] * mAxesScalings[Z_I];
	cur_delta[RX_I] *= -mAxesScalings[RX_I];
	cur_delta[RY_I] *= -mAxesScalings[RY_I];
		
	if (!absolute)
	{
		cur_delta[RX_I] *= time;
		cur_delta[RY_I] *= time;
	}
	sDelta[RX_I] += (cur_delta[RX_I] - sDelta[RX_I]) * time * mAvatarFeathering;
	sDelta[RY_I] += (cur_delta[RY_I] - sDelta[RY_I]) * time * mAvatarFeathering;
	
	handleRun((F32) sqrt(sDelta[Z_I]*sDelta[Z_I] + sDelta[X_I]*sDelta[X_I]));
	
//	//BD - Xbox360 Controller Support
	//     Use raw deltas, do not add any stupid limitations or extra dead zones
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

	static LLViewerCamera* viewer_cam = LLViewerCamera::getInstance();
	
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !mJoystickEnabled || !mJoystickFlycamEnabled)
	{
		return;
	}

	bool in_build_mode = LLToolMgr::getInstance()->inBuildMode();
	if (reset || mResetFlag)
	{
		sFlycamPosition = viewer_cam->getOrigin();
		sFlycamRotation = viewer_cam->getQuaternion();
		sFlycamZoom = viewer_cam->getView();
		
		resetDeltas(mMappedAxes);

		return;
	}

	F32 time = gFrameIntervalSeconds.value();

	//BD - Avoid making ridiculously big movements if there's a big drop in fps 
	time = llclamp(time, 0.125f, 0.2f);

	F32 cur_delta[7];
	bool is_zero = true;
	F32 flycam_feather = mFlycamFeathering;
	for (U32 i = 0; i < 7; i++)
	{
		if (i < 6)
			cur_delta[i] = -mAxes[mMappedAxes[i]];

//		//BD - Remappable Joystick Controls
		cur_delta[3] -= (F32)mBtn[mMappedButtons[ROLL_LEFT]];
		cur_delta[3] += (F32)mBtn[mMappedButtons[ROLL_RIGHT]];
		cur_delta[6] -= (F32)mBtn[mMappedButtons[ZOOM_OUT]];
		cur_delta[6] += (F32)mBtn[mMappedButtons[ZOOM_IN]];
		cur_delta[2] += (F32)mBtn[mMappedButtons[JUMP]];
		cur_delta[2] -= (F32)mBtn[mMappedButtons[CROUCH]];

//		//BD - Invertable Pitch Controls
		if (mJoystickInvertPitch)
			cur_delta[4] *= -1.f;

		if (mBtn[mMappedButtons[ZOOM_DEFAULT]])
			sFlycamZoom = gSavedSettings.getF32("CameraAngle");


		F32 tmp = cur_delta[i];
		if (mCursor3D)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;

		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i] - mAxesDeadzones[i+12], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i] + mAxesDeadzones[i+12], 0.f);
		}

		// We may want to scale camera movements up or down in build mode.
		// NOTE: this needs to remain after the deadzone calculation, otherwise
		// we have issues with flycam "jumping" when the build dialog is opened/closed  -Nyx
		if (in_build_mode)
		{
			if (i == X_I || i == Y_I || i == Z_I)
			{
				cur_delta[i] *= mFlycamBuildModeScale;
			}
		}

		cur_delta[i] *= mAxesScalings[i+12];
		
		if (!mCursor3D)
		{
			cur_delta[i] *= time;
		}

		//BD - Only smooth flycam zoom if we are not capping at the min/max otherwise the feathering
		//     ends up working against previous input, delaying zoom in movement when we just zoomed
		//     out beyond capped max for a bit and vise versa.
		if (i == 6
			&& (sFlycamZoom == viewer_cam->getMinView()
			|| sFlycamZoom == viewer_cam->getMaxView()))
		{
			flycam_feather = 3.0f;
		}

		sDelta[i] = sDelta[i] + (cur_delta[i] - sDelta[i]) * time * flycam_feather;
		is_zero = is_zero && (cur_delta[i] == 0.f);
	}
	
	// Clear AFK state if moved beyond the deadzone
	if (!is_zero && gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}

	sFlycamPosition += LLVector3(sDelta) * sFlycamRotation;

	LLMatrix3 rot_mat(sDelta[3], sDelta[4], sDelta[5]);
	sFlycamRotation = LLQuaternion(rot_mat)*sFlycamRotation;

	if (mAutoLeveling)
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
		sFlycamRotation = nlerp(llmin(flycam_feather * time, 1.f), sFlycamRotation, quat);
	}

	if (mZoomDirect)
	{
		sFlycamZoom = sLastDelta[6] * mAxesScalings[18] + mAxesScalings[18];
	}
	else
	{
		//BD - We need to cap zoom otherwise it internally counts higher causing
		//     the zoom level to not react until that extra has been removed first.
		sFlycamZoom += sDelta[6];
		sFlycamZoom = llclamp(sFlycamZoom, viewer_cam->getMinView(), viewer_cam->getMaxView());
	}

	LLMatrix3 mat(sFlycamRotation);

	viewer_cam->setView(sFlycamZoom);
	viewer_cam->setOrigin(sFlycamPosition);
	viewer_cam->mXAxis = LLVector3(mat.mMatrix[0]);
	viewer_cam->mYAxis = LLVector3(mat.mMatrix[1]);
	viewer_cam->mZAxis = LLVector3(mat.mMatrix[2]);
}

// -----------------------------------------------------------------------------
bool LLViewerJoystick::toggleFlycam()
{
	if (!mJoystickEnabled || !mJoystickFlycamEnabled)
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
	if (mDriverState != JDS_INITIALIZED || !mJoystickEnabled)
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

//	//BD - Remappable Joystick Controls
	if (mBtn[mMappedButtons[FLYCAM]] == 1)
	{
		if (mBtn[mMappedButtons[FLYCAM]] != toggle_flycam)
		{
			toggle_flycam = toggleFlycam() ? 1 : 0;
		}
	}
	else
	{
		toggle_flycam = 0;
	}
	
	if (!mOverrideCamera && !(LLToolMgr::getInstance()->inBuildMode() && mJoystickBuildEnabled))
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

//	//BD - Remappable Joystick Controls
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

	refreshEverything();
}

//BD - Xbox360 Controller Support
void LLViewerJoystick::setXboxDefaults()
{
	LL_INFOS() << "restoring Xbox360 Controller defaults..." << LL_ENDL;
	
	gSavedSettings.setS32("JoystickAxis0", 1);	// Z
	gSavedSettings.setS32("JoystickAxis1", 0);	// X
	gSavedSettings.setS32("JoystickAxis2", -1);	// Y
	gSavedSettings.setS32("JoystickAxis3", 2);	// Roll
	gSavedSettings.setS32("JoystickAxis4", 4);	// Pitch
	gSavedSettings.setS32("JoystickAxis5", 3);	// Yaw
	gSavedSettings.setS32("JoystickAxis6", -1);	// Zoom

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
	gSavedSettings.setF32("AvatarAxisScale2", .1f);
	gSavedSettings.setF32("AvatarAxisScale1", .2f);
	gSavedSettings.setF32("AvatarAxisScale4", 0.75f);
	gSavedSettings.setF32("AvatarAxisScale5", 1.0f);
	gSavedSettings.setF32("AvatarAxisScale3", 1.0f);
	gSavedSettings.setF32("BuildAxisScale0", 1.25f);
	gSavedSettings.setF32("BuildAxisScale2", 1.25f);
	gSavedSettings.setF32("BuildAxisScale1", 1.25f);
	gSavedSettings.setF32("BuildAxisScale4", 1.f);
	gSavedSettings.setF32("BuildAxisScale5", 1.f);
	gSavedSettings.setF32("BuildAxisScale3", 1.f);
	gSavedSettings.setF32("FlycamAxisScale0", 1.5f);
	gSavedSettings.setF32("FlycamAxisScale2", 1.5f);
	gSavedSettings.setF32("FlycamAxisScale1", 1.5f);
	gSavedSettings.setF32("FlycamAxisScale4", 1.0f);
	gSavedSettings.setF32("FlycamAxisScale5", 1.0f);
	gSavedSettings.setF32("FlycamAxisScale3", 0.75f);
	gSavedSettings.setF32("FlycamAxisScale6", 0.05f);
	
	gSavedSettings.setF32("AvatarAxisDeadZone0", .4f);
	gSavedSettings.setF32("AvatarAxisDeadZone2", .2f);
	gSavedSettings.setF32("AvatarAxisDeadZone1", .4f);
	gSavedSettings.setF32("AvatarAxisDeadZone3", .25f);
	gSavedSettings.setF32("AvatarAxisDeadZone4", .25f);
	gSavedSettings.setF32("AvatarAxisDeadZone5", .25f);
	gSavedSettings.setF32("BuildAxisDeadZone0", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone2", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone1", .2f);
	gSavedSettings.setF32("BuildAxisDeadZone3", .1f);
	gSavedSettings.setF32("BuildAxisDeadZone4", .1f);
	gSavedSettings.setF32("BuildAxisDeadZone5", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone0", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone2", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone1", .2f);
	gSavedSettings.setF32("FlycamAxisDeadZone3", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone4", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone5", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone6", .1f);
	
	gSavedSettings.setF32("AvatarFeathering", 3.0f);
	gSavedSettings.setF32("BuildFeathering", 3.f);
	gSavedSettings.setF32("FlycamFeathering", 0.1f);

	refreshEverything();
}

void LLViewerJoystick::refreshButtonMapping()
{
	mMappedButtons[ROLL_LEFT] = gSavedSettings.getS32("JoystickButtonRollLeft");
	mMappedButtons[ROLL_RIGHT] = gSavedSettings.getS32("JoystickButtonRollRight");
	mMappedButtons[ZOOM_OUT] = gSavedSettings.getS32("JoystickButtonZoomOut");
	mMappedButtons[ZOOM_IN] = gSavedSettings.getS32("JoystickButtonZoomIn");
	mMappedButtons[ZOOM_DEFAULT] = gSavedSettings.getS32("JoystickButtonZoomDefault");
	mMappedButtons[JUMP] = gSavedSettings.getS32("JoystickButtonJump");
	mMappedButtons[CROUCH] = gSavedSettings.getS32("JoystickButtonCrouch");
	mMappedButtons[FLY] = gSavedSettings.getS32("JoystickButtonFly");
	mMappedButtons[MOUSELOOK] = gSavedSettings.getS32("JoystickButtonMouselook");
	mMappedButtons[FLYCAM] = gSavedSettings.getS32("JoystickButtonFlycam");
	mMappedButtons[TOGGLE_RUN] = gSavedSettings.getS32("JoystickButtonRunToggle");
}

void LLViewerJoystick::refreshAxesMapping()
{
	mMappedAxes[X_AXIS] = gSavedSettings.getS32("JoystickAxis0");
	mMappedAxes[Y_AXIS] = gSavedSettings.getS32("JoystickAxis1");
	mMappedAxes[Z_AXIS] = gSavedSettings.getS32("JoystickAxis2");
	mMappedAxes[CAM_X_AXIS] = gSavedSettings.getS32("JoystickAxis3");
	mMappedAxes[CAM_Y_AXIS] = gSavedSettings.getS32("JoystickAxis4");
	mMappedAxes[CAM_Z_AXIS] = gSavedSettings.getS32("JoystickAxis5");
	mMappedAxes[CAM_W_AXIS] = gSavedSettings.getS32("JoystickAxis6");

	mAxesScalings[AV_AXIS_0] = gSavedSettings.getF32("AvatarAxisScale0");
	mAxesScalings[AV_AXIS_1] = gSavedSettings.getF32("AvatarAxisScale1");
	mAxesScalings[AV_AXIS_2] = gSavedSettings.getF32("AvatarAxisScale2");
	mAxesScalings[AV_AXIS_3] = gSavedSettings.getF32("AvatarAxisScale3");
	mAxesScalings[AV_AXIS_4] = gSavedSettings.getF32("AvatarAxisScale4");
	mAxesScalings[AV_AXIS_5] = gSavedSettings.getF32("AvatarAxisScale5");
	mAxesScalings[BUILD_AXIS_0] = gSavedSettings.getF32("BuildAxisScale0");
	mAxesScalings[BUILD_AXIS_1] = gSavedSettings.getF32("BuildAxisScale1");
	mAxesScalings[BUILD_AXIS_2] = gSavedSettings.getF32("BuildAxisScale2");
	mAxesScalings[BUILD_AXIS_3] = gSavedSettings.getF32("BuildAxisScale3");
	mAxesScalings[BUILD_AXIS_4] = gSavedSettings.getF32("BuildAxisScale4");
	mAxesScalings[BUILD_AXIS_5] = gSavedSettings.getF32("BuildAxisScale5");
	mAxesScalings[FLYCAM_AXIS_0] = gSavedSettings.getF32("FlycamAxisScale0");
	mAxesScalings[FLYCAM_AXIS_1] = gSavedSettings.getF32("FlycamAxisScale1");
	mAxesScalings[FLYCAM_AXIS_2] = gSavedSettings.getF32("FlycamAxisScale2");
	mAxesScalings[FLYCAM_AXIS_3] = gSavedSettings.getF32("FlycamAxisScale3");
	mAxesScalings[FLYCAM_AXIS_4] = gSavedSettings.getF32("FlycamAxisScale4");
	mAxesScalings[FLYCAM_AXIS_5] = gSavedSettings.getF32("FlycamAxisScale5");
	mAxesScalings[FLYCAM_AXIS_6] = gSavedSettings.getF32("FlycamAxisScale6");

	mAxesDeadzones[AV_AXIS_0] = gSavedSettings.getF32("AvatarAxisDeadZone0");
	mAxesDeadzones[AV_AXIS_1] = gSavedSettings.getF32("AvatarAxisDeadZone1");
	mAxesDeadzones[AV_AXIS_2] = gSavedSettings.getF32("AvatarAxisDeadZone2");
	mAxesDeadzones[AV_AXIS_3] = gSavedSettings.getF32("AvatarAxisDeadZone3");
	mAxesDeadzones[AV_AXIS_4] = gSavedSettings.getF32("AvatarAxisDeadZone4");
	mAxesDeadzones[AV_AXIS_5] = gSavedSettings.getF32("AvatarAxisDeadZone5");
	mAxesDeadzones[BUILD_AXIS_0] = gSavedSettings.getF32("BuildAxisDeadZone0");
	mAxesDeadzones[BUILD_AXIS_1] = gSavedSettings.getF32("BuildAxisDeadZone1");
	mAxesDeadzones[BUILD_AXIS_2] = gSavedSettings.getF32("BuildAxisDeadZone2");
	mAxesDeadzones[BUILD_AXIS_3] = gSavedSettings.getF32("BuildAxisDeadZone3");
	mAxesDeadzones[BUILD_AXIS_4] = gSavedSettings.getF32("BuildAxisDeadZone4");
	mAxesDeadzones[BUILD_AXIS_5] = gSavedSettings.getF32("BuildAxisDeadZone5");
	mAxesDeadzones[FLYCAM_AXIS_0] = gSavedSettings.getF32("FlycamAxisDeadZone0");
	mAxesDeadzones[FLYCAM_AXIS_1] = gSavedSettings.getF32("FlycamAxisDeadZone1");
	mAxesDeadzones[FLYCAM_AXIS_2] = gSavedSettings.getF32("FlycamAxisDeadZone2");
	mAxesDeadzones[FLYCAM_AXIS_3] = gSavedSettings.getF32("FlycamAxisDeadZone3");
	mAxesDeadzones[FLYCAM_AXIS_4] = gSavedSettings.getF32("FlycamAxisDeadZone4");
	mAxesDeadzones[FLYCAM_AXIS_5] = gSavedSettings.getF32("FlycamAxisDeadZone5");
	mAxesDeadzones[FLYCAM_AXIS_6] = gSavedSettings.getF32("FlycamAxisDeadZone6");
}

void LLViewerJoystick::refreshSettings()
{
	mAutoLeveling = gSavedSettings.getBOOL("AutoLeveling");
	mZoomDirect = gSavedSettings.getBOOL("ZoomDirect");
	mJoystickEnabled = gSavedSettings.getBOOL("JoystickEnabled");
	mJoystickFlycamEnabled = gSavedSettings.getBOOL("JoystickFlycamEnabled");
	mJoystickBuildEnabled = gSavedSettings.getBOOL("JoystickBuildEnabled");
	mJoystickAvatarEnabled = gSavedSettings.getBOOL("JoystickAvatarEnabled");
	mJoystickInvertPitch = gSavedSettings.getBOOL("JoystickInvertPitch");
	mJoystickMouselookYaw = gSavedSettings.getBOOL("JoystickMouselookYaw");
	mCursor3D = gSavedSettings.getBOOL("Cursor3D");
	mAutomaticFly = gSavedSettings.getBOOL("AutomaticFly");

	mFlycamFeathering = gSavedSettings.getF32("FlycamFeathering");
	mAvatarFeathering = gSavedSettings.getF32("AvatarFeathering");
	mBuildFeathering = gSavedSettings.getF32("BuildFeathering");
	mFlycamBuildModeScale = gSavedSettings.getF32("FlycamBuildModeScale");
	mJoystickRunThreshold = gSavedSettings.getF32("JoystickRunThreshold");
}

//static
void LLViewerJoystick::refreshEverything()
{
	refreshAxesMapping();
	refreshButtonMapping();
	refreshSettings();
}