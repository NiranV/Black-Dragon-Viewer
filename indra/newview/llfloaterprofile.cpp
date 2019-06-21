/**
 * @file llfloaterprofile.cpp
 * @brief Avatar profile floater.
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

#include "llfloaterprofile.h"

// Common
#include "llavatarnamecache.h"
#include "llslurl.h"
#include "lldateutil.h" //ageFromDate

// UI
#include "llavatariconctrl.h"
#include "llclipboard.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llloadingindicator.h"
#include "llmenubutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "llgrouplist.h"
#include "llpanel.h"
#include "llview.h"
#include "lluictrlfactory.h"

// Newview
#include "llagent.h" //gAgent
#include "llagentpicksinfo.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llfirstuse.h"
#include "llgroupactions.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llpanelblockedlist.h"
#include "llpanelprofileclassifieds.h"
#include "llpanelprofilepicks.h"
#include "lltrans.h"
#include "lltooldraganddrop.h"
#include "llurlaction.h"
#include "llviewercontrol.h"
#include "llvoiceclient.h"
#include "llweb.h"

static const std::string PANEL_PROFILE_VIEW = "panel_profile_view";
static const std::string PANEL_SECONDLIFE = "panel_profile_secondlife";
static const std::string PANEL_WEB = "panel_profile_web";
static const std::string PANEL_INTERESTS = "panel_profile_interests";
static const std::string PANEL_PICKS = "panel_profile_picks";
static const std::string PANEL_CLASSIFIEDS = "panel_profile_classifieds";
static const std::string PANEL_FIRSTLIFE = "panel_profile_firstlife";
static const std::string PANEL_NOTES = "panel_profile_notes";

static const S32 WANT_CHECKS = 8;
static const S32 SKILL_CHECKS = 6;

//*TODO: verify this limit
const S32 MAX_AVATAR_CLASSIFIEDS = 100;

const S32 MAX_REQUESTS = 5;


//////////////////////////////////////////////////////////////////////////
// LLProfileDropTarget

LLProfileDropTarget::LLProfileDropTarget(const LLProfileDropTarget::Params& p)
	: LLView(p),
	mAgentID(p.agent_id)
{}

void LLProfileDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	LL_INFOS() << "LLProfileDropTarget::doDrop()" << LL_ENDL;
}

BOOL LLProfileDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
	EDragAndDropType cargo_type,
	void* cargo_data,
	EAcceptance* accept,
	std::string& tooltip_msg)
{
	if (getParent())
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
			cargo_type, cargo_data, accept);

		return TRUE;
	}

	return FALSE;
}

static LLDefaultChildRegistry::Register<LLProfileDropTarget> r("profile_drop_target");

//////////////////////////////////////////////////////////////////////////
// LLProfileHandler

class LLProfileHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLProfileHandler() : LLCommandHandler("profile", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (params.size() < 1) return false;
		std::string agent_name = params[0];
		LL_INFOS() << "Profile, agent_name " << agent_name << LL_ENDL;
		std::string url = getProfileURL(agent_name);
		LLWeb::loadURLInternal(url);

		return true;
	}
};
LLProfileHandler gProfileHandler;


//////////////////////////////////////////////////////////////////////////
// LLAgentHandler

class LLAgentHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLAgentHandler() : LLCommandHandler("agent", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (params.size() < 2) return false;
		LLUUID avatar_id;
		if (!avatar_id.set(params[0], FALSE))
		{
			return false;
		}

		const std::string verb = params[1].asString();
		if (verb == "about")
		{
			LLAvatarActions::showProfile(avatar_id);
			return true;
		}

		if (verb == "inspect")
		{
			LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", avatar_id));
			return true;
		}

		if (verb == "im")
		{
			LLAvatarActions::startIM(avatar_id);
			return true;
		}

		if (verb == "pay")
		{
			if (!LLUI::sSettingGroups["config"]->getBOOL("EnableAvatarPay"))
			{
				LLNotificationsUtil::add("NoAvatarPay", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
				return true;
			}

			LLAvatarActions::pay(avatar_id);
			return true;
		}

		if (verb == "offerteleport")
		{
			LLAvatarActions::offerTeleport(avatar_id);
			return true;
		}

		if (verb == "requestfriend")
		{
			uuid_vec_t uuids;
			uuids.push_back(avatar_id);
			LLAvatarActions::requestFriendshipDialog(uuids);
			return true;
		}

		if (verb == "removefriend")
		{
			LLAvatarActions::removeFriendDialog(avatar_id);
			return true;
		}

		if (verb == "mute")
		{
			if (!LLAvatarActions::isBlocked(avatar_id))
			{
				LLAvatarActions::toggleBlock(avatar_id);
			}
			return true;
		}

		if (verb == "unmute")
		{
			if (LLAvatarActions::isBlocked(avatar_id))
			{
				LLAvatarActions::toggleBlock(avatar_id);
			}
			return true;
		}

		if (verb == "block")
		{
			if (params.size() > 2)
			{
				const std::string object_name = LLURI::unescape(params[2].asString());
				LLMute mute(avatar_id, object_name, LLMute::OBJECT);
				LLMuteList::getInstance()->add(mute);
				LLPanelBlockedList::showPanelAndSelect(mute.mID);
			}
			return true;
		}

		if (verb == "unblock")
		{
			if (params.size() > 2)
			{
				const std::string object_name = params[2].asString();
				LLMute mute(avatar_id, object_name, LLMute::OBJECT);
				LLMuteList::getInstance()->remove(mute);
			}
			return true;
		}
		return false;
	}
};
LLAgentHandler gAgentHandler;

class LLPickHandler : public LLCommandHandler
{
public:

	// requires trusted browser to trigger
	LLPickHandler() : LLCommandHandler("pick", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnablePicks"))
		{
			LLNotificationsUtil::add("NoPicks", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		// handle app/classified/create urls first
		if (params.size() == 1 && params[0].asString() == "create")
		{
			LLAvatarActions::showPicks(gAgent.getID());
			return true;
		}

		// then handle the general app/pick/{UUID}/{CMD} urls
		if (params.size() < 2)
		{
			return false;
		}

		// get the ID for the pick_id
		LLUUID pick_id;
		if (!pick_id.set(params[0], FALSE))
		{
			return false;
		}

		// edit the pick in the side tray.
		// need to ask the server for more info first though...
		const std::string verb = params[1].asString();
		if (verb == "edit")
		{
			LLAvatarActions::showPick(gAgent.getID(), pick_id);
			return true;
		}
		else
		{
			LL_WARNS() << "unknown verb " << verb << LL_ENDL;
			return false;
		}
	}
};
LLPickHandler gPickHandler;

class LLClassifiedHandler : public LLCommandHandler
{
public:
	// throttle calls from untrusted browsers
	LLClassifiedHandler() : LLCommandHandler("classified", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableClassifieds"))
		{
			LLNotificationsUtil::add("NoClassifieds", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		// handle app/classified/create urls first
		if (params.size() == 1 && params[0].asString() == "create")
		{
			LLAvatarActions::showClassifieds(gAgent.getID());
			return true;
		}

		// then handle the general app/classified/{UUID}/{CMD} urls
		if (params.size() < 2)
		{
			return false;
		}

		// get the ID for the classified
		LLUUID classified_id;
		if (!classified_id.set(params[0], FALSE))
		{
			return false;
		}

		// show the classified in the side tray.
		// need to ask the server for more info first though...
		const std::string verb = params[1].asString();
		if (verb == "about")
		{
			LLAvatarActions::showClassified(gAgent.getID(), classified_id, false);
			return true;
		}
		else if (verb == "edit")
		{
			LLAvatarActions::showClassified(gAgent.getID(), classified_id, true);
			return true;
		}

		return false;
	}
};
LLClassifiedHandler gClassifiedHandler;


LLFloaterProfile::LLFloaterProfile(const LLSD& key)
	: LLFloater(key),
	mAvatarId(key["id"].asUUID()),
	mStatusText(NULL),
	mWebBrowser(NULL)
{
	mCommitCallbackRegistrar.add("Profile.Action", boost::bind(&LLFloaterProfile::onCustomAction, this, _1, _2));
}

LLFloaterProfile::~LLFloaterProfile()
{
	if (mNameCallbackConnection.connected())
	{
		mNameCallbackConnection.disconnect();
	}

	//BD - Second Life Page
	if (mAvatarId.notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);
	}

	if (LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
	}

	if (LLAvatarPropertiesProcessor::instanceExists())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
	}
}

BOOL LLFloaterProfile::postBuild()
{
	childSetAction("ok_btn", boost::bind(&LLFloaterProfile::onOKBtn, this));
	childSetAction("cancel_btn", boost::bind(&LLFloaterProfile::onCancelBtn, this));

	mTabContainer = getChild<LLTabContainer>("panel_profile_tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLFloaterProfile::onTabChange, this));
	mPanelSecondlife = findChild<LLPanel>(PANEL_SECONDLIFE);
	mPanelWeb = findChild<LLPanel>(PANEL_WEB);
	mPanelInterests = findChild<LLPanel>(PANEL_INTERESTS);
	mPanelPicks = findChild<LLPanel>(PANEL_PICKS);
	mPanelClassifieds = findChild<LLPanel>(PANEL_CLASSIFIEDS);
	mPanelFirstlife = findChild<LLPanel>(PANEL_FIRSTLIFE);
	mPanelNotes = findChild<LLPanel>(PANEL_NOTES);

	mStatusText = getChild<LLTextBox>("status");
	mGroupList = getChild<LLGroupList>("group_list");
	mShowInSearchCheckbox = getChild<LLCheckBoxCtrl>("show_in_search_checkbox");
	mSecondLifePic = getChild<LLTextureCtrl>("2nd_life_pic");
	mSecondLifePicLayout = getChild<LLPanel>("image_stack");
	mSecondLifeDescriptionEdit = getChild<LLTextBase>("sl_description_edit");
	mTeleportButton = getChild<LLButton>("teleport");
	mShowOnMapButton = getChild<LLButton>("show_on_map_btn");
	mBlockButton = getChild<LLButton>("block");
	mUnblockButton = getChild<LLButton>("unblock");
	mNameLabel = getChild<LLUICtrl>("name_label");
	mDisplayNameButton = getChild<LLButton>("set_name");
	mDisplayNameButton->setCommitCallback(boost::bind(&LLFloaterProfile::onClickSetName, this));
	mAddFriendButton = getChild<LLButton>("add_friend");
	mGroupInviteButton = getChild<LLButton>("group_invite");
	mPayButton = getChild<LLButton>("pay");
	mIMButton = getChild<LLButton>("im");
	mCopyMenuButton = getChild<LLMenuButton>("copy_btn");
	mCopyMenuButton->setMenu("menu_name_field.xml", LLMenuButton::MP_BOTTOM_RIGHT);
	mGiveInvPanel = getChild<LLPanel>("give_layout");
	mActionsPanel = getChild<LLPanel>("actions_layout");

	//BD - Interests
	mWantToEditor = getChild<LLLineEditor>("want_to_edit");
	mSkillsEditor = getChild<LLLineEditor>("skills_edit");
	mLanguagesEditor = getChild<LLLineEditor>("languages_edit");
	for (S32 i = 0; i < WANT_CHECKS; ++i)
	{
		std::string check_name = llformat("chk%d", i);
		mWantChecks[i] = getChild<LLCheckBoxCtrl>(check_name);
	}

	for (S32 i = 0; i < SKILL_CHECKS; ++i)
	{
		std::string check_name = llformat("schk%d", i);
		mSkillChecks[i] = getChild<LLCheckBoxCtrl>(check_name);
	}

	//BD - Picks
	mPicksTabContainer = getChild<LLTabContainer>("tab_picks");
	mNoPicksLabel = getChild<LLUICtrl>("picks_panel_text");
	mNewButton = getChild<LLButton>("new_btn");
	mNewButton->setCommitCallback(boost::bind(&LLFloaterProfile::onClickNewBtn, this));
	mDeleteButton = getChild<LLButton>("delete_btn");
	mDeleteButton->setCommitCallback(boost::bind(&LLFloaterProfile::onClickDelete, this));

	//BD - Classifieds
	mClassifiedsTabContainer = getChild<LLTabContainer>("tab_classifieds");
	mNoClassifiedsLabel = getChild<LLUICtrl>("classifieds_panel_text");
	mNewClassifiedButton = getChild<LLButton>("new_classified_btn");
	mNewClassifiedButton->setCommitCallback(boost::bind(&LLFloaterProfile::onClickNewClassifiedBtn, this));
	mDeleteClassifiedButton = getChild<LLButton>("delete_classified_btn");
	mDeleteClassifiedButton->setCommitCallback(boost::bind(&LLFloaterProfile::onClickDeleteClassified, this));

	//BD - Web Panel
	mWebBrowser = getChild<LLMediaCtrl>("profile_html");
	mWebBrowser->addObserver(this);
	mWebBrowser->setHomePageUrl("about:blank");

	mUrlEdit = getChild<LLLineEditor>("url_edit");
	mLoadButton = getChild<LLUICtrl>("load");
	mLoadButton->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitLoad, this, _1));
	mWebProfileButton = getChild<LLButton>("web_profile_popout_btn");
	mWebProfileButton->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitWebProfile, this));

	//BD - First Life
	mDescriptionEdit = getChild<LLTextEditor>("fl_description_edit");
	mPicture = getChild<LLTextureCtrl>("real_world_pic");

	mOnlineStatus = getChild<LLCheckBoxCtrl>("status_check");
	mOnlineStatus->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitRights, this));
	mMapRights = getChild<LLCheckBoxCtrl>("map_check");
	mMapRights->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitRights, this));
	mEditObjectRights = getChild<LLCheckBoxCtrl>("objects_check");
	mEditObjectRights->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitRights, this));
	mNotesEditor = getChild<LLTextEditor>("notes_edit");
	mNotesEditor->setCommitCallback(boost::bind(&LLFloaterProfile::onCommitNotes, this));

	LLVoiceClient::getInstance()->addObserver((LLVoiceClientStatusObserver*)this);
	LLAvatarPropertiesProcessor::getInstance()->addObserver(mAvatarId, this);
	mNameCallbackConnection = LLAvatarNameCache::get(mAvatarId, boost::bind(&LLFloaterProfile::onAvatarNameCache, this, _1, _2));

	return TRUE;
}

void LLFloaterProfile::onOpen(const LLSD& key)
{
	resetData();
	requestData();
}


void LLFloaterProfile::requestData()
{
	//BD - Okay for some unknown reason, sending a single properties request for our own avatar will
	//     cause the properties processor to hang and infinite reprocess messages that it just deleted.
	//     I don't know what causes it but it makes editing the profile impossible and spams the log.
	//     In order to prevent this as fast as possible we simply send a request for everything at once
	//     process it, close down, then reopen it to request properties last.
	mSelfProfile = (gAgentID == mAvatarId);

	//BD - We can spam this as much as we want, sure it's not ideal but the processor/message system will be
	//     blocking further attempts anyway if theres still one active.
	if (mAvatarId.notNull())
	{
		//BD - Request picks data.
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPicksRequest(mAvatarId);
		//BD - Request classified data.
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarClassifiedsRequest(mAvatarId);
		//BD - Request properties and main avatar info.
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(mAvatarId);
		//BD - Request group list.
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarGroupsRequest(mAvatarId);
		//BD - Request notes.
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarNotesRequest(mAvatarId);

		//BD - Rights checkboxes dont need requesting data, not like everything else we can simply
		//     directly find out what we need to.
		processRightsProperties();
	}
}

void LLFloaterProfile::resetData()
{
	mSelfProfile = false;

	getChild<LLUICtrl>("complete_name")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("register_date")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("acc_status_text")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("partner_text")->setValue(LLStringUtil::null);

	// Set default image and 1:1 dimensions for it
	mSecondLifePic->setValue(mSecondLifePic->getDefaultImageAssetID());
	LLRect imageRect = mSecondLifePicLayout->getRect();
	mSecondLifePicLayout->reshape(imageRect.getHeight(), imageRect.getHeight());

	mSecondLifeDescriptionEdit->setValue(LLStringUtil::null);

	mPicksTabContainer->deleteAllTabs();
	mClassifiedsTabContainer->deleteAllTabs();

	mGroups.clear();
	mGroupList->setGroups(mGroups);

	mOnlineStatus->setValue(FALSE);
	mMapRights->setValue(FALSE);
	mEditObjectRights->setValue(FALSE);

	mPropertiesLoaded = false;
	mGroupsLoaded = false;
	mInterestsLoaded = false;
	mPicksLoaded = false;
	mClassifiedsLoaded = false;
	mNotesLoaded = false;
	mRequests = 0;
}

void LLFloaterProfile::processProperties(void* data, EAvatarProcessorType type)
{
	//BD - Main avatar info.
	if (APT_PROPERTIES == type)
	{
		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if (avatar_data && mAvatarId == avatar_data->avatar_id)
		{
			processProfileProperties(avatar_data);
			mPropertiesLoaded = true;
		}
	}

	//BD - Groups
	if (APT_GROUPS == type)
	{
		LLAvatarGroups* avatar_groups = static_cast<LLAvatarGroups*>(data);
		if (avatar_groups && mAvatarId == avatar_groups->avatar_id)
		{
			processGroupProperties(avatar_groups);
			//BD - The reason we do this is because our own profile is prone to getting stuck
			//     which forces us to kill the observer to stop it. Since our own groups always
			//     fully load with just a single kick we can safely shut down our own profile observer
			//     and leave it open for other people's profiles as they wont get stuck thus not
			//     hurting anyone and killing the observer on destroy anyway.
			if (mSelfProfile)
				mGroupsLoaded = true;
		}
	}

	//BD - Interests
	if (APT_INTERESTS_INFO == type)
	{
		const LLInterestsData* interests_data = static_cast<const LLInterestsData*>(data);
		if (interests_data && mAvatarId == interests_data->avatar_id)
		{
			processInterestProperties(interests_data);
			mInterestsLoaded = true;
		}
	}

	//BD - Notes
	if (APT_NOTES == type)
	{
		LLAvatarNotes* avatar_notes = static_cast<LLAvatarNotes*>(data);
		if (avatar_notes && mAvatarId == avatar_notes->target_id)
		{
			if (avatar_notes->notes.empty())
			{
				mNotesEditor->setValue(avatar_notes->notes);
			}
			else
			{
				mNotesEditor->clear();
			}
			mNotesEditor->setEnabled(TRUE);
			mNotesLoaded = true;
		}
	}

	//BD - Picks
	if (APT_PICKS == type)
	{
		LLAvatarPicks* avatar_picks = static_cast<LLAvatarPicks*>(data);
		if (avatar_picks && mAvatarId == avatar_picks->target_id)
		{
			processPickProperties(avatar_picks);
			mPicksLoaded = true;
		}
	}

	if (APT_PICK_INFO == type)
	{
		LLPickData* pick_info = static_cast<LLPickData*>(data);
		if (pick_info && pick_info->creator_id == mAvatarId)
		{
			S32 index = mPicksTabContainer->getPanelIndexByTitle(pick_info->name);
			LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getPanelByIndex(index));
			if (pick_panel)
			{
				pick_panel->processProperties(data, APT_PICK_INFO);
				pick_panel->setLoaded();
			}
		}
	}

	//BD - We do this to check whether all picks have been fully loaded.
	if (mPicksTabContainer->getTabCount() > 0)
	{
		for (S32 index = 0; index < mPicksTabContainer->getTabCount();)
		{
			LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getPanelByIndex(index));
			if (pick_panel)
			{
				bool loaded = pick_panel->getLoaded();
				mPicksLoaded = loaded;
				//BD - Break out if one isn't loaded, this will make mPicksLoaded stay at false and tell us
				//     we are not yet done loading all picks, continue to listen.
				if (!loaded)
					break;
			}
			++index;
		}
	}


	//BD - Classifieds
	if (APT_CLASSIFIEDS == type)
	{
		LLAvatarClassifieds* c_info = static_cast<LLAvatarClassifieds*>(data);
		if (c_info && mAvatarId == c_info->target_id)
		{
			processClassifiedProperties(c_info);
			mClassifiedsLoaded = true;
		}
	}

	if (APT_CLASSIFIED_INFO == type)
	{
		LLAvatarClassifiedInfo* classified_info = static_cast<LLAvatarClassifiedInfo*>(data);
		if (classified_info && classified_info->creator_id == mAvatarId)
		{
			S32 index = mClassifiedsTabContainer->getPanelIndexByTitle(classified_info->name);
			LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mClassifiedsTabContainer->getPanelByIndex(index));
			if (classified_panel)
			{
				classified_panel->processProperties(data, APT_CLASSIFIED_INFO);
				classified_panel->setInfoLoaded(true);
			}
		}
	}

	//BD - We do this to check whether all classifieds have been fully loaded.
	if (mClassifiedsTabContainer->getTabCount() > 0)
	{
		for (S32 index = 0; index < mClassifiedsTabContainer->getTabCount();)
		{
			LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mClassifiedsTabContainer->getPanelByIndex(index));
			if (classified_panel)
			{
				bool loaded = classified_panel->getInfoLoaded();
				mClassifiedsLoaded = loaded;
				//BD - Break out if one isn't loaded, this will make mPicksLoaded stay at false and tell us
				//     we are not yet done loading all picks, continue to listen.
				if (!loaded)
					break;
			}
			++index;
		}
	}

	//BD - Kill the processor early when we're done.
	if (mPropertiesLoaded && mGroupsLoaded && mInterestsLoaded
		&& mPicksLoaded && mClassifiedsLoaded && mNotesLoaded)
	{
		if (LLAvatarPropertiesProcessor::instanceExists())
		{
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
		}
	}
	updateBtns();
}

void LLFloaterProfile::processProfileProperties(const LLAvatarData* avatar_data)
{
	//BD - Assume we always have the rights, this includes ourselves and will be changed if we don't.
	bool has_online_rights = true;
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(avatar_data->avatar_id);
	if (relation)
	{
		has_online_rights = relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS);
	}
	processOnlineStatus(avatar_data->flags & AVATAR_ONLINE && has_online_rights);

	//BD - Common data
	// Refresh avatar id in cache with new info to prevent re-requests
	// and to make sure icons in text will be up to date
	LLAvatarIconIDCache::getInstance()->add(avatar_data->avatar_id, avatar_data->image_id);
	mNameLabel->setValue(LLSLURL("agent", avatar_data->avatar_id, "inspect").getSLURLString());

	LLStringUtil::format_map_t args;
	std::string birth_date = LLTrans::getString("AvatarBirthDateFormat");
	LLStringUtil::format(birth_date, LLSD().with("datetime", (S32)avatar_data->born_on.secondsSinceEpoch()));
	args["[REG_DATE]"] = birth_date;
	args["[AGE]"] = LLDateUtil::ageFromDate(avatar_data->born_on, LLDate::now());

	std::string register_date = getString("RegisterDateFormat", args);
	getChild<LLUICtrl>("register_date")->setValue(register_date);

	mSecondLifeDescriptionEdit->setValue(avatar_data->about_text);
	mSecondLifePic->setValue(avatar_data->image_id);

	//mCurrentDescription = avatar_data->fl_about_text;
	mDescriptionEdit->setValue(avatar_data->fl_about_text);
	mPicture->setValue(avatar_data->fl_image_id);

	//BD - We don't load the image loaded callbacks because they can crash the Viewer if called
	//     after the profile has been closed.
	//Don't bother about boost level, picker will set it
	//LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(avatar_data->image_id);
	/*if (imagep->getFullHeight())
	{
		onImageLoaded(true, imagep);
	}
	else
	{
		imagep->setLoadedCallback(onImageLoaded, MAX_DISCARD_LEVEL, FALSE, FALSE, this, NULL, FALSE);
	}*/

	if (mSelfProfile)
	{
		mShowInSearchCheckbox->setValue((BOOL)(avatar_data->flags & AVATAR_ALLOW_PUBLISH));
	}


	//BD - Partner data
	LLTextBase* partner_text = getChild<LLTextBase>("partner_text");
	if (avatar_data->partner_id.notNull())
	{
		partner_text->setText(LLSLURL("agent", avatar_data->partner_id, "inspect").getSLURLString());
	}
	else
	{
		partner_text->setText(getString("no_partner_text"));
	}

	
	//BD - Account data
	LLStringUtil::format_map_t acc_args;

	acc_args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(avatar_data);
	acc_args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(avatar_data);

	std::string caption_text = getString("CaptionTextAcctInfo", acc_args);
	getChild<LLUICtrl>("acc_status_text")->setValue(caption_text);
}

void LLFloaterProfile::processOnlineStatus(bool online)
{
	// *NOTE: GRANT_ONLINE_STATUS is always set to false while changing any other status.
	// When avatar disallow me to see her online status processOfflineNotification Message is received by the viewer
	// see comments for ChangeUserRights template message. EXT-453.
	// If GRANT_ONLINE_STATUS flag is changed it will be applied when viewer restarts. EXT-3880
	//const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(mAvatarId);

	//BD - Always show online status, just default it to "offline" if someone hides it. We might want to
	//     add a "unknown" status.
	//     Show us ourselves as online because we need to be online to see our profile, obviously.
	std::string status = getString(online || mSelfProfile ? "status_online" : "status_offline");

	mStatusText->setValue(status);
	mStatusText->setColor(online ?
		LLUIColorTable::instance().getColor("StatusUserOnline") :
		LLUIColorTable::instance().getColor("StatusUserOffline"));
}

void LLFloaterProfile::processGroupProperties(const LLAvatarGroups* avatar_groups)
{
	// *NOTE dzaporozhan
	// Group properties may arrive in two callbacks, we need to save them across
	// different calls. We can't do that in textbox as textbox may change the text.

	LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
	const LLAvatarGroups::group_list_t::const_iterator it_end = avatar_groups->group_list.end();

	for (; it_end != it; ++it)
	{
		LLAvatarGroups::LLGroupData group_data = *it;
		mGroups[group_data.group_name] = group_data.group_id;
	}

	mGroupList->setGroups(mGroups);
}

void LLFloaterProfile::processInterestProperties(const LLInterestsData* interests_data)
{
	for (S32 i = 0; i < WANT_CHECKS; ++i)
	{
		if (interests_data->want_to_mask & (1 << i))
		{
			mWantChecks[i]->setValue(TRUE);
		}
		else
		{
			mWantChecks[i]->setValue(FALSE);
		}
	}

	for (S32 i = 0; i < SKILL_CHECKS; ++i)
	{
		if (interests_data->skills_mask & (1 << i))
		{
			mSkillChecks[i]->setValue(TRUE);
		}
		else
		{
			mSkillChecks[i]->setValue(FALSE);
		}
	}

	mWantToEditor->setText(interests_data->want_to_text);
	mSkillsEditor->setText(interests_data->skills_text);
	mLanguagesEditor->setText(interests_data->languages_text);
}

void LLFloaterProfile::processPickProperties(const LLAvatarPicks* avatar_picks)
{
	LLUUID selected_id = mPickToSelectOnLoad;
	if (mPickToSelectOnLoad.isNull())
	{
		if (mPicksTabContainer->getTabCount() > 0)
		{
			LLPanelProfilePick* active_pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getCurrentPanel());
			if (active_pick_panel)
			{
				selected_id = active_pick_panel->getPickId();
			}
		}
	}

	mPicksTabContainer->deleteAllTabs();

	LLAvatarPicks::picks_list_t::const_iterator it = avatar_picks->picks_list.begin();
	for (; avatar_picks->picks_list.end() != it; ++it)
	{
		LLUUID pick_id = it->first;
		std::string pick_name = it->second;

		LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();

		pick_panel->setPickId(pick_id);
		pick_panel->setPickName(pick_name);
		pick_panel->setAvatarId(mAvatarId);

		mPicksTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(pick_panel).
			select_tab(selected_id == pick_id).
			label(pick_name));

		LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(mAvatarId, pick_id);

		if (selected_id == pick_id)
		{
			mPickToSelectOnLoad = LLUUID::null;
		}
	}

	BOOL no_data = !mPicksTabContainer->getTabCount();
	mNoPicksLabel->setVisible(no_data);
	if (no_data)
	{
		if (mSelfProfile)
		{
			mNoPicksLabel->setValue(LLTrans::getString("NoPicksText"));
		}
		else
		{
			mNoPicksLabel->setValue(LLTrans::getString("NoAvatarPicksText"));
		}
	}
	else if (selected_id.isNull())
	{
		mPicksTabContainer->selectFirstTab();
	}
}

void LLFloaterProfile::processClassifiedProperties(const LLAvatarClassifieds* c_info)
{
	// do not clear classified list in case we will receive two or more data packets.
	// list has been cleared in updateData(). (fix for EXT-6436)
	LLUUID selected_id = mClassifiedToSelectOnLoad;

	LLAvatarClassifieds::classifieds_list_t::const_iterator it = c_info->classifieds_list.begin();
	for (; c_info->classifieds_list.end() != it; ++it)
	{
		LLAvatarClassifieds::classified_data c_data = *it;

		LLPanelProfileClassified* classified_panel = LLPanelProfileClassified::create();

		LLSD params;
		params["classified_creator_id"] = mAvatarId;
		params["classified_id"] = c_data.classified_id;
		params["classified_name"] = c_data.name;
		params["from_search"] = (selected_id == c_data.classified_id); //SLURL handling and stats tracking
		params["edit"] = (selected_id == c_data.classified_id) && mClassifiedEditOnLoad;
		classified_panel->onOpen(params);

		mClassifiedsTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(classified_panel).
			select_tab(selected_id == c_data.classified_id).
			label(c_data.name));

		LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(c_data.classified_id);

		if (selected_id == c_data.classified_id)
		{
			mClassifiedToSelectOnLoad = LLUUID::null;
			mClassifiedEditOnLoad = false;
		}
	}

	BOOL no_data = !mClassifiedsTabContainer->getTabCount();
	mNoClassifiedsLabel->setVisible(no_data);
	if (no_data)
	{
		if (mSelfProfile)
		{
			mNoClassifiedsLabel->setValue(LLTrans::getString("NoClassifiedsText"));
		}
		else
		{
			mNoClassifiedsLabel->setValue(LLTrans::getString("NoAvatarClassifiedsText"));
		}
	}
	else if (selected_id.isNull())
	{
		mClassifiedsTabContainer->selectFirstTab();
	}
}

void LLFloaterProfile::processRightsProperties()
{
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(mAvatarId);
	// If true - we are viewing friend's profile, enable check boxes and set values.
	if (relation)
	{
		S32 rights = relation->getRightsGrantedTo();

		mOnlineStatus->setValue(LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
		mMapRights->setValue(LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
		mEditObjectRights->setValue(LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);
	}

	mOnlineStatus->setEnabled(bool(relation));
	mMapRights->setEnabled(bool(relation));
	mEditObjectRights->setEnabled(bool(relation));
}

//BD - Disabled, the layout will remain 1:1 for now as it crashes the Viewer if we happen
//     to close the profile after the image request has been sent but the image load hasn't
//     triggered yet, calling mSecondLifePicLayout will then crash the Viewer.
//     Multiple attempts to fix this were fruitless
/*void LLFloaterProfile::onImageLoaded(BOOL success, LLViewerFetchedTexture *imagep)
{
	if (isVisible(this))
	{
		LLRect imageRect = mSecondLifePicLayout->getRect();
		if (!success || imagep->getFullWidth() == imagep->getFullHeight())
		{
			if (mSecondLifePicLayout->isInVisibleChain())
				mSecondLifePicLayout->reshape(imageRect.getHeight(), imageRect.getHeight());
		}
		else
		{
			// assume 3:4, for sake of firestorm
			if (mSecondLifePicLayout->isInVisibleChain())
				mSecondLifePicLayout->reshape(imageRect.getHeight() * 4 / 3, imageRect.getHeight());
		}
	}
}

//static
void LLFloaterProfile::onImageLoaded(BOOL success,
	LLViewerFetchedTexture *src_vi,
	LLImageRaw* src,
	LLImageRaw* aux_src,
	S32 discard_level,
	BOOL final,
	void* userdata)
{
	if (!userdata) return;

	LLFloaterProfile* profile = (LLFloaterProfile*)userdata;
	if (profile)
	{
		profile->onImageLoaded(success, src_vi);
	}
}*/

void LLFloaterProfile::onTabChange()
{
	S32 index = mTabContainer->getCurrentPanelIndex();
	if (index == 1)
	{
		//BD - Open the webprofile page.
		if (!mURLWebProfile.empty())
		{
			mWebBrowser->setVisible(TRUE);
			mPerformanceTimer.start();
			mWebBrowser->navigateTo(mURLWebProfile, HTTP_CONTENT_TEXT_HTML);
		}
	}
	else
	{
		mWebBrowser->setVisible(FALSE);
		mWebBrowser->navigateTo("about:blank");
	}
	//Something could have changed, update our buttons.
	updateBtns();
}

void LLFloaterProfile::onCommitLoad(LLUICtrl* ctrl)
{
	if (!mURLHome.empty())
	{
		LLSD::String valstr = ctrl->getValue().asString();
		if (valstr.empty())
		{
			mWebBrowser->setVisible(TRUE);
			mPerformanceTimer.start();
			mWebBrowser->navigateTo(mURLHome, HTTP_CONTENT_TEXT_HTML);
		}
		else if (valstr == "popout")
		{
			// open in viewer's browser, new window
			LLWeb::loadURLInternal(mURLHome);
		}
		else if (valstr == "external")
		{
			// open in external browser
			LLWeb::loadURLExternal(mURLHome);
		}
	}
}

void LLFloaterProfile::onCommitWebProfile()
{
	// open the web profile floater
	LLAvatarActions::showProfileWeb(mAvatarId);
}


void LLFloaterProfile::onCommitNotes()
{
	if (mNotesLoaded)
	{
		std::string notes = mNotesEditor->getValue().asString();
		LLAvatarPropertiesProcessor::getInstance()->sendNotes(mAvatarId, notes);
	}
}

void LLFloaterProfile::onCommitRights()
{
	const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(mAvatarId);

	if (!buddy_relationship)
	{
		// Lets have a warning log message instead of having a crash. EXT-4947.
		LL_WARNS("LegacyProfile") << "Trying to modify rights for non-friend avatar. Skipped." << LL_ENDL;
		return;
	}

	S32 rights = 0;

	if (mOnlineStatus->getValue().asBoolean())
	{
		rights |= LLRelationship::GRANT_ONLINE_STATUS;
	}
	if (mMapRights->getValue().asBoolean())
	{
		rights |= LLRelationship::GRANT_MAP_LOCATION;
	}
	if (mEditObjectRights->getValue().asBoolean())
	{
		rights |= LLRelationship::GRANT_MODIFY_OBJECTS;
	}

	bool allow_modify_objects = mEditObjectRights->getValue().asBoolean();

	// if modify objects checkbox clicked
	if (buddy_relationship->isRightGrantedTo(
		LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
	{
		confirmModifyRights(allow_modify_objects, rights);
	}
	// only one checkbox can trigger commit, so store the rest of rights
	else
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(
			mAvatarId, rights);
	}
}


void LLFloaterProfile::rightsConfirmationCallback(const LLSD& notification,
	const LLSD& response, S32 rights)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(mAvatarId, rights);
	}
	else
	{
		mEditObjectRights->setValue(mEditObjectRights->getValue().asBoolean() ? FALSE : TRUE);
	}
}

void LLFloaterProfile::confirmModifyRights(bool grant, S32 rights)
{
	LLSD args;
	args["NAME"] = LLSLURL("agent", mAvatarId, "completename").getSLURLString();

	if (grant)
	{
		LLNotificationsUtil::add("GrantModifyRights", args, LLSD(),
			boost::bind(&LLFloaterProfile::rightsConfirmationCallback, this,
			_1, _2, rights));
	}
	else
	{
		LLNotificationsUtil::add("RevokeModifyRights", args, LLSD(),
			boost::bind(&LLFloaterProfile::rightsConfirmationCallback, this,
			_1, _2, rights));
	}
}


void LLFloaterProfile::updateBtns()
{
	mDisplayNameButton->setVisible(mSelfProfile);
	mSecondLifePic->setEnabled(mSelfProfile);
	mSecondLifeDescriptionEdit->setEnabled(mSelfProfile);
	mPicture->setEnabled(mSelfProfile);
	mDescriptionEdit->setEnabled(mSelfProfile);

	//BD - Hide the groups list layout if the group list is empty.
	getChild<LLPanel>("group_layout")->setVisible(!mGroups.empty());
	LLRect new_rect = getChild<LLPanel>("group_stack_layout")->getRect();
	getChild<LLPanel>("group_stack_layout")->reshape(new_rect.getWidth(), !mGroups.empty() ? 83.f : 26.f);

	//BD - Hide the about section if there is no profile description and this is not our own profile.
	getChild<LLPanel>("about_layout")->setVisible(mSelfProfile || !mSecondLifeDescriptionEdit->getText().empty());

	//BD - Hide invite to group button if its our own profile.
	getChild<LLPanel>("group_invite_layout")->setVisible(!mSelfProfile);

	//BD - Hide the close/ok buttons if this isn't our own profile.
	getChild<LLPanel>("close_layout")->setVisible(mSelfProfile);

	//BD - Show or keep the action buttons hidden.
	mActionsPanel->setVisible(!mSelfProfile);
	mGiveInvPanel->setVisible(!mSelfProfile);

	//BD - Disable the whole interests tab if this is not our profile.
	mTabContainer->getPanelByIndex(2)->setEnabled(mSelfProfile);

	LLUUID av_id = mAvatarId;
	bool is_buddy_online = LLAvatarTracker::instance().isBuddyOnline(mAvatarId);

	if (LLAvatarActions::isFriend(av_id))
	{
		mTeleportButton->setEnabled(is_buddy_online);
		//Disable "Add Friend" button for friends.
		mAddFriendButton->setEnabled(false);
	}
	else
	{
		mTeleportButton->setEnabled(is_buddy_online);
		mAddFriendButton->setEnabled(true);
	}

	mShowOnMapButton->setEnabled((is_buddy_online && is_agent_mappable(av_id)) || gAgent.isGodlike());
	bool is_blocked = LLAvatarActions::isBlocked(av_id);
	mBlockButton->setVisible(LLAvatarActions::canBlock(av_id) && !is_blocked);
	mUnblockButton->setVisible(is_blocked);

	//BD - Web
	mUrlEdit->setEnabled(mSelfProfile);

	//BD - Picks
	mNewButton->setVisible(mSelfProfile);
	mNewButton->setVisible(canAddNewPick());
	mDeleteButton->setVisible(mSelfProfile);
	mDeleteButton->setVisible(canDeletePick());

	//BD - Classifieds
	mNewClassifiedButton->setVisible(mSelfProfile);
	mNewClassifiedButton->setEnabled(canAddNewClassified());
	mDeleteClassifiedButton->setVisible(mSelfProfile);
	mDeleteClassifiedButton->setEnabled(canDeleteClassified());

	//BD - Hide the rights checkboxes and show the search checkboxes if this is our own profile.
	getChild<LLPanel>("rights_layout")->setVisible(!mSelfProfile);
	getChild<LLPanel>("search_layout")->setVisible(mSelfProfile);
}

void LLFloaterProfile::onClickSetName()
{
	LLAvatarNameCache::get(mAvatarId, boost::bind(&LLFloaterProfile::onAvatarNameCacheSetName, this, _1, _2));

	LLFirstUse::setDisplayName(false);
}

/*void LLFloaterProfile::onCommitTexture()
{
	LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(mSecondLifePic->getImageAssetID());
	if (imagep->getFullHeight())
	{
		onImageLoaded(true, imagep);
	}
	else
	{
		imagep->setLoadedCallback(onImageLoaded,
			MAX_DISCARD_LEVEL,
			FALSE,
			FALSE,
			this,
			NULL,
			FALSE);
	}
}*/

void LLFloaterProfile::onCustomAction(LLUICtrl* ctrl, const LLSD& param)
{
	if (param.asString() == "add_friend")
	{
		std::vector<LLUUID> ids;
		ids.push_back(mAvatarId);
		LLAvatarActions::requestFriendshipDialog(ids);
	}
	else if (param.asString() == "im")
	{
		LLAvatarActions::startIM(mAvatarId);
	}
	else if (param.asString() == "teleport")
	{
		LLAvatarActions::offerTeleport(mAvatarId);
	}
	else if (param.asString() == "show_on_map")
	{
		LLAvatarActions::showOnMap(mAvatarId);
	}
	else if (param.asString() == "pay")
	{
		LLAvatarActions::pay(mAvatarId);
	}
	else if (param.asString() == "block")
	{
		LLAvatarActions::toggleBlock(mAvatarId);
	}
	else if (param.asString() == "group_invite")
	{
		LLAvatarActions::inviteToGroup(mAvatarId);
	}
	else if (param.asString() == "show_group")
	{
		LLGroupActions::show(mGroupList->getSelectedUUID());
	}
	else if (param.asString() == "copy_display_name")
	{
		LLAvatarName av_name;
		if (!LLAvatarNameCache::get(mAvatarId, &av_name))
			return;

		LLUrlAction::copyURLToClipboard(av_name.getDisplayName(true));
	}
	else if (param.asString() == "copy_user_name")
	{
		LLAvatarName av_name;
		if (!LLAvatarNameCache::get(mAvatarId, &av_name))
			return;

		LLUrlAction::copyURLToClipboard(av_name.getAccountName());
	}
	else if (param.asString() == "copy_uuid")
	{
		LLAvatarActions::copyUUIDToClipboard(mAvatarId);
	}
	else if (param.asString() == "copy_slurl")
	{
		LLAvatarActions::copySLURLToClipboard(mAvatarId);
	}
}

void LLFloaterProfile::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	mNameCallbackConnection.disconnect();
	setTitle("Profile - " + av_name.getCompleteName());

	mCopyMenuButton->setVisible(TRUE);

	//BD - Web
	std::string username = av_name.getAccountName();
	if (username.empty())
	{
		username = LLCacheName::buildUsername(av_name.getDisplayName());
	}
	else
	{
		LLStringUtil::replaceChar(username, ' ', '.');
	}

	mURLWebProfile = getProfileURL(username);
}

void LLFloaterProfile::onAvatarNameCacheSetName(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	if (av_name.getDisplayName().empty())
	{
		// something is wrong, tell user to try again later
		LLNotificationsUtil::add("SetDisplayNameFailedGeneric");
		return;
	}

	LL_INFOS("LegacyProfile") << "name-change now " << LLDate::now() << " next_update "
		<< LLDate(av_name.mNextUpdate) << LL_ENDL;
	F64 now_secs = LLDate::now().secondsSinceEpoch();

	if (now_secs < av_name.mNextUpdate)
	{
		// if the update time is more than a year in the future, it means updates have been blocked
		// show a more general message
		static const S32 YEAR = 60 * 60 * 24 * 365;
		if (now_secs + YEAR < av_name.mNextUpdate)
		{
			LLNotificationsUtil::add("SetDisplayNameBlocked");
			return;
		}
	}

	LLFloaterReg::showInstance("display_name");
}

void LLFloaterProfile::onOKBtn()
{
	apply();
	closeFloater();
}

void LLFloaterProfile::onCancelBtn()
{
	closeFloater();
}

void LLFloaterProfile::onTeleportBtn()
{
	LLAvatarActions::offerTeleport(mAvatarId);
}


void LLFloaterProfile::onClickNewClassifiedBtn()
{
	mNoClassifiedsLabel->setVisible(FALSE);
	LLPanelProfileClassified* classified_panel = LLPanelProfileClassified::create();
	classified_panel->onOpen(LLSD());
	mClassifiedsTabContainer->addTabPanel(
		LLTabContainer::TabPanelParams().
		panel(classified_panel).
		select_tab(true).
		label(classified_panel->getClassifiedName()));
}

void LLFloaterProfile::onClickDeleteClassified()
{
	LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mClassifiedsTabContainer->getCurrentPanel());
	if (classified_panel)
	{
		LLUUID classified_id = classified_panel->getClassifiedId();
		LLSD args;
		args["PICK"] = classified_panel->getClassifiedName();
		LLSD payload;
		payload["classified_id"] = classified_id;
		payload["tab_idx"] = mClassifiedsTabContainer->getCurrentPanelIndex();
		LLNotificationsUtil::add("DeleteAvatarPick", args, payload,
			boost::bind(&LLFloaterProfile::callbackDeleteClassified, this, _1, _2));
	}
}

void LLFloaterProfile::callbackDeleteClassified(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		LLUUID classified_id = notification["payload"]["classified_id"].asUUID();
		S32 tab_idx = notification["payload"]["tab_idx"].asInteger();

		LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mClassifiedsTabContainer->getPanelByIndex(tab_idx));
		if (classified_panel && classified_panel->getClassifiedId() == classified_id)
		{
			mClassifiedsTabContainer->removeTabPanel(classified_panel);
		}

		if (classified_id.notNull())
		{
			LLAvatarPropertiesProcessor::getInstance()->sendClassifiedDelete(classified_id);
		}
	}
}

bool LLFloaterProfile::canAddNewClassified()
{
	return (mClassifiedsTabContainer->getTabCount() < MAX_AVATAR_CLASSIFIEDS);
}

bool LLFloaterProfile::canDeleteClassified()
{
	return (mClassifiedsTabContainer->getTabCount() > 0);
}


void LLFloaterProfile::onClickNewBtn()
{
	mNoClassifiedsLabel->setVisible(FALSE);
	LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();
	pick_panel->setAvatarId(mAvatarId);
	mPicksTabContainer->addTabPanel(
		LLTabContainer::TabPanelParams().
		panel(pick_panel).
		select_tab(true).
		label(pick_panel->getPickName()));
}

void LLFloaterProfile::onClickDelete()
{
	LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getCurrentPanel());
	if (pick_panel)
	{
		LLUUID pick_id = pick_panel->getPickId();
		LLSD args;
		args["PICK"] = pick_panel->getPickName();
		LLSD payload;
		payload["pick_id"] = pick_id;
		payload["tab_idx"] = mPicksTabContainer->getCurrentPanelIndex();
		LLNotificationsUtil::add("DeleteAvatarPick", args, payload,
			boost::bind(&LLFloaterProfile::callbackDeletePick, this, _1, _2));
	}
}

void LLFloaterProfile::callbackDeletePick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		LLUUID pick_id = notification["payload"]["pick_id"].asUUID();
		S32 tab_idx = notification["payload"]["tab_idx"].asInteger();

		LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getPanelByIndex(tab_idx));
		if (pick_panel && pick_panel->getPickId() == pick_id)
		{
			mPicksTabContainer->removeTabPanel(pick_panel);
		}

		if (pick_id.notNull())
		{
			LLAvatarPropertiesProcessor::getInstance()->sendPickDelete(pick_id);
		}
	}
}

bool LLFloaterProfile::canAddNewPick()
{
	return (!LLAgentPicksInfo::getInstance()->isPickLimitReached() &&
		mPicksTabContainer->getTabCount() < LLAgentPicksInfo::getInstance()->getMaxNumberOfPicks());
}

bool LLFloaterProfile::canDeletePick()
{
	return (mPicksTabContainer->getTabCount() > 0);
}


void LLFloaterProfile::apply()
{
	if (mSelfProfile)
	{
		if (mPropertiesLoaded)
		{
			LLAvatarData data = LLAvatarData();
			data.avatar_id = gAgentID;

			//BD - Second Life
			data.image_id = mSecondLifePic->getImageAssetID();
			data.about_text = mSecondLifeDescriptionEdit->getValue().asString();
			data.allow_publish = mShowInSearchCheckbox->getValue();

			//BD - Web
			if (mWebLoaded)
			{
				data.profile_url = mUrlEdit->getValue().asString();
			}

			//BD - First Life
			data.fl_image_id = mPicture->getImageAssetID();
			data.fl_about_text = mDescriptionEdit->getValue().asString();

			LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate(&data);
		}

		if (mInterestsLoaded)
		{
			//BD - Interests
			LLInterestsData interests_data = LLInterestsData();

			interests_data.want_to_mask = 0;
			for (S32 i = 0; i < WANT_CHECKS; ++i)
			{
				if (mWantChecks[i]->getValue().asBoolean())
				{
					interests_data.want_to_mask |= (1 << i);
				}
			}

			interests_data.skills_mask = 0;
			for (S32 i = 0; i < SKILL_CHECKS; ++i)
			{
				if (mSkillChecks[i]->getValue().asBoolean())
				{
					interests_data.skills_mask |= (1 << i);
				}
			}

			interests_data.want_to_text = mWantToEditor->getText();
			interests_data.skills_text = mSkillsEditor->getText();
			interests_data.languages_text = mLanguagesEditor->getText();

			LLAvatarPropertiesProcessor::getInstance()->sendInterestsInfoUpdate(&interests_data);
		}

		if (mPicksLoaded)
		{
			//BD - Picks
			for (S32 tab_idx = 0; tab_idx < mPicksTabContainer->getTabCount(); ++tab_idx)
			{
				LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mPicksTabContainer->getPanelByIndex(tab_idx));
				if (pick_panel)
				{
					pick_panel->apply();
				}
			}
		}
	}

	//BD - Notes
	onCommitNotes();
}


// virtual, called by LLAvatarTracker
void LLFloaterProfile::changed(U32 mask)
{
	if (!LLAvatarActions::isFriend(mAvatarId)) return;
	// For friend let check if he allowed me to see his status
	const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(mAvatarId);
	bool online = relationship->isOnline();
	processOnlineStatus(online);

	updateBtns();
}

// virtual, called by LLVoiceClient
void LLFloaterProfile::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if (status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	mVoiceStatus = LLAvatarActions::canCall() && (LLAvatarActions::isFriend(mAvatarId) ? LLAvatarTracker::instance().isBuddyOnline(mAvatarId) : TRUE);
}

void LLFloaterProfile::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch (event)
	{
	case MEDIA_EVENT_STATUS_TEXT_CHANGED:
		childSetValue("status_text", LLSD(self->getStatusText()));
		break;

	case MEDIA_EVENT_LOCATION_CHANGED:
		// don't set this or user will set there url to profile url
		// when clicking ok on there own profile.
		// childSetText("url_edit", self->getLocation() );
		break;

	case MEDIA_EVENT_NAVIGATE_BEGIN:
	{
		if (mFirstNavigate)
		{
			mFirstNavigate = false;
		}
		else
		{
			mPerformanceTimer.start();
		}
	}
	break;

	case MEDIA_EVENT_NAVIGATE_COMPLETE:
	{
		LLStringUtil::format_map_t args;
		args["[TIME]"] = llformat("%.2f", mPerformanceTimer.getElapsedTimeF32());
		childSetValue("status_text", LLSD(getString("LoadTime", args)));
	}
	break;

	default:
		// Having a default case makes the compiler happy.
		break;
	}
}