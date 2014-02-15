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
// [RLVa:KB] - Checked: 2010-04-05 (RLVa-1.2.2a)
#include "rlvhandler.h"
// [/RLVa:KB]

static LLWidgetNameRegistry::StaticRegistrar sRegisterAvatarListItemParams(&typeid(LLAvatarListItem::Params), "avatar_list_item");

LLAvatarListItem::Params::Params()
:	default_style("default_style"),
	voice_call_invited_style("voice_call_invited_style"),
	voice_call_joined_style("voice_call_joined_style"),
	voice_call_left_style("voice_call_left_style"),
	online_style("online_style"),
	offline_style("offline_style"),
	name_right_pad("name_right_pad", 0)
{};


LLAvatarListItem::LLAvatarListItem(bool not_from_ui_factory/* = true*/)
	: LLPanel(),
	LLFriendObserver(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
	mExtraInformation(NULL),
	mIconPermissionOnline(NULL),
	mIconPermissionMap(NULL),
	mIconPermissionEditMine(NULL),
	mIconPermissionEditTheirs(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mOnlineStatus(E_UNKNOWN),
// [RLVa:KB] - Checked: 2010-04-05 (RLVa-1.2.2a) | Added: RLVa-1.2.0d
	mRlvCheckShowNames(false),
// [/RLVa:KB]
	mShowPermissions(false),
	mHovered(false),
	mAvatarNameCacheConnection()
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
	mExtraInformation = getChild<LLTextBox>("extra_information");

	mIconPermissionOnline = getChild<LLIconCtrl>("permission_online_icon");
	mIconPermissionMap = getChild<LLIconCtrl>("permission_map_icon");
	mIconPermissionEditMine = getChild<LLIconCtrl>("permission_edit_mine_icon");
	mIconPermissionEditTheirs = getChild<LLIconCtrl>("permission_edit_theirs_icon");

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");

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
	showPermissions(mShowPermissions);
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
//	mInfoBtn->setVisible(mShowInfoBtn);
//	mProfileBtn->setVisible(mShowProfileBtn);
// [RLVa:KB] - Checked: 2010-04-05 (RLVa-1.2.2a) | Added: RLVa-1.2.0d
	mInfoBtn->setVisible( (mShowInfoBtn) && ((!mRlvCheckShowNames) || (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))) );
// [/RLVa:KB]

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
	// no need to check mAvatarId for null in this case
	setOnline(LLAvatarTracker::instance().isBuddyOnline(mAvatarId));

	if (mask & LLFriendObserver::POWERS)
	{
		showPermissions(mShowPermissions && mHovered);
	}
}

void LLAvatarListItem::setOnline(bool online)
{
	if (mOnlineStatus != E_UNKNOWN && (bool) mOnlineStatus == online)
		return;

	mOnlineStatus = (EOnlineStatus) online;

	// Change avatar name font style depending on the new online status.
	setState(online ? IS_ONLINE : IS_OFFLINE);
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
	mSpeakingIndicator->setSpeakerId(id, session_id);

	// We'll be notified on avatar online status changes
	if (!ignore_status_changes && mAvatarId.notNull())
		LLAvatarTracker::instance().addParticularFriendObserver(mAvatarId, this);

	if (is_resident)
	{
		mAvatarIcon->setValue(id);

		// Set avatar name.
		fetchAvatarName();
	}
}

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
//	if(mInfoBtn->getRect().pointInRect(x, y))
// [SL:KB] - Checked: 2010-10-31 (RLVa-1.2.2a) | Added: RLVa-1.2.2a
	if ( (mInfoBtn->getVisible()) && (mInfoBtn->getEnabled()) && (mInfoBtn->getRect().pointInRect(x, y)) )
// [/SL:KB]
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
	getChildView("selected_icon")->setVisible( value["selected"]);
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
	LLTextUtil::textboxSetHighlightedVal(mAvatarName, mAvatarNameStyle, name, highlight);
}

void LLAvatarListItem::onAvatarNameCache(const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();

//	setAvatarName(av_name.getDisplayName());
//	setAvatarToolTip(av_name.getUserName());
// [RLVa:KB] - Checked: 2010-10-31 (RLVa-1.2.2a) | Modified: RLVa-1.2.2a
	bool fRlvFilter = (mRlvCheckShowNames) && (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
	setAvatarName( (!fRlvFilter) ? av_name.getDisplayName() : RlvStrings::getAnonym(av_name) );
	setAvatarToolTip( (!fRlvFilter) ? av_name.getUserName() : RlvStrings::getAnonym(av_name) );
	// TODO-RLVa: bit of a hack putting this here. Maybe find a better way?
	mAvatarIcon->setDrawTooltip(!fRlvFilter);
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

	return item_icon_color_map;
}

bool LLAvatarListItem::showPermissions(bool visible)
{
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	LLColor4 permission = LLUIColorTable::instance().getColor("White");
	if(relation && visible)
	{
		mIconPermissionOnline->setColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) ? "White" : "White_25"));
		mIconPermissionMap->setColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) ? "White" : "White_25"));
		mIconPermissionEditMine->setColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS) ? "White" : "White_25"));
		mIconPermissionEditTheirs->setColor(LLUIColorTable::instance().getColor
			(relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "White" : "White_25"));
		mIconPermissionOnline->setVisible(true);
		mIconPermissionMap->setVisible(true);
		mIconPermissionEditMine->setVisible(true);
		mIconPermissionEditTheirs->setVisible(true);
	}

	return NULL != relation;
}
// EOF
