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