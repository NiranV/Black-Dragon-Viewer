/** 
 * @file llurlmatch.cpp
 * @author Martin Reddy
 * @brief Specifies a matched Url in a string, as returned by LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llurlmatch.h"

LLUrlMatch::LLUrlMatch() :
	mStart(0),
	mEnd(0),
	mUrl(""),
	mLabel(""),
	mTooltip(""),
	mIcon(""),
	mMenuName(""),
	mLocation(""),
	mUnderlineOnHoverOnly(false)
{
}

void LLUrlMatch::setValues(U32 start, U32 end, const std::string &url,
						   const std::string &label, const std::string &tooltip,
						   const std::string &icon, const LLStyle::Params& style,
						   const std::string &menu, const std::string &location,
						   const LLUUID& id, bool underline_on_hover_only)
{
	mStart = start;
	mEnd = end;
	mUrl = url;
	mLabel = label;
	mTooltip = tooltip;
	mIcon = icon;
	mStyle = style;
	mStyle.link_href = url;
	mMenuName = menu;
	mLocation = location;
	mID = id;
	mUnderlineOnHoverOnly = underline_on_hover_only;
}
