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
#include "llsettingsbase.h"

class LLComboBox;

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

	static S32 checkDeveloper(LLUUID id);

	static void askFactoryReset(const LLSD& param);
	static void doFactoryReset(const LLSD& notification, const LLSD& response);

//	//BD - Camera Recorder
	bool getCameraOverride() { return mCameraOverride; }

//	//BD - Windlight functions
	LLSettingsBase::ptr_t cullAssets(LLSettingsBase::ptr_t settings);
	void savePreset(std::string name, LLSettingsBase::ptr_t settings);
	void deletePreset(std::string name, std::string folder = "skies");
	void loadPresetsFromDir(LLComboBox* combo, std::string folder = "skies");
	bool doLoadPreset(const std::string& path);
	void loadAllPresets(LLSD& presets);
	static std::string getWindlightDir(std::string folder, bool system = false);
	bool checkPermissions(LLUUID uuid);
	void onSelectPreset(LLComboBox* combo, LLSettingsBase::ptr_t settings);
	void addInventoryPresets(LLComboBox* combo, LLSettingsBase::ptr_t settings);
	void loadItem(LLSettingsBase::ptr_t settings);
	bool loadPreset(std::string filename, LLSettingsBase::ptr_t settings);
	void loadSettingFromFile(const std::vector<std::string>& filenames);

	bool mCameraOverride;

	LLSD mDefaultSkyPresets;
	LLSD mDefaultWaterPresets;
	LLSD mDefaultDayCyclePresets;

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
