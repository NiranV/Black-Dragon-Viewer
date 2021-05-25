/** 
 * @file llfloaterenvironmentsettings.cpp
 * @brief LLFloaterEnvironmentSettings class definition
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterenvironmentsettings.h"

#include "llcombobox.h"
#include "llradiogroup.h"
#include "llenvironment.h"

//BD
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"

#include "bdfunctions.h"

LLFloaterEnvironmentSettings::LLFloaterEnvironmentSettings(const LLSD &key)
: 	 LLFloater(key)
	//BD
	,mRegionSettingsButton(NULL)
	,mDayCycleSettingsCheck(NULL)
	,mWaterPresetCombo(NULL)
	,mSkyPresetCombo(NULL)
	,mDayCyclePresetCombo(NULL)
{	
}

// virtual
BOOL LLFloaterEnvironmentSettings::postBuild()
{
	//BD
	mRegionSettingsButton = getChild<LLButton>("estate_btn");
	mRegionSettingsButton->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSwitchRegionSettings, this));

	//BD
	mDayCycleSettingsCheck = getChild<LLCheckBoxCtrl>("sky_dayc_settings_check");
	mDayCycleSettingsCheck->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onChangeToRegion, this));

	mWaterPresetCombo = getChild<LLComboBox>("water_settings_preset_combo");
	mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectWaterPreset, this));

	mSkyPresetCombo = getChild<LLComboBox>("sky_settings_preset_combo");
	mSkyPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectSkyPreset, this));

	mDayCyclePresetCombo = getChild<LLComboBox>("dayc_settings_preset_combo");
	mDayCyclePresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectDayCyclePreset, this));

	childSetCommitCallback("cancel_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnCancel, this), NULL);

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	//BD - Get our current environment presets, initialize defaults if we don't have one
	//     to make sure we have something, otherwise loading presets is not going to work.
	LLEnvironment &env(LLEnvironment::instance());
	mLiveSky = env.getCurrentSky();
	if (mLiveSky)
		mLiveSky->defaults();
	mLiveWater = env.getCurrentWater();
	if (mLiveWater)
		mLiveWater->defaults();
	mLiveDay = env.getCurrentDay();
	if (mLiveDay)
		mLiveDay->defaults();

	// Populate the combo boxes with appropriate lists of available presets.
	populateSkyPresetsList();
	populateDayCyclePresetsList();
	populateWaterPresetsList();

	//BD - Refresh all presets.
	refresh();
}

void LLFloaterEnvironmentSettings::onSwitchRegionSettings()
{
	LLEnvironment &env(LLEnvironment::instance());
	if (mRegionSettingsButton->getValue().asBoolean())
	{
		//BD - Make sure that if we use the region light we disallow saving.
		env.setLocalPreset(false);
		env.clearEnvironment(LLEnvironment::ENV_LOCAL);
		env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
		env.updateEnvironment(F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));
		mLiveDay = env.getCurrentDay();
		refresh();
	}
	else
	{
		//BD - When we switch back to our presets, allow us to save again.
		env.setLocalPreset(true);
		onChangeToRegion();
	}
}

void LLFloaterEnvironmentSettings::onChangeToRegion()
{
	LLEnvironment &env(LLEnvironment::instance());
	if (mDayCycleSettingsCheck->getValue().asBoolean())
	{
		if (mLiveDay && mDayCyclePresetCombo->getValue())
		{
			//BD - When switching to daycycles, load and apply whatever daycycle
			//     we currently have selected.
			onSelectDayCyclePreset();
		}
		else
		{
			env.clearEnvironment(LLEnvironment::ENV_LOCAL);
			env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
			env.updateEnvironment(F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));
			mLiveDay = env.getCurrentDay();
			refresh();
		}
	}
	else
	{
		if (mLiveSky && mSkyPresetCombo->getValue())
		{
			//BD - When switching to skies, load and apply whatever sky
			//     we currently have selected.
			onSelectSkyPreset();
		}
		else
		{
			//BD - If we dont have a sky preset simply use the default midday.
			env.setEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::KNOWN_SKY_MIDDAY);
			env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
			env.updateEnvironment(F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));
			refresh();
		}
	}
}

void LLFloaterEnvironmentSettings::onSelectWaterPreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mWaterPresetCombo, mLiveWater);
	gSavedSettings.setString("WaterPresetName", mWaterPresetCombo->getValue());
	refresh();
}

void LLFloaterEnvironmentSettings::onSelectSkyPreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mSkyPresetCombo, mLiveSky);
	gSavedSettings.setString("SkyPresetName", mSkyPresetCombo->getValue());
	refresh();
}

void LLFloaterEnvironmentSettings::onSelectDayCyclePreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mDayCyclePresetCombo, mLiveDay);
	gSavedSettings.setString("DayCycleName", mDayCyclePresetCombo->getValue());
	refresh();
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	// Revert environment to user preferences.
	/*LLEnvironment &env(LLEnvironment::instance());
	env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	env.clearEnvironment(LLEnvironment::ENV_EDIT);
	env.setLocalPreset(false);*/
	closeFloater();
}

void LLFloaterEnvironmentSettings::refresh()
{
	bool use_region_settings = false;
	LLSettingsSky::ptr_t sky = LLEnvironment::instance().getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
	if (!sky)
		use_region_settings = true;

	//BD - Set up radio buttons according to user preferences.
	mRegionSettingsButton->setValue(use_region_settings);

	// Select the current presets in combo boxes.
	if (mLiveWater)
		mWaterPresetCombo->setValue(gSavedSettings.getString("WaterPresetName"));
	if (mLiveSky)
		mSkyPresetCombo->setValue(gSavedSettings.getString("SkyPresetName"));
	if (mLiveDay)
		mDayCyclePresetCombo->setValue(gSavedSettings.getString("DayCycleName"));
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mWaterPresetCombo, "water");
	gDragonLibrary.addInventoryPresets(mWaterPresetCombo, mLiveWater);
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mSkyPresetCombo, "skies");
	gDragonLibrary.addInventoryPresets(mSkyPresetCombo, mLiveSky);
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mDayCyclePresetCombo, "days");
	gDragonLibrary.addInventoryPresets(mDayCyclePresetCombo, mLiveDay);
}
