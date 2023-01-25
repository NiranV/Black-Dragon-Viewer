/** 
 * @file llpanellogin.cpp
 * @brief Login dialog and logo display
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

#include "llviewerprecompiledheaders.h"

#include "llpanellogin.h"
#include "lllayoutstack.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llmd5.h"
#include "v4color.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcommandhandler.h"		// for secondlife:///app/login/
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llsecapi.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llslurl.h"
#include "llversioninfo.h"
#include "llviewerhelp.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"			// for handle_preferences()
#include "llviewernetwork.h"
#include "llviewerwindow.h"			// to link into child list
#include "lluictrlfactory.h"
#include "llweb.h"
#include "llmediactrl.h"
#include "llrootview.h"

#include "llfloatertos.h"
#include "lltrans.h"
#include "llglheaders.h"
#include "llpanelloginlistener.h"


#if LL_WINDOWS
#pragma warning(disable: 4355)      // 'this' used in initializer list
#endif  // LL_WINDOWS

#include "llsdserialize.h"

LLPanelLogin *LLPanelLogin::sInstance = NULL;
BOOL LLPanelLogin::sCapslockDidNotification = FALSE;
BOOL LLPanelLogin::sCredentialSet = FALSE;

// Helper functions

LLPointer<LLCredential> load_user_credentials(std::string &user_key)
{
    if (gSecAPIHandler->hasCredentialMap("login_list", LLGridManager::getInstance()->getGrid()))
    {
        // user_key should be of "name Resident" format
        return gSecAPIHandler->loadFromCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), user_key);
    }
    else
    {
        // legacy (or legacy^2, since it also tries to load from settings)
        return gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
    }
}

class LLLoginLocationAutoHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLLoginLocationAutoHandler() : LLCommandHandler("location_login", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)
		{
			if (tokens.size() == 0 || tokens.size() > 4)
				return false;

			// unescape is important - uris with spaces are escaped in this code path
			// (e.g. space -> %20) and the code to log into a region doesn't support that.
			const std::string region = LLURI::unescape(tokens[0].asString());

			// just region name as payload 
			if (tokens.size() == 1)
			{
				// region name only - slurl will end up as center of region
				LLSLURL slurl(region);
				LLPanelLogin::autologinToLocation(slurl);
			}
			else
				// region name and x coord as payload 
				if (tokens.size() == 2)
				{
					// invalid to only specify region and x coordinate
					// slurl code will revert to same as region only, so do this anyway
					LLSLURL slurl(region);
					LLPanelLogin::autologinToLocation(slurl);
				}
				else
					// region name and x/y coord as payload 
					if (tokens.size() == 3)
					{
						// region and x/y specified - default z to 0
						F32 xpos;
						std::istringstream codec(tokens[1].asString());
						codec >> xpos;

						F32 ypos;
						codec.clear();
						codec.str(tokens[2].asString());
						codec >> ypos;

						const LLVector3 location(xpos, ypos, 0.0f);
						LLSLURL slurl(region, location);

						LLPanelLogin::autologinToLocation(slurl);
					}
					else
						// region name and x/y/z coord as payload 
						if (tokens.size() == 4)
						{
							// region and x/y/z specified - ok
							F32 xpos;
							std::istringstream codec(tokens[1].asString());
							codec >> xpos;

							F32 ypos;
							codec.clear();
							codec.str(tokens[2].asString());
							codec >> ypos;

							F32 zpos;
							codec.clear();
							codec.str(tokens[3].asString());
							codec >> zpos;

							const LLVector3 location(xpos, ypos, zpos);
							LLSLURL slurl(region, location);

							LLPanelLogin::autologinToLocation(slurl);
						};
		}
		return true;
	}
};
LLLoginLocationAutoHandler gLoginLocationAutoHandler;

//---------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------
LLPanelLogin::LLPanelLogin(const LLRect &rect,
						 void (*callback)(S32 option, void* user_data),
						 void *cb_data)
:	LLPanel(),
	mCallback(callback),
	mCallbackData(cb_data),
	mListener(new LLPanelLoginListener(this))
{
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	mPasswordModified = FALSE;

	sInstance = this;

	buildFromFile("panel_login.xml");

	LLView* login_holder = gViewerWindow->getLoginPanelHolder();
	if (login_holder)
	{
		login_holder->addChild(this);
	}

	reshape(rect.getWidth(), rect.getHeight());

	//BD
	mLoginBtn = getChild<LLButton>("connect_btn");
	mLoginBtn->setCommitCallback(boost::bind(&LLPanelLogin::onClickConnect, this));
	mLoginBtn->setFocus(true);

	mFavoritesCombo = getChild<LLComboBox>("start_location_combo");
	mFavoritesCombo->setReturnCallback(boost::bind(&LLPanelLogin::onClickConnect, this));
	mFavoritesCombo->setFocusLostCallback(boost::bind(&LLPanelLogin::onLocationSLURL, this));

	mUsernameCombo = getChild<LLComboBox>("username_combo");
	mUsernameCombo->setCommitCallback(boost::bind(&LLPanelLogin::onUserListCommit, this));
	// STEAM-14: When user presses Enter with this field in focus, initiate login
	mUsernameCombo->setReturnCallback(boost::bind(&LLPanelLogin::onClickConnect, this));
	mUsernameCombo->setKeystrokeOnEsc(TRUE);

	mPasswordEdit = getChild<LLLineEditor>("password_edit");
	mPasswordEdit->setKeystrokeCallback(boost::bind(&LLPanelLogin::onPassKey, this), NULL);
	// STEAM-14: When user presses Enter with this field in focus, initiate login
	mPasswordEdit->setCommitCallback(boost::bind(&LLPanelLogin::onClickConnect, this));

	mRememberPassCheck = getChild<LLCheckBoxCtrl>("remember_password");
	mRememberMeCheck = getChild<LLCheckBoxCtrl>("remember_name");
	mRememberMeCheck->setCommitCallback(boost::bind(&LLPanelLogin::onRememberUserCheck, this));

	mGridCombo = getChild<LLComboBox>("server_combo");
	mGridCombo->setCommitCallback(boost::bind(&LLPanelLogin::onSelectServer, this));
	// Load all of the grids, sorted, and then add a bar and the current grid at the top
    std::string current_grid = LLGridManager::getInstance()->getGrid();
    std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
    for (std::map<std::string, std::string>::iterator grid_choice = known_grids.begin();
        grid_choice != known_grids.end();
        grid_choice++)
    {
        if (!grid_choice->first.empty() && current_grid != grid_choice->first)
        {
            // _LL_DEBUGS("AppInit") << "adding " << grid_choice->first << LL_ENDL;
			mGridCombo->add(grid_choice->second, grid_choice->first);
        }
    }
	mGridCombo->sortByName();

    // _LL_DEBUGS("AppInit") << "adding current " << current_grid << LL_ENDL;
	mGridCombo->add(LLGridManager::getInstance()->getGridLabel(),
        current_grid,
        ADD_TOP);
	mGridCombo->selectFirstItem();

	LLSLURL start_slurl(LLStartUp::getStartSLURL());
	// The StartSLURL might have been set either by an explicit command-line
	// argument (CmdLineLoginLocation) or by default.
	// current_grid might have been set either by an explicit command-line
	// argument (CmdLineGridChoice) or by default.
	// If the grid specified by StartSLURL is the same as current_grid, the
	// distinction is moot.
	// If we have an explicit command-line SLURL, use that.
	// If we DON'T have an explicit command-line SLURL but we DO have an
	// explicit command-line grid, which is different from the default SLURL's
	// -- do NOT override the explicit command-line grid with the grid from
	// the default SLURL!
	bool force_grid{ start_slurl.getGrid() != current_grid &&
					 gSavedSettings.getString("CmdLineLoginLocation").empty() &&
				   ! gSavedSettings.getString("CmdLineGridChoice").empty() };
	if (!start_slurl.isSpatial()) // has a start been established by the command line or NextLoginLocation ? 
	{
		// no, so get the preference setting
		std::string defaultStartLocation = gSavedSettings.getString("LoginLocation");
		LL_INFOS("AppInit") << "default LoginLocation '" << defaultStartLocation << "'" << LL_ENDL;
		LLSLURL defaultStart(defaultStartLocation);
		if ( defaultStart.isSpatial() && ! force_grid )
		{
			LLStartUp::setStartSLURL(defaultStart);
		}
		else
		{
			LL_INFOS("AppInit") << (force_grid? "--grid specified" : "no valid LoginLocation") << ", using home" << LL_ENDL;
			LLSLURL homeStart(LLSLURL::SIM_LOCATION_HOME);
			LLStartUp::setStartSLURL(homeStart);
		}
	}
	else if (! force_grid)
	{
		onUpdateStartSLURL(start_slurl); // updates grid if needed
	}

	//BD - Show last logged in user favorites in "Start at" combo.
	addFavoritesToStartLocation();

	getChild<LLButton>("quit_btn")->setCommitCallback(boost::bind(&LLPanelLogin::onClickQuit, this));
	getChild<LLButton>("forgot_password_text")->setCommitCallback(boost::bind(&LLPanelLogin::onClickForgotPassword, this));
	getChild<LLButton>("create_new_account_text")->setCommitCallback(boost::bind(&LLPanelLogin::onClickNewAccount, this));

	//BD - Version info.
	std::string channel = LLVersionInfo::instance().getChannel();
	std::string version = llformat("%s (%d)", LLVersionInfo::instance().getShortVersion().c_str(), LLVersionInfo::instance().getBuild());
	LLButton* channel_text = getChild<LLButton>("channel_text");
	channel_text->setLabelArg("[CHANNEL]", channel);
	channel_text->setLabelArg("[VERSION]", version);
	channel_text->setLabelArg("[VIEWER_VERSION_LOCAL]", LLTrans::getString("VIEWER_VERSION_LOCAL"));
	channel_text->setLabelArg("[VIEWER_VERSION_IDENTIFIER]", LLTrans::getString("VIEWER_VERSION_IDENTIFIER"));

	//BD - Intel GPU's are trash and their performance is lackluster, show the
	//     user a warning that they will have to expect subpar performance.
	bool is_good_gpu = (gGLManager.mIsNVIDIA || gGLManager.mIsAMD);
	getChild<LLPanel>("intel_warning_panel")->setVisible(!is_good_gpu);
	getChild<LLUICtrl>("intel_warning_icon1")->setVisible(!is_good_gpu);
	getChild<LLUICtrl>("intel_warning_icon2")->setVisible(!is_good_gpu);

	//BD - Force preferences to initialize.
	LLFloaterReg::getInstance("preferences");

    LLCheckBoxCtrl* remember_name = getChild<LLCheckBoxCtrl>("remember_name");
    remember_name->setCommitCallback(boost::bind(&LLPanelLogin::onRememberUserCheck, this));
    getChild<LLCheckBoxCtrl>("remember_password")->setCommitCallback(boost::bind(&LLPanelLogin::onRememberPasswordCheck, this));
}

void LLPanelLogin::addFavoritesToStartLocation()
{
	// Clear the combo.
	if (!mFavoritesCombo) return;
	int num_items = mFavoritesCombo->getItemCount();
	for (int i = num_items - 1; i > 1; i--)
	{
		mFavoritesCombo->remove(i);
	}

	// Load favorites into the combo.
	std::string user_defined_name = mUsernameCombo->getSimple();
	LLStringUtil::trim(user_defined_name);
	LLStringUtil::toLower(user_defined_name);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites_" + LLGridManager::getInstance()->getGrid() + ".xml");
	std::string old_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	updateLoginButtons();

	std::string::size_type index = user_defined_name.find_first_of(" ._");
	if (index != std::string::npos)
	{
		std::string username = user_defined_name.substr(0, index);
		std::string lastname = user_defined_name.substr(index+1);
		if (lastname == "resident")
		{
			user_defined_name = username;
		}
		else
		{
			user_defined_name = username + " " + lastname;
		}
	}

	LLSD fav_llsd;
	llifstream file;
	file.open(filename.c_str());
	if (!file.is_open())
	{
		file.open(old_filename.c_str());
		if (!file.is_open()) return;
	}
	LLSDSerialize::fromXML(fav_llsd, file);
	for (LLSD::map_const_iterator iter = fav_llsd.beginMap();
		iter != fav_llsd.endMap(); ++iter)
	{
		// The account name in stored_favorites.xml has Resident last name even if user has
		// a single word account name, so it can be compared case-insensitive with the
		// user defined "firstname lastname".
		S32 res = LLStringUtil::compareInsensitive(user_defined_name, iter->first);
		if (res != 0)
		{
			// _LL_DEBUGS() << "Skipping favorites for " << iter->first << LL_ENDL;
			continue;
		}

		mFavoritesCombo->addSeparator();
		// _LL_DEBUGS() << "Loading favorites for " << iter->first << LL_ENDL;
		LLSD user_llsd = iter->second;
        bool update_password_setting = true;
		for (LLSD::array_const_iterator iter1 = user_llsd.beginArray();
			iter1 != user_llsd.endArray(); ++iter1)
		{
            if ((*iter1).has("save_password"))
            {
                bool save_password = (*iter1)["save_password"].asBoolean();
                gSavedSettings.setBOOL("RememberPassword", save_password);
                if (!save_password)
                {
                    getChild<LLButton>("connect_btn")->setEnabled(false);
                }
                update_password_setting = false;
            }

            std::string label = (*iter1)["name"].asString();
			std::string value = (*iter1)["slurl"].asString();
			if(label != "" && value != "")
			{
				mFavoritesCombo->add(label, value);
				if ( LLStartUp::getStartSLURL().getSLURLString() == value)
				{
					mFavoritesCombo->selectByValue(value);
				}
			}
		}
        if (update_password_setting)
        {
            gSavedSettings.setBOOL("UpdateRememberPasswordSetting", TRUE);
        }
		break;
	}
	if (mFavoritesCombo->getValue().asString().empty())
	{
		mFavoritesCombo->selectFirstItem();
        // Value 'home' or 'last' should have been taken from NextLoginLocation
        // but NextLoginLocation was not set, so init it from combo explicitly
        onLocationSLURL();
	}
}

LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// Controls having keyboard focus by default
	// must reset it on destroy. (EXT-2748)
	gFocusMgr.setDefaultKeyboardFocus(NULL);
}

// virtual
void LLPanelLogin::draw()
{
	LLPanel::draw();
}

// virtual
BOOL LLPanelLogin::handleKeyHere(KEY key, MASK mask)
{
	return LLPanel::handleKeyHere(key, mask);
}

// static
void LLPanelLogin::giveFocus()
{
	if( sInstance )
	{
		// Grab focus and move cursor to first blank input field
		std::string username = sInstance->mUsernameCombo->getValue().asString();
		std::string pass = sInstance->mPasswordEdit->getValue().asString();

		if (!username.empty() && pass.empty())
		{
			// User saved his name but not his password.  Move
			// focus to password field.
			sInstance->mPasswordEdit->setFocus(TRUE);
			sInstance->mPasswordEdit->selectAll();
		}
		else
		{
			// User doesn't have a name, so start there.
			sInstance->mUsernameCombo->setFocus(TRUE);
		}
	}
}

// static
void LLPanelLogin::show(const LLRect &rect,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
    if (!LLPanelLogin::sInstance)
    {
        new LLPanelLogin(rect, callback, callback_data);
    }

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		sInstance->setFocus(TRUE);
	}

	// Make sure that focus always goes here (and use the latest sInstance that was just created)
	gFocusMgr.setDefaultKeyboardFocus(sInstance);
}

//static
void LLPanelLogin::reshapePanel()
{
    if (sInstance)
    {
        LLRect rect = sInstance->getRect();
        sInstance->reshape(rect.getWidth(), rect.getHeight());
    }
}

//static
void LLPanelLogin::populateFields(LLPointer<LLCredential> credential, bool remember_user, bool remember_psswrd)
{
    if (!sInstance)
    {
        LL_WARNS() << "Attempted fillFields with no login view shown" << LL_ENDL;
        return;
    }

    sInstance->getChild<LLUICtrl>("remember_name")->setValue(remember_user);
    LLUICtrl* remember_password = sInstance->getChild<LLUICtrl>("remember_password");
    remember_password->setValue(remember_user && remember_psswrd);
    remember_password->setEnabled(remember_user);
    sInstance->populateUserList(credential);
}

//static
void LLPanelLogin::resetFields()
{
    if (!sInstance)
    {
        // class not existing at this point might happen since this
        // function is used to reset list in case of changes by external sources
        return;
    }

	LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
	sInstance->populateUserList(cred);
}

// static
void LLPanelLogin::setFields(LLPointer<LLCredential> credential)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted fillFields with no login view shown" << LL_ENDL;
		return;
	}
	sCredentialSet = TRUE;
	LL_INFOS("Credentials") << "Setting login fields to " << *credential << LL_ENDL;

	LLSD identifier = credential.notNull() ? credential->getIdentifier() : LLSD();

	if(identifier.has("type") && (std::string)identifier["type"] == "agent") 
	{
		// not nessesary for panel_login.xml, needed for panel_login_first.xml
		std::string firstname = identifier["first_name"].asString();
		std::string lastname = identifier["last_name"].asString();
	    std::string login_id = firstname;
	    if (!lastname.empty() && lastname != "Resident" && lastname != "resident")
	    {
		    // support traditional First Last name SLURLs
		    login_id += " ";
		    login_id += lastname;
	    }
		sInstance->mUsernameCombo->setLabel(login_id);
	}
	else if(identifier.has("type") && (std::string)identifier["type"] == "account")
	{
		std::string login_id = identifier["account_name"].asString();
		sInstance->mUsernameCombo->setLabel(login_id);
	}
	else
	{
		sInstance->mUsernameCombo->setLabel(std::string());
	}

	sInstance->addFavoritesToStartLocation();
	// if the password exists in the credential, set the password field with
	// a filler to get some stars
	LLSD authenticator = credential.notNull() ? credential->getAuthenticator() : LLSD();
	LL_INFOS("Credentials") << "Setting authenticator field " << authenticator["type"].asString() << LL_ENDL;
	if(authenticator.isMap() && 
	   authenticator.has("secret") && 
	   (authenticator["secret"].asString().size() > 0))
	{
		
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD characters so we get a 
		// nice row of asterisks.
		const std::string filler("123456789!123456");
		sInstance->mPasswordEdit->setValue(filler);
		sInstance->updateLoginButtons();
	}
	else
	{
		sInstance->mPasswordEdit->setValue(std::string());
	}
}

// static
void LLPanelLogin::getFields(LLPointer<LLCredential>& credential, bool& remember_user, bool& remember_psswrd)
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getFields with no login view shown" << LL_ENDL;
		return;
	}

	LLSD identifier = LLSD::emptyMap();
	LLSD authenticator = LLSD::emptyMap();

	std::string username = sInstance->mUsernameCombo->getSimple();
	std::string password = sInstance->mPasswordEdit->getValue().asString();
	LLStringUtil::trim(username);

	LL_INFOS("Credentials", "Authentication") << "retrieving username:" << username << LL_ENDL;
	// determine if the username is a first/last form or not.
	size_t separator_index = username.find_first_of(' ');

	if (separator_index == username.npos && !LLGridManager::getInstance()->isSystemGrid())
	{
		LL_INFOS("Credentials", "Authentication") << "account: " << username << LL_ENDL;
		// single username, so this is a 'clear' identifier
		identifier["type"] = CRED_IDENTIFIER_TYPE_ACCOUNT;
		identifier["account_name"] = username;
		
		if (LLPanelLogin::sInstance->mPasswordModified)
		{
			// password is plaintext
			authenticator["type"] = CRED_AUTHENTICATOR_TYPE_CLEAR;
			authenticator["secret"] = password;
		}
        else
        {
            credential = load_user_credentials(username);
            if (credential.notNull())
            {
                authenticator = credential->getAuthenticator();
                if (authenticator.emptyMap())
                {
                    // Likely caused by user trying to log in to non-system grid
                    // with unsupported name format, just retry
                    LL_WARNS() << "Authenticator failed to load for: " << username << LL_ENDL;
                    // password is plaintext
                    authenticator["type"] = CRED_AUTHENTICATOR_TYPE_CLEAR;
                    authenticator["secret"] = password;
                }
            }
        }
	}
	else
	{
		// Be lenient in terms of what separators we allow for two-word names
		// and allow legacy users to login with firstname.lastname
		separator_index = username.find_first_of(" ._");
		std::string first = username.substr(0, separator_index);
		std::string last;
		if (separator_index != username.npos)
		{
			last = username.substr(separator_index+1, username.npos);
			LLStringUtil::trim(last);
		}
		else
		{
			// ...on Linden grids, single username users as considered to have
			// last name "Resident"
			// *TODO: Make login.cgi support "account_name" like above
			last = "Resident";
		}
		
		if (last.find_first_of(' ') == last.npos)
		{
			LL_INFOS("Credentials", "Authentication") << "agent: " << username << LL_ENDL;
			// traditional firstname / lastname
			identifier["type"] = CRED_IDENTIFIER_TYPE_AGENT;
			identifier["first_name"] = first;
			identifier["last_name"] = last;
		
			if (LLPanelLogin::sInstance->mPasswordModified)
			{
				authenticator = LLSD::emptyMap();
				authenticator["type"] = CRED_AUTHENTICATOR_TYPE_HASH;
				authenticator["algorithm"] = "md5";
				LLMD5 pass((const U8 *)password.c_str());
				char md5pass[33];               /* Flawfinder: ignore */
				pass.hex_digest(md5pass);
				authenticator["secret"] = md5pass;
			}
            else
            {
                std::string key = first + "_" + last;
                LLStringUtil::toLower(key);
                credential = load_user_credentials(key);
                if (credential.notNull())
                {
                    authenticator = credential->getAuthenticator();
                }
            }
		}
	}
	credential = gSecAPIHandler->createCredential(LLGridManager::getInstance()->getGrid(), identifier, authenticator);

    remember_psswrd = sInstance->getChild<LLUICtrl>("remember_password")->getValue();
    remember_user = sInstance->getChild<LLUICtrl>("remember_name")->getValue();
}


// static
BOOL LLPanelLogin::areCredentialFieldsDirty()
{
	if (!sInstance)
	{
		LL_WARNS() << "Attempted getServer with no login view shown" << LL_ENDL;
	}
	else
	{
		std::string username = sInstance->mUsernameCombo->getValue().asString();
		LLStringUtil::trim(username);
		if (sInstance->mUsernameCombo->getCurrentIndex() == -1 && !username.empty())
		{
			return true;
		}
		if(sInstance->mPasswordEdit->isDirty()) 
		{
			return true;
		}
	}
	return false;	
}

// static - called from LLStartUp::setStartSLURL
void LLPanelLogin::onUpdateStartSLURL(const LLSLURL& new_start_slurl)
{
	if (!sInstance) return;

	// _LL_DEBUGS("AppInit") << new_start_slurl.asString() << LL_ENDL;

	/*
	* Determine whether or not the new_start_slurl modifies the grid.
	*
	* Note that some forms that could be in the slurl are grid-agnostic.,
	* such as "home".  Other forms, such as
	* https://grid.example.com/region/Party%20Town/20/30/5
	* specify a particular grid; in those cases we want to change the grid
	* and the grid selector to match the new value.
	*/
	enum LLSLURL::SLURL_TYPE new_slurl_type = new_start_slurl.getType();
	switch (new_slurl_type)
	{
	case LLSLURL::LOCATION:
	{
		std::string slurl_grid = LLGridManager::getInstance()->getGrid(new_start_slurl.getGrid());
		if (!slurl_grid.empty()) // is that a valid grid?
		{
			if (slurl_grid != LLGridManager::getInstance()->getGrid()) // new grid?
			{
				// the slurl changes the grid, so update everything to match
				LLGridManager::getInstance()->setGridChoice(slurl_grid);

				// update the grid selector to match the slurl
				std::string server_label(LLGridManager::getInstance()->getGridLabel(slurl_grid));
				sInstance->mGridCombo->setSimple(server_label);

				updateServer(); // to change the links and splash screen
			}
			if (new_start_slurl.getLocationString().length())
			{
					
				sInstance->mFavoritesCombo->setLabel(new_start_slurl.getLocationString());
				sInstance->updateLoginButtons();
			}
		}
		else
		{
			// the grid specified by the slurl is not known
			LLNotificationsUtil::add("InvalidLocationSLURL");
			LL_WARNS("AppInit") << "invalid LoginLocation:" << new_start_slurl.asString() << LL_ENDL;
			sInstance->mFavoritesCombo->setTextEntry(LLStringUtil::null);
		}
	}
	break;

	case LLSLURL::HOME_LOCATION:
		LL_INFOS("AppInit") << "Location setStartSLURL: " << new_start_slurl.asString() << LL_ENDL;
		break;

	default:
		LL_WARNS("AppInit") << "invalid login slurl, using home" << LL_ENDL;
		break;
	}
}

void LLPanelLogin::setLocation(const LLSLURL& slurl)
{
	// _LL_DEBUGS("AppInit") << "setting Location " << slurl.asString() << LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above
}

void LLPanelLogin::autologinToLocation(const LLSLURL& slurl)
{
	// _LL_DEBUGS("AppInit") << "automatically logging into Location " << slurl.asString() << LL_ENDL;
	LLStartUp::setStartSLURL(slurl); // calls onUpdateStartSLURL, above

	if (sInstance)
	{
		sInstance->onClickConnect();
	}
}

// static
void LLPanelLogin::closePanel()
{
	if (sInstance)
	{
		if (LLPanelLogin::sInstance->getParent())
		{
			LLPanelLogin::sInstance->getParent()->removeChild(LLPanelLogin::sInstance);
		}

		delete sInstance;
		sInstance = NULL;
	}
}

//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------
//BD
void LLPanelLogin::onClickQuit()
{
	LLAppViewer::instance()->userQuit();
	return;
}

// static
void LLPanelLogin::onClickConnect()
{
	if (sInstance && sInstance->mCallback)
	{
		/*if (commit_fields)
		{
			// JC - Make sure the fields all get committed.
			sInstance->setFocus(FALSE);
		}*/

		LLSD combo_val = sInstance->mGridCombo->getSelectedValue();

		// the grid definitions may come from a user-supplied grids.xml, so they may not be good
		// _LL_DEBUGS("AppInit")<<"grid "<<combo_val.asString()<<LL_ENDL;
		try
		{
			LLGridManager::getInstance()->setGridChoice(combo_val.asString());
		}
		catch (const LLInvalidGridName& ex)
		{
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);
			return;
		}

		// The start location SLURL has already been sent to LLStartUp::setStartSLURL

		std::string username = sInstance->mUsernameCombo->getValue().asString();
		std::string password = sInstance->mPasswordEdit->getValue().asString();
		
		if(username.empty())
		{
			// user must type in something into the username field
			LLNotificationsUtil::add("MustHaveAccountToLogIn");
		}
		else if(password.empty())
		{
		    LLNotificationsUtil::add("MustEnterPasswordToLogIn");
		}
		else
		{
			sCredentialSet = FALSE;
			LLPointer<LLCredential> cred;
			bool remember_1, remember_2;
			getFields(cred, remember_1, remember_2);
			std::string identifier_type;
			cred->identifierType(identifier_type);
			LLSD allowed_credential_types;
			LLGridManager::getInstance()->getLoginIdentifierTypes(allowed_credential_types);
			
			// check the typed in credential type against the credential types expected by the server.
			for(LLSD::array_iterator i = allowed_credential_types.beginArray();
				i != allowed_credential_types.endArray();
				i++)
			{
				
				if(i->asString() == identifier_type)
				{
					// yay correct credential type
					sInstance->mCallback(0, sInstance->mCallbackData);
					return;
				}
			}
			
			// Right now, maingrid is the only thing that is picky about
			// credential format, as it doesn't yet allow account (single username)
			// format creds.  - Rox.  James, we wanna fix the message when we change
			// this.
			LLNotificationsUtil::add("InvalidCredentialFormat");			
		}
	}
}

//BD
// static
void LLPanelLogin::onClickNewAccount()
{
	if (sInstance)
	{
		LLWeb::loadURLExternal(LLTrans::getString("create_account_url"));
	}
}


// static
void LLPanelLogin::onClickVersion()
{
	LLFloaterReg::showInstance("sl_about"); 
}

//static
void LLPanelLogin::onClickForgotPassword()
{
	if (sInstance )
	{
		LLWeb::loadURLExternal(sInstance->getString( "forgot_password_url" ));
	}
}

//static
void LLPanelLogin::onClickSignUp()
{
	if (sInstance)
	{
		LLWeb::loadURLExternal(sInstance->getString("sign_up_url"));
	}
}

// static
void LLPanelLogin::onUserNameTextEnty()
{
    sInstance->mPasswordModified = true;
    sInstance->mPasswordEdit->setValue(std::string());
    sInstance->addFavoritesToStartLocation(); //will call updateLoginButtons()
}

// static
void LLPanelLogin::onUserListCommit()
{
    if (sInstance)
    {
        static S32 ind = -1;
        if (ind != mUsernameCombo->getCurrentIndex())
        {
			std::string user_key = mUsernameCombo->getSelectedValue();
            LLPointer<LLCredential> cred = gSecAPIHandler->loadFromCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), user_key);
            setFields(cred);
            sInstance->mPasswordModified = false;
        }
        else
        {
           std::string pass = sInstance->mPasswordEdit->getValue().asString();
           if (pass.empty())
           {
               sInstance->giveFocus();
           }
           else
           {
               onClickConnect();
           }
        }
    }
}

// static
void LLPanelLogin::onRememberUserCheck(void*)

{
    if (sInstance)
    {
        bool remember = remember_name->getValue().asBoolean();
        if (!sInstance->mFirstLoginThisInstall
            && user_combo->getCurrentIndex() != -1
            && !remember)
        {
            remember = true;
			sInstance->mRememberMeCheck->setValue(true);
            LLNotificationsUtil::add("LoginCantRemoveUsername");
        }
        if (!remember)
        {
            sInstance->mRememberPassCheck->setValue(false);
        }
		sInstance->mRememberPassCheck->setEnabled(remember);
    }
}

void LLPanelLogin::onRememberPasswordCheck(void*)
{
    if (sInstance)
    {
        gSavedSettings.setBOOL("UpdateRememberPasswordSetting", TRUE);
    }
}

// static
void LLPanelLogin::onPassKey()
{
	mPasswordModified = TRUE;
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == FALSE)
	{
		// *TODO: use another way to notify user about enabled caps lock, see EXT-6858
		sCapslockDidNotification = TRUE;
	}
	updateLoginButtons();
}


void LLPanelLogin::updateServer()
{
	if (sInstance)
	{
		try 
		{
			// if they've selected another grid, we should load the credentials
			// for that grid and set them to the UI. But if there were any modifications to
			// fields, modifications should carry over.
			// Not sure if it should carry over password but it worked like this before login changes
			// Example: you started typing in and found that your are under wrong grid,
			// you switch yet don't lose anything
			if (sInstance->areCredentialFieldsDirty())
			{
				// save modified creds
				std::string username = sInstance->mUsernameCombo->getSimple();
				LLStringUtil::trim(username);
				std::string password = sInstance->mPasswordEdit->getValue().asString();

				// populate dropbox and setFields
				// Note: following call is related to initializeLoginInfo()
				LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
				sInstance->populateUserList(credential);

				// restore creds
				sInstance->mUsernameCombo->setTextEntry(username);
				sInstance->mPasswordEdit->setValue(password);
			}
			else
			{
				// populate dropbox and setFields
				// Note: following call is related to initializeLoginInfo()
				LLPointer<LLCredential> credential = gSecAPIHandler->loadCredential(LLGridManager::getInstance()->getGrid());
				sInstance->populateUserList(credential);
			}

			// update the login panel links 
			bool system_grid = LLGridManager::getInstance()->isSystemGrid();

			//BD
			sInstance->getChildView("create_new_account_text")->setVisible( system_grid);
			sInstance->getChildView("forgot_password_text")->setVisible(system_grid);
		}
		catch (const LLInvalidGridName& ex)
		{
			LL_WARNS("AppInit")<<"server '"<<ex.name()<<"' selection failed"<<LL_ENDL;
			LLSD args;
			args["GRID"] = ex.name();
			LLNotificationsUtil::add("InvalidGrid", args);	
			return;
		}
	}
}

void LLPanelLogin::updateLoginButtons()
{
	mLoginBtn->setEnabled(mUsernameCombo->getValue().asString().length() != 0 
						&& mPasswordEdit->getValue().asString().length() != 0);

	if (mUsernameCombo->getCurrentIndex() != -1)
	{
		mRememberMeCheck->setValue(true);
	} // Note: might be good idea to do "else remember_name->setValue(mRememberedState)" but it might behave 'weird' to user
}

void LLPanelLogin::populateUserList(LLPointer<LLCredential> credential)
{
    mUsernameCombo->removeall();
	mUsernameCombo->clear();

    if (gSecAPIHandler->hasCredentialMap("login_list", LLGridManager::getInstance()->getGrid()))
    {
        LLSecAPIHandler::credential_map_t credencials;
        gSecAPIHandler->loadCredentialMap("login_list", LLGridManager::getInstance()->getGrid(), credencials);

        LLSecAPIHandler::credential_map_t::iterator cr_iter = credencials.begin();
        LLSecAPIHandler::credential_map_t::iterator cr_end = credencials.end();
        while (cr_iter != cr_end)
        {
            if (cr_iter->second.notNull()) // basic safety in case of future changes
            {
                // cr_iter->first == user_id , to be able to be find it in case we select it
				mUsernameCombo->add(LLPanelLogin::getUserName(cr_iter->second), cr_iter->first, ADD_BOTTOM, TRUE);
            }
            cr_iter++;
        }

		if (credential.isNull() || !mUsernameCombo->setSelectedByValue(LLSD(credential->userID()), true))
        {
            // selection failed, just deselect whatever might be selected
			mUsernameCombo->setValue(std::string());
            mPasswordEdit->setValue(std::string());

            // selection failed, fields will be mepty
            updateLoginButtons();
        }
        else
        {
            setFields(credential);
        }
    }
    else
    {
        if (credential.notNull())
        {
            const LLSD &ident = credential->getIdentifier();
            if (ident.isMap() && ident.has("type"))
            {
				mUsernameCombo->add(LLPanelLogin::getUserName(credential), credential->userID(), ADD_BOTTOM, TRUE);
                // this llsd might hold invalid credencial (failed login), so
                // do not add to the list, just set field.
                setFields(credential);
            }
            else
            {
                updateLoginButtons();
            }
        }
        else
        {
            updateLoginButtons();
        }
    }
}


void LLPanelLogin::onSelectServer()
{
	// The user twiddled with the grid choice ui.
	// apply the selection to the grid setting.
	//LLComboBox* server_combo = getChild<LLComboBox>("server_combo");
	LLSD server_combo_val = mGridCombo->getSelectedValue();
	LL_INFOS("AppInit") << "grid "<<server_combo_val.asString()<< LL_ENDL;
	LLGridManager::getInstance()->setGridChoice(server_combo_val.asString());
	addFavoritesToStartLocation();
	
	/*
	 * Determine whether or not the value in the start_location_combo makes sense
	 * with the new grid value.
	 *
	 * Note that some forms that could be in the location combo are grid-agnostic,
	 * such as "MyRegion/128/128/0".  There could be regions with that name on any
	 * number of grids, so leave them alone.  Other forms, such as
	 * https://grid.example.com/region/Party%20Town/20/30/5 specify a particular
	 * grid; in those cases we want to clear the location.
	 */
	S32 index = mFavoritesCombo->getCurrentIndex();
	switch (index)
	{
	case 0: // last location
        LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_LAST));
        break;
	case 1: // home location
        LLStartUp::setStartSLURL(LLSLURL(LLSLURL::SIM_LOCATION_HOME));
		break;
		
	default:
		{
			std::string location = mFavoritesCombo->getValue().asString();
			LLSLURL slurl(location); // generata a slurl from the location combo contents
			if (location.empty()
				|| (slurl.getType() == LLSLURL::LOCATION
				    && slurl.getGrid() != LLGridManager::getInstance()->getGrid())
				   )
			{
				// the grid specified by the location is not this one, so clear the combo
				mFavoritesCombo->setCurrentByIndex(0); // last location on the new grid
				onLocationSLURL();
			}
		}			
		break;
	}

	updateServer();
}

void LLPanelLogin::onLocationSLURL()
{
	//LLComboBox* location_combo = getChild<LLComboBox>("start_location_combo");
	std::string location = mFavoritesCombo->getValue().asString();
	// _LL_DEBUGS("AppInit") << location << LL_ENDL;

	LLStartUp::setStartSLURL(location); // calls onUpdateStartSLURL, above 
}

// static
std::string LLPanelLogin::getUserName(LLPointer<LLCredential> &cred)
{
    if (cred.isNull())
    {
        return "unknown";
    }
    const LLSD &ident = cred->getIdentifier();

    if (!ident.isMap())
    {
        return "unknown";
    }
    else if ((std::string)ident["type"] == "agent")
    {
        std::string second_name = ident["last_name"];
        if (second_name == "resident" || second_name == "Resident")
        {
            return (std::string)ident["first_name"];
        }
        return (std::string)ident["first_name"] + " " + (std::string)ident["last_name"];
    }
    else if ((std::string)ident["type"] == "account")
    {
        return LLCacheName::cleanFullName((std::string)ident["account_name"]);
    }

    return "unknown";
}
