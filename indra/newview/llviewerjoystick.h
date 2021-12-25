/** 
 * @file llviewerjoystick.h
 * @brief Viewer joystick / NDOF device functionality.
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

#ifndef LL_LLVIEWERJOYSTICK_H
#define LL_LLVIEWERJOYSTICK_H

#include "stdtypes.h"

#if LIB_NDOF
#include "ndofdev_external.h"
#else
#define NDOF_Device	void
#define NDOF_HotPlugResult S32
#endif

typedef enum e_joystick_driver_state
{
	JDS_UNINITIALIZED,
	JDS_INITIALIZED,
	JDS_INITIALIZING
} EJoystickDriverState;

//BD - Optimized Joystick Mappings
typedef enum E_Buttons
{
	ROLL_LEFT = 0,
	ROLL_RIGHT,
	ROLL_DEFAULT ,
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_DEFAULT,
	JUMP,
	CROUCH,
	FLY,
	MOUSELOOK,
	FLYCAM,
	TOGGLE_RUN,
	MAX_BUTTONS
} E_buttons;

typedef enum E_Axes
{
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2,
	CAM_X_AXIS = 3,
	CAM_Y_AXIS = 4,
	CAM_Z_AXIS = 5,
	CAM_W_AXIS = 6,
	MAX_AXES = 7
} E_Axes;

typedef enum E_Scalings
{
	AV_AXIS_0 = 0,
	AV_AXIS_1 = 1,
	AV_AXIS_2 = 2,
	AV_AXIS_3 = 3,
	AV_AXIS_4 = 4,
	AV_AXIS_5 = 5,
	BUILD_AXIS_0 = 6,
	BUILD_AXIS_1 = 7,
	BUILD_AXIS_2 = 8,
	BUILD_AXIS_3 = 9,
	BUILD_AXIS_4 = 10,
	BUILD_AXIS_5 = 11,
	FLYCAM_AXIS_0 = 12,
	FLYCAM_AXIS_1 = 13,
	FLYCAM_AXIS_2 = 14,
	FLYCAM_AXIS_3 = 15,
	FLYCAM_AXIS_4 = 16,
	FLYCAM_AXIS_5 = 17,
	FLYCAM_AXIS_6 = 18,
	MAX_SCALINGS = 19
} E_Scalings;


class LLViewerJoystick : public LLSingleton<LLViewerJoystick>
{
	LLSINGLETON(LLViewerJoystick);
public:
	//LLViewerJoystick();
	virtual ~LLViewerJoystick();

	void init(bool autoenable);
	void initDevice(LLSD &guid);
	void initDevice(void * preffered_device /*LPDIRECTINPUTDEVICE8*/);
	void initDevice(void * preffered_device /*LPDIRECTINPUTDEVICE8*/, std::string &name, LLSD &guid);
	void terminate();

	void updateStatus();
	void scanJoystick();
	void moveObjects(bool reset = false);
	void moveAvatar(bool reset = false);
	void moveFlycam(bool reset = false);
	bool isJoystickInitialized() const {return (mDriverState==JDS_INITIALIZED);}
	bool isLikeSpaceNavigator() const;
	void setNeedsReset(bool reset = true) { mResetFlag = reset; }
	void setCameraNeedsUpdate(bool b)     { mCameraUpdated = b; }
	bool getCameraNeedsUpdate() const     { return mCameraUpdated; }
	bool getOverrideCamera() { return mOverrideCamera; }
	void setOverrideCamera(bool val);
	bool toggleFlycam();
	void setSNDefaults();
//	//BD - Xbox360 Controller Support
	void setXboxDefaults();
	//BD - Optimized Joystick Mappings
	void refreshButtonMapping();
	void refreshAxesMapping();
	void refreshSettings();

	void refreshEverything();

	long					mBtn[16];
	F32						mAxes[MAX_AXES];
	
	bool isDeviceUUIDSet();
	LLSD getDeviceUUID(); //unconverted, OS dependent value wrapped into LLSD, for comparison/search
	std::string getDeviceUUIDString(); // converted readable value for settings
	std::string getDescription();

protected:
	void updateEnabled(bool autoenable);
	void handleRun(F32 inc);
	void agentSlide(F32 inc);
	void agentPush(F32 inc);
	void agentFly(F32 inc);
	void agentPitch(F32 pitch_inc);
	void agentYaw(F32 yaw_inc);
	void agentJump();
	void resetDeltas(S32 axis[]);
	void loadDeviceIdFromSettings();
#if LIB_NDOF
	static NDOF_HotPlugResult HotPlugAddCallback(NDOF_Device *dev);
	static void HotPlugRemovalCallback(NDOF_Device *dev);
#endif
	
private:
	EJoystickDriverState	mDriverState;
	NDOF_Device				*mNdofDev;
	bool					mResetFlag;
	bool					mCameraUpdated;
	bool 					mOverrideCamera;
	U32						mJoystickRun;
	LLSD					mLastDeviceUUID; // _GUID as U8 binary map, integer 1 for no device/ndof's device
	
	static F32				sLastDelta[MAX_AXES];
	static F32				sDelta[MAX_AXES];

	//BD - Optimized Joystick Mappings
	S32 mMappedButtons[MAX_BUTTONS];
	S32 mMappedAxes[MAX_AXES];
	F32 mAxesScalings[MAX_SCALINGS];
	F32 mAxesDeadzones[MAX_SCALINGS];

	bool mAutoLeveling;
	bool mZoomDirect;
	bool mJoystickEnabled;
	bool mJoystickFlycamEnabled;
	bool mJoystickBuildEnabled;
	bool mJoystickAvatarEnabled;
	bool mJoystickInvertPitch;
	bool mJoystickMouselookYaw;
	bool mCursor3D;
	bool mAutomaticFly;

	F32 mFlycamFeathering;
	F32 mAvatarFeathering;
	F32 mBuildFeathering;
	F32 mFlycamBuildModeScale;
	F32 mJoystickRunThreshold;
};

extern LLViewerJoystick* gJoystick;

#endif
