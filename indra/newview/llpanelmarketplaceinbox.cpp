/** 
 * @file llpanelmarketplaceinbox.cpp
 * @brief Panel for marketplace inbox
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llpanelmarketplaceinbox.h"
#include "llpanelmarketplaceinboxinventory.h"

#include "llappviewer.h"
#include "llbutton.h"
#include "llinventorypanel.h"
#include "llfloatersidepanelcontainer.h"
#include "llfolderview.h"
#include "llsidepanelinventory.h"
#include "llviewercontrol.h"


static LLPanelInjector<LLPanelMarketplaceInbox> t_panel_marketplace_inbox("panel_marketplace_inbox");

const LLPanelMarketplaceInbox::Params& LLPanelMarketplaceInbox::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanelMarketplaceInbox>(); 
}

// protected
LLPanelMarketplaceInbox::LLPanelMarketplaceInbox(const Params& p)
	: LLPanel(p)
	, mSavedFolderState(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}

LLPanelMarketplaceInbox::~LLPanelMarketplaceInbox()
{
	delete mSavedFolderState;
}

// virtual
BOOL LLPanelMarketplaceInbox::postBuild()
{
	LLFocusableElement::setFocusReceivedCallback(boost::bind(&LLPanelMarketplaceInbox::onFocusReceived, this));
	
	return TRUE;
}

void LLPanelMarketplaceInbox::onSelectionChange()
{
}

void LLPanelMarketplaceInbox::onFocusReceived()
{
	LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
	if (sidepanel_inventory)
	{
		sidepanel_inventory->clearSelections(true, false);
		}
	
	gSavedPerAccountSettings.setU32("LastInventoryInboxActivity", time_corrected());
}

BOOL LLPanelMarketplaceInbox::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;
	return TRUE;
}

void LLPanelMarketplaceInbox::draw()
{	
	LLPanel::draw();
}
