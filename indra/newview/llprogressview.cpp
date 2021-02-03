/** 
 * @file llprogressview.cpp
 * @brief LLProgressView class implementation
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

#include "llprogressview.h"

#include "indra_constants.h"
#include "llmath.h"
#include "llgl.h"
#include "llrender.h"
#include "llui.h"
#include "llfontgl.h"
#include "lltimer.h"
#include "lltextbox.h"
#include "llglheaders.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llnotifications.h"
#include "llprogressbar.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llweb.h"
#include "lluictrlfactory.h"
#include "llpanellogin.h"

//BD
#include "lliconctrl.h"

#include <time.h>

LLProgressView* LLProgressView::sInstance = NULL;

S32 gStartImageWidth = 1;
S32 gStartImageHeight = 1;
const F32 FADE_TO_WORLD_TIME = 1.0f;
// ## Zi: Fade teleport screens
const F32 FADE_FROM_LOGIN_TIME = 0.7f;

//BD
const F32 CYCLE_TIMER = 7.0f;

static LLPanelInjector<LLProgressView> r("progress_view");

// XUI: Translate
LLProgressView::LLProgressView() 
:	LLPanel(),
	mPercentDone( 0.f ),
	mMouseDownInActiveArea( false ),
	mUpdateEvents("LLProgressView"),
	mFadeToWorldTimer(),
	mFadeFromLoginTimer(),
	mStartupComplete(false),
	//BD
	mTipCycleTimer()
{
	mUpdateEvents.listen("self", boost::bind(&LLProgressView::handleUpdate, this, _1));
}

BOOL LLProgressView::postBuild()
{
	mProgressBar = getChild<LLProgressBar>("login_progress_bar");

	mCancelBtn = getChild<LLButton>("cancel_btn");
	mCancelBtn->setClickedCallback(  LLProgressView::onCancelButtonClicked, NULL );
	// ## Zi: Fade teleport screens
	mFadeToWorldTimer.stop();
	mFadeFromLoginTimer.stop();

	//BD
	mTipCycleTimer.getStarted();
	mMessageText = getChild<LLUICtrl>("message_text");
	mStateText = getChild<LLUICtrl>("state_text");
	mStatusText = getChild<LLUICtrl>("status_text");
	mPercentText = getChild<LLTextBox>("percent_text");

	// hidden initially, until we need it
	setVisible(FALSE);

	LLNotifications::instance().getChannel("AlertModal")->connectChanged(boost::bind(&LLProgressView::onAlertModal, this, _1));

	sInstance = this;
	return TRUE;
}


LLProgressView::~LLProgressView()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

BOOL LLProgressView::handleHover(S32 x, S32 y, MASK mask)
{
	if( childrenHandleHover( x, y, mask ) == NULL )
	{
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
	}
	return TRUE;
}


BOOL LLProgressView::handleKeyHere(KEY key, MASK mask)
{
	// Suck up all keystokes except CTRL-Q.
	if( ('Q' == key) && (MASK_CONTROL == mask) )
	{
		LLAppViewer::instance()->userQuit();
	}
	return TRUE;
}

void LLProgressView::setStartupComplete()
{
	mStartupComplete = true;

	// ## Zi: Fade teleport screens
	mFadeFromLoginTimer.stop();
	mFadeToWorldTimer.start();
}

void LLProgressView::setProgressVisible(BOOL visible, BOOL logout)
{
    if (!visible && mFadeFromLoginTimer.getStarted())
    {
        mFadeFromLoginTimer.stop();
    }
	// hiding progress view
	if (getVisible() && !visible)
	{
		LLPanel::setVisible(FALSE);
	}
	// showing progress view
	else if (visible && (!getVisible() || mFadeToWorldTimer.getStarted()))
	{
		if (logout)
		{
			getChild<LLIconCtrl>("loading_bg")->setVisible(false);
			LLPanel::setBackgroundVisible(true);
		}
		setFocus(TRUE);
		mFadeToWorldTimer.stop();
		LLPanel::setVisible(TRUE);
	} 
}

// ## Zi: Fade teleport screens
void LLProgressView::fade(BOOL in)
{
	if(in)
	{
		mFadeFromLoginTimer.start();
		mFadeToWorldTimer.stop();
		setVisible(TRUE);
	}
	else
	{
		mFadeFromLoginTimer.stop();
		mFadeToWorldTimer.start();
		// set visibility will be done in the draw() method after fade
	}
}
// ## Zi: Fade teleport screens

void LLProgressView::setTip()
{
	if(mTipCycleTimer.getElapsedTimeAndResetF32() > CYCLE_TIMER)
	{
		srand( (unsigned)time( NULL ) );
		int mRandom = rand() % 25;
		std::string output = llformat("LoadingTip %i" , mRandom);
		mMessageText->setValue(getString(output));
	}
}

void LLProgressView::setText(const std::string& string)
{
	mStatusText->setValue(string);
}

void LLProgressView::setMessage(const std::string& string)
{
	mStateText->setValue(string);
}

/*
void LLProgressView::drawStartTexture(F32 alpha)
{
	gGL.pushMatrix();	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f(0.f, 0.f, 0.f, alpha);		// ## Zi: Fade teleport screens
	gl_rect_2d(getRect());
	gGL.popMatrix();
}*/


void LLProgressView::draw()
{
	static LLTimer timer;
	//BD
	F32 alpha;

	//BD
	setTip();

	if (mFadeFromLoginTimer.getStarted())
	{
		//BD
		alpha = clamp_rescale(mFadeFromLoginTimer.getElapsedTimeF32(), 0.f, FADE_FROM_LOGIN_TIME, 0.f, 1.f);

		LLViewDrawContext context(alpha);

		LLPanel::draw();

		// ## Zi: Fade teleport screens
		if (mFadeFromLoginTimer.getElapsedTimeF32() > FADE_FROM_LOGIN_TIME )
		{
			mFadeFromLoginTimer.stop();
			LLPanelLogin::closePanel();
			//BD - Enable the quit button after fade so we avoid spurious double-clicks on login.
			mCancelBtn->setEnabled(true);
		}

		return;
	}

	// handle fade out to world view when we're asked to
	if (mFadeToWorldTimer.getStarted())
	{
		//BD
		alpha = clamp_rescale(mFadeToWorldTimer.getElapsedTimeF32(), 0.f, FADE_TO_WORLD_TIME, 1.f, 0.f);

		LLViewDrawContext context(alpha);

		LLPanel::draw();

		// faded out completely - remove panel and reveal world
		if (mFadeToWorldTimer.getElapsedTimeF32() > FADE_TO_WORLD_TIME )
		{
			mFadeToWorldTimer.stop();

			// Fade is complete, release focus
			gFocusMgr.releaseFocusIfNeeded( this );

			// turn off panel that hosts intro so we see the world
			setVisible(FALSE);

			if (LLStartUp::getStartupState() >= STATE_STARTED)
			{
				//BD - Make progress screen transparent.
				LLPanel::setBackgroundVisible(false);
				getChild<LLIconCtrl>("loading_bg")->setVisible(true);
			}

			gStartTexture = NULL;
		}
		return;
	}

	LLPanel::draw();
}

void LLProgressView::setPercent(const F32 percent)
{
	//BD
	S32 percent_label = std::min(100.f, percent);
	mPercentText->setValue(percent_label);
	mProgressBar->setValue(percent);
}

/*void LLProgressView::setMessage(const std::string& msg)
{
	mMessage = msg;
	getChild<LLUICtrl>("message_text")->setValue(mMessage);
}

void LLProgressView::loadLogo(const std::string &path,
                              const U8 image_codec,
                              const LLRect &pos_rect,
                              const LLRectf &clip_rect,
                              const LLRectf &offset_rect)
{
    // We need these images very early, so we have to force-load them, otherwise they might not load in time.
    if (!gDirUtilp->fileExists(path))
    {
        return;
    }

    LLPointer<LLImageFormatted> start_image_frmted = LLImageFormatted::createFromType(image_codec);
    if (!start_image_frmted->load(path))
    {
        LL_WARNS("AppInit") << "Image load failed: " << path << LL_ENDL;
        return;
    }

    LLPointer<LLImageRaw> raw = new LLImageRaw;
    if (!start_image_frmted->decode(raw, 0.0f))
    {
        LL_WARNS("AppInit") << "Image decode failed " << path << LL_ENDL;
        return;
    }
    // HACK: getLocalTexture allows only power of two dimentions
    raw->expandToPowerOfTwo();

    TextureData data;
    data.mTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
    data.mDrawRect = pos_rect;
    data.mClipRect = clip_rect;
    data.mOffsetRect = offset_rect;
    mLogosList.push_back(data);
}

void LLProgressView::initLogos()
{
    mLogosList.clear();

    const U8 image_codec = IMG_CODEC_PNG;
    const LLRectf default_clip(0.f, 1.f, 1.f, 0.f);
    const S32 default_height = 28;
    const S32 default_pad = 15;

    S32 icon_width, icon_height;

    // We don't know final screen rect yet, so we can't precalculate position fully
    LLTextBox *logos_label = getChild<LLTextBox>("logos_lbl");
    S32 texture_start_x = logos_label->getFont()->getWidthF32(logos_label->getText()) + default_pad;
    S32 texture_start_y = -7;

    // Normally we would just preload these textures from textures.xml,
    // and display them via icon control, but they are only needed on
    // startup and preloaded/UI ones stay forever
    // (and this code was done already so simply reused it)
    std::string temp_str = gDirUtilp->getExpandedFilename(LL_PATH_DEFAULT_SKIN, "textures", "3p_icons");

    temp_str += gDirUtilp->getDirDelimiter();

#ifdef LL_FMODSTUDIO
    // original image size is 264x96, it is on longer side but
    // with no internal paddings so it gets additional padding
    icon_width = 77;
    icon_height = 21;
    S32 pad_fmod_y = 4;
    texture_start_x++;
    loadLogo(temp_str + "fmod_logo.png",
        image_codec,
        LLRect(texture_start_x, texture_start_y + pad_fmod_y + icon_height, texture_start_x + icon_width, texture_start_y + pad_fmod_y),
        default_clip,
        default_clip);

    texture_start_x += icon_width + default_pad + 1;
#endif //LL_FMODSTUDIO
#ifdef LL_HAVOK
    // original image size is 342x113, central element is on a larger side
    // plus internal padding, so it gets slightly more height than desired 32
    icon_width = 88;
    icon_height = 29;
    S32 pad_havok_y = -1;
    loadLogo(temp_str + "havok_logo.png",
        image_codec,
        LLRect(texture_start_x, texture_start_y + pad_havok_y + icon_height, texture_start_x + icon_width, texture_start_y + pad_havok_y),
        default_clip,
        default_clip);

    texture_start_x += icon_width + default_pad;
#endif //LL_HAVOK

    // 108x41
    icon_width = 74;
    icon_height = default_height;
    loadLogo(temp_str + "vivox_logo.png",
        image_codec,
        LLRect(texture_start_x, texture_start_y + icon_height, texture_start_x + icon_width, texture_start_y),
        default_clip,
        default_clip);
}

void LLProgressView::initStartTexture(S32 location_id, bool is_in_production)
{
    if (gStartTexture.notNull())
    {
        gStartTexture = NULL;
        LL_INFOS("AppInit") << "re-initializing start screen" << LL_ENDL;
    }

    LL_DEBUGS("AppInit") << "Loading startup bitmap..." << LL_ENDL;

    U8 image_codec = IMG_CODEC_PNG;
    std::string temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter();

    if ((S32)START_LOCATION_ID_LAST == location_id)
    {
        temp_str += LLStartUp::getScreenLastFilename();
    }
    else
    {
        std::string path = temp_str + LLStartUp::getScreenHomeFilename();

        if (!gDirUtilp->fileExists(path) && is_in_production)
        {
            // Fallback to old file, can be removed later
            // Home image only sets when user changes home, so it will take time for users to switch to pngs
            temp_str += "screen_home.bmp";
            image_codec = IMG_CODEC_BMP;
        }
        else
        {
            temp_str = path;
        }
    }

    LLPointer<LLImageFormatted> start_image_frmted = LLImageFormatted::createFromType(image_codec);

    // Turn off start screen to get around the occasional readback 
    // driver bug
    if (!gSavedSettings.getBOOL("UseStartScreen"))
    {
        LL_INFOS("AppInit") << "Bitmap load disabled" << LL_ENDL;
        return;
    }
    else if (!start_image_frmted->load(temp_str))
    {
        LL_WARNS("AppInit") << "Bitmap load failed" << LL_ENDL;
        gStartTexture = NULL;
    }
    else
    {
        gStartImageWidth = start_image_frmted->getWidth();
        gStartImageHeight = start_image_frmted->getHeight();

        LLPointer<LLImageRaw> raw = new LLImageRaw;
        if (!start_image_frmted->decode(raw, 0.0f))
        {
            LL_WARNS("AppInit") << "Bitmap decode failed" << LL_ENDL;
            gStartTexture = NULL;
        }
        else
        {
            // HACK: getLocalTexture allows only power of two dimentions
            raw->expandToPowerOfTwo();
            gStartTexture = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
        }
    }

    if (gStartTexture.isNull())
    {
        gStartTexture = LLViewerTexture::sBlackImagep;
        gStartImageWidth = gStartTexture->getWidth();
        gStartImageHeight = gStartTexture->getHeight();
    }
}

void LLProgressView::initTextures(S32 location_id, bool is_in_production)
{
    initStartTexture(location_id, is_in_production);
    initLogos();

    childSetVisible("panel_icons", mLogosList.empty() ? FALSE : TRUE);
    childSetVisible("panel_top_spacer", mLogosList.empty() ? TRUE : FALSE);
}

void LLProgressView::releaseTextures()
{
    gStartTexture = NULL;
    mLogosList.clear();

    childSetVisible("panel_top_spacer", TRUE);
    childSetVisible("panel_icons", FALSE);
}*/

void LLProgressView::setCancelButtonVisible(BOOL b, const std::string& label)
{
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}

// static
void LLProgressView::onCancelButtonClicked(void*)
{
	// Quitting viewer here should happen only when "Quit" button is pressed while starting up.
	// Check for startup state is used here instead of teleport state to avoid quitting when
	// cancel is pressed while teleporting inside region (EXT-4911)
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		LL_INFOS() << "User requesting quit during login" << LL_ENDL;
		LLAppViewer::instance()->requestQuit();
	}
	else
	{
		gAgent.teleportCancel();
		sInstance->mCancelBtn->setEnabled(FALSE);
		sInstance->setVisible(FALSE);
	}
}

bool LLProgressView::handleUpdate(const LLSD& event_data)
{
	LLSD percent = event_data.get("percent");
	
	if(percent.isDefined())
	{
		setPercent(percent.asReal());
	}
	return false;
}

bool LLProgressView::onAlertModal(const LLSD& notify)
{
	// if the progress view is visible, it will obscure the notification window
	// in this case, we want to auto-accept WebLaunchExternalTarget notifications
	if (isInVisibleChain() && notify["sigtype"].asString() == "add")
	{
		LLNotificationPtr notifyp = LLNotifications::instance().find(notify["id"].asUUID());
		if (notifyp && notifyp->getName() == "WebLaunchExternalTarget")
		{
			notifyp->respondWithDefault();
		}
	}
	return false;
}