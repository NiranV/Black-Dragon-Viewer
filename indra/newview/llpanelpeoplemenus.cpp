/** 
 * @file llpanelpeoplemenus.h
 * @brief Menus used by the side tray "People" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// libs
#include "llmenugl.h"
#include "lluictrlfactory.h"

#include "llpanelpeoplemenus.h"

// newview
#include "llagent.h"
#include "llagentdata.h"			// for gAgentID
#include "llavataractions.h"
#include "llcallingcard.h"			// for LLAvatarTracker
#include "lllogchat.h"
#include "llparcel.h"
#include "llviewermenu.h"			// for gMenuHolder
#include "llconversationmodel.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "roles_constants.h"

//BD - Right Click Menu
#include "llmutelist.h"

namespace LLPanelPeopleMenus
{

PeopleContextMenu gPeopleContextMenu;
NearbyPeopleContextMenu gNearbyPeopleContextMenu;

//== PeopleContextMenu ===============================================================

LLContextMenu* PeopleContextMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	LLContextMenu* menu;

	//BD - Right Click Menu
	//     Doesn't matter. These can be used for both single and multi-selections.
	registrar.add("Avatar.ResetSkeleton", boost::bind(&PeopleContextMenu::resetSkeleton, this));
	registrar.add("Avatar.ResetSkeletonAndAnimations", boost::bind(&PeopleContextMenu::resetSkeletonAndAnimations, this));
	registrar.add("Avatar.AddFriend", boost::bind(&LLAvatarActions::requestFriendshipDialog, mUUIDs));
	registrar.add("Avatar.RemoveFriend", boost::bind(&LLAvatarActions::removeFriendsDialog, mUUIDs));
	registrar.add("Avatar.Derender", boost::bind(&PeopleContextMenu::derenderAvatar, this));
	registrar.add("Avatar.OfferTeleport", boost::bind(&PeopleContextMenu::offerTeleport, this));
	//BD - Empower someone with rights or revoke them.
	registrar.add("Avatar.GrantPermissions", boost::bind(&PeopleContextMenu::grantPermissions, this, _2));
	enable_registrar.add("Avatar.CheckPermissions", boost::bind(&PeopleContextMenu::checkPermissions, this, _2));

	if ( mUUIDs.size() == 1 )
	{
		// Set up for one person selected menu

		const LLUUID& id = mUUIDs.front();
		registrar.add("Avatar.Profile",			boost::bind(&LLAvatarActions::showProfile,					id));
		registrar.add("Avatar.IM",				boost::bind(&LLAvatarActions::startIM,						id));
		registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startCall,					id));
		registrar.add("Avatar.OfferTeleport",	boost::bind(&PeopleContextMenu::offerTeleport,				this));
		registrar.add("Avatar.ZoomIn",			boost::bind(&handle_zoom_to_object,							id));
		registrar.add("Avatar.ShowOnMap",		boost::bind(&LLAvatarActions::showOnMap,					id));
		registrar.add("Avatar.Share",			boost::bind(&LLAvatarActions::share,						id));
		registrar.add("Avatar.Pay",				boost::bind(&LLAvatarActions::pay,							id));
		registrar.add("Avatar.BlockUnblock",	boost::bind(&LLAvatarActions::toggleBlock,					id));
		registrar.add("Avatar.InviteToGroup",	boost::bind(&LLAvatarActions::inviteToGroup,				id));
		registrar.add("Avatar.TeleportRequest",	boost::bind(&PeopleContextMenu::requestTeleport,			this));
		registrar.add("Avatar.Calllog",			boost::bind(&LLAvatarActions::viewChatHistory,				id));
		registrar.add("Avatar.Freeze",			boost::bind(&LLAvatarActions::freezeAvatar,					id));
		registrar.add("Avatar.Eject",			boost::bind(&PeopleContextMenu::eject,						this));

		//BD - Report Abuse
		registrar.add("Avatar.AbuseReport", boost::bind(&LLAvatarActions::report,							id));

//		//BD - SSFUI
		registrar.add("Avatar.GetUUID",			boost::bind(&LLAvatarActions::copyUUIDToClipboard,			id));
		registrar.add("Avatar.GetSLURL",		boost::bind(&LLAvatarActions::copySLURLToClipboard,			id));

		//BD - Right Click Menu
		registrar.add("Avatar.BlockUnblockText",boost::bind(&PeopleContextMenu::toggleMuteText,				this));
		registrar.add("Avatar.MuteUnmute",		boost::bind(&LLAvatarActions::toggleMuteVoice,				id));
		registrar.add("Avatar.SetImpostorMode",	boost::bind(&PeopleContextMenu::setImpostorMode,			this, _2));

		enable_registrar.add("Avatar.EnableItem", boost::bind(&PeopleContextMenu::enableContextMenuItem,	this, _2));
		enable_registrar.add("Avatar.CheckItem",  boost::bind(&PeopleContextMenu::checkContextMenuItem,		this, _2));
		enable_registrar.add("Avatar.EnableFreezeEject", boost::bind(&PeopleContextMenu::enableFreezeEject, this, _2));

		//BD - Right Click Menu
		enable_registrar.add("Avatar.CheckImpostorMode", boost::bind(&PeopleContextMenu::checkImpostorMode, this, _2));

		// create the context menu from the XUI
		menu = createFromFile("menu_people_nearby.xml");
		buildContextMenu(*menu, 0x0);
	}
	else
	{
		// Set up for multi-selected People

		registrar.add("Avatar.IM",				boost::bind(&PeopleContextMenu::startConference,		this));
		registrar.add("Avatar.Call",			boost::bind(&LLAvatarActions::startAdhocCall,			mUUIDs, LLUUID::null));
		// registrar.add("Avatar.Share",		boost::bind(&LLAvatarActions::startIM,					mUUIDs)); // *TODO: unimplemented
		// registrar.add("Avatar.Pay",			boost::bind(&LLAvatarActions::pay,						mUUIDs)); // *TODO: unimplemented

		enable_registrar.add("Avatar.EnableItem",	boost::bind(&PeopleContextMenu::enableContextMenuItem, this, _2));

		// create the context menu from the XUI
		menu = createFromFile("menu_people_nearby_multiselect.xml");
		buildContextMenu(*menu, ITEM_IN_MULTI_SELECTION);
	}

    return menu;
}

void PeopleContextMenu::buildContextMenu(class LLMenuGL& menu, U32 flags)
{
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
	
	if (flags & ITEM_IN_MULTI_SELECTION)
	{
		items.push_back(std::string("add_friends"));
		items.push_back(std::string("remove_friends"));
		items.push_back(std::string("im"));
		items.push_back(std::string("call"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("offer_teleport"));
		//BD - Would this make sense?
		//items.push_back(std::string("derender"));
		//BD - Right Click Menu
		items.push_back(std::string("reset_skeleton_separator"));
		items.push_back(std::string("reset_skeleton"));
		items.push_back(std::string("reset_skeleton_animations"));
		//BD - Empower someone with rights or revoke them.
		items.push_back(std::string("permissions"));
		items.push_back(std::string("grant_online"));
		items.push_back(std::string("grant_map"));
		items.push_back(std::string("grant_modify"));
	}
	else 
	{
		items.push_back(std::string("view_profile"));
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("request_teleport"));
		items.push_back(std::string("voice_call"));
		items.push_back(std::string("chat_history"));
		items.push_back(std::string("separator_chat_history"));
		items.push_back(std::string("add_friend"));
		items.push_back(std::string("remove_friend"));
		items.push_back(std::string("invite_to_group"));
		items.push_back(std::string("separator_invite_to_group"));
		items.push_back(std::string("map"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("block_unblock"));
		//BD - Report Abuse
		items.push_back(std::string("report"));
//		//BD - SSFUI
		items.push_back(std::string("copy_avatar_separator"));
		items.push_back(std::string("CopyUUID"));
		items.push_back(std::string("CopySLURL"));
		//BD - Right Click Menu
		items.push_back(std::string("reset_skeleton_separator"));
		items.push_back(std::string("reset_skeleton"));
		items.push_back(std::string("reset_skeleton_animations"));
		items.push_back(std::string("mute_voice"));
		items.push_back(std::string("block_text"));
		items.push_back(std::string("derender"));
		items.push_back(std::string("render_separator"));
		items.push_back(std::string("render_exceptions"));
		items.push_back(std::string("render_normal"));
		items.push_back(std::string("always_render"));
		items.push_back(std::string("dont_render"));
		items.push_back(std::string("render_avatar"));
		//BD - Empower someone with rights or revoke them.
		items.push_back(std::string("permissions"));
		items.push_back(std::string("grant_online"));
		items.push_back(std::string("grant_map"));
		items.push_back(std::string("grant_modify"));
	}

    hide_context_entries(menu, items, disabled_items);
}

bool PeopleContextMenu::enableContextMenuItem(const LLSD& userdata)
{
	if(gAgent.getID() == mUUIDs.front())
	{
		return false;
	}
	std::string item = userdata.asString();

	// Note: can_block and can_delete is used only for one person selected menu
	// so we don't need to go over all uuids.

	if (item == std::string("can_block"))
	{
		const LLUUID& id = mUUIDs.front();
		return LLAvatarActions::canBlock(id);
	}
	else if (item == std::string("can_add"))
	{
		// We can add friends if:
		// - there are selected people
		// - and there are no friends among selection yet.

		bool result = (mUUIDs.size() > 0);

		uuid_vec_t::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}

		return result;
	}
	else if (item == std::string("can_delete"))
	{
		// We can remove friends if:
		// - there are selected people
		// - and there are only friends among selection.

		bool result = (mUUIDs.size() > 0);

		uuid_vec_t::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( !LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}

		return result;
	}
	else if (item == std::string("can_call"))
	{
		return LLAvatarActions::canCall();
	}
	else if (item == std::string("can_zoom_in"))
	{
		const LLUUID& id = mUUIDs.front();

		return gObjectList.findObject(id);
	}
	else if (item == std::string("can_show_on_map"))
	{
		const LLUUID& id = mUUIDs.front();

		return (LLAvatarTracker::instance().isBuddyOnline(id) && is_agent_mappable(id))
					|| gAgent.isGodlike();
	}
	else if(item == std::string("can_offer_teleport"))
	{
		return LLAvatarActions::canOfferTeleport(mUUIDs);
	}
	else if (item == std::string("can_callog"))
	{
		return LLLogChat::isTranscriptExist(mUUIDs.front());
	}
	//BD - Report Abuse
	else if (item == std::string("can_im") || item == std::string("can_invite") ||
	         item == std::string("can_share") || item == std::string("can_pay") ||
			 item == std::string("can_report"))
	{
		return true;
	}
	return false;
}

bool PeopleContextMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& id = mUUIDs.front();

	if (item == std::string("is_blocked"))
	{
		return LLAvatarActions::isBlocked(id);
	}

	//BD - Right Click Menu
	if (item == std::string("is_muted"))
	{
		return LLAvatarActions::isMuted(id, LLMute::flagVoiceChat);
	}

	if (item == std::string("is_text_blocked"))
	{
		return LLAvatarActions::isMuted(id, LLMute::flagTextChat);
	}

	return false;
}

bool PeopleContextMenu::enableFreezeEject(const LLSD& userdata)
{
    if((gAgent.getID() == mUUIDs.front()) || (mUUIDs.size() != 1))
    {
        return false;
    }

    const LLUUID& id = mUUIDs.front();

    // Use avatar_id if available, otherwise default to right-click avatar
    LLVOAvatar* avatar = NULL;
    if (id.notNull())
    {
        LLViewerObject* object = gObjectList.findObject(id);
        if (object)
        {
            if( !object->isAvatar() )
            {
                object = NULL;
            }
            avatar = (LLVOAvatar*) object;
        }
    }
    if (!avatar) return false;

    // Gods can always freeze
    if (gAgent.isGodlike()) return true;

    // Estate owners / managers can freeze
    // Parcel owners can also freeze
    const LLVector3& pos = avatar->getPositionRegion();
    const LLVector3d& pos_global = avatar->getPositionGlobal();
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos_global)->getParcel();
    LLViewerRegion* region = avatar->getRegion();
    if (!region) return false;

    bool new_value = region->isOwnedSelf(pos);
    if (!new_value || region->isOwnedGroup(pos))
    {
        new_value = LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_ADMIN);
    }
    return new_value;
}

void PeopleContextMenu::requestTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::teleportRequest(),
	// so we have to use a wrapper.
	LLAvatarActions::teleportRequest(mUUIDs.front());
}

void PeopleContextMenu::offerTeleport()
{
	// boost::bind cannot recognize overloaded method LLAvatarActions::offerTeleport(),
	// so we have to use a wrapper.
	LLAvatarActions::offerTeleport(mUUIDs);
}

void PeopleContextMenu::eject()
{
	if((gAgent.getID() == mUUIDs.front()) || (mUUIDs.size() != 1))
	{
		return;
	}

	const LLUUID& id = mUUIDs.front();

	// Use avatar_id if available, otherwise default to right-click avatar
	LLVOAvatar* avatar = NULL;
	if (id.notNull())
	{
		LLViewerObject* object = gObjectList.findObject(id);
		if (object)
		{
			if( !object->isAvatar() )
			{
				object = NULL;
			}
			avatar = (LLVOAvatar*) object;
		}
	}
	if (!avatar) return;
	LLSD payload;
	payload["avatar_id"] = avatar->getID();
	std::string fullname = avatar->getFullname();

	const LLVector3d& pos = avatar->getPositionGlobal();
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos)->getParcel();
	LLAvatarActions::ejectAvatar(id ,LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_MANAGE_BANNED));
}

void PeopleContextMenu::startConference()
{
	uuid_vec_t uuids;
	for (uuid_vec_t::const_iterator it = mUUIDs.begin(); it != mUUIDs.end(); ++it)
	{
		if(*it != gAgentID)
		{
			uuids.push_back(*it);
		}
	}
	LLAvatarActions::startConference(uuids);
}

//BD - Right Click Menu
void PeopleContextMenu::toggleMuteText()
{
	LLAvatarActions::toggleMute(mUUIDs.front(), LLMute::flagTextChat);
}

void PeopleContextMenu::resetSkeleton()
{
	doResetSkeleton(false);
}

void PeopleContextMenu::resetSkeletonAndAnimations()
{
	doResetSkeleton(true);
}

void PeopleContextMenu::doResetSkeleton(bool reset_animations)
{
	uuid_vec_t uuids;
	for (uuid_vec_t::const_iterator it = mUUIDs.begin(); it != mUUIDs.end(); ++it)
	{
		const LLUUID& id = *it;
		LLVOAvatar* avatar = NULL;
		if (id.notNull())
		{
			LLViewerObject* object = gObjectList.findObject(id);
			if (object)
			{
				if (!object->isAvatar())
				{
					object = NULL;
				}
				avatar = (LLVOAvatar*)object;
			}
		}
		if (!avatar)
			continue;

		avatar->resetSkeleton(reset_animations);
	}
}

bool PeopleContextMenu::checkImpostorMode(const LLSD& userdata)
{
	LLVOAvatar* avatar = NULL;
	if (mUUIDs.front().notNull())
	{
		LLViewerObject* object = gObjectList.findObject(mUUIDs.front());
		if (object)
		{
			if (!object->isAvatar())
			{
				object = NULL;
			}
			avatar = (LLVOAvatar*)object;
		}
	}
	if (!avatar) return false;

	U32 mode = userdata.asInteger();
	switch (mode)
	{
	case 0:
		return (avatar->getVisualMuteSettings() == LLVOAvatar::AV_RENDER_NORMALLY);
	case 1:
		return (avatar->getVisualMuteSettings() == LLVOAvatar::AV_DO_NOT_RENDER);
	case 2:
		return (avatar->getVisualMuteSettings() == LLVOAvatar::AV_ALWAYS_RENDER);
	default:
		return false;
	}
};

bool PeopleContextMenu::setImpostorMode(const LLSD& userdata)
{
	LLVOAvatar* avatar = NULL;
	if (mUUIDs.front().notNull())
	{
		LLViewerObject* object = gObjectList.findObject(mUUIDs.front());
		if (object)
		{
			if (!object->isAvatar())
			{
				object = NULL;
			}
			avatar = (LLVOAvatar*)object;
		}
	}
	if (!avatar) return false;

	U32 mode = userdata.asInteger();
	switch (mode)
	{
	case 0:
		avatar->setVisualMuteSettings(LLVOAvatar::AV_RENDER_NORMALLY);
		break;
	case 1:
		avatar->setVisualMuteSettings(LLVOAvatar::AV_DO_NOT_RENDER);
		break;
	case 2:
		avatar->setVisualMuteSettings(LLVOAvatar::AV_ALWAYS_RENDER);
		break;
	default:
		return false;
	}

	LLVOAvatar::cullAvatarsByPixelArea();
	return true;
};

//BD - Empower someone with rights or revoke them.
void PeopleContextMenu::grantPermissions(const LLSD& userdata)
{
	if (mUUIDs.empty()) return;

	S32 power = userdata.asInteger();
	LLAvatarActions::empower(mUUIDs, power);
};

bool PeopleContextMenu::checkPermissions(const LLSD& userdata)
{
	if (mUUIDs.front().isNull()) return false;

	LLAvatarTracker& at = LLAvatarTracker::instance();
	if (!at.isBuddy(mUUIDs.front())) return false;

	const LLRelationship* relation = at.getBuddyInfo(mUUIDs.front());
	if (!relation) return false;

	S32 power = userdata.asInteger();
	S32 rights = relation->getRightsGrantedTo();
	if (rights & power) return true;

	return false;
};

//BD - Derender
void PeopleContextMenu::derenderAvatar()
{
	//BD - Allow derendering everything in selection instead of just one link or the root prim.
	for (LLUUID uuid : mUUIDs)
	{
		if (!(uuid == gAgentID))
		{
			LLViewerObject *objectp = gObjectList.findObject(uuid);
			{
				gObjectList.killObject(objectp, true);
			}
		}
	}
};

//== NearbyPeopleContextMenu ===============================================================

void NearbyPeopleContextMenu::buildContextMenu(class LLMenuGL& menu, U32 flags)
{
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
	
		if (flags & ITEM_IN_MULTI_SELECTION)
	{
		items.push_back(std::string("add_friends"));
		items.push_back(std::string("remove_friends"));
		items.push_back(std::string("im"));
		items.push_back(std::string("call"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("derender"));
		//BD - Right Click Menu
		items.push_back(std::string("reset_skeleton_separator"));
		items.push_back(std::string("reset_skeleton"));
		items.push_back(std::string("reset_skeleton_animations"));
		//BD - Empower someone with rights or revoke them.
		items.push_back(std::string("permissions"));
		items.push_back(std::string("grant_online"));
		items.push_back(std::string("grant_map"));
		items.push_back(std::string("grant_modify"));
	}
	else 
	{
		items.push_back(std::string("view_profile"));
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("request_teleport"));
		items.push_back(std::string("voice_call"));
		items.push_back(std::string("chat_history"));
		items.push_back(std::string("separator_chat_history"));
		items.push_back(std::string("add_friend"));
		items.push_back(std::string("remove_friend"));
		items.push_back(std::string("invite_to_group"));
		items.push_back(std::string("separator_invite_to_group"));
		items.push_back(std::string("zoom_in"));
		items.push_back(std::string("map"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("block_unblock"));
		items.push_back(std::string("report"));
		items.push_back(std::string("freeze"));
		items.push_back(std::string("eject"));
//		//BD - SSFUI
		items.push_back(std::string("copy_avatar_separator"));
		items.push_back(std::string("CopyUUID"));
		items.push_back(std::string("CopySLURL"));
		//BD - Right Click Menu
		items.push_back(std::string("reset_skeleton_separator"));
		items.push_back(std::string("reset_skeleton"));
		items.push_back(std::string("reset_skeleton_animations"));
		items.push_back(std::string("mute_voice"));
		items.push_back(std::string("block_text"));
		items.push_back(std::string("derender"));
		items.push_back(std::string("render_separator"));
		items.push_back(std::string("render_exceptions"));
		items.push_back(std::string("render_normal"));
		items.push_back(std::string("always_render"));
		items.push_back(std::string("dont_render"));
		items.push_back(std::string("render_avatar"));
		//BD - Empower someone with rights or revoke them.
		items.push_back(std::string("permissions"));
		items.push_back(std::string("grant_online"));
		items.push_back(std::string("grant_map"));
		items.push_back(std::string("grant_modify"));
	}

    hide_context_entries(menu, items, disabled_items);
}

} // namespace LLPanelPeopleMenus
