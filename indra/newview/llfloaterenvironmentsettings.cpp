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
#include "llcheckboxctrl.h"

#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llwaterparammanager.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"

//BD
#include "llviewercontrol.h"

LLFloaterEnvironmentSettings::LLFloaterEnvironmentSettings(const LLSD &key)
: 	 LLFloater(key)
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
	mRegionSettingsButton = getChild<LLButton>("region_settings_radio_group");
	mRegionSettingsButton->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSwitchRegionSettings, this));

	mDayCycleSettingsCheck = getChild<LLCheckBoxCtrl>("sky_dayc_settings_check");
	mDayCycleSettingsCheck->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::apply, this));

	mWaterPresetCombo = getChild<LLComboBox>("water_settings_preset_combo");
	mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectWaterPreset, this));

	mSkyPresetCombo = getChild<LLComboBox>("sky_settings_preset_combo");
	mSkyPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectSkyPreset, this));

	mDayCyclePresetCombo = getChild<LLComboBox>("dayc_settings_preset_combo");
	mDayCyclePresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectDayCyclePreset, this));

	childSetCommitCallback("cancel_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnCancel, this), NULL);
	getChild<LLUICtrl>("cancel_btn")->setRightMouseDownCallback(boost::bind(&LLEnvManagerNew::dumpPresets, LLEnvManagerNew::getInstance()));

	LLEnvManagerNew::instance().setPreferencesChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::refresh, this));
	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEnvironmentSettings::populateDayCyclePresetsList, this));
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateSkyPresetsList, this));
	LLWaterParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateWaterPresetsList, this));

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	refresh();
}

void LLFloaterEnvironmentSettings::onSwitchRegionSettings()
{
	getChild<LLView>("user_environment_settings")->setEnabled(!gSavedSettings.getBOOL("UseEnvironmentFromRegion"));

	apply();
}

void LLFloaterEnvironmentSettings::onSelectWaterPreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onSelectSkyPreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onSelectDayCyclePreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	// Revert environment to user preferences.
	LLEnvManagerNew::instance().usePrefs();
	closeFloater();
}

void LLFloaterEnvironmentSettings::refresh()
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();

	bool use_region_settings	= env_mgr.getUseRegionSettings();

	// Set up radio buttons according to user preferences.
	mRegionSettingsButton->setValue(use_region_settings);

	// Populate the combo boxes with appropriate lists of available presets.
	populateWaterPresetsList();
	populateSkyPresetsList();
	populateDayCyclePresetsList();

	// Enable/disable other controls based on user preferences.
	getChild<LLView>("user_environment_settings")->setEnabled(!use_region_settings);

	// Select the current presets in combo boxes.
	mWaterPresetCombo->selectByValue(env_mgr.getWaterPresetName());
	mSkyPresetCombo->selectByValue(env_mgr.getSkyPresetName());
	mDayCyclePresetCombo->selectByValue(env_mgr.getDayCycleName());
}

void LLFloaterEnvironmentSettings::apply()
{
	// Update environment with the user choice.
	bool use_region_settings	= gSavedSettings.getBOOL("UseEnvironmentFromRegion");
	bool use_fixed_sky			= gSavedSettings.getBOOL("UseDayCycle");
	std::string water_preset	= mWaterPresetCombo->getValue().asString();
	std::string sky_preset		= mSkyPresetCombo->getValue().asString();
	std::string day_cycle		= mDayCyclePresetCombo->getValue().asString();

	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	if (use_region_settings)
	{
		env_mgr.setUseRegionSettings(true, gSavedSettings.getBOOL("RenderInterpolateWindlight"));
	}
	else
	{
		if(use_fixed_sky)
		{
			env_mgr.setUseDayCycle(day_cycle, gSavedSettings.getBOOL("RenderInterpolateWindlight"));
		}
		else
		{
			env_mgr.setUseSkyPreset(sky_preset, gSavedSettings.getBOOL("RenderInterpolateWindlight"));
		}

		env_mgr.setUseWaterPreset(water_preset, gSavedSettings.getBOOL("RenderInterpolateWindlight"));
	}
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();

	std::list<std::string> user_presets, system_presets;
	LLWaterParamManager::instance().getPresetNames(user_presets, system_presets);

	// Add user presets first.
	for (std::list<std::string>::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}

	if (user_presets.size() > 0)
	{
		mWaterPresetCombo->addSeparator();
	}

	// Add system presets.
	for (std::list<std::string>::const_iterator it = system_presets.begin(); it != system_presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t region_presets; // unused as we don't list region presets here
	LLWLParamManager::preset_name_list_t user_presets, sys_presets;
	LLWLParamManager::instance().getPresetNames(region_presets, user_presets, sys_presets);

	// Add user presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	if (!user_presets.empty())
	{
		mSkyPresetCombo->addSeparator();
	}

	// Add system presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = sys_presets.begin(); it != sys_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	mDayCyclePresetCombo->removeall();

	LLDayCycleManager::preset_name_list_t user_days, sys_days;
	LLDayCycleManager::instance().getPresetNames(user_days, sys_days);

	// Add user days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
	{
		mDayCyclePresetCombo->add(*it);
	}

	if (user_days.size() > 0)
	{
		mDayCyclePresetCombo->addSeparator();
	}

	// Add system days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
	{
		mDayCyclePresetCombo->add(*it);
	}
}
