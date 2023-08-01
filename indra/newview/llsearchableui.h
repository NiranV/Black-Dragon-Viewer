/**
* @file llsearchableui.h
*
* $LicenseInfo:firstyear=2019&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_SEARCHABLE_UI_H
#define LL_SEARCHABLE_UI_H

#include "llsearchablecontrol.h"
#include "llmenugl.h"
#include "llview.h"

class LLMenuItemGL;
class LLView;
class LLPanel;
class LLTabContainer;

class LLSearchableItem;
class LLSearchableEntry;


class LLSearchableUI : 
	public LLSearchableControl
{
public:
	LLSearchableUI();
	/*virtual*/ ~LLSearchableUI();

	struct LLPanelData;
	struct LLTabContainerData;

	typedef boost::shared_ptr<LLSearchableItem> LLSearchableItemPtr;
	typedef boost::shared_ptr<LLSearchableEntry> LLSearchableEntryPtr;
	typedef boost::shared_ptr<LLPanelData> LLPanelDataPtr;
	typedef boost::shared_ptr<LLTabContainerData> LLTabContainerDataPtr;

	typedef std::vector<LLTabContainerData> tTabContainerDataList;
	typedef std::vector<LLSearchableItemPtr> tSearchableItemList;
	typedef std::vector<LLSearchableEntryPtr> tSearchableEntryList;
	typedef std::vector<LLPanelDataPtr> tPanelDataList;

	struct LLPanelData
	{
		LLPanel const *mPanel;
		std::string mLabel;

		std::vector<boost::shared_ptr<LLSearchableItem>> mChildren;
		std::vector<boost::shared_ptr<LLPanelData>> mChildPanel;

		virtual ~LLPanelData();

		void setNotHighlighted();
		virtual bool hightlightAndHide(LLWString const &aFilter);
	};

	struct LLTabContainerData : public LLPanelData
	{
		LLTabContainer *mTabContainer;
		virtual bool hightlightAndHide(LLWString const &aFilter);
	};

	struct LLTabData
	{
		LLTabContainerDataPtr mRootTab;
		LLWString mLastFilter;
	};

	struct LLMenuData
	{
		LLSearchableEntryPtr mRootMenu;
		LLWString mLastFilter;
	};
};

class LLSearchableItem : public LLSearchableControl
{
public:
	LLSearchableItem();
	/*virtual*/ ~LLSearchableItem();

	LLWString mLabel;
	LLView const *mView;
	const LLSearchableControl *mCtrl;

	std::vector<boost::shared_ptr<LLSearchableItem>> mChildren;

	void setNotHighlighted();
	virtual bool hightlightAndHide(LLWString const &aFilter);

	virtual std::string _getSearchText() const
	{
		return 0;
	}

	virtual void onSetHighlight() const // When highlight, really do highlight the label
	{
	}
};

class LLSearchableEntry : public LLSearchableControl
{
public:
	LLSearchableEntry();
	/*virtual*/ ~LLSearchableEntry();

	LLWString mLabel;
	LLMenuItemGL *mMenu;
	LLSearchableUI::tSearchableEntryList mChildren;
	const LLSearchableControl *mCtrl;
	bool mWasHiddenBySearch;

	void setNotHighlighted();
	virtual bool hightlightAndHide(LLWString const &aFilter, bool hide = true);

	virtual std::string _getSearchText() const
	{
		return mMenu->getLabel() + mMenu->getToolTip();
	}

	virtual void onSetHighlight() const // When highlight, really do highlight the label
	{
		if (mMenu)
			mMenu->setHighlighted(getHighlighted());
	}
};

#endif
