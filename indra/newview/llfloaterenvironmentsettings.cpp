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
	mDayCycleSettingsCheck->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::apply, this));

	mWaterPresetCombo = getChild<LLComboBox>("water_settings_preset_combo");
	mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectWaterPreset, this));

	mSkyPresetCombo = getChild<LLComboBox>("sky_settings_preset_combo");
	mSkyPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectSkyPreset, this));

	mDayCyclePresetCombo = getChild<LLComboBox>("dayc_settings_preset_combo");
	mDayCyclePresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectDayCyclePreset, this));

	childSetCommitCallback("cancel_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnCancel, this), NULL);

	//LLEnvironment::instance().setEnvironmentChanged();

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	LLEnvironment &env(LLEnvironment::instance());
	mLiveSky = env.getCurrentSky();
	mLiveWater = env.getCurrentWater();
	mLiveDay = env.getCurrentDay();

	//BD - Refresh all presets.
	refresh();
}

void LLFloaterEnvironmentSettings::onSwitchRegionSettings()
{
	LLEnvironment &env(LLEnvironment::instance());
	env.clearEnvironment(LLEnvironment::ENV_LOCAL);
	env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	env.updateEnvironment();
	mLiveDay = env.getCurrentDay();
}

void LLFloaterEnvironmentSettings::onSelectWaterPreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mWaterPresetCombo, mLiveWater);
}

void LLFloaterEnvironmentSettings::onSelectSkyPreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mSkyPresetCombo, mLiveSky);
}

void LLFloaterEnvironmentSettings::onSelectDayCyclePreset()
{
	//BD
	gDragonLibrary.onSelectPreset(mDayCyclePresetCombo, mLiveDay);
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	// Revert environment to user preferences.
	LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
	closeFloater();
}

void LLFloaterEnvironmentSettings::refresh()
{
	//LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	LLEnvironment &env(LLEnvironment::instance());
	LLSettingsSky::ptr_t sky = env.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
	LLSettingsDay::ptr_t day_cycle = env.getEnvironmentDay(LLEnvironment::ENV_LOCAL);
	bool use_region_settings = !sky;

	//BD - Set up radio buttons according to user preferences.
	mRegionSettingsButton->setValue(use_region_settings);

	// Populate the combo boxes with appropriate lists of available presets.
	gDragonLibrary.loadPresetsFromDir(mWaterPresetCombo, "water");
	gDragonLibrary.loadPresetsFromDir(mSkyPresetCombo, "skies");
	gDragonLibrary.loadPresetsFromDir(mDayCyclePresetCombo, "days");

	// Select the current presets in combo boxes.
	LLSettingsWater::ptr_t water = env.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL);
	if (water)
		mWaterPresetCombo->selectByValue(water->getName());
	if (sky)
		mSkyPresetCombo->selectByValue(sky->getName());
	if (day_cycle)
		mDayCyclePresetCombo->selectByValue(day_cycle->getName());
}

void LLFloaterEnvironmentSettings::apply()
{
	// Update environment with the user choice.
	//BD
	//LLEnvironment &env(LLEnvironment::instance());
	//LLSettingsSky::ptr_t sky = env.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
	//LLSettingsDay::ptr_t day = env.getEnvironmentDay(LLEnvironment::ENV_LOCAL);
	//bool use_region_settings	= !sky;
	//bool use_fixed_sky			= !day;
	//bool windlight_transition	= gSavedSettings.getBOOL("RenderInterpolateWindlight");
	//std::string water_preset	= mWaterPresetCombo->getValue().asString();
	//std::string sky_preset		= mSkyPresetCombo->getValue().asString();
	//std::string day_cycle		= mDayCyclePresetCombo->getValue().asString();

	//LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	/*if (use_region_settings)
	{
		env.clearEnvironment(LLEnvironment::ENV_LOCAL);
		env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
		env.updateEnvironment();
		//BD - Animated Windlight Transition
		//env_mgr.setUseRegionSettings(true, windlight_transition);
	}
	else
	{
		if(use_fixed_sky)
		{
			//BD - Animated Windlight Transition
			gDragonLibrary.loadPreset(day_cycle, mLiveDay);
			env_mgr.setUseDayCycle(day_cycle, windlight_transition);
		}
		else
		{
			//BD - Animated Windlight Transition
			gDragonLibrary.loadPreset(sky_preset, mLiveSky);
			//env_mgr.setUseSkyPreset(sky_preset, windlight_transition);
		}

		//BD - Animated Windlight Transition
		gDragonLibrary.loadPreset(water_preset, mLiveWater);
		//env_mgr.setUseWaterPreset(water_preset, windlight_transition);
	}*/
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mWaterPresetCombo, "water");
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mSkyPresetCombo, "skies");
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	gDragonLibrary.loadPresetsFromDir(mDayCyclePresetCombo, "days");
}
