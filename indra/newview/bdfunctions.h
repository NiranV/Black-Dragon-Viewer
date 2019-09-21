/**
*
* Copyright (C) 2018, NiranV Dean
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
*/


#ifndef BD_FUNCTIONS_H
#define BD_FUNCTIONS_H

#include <string>

#include "llsliderctrl.h"
#include "llbutton.h"
#include "lltabcontainer.h"

class BDFunctions
{
public:
	BDFunctions();
	/*virtual*/	~BDFunctions();

	void initializeControls();

//	//BD - Debug Arrays
	static void onCommitX(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitY(LLUICtrl* ctrl, const LLSD& param);
	static void onCommitZ(LLUICtrl* ctrl, const LLSD& param);
//	//BD - Vector4
	static void onCommitW(LLUICtrl* ctrl, const LLSD& param);

	static void onControlLock(LLUICtrl* ctrl, const LLSD& param);

//	//BD - Revert to Default
	static BOOL resetToDefault(LLUICtrl* ctrl);

	static void invertValue(LLUICtrl* ctrl);

	static std::string escapeString(const std::string& str);

	static void triggerWarning(LLUICtrl* ctrl, const LLSD& param);

	static void openPreferences(const LLSD& param);

	static void askFactoryReset(const LLSD& param);
	static void doFactoryReset(const LLSD& notification, const LLSD& response);

//	//BD - Camera Recorder
	bool getCameraOverride() { return mCameraOverride; }

	bool mCameraOverride;

	//BD - Cached Settings
	//     llvoavatar.cpp
	bool mAllowWalkingBackwards;
	F32 mAvatarRotateThresholdSlow;
	F32 mAvatarRotateThresholdFast;
	F32 mAvatarRotateThresholdMouselook;
	F32 mMovementRotationSpeed;

	bool mUseFreezeWorld;

	bool mDebugAvatarRezTime;
};

extern BDFunctions gDragonLibrary;

#endif
