/**
* @file llsearchablecontrol.h
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

#ifndef LL_SEARCHABLE_CONTROL_H
#define LL_SEARCHABLE_CONTROL_H

#include "lluicolortable.h"
#include "lluicolor.h"

class LLSearchableControl
{
	mutable bool mIsHighlighed;
public:
	LLSearchableControl();
	virtual ~LLSearchableControl();

	void setHighlighted(bool aVal) const;

	bool getHighlighted() const;

	LLColor4 getHighlightColor() const;

	std::string getSearchText() const;
protected:
	virtual std::string _getSearchText() const { return ""; }
	virtual void onSetHighlight() const {}
};


#endif