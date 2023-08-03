/** 
 * @file llavatarlistitem.h
 * @brief avatar list item header file
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

#ifndef LL_LLAVATARLISTITEM_H
#define LL_LLAVATARLISTITEM_H

#include <boost/signals2.hpp>

#include "llpanel.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llstyle.h"

#include "llcallingcard.h" // for LLFriendObserver

class LLAvatarIconCtrl;
class LLOutputMonitorCtrl;
class LLAvatarName;
class LLIconCtrl;

class LLAvatarListItem : public LLPanel, public LLFriendObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLStyle::Params>	default_style,
									voice_call_invited_style,
									voice_call_joined_style,
									voice_call_left_style,
									online_style,
									offline_style,
									//BD - Developer tracker
									developer_style;

		Optional<S32>				name_right_pad;

		Params();
	};

	typedef enum e_item_state_type {
		IS_DEFAULT,
		IS_VOICE_INVITED,
		IS_VOICE_JOINED,
		IS_VOICE_LEFT,
		IS_ONLINE,
		IS_OFFLINE,
		//BD - Developer tracker
		IS_DEVELOPER,
	} EItemState;

	/**
	 * Creates an instance of LLAvatarListItem.
	 *
	 * It is not registered with LLDefaultChildRegistry. It is built via LLUICtrlFactory::buildPanel
	 * or via registered LLCallbackMap depend on passed parameter.
	 * 
	 * @param not_from_ui_factory if true instance will be build with LLUICtrlFactory::buildPanel 
	 * otherwise it should be registered via LLCallbackMap before creating.
	 */
	LLAvatarListItem(bool not_from_ui_factory = true);
	virtual ~LLAvatarListItem();

	virtual BOOL postBuild();

	//BD
	/*virtual*/ void draw();
	
	//Processes notification from speaker indicator to update children when indicator's visibility is changed.
	virtual S32	notifyParent(const LLSD& info);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void setValue(const LLSD& value);
	virtual void changed(U32 mask); // from LLFriendObserver

	void updateAvatarName(); // re-query the name cache
	void setAvatarName(const std::string& name);
	void setAvatarToolTip(const std::string& tooltip);
	void setHighlight(const std::string& highlight);
	void setState(EItemState item_style);
	void setAvatarId(const LLUUID& id, const LLUUID& session_id, bool ignore_status_changes = false, bool is_resident = true);
	void showSpeakingIndicator(bool show);
	void setShowPermissions(bool show) { mShowPermissions = show; };
	void setLastInteractionTime(U32 secs_since);
	void setShowCompleteName(bool show) { mShowCompleteName = show;};

	//BD - Developer tracker
	void setOnline(bool online, bool is_dev = false);
	//BD
	void showExtraInformation(bool show);
	void setExtraInformation(const std::string& information);
	//BD - Empower someone with rights or revoke them.
	void empowerFriend(LLUICtrl* ctrl);
	
	const LLUUID& getAvatarId() const;
	std::string getAvatarName() const;
	std::string getAvatarToolTip() const;

	void onInfoBtnClick();

	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

protected:
	
	//Contains indicator to show voice activity.  
	LLOutputMonitorCtrl* mSpeakingIndicator;

	LLAvatarIconCtrl* mAvatarIcon;

	//BD - Empower someone with rights or revoke them.
	/// Panel of the Indicator for permission to see me online.
	LLButton* mIconPermissionOnline;
	/// Panel of the Indicator for permission to see my position on the map.
	LLButton* mIconPermissionMap;
	/// Panel of the Indicator for permission to edit my objects.
	LLButton* mIconPermissionEditMine;
	/// Panel of the Indicator for permission to edit their objects.
	LLButton* mIconPermissionEditTheirs;

private:

	typedef enum e_online_status {
		E_OFFLINE,
		E_ONLINE,
		E_UNKNOWN,
		//BD - Developer tracker
		E_DEVELOPER,
	} EOnlineStatus;

	void setNameInternal(const std::string& name, const std::string& highlight);
	void onAvatarNameCache(const LLAvatarName& av_name);

	std::string formatSeconds(U32 secs);

	typedef std::map<EItemState, LLColor4> icon_color_map_t;
	static icon_color_map_t& getItemIconColorMap();

	//Update visibility of active permissions icons.
	bool showPermissions(bool visible);

	LLTextBox* mAvatarName;
	//BD
	LLTextBox* mExtraInformation;
	LLStyle::Params mAvatarNameStyle;
	
	LLButton* mInfoBtn;

	LLIconCtrl* mSelectedIcon;

	LLUUID mAvatarId;
	std::string mHighlihtSubstring; // substring to highlight
	EOnlineStatus mOnlineStatus;
	//Flag indicating that info/profile button shouldn't be shown at all.
	//Speaker indicator and avatar name coords are translated accordingly
	bool mShowInfoBtn;

	/// indicates whether to show icons representing permissions granted
	bool mShowPermissions;

	/// true when the mouse pointer is hovering over this item
	bool mHovered;

	bool mShowCompleteName;
	std::string mGreyOutUsername;

	void fetchAvatarName();
	boost::signals2::connection mAvatarNameCacheConnection;
};

#endif //LL_LLAVATARLISTITEM_H
