/** 
 * @file llavatarlistitem.cpp
 * @brief avatar list item source file
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

#include <boost/signals2.hpp>

#include "llavataractions.h"
#include "llavatarlistitem.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "lltextutil.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llavatariconctrl.h"
#include "lloutputmonitorctrl.h"
#include "lltooldraganddrop.h"
// [RLVa:KB] - Checked: RLVa-2.0.1
#include "rlvactions.h"
#include "rlvcommon.h"
// [/RLVa:KB]
//BD
#include "llpanelpeople.h"

static LLWidgetNameRegistry::StaticRegistrar sRegisterAvatarListItemParams(&typeid(LLAvatarListItem::Params), "avatar_list_item");

LLAvatarListItem::Params::Params()
	: default_style("default_style"),
	voice_call_invited_style("voice_call_invited_style"),
	voice_call_joined_style("voice_call_joined_style"),
	voice_call_left_style("voice_call_left_style"),
	online_style("online_style"),
	offline_style("offline_style"),
	//BD - Developer tracker
	developer_style("developer_style"),
	name_right_pad("name_right_pad", 0)
{};


LLAvatarListItem::LLAvatarListItem(bool not_from_ui_factory/* = true*/)
	: LLPanel(),
	LLFriendObserver(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
	//BD
	mExtraInformation(NULL),
	mIconPermissionOnline(NULL),
	mIconPermissionMap(NULL),
	mIconPermissionEditMine(NULL),
	mIconPermissionEditTheirs(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mSelectedIcon(NULL),
	mOnlineStatus(E_UNKNOWN),
// [RLVa:KB] - Checked: RLVa-1.2.0
	mRlvCheckShowNames(false),
// [/RLVa:KB]
	mShowPermissions(false),
	mShowCompleteName(false),
	mHovered(false),
	mAvatarNameCacheConnection(),
	mGreyOutUsername("")
{
	if (not_from_ui_factory)
	{
		buildFromFile("panel_avatar_list_item.xml");
	}
	// *NOTE: mantipov: do not use any member here. They can be uninitialized here in case instance
	// is created from the UICtrlFactory
}

LLAvatarListItem::~LLAvatarListItem()
{
	if (mAvatarId.notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);
	}

	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
}

BOOL  LLAvatarListItem::postBuild()
{
	mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mAvatarName = getChild<LLTextBox>("avatar_name");
	//BD
	mExtraInformation = getChild<LLTextBox>("extra_information");

	//BD - Empower someone with rights or revoke them.
	mIconPermissionOnline = getChild<LLButton>("permission_online_icon");
	mIconPermissionMap = getChild<LLButton>("permission_map_icon");
	mIconPermissionEditMine = getChild<LLButton>("permission_edit_mine_icon");
	mIconPermissionEditTheirs = getChild<LLButton>("permission_edit_theirs_icon");

	mSelectedIcon = getChild<LLIconCtrl>("selected_icon");

	mIconPermissionOnline->setCommitCallback(boost::bind(&LLAvatarListItem::empowerFriend, this, _1));
	mIconPermissionMap->setCommitCallback(boost::bind(&LLAvatarListItem::empowerFriend, this, _1));
	mIconPermissionEditMine->setCommitCallback(boost::bind(&LLAvatarListItem::empowerFriend, this, _1));

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");
	mSpeakingIndicator->setChannelState(LLOutputMonitorCtrl::UNDEFINED_CHANNEL);

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onInfoBtnClick, this));

	return TRUE;
}

void LLAvatarListItem::fetchAvatarName()
{
	if (mAvatarId.notNull())
	{
		if (mAvatarNameCacheConnection.connected())
		{
			mAvatarNameCacheConnection.disconnect();
		}
		mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLAvatarListItem::onAvatarNameCache, this, _2));
	}
}


void LLAvatarListItem::draw()
{
	//BD - Simple check to prevent it from firing it infinitely.
	if ((bool)mIconPermissionOnline->getVisible() != mShowPermissions)
	{
		showPermissions(mShowPermissions);
	}
	LLPanel::draw();
}

S32 LLAvatarListItem::notifyParent(const LLSD& info)
{
	if (info.has("visibility_changed"))
	{
		return 1;
	}
	return LLPanel::notifyParent(info);
}

void LLAvatarListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( true);

	mHovered = true;
	LLPanel::onMouseEnter(x, y, mask);
}

void LLAvatarListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( false);

	mHovered = false;
	LLPanel::onMouseLeave(x, y, mask);
}

// virtual, called by LLAvatarTracker
void LLAvatarListItem::changed(U32 mask)
{
	//BD - Developer tracker
	//     Only force my listitem as developer style if we are not friends,
	//     so we respect online status.
	//     No need to check mAvatarId for null in this case
	setOnline(LLAvatarTracker::instance().isBuddyOnline(mAvatarId), LLAvatarTracker::instance().isBuddy(mAvatarId) ? false : LLAvatarTracker::instance().isDeveloper(mAvatarId));
}

void LLAvatarListItem::setOnline(bool online, bool is_dev)
{
	// *FIX: setName() overrides font style set by setOnline(). Not an issue ATM.

	if (mOnlineStatus != E_UNKNOWN && (bool) mOnlineStatus == online)
		return;

	mOnlineStatus = (EOnlineStatus) online;

	//BD - Developer tracker
	//     Change avatar name font style depending on the new online status.
	setState(is_dev ? IS_DEVELOPER : online ? IS_ONLINE : IS_OFFLINE);
}

void LLAvatarListItem::setAvatarName(const std::string& name)
{
	setNameInternal(name, mHighlihtSubstring);
}

void LLAvatarListItem::setAvatarToolTip(const std::string& tooltip)
{
	mAvatarName->setToolTip(tooltip);
}

void LLAvatarListItem::setHighlight(const std::string& highlight)
{
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring = highlight);
}

void LLAvatarListItem::setState(EItemState item_style)
{
	const LLAvatarListItem::Params& params = LLUICtrlFactory::getDefaultParams<LLAvatarListItem>();

	switch(item_style)
	{
	default:
	case IS_DEFAULT:
		mAvatarNameStyle = params.default_style();
		break;
	case IS_VOICE_INVITED:
		mAvatarNameStyle = params.voice_call_invited_style();
		break;
	case IS_VOICE_JOINED:
		mAvatarNameStyle = params.voice_call_joined_style();
		break;
	case IS_VOICE_LEFT:
		mAvatarNameStyle = params.voice_call_left_style();
		break;
	case IS_ONLINE:
		mAvatarNameStyle = params.online_style();
		break;
	case IS_OFFLINE:
		mAvatarNameStyle = params.offline_style();
		break;
	//BD - Developer tracker
	case IS_DEVELOPER:
		mAvatarNameStyle = params.developer_style();
		break;
	}

	// *NOTE: You cannot set the style on a text box anymore, you must
	// rebuild the text.  This will cause problems if the text contains
	// hyperlinks, as their styles will be wrong.
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring);

	icon_color_map_t& item_icon_color_map = getItemIconColorMap();
	mAvatarIcon->setColor(item_icon_color_map[item_style]);
}

void LLAvatarListItem::setAvatarId(const LLUUID& id, const LLUUID& session_id, bool ignore_status_changes/* = false*/, bool is_resident/* = true*/)
{
	if (mAvatarId.notNull())
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);

	mAvatarId = id;

	// We'll be notified on avatar online status changes
	if (!ignore_status_changes && mAvatarId.notNull())
		LLAvatarTracker::instance().addParticularFriendObserver(mAvatarId, this);

	//BD - Enable the developer tag if it's me.
	if (LLAvatarTracker::instance().isDeveloper(id))
	{
		getChild<LLIconCtrl>("developer_tag")->setVisible(TRUE);
		getChild<LLTextBox>("developer_text")->setVisible(TRUE);
	}

	mSpeakingIndicator->setSpeakerId(id, session_id);

	if (is_resident)
	{
		mAvatarIcon->setValue(id);

		// Set avatar name.
		fetchAvatarName();
	}
}

//BD
void LLAvatarListItem::showExtraInformation(bool show)
{
	if (show)
		return;

	mExtraInformation->setVisible(false);
}

void LLAvatarListItem::setExtraInformation(const std::string& information)
{
	mExtraInformation->setValue(information);
}
	 	 
// This will overwrite any extra information otherwise being displayed
void LLAvatarListItem::setLastInteractionTime(U32 secs_since)
{
	setExtraInformation(formatSeconds(secs_since));
}

void LLAvatarListItem::showSpeakingIndicator(bool visible)
{
	// Already done? Then do nothing.
	if (mSpeakingIndicator->getVisible() == (BOOL)visible)
		return;
}

void LLAvatarListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mAvatarId));
}

BOOL LLAvatarListItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if(mInfoBtn->getRect().pointInRect(x, y))
	{
		onInfoBtnClick();
		return TRUE;
	}
	return LLPanel::handleDoubleClick(x, y, mask);
}

void LLAvatarListItem::setValue( const LLSD& value )
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
		mSelectedIcon->setVisible( value["selected"]);
}

const LLUUID& LLAvatarListItem::getAvatarId() const
{
	return mAvatarId;
}

std::string LLAvatarListItem::getAvatarName() const
{
	return mAvatarName->getValue();
}

std::string LLAvatarListItem::getAvatarToolTip() const
{
	return mAvatarName->getToolTip();
}

void LLAvatarListItem::updateAvatarName()
{
	fetchAvatarName();
}

//== PRIVATE SECITON ==========================================================

void LLAvatarListItem::setNameInternal(const std::string& name, const std::string& highlight)
{
    if(mShowCompleteName && highlight.empty())
    {
        LLTextUtil::textboxSetGreyedVal(mAvatarName, mAvatarNameStyle, name, mGreyOutUsername);
    }
    else
    {
        LLTextUtil::textboxSetHighlightedVal(mAvatarName, mAvatarNameStyle, name, highlight);
    }
}

void LLAvatarListItem::onAvatarNameCache(const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();

	mGreyOutUsername = "";
	std::string name_string = mShowCompleteName? av_name.getCompleteName(false) : av_name.getDisplayName();
	if(av_name.getCompleteName() != av_name.getUserName())
	{
	    mGreyOutUsername = "[ " + av_name.getUserName(true) + " ]";
	    LLStringUtil::toLower(mGreyOutUsername);
	}
//	setAvatarName(name_string);
//	setAvatarToolTip(av_name.getUserName());
// [RLVa:KB] - Checked: RLVa-1.2.2
	bool fRlvCanShowName = (!mRlvCheckShowNames) || (RlvActions::canShowName(RlvActions::SNC_DEFAULT, mAvatarId));

	setAvatarName( (fRlvCanShowName) ?  name_string : RlvStrings::getAnonym(av_name) );
	setAvatarToolTip( (fRlvCanShowName) ? av_name.getUserName() : RlvStrings::getAnonym(av_name) );
	// TODO-RLVa: bit of a hack putting this here. Maybe find a better way?
	mAvatarIcon->setDrawTooltip(fRlvCanShowName);
// [/RLVa:KB]

	//requesting the list to resort
	notifyParent(LLSD().with("sort", LLSD()));
}

// Convert given number of seconds to a string like "23 minutes", "15 hours" or "3 years",
// taking i18n into account. The format string to use is taken from the panel XML.
std::string LLAvatarListItem::formatSeconds(U32 secs)
{
	static const U32 LL_ALI_MIN		= 60;
	static const U32 LL_ALI_HOUR	= LL_ALI_MIN	* 60;
	static const U32 LL_ALI_DAY		= LL_ALI_HOUR	* 24;
	static const U32 LL_ALI_WEEK	= LL_ALI_DAY	* 7;
	static const U32 LL_ALI_MONTH	= LL_ALI_DAY	* 30;
	static const U32 LL_ALI_YEAR	= LL_ALI_DAY	* 365;

	std::string fmt; 
	U32 count = 0;

	if (secs >= LL_ALI_YEAR)
	{
		fmt = "FormatYears"; count = secs / LL_ALI_YEAR;
	}
	else if (secs >= LL_ALI_MONTH)
	{
		fmt = "FormatMonths"; count = secs / LL_ALI_MONTH;
	}
	else if (secs >= LL_ALI_WEEK)
	{
		fmt = "FormatWeeks"; count = secs / LL_ALI_WEEK;
	}
	else if (secs >= LL_ALI_DAY)
	{
		fmt = "FormatDays"; count = secs / LL_ALI_DAY;
	}
	else if (secs >= LL_ALI_HOUR)
	{
		fmt = "FormatHours"; count = secs / LL_ALI_HOUR;
	}
	else if (secs >= LL_ALI_MIN)
	{
		fmt = "FormatMinutes"; count = secs / LL_ALI_MIN;
	}
	else
	{
		fmt = "FormatSeconds"; count = secs;
	}

	LLStringUtil::format_map_t args;
	args["[COUNT]"] = llformat("%u", count);
	return getString(fmt, args);
}

// static
LLAvatarListItem::icon_color_map_t& LLAvatarListItem::getItemIconColorMap()
{
	static icon_color_map_t item_icon_color_map;
	if (!item_icon_color_map.empty()) return item_icon_color_map;

	item_icon_color_map.insert(
		std::make_pair(IS_DEFAULT,
		LLUIColorTable::instance().getColor("AvatarListItemIconDefaultColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_INVITED,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceInvitedColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_JOINED,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceJoinedColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_VOICE_LEFT,
		LLUIColorTable::instance().getColor("AvatarListItemIconVoiceLeftColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_ONLINE,
		LLUIColorTable::instance().getColor("AvatarListItemIconOnlineColor", LLColor4::white)));

	item_icon_color_map.insert(
		std::make_pair(IS_OFFLINE,
		LLUIColorTable::instance().getColor("AvatarListItemIconOfflineColor", LLColor4::white)));

	//BD - Developer tracker
	item_icon_color_map.insert(
		std::make_pair(IS_DEVELOPER,
		LLUIColorTable::instance().getColor("AvatarListItemIconOnlineColor", LLColor4::white)));

	return item_icon_color_map;
}

//BD
bool LLAvatarListItem::showPermissions(bool visible)
{
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if(relation)
	{
		mIconPermissionOnline->setImageColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) ? "White" : "White_25"));
		mIconPermissionMap->setImageColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) ? "White" : "White_25"));
		mIconPermissionEditMine->setImageColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS) ? "White" : "White_25"));
		mIconPermissionEditTheirs->setImageColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "White" : "White_25"));
		mIconPermissionOnline->setVisible(visible);
		mIconPermissionMap->setVisible(visible);
		mIconPermissionEditMine->setVisible(visible);
		mIconPermissionEditTheirs->setVisible(visible);
	}

	return NULL != relation;
}

//BD - Empower someone with rights or revoke them.
void LLAvatarListItem::empowerFriend(LLUICtrl* ctrl)
{
	std::string name = ctrl->getName();
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (name == "permission_edit_mine_icon")
	{
		mIconPermissionEditMine->setImageColor(LLUIColorTable::instance().getColor
			(!relation->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS) ? "White" : "White_25"));
		LLAvatarActions::empowerFriend(mAvatarId, 4);
	}
	else if (name == "permission_map_icon")
	{
		mIconPermissionMap->setImageColor(LLUIColorTable::instance().getColor
			(!relation->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) ? "White" : "White_25"));
		LLAvatarActions::empowerFriend(mAvatarId, 2);
	}
	else
	{
		mIconPermissionOnline->setImageColor(LLUIColorTable::instance().getColor
			(!relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) ? "White" : "White_25"));
		LLAvatarActions::empowerFriend(mAvatarId, 1);
	}
}

// EOF
