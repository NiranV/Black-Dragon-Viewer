/** 
 * @file llfloaterimcontainerbase.cpp
 * @brief Multifloater containing active IM sessions in separate tab container tabs
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

#include "llagent.h"
#include "llavatariconctrl.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llgroupiconctrl.h"
#include "llfloaterimsessiontab.h"
#include "llfloaterreg.h"
#include "llfloaterimcontainer.h"
#include "llfloaterimcontainerbase.h"
#include "llfloaterimcontainertab.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"

//
// LLFloaterIMContainerBase
//
LLFloaterIMContainerBase::LLFloaterIMContainerBase(const LLSD& seed, const Params& params /*= getDefaultParams()*/)
:	LLMultiFloater(seed, params)
{
	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLFloaterIMContainerBase::~LLFloaterIMContainerBase()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

// static
void LLFloaterIMContainerBase::onCurrentChannelChanged(const LLUUID& session_id)
{
    if (session_id != LLUUID::null)
    {
    	LLFloaterIMContainerBase::getInstance()->showConversation(session_id);
    }
}

BOOL LLFloaterIMContainerBase::postBuild()
{
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));

	return TRUE;
}

// virtual
void LLFloaterIMContainerBase::addFloater(LLFloater* floaterp,
									  BOOL select_added_floater,
									  LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	LLUUID session_id = floaterp->getKey();
// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2)
	if (session_id.isNull())
	{
		// Re-insert the nearby chat floater at the start
		insertion_point = LLTabContainer::START;
	}
// [/SL:KB]

	// Add the floater
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLIconCtrl* icon = 0;

// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2)
	if (session_id.isNull())
	{
		// Don't allow the nearby chat tab to be drag-rearranged
		mTabContainer->lockTabs(1);

		// Add an icon for the nearby chat floater
		LLIconCtrl::Params icon_params;
		icon_params.image = LLUI::getUIImage("Command_Chat_Icon");
		icon = LLUICtrlFactory::instance().create<LLIconCtrl>(icon_params);
	}
	else if (gAgent.isInGroup(session_id, TRUE))
// [/SL:KB]
//	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainerBase::onCloseFloater, this, session_id));
	}
	else
	{   LLUUID avatar_id = session_id.notNull()?
		    LLIMModel::getInstance()->getOtherParticipantID(session_id) : LLUUID();

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLFloaterIMContainerBase::onCloseFloater, this, session_id));
	}
	mTabContainer->setTabImage(floaterp, icon);
}

// [SL:KB] - Patch: Chat-NearbyChatBar | Checked: 2011-11-17 (Catznip-3.2)
void LLFloaterIMContainerBase::removeFloater(LLFloater* floaterp)
{
	LLUUID idSession = floaterp->getKey();
	if (idSession.isNull())
	{
		mTabContainer->unlockTabs();
	}
	LLMultiFloater::removeFloater(floaterp);
}
// [/SL:KB]

void LLFloaterIMContainerBase::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
	setFocus(TRUE);
}

// static
LLFloaterIMContainerBase* LLFloaterIMContainerBase::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterIMContainerBase>("im_container");
}

// static
LLFloaterIMContainerBase* LLFloaterIMContainerBase::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterIMContainerBase>("im_container");
}

// static
LLFloater* LLFloaterIMContainerBase::buildFloater(const LLSD& sdKey)
{
	LLFloater* pIMContainer = NULL;
	if (gSavedSettings.getBOOL("IMUseTabbedContainer"))
		pIMContainer = new LLFloaterIMContainerTab(sdKey);
	else
		pIMContainer = new LLFloaterIMContainerView(sdKey);
	return pIMContainer;
}

// static
const std::string& LLFloaterIMContainerBase::getFloaterXMLFile()
{
	static const std::string strFile = 
		(gSavedSettings.getBOOL("IMUseTabbedContainer")) 
			? (!gSavedSettings.getBOOL("IMUseVerticalTabs")) ? "floater_im_container_tab_horiz.xml" : "floater_im_container_tab_vert.xml"
			: "floater_im_container.xml";
	return strFile;
}

void LLFloaterIMContainerBase::setMinimized(BOOL b)
{
	bool was_minimized = isMinimized();
	LLMultiFloater::setMinimized(b);

	//Switching from minimized to un-minimized
	if(was_minimized && !b)
	{
		const LLUUID& session_id = getSelectedSession();
		LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::findConversation(session_id);

		if(session_floater && !session_floater->isTornOff())
		{
			//When in DND mode, remove stored IM notifications
			//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
			if(gAgent.isDoNotDisturb() && session_id.notNull())
			{
				LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, session_id);
			}
		}
	}
}

// static
bool LLFloaterIMContainerBase::isConversationLoggingAllowed()
{
	return gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 0;
}

// EOF
