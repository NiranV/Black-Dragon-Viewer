/**
 * @file llfloaterprofile.h
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

#ifndef LL_LLFLOATERPROFILE_H
#define LL_LLFLOATERPROFILE_H

#include "llavatarnamecache.h"
#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llmediactrl.h"
#include "llvoiceclient.h"
#include "llgrouplist.h"

class LLPanel;
class LLTabContainer;
class LLCheckBoxCtrl;
class LLMenuButton;
class LLViewerFetchedTexture;
class LLTextureCtrl;
class LLTextEditor;
class LLGroupList;
class LLTextBox;
class LLTextBase;
class LLLineEditor;
class LLFrameTimer;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLProfileDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLProfileDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<LLUUID> agent_id;
		Params()
			: agent_id("agent_id")
		{
			changeDefault(mouse_opaque, false);
			changeDefault(follows.flags, FOLLOWS_ALL);
		}
	};

	LLProfileDropTarget(const Params&);
	~LLProfileDropTarget() {}

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

	void setAgentID(const LLUUID &agent_id)     { mAgentID = agent_id; }

protected:
	LLUUID mAgentID;
};


class LLFloaterProfile 
	: public LLFloater
	, public LLAvatarPropertiesObserver
	, public LLFriendObserver
	, public LLVoiceClientStatusObserver
	, public LLViewerMediaObserver
{
    LOG_CLASS(LLFloaterProfile);
public:

	LLFloaterProfile(const LLSD& key);			
	virtual ~LLFloaterProfile();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ BOOL postBuild();

	//////////////////////////
	//BD - Profile Tabs
	//////////////////////////
	/*virtual*/ void requestData();
	void resetData();

	virtual void processProperties(void* data, EAvatarProcessorType type);
	/*virtual*/ void processProfileProperties(const LLAvatarData* avatar_data);
	/*virtual*/ void processGroupProperties(const LLAvatarGroups* avatar_groups);
	/*virtual*/ void processInterestProperties(const LLInterestsData* interests_data);
	/*virtual*/ void processClassifiedProperties(const LLAvatarClassifieds* c_info);
	/*virtual*/ void processPickProperties(const LLAvatarPicks* avatar_picks);
	void processOnlineStatus(bool online);

	/*virtual*/ void updateBtns();

	// Saves changes.
	void apply();
	void closeSaveConfirm(const LLSD& notification,	const LLSD& response);

	//void onCommitTexture();
	void onTabChange();
	void onCustomAction(LLUICtrl* ctrl, const LLSD& param);
	void onCloseBtn();

	//void onImageLoaded(BOOL success, LLViewerFetchedTexture *imagep);
	//static void onImageLoaded(BOOL success,
	//		LLViewerFetchedTexture *src_vi,
	//		LLImageRaw* src,
	//		LLImageRaw* aux_src,
	//		S32 discard_level,
	//		BOOL final,
	//		void* userdata);

	void onProfilePicChange() { mUnsavedChanges = true; }

	//////////////////////////
	//BD - Second Life Panel
	//////////////////////////
	// LLFriendObserver trigger
	virtual void changed(U32 mask);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	//////////////////////////
	//BD - Web Panel
	//////////////////////////
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	//////////////////////////
	//BD - Picks Panel
	//////////////////////////
	void showPick(const LLUUID& pick_id = LLUUID::null);

	//////////////////////////
	//BD - Classifieds Panel
	//////////////////////////
	void showClassified(const LLUUID& classified_id = LLUUID::null, bool edit = false);

private:
	//////////////////////////
	//BD - Profile Panel
	//////////////////////////

	LLPanel*					mPanelSecondlife;
	LLPanel*					mPanelWeb;
	LLPanel*					mPanelInterests;
	LLPanel*					mPanelPicks;
	LLPanel*					mPanelClassifieds;
	LLPanel*					mPanelFirstlife;
	LLPanel*					mPanelNotes;
	LLTabContainer*             mTabContainer;

	//////////////////////////
	//BD - Second Life Panel
	//////////////////////////
	void onAvatarNameCacheSetName(const LLUUID& id, const LLAvatarName& av_name);
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

	void openGroupProfile();

	void onTeleportBtn();
	void onOKBtn();
	void onCancelBtn();

	void onDescriptionFocusChange();
	void onClickSetName();

	typedef std::map<std::string, LLUUID> group_map_t;
	group_map_t				mGroups;

	std::string			mOriginalDescription;
	LLTextBox*			mStatusText;
	LLGroupList*		mGroupList;
	LLCheckBoxCtrl*		mShowInSearchCheckbox;
	LLTextureCtrl*		mSecondLifePic;
	LLPanel*			mSecondLifePicLayout;
	LLTextBase*			mSecondLifeDescriptionEdit;
	LLButton*			mTeleportButton;
	LLButton*			mShowOnMapButton;
	LLButton*			mBlockButton;
	LLButton*			mUnblockButton;
	LLUICtrl*           mNameLabel;
	LLButton*			mDisplayNameButton;
	LLButton*			mAddFriendButton;
	LLButton*			mGroupInviteButton;
	LLButton*			mPayButton;
	LLButton*			mIMButton;
	LLMenuButton*		mCopyMenuButton;
	LLPanel*			mGiveInvPanel;
	LLPanel*			mActionsPanel;

	bool				mVoiceStatus;
	bool				mUnsavedChanges;

	LLAvatarNameCache::callback_connection_t mNameCallbackConnection;

	//////////////////////////
	//BD - Web Panel
	//////////////////////////
	void onCommitLoad(LLUICtrl* ctrl);
	void onCommitWebProfile();

	std::string			mURLHome;
	std::string			mURLWebProfile;
	LLMediaCtrl*		mWebBrowser;
	LLButton*			mWebProfileButton;
	LLUICtrl*			mLoadButton;
	LLLineEditor*		mUrlEdit;

	LLFrameTimer		mPerformanceTimer;
	bool				mFirstNavigate;

	//////////////////////////
	//BD - Interests Panel
	//////////////////////////
	LLCheckBoxCtrl*	mWantChecks[8];
	LLCheckBoxCtrl*	mSkillChecks[6];
	LLLineEditor*	mWantToEditor;
	LLLineEditor*	mSkillsEditor;
	LLLineEditor*	mLanguagesEditor;

	//////////////////////////
	//BD - Picks Panel
	//////////////////////////
	void onClickNewBtn();
	void onClickDelete();
	void callbackDeletePick(const LLSD& notification, const LLSD& response);

	bool canAddNewPick();
	bool canDeletePick();

	LLTabContainer* mPicksTabContainer;
	LLUICtrl*       mNoPicksLabel;
	LLButton*       mNewButton;
	LLButton*       mDeleteButton;

	LLUUID          mPickToSelectOnLoad;

	//////////////////////////
	//BD - Classifieds Panel
	//////////////////////////
	void onClickNewClassifiedBtn();
	void onClickDeleteClassified();
	void callbackDeleteClassified(const LLSD& notification, const LLSD& response);

	bool canAddNewClassified();
	bool canDeleteClassified();

	LLTabContainer* mClassifiedsTabContainer;
	LLUICtrl*       mNoClassifiedsLabel;
	LLButton*       mNewClassifiedButton;
	LLButton*       mDeleteClassifiedButton;

	LLUUID          mClassifiedToSelectOnLoad;
	bool            mClassifiedEditOnLoad;

	//////////////////////////
	//BD - First Life Panel
	//////////////////////////
	LLTextEditor*	mDescriptionEdit;
	LLTextureCtrl*  mPicture;

	std::string		mCurrentDescription;

	//////////////////////////
	//BD - Notes Panel
	//////////////////////////
	// Fills rights data for friends.
	void processRightsProperties();

	void rightsConfirmationCallback(const LLSD& notification, const LLSD& response, S32 rights);
	void confirmModifyRights(bool grant, S32 rights);
	void onCommitRights();
	void onCommitNotes();
	void onFirstDescriptionFocusChange();

	std::string			mOriginalFirstDescription;
	LLCheckBoxCtrl*     mOnlineStatus;
	LLCheckBoxCtrl*     mMapRights;
	LLCheckBoxCtrl*     mEditObjectRights;
	LLTextEditor*       mNotesEditor;

	//////////////////////////
	//BD - Profile Tabs
	//////////////////////////
	LLUUID			mAvatarId;

	S32				mRequests;

	bool			mSelfProfile;

	bool			mPropertiesLoaded;
	bool			mWebLoaded;
	bool			mPicksLoaded;
	bool			mClassifiedsLoaded;
	bool			mGroupsLoaded;
	bool			mInterestsLoaded;
	bool			mNotesLoaded;
};

#endif // LL_LLFLOATERPROFILE_H
