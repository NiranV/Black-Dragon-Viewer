/** 
 * @file llpanelpermissions.h
 * @brief LLPanelPermissions class header file
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

#ifndef LL_LLPANELPERMISSIONS_H
#define LL_LLPANELPERMISSIONS_H

#include "llpanel.h"
#include "llstyle.h"
#include "lluuid.h"
#include "llgroupiconctrl.h"
#include "llavatariconctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llpanelpermissions
//
// Panel for permissions of an object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLAvatarName;
class LLTextBox;
class LLNameBox;
class LLViewerInventoryItem;
//BD - SSFUI
class LLTextBox;

class LLPanelPermissions : public LLPanel
{
public:
	LLPanelPermissions();
	virtual ~LLPanelPermissions();

	/*virtual*/	BOOL	postBuild();
	void updateOwnerName(const LLUUID& owner_id, const LLAvatarName& owner_name, const LLStyle::Params& style_params);
	void updateCreatorName(const LLUUID& creator_id, const LLAvatarName& creator_name, const LLStyle::Params& style_params);
	void refresh();							// refresh all labels as needed

protected:
	// statics
	void onClickClaim();
	void onClickRelease();
	void onClickGroup();
	void cbGroupID(LLUUID group_id);
	void onClickDeedToGroup();

	void onCommitPerm(LLUICtrl *ctrl, U8 field, U32 perm);

	void onCommitGroupShare(LLUICtrl *ctrl);

	void onCommitEveryoneMove(LLUICtrl *ctrl);
	void onCommitEveryoneCopy(LLUICtrl *ctrl);

	void onCommitNextOwnerModify(LLUICtrl* ctrl);
	void onCommitNextOwnerCopy(LLUICtrl* ctrl);
	void onCommitNextOwnerTransfer(LLUICtrl* ctrl);
	
	void onCommitName(LLUICtrl* ctrl);
	void onCommitDesc(LLUICtrl* ctrl);

	void onCommitSaleInfo(LLUICtrl* ctrl);
	void onCommitSaleType(LLUICtrl* ctrl);	
	void setAllSaleInfo();

	void onCommitClickAction(LLUICtrl* ctrl);
	void onCommitIncludeInSearch(LLUICtrl* ctrl);

	static LLViewerInventoryItem* findItem(LLUUID &object_id);

protected:
	void disableAll();
	
private:
	LLNameBox*		mLabelGroupName;		// group name
	LLTextBox*		mLabelOwnerName;
	LLTextBox*		mLabelCreatorName;
//	//BD - SSFUI
	LLTextBox*		mGroupNameSLURL;
	LLUUID			mCreatorID;
	LLUUID			mOwnerID;
	LLUUID			mLastOwnerID;

	LLButton*		mBtnDeedToGroup;

	LLLineEditor* mLabelName;
	LLLineEditor* mLabelDescription;

	LLLineEditor* mObjectName;
	LLLineEditor* mObjectDescription;

	LLComboBox* mSaleType;
	LLComboBox* mClickAction;

	LLButton* mBtnSetGroup;
	LLCheckBoxCtrl* mCheckboxGroupShare;

	LLCheckBoxCtrl* mCheckboxEveryoneMove;
	LLCheckBoxCtrl* mCheckboxEveryoneCopy;
	LLCheckBoxCtrl* mCheckboxForSale;
	LLUICtrl* mEditCost;

	LLCheckBoxCtrl* mCheckboxNextOwnerModify;
	LLCheckBoxCtrl* mCheckboxNextOwnerCopy;
	LLCheckBoxCtrl* mCheckboxNextOwnerTransfer;

	LLUICtrl* mPermModify;
	LLUICtrl* mPathfindingAttributes;

	LLUICtrl* mCreator;
	LLUICtrl* mOwner;
	LLUICtrl* mGroup;
	LLUICtrl* mName;
	LLUICtrl* mDescription;
	LLUICtrl* mCost;

	LLUICtrl* mB;
	LLUICtrl* mO;
	LLUICtrl* mG;
	LLUICtrl* mE;
	LLUICtrl* mN;
	LLUICtrl* mF;

	LLUICtrl* mNextOwner;
	LLUICtrl* mNextOwnerModify;
	LLUICtrl* mNextOwnerCopy;
	LLUICtrl* mNextOwnerTransfer;

	LLCheckBoxCtrl* mCheckboxSearch;

	LLUICtrl* mLabelClickAction;

	boost::signals2::connection mOwnerCacheConnection;
	boost::signals2::connection mCreatorCacheConnection;
};


#endif // LL_LLPANELPERMISSIONS_H
