/**
 * @file llpanellogin.h
 * @brief Login username entry fields.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPANELLOGIN_H
#define LL_LLPANELLOGIN_H

#include "llpanel.h"
#include "llpointer.h"			// LLPointer<>
#include "llmediactrl.h"	// LLMediaCtrlObserver
#include <boost/scoped_ptr.hpp>

#include "llcombobox.h"
#include "llbutton.h"
#include "lllineeditor.h"
#include "llcheckboxctrl.h"

class LLLineEditor;
class LLUIImage;
class LLPanelLoginListener;
class LLSLURL;
class LLCredential;

class LLPanelLogin:	
	public LLPanel,
	public LLViewerMediaObserver
{
	LOG_CLASS(LLPanelLogin);
public:
	LLPanelLogin(const LLRect &rect,
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	//BD
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void draw();

	static void show(const LLRect &rect,
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);
	static void reshapePanel();

	static void populateFields(LLPointer<LLCredential> credential, bool remember_user, bool remember_psswrd);
	static void resetFields();
	static void getFields(LLPointer<LLCredential>& credential, bool& remember_user, bool& remember_psswrd);

	static BOOL isCredentialSet() { return sCredentialSet; }

	static BOOL areCredentialFieldsDirty();
	static void setLocation(const LLSLURL& slurl);
	static void autologinToLocation(const LLSLURL& slurl);

	static void closePanel();

	void setSiteIsAlive( bool alive );

	static void giveFocus();
	
	// inherited from LLViewerMediaObserver
	static void updateServer();  // update the combo box, change the login page to the new server, clear the combo

	/// to be called from LLStartUp::setStartSLURL
	static void onUpdateStartSLURL(const LLSLURL& new_start_slurl);

	// extract name from cred in a format apropriate for username field
	static std::string getUserName(LLPointer<LLCredential> &cred);

private:
	friend class LLPanelLoginListener;
	void addFavoritesToStartLocation();
	void onSelectServer();
	void onLocationSLURL();

	void setFields(LLPointer<LLCredential> credential);

	void onClickConnect();
	void onClickNewAccount();
	void onClickVersion();
	void onClickForgotPassword();
	void onClickSignUp();
	void onUserNameTextEnty();
	void onUserListCommit();
	void onPassKey();

	//BD
	void onClickQuit();

	static void onRememberUserCheck(void*);
    static void onRememberPasswordCheck(void*);

private:
	boost::scoped_ptr<LLPanelLoginListener> mListener;

	void updateLoginButtons();
	void populateUserList(LLPointer<LLCredential> credential);

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	BOOL            mPasswordModified;

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;
    
    static BOOL sCredentialSet;

	LLComboBox*		mUsernameCombo;
	LLLineEditor*	mPasswordEdit;
	LLComboBox*		mFavoritesCombo;
	LLComboBox*		mGridCombo;

	LLButton*		mLoginBtn;
	LLCheckBoxCtrl*	mRememberMeCheck;
	LLCheckBoxCtrl*	mRememberPassCheck;
};

#endif