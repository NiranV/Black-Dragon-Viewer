/**
 * @file llfloaterfixedenvironment.cpp
 * @brief Floaters to create and edit fixed settings for sky and water.
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

#include "llfloaterfixedenvironment.h"

#include <boost/make_shared.hpp>

 // libs
#include "llbutton.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "llfilepicker.h"
#include "lllocalbitmaps.h"
#include "llsettingspicker.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread
#include "llviewerparcelmgr.h"

// newview
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llenvironment.h"
#include "llagent.h"
#include "llparcel.h"
#include "lltrans.h"

#include "llsettingsvo.h"
#include "llinventorymodel.h"

//BD
#include "llcombobox.h"
#include "bdfunctions.h"
#include "llsdserialize.h"
#include "llsettingsvo.h"
#include "llviewermenufile.h"
#include "llinventorymodel.h"
#include "llinventoryfunctions.h"
#include "llagent.h"
#include "lltexturectrl.h"

extern LLControlGroup gSavedSettings;

namespace
{
	const std::string FIELD_SETTINGS_NAME("sky_preset_combo");

	const std::string CONTROL_TAB_AREA("tab_settings");

	const std::string BUTTON_NAME_IMPORT("btn_import");
	const std::string BUTTON_NAME_COMMIT("btn_commit");
	const std::string BUTTON_NAME_CANCEL("btn_cancel");
	const std::string BUTTON_NAME_FLYOUT("btn_flyout");
	const std::string BUTTON_NAME_LOAD("btn_load");

	const std::string ACTION_SAVE("save_settings");
	const std::string ACTION_SAVELOCAL("save_as_local_setting");
	const std::string ACTION_SAVEAS("save_as_new_settings");
	const std::string ACTION_COMMIT("commit_changes");
	const std::string ACTION_APPLY_LOCAL("apply_local");
	const std::string ACTION_APPLY_PARCEL("apply_parcel");
	const std::string ACTION_APPLY_REGION("apply_region");

	const std::string XML_FLYOUTMENU_FILE("menu_save_settings.xml");

	//BD - Windlight Stuff
	const S32 FLOATER_ENVIRONMENT_UPDATE(-2);

	//const std::string   BTN_RESET("reset");
	const std::string   BTN_SAVE("save");
	const std::string   BTN_DELETE("delete");
	//const std::string   BTN_IMPORT("import");
}

//=========================================================================
const std::string LLFloaterFixedEnvironment::KEY_INVENTORY_ID("inventory_id");


//=========================================================================

class LLFixedSettingCopiedCallback : public LLInventoryCallback
{
public:
	LLFixedSettingCopiedCallback(LLHandle<LLFloater> handle) : mHandle(handle) {}

	virtual void fire(const LLUUID& inv_item_id)
	{
		if (!mHandle.isDead())
		{
			LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);
			if (item)
			{
				LLFloaterFixedEnvironment* floater = (LLFloaterFixedEnvironment*)mHandle.get();
				floater->onInventoryCreated(item->getAssetUUID(), inv_item_id);
			}
		}
	}

private:
	LLHandle<LLFloater> mHandle;
};

//=========================================================================
LLFloaterFixedEnvironment::LLFloaterFixedEnvironment(const LLSD &key) :
	LLFloater(key),
	mFlyoutControl(nullptr),
	mInventoryId(),
	mInventoryItem(nullptr),
	mIsDirty(false),
	mCanCopy(false),
	mCanMod(false),
	mCanTrans(false),
	//BD
	mIsLocalEdit(false)
{
}

LLFloaterFixedEnvironment::~LLFloaterFixedEnvironment()
{
	delete mFlyoutControl;
}

BOOL LLFloaterFixedEnvironment::postBuild()
{
	mTab = getChild<LLTabContainer>(CONTROL_TAB_AREA);
	mTxtName = getChild<LLComboBox>(FIELD_SETTINGS_NAME);
	mTxtName->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSelectPreset(); });

	getChild<LLButton>(BUTTON_NAME_IMPORT)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });
	getChild<LLButton>(BUTTON_NAME_CANCEL)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onClickCloseBtn(); });
	getChild<LLButton>(BUTTON_NAME_LOAD)->setClickedCallback([this](LLUICtrl *, const LLSD &) { onButtonLoad(); });

	//BD
	getChild<LLUICtrl>(BTN_DELETE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonDelete(); });

	mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BUTTON_NAME_COMMIT, BUTTON_NAME_FLYOUT, XML_FLYOUTMENU_FILE, false);
	mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });
	mFlyoutControl->setMenuItemVisible(ACTION_COMMIT, false);

	return TRUE;
}

void LLFloaterFixedEnvironment::onOpen(const LLSD& key)
{
	mSettings = nullptr;
	mInventoryItem = nullptr;

	LLUUID invid;
	if (key.has(KEY_INVENTORY_ID))
	{
		invid = key[KEY_INVENTORY_ID].asUUID();
	}

	loadInventoryItem(invid);
	if (!mInventoryItem)
		LL_INFOS("SETTINGS") << "Environment Edit opened with no UUID, assuming this is local edit." << LL_ENDL;
	else
		LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;

	mIsLocalEdit = !mInventoryItem;
	updateEditEnvironment();

	//ssyncronizeTabs();
	populatePresetsList();
	refresh();
}

void LLFloaterFixedEnvironment::onClose(bool app_quitting)
{
	doCloseInventoryFloater(app_quitting);
}

void LLFloaterFixedEnvironment::onFocusReceived()
{
	if (isInVisibleChain())
	{
		mPreviousSettings = mSettings;
		updateEditEnvironment();
		if (mPreviousSettings != mSettings)
		{
			refresh();
		}
	}
}

void LLFloaterFixedEnvironment::onFocusLost()
{
}

void LLFloaterFixedEnvironment::refresh()
{
	if (!mSettings)
	{
		// disable everything.
		return;
	}

	//BD - If we enter through an item we will proceed as normal, checking for its permissions
	//     to determine what we can and can't do. If we enter through any other means (local) we
	//     lock out all options and only display local saving if this is a local preset we loaded
	//     which we currently can only know AFTER specifically switching to a local preset for the
	//     first time.
	//     Note: Since there is no way of saving the state of whether we have a local preset loaded
	//     or not (which could also be manipulated if its a setting) we'll sadly have to assume that
	//     we don't have a local preset even if we do, forcing the user to switch the preset once
	//     to trigger the check. It's a minor inconvenience but this horrible system currently leaves
	//     me with no other option.
	bool is_inventory_avail = canUseInventory();
	bool is_local = LLEnvironment::instance().isLocalPreset();
	bool visible = is_inventory_avail && mCanMod && !mInventoryId.isNull();
	mFlyoutControl->setSelectedItem(!visible ? is_local ? ACTION_SAVELOCAL : ACTION_APPLY_PARCEL : ACTION_SAVE);
	mFlyoutControl->setMenuItemVisible(ACTION_SAVE, visible);
	mFlyoutControl->setMenuItemVisible(ACTION_SAVEAS, is_inventory_avail && (mCanCopy || is_local));
	mFlyoutControl->setMenuItemVisible(ACTION_SAVELOCAL, mCanCopy || is_local);
	mFlyoutControl->setMenuItemVisible(ACTION_APPLY_PARCEL, canApplyParcel());
	mFlyoutControl->setMenuItemVisible(ACTION_APPLY_REGION, canApplyRegion());

	std::string debug = mSettings->getSettingsType() == "sky" ? "SkyPresetName" : "WaterPresetName";
	std::string preset_name = mIsLocalEdit ? gSavedSettings.getString(debug) : mSettings->getName();
	mTxtName->setLabel(preset_name);
	mTxtName->setValue(preset_name);
	mTxtName->setEnabled(mCanMod);

	std::string title = is_local ? getString("CanSave") : getString("CantSave");
	setTitle(title);

	syncronizeTabs();
}

void LLFloaterFixedEnvironment::populatePresetsList()
{
    mSettings = settings; // shouldn't this do buildDeepCloneAndUncompress() ?
    updateEditEnvironment();
    syncronizeTabs();
    refresh();
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);

	//BD
	std::string type = mSettings->getSettingsType();
	std::string folder = type == "sky" ? "skies" : "water";
	gDragonLibrary.loadPresetsFromDir(mTxtName, folder);
	gDragonLibrary.addInventoryPresets(mTxtName, mSettings);

    // teach user about HDR settings
    if (mSettings && ((LLSettingsSky*)mSettings.get())->canAutoAdjust())
    {
        LLNotificationsUtil::add("AutoAdjustHDRSky");
    }
}

void LLFloaterFixedEnvironment::syncronizeTabs()
{
	S32 count = mTab->getTabCount();

	for (S32 idx = 0; idx < count; ++idx)
	{
		LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
		if (panel)
		{
			panel->setSettings(mSettings);
			panel->setCanChangeSettings(mCanMod);
			panel->refresh();
		}
	}
}

LLFloaterSettingsPicker * LLFloaterFixedEnvironment::getSettingsPicker()
{
	LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mInventoryFloater.get());

	// Show the dialog
	if (!picker)
	{
		picker = new LLFloaterSettingsPicker(this,
			LLUUID::null);

		mInventoryFloater = picker->getHandle();

		picker->setCommitCallback([this](LLUICtrl *, const LLSD &data) { onPickerCommitSetting(data["ItemId"].asUUID() /*data["Track"]*/); });
	}

	return picker;
}

void LLFloaterFixedEnvironment::loadInventoryItem(const LLUUID  &inventoryId, bool can_trans)
{
	if (inventoryId.isNull())
	{
		mInventoryItem = nullptr;
		mInventoryId.setNull();
		//BD - Make it so presets are always editable but only saveable when its a local preset
		//     or we have an item with permissions.
		bool is_local = LLEnvironment::instance().isLocalPreset();
		mCanMod = true;
		mCanCopy = is_local;
		mCanTrans = is_local;
		return;
	}

	mInventoryId = inventoryId;
	LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
	mInventoryItem = gInventory.getItem(mInventoryId);

	if (!mInventoryItem)
	{
		LL_WARNS("SETTINGS") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;
		LLNotificationsUtil::add("CantFindInvItem");
		closeFloater();

		mInventoryId.setNull();
		mInventoryItem = nullptr;
		return;
	}

	if (mInventoryItem->getAssetUUID().isNull())
	{
		LL_WARNS("ENVIRONMENT") << "Asset ID in inventory item is NULL (" << mInventoryId << ")" << LL_ENDL;
		LLNotificationsUtil::add("UnableEditItem");
		closeFloater();

		mInventoryId.setNull();
		mInventoryItem = nullptr;
		return;
	}

	mCanCopy = mInventoryItem->getPermissions().allowCopyBy(gAgent.getID());
	mCanMod = mInventoryItem->getPermissions().allowModifyBy(gAgent.getID());
	mCanTrans = can_trans && mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());

	LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
		[this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}


void LLFloaterFixedEnvironment::checkAndConfirmSettingsLoss(LLFloaterFixedEnvironment::on_confirm_fn cb)
{
	if (isDirty())
	{
		LLSD args(LLSDMap("TYPE", mSettings->getSettingsType())
			("NAME", mSettings->getName()));

		// create and show confirmation textbox
		LLNotificationsUtil::add("SettingsConfirmLoss", args, LLSD(),
			[cb](const LLSD&notif, const LLSD&resp)
		{
			S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
			if (opt == 0)
				cb();
		});
	}
	else if (cb)
	{
		cb();
	}
}

void LLFloaterFixedEnvironment::onPickerCommitSetting(LLUUID item_id)
{
	loadInventoryItem(item_id);
	//     mInventoryId = item_id;
	//     mInventoryItem = gInventory.getItem(mInventoryId);
	// 
	//     mCanCopy = mInventoryItem->getPermissions().allowCopyBy(gAgent.getID());
	//     mCanMod = mInventoryItem->getPermissions().allowModifyBy(gAgent.getID());
	//     mCanTrans = mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
	// 
	//     LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
	//         [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}

void LLFloaterFixedEnvironment::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
	if (mInventoryItem && mInventoryItem->getAssetUUID() != asset_id)
	{
		LL_WARNS("ENVIRONMENT") << "Discarding obsolete asset callback" << LL_ENDL;
		return;
	}

	clearDirtyFlag();

	if (!settings || status)
	{
		LLSD args;
		args["NAME"] = (mInventoryItem) ? mInventoryItem->getName() : asset_id.asString();
		LLNotificationsUtil::add("FailedToFindSettings", args);
		closeFloater();
		return;
	}

	if (mCanCopy)
		settings->clearFlag(LLSettingsBase::FLAG_NOCOPY);
	else
		settings->setFlag(LLSettingsBase::FLAG_NOCOPY);

	if (mCanMod)
		settings->clearFlag(LLSettingsBase::FLAG_NOMOD);
	else
		settings->setFlag(LLSettingsBase::FLAG_NOMOD);

	if (mCanTrans)
		settings->clearFlag(LLSettingsBase::FLAG_NOTRANS);
	else
		settings->setFlag(LLSettingsBase::FLAG_NOTRANS);

	mSettings = settings;
	if (mInventoryItem)
		mSettings->setName(mInventoryItem->getName());
}

void LLFloaterFixedEnvironment::onButtonImport()
{
	checkAndConfirmSettingsLoss([this]() { doImportFromDisk(); });
}

void LLFloaterFixedEnvironment::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
	std::string ctrl_action = ctrl->getName();

	std::string local_desc;
	LLSettingsBase::ptr_t setting_clone;
	bool is_local = false; // because getString can be empty
	if (mSettings->getSettingsType() == "water")
	{
		LLSettingsWater::ptr_t water = std::static_pointer_cast<LLSettingsWater>(mSettings);
		if (water)
		{
			setting_clone = water->buildClone();
			// LLViewerFetchedTexture and check for FTT_LOCAL_FILE or check LLLocalBitmapMgr
			if (LLLocalBitmapMgr::getInstance()->isLocal(water->getNormalMapID()))
			{
				local_desc = LLTrans::getString("EnvironmentNormalMap");
				is_local = true;
			}
			else if (LLLocalBitmapMgr::getInstance()->isLocal(water->getTransparentTextureID()))
			{
				local_desc = LLTrans::getString("EnvironmentTransparent");
				is_local = true;
			}
		}
	}
	else if (mSettings->getSettingsType() == "sky")
	{
		LLSettingsSky::ptr_t sky = std::static_pointer_cast<LLSettingsSky>(mSettings);
		if (sky)
		{
			setting_clone = sky->buildClone();
			if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getSunTextureId()))
			{
				local_desc = LLTrans::getString("EnvironmentSun");
				is_local = true;
			}
			else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getMoonTextureId()))
			{
				local_desc = LLTrans::getString("EnvironmentMoon");
				is_local = true;
			}
			else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getCloudNoiseTextureId()))
			{
				local_desc = LLTrans::getString("EnvironmentCloudNoise");
				is_local = true;
			}
			else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getBloomTextureId()))
			{
				local_desc = LLTrans::getString("EnvironmentBloom");
				is_local = true;
			}
		}
	}

	if (is_local)
	{
		LLSD args;
		args["FIELD"] = local_desc;
		LLNotificationsUtil::add("WLLocalTextureFixedBlock", args);
		return;
	}

	if (ctrl_action == ACTION_SAVE)
	{
		doApplyUpdateInventory(setting_clone);
	}
	else if (ctrl_action == ACTION_SAVELOCAL)
	{
		onButtonSave();
	}
	else if (ctrl_action == ACTION_SAVEAS)
	{
		LLSD args;
		args["DESC"] = mSettings->getName();
		LLNotificationsUtil::add("SaveSettingAs", args, LLSD(), boost::bind(&LLFloaterFixedEnvironment::onSaveAsCommit, this, _1, _2, setting_clone));
	}
	else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
		(ctrl_action == ACTION_APPLY_PARCEL) ||
		(ctrl_action == ACTION_APPLY_REGION))
	{
		doApplyEnvironment(ctrl_action, setting_clone);
	}
	else
	{
		LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
	}
}

void LLFloaterFixedEnvironment::onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsBase::ptr_t &settings)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		std::string settings_name = response["message"].asString();

		LLInventoryObject::correctInventoryName(settings_name);
		if (settings_name.empty())
		{
			// Ideally notification should disable 'OK' button if name won't fit our requirements,
			// for now either display notification, or use some default name
			settings_name = "Unnamed";
		}

		//BD - Since "Save As (Item)" is currently shown and enabled due to the loadInventoryItem()
		//     call always returning everything true (because we started the edit locally and don't
		//     have an inventory item) we have to cull the texture ID's here. "Save (Item)" is
		//     already included and hidden by the checks against whether we are editing an inventory
		//     item which also includes the baseline permission checks. This does not so we need to
		//     apply our own.
		LLSD Params = mSettings->getSettings();
		std::string type = mSettings->getSettingsType();
		if (type == "sky")
		{
			LLUUID moon_id = mTab->getChild<LLTextureCtrl>("moon_image")->getImageItemID();
			LLUUID cloud_id = mTab->getChild<LLTextureCtrl>("cloud_map")->getImageItemID();
			LLUUID sun_id = mTab->getChild<LLTextureCtrl>("sun_image")->getImageItemID();

			LLSD sd = mSettings->getSettings();
			if (!gDragonLibrary.checkPermissions(sun_id))
				mSettings->setLLSD("sun_id", LLUUID::null);
			if (!gDragonLibrary.checkPermissions(moon_id))
				mSettings->setLLSD("moon_id", LLUUID::null);
			if (!gDragonLibrary.checkPermissions(cloud_id))
				mSettings->setLLSD("cloud_id", LLUUID::null);
		}
		else
		{
			LLUUID normal_map = mTab->getChild<LLTextureCtrl>("water_normal_map")->getImageItemID();
			if (!gDragonLibrary.checkPermissions(normal_map))
				mSettings->setLLSD("normal_map", LLUUID::null);
		}

		if (mCanMod)
		{
			doApplyCreateNewInventory(settings_name, mSettings);
		}
		else if (mInventoryItem)
		{
			const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
			LLUUID parent_id = mInventoryItem->getParentUUID();
			if (marketplacelistings_id == parent_id)
			{
				parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
			}

			LLPointer<LLInventoryCallback> cb = new LLFixedSettingCopiedCallback(getHandle());
			copy_inventory_item(
				gAgent.getID(),
				mInventoryItem->getPermissions().getOwner(),
				mInventoryItem->getUUID(),
				parent_id,
				settings_name,
				cb);
		}
		else
		{
			LL_WARNS() << "Failed to copy fixed env setting" << LL_ENDL;
		}
	}
}

void LLFloaterFixedEnvironment::onClickCloseBtn(bool app_quitting)
{
	if (!app_quitting)
		checkAndConfirmSettingsLoss([this]() { closeFloater(); clearDirtyFlag(); });
	else
		closeFloater();
}

void LLFloaterFixedEnvironment::onButtonLoad()
{
	checkAndConfirmSettingsLoss([this]() { doSelectFromInventory(); });
}

void LLFloaterFixedEnvironment::doApplyCreateNewInventory(std::string settings_name, const LLSettingsBase::ptr_t &settings)
{
	if (mInventoryItem)
	{
		LLUUID parent_id = mInventoryItem->getParentUUID();
		U32 next_owner_perm = mInventoryItem->getPermissions().getMaskNextOwner();
		LLSettingsVOBase::createInventoryItem(settings, next_owner_perm, parent_id, settings_name,
			[this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
	}
	else
	{
		LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
		// This method knows what sort of settings object to create.
		LLSettingsVOBase::createInventoryItem(settings, parent_id, settings_name,
			[this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
	}
}

void LLFloaterFixedEnvironment::doApplyUpdateInventory(const LLSettingsBase::ptr_t &settings)
{
	// _LL_DEBUGS("ENVEDIT") << "Update inventory for " << mInventoryId << LL_ENDL;
	if (mInventoryId.isNull())
	{
		LLSettingsVOBase::createInventoryItem(settings, gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS), std::string(),
			[this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
	}
	else
	{
		LLSettingsVOBase::updateInventoryItem(settings, mInventoryId,
			[this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
	}
}

void LLFloaterFixedEnvironment::doApplyEnvironment(const std::string &where, const LLSettingsBase::ptr_t &settings)
{
	U32 flags(0);

	if (mInventoryItem)
	{
		if (!mInventoryItem->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID()))
			flags |= LLSettingsBase::FLAG_NOMOD;
		if (!mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
			flags |= LLSettingsBase::FLAG_NOTRANS;
	}

	flags |= settings->getFlags();
	settings->setFlag(flags);

	if (where == ACTION_APPLY_LOCAL)
	{
		settings->setName("Local"); // To distinguish and make sure there is a name. Safe, because this is a copy.
		LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, settings);
	}
	else if (where == ACTION_APPLY_PARCEL)
	{
		LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());

		if ((!parcel) || (parcel->getLocalID() == INVALID_PARCEL_ID))
		{
			LL_WARNS("ENVIRONMENT") << "Can not identify parcel. Not applying." << LL_ENDL;
			LLNotificationsUtil::add("WLParcelApplyFail");
			return;
		}

		if (mInventoryItem && !isDirty())
		{
			LLEnvironment::instance().updateParcel(parcel->getLocalID(), mInventoryItem->getAssetUUID(), mInventoryItem->getName(), LLEnvironment::NO_TRACK, -1, -1, flags);
		}
		else if (settings->getSettingsType() == "sky")
		{
			LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsSky>(settings), -1, -1);
		}
		else if (settings->getSettingsType() == "water")
		{
			LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsWater>(settings), -1, -1);
		}
	}
	else if (where == ACTION_APPLY_REGION)
	{
		if (mInventoryItem && !isDirty())
		{
			LLEnvironment::instance().updateRegion(mInventoryItem->getAssetUUID(), mInventoryItem->getName(), LLEnvironment::NO_TRACK, -1, -1, flags);
		}
		else if (settings->getSettingsType() == "sky")
		{
			LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsSky>(settings), -1, -1);
		}
		else if (settings->getSettingsType() == "water")
		{
			LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsWater>(settings), -1, -1);
		}
	}
	else
	{
		LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
		return;
	}

}

void LLFloaterFixedEnvironment::doCloseInventoryFloater(bool quitting)
{
	LLFloater* floaterp = mInventoryFloater.get();

	if (floaterp)
	{
		floaterp->closeFloater(quitting);
	}
}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
	LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

	if (inventory_id.isNull() || !results["success"].asBoolean())
	{
		LLNotificationsUtil::add("CantCreateInventory");
		return;
	}
	onInventoryCreated(asset_id, inventory_id);
}

void LLFloaterFixedEnvironment::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id)
{
	bool can_trans = true;
	if (mInventoryItem)
	{
		LLPermissions perms = mInventoryItem->getPermissions();

		LLInventoryItem *created_item = gInventory.getItem(mInventoryId);

		if (created_item)
		{
			can_trans = perms.allowOperationBy(PERM_TRANSFER, gAgent.getID());
			created_item->setPermissions(perms);
			created_item->updateServer(false);
		}
	}
	clearDirtyFlag();
	setFocus(TRUE);                 // Call back the focus...
	loadInventoryItem(inventory_id, can_trans);
}

void LLFloaterFixedEnvironment::onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
	LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been updated with asset " << asset_id << " results are:" << results << LL_ENDL;

	clearDirtyFlag();
	if (inventory_id != mInventoryId)
	{
		loadInventoryItem(inventory_id);
	}
}


void LLFloaterFixedEnvironment::clearDirtyFlag()
{
	mIsDirty = false;

	S32 count = mTab->getTabCount();

	for (S32 idx = 0; idx < count; ++idx)
	{
		LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
		if (panel)
			panel->clearIsDirty();
	}

}

void LLFloaterFixedEnvironment::doSelectFromInventory()
{
	LLFloaterSettingsPicker *picker = getSettingsPicker();

	picker->setSettingsFilter(mSettings->getSettingsTypeValue());
	picker->openFloater();
	picker->setFocus(TRUE);
}

void LLFloaterFixedEnvironment::onPanelDirtyFlagChanged(bool value)
{
	if (value)
		setDirtyFlag();
}

//-------------------------------------------------------------------------
bool LLFloaterFixedEnvironment::canUseInventory() const
{
	return LLEnvironment::instance().isInventoryEnabled();
}

bool LLFloaterFixedEnvironment::canApplyRegion() const
{
	return gAgent.canManageEstate();
}

bool LLFloaterFixedEnvironment::canApplyParcel() const
{
	return LLEnvironment::instance().canAgentUpdateParcelEnvironment();
}

//=========================================================================
LLFloaterFixedEnvironmentWater::LLFloaterFixedEnvironmentWater(const LLSD &key) :
	LLFloaterFixedEnvironment(key)
{}

BOOL LLFloaterFixedEnvironmentWater::postBuild()
{
	if (!LLFloaterFixedEnvironment::postBuild())
		return FALSE;

	LLPanelSettingsWater * panel;
	panel = new LLPanelSettingsWaterMainTab;
	panel->buildFromFile("panel_settings_water_settings.xml");
	panel->setWater(std::static_pointer_cast<LLSettingsWater>(mSettings));
	panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

	panel = new LLPanelSettingsWaterSecondaryTab;
	panel->buildFromFile("panel_settings_water_image.xml");
	panel->setWater(std::static_pointer_cast<LLSettingsWater>(mSettings));
	panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

	return TRUE;
}

void LLFloaterFixedEnvironmentWater::updateEditEnvironment(void)
{
	mEnvironment = mIsLocalEdit ? LLEnvironment::ENV_LOCAL : LLEnvironment::ENV_EDIT;
	if (mIsLocalEdit)
		mSettings = LLEnvironment::instance().getCurrentWater();

	LLEnvironment::instance().setEnvironment(mEnvironment, std::static_pointer_cast<LLSettingsWater>(mSettings));
	LLEnvironment::instance().setSelectedEnvironment(mEnvironment);
	//BD - This needs to stay instant otherwise changing the settings will be done with a transition
	//     potentially breaking the entire floater due to in-transition changes.
	LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterFixedEnvironmentWater::onOpen(const LLSD& key)
{
	mSettings = LLEnvironment::instance().getEnvironmentFixedWater(LLEnvironment::ENV_CURRENT)->buildClone();
	if (!mSettings)
	{
		// Initialize the settings, take a snapshot of the current water. 
		mSettings->setName("Snapshot water (new)");
		// TODO: Should we grab sky and keep it around for reference?
	}

	LLFloaterFixedEnvironment::onOpen(key);
}

void LLFloaterFixedEnvironmentWater::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false);
}

void LLFloaterFixedEnvironmentWater::loadWaterSettingFromFile(const std::vector<std::string>& filenames)
{
    LLSD messages;
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];
    //LL_DEBUGS("ENVEDIT") << "Selected file: " << filename << LL_ENDL;
    LLSettingsWater::ptr_t legacywater = LLEnvironment::createWaterFromLegacyPreset(filename, messages);

    if (!legacywater)
    {   
        LLNotificationsUtil::add("WLImportFail", messages);
        return;
    }

    loadInventoryItem(LLUUID::null);

    setDirtyFlag();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, legacywater);
    setEditSettings(legacywater);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT, true);
}

//=========================================================================
LLFloaterFixedEnvironmentSky::LLFloaterFixedEnvironmentSky(const LLSD &key) :
	LLFloaterFixedEnvironment(key)
{}

BOOL LLFloaterFixedEnvironmentSky::postBuild()
{
	if (!LLFloaterFixedEnvironment::postBuild())
		return FALSE;

	LLPanelSettingsSky * panel;
	panel = new LLPanelSettingsSkyAtmosTab;
	panel->buildFromFile("panel_settings_sky_atmos.xml");
	panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
	panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

	panel = new LLPanelSettingsSkySunMoonTab;
	panel->buildFromFile("panel_settings_sky_sunmoon.xml");
	panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
	panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

	panel = new LLPanelSettingsSkyCloudTab;
	panel->buildFromFile("panel_settings_sky_clouds.xml");
	panel->setSky(std::static_pointer_cast<LLSettingsSky>(mSettings));
	panel->setOnDirtyFlagChanged([this](LLPanel *, bool value) { onPanelDirtyFlagChanged(value); });
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(false));

	return TRUE;
}

void LLFloaterFixedEnvironmentSky::updateEditEnvironment(void)
{
	mEnvironment = mIsLocalEdit ? LLEnvironment::ENV_LOCAL : LLEnvironment::ENV_EDIT;
	if (mIsLocalEdit)
		mSettings = LLEnvironment::instance().getCurrentSky();

	LLEnvironment::instance().setEnvironment(mEnvironment, std::static_pointer_cast<LLSettingsSky>(mSettings));
	LLEnvironment::instance().setSelectedEnvironment(mEnvironment);
	//BD - This needs to stay instant otherwise changing the settings will be done with a transition
	//     potentially breaking the entire floater due to in-transition changes.
	LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterFixedEnvironmentSky::onOpen(const LLSD& key)
{
	mSettings = LLEnvironment::instance().getEnvironmentFixedSky(LLEnvironment::ENV_CURRENT)->buildClone();
	if (!mSettings)
	{
		// Initialize the settings, take a snapshot of the current water. 
		mSettings->setName("Snapshot sky (new)");
		LLEnvironment::instance().saveBeaconsState();
		// TODO: Should we grab water and keep it around for reference?
	}

	LLFloaterFixedEnvironment::onOpen(key);
}

void LLFloaterFixedEnvironmentSky::onClose(bool app_quitting)
{
	LLEnvironment::instance().revertBeaconsState();

	LLFloaterFixedEnvironment::onClose(app_quitting);
}

void LLFloaterFixedEnvironmentSky::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterFixedEnvironmentSky::loadSkySettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false);
}

void LLFloaterFixedEnvironmentSky::loadSkySettingFromFile(const std::vector<std::string>& filenames)
{
	if (filenames.size() < 1) return;
	loadPreset(filenames[0], "sky");
}

//BD - Windlight Stuff
//=====================================================================================================

void LLFloaterFixedEnvironment::onButtonSave()
{
	LLSD Params = mSettings->getSettings();
	std::string type = mSettings->getSettingsType();
	if (type == "sky")
	{
		LLUUID moon_id = mTab->getChild<LLTextureCtrl>("moon_image")->getImageItemID();
		LLUUID cloud_id = mTab->getChild<LLTextureCtrl>("cloud_map")->getImageItemID();
		LLUUID sun_id = mTab->getChild<LLTextureCtrl>("sun_image")->getImageItemID();

		LLSD sd = mSettings->getSettings();
		if (!gDragonLibrary.checkPermissions(sun_id))
			mSettings->setLLSD("sun_id", LLUUID::null);
		if (!gDragonLibrary.checkPermissions(moon_id))
			mSettings->setLLSD("moon_id", LLUUID::null);
		if (!gDragonLibrary.checkPermissions(cloud_id))
			mSettings->setLLSD("cloud_id", LLUUID::null);
	}
	else
	{
		LLUUID normal_map = mTab->getChild<LLTextureCtrl>("water_normal_map")->getImageItemID();
		if (!gDragonLibrary.checkPermissions(normal_map))
			mSettings->setLLSD("normal_map", LLUUID::null);
	}

	gDragonLibrary.savePreset(mTxtName->getValue(), mSettings);
	populatePresetsList();
}

void LLFloaterFixedEnvironment::onButtonDelete()
{
	std::string type = mSettings->getSettingsType();
	std::string folder = type == "sky" ? "skies" : "water";
	gDragonLibrary.deletePreset(mTxtName->getValue(), folder);
	populatePresetsList();
}

void LLFloaterFixedEnvironment::onSelectPreset()
{
	std::string type = mSettings->getSettingsType();
	std::string folder = type == "sky" ? "skies" : "water";

	//BD - First attempt to load it as inventory item.
	if (mTxtName->getValue().isUUID())
	{
		LLUUID uuid = mTxtName->getValue();
		LLViewerInventoryItem* item = gInventory.getItem(uuid);
		if (item)
		{
			LLSettingsVOBase::getSettingsAsset(item->getAssetUUID(), [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { loadItem(settings); });
			//BD - Assume loading was successful.
			return;
		}
	}

	//BD - Loading as inventory item failed so it must be a local preset.
	std::string name = mTxtName->getValue().asString();

	if (!loadPreset(name, type))
	{
		LLNotificationsUtil::add("BDCantLoadPreset");
		LL_WARNS("Windlight") << "Failed to load " << type << " preset from:" << name << LL_ENDL;
	}
}

void LLFloaterFixedEnvironment::loadItem(LLSettingsBase::ptr_t settings)
{
	if (!settings) return;

	setDirtyFlag();
	mSettings = settings;
	LLEnvironment &env(LLEnvironment::instance());
	std::string type = settings->getSettingsType();
	if (type == "sky")
	{
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsSky>(settings));
		setEditSettings(std::static_pointer_cast<LLSettingsSky>(settings));
	}
	else
	{
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsWater>(settings));
		setEditSettings(std::static_pointer_cast<LLSettingsWater>(settings));
	}
	env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));

	mIsLocalEdit = false;
	gSavedSettings.setString(mSettings->getSettingsType() == "sky" ? "SkyPresetName" : "WaterPresetName", mSettings->getName());

	if (type == "sky")
	{
		LLEnvironment::instance().setEnvironment(mEnvironment, std::static_pointer_cast<LLSettingsSky>(mSettings));
	}
	else
	{
		LLEnvironment::instance().setEnvironment(mEnvironment, std::static_pointer_cast<LLSettingsWater>(mSettings));
	}
	LLEnvironment::instance().setSelectedEnvironment(mEnvironment);
	LLEnvironment::instance().updateEnvironment(F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));

	updateEditEnvironment();
	refresh();
}

bool LLFloaterFixedEnvironment::loadPreset(std::string filename, std::string type)
{
	if (filename.empty()) return false;

	//BD - If we get here we are loading a local preset which we assume allows us to save.
	LLEnvironment::instance().setLocalPreset(true);

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

	LLSD messages;
	LLSettingsBase::ptr_t settings;
	if (type == "sky")
	{
		settings = !params_data.has("version") ? LLEnvironment::instance().createSkyFromLegacyPreset(filename, messages)
			: LLEnvironment::instance().createSkyFromPreset(filename, messages);
	}
	else
	{
		settings = !params_data.has("version") ? LLEnvironment::instance().createWaterFromLegacyPreset(filename, messages)
			: LLEnvironment::instance().createWaterFromPreset(filename, messages);
	}

	if (!settings)
	{
		LLNotificationsUtil::add("WLImportFail", messages);
		return false;
	}

	mSettings = settings;
	loadInventoryItem(LLUUID::null);

	mIsLocalEdit = true;

	LLEnvironment &env(LLEnvironment::instance());
	if (type == "sky")
	{
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsSky>(settings), -2);
		setEditSettings(std::static_pointer_cast<LLSettingsSky>(settings));
	}
	else
	{
		env.setEnvironment(LLEnvironment::ENV_LOCAL, std::static_pointer_cast<LLSettingsWater>(settings), -2);
		setEditSettings(std::static_pointer_cast<LLSettingsWater>(settings));
	}
	env.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
	env.updateEnvironment(F32Seconds(gSavedSettings.getF32("RenderWindlightInterpolateTime")));

	std::string control = type == "sky" ? "SkyPresetName" : "WaterPresetName";
	gSavedSettings.setString(control, filename);

	refresh();

	return true;
}