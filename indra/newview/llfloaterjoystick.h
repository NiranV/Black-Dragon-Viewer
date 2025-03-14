/** 
 * @file llfloaterjoystick.h
 * @brief Joystick preferences panel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLFLOATERJOYSTICK_H
#define LL_LLFLOATERJOYSTICK_H

#include "llfloater.h"
#include "llstatview.h"

class LLCheckBoxCtrl;
class LLComboBox;

class LLFloaterJoystick : public LLFloater
{
public:
	LLFloaterJoystick(const LLSD& key);
	~LLFloaterJoystick();

	virtual bool postBuild();
	virtual void refresh();
	virtual void apply();	// Apply the changed values.
	virtual void cancel();	// Cancel the changed values.
	virtual void draw();
	static  void setSNDefaults();
//	//BD - Xbox360 Controller Support
	static  void setXboxDefaults();

	void refreshAll();

    static bool addDeviceCallback(std::string& name, LLSD& value, void* userdata);
    void addDevice(std::string &name, LLSD& value);

protected:

	void refreshListOfDevices();

private:
	void initFromSettings();
	
	static void onCommitJoystickEnabled(LLUICtrl*, void*);
	static void onClickRestoreSNDefaults(void*);
	static void onClickCancel(void*);
	static void onClickOK(void*);
//	//BD - Xbox360 Controller Support
	static void onClickRestoreXboxDefaults(void*);

private:
	// Device prefs
	bool mJoystickEnabled;
	LLSD mJoystickId;
	S32 mJoystickAxis[7];
	bool m3DCursor;
	bool mAutoLeveling;
	bool mZoomDirect;

	// Modes prefs
	bool mAvatarEnabled;
	bool mBuildEnabled;
	bool mFlycamEnabled;
	F32 mAvatarAxisScale[6];
	F32 mBuildAxisScale[6];
	F32 mFlycamAxisScale[7];
	F32 mAvatarAxisDeadZone[6];
	F32 mBuildAxisDeadZone[6];
	F32 mFlycamAxisDeadZone[7];
	F32 mAvatarFeathering;
	F32 mBuildFeathering;
	F32 mFlycamFeathering;

	// Controls that can disable the flycam
	LLCheckBoxCtrl	*mCheckFlycamEnabled;
	LLComboBox		*mJoysticksCombo;

    bool mHasDeviceList;
    bool mJoystickInitialized;
    LLUUID mCurrentDeviceId;

	// stats view 
	LLStatBar* mAxisStatsBar[6];

//	//BD - Custom Joystick Mapping
	LLButton* mAxisButton[16];
};

#endif
