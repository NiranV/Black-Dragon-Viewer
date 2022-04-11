/**
* @file llpanelprofile.h
* @brief Profile panel
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

#ifndef LL_LLPANELPROFILE_H
#define LL_LLPANELPROFILE_H

#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llfloater.h"
#include "llpanel.h"
#include "llpanelavatar.h"
#include "llmediactrl.h"
#include "llvoiceclient.h"

// class LLPanelProfileClassifieds;
// class LLTabContainer;

// class LLPanelProfileSecondLife;
// class LLPanelProfileWeb;
// class LLPanelProfilePicks;
// class LLPanelProfileFirstLife;
// class LLPanelProfileNotes;

class LLAvatarName;
class LLButton;
class LLCheckBoxCtrl;
class LLIconCtrl;
class LLTabContainer;
class LLTextBox;
class LLTextureCtrl;
class LLMediaCtrl;
class LLGroupList;
class LLTextBase;
class LLMenuButton;
class LLLineEditor;
class LLTextEditor;
class LLPanelProfileClassifieds;
class LLPanelProfilePicks;
class LLViewerFetchedTexture;

/**
* Panel for displaying Avatar's second life related info.
*/
class LLPanelProfileSecondLife
	: public LLPanelProfileTab
	, public LLFriendObserver
	, public LLVoiceClientStatusObserver
{
public:
	LLPanelProfileSecondLife();
	/*virtual*/ ~LLPanelProfileSecondLife();

	/*virtual*/ void onOpen(const LLSD& key);

	/**
	 * Saves changes.
	 */
	void apply(LLAvatarData* data);

	/**
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	/*virtual*/ void setAvatarId(const LLUUID& avatar_id);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void resetData();

	/**
	 * Sends update data request to server.
	 */
	/*virtual*/ void updateData();

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    void setProfileImageUploading(bool loading);
    void setProfileImageUploaded(const LLUUID &image_asset_id);

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	/**
	 * Process profile related data received from server.
	 */
	virtual void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Processes group related data received from server.
	 */
	virtual void processGroupProperties(const LLAvatarGroups* avatar_groups);

	/**
	 * Fills common for Avatar profile and My Profile fields.
	 */
	virtual void fillCommonData(const LLAvatarData* avatar_data);

	/**
	 * Fills partner data.
	 */
	virtual void fillPartnerData(const LLAvatarData* avatar_data);

	/**
	 * Fills account status.
	 */
	virtual void fillAccountStatus(const LLAvatarData* avatar_data);

    void onImageLoaded(BOOL success, LLViewerFetchedTexture *imagep);
    static void onImageLoaded(BOOL success,
                              LLViewerFetchedTexture *src_vi,
                              LLImageRaw* src,
                              LLImageRaw* aux_src,
                              S32 discard_level,
                              BOOL final,
                              void* userdata);

	bool isGrantedToSeeOnlineStatus();

	/**
	 * Displays avatar's online status if possible.
	 *
	 * Requirements from EXT-3880:
	 * For friends:
	 * - Online when online and privacy settings allow to show
	 * - Offline when offline and privacy settings allow to show
	 * - Else: nothing
	 * For other avatars:
	 *	- Online when online and was not set in Preferences/"Only Friends & Groups can see when I am online"
	 *	- Else: Offline
	 */
	void updateOnlineStatus();
	void processOnlineStatus(bool online);

private:
    /*virtual*/ void updateButtons();
    void onCommitMenu(const LLSD& userdata);
    bool onEnableMenu(const LLSD& userdata);
    bool onCheckMenu(const LLSD& userdata);
	void onAvatarNameCacheSetName(const LLUUID& id, const LLAvatarName& av_name);

    void setDescriptionText(const std::string &text);
    void onSetDescriptionDirty();
    void onSaveDescriptionChanges();
    void onDiscardDescriptionChanges();

private:
	typedef std::map<std::string, LLUUID> group_map_t;
	group_map_t				mGroups;
	void					openGroupProfile();

	LLGroupList*		mGroupList;
	LLCheckBoxCtrl*		mShowInSearchCheckbox;
    LLIconCtrl*			mSecondLifePic;
	LLPanel*			mSecondLifePicLayout;
    LLTextEditor*		mDescriptionEdit;
    LLMenuButton*		mAgentActionMenuButton;
    LLButton*			mSaveDescriptionChanges;
    LLButton*			mDiscardDescriptionChanges;

	bool				mVoiceStatus;
    bool				mWaitingForImageUpload;
    std::string			mDescriptionText;
    LLUUID mImageAssetId;

	boost::signals2::connection	mAvatarNameCacheConnection;
};


/**
* Panel for displaying Avatar's web profile and home page.
*/
class LLPanelProfileWeb
	: public LLPanelProfileTab
	, public LLViewerMediaObserver
{
public:
	LLPanelProfileWeb();
	/*virtual*/ ~LLPanelProfileWeb();

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void resetData();

	/**
	 * Saves changes.
	 */
	void apply(LLAvatarData* data);

	/**
	 * Loads web profile.
	 */
	/*virtual*/ void updateData();

	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	/*virtual*/ void updateButtons();
	void onCommitLoad(LLUICtrl* ctrl);

private:
	std::string			mURLHome;
	std::string			mURLWebProfile;
	LLMediaCtrl*		mWebBrowser;

	LLFrameTimer		mPerformanceTimer;
	bool				mFirstNavigate;

	boost::signals2::connection	mAvatarNameCacheConnection;
};

/**
* Panel for displaying Avatar's first life related info.
*/
class LLPanelProfileFirstLife
	: public LLPanelProfileTab
{
public:
	LLPanelProfileFirstLife();
	/*virtual*/ ~LLPanelProfileFirstLife();

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);
    void processProperties(const LLAvatarData* avatar_data);

	void resetData();

	/**
	 * Saves changes.
	 */
	void apply(LLAvatarData* data);

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	/*virtual*/ void updateButtons();
	void onDescriptionFocusReceived();

	LLTextEditor*	mDescriptionEdit;
    LLTextureCtrl*  mPicture;
    LLButton* mChangePhoto;
    LLButton* mRemovePhoto;
    LLButton* mSaveChanges;
    LLButton* mDiscardChanges;

	bool			mIsEditing;
	std::string		mCurrentDescription;
};

/**
 * Panel for displaying Avatar's notes and modifying friend's rights.
 */
class LLPanelProfileNotes
	: public LLPanelProfileTab
	, public LLFriendObserver
{
public:
	LLPanelProfileNotes();
	/*virtual*/ ~LLPanelProfileNotes();

	virtual void setAvatarId(const LLUUID& avatar_id);

	/**
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);
    void processProperties(LLAvatarNotes* avatar_notes);

	void resetData();

	/*virtual*/ void updateData();

	/**
	 * Saves changes.
	 */
	virtual void apply();

    void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

protected:
	/**
	 * Fills rights data for friends.
	 */
	void fillRightsData();

	void rightsConfirmationCallback(const LLSD& notification, const LLSD& response);
	void confirmModifyRights(bool grant);
	void onCommitRights();
	void onCommitNotes();
	void enableCheckboxes(bool enable);

	void applyRights();

    LLCheckBoxCtrl*     mOnlineStatus;
	LLCheckBoxCtrl*     mMapRights;
	LLCheckBoxCtrl*     mEditObjectRights;
	LLTextEditor*       mNotesEditor;

    std::string			mURLWebProfile;

    boost::signals2::connection	mAvatarNameCacheConnection;
};


/**
* Container panel for the profile tabs
*/
class LLPanelProfile
    : public LLPanelProfileTab
{
public:
    LLPanelProfile();
    /*virtual*/ ~LLPanelProfile();

    /*virtual*/ BOOL postBuild();

    /*virtual*/ void updateData();

    /*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

    /*virtual*/ void onOpen(const LLSD& key);

    /**
     * Saves changes.
     */
    void apply();

    void showPick(const LLUUID& pick_id = LLUUID::null);
    bool isPickTabSelected();
    bool isNotesTabSelected();

    void updateBtnsVisibility();

    void showClassified(const LLUUID& classified_id = LLUUID::null, bool edit = false);

    LLAvatarData getAvatarData() { return mAvatarData; };

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

private:
    void onTabChange();

    LLPanelProfileSecondLife*   mPanelSecondlife;
    LLPanelProfileWeb*          mPanelWeb;
    LLPanelProfilePicks*        mPanelPicks;
    LLPanelProfileClassifieds*  mPanelClassifieds;
    LLPanelProfileFirstLife*    mPanelFirstlife;
    LLPanelProfileNotes*        mPanelNotes;
    LLTabContainer*             mTabContainer;

    // Todo: due to server taking minutes to update this needs a more long term storage
    // to reuse recently saved values if user opens floater again
    // Storage implementation depends onto how a cap will be implemented, if cap will be
    // enought to fully update LLAvatarPropertiesProcessor, then this storage can be
    // implemented there.
    LLAvatarData mAvatarData;
};

#endif //LL_LLPANELPROFILE_H
