/** 
 * @file llpanelpermissions.cpp
 * @brief LLPanelPermissions class implementation
 * This class represents the panel in the build view for
 * viewing/editing object names, owners, permissions, etc.
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

#include "llpanelpermissions.h"

// library includes
#include "lluuid.h"
#include "llpermissions.h"
#include "llcategory.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llstring.h"

// project includes
#include "llviewerwindow.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llviewerobject.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llstatusbar.h"		// for getBalance()
#include "lllineeditor.h"
#include "llcombobox.h"
#include "lluiconstants.h"
#include "lldbstrings.h"
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llnamebox.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "roles_constants.h"
#include "llgroupactions.h"
#include "llgroupiconctrl.h"
#include "lltrans.h"
#include "llinventorymodel.h"

#include "llavatarnamecache.h"
#include "llcachename.h"

// [RLVa:KB] - Checked: 2010-08-25 (RLVa-1.2.2a)
#include "llslurl.h"
#include "rlvactions.h"
#include "rlvcommon.h"
// [/RLVa:KB]

U8 string_value_to_click_action(std::string p_value);
std::string click_action_to_string_value( U8 action);

U8 string_value_to_click_action(std::string p_value)
{
	if(p_value == "Touch")
	{
		return CLICK_ACTION_TOUCH;
	}
	if(p_value == "Sit")
	{
		return CLICK_ACTION_SIT;
	}
	if(p_value == "Buy")
	{
		return CLICK_ACTION_BUY;
	}
	if(p_value == "Pay")
	{
		return CLICK_ACTION_PAY;
	}
	if(p_value == "Open")
	{
		return CLICK_ACTION_OPEN;
	}
	if(p_value == "Zoom")
	{
		return CLICK_ACTION_ZOOM;
	}
	if (p_value == "None")
	{
		return CLICK_ACTION_DISABLED;
	}
	return CLICK_ACTION_TOUCH;
}

std::string click_action_to_string_value( U8 action)
{
	switch (action) 
	{
		case CLICK_ACTION_TOUCH:
		default:	
			return "Touch";
			break;
		case CLICK_ACTION_SIT:
			return "Sit";
			break;
		case CLICK_ACTION_BUY:
			return "Buy";
			break;
		case CLICK_ACTION_PAY:
			return "Pay";
			break;
		case CLICK_ACTION_OPEN:
			return "Open";
			break;
		case CLICK_ACTION_ZOOM:
			return "Zoom";
			break;
		case CLICK_ACTION_DISABLED:
			return "None";
			break;
	}
}

///----------------------------------------------------------------------------
/// Class llpanelpermissions
///----------------------------------------------------------------------------

// Default constructor
LLPanelPermissions::LLPanelPermissions() :
	LLPanel()
{
	setMouseOpaque(FALSE);
}

BOOL LLPanelPermissions::postBuild()
{
	mObjectName = getChild<LLLineEditor>("Object Name");
	mObjectName->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitName, this, _1));
	mObjectName->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);

	mObjectDescription = getChild<LLLineEditor>("Object Description");
	mObjectDescription->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
	mObjectDescription->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitDesc, this, _1));

	mBtnSetGroup = getChild<LLButton>("button set group");
	mBtnSetGroup->setCommitCallback(boost::bind(&LLPanelPermissions::onClickGroup, this));

	mCheckboxGroupShare = getChild<LLCheckBoxCtrl>("checkbox share with group");
	mCheckboxGroupShare->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitGroupShare, this, _1));
	mCheckboxEveryoneMove = getChild<LLCheckBoxCtrl>("checkbox allow everyone move");
	mCheckboxEveryoneMove->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitEveryoneMove, this, _1));
	mCheckboxEveryoneCopy = getChild<LLCheckBoxCtrl>("checkbox allow everyone copy");
	mCheckboxEveryoneCopy->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitEveryoneCopy, this, _1));
	mCheckboxForSale = getChild<LLCheckBoxCtrl>("checkbox for sale");
	mCheckboxForSale->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitSaleInfo, this, _1));
	mEditCost = getChild<LLUICtrl>("Edit Cost");
	mEditCost->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitSaleInfo, this, _1));

	mCheckboxNextOwnerModify = getChild<LLCheckBoxCtrl>("checkbox next owner can modify");
	mCheckboxNextOwnerModify->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitNextOwnerModify, this, _1));
	mCheckboxNextOwnerCopy = getChild<LLCheckBoxCtrl>("checkbox next owner can copy");
	mCheckboxNextOwnerCopy->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitNextOwnerCopy, this, _1));
	mCheckboxNextOwnerTransfer = getChild<LLCheckBoxCtrl>("checkbox next owner can transfer");
	mCheckboxNextOwnerTransfer->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitNextOwnerTransfer, this, _1));
	
	mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");
	mLabelOwnerName = getChild<LLTextBox>("Owner Name");
	mLabelCreatorName = getChild<LLTextBox>("Creator Name");
//	//BD - SSFUI
	mGroupNameSLURL = getChild<LLTextBox>("Group Name Label");

	mBtnDeedToGroup = getChild<LLButton>("button deed");
	mBtnDeedToGroup->setCommitCallback(boost::bind(&LLPanelPermissions::onClickDeedToGroup, this));
	mLabelName = getChild<LLLineEditor>("Object Name");
	mLabelDescription = getChild<LLLineEditor>("Object Description");

	mSaleType = getChild<LLComboBox>("sale type");
	mSaleType->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitSaleType, this, _1));
	mClickAction = getChild<LLComboBox>("clickaction");
	mClickAction->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitClickAction, this, _1));

	mCheckboxSearch = getChild<LLCheckBoxCtrl>("search_check");
	mCheckboxSearch->setCommitCallback(boost::bind(&LLPanelPermissions::onCommitIncludeInSearch, this, _1));


	mPermModify = getChild<LLUICtrl>("perm_modify");
	mPathfindingAttributes = getChild<LLUICtrl>("pathfinding_attributes_value");

	mCreator = getChild<LLUICtrl>("Creator:");
	mOwner = getChild<LLUICtrl>("Owner:");
	mGroup = getChild<LLUICtrl>("Group:");
	mName = getChild<LLUICtrl>("Name:");
	mDescription = getChild<LLUICtrl>("Description:");
	mCost = getChild<LLUICtrl>("Edit Cost");

	mB = getChild<LLUICtrl>("B:");
	mO = getChild<LLUICtrl>("O:");
	mG = getChild<LLUICtrl>("G:");
	mE = getChild<LLUICtrl>("E:");
	mN = getChild<LLUICtrl>("N:");
	mF = getChild<LLUICtrl>("F:");

	mNextOwner = getChild<LLUICtrl>("Next owner can:");
	mNextOwnerModify = getChild<LLUICtrl>("checkbox next owner can modify");
	mNextOwnerCopy = getChild<LLUICtrl>("checkbox next owner can copy");
	mNextOwnerTransfer = getChild<LLUICtrl>("checkbox next owner can transfer");

	mLabelClickAction = getChild<LLUICtrl>("label click action");

	return TRUE;
}


LLPanelPermissions::~LLPanelPermissions()
{
	if (mOwnerCacheConnection.connected())
	{
		mOwnerCacheConnection.disconnect();
	}
	if (mCreatorCacheConnection.connected())
	{
		mCreatorCacheConnection.disconnect();
	}
	// base class will take care of everything
}


void LLPanelPermissions::disableAll()
{
	mPermModify->setEnabled(FALSE);
	mPermModify->setValue(LLStringUtil::null);

	mPathfindingAttributes->setEnabled(FALSE);
	mPathfindingAttributes->setValue(LLStringUtil::null);

	mCreator->setEnabled(FALSE);
	mLabelCreatorName->setValue(LLStringUtil::null);
	mLabelCreatorName->setEnabled(FALSE);

	mOwner->setEnabled(FALSE);
	mLabelOwnerName->setValue(LLStringUtil::null);
	mLabelOwnerName->setEnabled(FALSE);

	mGroup->setEnabled(FALSE);
	mLabelGroupName->setValue(LLStringUtil::null);
	mLabelGroupName->setEnabled(FALSE);
	mBtnSetGroup->setEnabled(FALSE);

	mObjectName->setValue(LLStringUtil::null);
	mObjectName->setEnabled(FALSE);
	mName->setEnabled(FALSE);
	mDescription->setEnabled(FALSE);
	mObjectDescription->setValue(LLStringUtil::null);
	mObjectDescription->setEnabled(FALSE);
		
	mCheckboxGroupShare->setValue(FALSE);
	mCheckboxGroupShare->setEnabled(FALSE);
	mBtnDeedToGroup->setEnabled(FALSE);

	mCheckboxEveryoneMove->setValue(FALSE);
	mCheckboxEveryoneMove->setEnabled(FALSE);
	mCheckboxEveryoneCopy->setValue(FALSE);
	mCheckboxEveryoneCopy->setEnabled(FALSE);

	//Next owner can:
	mNextOwner->setEnabled(FALSE);
	mNextOwnerModify->setValue(FALSE);
	mNextOwnerModify->setEnabled(FALSE);
	mNextOwnerCopy->setValue(FALSE);
	mNextOwnerCopy->setEnabled(FALSE);
	mNextOwnerTransfer->setValue(FALSE);
	mNextOwnerTransfer->setEnabled(FALSE);

	//checkbox for sale
	mCheckboxForSale->setValue(FALSE);
	mCheckboxForSale->setEnabled(FALSE);

	//checkbox include in search
	mCheckboxSearch->setValue(FALSE);
	mCheckboxSearch->setEnabled(FALSE);
	
	mSaleType->setValue(LLSaleInfo::FS_COPY);
	mSaleType->setEnabled(FALSE);
		
	mCost->setEnabled(FALSE);
	mCost->setValue(getString("Cost Default"));
	mEditCost->setValue(LLStringUtil::null);
	mEditCost->setEnabled(FALSE);
		
	mLabelClickAction->setEnabled(FALSE);
	mClickAction->setEnabled(FALSE);
	mClickAction->clear();

	mB->setVisible(FALSE);
	mO->setVisible(FALSE);
	mG->setVisible(FALSE);
	mE->setVisible(FALSE);
	mN->setVisible(FALSE);
	mF->setVisible(FALSE);
}

void LLPanelPermissions::refresh()
{
	std::string deedText;
	if (gWarningSettings.getBOOL("DeedObject"))
	{
		deedText = getString("text deed continued");
	}
	else
	{
		deedText = getString("text deed");
	}
	mBtnDeedToGroup->setLabelSelected(deedText);
	mBtnDeedToGroup->setLabelUnselected(deedText);

	BOOL root_selected = TRUE;
	LLSelectMgr* select_mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle selection = select_mgr->getSelection();
	LLSelectNode* nodep = selection->getFirstRootNode();
	S32 object_count = selection->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = selection->getFirstNode();
		object_count = selection->getObjectCount();
		root_selected = FALSE;
	}

	//BOOL attachment_selected = LLSelectMgr::getInstance()->getSelection()->isAttachment();
	//attachment_selected = false;
	LLViewerObject* objectp = NULL;
	if(nodep) objectp = nodep->getObject();
	if(!nodep || !objectp)// || attachment_selected)
	{
		// ...nothing selected
		disableAll();
		return;
	}

	// figure out a few variables
	const BOOL is_one_object = (object_count == 1);
	
	// BUG: fails if a root and non-root are both single-selected.
	BOOL is_perm_modify = (selection->getFirstRootNode() 
						&& select_mgr->selectGetRootsModify())
						|| select_mgr->selectGetModify();
	BOOL is_nonpermanent_enforced = (selection->getFirstRootNode() 
									&& select_mgr->selectGetRootsNonPermanentEnforced())
									|| select_mgr->selectGetNonPermanentEnforced();
	const LLFocusableElement* keyboard_focus_view = gFocusMgr.getKeyboardFocus();

	S32 string_index = 0;
	std::string MODIFY_INFO_STRINGS[] =
	{
		getString("text modify info 1"),
		getString("text modify info 2"),
		getString("text modify info 3"),
		getString("text modify info 4"),
		getString("text modify info 5"),
		getString("text modify info 6")
	};

	if (!is_perm_modify)
	{
		string_index += 2;
	}
	else if (!is_nonpermanent_enforced)
	{
		string_index += 4;
	}
	if (!is_one_object)
	{
		++string_index;
	}
	mPermModify->setEnabled(TRUE);
	mPermModify->setValue(MODIFY_INFO_STRINGS[string_index]);

	std::string pfAttrName;

	if ((selection->getFirstRootNode() 
		&& select_mgr->selectGetRootsNonPathfinding())
		|| select_mgr->selectGetNonPathfinding())
	{
		pfAttrName = "Pathfinding_Object_Attr_None";
	}
	else if ((selection->getFirstRootNode() 
		&& select_mgr->selectGetRootsPermanent())
		|| select_mgr->selectGetPermanent())
	{
		pfAttrName = "Pathfinding_Object_Attr_Permanent";
	}
	else if ((selection->getFirstRootNode() 
		&& select_mgr->selectGetRootsCharacter())
		|| select_mgr->selectGetCharacter())
	{
		pfAttrName = "Pathfinding_Object_Attr_Character";
	}
	else
	{
		pfAttrName = "Pathfinding_Object_Attr_MultiSelect";
	}

	mPathfindingAttributes->setEnabled(TRUE);
	mPathfindingAttributes->setValue(LLTrans::getString(pfAttrName));
	
	// Update creator text field
	mCreator->setEnabled(TRUE);
	std::string creator_app_link;
// [RLVa:KB] - Checked: 2010-11-02 (RLVa-1.2.2a) | Modified: RLVa-1.2.2a
	const bool creators_identical = LLSelectMgr::getInstance()->selectGetCreator(mCreatorID, creator_app_link);
	std::string owner_app_link;
	const bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_app_link);
// [/RLVa:KB]

	LLAvatarName av_name;
// [RLVa:KB] - Checked: RLVa-2.0.1
	// Only anonymize the creator if all of the selection was created by the same avie who's also the owner or they're a nearby avie
	if ( (RlvActions::isRlvEnabled()) && (creators_identical) && (!RlvActions::canShowName(RlvActions::SNC_DEFAULT, mCreatorID)) && ( (mCreatorID == mOwnerID) || (RlvUtil::isNearbyAgent(mCreatorID))) )
	{
		creator_app_link = LLSLURL("agent", mCreatorID, "rlvanonym").getSLURLString();
	}
	mLabelCreatorName->setText(creator_app_link);
// [/RLVa:KB]
	mLabelCreatorName->setEnabled(TRUE);

	// Update owner text field
	mOwner->setEnabled(TRUE);

//	std::string owner_app_link;
//	const BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_app_link);


	LLUUID owner_id = mOwnerID;
	if (select_mgr->selectIsGroupOwned())
	{
		// Group owned already displayed by selectGetOwner
// [RLVa:KB] - Checked: RLVa-2.0.1
		mLabelOwnerName->setValue(owner_app_link);
// [/RLVa:KB]
	}
	else
	{
		if (owner_id.isNull())
		{
			// Display last owner if public
			std::string last_owner_app_link;
			select_mgr->selectGetLastOwner(mLastOwnerID, last_owner_app_link);

			owner_id = mLastOwnerID;
		}
	}

	mLabelOwnerName->setEnabled(TRUE);
// [RLVa:KB] - Moved further down to avoid an annoying flicker when the text is set twice in a row

// [RLVa:KB] - Checked: RLVa-2.0.1
	if ( (RlvActions::isRlvEnabled()) && (!RlvActions::canShowName(RlvActions::SNC_DEFAULT)) )
	{
		// Only anonymize the creator if all of the selection was created by the same avie who's also the owner or they're a nearby avie
		if ( (creators_identical) && (!RlvActions::canShowName(RlvActions::SNC_DEFAULT, mCreatorID)) && ((mCreatorID == mOwnerID) || (RlvUtil::isNearbyAgent(mCreatorID))) )
			creator_app_link = LLSLURL("agent", mCreatorID, "rlvanonym").getSLURLString();

		// Only anonymize the owner name if all of the selection is owned by the same avie and isn't group owned
		if ( (owners_identical) && (!LLSelectMgr::getInstance()->selectIsGroupOwned()) && (!RlvActions::canShowName(RlvActions::SNC_DEFAULT, mOwnerID)) )
			owner_app_link = LLSLURL("agent", mOwnerID, "rlvanonym").getSLURLString();
	}

//	//BD - SSFUI
	std::string creator_slurl = LLSLURL("agent", mCreatorID, "inspect").getSLURLString();
	mLabelCreatorName->setValue(creator_slurl);
	mLabelCreatorName->setEnabled(TRUE);

//	//BD - SSFUI
	std::string owner_slurl = LLSLURL("agent", owner_id, "inspect").getSLURLString();
	mLabelOwnerName->setValue(owner_slurl);
// [/RLVa:KB]

//	style_params.link_href = owner_app_link;
// [RLVa:KB] - Checked: RLVa-2.0.1
	if ( (RlvActions::isRlvEnabled()) && (owners_identical) && (!RlvActions::canShowName(RlvActions::SNC_DEFAULT, mOwnerID)) )
	{
		owner_app_link = LLSLURL("agent", mOwnerID, "rlvanonym").getSLURLString();
	}
	mLabelOwnerName->setText(owner_app_link);

	// update group text field
	mGroup->setEnabled(TRUE);
	LLUUID group_id;
	BOOL groups_identical = select_mgr->selectGetGroup(group_id);
//	//BD - SSFUI
	std::string group_slurl = LLSLURL("group", group_id, "inspect").getSLURLString();
	if (groups_identical)
	{
		if (mGroupNameSLURL)
		{
			mGroupNameSLURL->setValue(group_slurl);
		}
	}
	else
	{
		if (mLabelGroupName)
		{
			mLabelGroupName->setNameID(LLUUID::null, TRUE);
			mLabelGroupName->refresh(LLUUID::null, std::string(), true);
		}
	}

	if(group_id.notNull())
	{
		mGroupNameSLURL->setVisible(TRUE);
		mLabelGroupName->setVisible(FALSE);
	}
	else
	{
		mGroupNameSLURL->setVisible(FALSE);
		mLabelGroupName->setVisible(TRUE);
	}

	mBtnSetGroup->setEnabled(root_selected && owners_identical && (mOwnerID == gAgent.getID()) && is_nonpermanent_enforced);

	mName->setEnabled(TRUE);
	mDescription->setEnabled(TRUE);
	

	if (is_one_object)
	{
		if (keyboard_focus_view != mLabelName)
		{
			mLabelName->setValue(nodep->mName);
		}

		if (mLabelDescription)
		{
			if (keyboard_focus_view != mLabelDescription)
			{
				mLabelDescription->setText(nodep->mDescription);
			}
		}
	}
	else
	{
		mLabelName->setValue(LLStringUtil::null);
		mLabelDescription->setText(LLStringUtil::null);
	}

	// figure out the contents of the name, description, & category
	BOOL edit_name_desc = FALSE;
	if (is_one_object && objectp->permModify() && !objectp->isPermanentEnforced())
	{
		edit_name_desc = TRUE;
	}
	if (edit_name_desc)
	{
		mLabelName->setEnabled(TRUE);
		mLabelDescription->setEnabled(TRUE);
	}
	else
	{
		mLabelName->setEnabled(FALSE);
		mLabelDescription->setEnabled(FALSE);
	}

	S32 total_sale_price = 0;
	S32 individual_sale_price = 0;
	BOOL is_for_sale_mixed = FALSE;
	BOOL is_sale_price_mixed = FALSE;
	U32 num_for_sale = FALSE;
    select_mgr->selectGetAggregateSaleInfo(num_for_sale,
											is_for_sale_mixed,
											is_sale_price_mixed,
											total_sale_price,
											individual_sale_price);

	const BOOL self_owned = (gAgent.getID() == mOwnerID);
	const BOOL group_owned = select_mgr->selectIsGroupOwned() ;
	const BOOL public_owned = (mOwnerID.isNull() && !select_mgr->selectIsGroupOwned());
	const BOOL can_transfer = select_mgr->selectGetRootsTransfer();
	const BOOL can_copy = select_mgr->selectGetRootsCopy();

	if (!owners_identical)
	{
		mCost->setEnabled(FALSE);
		mEditCost->setValue(LLStringUtil::null);
		mEditCost->setEnabled(FALSE);
	}
	// You own these objects.
	else if (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		// If there are multiple items for sale then set text to PRICE PER UNIT.
		if (num_for_sale > 1)
		{
			mCost->setValue(getString("Cost Per Unit"));
		}
		else
		{
			mCost->setValue(getString("Cost Default"));
		}
		
		if (!mEditCost->hasFocus())
		{
			// If the sale price is mixed then set the cost to MIXED, otherwise
			// set to the actual cost.
			if ((num_for_sale > 0) && is_for_sale_mixed)
			{
				mEditCost->setTentative(TRUE);
			}
			else if ((num_for_sale > 0) && is_sale_price_mixed)
			{
				mEditCost->setTentative(TRUE);
			}
			else 
			{
				mEditCost->setValue(individual_sale_price);
			}
		}
		// The edit fields are only enabled if you can sell this object
		// and the sale price is not mixed.
		BOOL enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : FALSE;
		mCost->setEnabled(enable_edit);
		mEditCost->setEnabled(enable_edit);
	}
	// Someone, not you, owns these objects.
	else if (!public_owned)
	{
		mCost->setEnabled(FALSE);
		mEditCost->setEnabled(FALSE);
		
		// Don't show a price if none of the items are for sale.
		if (num_for_sale)
			mEditCost->setValue(llformat("%d", total_sale_price));
		else
			mEditCost->setValue(LLStringUtil::null);

		// If multiple items are for sale, set text to TOTAL PRICE.
		if (num_for_sale > 1)
			mCost->setValue(getString("Cost Total"));
		else
			mCost->setValue(getString("Cost Default"));
	}
	// This is a public object.
	else
	{
		mCost->setEnabled(FALSE);
		mCost->setValue(getString("Cost Default"));
		
		mEditCost->setValue(LLStringUtil::null);
		mEditCost->setEnabled(FALSE);
	}

	// Enable and disable the permissions checkboxes
	// based on who owns the object.
	// TODO: Creator permissions

	U32 base_mask_on 			= 0;
	U32 base_mask_off		 	= 0;
	U32 owner_mask_off			= 0;
	U32 owner_mask_on 			= 0;
	U32 group_mask_on 			= 0;
	U32 group_mask_off 			= 0;
	U32 everyone_mask_on 		= 0;
	U32 everyone_mask_off 		= 0;
	U32 next_owner_mask_on 		= 0;
	U32 next_owner_mask_off		= 0;

	BOOL valid_base_perms 		= select_mgr->selectGetPerm(PERM_BASE,
															&base_mask_on,
															&base_mask_off);
	//BOOL valid_owner_perms =//
	select_mgr->selectGetPerm(PERM_OWNER,
															&owner_mask_on,
															&owner_mask_off);
	BOOL valid_group_perms 		= select_mgr->selectGetPerm(PERM_GROUP,
															&group_mask_on,
															&group_mask_off);
	
	BOOL valid_everyone_perms 	= select_mgr->selectGetPerm(PERM_EVERYONE,
															&everyone_mask_on,
															&everyone_mask_off);
	
	BOOL valid_next_perms 		= select_mgr->selectGetPerm(PERM_NEXT_OWNER,
															&next_owner_mask_on,
															&next_owner_mask_off);


	if (gSavedSettings.getBOOL("DebugPermissions") )
	{
		if (valid_base_perms)
		{
			getChild<LLUICtrl>("B:")->setValue("B: " + mask_to_string(base_mask_on));
			getChildView("B:")->setVisible(TRUE);
			getChild<LLUICtrl>("O:")->setValue("O: " + mask_to_string(owner_mask_on));
			getChildView("O:")->setVisible(TRUE);
			getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(group_mask_on));
			getChildView("G:")->setVisible(TRUE);
			getChild<LLUICtrl>("E:")->setValue("E: " + mask_to_string(everyone_mask_on));
			getChildView("E:")->setVisible(TRUE);
			getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(next_owner_mask_on));
			getChildView("N:")->setVisible(TRUE);
		}
		else if(!root_selected)
		{
			if(object_count == 1)
			{
				LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
				if (node && node->mValid)
				{
					getChild<LLUICtrl>("B:")->setValue("B: " + mask_to_string( node->mPermissions->getMaskBase()));
					getChildView("B:")->setVisible(TRUE);
					getChild<LLUICtrl>("O:")->setValue("O: " + mask_to_string(node->mPermissions->getMaskOwner()));
					getChildView("O:")->setVisible(TRUE);
					getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(node->mPermissions->getMaskGroup()));
					getChildView("G:")->setVisible(TRUE);
					getChild<LLUICtrl>("E:")->setValue("E: " + mask_to_string(node->mPermissions->getMaskEveryone()));
					getChildView("E:")->setVisible(TRUE);
					getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(node->mPermissions->getMaskNextOwner()));
					getChildView("N:")->setVisible(TRUE);
				}
			}
		}
		else
		{
		    getChildView("B:")->setVisible(FALSE);
		    getChildView("O:")->setVisible(FALSE);
		    getChildView("G:")->setVisible(FALSE);
		    getChildView("E:")->setVisible(FALSE);
		    getChildView("N:")->setVisible(FALSE);
		}

		U32 flag_mask = 0x0;
		if (objectp->permMove()) 		flag_mask |= PERM_MOVE;
		if (objectp->permModify()) 		flag_mask |= PERM_MODIFY;
		if (objectp->permCopy()) 		flag_mask |= PERM_COPY;
		if (objectp->permTransfer()) 	flag_mask |= PERM_TRANSFER;

		getChild<LLUICtrl>("F:")->setValue("F: " + mask_to_string(flag_mask));
		getChildView("F:")->setVisible(								TRUE);
	}
	else
	{
		mB->setVisible(FALSE);
		mO->setVisible(FALSE);
		mG->setVisible(FALSE);
		mE->setVisible(FALSE);
		mN->setVisible(FALSE);
		mF->setVisible(FALSE);
	}

	BOOL has_change_perm_ability = FALSE;
	BOOL has_change_sale_ability = FALSE;

	if (valid_base_perms && is_nonpermanent_enforced &&
		(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = TRUE;
	}
	if (valid_base_perms && is_nonpermanent_enforced &&
	   (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_SET_SALE))))
	{
		has_change_sale_ability = TRUE;
	}

	if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
	{
		// ...must select root to choose permissions
		mPermModify->setValue(getString("text modify warning"));
	}

	if (has_change_perm_ability)
	{
		mCheckboxGroupShare->setEnabled(TRUE);
		mCheckboxEveryoneMove->setEnabled(owner_mask_on & PERM_MOVE);
		mCheckboxEveryoneCopy->setEnabled(owner_mask_on & PERM_COPY && owner_mask_on & PERM_TRANSFER);
	}
	else
	{
		mCheckboxGroupShare->setEnabled(FALSE);
		mCheckboxEveryoneMove->setEnabled(FALSE);
		mCheckboxEveryoneCopy->setEnabled(FALSE);
	}

	if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
	{
		mCheckboxForSale->setEnabled(can_transfer || (!can_transfer && num_for_sale));
		mCheckboxForSale->setTentative(is_for_sale_mixed);
		mSaleType->setEnabled(num_for_sale && can_transfer && !is_sale_price_mixed);

		mNextOwner->setEnabled(TRUE);
		mCheckboxNextOwnerModify->setEnabled(base_mask_on & PERM_MODIFY);
		mCheckboxNextOwnerCopy->setEnabled(base_mask_on & PERM_COPY);
		mCheckboxNextOwnerTransfer->setEnabled(next_owner_mask_on & PERM_COPY);
	}
	else 
	{
		mCheckboxForSale->setEnabled(FALSE);
		mSaleType->setEnabled(FALSE);

		mNextOwner->setEnabled(FALSE);
		mCheckboxNextOwnerModify->setEnabled(FALSE);
		mCheckboxNextOwnerCopy->setEnabled(FALSE);
		mCheckboxNextOwnerTransfer->setEnabled(FALSE);
	}

	if (valid_group_perms)
	{
		if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
		{
			mCheckboxGroupShare->setValue(TRUE);
			mCheckboxGroupShare->setTentative(FALSE);
			mBtnDeedToGroup->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
		else if ((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
		{
			mCheckboxGroupShare->setValue(FALSE);
			mCheckboxGroupShare->setTentative(FALSE);
			mBtnDeedToGroup->setEnabled(FALSE);
		}
		else
		{
			mCheckboxGroupShare->setValue(TRUE);
			mCheckboxGroupShare->setTentative(!has_change_perm_ability);
			mBtnDeedToGroup->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
	}			

	if (valid_everyone_perms)
	{
		// Move
		if (everyone_mask_on & PERM_MOVE)
		{
			mCheckboxEveryoneMove->setValue(TRUE);
			mCheckboxEveryoneMove->setTentative(FALSE);
		}
		else if (everyone_mask_off & PERM_MOVE)
		{
			mCheckboxEveryoneMove->setValue(FALSE);
			mCheckboxEveryoneMove->setTentative(FALSE);
		}
		else
		{
			mCheckboxEveryoneMove->setValue(TRUE);
			mCheckboxEveryoneMove->setTentative(TRUE);
		}

		// Copy == everyone can't copy
		if (everyone_mask_on & PERM_COPY)
		{
			mCheckboxEveryoneCopy->setValue(TRUE);
			mCheckboxEveryoneCopy->setTentative(!can_copy || !can_transfer);
		}
		else if (everyone_mask_off & PERM_COPY)
		{
			mCheckboxEveryoneCopy->setValue(FALSE);
			mCheckboxEveryoneCopy->setTentative(FALSE);
		}
		else
		{
			mCheckboxEveryoneCopy->setValue(TRUE);
			mCheckboxEveryoneCopy->setTentative(TRUE);
		}
	}

	if (valid_next_perms)
	{
		// Modify == next owner canot modify
		if (next_owner_mask_on & PERM_MODIFY)
		{
			mNextOwnerModify->setValue(TRUE);
			mNextOwnerModify->setTentative(FALSE);
		}
		else if (next_owner_mask_off & PERM_MODIFY)
		{
			mNextOwnerModify->setValue(FALSE);
			mNextOwnerModify->setTentative(FALSE);
		}
		else
		{
			mNextOwnerModify->setValue(TRUE);
			mNextOwnerModify->setTentative(TRUE);
		}

		// Copy == next owner cannot copy
		if (next_owner_mask_on & PERM_COPY)
		{		
			mNextOwnerCopy->setValue(TRUE);
			mNextOwnerCopy->setTentative(!can_copy);
		}
		else if (next_owner_mask_off & PERM_COPY)
		{
			mNextOwnerCopy->setValue(FALSE);
			mNextOwnerCopy->setTentative(FALSE);
		}
		else
		{
			mNextOwnerCopy->setValue(TRUE);
			mNextOwnerCopy->setTentative(TRUE);
		}

		// Transfer == next owner cannot transfer
		if (next_owner_mask_on & PERM_TRANSFER)
		{
			mNextOwnerTransfer->setValue(TRUE);
			mNextOwnerTransfer->setTentative(!can_transfer);
		}
		else if (next_owner_mask_off & PERM_TRANSFER)
		{
			mNextOwnerTransfer->setValue(FALSE);
			mNextOwnerTransfer->setTentative(FALSE);
		}
		else
		{
			mNextOwnerTransfer->setValue(TRUE);
			mNextOwnerTransfer->setTentative(TRUE);
		}
	}

	// reflect sale information
	LLSaleInfo sale_info;
	BOOL valid_sale_info = select_mgr->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();


	//LLComboBox* combo_sale_type = getChild<LLComboBox>("sale type");
	if (valid_sale_info)
	{
		mSaleType->setValue(sale_type == LLSaleInfo::FS_NOT ? LLSaleInfo::FS_COPY : sale_type);
		mSaleType->setTentative(FALSE); // unfortunately this doesn't do anything at the moment.
	}
	else
	{
		// default option is sell copy, determined to be safest
		mSaleType->setValue(LLSaleInfo::FS_COPY);
		mSaleType->setTentative(TRUE); // unfortunately this doesn't do anything at the moment.
	}

	mCheckboxForSale->setValue((num_for_sale != 0));
	//getChild<LLUICtrl>("checkbox for sale")->setValue((num_for_sale != 0));

	// HACK: There are some old objects in world that are set for sale,
	// but are no-transfer.  We need to let users turn for-sale off, but only
	// if for-sale is set.
	bool cannot_actually_sell = !can_transfer || (!can_copy && sale_type == LLSaleInfo::FS_COPY);
	if (cannot_actually_sell)
	{
		if (num_for_sale && has_change_sale_ability)
		{
			mCheckboxForSale->setEnabled(true);
		}
	}
	
	// Check search status of objects
	const BOOL all_volume = select_mgr->selectionAllPCode( LL_PCODE_VOLUME );
	bool include_in_search;
	const BOOL all_include_in_search = select_mgr->selectionGetIncludeInSearch(&include_in_search);
	mCheckboxSearch->setEnabled(has_change_sale_ability && all_volume);
	mCheckboxSearch->setValue(include_in_search);
	mCheckboxSearch->setTentative(!all_include_in_search);

	// Click action (touch, sit, buy)
	U8 click_action = 0;
	if (select_mgr->selectionGetClickAction(&click_action))
	{
		const std::string combo_value = click_action_to_string_value(click_action);
		mClickAction->setValue(LLSD(combo_value));
	}

	if(selection->isAttachment())
	{
		mCheckboxForSale->setEnabled(FALSE);
		mEditCost->setEnabled(FALSE);
		mSaleType->setEnabled(FALSE);
	}

	mLabelClickAction->setEnabled(is_perm_modify && is_nonpermanent_enforced  && all_volume);
	mClickAction->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);
}

void LLPanelPermissions::updateOwnerName(const LLUUID& owner_id, const LLAvatarName& owner_name, const LLStyle::Params& style_params)
{
	if (mOwnerCacheConnection.connected())
	{
		mOwnerCacheConnection.disconnect();
	}
	mLabelOwnerName->setText(owner_name.getCompleteName(), style_params);
}

void LLPanelPermissions::updateCreatorName(const LLUUID& creator_id, const LLAvatarName& creator_name, const LLStyle::Params& style_params)
{
	if (mCreatorCacheConnection.connected())
	{
		mCreatorCacheConnection.disconnect();
	}
	mLabelCreatorName->setText(creator_name.getCompleteName(), style_params);
}

// static
void LLPanelPermissions::onClickClaim()
{
	// try to claim ownership
	LLSelectMgr::getInstance()->sendOwner(gAgent.getID(), gAgent.getGroupID());
}

// static
void LLPanelPermissions::onClickRelease()
{
	// try to release ownership
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null);
}

void LLPanelPermissions::onClickGroup()
{
	LLUUID owner_id;
	std::string name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
	LLFloater* parent_floater = gFloaterView->getParentFloater(this);

	if(owners_identical && (owner_id == gAgent.getID()))
	{
		LLFloaterGroupPicker* fg = 	LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
		if (fg)
		{
			fg->setSelectGroupCallback( boost::bind(&LLPanelPermissions::cbGroupID, this, _1) );

			if (parent_floater)
			{
				LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
				fg->setOrigin(new_rect.mLeft, new_rect.mBottom);
				parent_floater->addDependentFloater(fg);
			}
		}
	}
}

void LLPanelPermissions::cbGroupID(LLUUID group_id)
{
	if(mLabelGroupName)
	{
		mLabelGroupName->setNameID(group_id, TRUE);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLUUID group_id;
		BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if(group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
		}
	}
	return false;
}

void LLPanelPermissions::onClickDeedToGroup()
{
	LLNotificationsUtil::add( "DeedObjectToGroup", LLSD(), LLSD(), callback_deed_to_group);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

// static
void LLPanelPermissions::onCommitPerm(LLUICtrl *ctrl, U8 field, U32 perm)
{
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	if(!object) return;

	// Checkbox will have toggled itself
	// LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	BOOL new_state = check->get();
	
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);
}

// static
void LLPanelPermissions::onCommitGroupShare(LLUICtrl *ctrl)
{
	onCommitPerm(ctrl, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}

// static
void LLPanelPermissions::onCommitEveryoneMove(LLUICtrl *ctrl)
{
	onCommitPerm(ctrl, PERM_EVERYONE, PERM_MOVE);
}


// static
void LLPanelPermissions::onCommitEveryoneCopy(LLUICtrl *ctrl)
{
	onCommitPerm(ctrl, PERM_EVERYONE, PERM_COPY);
}

// static
void LLPanelPermissions::onCommitNextOwnerModify(LLUICtrl* ctrl)
{
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerModify" << LL_ENDL;
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_MODIFY);
}

// static
void LLPanelPermissions::onCommitNextOwnerCopy(LLUICtrl* ctrl)
{
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerCopy" << LL_ENDL;
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_COPY);
}

// static
void LLPanelPermissions::onCommitNextOwnerTransfer(LLUICtrl* ctrl)
{
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerTransfer" << LL_ENDL;
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_TRANSFER);
}

// static
void LLPanelPermissions::onCommitName(LLUICtrl*)
{
	LLSelectMgr::getInstance()->selectionSetObjectName(mObjectName->getText());
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->isAttachment() && (selection->getNumNodes() == 1) && !mObjectName->getText().empty())
	{
		LLUUID object_id = selection->getFirstObject()->getAttachmentItemID();
		LLViewerInventoryItem* item = findItem(object_id);
		if (item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			new_item->rename(mObjectName->getText());
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
	}
}


// static
void LLPanelPermissions::onCommitDesc(LLUICtrl*)
{
	LLSelectMgr::getInstance()->selectionSetObjectDescription(mObjectDescription->getText());
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->isAttachment() && (selection->getNumNodes() == 1))
	{
		LLUUID object_id = selection->getFirstObject()->getAttachmentItemID();
		LLViewerInventoryItem* item = findItem(object_id);
		if (item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			new_item->setDescription(mObjectDescription->getText());
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
	}
}

// static
void LLPanelPermissions::onCommitSaleInfo(LLUICtrl*)
{
	setAllSaleInfo();
}

// static
void LLPanelPermissions::onCommitSaleType(LLUICtrl*)
{
	setAllSaleInfo();
}

void LLPanelPermissions::setAllSaleInfo()
{
	LL_INFOS() << "LLPanelPermissions::setAllSaleInfo()" << LL_ENDL;
	LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_NOT;

	LLCheckBoxCtrl *checkPurchase = getChild<LLCheckBoxCtrl>("checkbox for sale");
	
	// Set the sale type if the object(s) are for sale.
	if(checkPurchase && checkPurchase->get())
	{
		sale_type = static_cast<LLSaleInfo::EForSale>(getChild<LLComboBox>("sale type")->getValue().asInteger());
	}

	S32 price = -1;
	
	LLSpinCtrl *edit_price = getChild<LLSpinCtrl>("Edit Cost");
	price = (edit_price->getTentative()) ? DEFAULT_PRICE : edit_price->getValue().asInteger();

	// If somehow an invalid price, turn the sale off.
	if (price < 0)
		sale_type = LLSaleInfo::FS_NOT;

	LLSaleInfo old_sale_info;
	LLSelectMgr::getInstance()->selectGetSaleInfo(old_sale_info);

	LLSaleInfo new_sale_info(sale_type, price);
	LLSelectMgr::getInstance()->selectionSetObjectSaleInfo(new_sale_info);

    // Note: won't work right if a root and non-root are both single-selected (here and other places).
    BOOL is_perm_modify = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
                           && LLSelectMgr::getInstance()->selectGetRootsModify())
                          || LLSelectMgr::getInstance()->selectGetModify();
    BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
                                     && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
                                    || LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();

    if (is_perm_modify && is_nonpermanent_enforced)
    {
        struct f : public LLSelectedObjectFunctor
        {
            virtual bool apply(LLViewerObject* object)
            {
                return object->getClickAction() == CLICK_ACTION_BUY
                    || object->getClickAction() == CLICK_ACTION_TOUCH;
            }
        } check_actions;

        // Selection should only contain objects that are of target
        // action already or of action we are aiming to remove.
        bool default_actions = LLSelectMgr::getInstance()->getSelection()->applyToObjects(&check_actions);

        if (default_actions && old_sale_info.isForSale() != new_sale_info.isForSale())
        {
            U8 new_click_action = new_sale_info.isForSale() ? CLICK_ACTION_BUY : CLICK_ACTION_TOUCH;
            LLSelectMgr::getInstance()->selectionSetClickAction(new_click_action);
        }
    }
}

struct LLSelectionPayable : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* obj)
	{
		// can pay if you or your parent has money() event in script
		LLViewerObject* parent = (LLViewerObject*)obj->getParent();
		return (obj->flagTakesMoney() 
			   || (parent && parent->flagTakesMoney()));
	}
};

// static
void LLPanelPermissions::onCommitClickAction(LLUICtrl* ctrl)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if (!box) return;
	std::string value = box->getValue().asString();
	U8 click_action = string_value_to_click_action(value);
	
	if (click_action == CLICK_ACTION_BUY)
	{
		LLSaleInfo sale_info;
		LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
		if (!sale_info.isForSale())
		{
			LLNotificationsUtil::add("CantSetBuyObject");

			// Set click action back to its old value
			U8 click_action = 0;
			LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
			std::string item_value = click_action_to_string_value(click_action);
			box->setValue(LLSD(item_value));
			return;
		}
	}
	else if (click_action == CLICK_ACTION_PAY)
	{
		// Verify object has script with money() handler
		LLSelectionPayable payable;
		bool can_pay = LLSelectMgr::getInstance()->getSelection()->applyToObjects(&payable);
		if (!can_pay)
		{
			// Warn, but do it anyway.
			LLNotificationsUtil::add("ClickActionNotPayable");
		}
	}
	LLSelectMgr::getInstance()->selectionSetClickAction(click_action);
}

// static
void LLPanelPermissions::onCommitIncludeInSearch(LLUICtrl* ctrl)
{
	LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
	llassert(box);

	LLSelectMgr::getInstance()->selectionSetIncludeInSearch(box->get());
}


LLViewerInventoryItem* LLPanelPermissions::findItem(LLUUID &object_id)
{
	if (!object_id.isNull())
	{
		return gInventory.getItem(object_id);
	}
	return NULL;
}
