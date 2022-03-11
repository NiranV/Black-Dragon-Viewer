/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
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

#include "llstatusbar.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llpanelnearbymedia.h"
#include "llpanelpresetspulldown.h"
#include "llpanelvolumepulldown.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhints.h"
#include "llhudicon.h"
#include "llnavigationbar.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llrootview.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewermedia.h"
#include "llviewermenu.h"	// for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "lltrans.h"

// library includes
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include "llsearchableui.h"
#include "llsearcheditor.h"

// system includes
#include <iomanip>

//BD
#include "llfloatersidepanelcontainer.h"
#include "lliconctrl.h"
#include "llsidepanelinventory.h"
#include "lltoolbarview.h"
#include "bdpaneldrawdistance.h"
//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const S32 SIM_STAT_WIDTH = 8;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY		= 3.f; // How long the balance and health icons should flash after a change.

static void onClickVolume(void*);

LLStatusBar::LLStatusBar(const LLRect& rect)
	: LLPanel(),
	mTextTime(NULL),
	mSGBandwidth(NULL),
	mSGPacketLoss(NULL),
	mBtnVolume(NULL),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0),
	//BD
	mMediaEnabled(false),
	mShowNetStats(false)
{
	setRect(rect);
	
	// status bar can possible overlay menus?
	setMouseOpaque(FALSE);

	mHealthTimer = new LLFrameTimer();

	buildFromFile("panel_status_bar.xml");
}

LLStatusBar::~LLStatusBar()
{
	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	refresh();
	LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	show_navbar_context_menu(this,x,y);
	return TRUE;
}

BOOL LLStatusBar::postBuild()
{
	gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

	mTextTime = getChild<LLTextBox>("TimeText" );
//	//BD - Statusbar Framerate Count
	mFPSText = getChild<LLTextBox>("FPSText");

	//BD
	mMediaEnabled = (gSavedSettings.getBOOL("AudioStreamingMusic") || gSavedSettings.getBOOL("AudioStreamingMedia"));
	gSavedSettings.getControl("AudioStreamingMedia")->getSignal()->connect(boost::bind(&LLStatusBar::setMediaEnabled, this, _2));
	gSavedSettings.getControl("AudioStreamingMusic")->getSignal()->connect(boost::bind(&LLStatusBar::setMediaEnabled, this, _2));

	mShowNetStats = gSavedSettings.getBOOL("ShowNetStats");
	gSavedSettings.getControl("ShowNetStats")->getSignal()->connect(boost::bind(&LLStatusBar::setShowNetStats, this, _2));

	mIconPresets = getChild<LLIconCtrl>( "presets_icon" );
	mIconPresets->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterPresets, this));

	mBtnVolume = getChild<LLButton>( "volume_btn" );
	mBtnVolume->setClickedCallback( onClickVolume, this );
	mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));

	mMediaToggle = getChild<LLButton>("media_toggle_btn");
	mMediaToggle->setClickedCallback( &LLStatusBar::onClickMediaToggle, this );
	mMediaToggle->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterNearbyMedia, this));

//	//BD - Quick Draw Distance Slider
	mDrawDistance = getChild<LLIconCtrl>("draw_distance_icon");
	mDrawDistance->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterDrawDistance, this));

	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));

	// Adding Net Stat Graph
	S32 x = getRect().getWidth() - 2;
	S32 y = 0;
	LLRect r;
	//BD
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT+34, x, y+32);
	LLStatGraph::Params sgp;
	sgp.name("BandwidthGraph");
	sgp.rect(r);
	sgp.follows.flags(FOLLOWS_TOP | FOLLOWS_RIGHT);
	sgp.mouse_opaque(false);
	sgp.stat.count_stat_float(&LLStatViewer::ACTIVE_MESSAGE_DATA_RECEIVED);
	sgp.units("Kbps");
	sgp.precision(0);
	sgp.per_sec(true);
	mSGBandwidth = LLUICtrlFactory::create<LLStatGraph>(sgp);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;
	mSGBandwidth->setVisible(mShowNetStats);

	//BD
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT+34, x, y+32);
	//these don't seem to like being reused
	LLStatGraph::Params pgp;
	pgp.name("PacketLossPercent");
	pgp.rect(r);
	pgp.follows.flags(FOLLOWS_TOP | FOLLOWS_RIGHT);
	pgp.mouse_opaque(false);
	pgp.stat.sample_stat_float(&LLStatViewer::PACKETS_LOST_PERCENT);
	pgp.units("%");
	pgp.min(0.f);
	pgp.max(5.f);
	pgp.precision(1);
	pgp.per_sec(false);
	LLStatGraph::Thresholds thresholds;
	thresholds.threshold.add(LLStatGraph::ThresholdParams().value(0.1).color(LLColor4::green))
						.add(LLStatGraph::ThresholdParams().value(0.25f).color(LLColor4::yellow))
						.add(LLStatGraph::ThresholdParams().value(0.6f).color(LLColor4::red));
	pgp.thresholds(thresholds);
	mSGPacketLoss = LLUICtrlFactory::create<LLStatGraph>(pgp);
	addChild(mSGPacketLoss);
	mSGPacketLoss->setVisible(mShowNetStats);

	mPanelPresetsPulldown = new LLPanelPresetsPulldown();
	addChild(mPanelPresetsPulldown);
	mPanelPresetsPulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelPresetsPulldown->setVisible(FALSE);

	mPanelVolumePulldown = new LLPanelVolumePulldown();
	addChild(mPanelVolumePulldown);
	mPanelVolumePulldown->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelVolumePulldown->setVisible(FALSE);

	mPanelNearByMedia = new LLPanelNearByMedia();
	addChild(mPanelNearByMedia);
	mPanelNearByMedia->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelNearByMedia->setVisible(FALSE);

//	//BD - Quick Draw Distance Slider
	mPanelDrawDistance = new BDPanelDrawDistance();
	addChild(mPanelDrawDistance);
	mPanelDrawDistance->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);
	mPanelDrawDistance->setVisible(FALSE);

	return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	if (mShowNetStats)
	{
		// Adding Net Stat Meter back in
		F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
		mSGBandwidth->setMin(0.f);
		mSGBandwidth->setMax(bwtotal*1.25f);
		//mSGBandwidth->setThreshold(0, bwtotal*0.75f);
		//mSGBandwidth->setThreshold(1, bwtotal);
		//mSGBandwidth->setThreshold(2, bwtotal);
	}

//	//BD - Statusbar Framerate Count
	//     Oh no, we finally did it, we clamped it. No performance gain whatsoever.
	if (mFPSUpdateTimer.getElapsedTimeF32() > 0.1f)
	{
		mFPSUpdateTimer.reset();
		LLTrace::Recording& recording = LLTrace::get_frame_recording().getLastRecording();
		mFPSText->setValue(recording.getPerSec(LLStatViewer::FPS));
	}
	
	// update clock every 10 seconds
	if(mClockUpdateTimer.getElapsedTimeF32() > 10.f)
	{
		mClockUpdateTimer.reset();

		// Get current UTC time, adjusted for the user's clock
		// being off.
		time_t utc_time;
		utc_time = time_corrected();

		std::string timeStr = getString("time");
		LLSD substitution;
		substitution["datetime"] = (S32) utc_time;
		LLStringUtil::format (timeStr, substitution);
		mTextTime->setText(timeStr);

		// set the tooltip to have the date
		std::string dtStr = getString("timeTooltip");
		LLStringUtil::format (dtStr, substitution);
		mTextTime->setToolTip (dtStr);
	}

	// update the master volume button state
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	mBtnVolume->setToggleState(mute_audio);

	LLViewerMedia* media_inst = LLViewerMedia::getInstance();

	// Disable media toggle if there's no media, parcel media, and no parcel audio
	// (or if media is disabled)
	bool button_enabled = mMediaEnabled &&
						  (media_inst->hasInWorldMedia() || media_inst->hasParcelMedia() || media_inst->hasParcelAudio());
	mMediaToggle->setEnabled(button_enabled);
	// Note the "sense" of the toggle is opposite whether media is playing or not
	bool any_media_playing = (media_inst->isAnyMediaPlaying() || 
							  media_inst->isParcelMediaPlaying() ||
							  media_inst->isParcelAudioPlaying());
	mMediaToggle->setValue(!any_media_playing);
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
//	//BD - Hide UI In Mouselook
	if(gSavedSettings.getBOOL("AllowUIHidingInML"))
	{
		setVisible(visible);
	}
}

// static
/*void LLStatusBar::sendMoneyBalanceRequest()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );

    if (gDisconnected)
    {
        //LL_DEBUGS() << "Trying to send message when disconnected, skipping balance request!" << LL_ENDL;
        return;
    }
    if (!gAgent.getRegion())
    {
        //LL_DEBUGS() << "LLAgent::sendReliableMessage No region for agent yet, skipping balance request!" << LL_ENDL;
        return;
    }
    // Double amount of retries due to this request initially happening during busy stage
    // Ideally this should be turned into a capability
    gMessageSystem->sendReliable(gAgent.getRegionHost(), LL_DEFAULT_RELIABLE_RETRIES * 2, TRUE, LL_PING_BASED_TIMEOUT_DUMMY, NULL, NULL);
}*/

void LLStatusBar::setHealth(S32 health)
{
	//LL_INFOS() << "Setting health to: " << buffer << LL_ENDL;
	if( mHealth > health )
	{
		if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
		{
			if (isAgentAvatarValid())
			{
				if (gAgentAvatarp->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}

		mHealthTimer->reset();
		mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
	}

	mHealth = health;
}

S32 LLStatusBar::getHealth() const
{
	return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}


void LLStatusBar::onMouseEnterPresets()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLIconCtrl* icon =  getChild<LLIconCtrl>( "presets_icon" );
	LLRect icon_rect = icon->getRect();
	LLRect pulldown_rect = mPanelPresetsPulldown->getRect();
	pulldown_rect.setLeftTopAndSize(icon_rect.mLeft -
	     (pulldown_rect.getWidth() - icon_rect.getWidth()),
			       icon_rect.mBottom,
			       pulldown_rect.getWidth(),
			       pulldown_rect.getHeight());

	pulldown_rect.translate(popup_holder->getRect().getWidth() - pulldown_rect.mRight, 0);
	mPanelPresetsPulldown->setShape(pulldown_rect);

	// show the master presets pull-down
	LLUI::getInstance()->clearPopups();
	LLUI::getInstance()->addPopup(mPanelPresetsPulldown);
	mPanelNearByMedia->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(FALSE);
	mPanelPresetsPulldown->setVisible(TRUE);
}

void LLStatusBar::onMouseEnterVolume()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLButton* volbtn =  getChild<LLButton>( "volume_btn" );
	LLRect vol_btn_rect = volbtn->getRect();
	LLRect volume_pulldown_rect = mPanelVolumePulldown->getRect();
	volume_pulldown_rect.setLeftTopAndSize(vol_btn_rect.mLeft -
	     (volume_pulldown_rect.getWidth() - vol_btn_rect.getWidth()),
			       vol_btn_rect.mBottom,
			       volume_pulldown_rect.getWidth(),
			       volume_pulldown_rect.getHeight());

	volume_pulldown_rect.translate(popup_holder->getRect().getWidth() - volume_pulldown_rect.mRight, 0);
	mPanelVolumePulldown->setShape(volume_pulldown_rect);


	// show the master volume pull-down
	LLUI::getInstance()->clearPopups();
	LLUI::getInstance()->addPopup(mPanelVolumePulldown);
	mPanelPresetsPulldown->setVisible(FALSE);
	mPanelNearByMedia->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(TRUE);
//	//BD - Quick Draw Distance Slider
	mPanelDrawDistance->setVisible(FALSE);
}

void LLStatusBar::onMouseEnterNearbyMedia()
{
	LLView* popup_holder = gViewerWindow->getRootView()->getChildView("popup_holder");
	LLRect nearby_media_rect = mPanelNearByMedia->getRect();
	LLButton* nearby_media_btn =  getChild<LLButton>( "media_toggle_btn" );
	LLRect nearby_media_btn_rect = nearby_media_btn->getRect();
	nearby_media_rect.setLeftTopAndSize(nearby_media_btn_rect.mLeft - 
										(nearby_media_rect.getWidth() - nearby_media_btn_rect.getWidth())/2,
										nearby_media_btn_rect.mBottom,
										nearby_media_rect.getWidth(),
										nearby_media_rect.getHeight());
	// force onscreen
	nearby_media_rect.translate(popup_holder->getRect().getWidth() - nearby_media_rect.mRight, 0);
	
	// show the master volume pull-down
	mPanelNearByMedia->setShape(nearby_media_rect);
	LLUI::getInstance()->clearPopups();
	LLUI::getInstance()->addPopup(mPanelNearByMedia);

	mPanelPresetsPulldown->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(FALSE);
	mPanelNearByMedia->setVisible(TRUE);
//	//BD - Quick Draw Distance Slider
	mPanelDrawDistance->setVisible(FALSE);
}

//BD - Quick Draw Distance Slider
void LLStatusBar::onMouseEnterDrawDistance()
{
	//LLUICtrl* statusbar_stack = getChild<LLUICtrl>("statusbar_stack");
	LLRect draw_distance_rect = mPanelDrawDistance->getRect();
	LLIconCtrl* draw_distance_icon =  getChild<LLIconCtrl>( "draw_distance_icon" );
	LLRect draw_distance_icon_rect = draw_distance_icon->getRect();
	
	LLRect new_rect;
	draw_distance_icon->localRectToOtherView(draw_distance_icon->getLocalRect(), &new_rect, this);

	draw_distance_rect.setLeftTopAndSize(new_rect.mLeft - 7,
					new_rect.mBottom,
					draw_distance_rect.getWidth(),
					draw_distance_rect.getHeight());

	// force onscreen
	//draw_distance_rect.translate(statusbar_stack->getRect().mLeft + draw_distance_rect.mLeft, 0);
	
	// show the draw distance pull-down
	mPanelDrawDistance->setShape(draw_distance_rect);
	LLUI::getInstance()->clearPopups();
	LLUI::getInstance()->addPopup(mPanelDrawDistance);

	mPanelNearByMedia->setVisible(FALSE);
	mPanelVolumePulldown->setVisible(FALSE);
	mPanelDrawDistance->setVisible(TRUE);
}

static void onClickVolume(void* data)
{
	// toggle the master mute setting
	bool mute_audio = LLAppViewer::instance()->getMasterSystemAudioMute();
	LLAppViewer::instance()->setMasterSystemAudioMute(!mute_audio);	
}

//static 
void LLStatusBar::onClickMediaToggle(void* data)
{
	LLStatusBar *status_bar = (LLStatusBar*)data;
	// "Selected" means it was showing the "play" icon (so media was playing), and now it shows "pause", so turn off media
	bool pause = status_bar->mMediaToggle->getValue();
	LLViewerMedia::getInstance()->setAllMediaPaused(pause);
}

BOOL can_afford_transaction(S32 cost)
{
	//BD
	LLSidepanelInventory* sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	return((cost <= 0)||((sidepanel_inventory) && (sidepanel_inventory->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
	refresh();
}

//BD
void LLStatusBar::setShowNetStats(bool enabled)
{
	mShowNetStats = enabled;
	mSGBandwidth->setVisible(enabled);
	mSGPacketLoss->setVisible(enabled);
}


// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
	// Requires "trusted" browser/URL source
	LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1
			&& tokens[0].asString() == "request")
		{
			//BD
			LLSidepanelInventory::sendMoneyBalanceRequest();
			return true;
		}
		return false;
	}
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
