/** 
 * @file LLSidepanelInventory.h
 * @brief Side Bar "Inventory" panel
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

#ifndef LL_LLSIDEPANELINVENTORY_H
#define LL_LLSIDEPANELINVENTORY_H

#include "llpanel.h"

//BD
#include "llinventoryobserver.h"

class LLButton;
class LLFolderViewItem;
class LLInboxAddedObserver;
class LLInventoryCategoriesObserver;
class LLInventoryItem;
class LLInventoryPanel;
class LLLayoutPanel;
class LLPanelMainInventory;
class LLSidepanelItemInfo;
class LLSidepanelTaskInfo;
//BD
class LLTextBox;

//BD
class LLSidepanelInventory : public LLPanel, LLInventoryObserver
{
public:
	LLSidepanelInventory();
	virtual ~LLSidepanelInventory();

private:
	void updateInbox();
	//BD
	void onClickBuyCurrency();
	static void onClickBalance(void* data);
	
public:
	void observeInboxCreation();
	void observeInboxModifications(const LLUUID& inboxID);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	//BD
	/*virtual*/ void draw();
	/*virtual*/ void changed(U32);

	void		setBalance(S32 balance);
	void		debitBalance(S32 debit);
	void		creditBalance(S32 credit);

	S32			getBalance() const;

	void setLandCredit(S32 credit);
	void setLandCommitted(S32 committed);

	BOOL isUserTiered() const;

	S32 getSquareMetersCommitted() const;
	S32 getSquareMetersLeft() const;
	S32 getSquareMetersCredit() const;

	// Request the latest currency balance from the server
	static void sendMoneyBalanceRequest();


	LLInventoryPanel* getActivePanel(); // Returns an active inventory panel, if any.
	void selectAllItemsPanel();
	LLInventoryPanel* getInboxPanel() const { return mInventoryPanelInbox.get(); }

	LLPanelMainInventory* getMainInventoryPanel() const { return mPanelMainInventory; }
	BOOL isMainInventoryPanelActive() const;

	void clearSelections(bool clearMain, bool clearInbox);
    std::set<LLFolderViewItem*> getInboxSelectionList();

	void showInventoryPanel();
    void initInventoryViews();

	// checks can share selected item(s)
	bool canShare();

	void onToggleInboxBtn();

	void enableInbox(bool enabled);
    void toggleInbox();
    void hideInbox();
	
	void openInbox();
	
	bool isInboxEnabled() const { return mInboxEnabled; }

	static void cleanup();

protected:
	// Tracks highlighted (selected) item in inventory panel.
	LLInventoryItem *getSelectedItem();
	U32 getSelectedCount();
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	// "wear", "teleport", etc.
	void performActionOnSelection(const std::string &action);

	bool canWearSelected(); // check whether selected items can be worn

	void onInboxChanged(const LLUUID& inbox_id);

	//BD
	void updateItemcountText();

	//
	// UI Elements
	//
private:
	LLPanel*					mInventoryPanel; // Main inventory view
	LLHandle<LLInventoryPanel>	mInventoryPanelInbox;
	LLPanelMainInventory*		mPanelMainInventory;

    LLLayoutPanel* mInboxLayoutPanel;

protected:
	void 						onBackButtonClicked();

private:
	bool						mInboxEnabled;

	LLInventoryCategoriesObserver* 	mCategoriesObserver;
	LLInboxAddedObserver*			mInboxAddedObserver;

	//BD
	LLUICtrl*                   mCounterCtrl;

	S32							mBalance;
	S32							mItemCount;
	S32							mSquareMetersCredit;
	S32							mSquareMetersCommitted;

	std::string 				mItemCountString;

	LLTextBox					*mBoxBalance;

	LLFrameTimer*				mBalanceTimer;
};

#endif //LL_LLSIDEPANELINVENTORY_H
