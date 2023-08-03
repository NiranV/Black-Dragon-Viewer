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
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfocusmgr.h"

//BD
#include "lldefs.h"

LLViewerJoystick* gJoystick = NULL;

#if LL_WINDOWS && !LL_MESA_HEADLESS
// Require DirectInput version 8
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#endif


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


#if LIB_NDOF
std::ostream& operator<<(std::ostream& out, NDOF_Device* ptr)
{
    if (! ptr)
    {
        return out << "nullptr";
    }
    out << "NDOF_Device{ ";
    out << "axes [";
    const char* delim = "";
    for (short axis = 0; axis < ptr->axes_count; ++axis)
    {
        out << delim << ptr->axes[axis];
        delim = ", ";
    }
    out << "]";
    out << ", buttons [";
    delim = "";
    for (short button = 0; button < ptr->btn_count; ++button)
    {
        out << delim << ptr->buttons[button];
        delim = ", ";
    }
    out << "]";
    out << ", range " << ptr->axes_min << ':' << ptr->axes_max;
    // If we don't coerce these to unsigned, they're streamed as characters,
    // e.g. ctrl-A or nul.
    out << ", absolute " << unsigned(ptr->absolute);
    out << ", valid " << unsigned(ptr->valid);
    out << ", manufacturer '" << ptr->manufacturer << "'";
    out << ", product '" << ptr->product << "'";
    out << ", private " << ptr->private_data;
    out << " }";
    return out;
}
#endif // LIB_NDOF


#if LL_WINDOWS && !LL_MESA_HEADLESS
// this should reflect ndof and set axises, see ndofdev_win.cpp from ndof package
BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* inst, VOID* user_data)
{
    if (inst->dwType & DIDFT_AXIS)
    {
        LPDIRECTINPUTDEVICE8 device = *((LPDIRECTINPUTDEVICE8 *)user_data);
        DIPROPRANGE diprg;
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYID;
        diprg.diph.dwObj = inst->dwType; // specify the enumerated axis

        // Set the range for the axis
        diprg.lMin = (long)-MAX_JOYSTICK_INPUT_VALUE;
        diprg.lMax = (long)+MAX_JOYSTICK_INPUT_VALUE;
        HRESULT hr = device->SetProperty(DIPROP_RANGE, &diprg.diph);

        if (FAILED(hr))
        {
            return DIENUM_STOP;
        }
    }

    return DIENUM_CONTINUE;
}

BOOL CALLBACK di8_devices_callback(LPCDIDEVICEINSTANCE device_instance_ptr, LPVOID pvRef)
{
    // Note: If a single device can function as more than one DirectInput
    // device type, it is enumerated as each device type that it supports.
    // Capable of detecting devices like Oculus Rift
    if (device_instance_ptr)
    {
        std::string product_name = utf16str_to_utf8str(llutf16string(device_instance_ptr->tszProductName));

        LLSD guid = LLViewerJoystick::getInstance()->getDeviceUUID();

        bool init_device = false;
        if (guid.isBinary())
        {
            std::vector<U8> bin_bucket = guid.asBinary();
            init_device = memcmp(&bin_bucket[0], &device_instance_ptr->guidInstance, sizeof(GUID)) == 0;
        }
        else
        {
            // It might be better to init space navigator here, but if system doesn't has one,
            // ndof will pick a random device, it is simpler to pick first device now to have an id
            init_device = true;
        }

        if (init_device)
        {
            //LL_DEBUGS("Joystick") << "Found and attempting to use device: " << product_name << LL_ENDL;
            LPDIRECTINPUT8       di8_interface = *((LPDIRECTINPUT8 *)gViewerWindow->getWindow()->getDirectInput8());
            LPDIRECTINPUTDEVICE8 device = NULL;

            HRESULT status = di8_interface->CreateDevice(
                device_instance_ptr->guidInstance, // REFGUID rguid,
                &device,                           // LPDIRECTINPUTDEVICE * lplpDirectInputDevice,
                NULL                               // LPUNKNOWN pUnkOuter
                );

            if (status == DI_OK)
            {
                // prerequisite for aquire()
                //LL_DEBUGS("Joystick") << "Device created" << LL_ENDL;
                status = device->SetDataFormat(&c_dfDIJoystick); // c_dfDIJoystick2
            }

            if (status == DI_OK)
            {
                // set properties
                //LL_DEBUGS("Joystick") << "Format set" << LL_ENDL;
                status = device->EnumObjects(EnumObjectsCallback, &device, DIDFT_ALL);
            }

            if (status == DI_OK)
            {
                //LL_DEBUGS("Joystick") << "Properties updated" << LL_ENDL;

                S32 size = sizeof(GUID);
                LLSD::Binary data; //just an std::vector
                data.resize(size);
                memcpy(&data[0], &device_instance_ptr->guidInstance /*POD _GUID*/, size);
                LLViewerJoystick::getInstance()->initDevice(&device, product_name, LLSD(data));
                return DIENUM_STOP;
            }
        }
        else
        {
            //LL_DEBUGS("Joystick") << "Found device: " << product_name << LL_ENDL;
        }
    }
    return DIENUM_CONTINUE;
}

// Windows guids
// This is GUID2 so teoretically it can be memcpy copied into LLUUID
void guid_from_string(GUID &guid, const std::string &input)
{
    CLSIDFromString(utf8str_to_utf16str(input).c_str(), &guid);
}

std::string string_from_guid(const GUID &guid)
{
    OLECHAR* guidString; //wchat
    StringFromCLSID(guid, &guidString);

    // use guidString...

    std::string res = utf16str_to_utf8str(llutf16string(guidString));
    // ensure memory is freed
    ::CoTaskMemFree(guidString);

    return res;
}
#endif

// -----------------------------------------------------------------------------
void LLViewerJoystick::updateEnabled(bool autoenable)
{
	if (mDriverState == JDS_UNINITIALIZED)
	{
		gSavedSettings.setBOOL("JoystickEnabled", FALSE );
	}
	else
	{
		// autoenable if user specifically chose this device
		if (autoenable && (isLikeSpaceNavigator() || isDeviceUUIDSet())) 
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
	if (gJoystick->mDriverState == JDS_UNINITIALIZED)
	{
		LL_INFOS("Joystick") << "HotPlugAddCallback: will use device:" << LL_ENDL;
		ndof_dump(stderr, dev);
		gJoystick->mNdofDev = dev;
		gJoystick->mDriverState = JDS_INITIALIZED;
		res = NDOF_KEEP_HOTPLUGGED;
	}
	gJoystick->updateEnabled(true);
    return res;
}
#endif

// -----------------------------------------------------------------------------
#if LIB_NDOF
void LLViewerJoystick::HotPlugRemovalCallback(NDOF_Device *dev)
{
	if (gJoystick->mNdofDev == dev)
	{
		LL_INFOS("joystick") << "HotPlugRemovalCallback: joystick->mNdofDev=" 
				<< gJoystick->mNdofDev << "; removed device:" << LL_ENDL;
		ndof_dump(stderr, dev);
		gJoystick->mDriverState = JDS_UNINITIALIZED;
	}
	gJoystick->updateEnabled(true);
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

    mLastDeviceUUID = LLSD::Integer(1);
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
	gJoystick = LLViewerJoystick::getInstance();

#if LIB_NDOF
	static bool libinit = false;
	mDriverState = JDS_INITIALIZING;

    loadDeviceIdFromSettings();

	if (libinit == false)
	{
		// Note: The HotPlug callbacks are not actually getting called on Windows
		if (ndof_libinit(HotPlugAddCallback, 
						 HotPlugRemovalCallback, 
						 gViewerWindow->getWindow()->getDirectInput8()))
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
            // di8_devices_callback callback is immediate and happens in scope of getInputDevices()
#if LL_WINDOWS && !LL_MESA_HEADLESS
            // space navigator is marked as DI8DEVCLASS_GAMECTRL in ndof lib
            U32 device_type = DI8DEVCLASS_GAMECTRL;
            void* callback = &di8_devices_callback;
#else
            // MAC doesn't support device search yet
            // On MAC there is an ndof_idsearch and it is possible to specify product
            // and manufacturer in NDOF_Device for ndof_init_first to pick specific one
            U32 device_type = 0;
            void* callback = NULL;
#endif
            if (!gViewerWindow->getWindow()->getInputDevices(device_type, callback, NULL))
            {
                LL_INFOS("Joystick") << "Failed to gather devices from window. Falling back to ndof's init" << LL_ENDL;
                // Failed to gather devices from windows, init first suitable one
                mLastDeviceUUID = LLSD();
                void *preffered_device = NULL;
                initDevice(preffered_device);
            }

            if (mDriverState == JDS_INITIALIZING)
            {
                LL_INFOS("Joystick") << "Found no matching joystick devices." << LL_ENDL;
                mDriverState = JDS_UNINITIALIZED;
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
	LL_INFOS() << "Product Name = " << mNdofDev->product << LL_ENDL;

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
	LL_INFOS("Joystick") << "ndof: mDriverState=" << mDriverState << "; mNdofDev=" 
			<< mNdofDev << "; libinit=" << libinit << LL_ENDL;
#endif
}

void LLViewerJoystick::initDevice(LLSD &guid)
{
#if LIB_NDOF
    mLastDeviceUUID = guid;

#if LL_WINDOWS && !LL_MESA_HEADLESS
    // space navigator is marked as DI8DEVCLASS_GAMECTRL in ndof lib
    U32 device_type = DI8DEVCLASS_GAMECTRL;
    void* callback = &di8_devices_callback;
#else
    // MAC doesn't support device search yet
    // On MAC there is an ndof_idsearch and it is possible to specify product
    // and manufacturer in NDOF_Device for ndof_init_first to pick specific one
    U32 device_type = 0;
    void* callback = NULL;
#endif

    mDriverState = JDS_INITIALIZING; 
    if (!gViewerWindow->getWindow()->getInputDevices(device_type, callback, NULL))
    {
        LL_INFOS("Joystick") << "Failed to gather devices from window. Falling back to ndof's init" << LL_ENDL;
        // Failed to gather devices from windows, init first suitable one
        void *preffered_device = NULL;
        mLastDeviceUUID = LLSD();
        initDevice(preffered_device);
    }

    if (mDriverState == JDS_INITIALIZING)
    {
        LL_INFOS("Joystick") << "Found no matching joystick devices." << LL_ENDL;
        mDriverState = JDS_UNINITIALIZED;
    }
#endif
}

void LLViewerJoystick::initDevice(void * preffered_device /*LPDIRECTINPUTDEVICE8*/, std::string &name, LLSD &guid)
{
#if LIB_NDOF
    mLastDeviceUUID = guid;

    strncpy(mNdofDev->product, name.c_str(), sizeof(mNdofDev->product));
    mNdofDev->manufacturer[0] = '\0';

    initDevice(preffered_device);
#endif
}

void LLViewerJoystick::initDevice(void * preffered_device /* LPDIRECTINPUTDEVICE8* */)
{
#if LIB_NDOF
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
    // On windows preffered_device needs to be a pointer to LPDIRECTINPUTDEVICE8
    if (ndof_init_first(mNdofDev, preffered_device))
    {
        mDriverState = JDS_UNINITIALIZED;
        LL_WARNS() << "ndof_init_first FAILED" << LL_ENDL;
    }
    else
    {
        mDriverState = JDS_INITIALIZED;
    }
#endif
}

// -----------------------------------------------------------------------------
void LLViewerJoystick::terminate()
{
#if LIB_NDOF
    if (mNdofDev != NULL)
    {
        ndof_libcleanup(); // frees alocated memory in mNdofDev
        mDriverState = JDS_UNINITIALIZED;
        mNdofDev = NULL;
        LL_INFOS("Joystick") << "Terminated connection with NDOF device." << LL_ENDL;
    }
#endif
	delete gJoystick;
	gJoystick = NULL;
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
			gAgent.setRunning();
			gAgent.sendWalkRun(gAgent.getRunning());
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
				gAgent.clearRunning();
				gAgent.sendWalkRun(gAgent.getRunning());
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
	time = llclamp(time, 0.016f, 0.033f);

	// max feather is 32
	bool is_zero = true;

//	//BD - Remappable Joystick Controls
	cur_delta[CAM_X_AXIS] -= (F32)mBtn[mMappedButtons[ROLL_LEFT]];
	cur_delta[CAM_X_AXIS] += (F32)mBtn[mMappedButtons[ROLL_RIGHT]];
	cur_delta[Z_AXIS] += (F32)mBtn[mMappedButtons[JUMP]];
	cur_delta[Z_AXIS] -= (F32)mBtn[mMappedButtons[CROUCH]];
	
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[mMappedAxes[i]];

//		//BD - Invertable Pitch Controls
		if (mJoystickInvertPitch)
			cur_delta[4] *= -1.f;

		F32 tmp = cur_delta[i];
		F32 axis_deadzone = mAxesDeadzones[i + 6];
		if (mCursor3D || llabs(cur_delta[i]) < axis_deadzone)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;
		is_zero = is_zero && (cur_delta[i] == 0.f);
			
		//BD - We assume that delta 1.0 is the maximum.
		if (llabs(cur_delta[i]) > axis_deadzone)
		{
			//BD - Clamp the delta between 1 and -1 while taking the deadzone into account.
			if (cur_delta[i] > 0)
			{
				cur_delta[i] = llclamp(cur_delta[i] - axis_deadzone, 0.f, 1.f - axis_deadzone);
			}
			else
			{
				cur_delta[i] = llclamp(cur_delta[i] + axis_deadzone, -1.f + axis_deadzone, 0.f);
			}
			//BD - Rescale the remaining delta to match the maximum to get a new clean 0 to 1 range.
			cur_delta[i] = cur_delta[i] / (1.f - axis_deadzone);
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
	time = llclamp(time, 0.016f, 0.033f);

	// note: max feather is 32.0
	
	F32 cur_delta[6];
	F32 val, dom_mov = 0.f;
	U32 dom_axis = Z_I;
//	//BD - Remappable Joystick Controls
	mAxes[Z_AXIS] += (F32)mBtn[mMappedButtons[JUMP]];
	mAxes[Z_AXIS] -= (F32)mBtn[mMappedButtons[CROUCH]];

	// remove dead zones and determine biggest movement on the joystick 
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[mMappedAxes[i]];
		F32 axis_deadzone = mAxesDeadzones[i];

		//BD - We assume that delta 1.0 is the maximum.
		if (llabs(cur_delta[i]) > axis_deadzone)
		{
			//BD - Clamp the delta between 1 and -1 while taking the deadzone into account.
			if (cur_delta[i] > 0)
			{
				cur_delta[i] = llclamp(cur_delta[i] - axis_deadzone, 0.f, 1.f - axis_deadzone);
			}
			else
			{
				cur_delta[i] = llclamp(cur_delta[i] + axis_deadzone, -1.f + axis_deadzone, 0.f);
			}
			//BD - Rescale the remaining delta to match the maximum to get a new clean 0 to 1 range.
			cur_delta[i] = cur_delta[i] / (1.f - axis_deadzone);
		}
		else
		{
			cur_delta[i] = 0.f;
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
		
		cur_delta[i] *= mAxesScalings[i] * time;

		//sDelta[i] = sDelta[i] + (cur_delta[i] - sDelta[i]) * time;
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
		cur_delta[RX_I] = -cur_delta[RX_I];

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

	sDelta[X_I] = -cur_delta[X_I];
	sDelta[Y_I] = -cur_delta[Y_I];
	sDelta[Z_I] = -cur_delta[Z_I];
	cur_delta[RX_I] *= -mAxesScalings[RX_I];
	cur_delta[RY_I] *= -mAxesScalings[RY_I];
		
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
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !mJoystickEnabled || !mJoystickFlycamEnabled)
	{
		return;
	}

	static LLQuaternion 		sFlycamRotation;
	static LLVector3    		sFlycamPosition;
	static F32          		sFlycamZoom;
	static LLViewerCamera* viewer_cam = LLViewerCamera::getInstance();

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
	time = llclamp(time, 0.016f, 0.033f);

	F32 flycam_feather = mFlycamFeathering;
	F32 cur_delta[MAX_AXES];
	F32 max_angle = viewer_cam->getMaxView();
	F32 min_angle = viewer_cam->getMinView();

	//BD - Slam zoom back to default and kill any delta we might have.
	if (mBtn[mMappedButtons[ZOOM_DEFAULT]] == 1)
	{
		sFlycamZoom = gSavedSettings.getF32("CameraAngle");
		sDelta[CAM_W_AXIS] = 0.0f;
	}

	//BD - Only smooth flycam zoom if we are not capping at the min/max otherwise the feathering
	//     ends up working against previous input, delaying zoom in movement when we just zoomed
	//     out beyond capped max for a bit and vise versa.
	if ((sFlycamZoom <= min_angle
		|| sFlycamZoom >= max_angle))
	{
		flycam_feather = 3.0f;
	}

//	//BD - Remappable Joystick Controls
	if(mMappedButtons[ROLL_LEFT] >= 0)
		mAxes[mMappedAxes[CAM_X_AXIS]] -= (F32)mBtn[mMappedButtons[ROLL_LEFT]];
	if (mMappedButtons[ROLL_RIGHT] >= 0)
		mAxes[mMappedAxes[CAM_X_AXIS]] += (F32)mBtn[mMappedButtons[ROLL_RIGHT]];

	bool is_zero = true;
	for (U32 i = 0; i < 7; i++)
	{
		cur_delta[i] = -mAxes[mMappedAxes[i]];

		cur_delta[CAM_W_AXIS] -= (F32)mBtn[mMappedButtons[ZOOM_OUT]];
		cur_delta[CAM_W_AXIS] += (F32)mBtn[mMappedButtons[ZOOM_IN]];
		cur_delta[Z_AXIS] += (F32)mBtn[mMappedButtons[JUMP]];
		cur_delta[Z_AXIS] -= (F32)mBtn[mMappedButtons[CROUCH]];

		F32 tmp = cur_delta[i];
		F32 axis_deadzone = mAxesDeadzones[i + 12];
		if (mCursor3D || llabs(cur_delta[i]) < axis_deadzone)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;

		//BD - We assume that delta 1.0 is the maximum.
		if (llabs(cur_delta[i]) > axis_deadzone)
		{
			//BD - Clamp the delta between 1 and -1 while taking the deadzone into account.
			if (cur_delta[i] > 0)
			{
				cur_delta[i] = llclamp(cur_delta[i] - axis_deadzone, 0.f, 1.f - axis_deadzone);
			}
			else
			{
				cur_delta[i] = llclamp(cur_delta[i] + axis_deadzone, -1.f + axis_deadzone, 0.f);
			}
			//BD - Rescale the remaining delta to match the maximum to get a new clean 0 to 1 range.
			cur_delta[i] = cur_delta[i] / (1.f - axis_deadzone);
		}

		// We may want to scale camera movements up or down in build mode.
		// NOTE: this needs to remain after the deadzone calculation, otherwise
		// we have issues with flycam "jumping" when the build dialog is opened/closed  -Nyx
		if (LLToolMgr::getInstance()->inBuildMode())
		{
			if (i == X_I || i == Y_I || i == Z_I)
			{
				cur_delta[i] *= mFlycamBuildModeScale;
			}
		}

		cur_delta[i] *= mAxesScalings[i + 12];

		if (!mCursor3D)
		{
			cur_delta[i] *= time;
		}

		sDelta[i] = sDelta[i] + (cur_delta[i] - sDelta[i]) * time * flycam_feather;
		is_zero = is_zero && (cur_delta[i] == 0.f);
	}

//	//BD - Invertable Pitch Controls
	if (mJoystickInvertPitch)
		cur_delta[CAM_Y_AXIS] = -cur_delta[CAM_Y_AXIS];
	
	// Clear AFK state if moved beyond the deadzone
	if (!is_zero && gAwayTimer.getElapsedTimeF32() > LLAgent::MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}

	sFlycamPosition += LLVector3(sDelta) * sFlycamRotation;
	LLMatrix3 rot_mat(sDelta[CAM_X_AXIS], sDelta[CAM_Y_AXIS], sDelta[CAM_Z_AXIS]);
	sFlycamRotation = LLQuaternion(rot_mat)*sFlycamRotation;

	if (mAutoLeveling || mBtn[mMappedButtons[ROLL_DEFAULT]] == 1)
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
		LLQuaternion lerp = nlerp(llmin(flycam_feather * time, 1.f), sFlycamRotation, quat);
		sFlycamRotation = mBtn[mMappedButtons[ROLL_DEFAULT]] == 1 ? quat : lerp;
	}

	if (mZoomDirect)
	{
		sFlycamZoom = sLastDelta[CAM_W_AXIS] * mAxesScalings[FLYCAM_AXIS_6] + mAxesScalings[FLYCAM_AXIS_6];
	}
	else
	{
		//BD - We need to cap zoom otherwise it internally counts higher causing
		//     the zoom level to not react until that extra has been removed first.
		sFlycamZoom = llclamp(sFlycamZoom + sDelta[CAM_W_AXIS], viewer_cam->getMinView(), viewer_cam->getMaxView());
	}

	LLMatrix3 mat(sFlycamRotation);

	viewer_cam->setView(sFlycamZoom);
	viewer_cam->setOrigin(sFlycamPosition);
	viewer_cam->mXAxis = LLVector3(mat.mMatrix[X_AXIS]);
	viewer_cam->mYAxis = LLVector3(mat.mMatrix[Y_AXIS]);
	viewer_cam->mZAxis = LLVector3(mat.mMatrix[Z_AXIS]);
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
bool LLViewerJoystick::isDeviceUUIDSet()
{
#if LL_WINDOWS && !LL_MESA_HEADLESS
    // for ease of comparison and to dial less with platform specific variables, we store id as LLSD binary
    return mLastDeviceUUID.isBinary();
#else
    return false;
#endif
}

LLSD LLViewerJoystick::getDeviceUUID()
{
    return mLastDeviceUUID;
}

std::string LLViewerJoystick::getDeviceUUIDString()
{
#if LL_WINDOWS && !LL_MESA_HEADLESS
    // Might be simpler to just convert _GUID into string everywhere, store and compare as string
    if (mLastDeviceUUID.isBinary())
    {
        S32 size = sizeof(GUID);
        LLSD::Binary data = mLastDeviceUUID.asBinary();
        GUID guid;
        memcpy(&guid, &data[0], size);
        return string_from_guid(guid);
    }
    else
    {
        return std::string();
    }
#else
    return std::string();
    // return mLastDeviceUUID;
#endif
}

void LLViewerJoystick::loadDeviceIdFromSettings()
{
#if LL_WINDOWS && !LL_MESA_HEADLESS
    // We can't save binary data to gSavedSettings, somebody editing the file will corrupt it,
    // so _GUID data gets converted to string (we probably can convert it to LLUUID with memcpy)
    // and here we need to convert it back to binary from string
    std::string device_string = gSavedSettings.getString("JoystickDeviceUUID");
    if (device_string.empty())
    {
        mLastDeviceUUID = LLSD();
    }
    else
    {
        //LL_DEBUGS("Joystick") << "Looking for device by id: " << device_string << LL_ENDL;
        GUID guid;
        guid_from_string(guid, device_string);
        S32 size = sizeof(GUID);
        LLSD::Binary data; //just an std::vector
        data.resize(size);
        memcpy(&data[0], &guid /*POD _GUID*/, size);
        // We store this data in LLSD since LLSD is versatile and will be able to handle both GUID2
        // and any data MAC will need for device selection
        mLastDeviceUUID = LLSD(data);
    }
#else
    mLastDeviceUUID = LLSD();
    //mLastDeviceUUID = gSavedSettings.getLLSD("JoystickDeviceUUID");
#endif
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
	LL_INFOS("Joystick") << "restoring SpaceNavigator defaults..." << LL_ENDL;

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
	gSavedSettings.setS32("JoystickButtonRollDefault", -1);
	
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
	gSavedSettings.setS32("JoystickButtonRollDefault", -1);
	
	gSavedSettings.setBOOL("Cursor3D", false);
	gSavedSettings.setBOOL("AutoLeveling", false);
	gSavedSettings.setBOOL("ZoomDirect", false);
	
	gSavedSettings.setF32("AvatarAxisScale0", 1.f);
	gSavedSettings.setF32("AvatarAxisScale2", 1.f);
	gSavedSettings.setF32("AvatarAxisScale1", 1.f);
	gSavedSettings.setF32("AvatarAxisScale4", 1.f);
	gSavedSettings.setF32("AvatarAxisScale5", 1.f);
	gSavedSettings.setF32("AvatarAxisScale3", 1.f);
	gSavedSettings.setF32("BuildAxisScale0", 1.25f);
	gSavedSettings.setF32("BuildAxisScale2", 1.25f);
	gSavedSettings.setF32("BuildAxisScale1", 1.25f);
	gSavedSettings.setF32("BuildAxisScale4", 1.f);
	gSavedSettings.setF32("BuildAxisScale5", 1.f);
	gSavedSettings.setF32("BuildAxisScale3", 1.f);
	gSavedSettings.setF32("FlycamAxisScale0", 5.0f);
	gSavedSettings.setF32("FlycamAxisScale2", 5.0f);
	gSavedSettings.setF32("FlycamAxisScale1", 5.0f);
	gSavedSettings.setF32("FlycamAxisScale4", 2.0f);
	gSavedSettings.setF32("FlycamAxisScale5", 2.5f);
	gSavedSettings.setF32("FlycamAxisScale3", 2.0f);
	gSavedSettings.setF32("FlycamAxisScale6", 1.0f);
	
	gSavedSettings.setF32("AvatarAxisDeadZone0", .6f);
	gSavedSettings.setF32("AvatarAxisDeadZone2", .3f);
	gSavedSettings.setF32("AvatarAxisDeadZone1", .6f);
	gSavedSettings.setF32("AvatarAxisDeadZone3", .3f);
	gSavedSettings.setF32("AvatarAxisDeadZone4", .3f);
	gSavedSettings.setF32("AvatarAxisDeadZone5", .3f);
	gSavedSettings.setF32("BuildAxisDeadZone0", .25f);
	gSavedSettings.setF32("BuildAxisDeadZone2", .25f);
	gSavedSettings.setF32("BuildAxisDeadZone1", .25f);
	gSavedSettings.setF32("BuildAxisDeadZone3", .3f);
	gSavedSettings.setF32("BuildAxisDeadZone4", .3f);
	gSavedSettings.setF32("BuildAxisDeadZone5", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone0", .25f);
	gSavedSettings.setF32("FlycamAxisDeadZone2", .25f);
	gSavedSettings.setF32("FlycamAxisDeadZone1", .25f);
	gSavedSettings.setF32("FlycamAxisDeadZone3", .1f);
	gSavedSettings.setF32("FlycamAxisDeadZone4", .3f);
	gSavedSettings.setF32("FlycamAxisDeadZone5", .3f);
	gSavedSettings.setF32("FlycamAxisDeadZone6", .1f);
	
	gSavedSettings.setF32("AvatarFeathering", 20.0f);
	gSavedSettings.setF32("BuildFeathering", 3.f);
	gSavedSettings.setF32("FlycamFeathering", 1.0f);

	refreshEverything();
}

void LLViewerJoystick::refreshButtonMapping()
{
	mMappedButtons[ROLL_LEFT] = gSavedSettings.getS32("JoystickButtonRollLeft");
	mMappedButtons[ROLL_RIGHT] = gSavedSettings.getS32("JoystickButtonRollRight");
	mMappedButtons[ROLL_DEFAULT] = gSavedSettings.getS32("JoystickButtonRollDefault");
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