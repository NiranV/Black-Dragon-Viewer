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

#include "llviewerprecompiledheaders.h"

#include "bdfunctions.h"

#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llcombobox.h"
#include "lldiriterator.h"
#include "llnotificationsutil.h"
#include "llnotifications.h"
#include "llstartup.h"

//BD - Windlight Stuff
#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llsettingswater.h"
#include "llagent.h"
#include "llinventoryfunctions.h"
#include "llsettingsvo.h"
#include "llenvironment.h"

#include "message.h"

BDFunctions gDragonLibrary;


BDFunctions::BDFunctions()
	: mAllowWalkingBackwards(TRUE)
	, mAvatarRotateThresholdSlow(2.f)
	, mAvatarRotateThresholdFast(2.f)
	, mAvatarRotateThresholdMouselook(120.f)
	, mMovementRotationSpeed(0.2f)
	, mUseFreezeWorld(FALSE)
	, mDebugAvatarRezTime(FALSE)
{
}

BDFunctions::~BDFunctions()
{
}

void BDFunctions::initializeControls()
{
	mAllowWalkingBackwards = gSavedSettings.getBOOL("AllowWalkingBackwards");
	mAvatarRotateThresholdSlow = gSavedSettings.getF32("AvatarRotateThresholdSlow");
	mAvatarRotateThresholdFast = gSavedSettings.getF32("AvatarRotateThresholdFast");
	mAvatarRotateThresholdMouselook = gSavedSettings.getF32("AvatarRotateThresholdMouselook");
	mMovementRotationSpeed = gSavedSettings.getF32("MovementRotationSpeed");

	mUseFreezeWorld = gSavedSettings.getBOOL("UseFreezeWorld");

	mDebugAvatarRezTime = gSavedSettings.getBOOL("DebugAvatarRezTime");
}

//BD - Array Debugs
void BDFunctions::onCommitX(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find Z or W if our control is not a vector3 or vector4
			//     respectively. Nothing bad happens if we do this but we don't need a pointless
			//     UI warning either.
			if (type == TYPE_VEC3 || type == TYPE_VEC3D
				|| type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}

			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VY] = value;
		vec4.mV[VZ] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VX] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
}

void BDFunctions::onCommitY(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find Z or W if our control is not a vector3 or vector4
			//     respectively. Nothing bad happens if we do this but we don't need a pointless
			//     UI warning either.
			if (type == TYPE_VEC3 || type == TYPE_VEC3D
				|| type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}

			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VZ] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VY] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
}

void BDFunctions::onCommitZ(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	eControlType type = control->type();
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			//BD - Don't even attempt to find W if our control is not a vector4. Nothing bad 
			//     happens if we do this but we don't need a pointless UI warning either.
			if (type == TYPE_VEC4)
			{
				other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_W");
				if (other_ctrl)
				{
					other_ctrl->setValue(value);
				}
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VY] = value;
		vec4.mV[VW] = value;
	}
	vec4.mV[VZ] = value;
	gSavedSettings.setUntypedValue(param.asString(), vec4.getValue());

	//BD - Trigger Warning System
	gDragonLibrary.triggerWarning(ctrl, param);
}

//BD - Vector4
void BDFunctions::onCommitW(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	LLVector4 vec4 = LLVector4(control->getValue());
	F32 value = ctrl->getValue().asReal();
	//BD - Option is in link mode.
	if (control->isLocked())
	{
		//BD - Attempt to find the other controls.
		//     Usually we name them control_name + array (e.g RenderDebug_X)
		LLView* parent_view = ctrl->getParent();
		if (parent_view)
		{
			LLUICtrl* other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_X");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Y");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}

			other_ctrl = parent_view->getChild<LLUICtrl>(param.asString() + "_Z");
			if (other_ctrl)
			{
				other_ctrl->setValue(value);
			}
		}

		vec4.mV[VX] = value;
		vec4.mV[VY] = value;
		vec4.mV[VZ] = value;
	}
	vec4.mV[VW] = value;
	gSavedSettings.setVector4(param.asString(), vec4);

	//BD - Trigger Warning System
	triggerWarning(ctrl, param);
}

//BD - Array Lock feature
void BDFunctions::onControlLock(LLUICtrl* ctrl, const LLSD& param)
{
	LLControlVariable* control = gSavedSettings.getControl(param.asString());
	control->setLocked(ctrl->getValue().asBoolean());
}

//BD - Revert to Default
BOOL BDFunctions::resetToDefault(LLUICtrl* ctrl)
{
	LLControlVariable* control = ctrl->getControlVariable();
	if (control)
	{
		control->resetToDefault(true);
		return TRUE;
	}
	return FALSE;
}

//BD - Revert to Default
void BDFunctions::invertValue(LLUICtrl* ctrl)
{
	LLControlVariable* control = ctrl->getControlVariable();
	F32 val;
	//BD - If we find a debug setting linked to the UI widget, change that.
	//     Should we find no debug setting linked we'll change the widget value it will hopefully
	//     trigger whatever is necessary to apply the change or use the new value.
	if (control)
	{
		val = control->getValue().asReal();
		control->setValue(-val);
	}
	else
	{
		val = ctrl->getValue().asReal();
		ctrl->setValue(-val);
	}
}

//BD - Escape string
std::string BDFunctions::escapeString(const std::string& str)
{
	//BD - Don't use LLURI::escape() because it doesn't encode '-' characters
	//     which may break handling of some poses.
	//     From Singularity Viewer.
	static const char hex[] = "0123456789ABCDEF";
	std::stringstream escaped_str;
	for (std::string::const_iterator iter = str.begin(); iter != str.end(); ++iter)
	{
		switch (*iter) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			escaped_str << (*iter);
			break;
		default:
			unsigned char c = (unsigned char)(*iter);
			escaped_str << '%' << hex[c >> 4] << hex[c & 0xF];
		}
	}
	return escaped_str.str();
}

//BD - Trigger Warning System
void BDFunctions::triggerWarning(LLUICtrl* ctrl, const LLSD& param)
{
	LLSD val = ctrl->getValue();
	LLControlVariable* control = ctrl->getControlVariable();
	//BD - Debug was not found, maybe it is an account setting.
	if (!control)
	{
		control = ctrl->getControlVariable();

		//BD - Still not found, check whether we can get the debug via the param.
		if (!control)
		{
			control = gSavedSettings.getControl(param.asString());

			//BD - Debug was not found, maybe it is an account setting.
			if (!control)
			{
				control = gSavedPerAccountSettings.getControl(param.asString());

				//BD - Give up.
				if (!control) return;
			}
		}
	}

	eControlType type = control->type();
	std::string name = control->getName();
	F32 max_val = control->getMaxValue();
	F32 min_val = control->getMinValue();

	//BD - We name all our warnings by the control's name, this makes it super easy
	//     to distinguish which warning triggers for which control and when, this also
	//     plays well into the fact that certain widgets will have other functions already
	//     present such as Vector sliders which use the control name in its parameter.
	if (type == TYPE_BOOLEAN)
	{
		if (val.asBoolean())
		{
			LLNotificationsUtil::add(name);
			//BD - I don't think these one-off options should be muted after the first sight.
			//     keep pestering the user that the option they enabled is up to no good.
			//LLNotifications::instance().setIgnored(name, true);
		}
	}
	//else if (type == TYPE_S32 || type == TYPE_U32 || type == TYPE_F32)
	else if (type == TYPE_VEC2 || type == TYPE_VEC3 || type == TYPE_VEC3D || type == TYPE_VEC4
			|| type == TYPE_S32 || type == TYPE_U32 || type == TYPE_F32)
	{
		if (val.asReal() > max_val)
		{
			LLNotificationsUtil::add(name + "_Max");
			//BD - Don't spam the user every step.
			LLNotifications::instance().setIgnored(name + "_Max", true);
			LL_WARNS("Notifications") << "Value set to: " << val.asReal() << " but maximum allowed is: " << max_val << LL_ENDL;
		}
		else if (val.asReal() < min_val)
		{
			LLNotificationsUtil::add(name + "_Min");
			//BD - Don't spam the user every step.
			LLNotifications::instance().setIgnored(name + "_Min", true);
			LL_WARNS("Notifications") << "Value set to: " << val.asReal() << " but minimum allowed is: " << min_val << LL_ENDL;
		}
	}
}

//BD - Trigger Warning System
void BDFunctions::openPreferences( const LLSD& param)
{
	LLFloaterReg::toggleInstanceOrBringToFront("preferences");
	LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
	if (preferences && preferences->getVisible() && !preferences->isMinimized())
	{
		LLTabContainer* prefs_tabs = preferences->getChild<LLTabContainer>("pref core");
		if (prefs_tabs)
		{
			prefs_tabs->selectTabByName(param);
		}
	}
}

//BD - Factory Reset
void BDFunctions::askFactoryReset(const LLSD& param)
{
	LLNotificationsUtil::add(param,	LLSD(),	LLSD(),	&doFactoryReset);
}

void BDFunctions::doFactoryReset(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	//BD - Cancel
	if (option == 1)
	{
		return;
	}

	LL_WARNS("Notifications") << "ABANDON SHIP. We are resetting the Viewer to stock." << LL_ENDL;
	gSavedSettings.doFactoryReset();
	//BD - Do not allow resetting personal settings before we've logged in.
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		gSavedPerAccountSettings.doFactoryReset();
	}
}


//BD - Windlight Stuff
//=====================================================================================================
// Used for sorting
struct SortItemPtrsByName
{
	bool operator()(const LLInventoryItem* i1, const LLInventoryItem* i2)
	{
		return (LLStringUtil::compareDict(i1->getName(), i2->getName()) < 0);
	}
};

bool BDFunctions::checkPermissions(LLUUID uuid)
{
	const LLInventoryItem *item = gInventory.getItem(uuid);
	if (!item)
		return false;

	LLPermissions perms = item->getPermissions();
	if (!perms.allowTransferTo(gAgentID)
		&& !perms.allowModifyBy(gAgentID)
		&& !perms.allowCopyBy(gAgentID))
		return false;

	return true;
}

void BDFunctions::addInventoryPresets(LLComboBox* combo, LLSettingsBase::ptr_t settings)
{
	if (!combo || !settings) return;

	LLSettingsType::type_e type = settings->getSettingsType() == "sky" ? LLSettingsType::ST_SKY : LLSettingsType::ST_WATER;

	// Get all inventory items that are animations
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLIsTypeWithPermissions is_copyable_animation(LLAssetType::AT_SETTINGS,
		PERM_TRANSFER,
		gAgent.getID(),
		gAgent.getGroupID());
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_copyable_animation);

	// Copy into something we can sort
	std::vector<LLViewerInventoryItem*> animations;

	S32 i;
	S32 count = items.size();
	for (i = 0; i < count; ++i)
	{
		animations.push_back(items.at(i));
	}

	// Do the sort
	std::sort(animations.begin(), animations.end(), SortItemPtrsByName());

	combo->addSeparator();

	// And load up the combobox
	std::vector<LLViewerInventoryItem*>::iterator it;
	for (it = animations.begin(); it != animations.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;
		if (item->getSettingsType() == type)
			combo->add(item->getName(), item->getUUID(), ADD_BOTTOM);
	}
}

void BDFunctions::onSelectPreset(LLComboBox* combo, LLSettingsBase::ptr_t settings)
{
	if (!combo || !settings) return;

	std::string type = settings->getSettingsType();
	std::string folder = type == "sky" ? "skies" : "water";

	//BD - First attempt to load it as inventory item.
	if (combo->getValue().isUUID())
	{
		LLUUID uuid = combo->getValue();
		LLViewerInventoryItem* item = gInventory.getItem(uuid);
		if (item)
		{
			LLSettingsVOBase::getSettingsAsset(item->getAssetUUID(), [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { loadItem(settings); });
			//BD - Assume loading was successful.
			return;
		}
	}

	//BD - Loading as inventory item failed so it must be a local preset.
	std::string name = gDragonLibrary.escapeString(combo->getValue().asString());
	std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/" + folder, name + ".xml");
	if (!loadPreset(dir, settings))
	{
		//BD - Last attempt, try to find it in user_settings.
		dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "windlight/" + folder, name + ".xml");
		if (!loadPreset(dir, settings))
		{
			LLNotificationsUtil::add("BDCantLoadPreset");
			LL_WARNS("Windlight") << "Failed to load sky preset from:" << dir << LL_ENDL;
		}
	}
}

void BDFunctions::loadItem(LLSettingsBase::ptr_t settings)
{
	if (!settings) return;

	LLEnvironment &env(LLEnvironment::instance());
	std::string type = settings->getSettingsType();
	if (type == "sky")
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsSky>(settings));
	else
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsWater>(settings));
	env.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

bool BDFunctions::loadPreset(std::string filename, LLSettingsBase::ptr_t settings)
{
	if (!settings || filename.empty()) return false;

	llifstream xml_file;
	xml_file.open(filename);
	if (!xml_file)
		return false;

	LLSD params_data;
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	if (parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
	{
		xml_file.close();
		LLNotificationsUtil::add("BDCantParsePreset");
		return false;
	}
	xml_file.close();

	LLEnvironment &env(LLEnvironment::instance());
	LLSD messages;
	std::string type = settings->getSettingsType();
	if (type == "sky")
		settings = !params_data.has("version") ? env.createSkyFromLegacyPreset(filename, messages) : env.createSkyFromPreset(filename, messages);
	else
		settings = !params_data.has("version") ? env.createWaterFromLegacyPreset(filename, messages) : env.createWaterFromPreset(filename, messages);

	if (!settings)
	{
		LLNotificationsUtil::add("WLImportFail", messages);
		return false;
	}

	env.setEnvironment(LLEnvironment::ENV_LOCAL, settings, -2);
	env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	env.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);

	return true;
}

//BD - Windlight functions
void BDFunctions::savePreset(std::string name, LLSettingsBase::ptr_t settings)
{
	if (!settings || name.empty()) return;

	LLSD Params;
	std::string folder;
	Params = settings->getSettings();
	if (settings->getSettingsType() == "sky")
	{
		folder = "skies";
		LLSettingsSky::ptr_t sky = std::static_pointer_cast<LLSettingsSky>(settings);
		if (sky)
		{
			//BD - Remove sun/cloud/moon texture information from preset if we don't have
			//     permissions to include them.
			if (!checkPermissions(sky->getMoonTextureId()))
				Params.erase("moon_id");

			if (!checkPermissions(sky->getSunTextureId()))
				Params.erase("sun_id");

			if (!checkPermissions(sky->getCloudNoiseTextureId()))
				Params.erase("cloud_id");
		}
	}
	else
	{
		folder = "water";
		LLSettingsWater::ptr_t water = std::static_pointer_cast<LLSettingsWater>(settings);
		if (water)
		{
			//BD - Remove water texture information from preset if we don't have
			//     permissions to include them.
			if (!checkPermissions(water->getNormalMapID()))
				Params.erase("normal_map");
		}
	}

	// make an empty llsd
	std::string pathName(getUserDir(folder) + escapeString(name) + ".xml");

	Params["version"] = "eep";

	// write to file
	llofstream presetsXML(pathName.c_str());
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(Params, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
	presetsXML.close();
}

void BDFunctions::deletePreset(std::string name, std::string folder)
{
	if (name.empty() || folder.empty()) return;

	// Don't allow deleting system presets.
	if ((folder == "skies" && gDragonLibrary.mDefaultSkyPresets.has(name))
		|| (folder == "water" && gDragonLibrary.mDefaultWaterPresets.has(name)))
	{
		LLNotificationsUtil::add("WLNoEditDefault");
		return;
	}
	else
	{
		std::string path_name(getUserDir(folder));
		std::string escaped_name = escapeString(name);

		if (gDirUtilp->deleteFilesInDir(path_name, escaped_name + ".xml") < 1)
		{
			LLNotificationsUtil::add("BDCantRemovePreset");
			LL_WARNS("WindLight") << "Error removing sky preset " << name << " from disk" << LL_ENDL;
		}
	}
}

void BDFunctions::loadPresetsFromDir(LLComboBox* combo, std::string folder)
{
	if (!combo || folder.empty()) return;

	bool is_sky = folder == "skies";
	combo->clearRows();
	std::string dir = getSysDir(folder);
	std::string file;
	if (!dir.empty() || gDirUtilp->fileExists(dir))
	{
		LLDirIterator dir_iter(dir, "*.xml");
		while (dir_iter.next(file))
		{
			std::string path = gDirUtilp->add(dir, file);
			std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);
			combo->add(name);
			if (is_sky)
				mDefaultSkyPresets[name] = name;
			else
				mDefaultWaterPresets[name] = name;

			if (!doLoadPreset(path))
			{
				LL_WARNS() << "Error loading sky preset from " << path << LL_ENDL;
			}
		}
	}

	combo->addSeparator();

	dir = getUserDir(folder);
	if (!dir.empty() || gDirUtilp->fileExists(dir))
	{
		LLDirIterator dir_it(dir, "*.xml");
		while (dir_it.next(file))
		{
			std::string path = gDirUtilp->add(dir, file);
			std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);
			combo->add(name);

			if (!doLoadPreset(path))
			{
				LL_WARNS() << "Error loading sky preset from " << path << LL_ENDL;
			}
		}
	}
}

bool BDFunctions::doLoadPreset(const std::string& path)
{
	llifstream xml_file;
	std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), true));

	xml_file.open(path.c_str());
	if (!xml_file)
	{
		return false;
	}

	LL_DEBUGS("AppInit", "Shaders") << "Loading sky " << name << LL_ENDL;

	LLSD params_data;
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED);
	xml_file.close();

	return true;
}

//BD - Multiple Viewer Presets
// static
std::string BDFunctions::getSysDir(std::string folder)
{
	if (folder.empty()) return NULL;

	std::string sys_dir = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", folder, "");
	return sys_dir;
}

// static
std::string BDFunctions::getUserDir(std::string folder)
{
	if (folder.empty()) return NULL;

	std::string	user_dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "windlight", folder, "");
	return user_dir;
}