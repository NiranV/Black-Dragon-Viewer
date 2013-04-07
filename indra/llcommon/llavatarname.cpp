/** 
 * @file llavatarname.cpp
 * @brief Represents name-related data for an avatar, such as the
 * username/SLID ("bobsmith123" or "james.linden") and the display
 * name ("James Cook")
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "linden_common.h"

#include "llavatarname.h"

#include "lldate.h"
#include "llsd.h"

// Store these in pre-built std::strings to avoid memory allocations in
// LLSD map lookups
static const std::string USERNAME("username");
static const std::string DISPLAY_NAME("display_name");
static const std::string LEGACY_FIRST_NAME("legacy_first_name");
static const std::string LEGACY_LAST_NAME("legacy_last_name");
static const std::string IS_DISPLAY_NAME_DEFAULT("is_display_name_default");
static const std::string DISPLAY_NAME_EXPIRES("display_name_expires");
static const std::string DISPLAY_NAME_NEXT_UPDATE("display_name_next_update");

// [SL:KB] - Patch: DisplayNames-AgentLinkShowUsernames | Checked: 2011-04-17 (Catznip-3.0.0a) | Modified: Catznip-2.6.0a
LLAvatarName::EShowUsername LLAvatarName::s_eShowUsername = LLAvatarName::SHOW_ALWAYS;
// [/SL:KB]

LLAvatarName::LLAvatarName()
:	mUsername(),
	mDisplayName(),
	mLegacyFirstName(),
	mLegacyLastName(),
	mIsDisplayNameDefault(false),
	mIsTemporaryName(false),
	mExpires(F64_MAX),
	mNextUpdate(0.0)
{ }

bool LLAvatarName::operator<(const LLAvatarName& rhs) const
{
	if (mUsername == rhs.mUsername)
		return mDisplayName < rhs.mDisplayName;
	else
		return mUsername < rhs.mUsername;
}

LLSD LLAvatarName::asLLSD() const
{
	LLSD sd;
	sd[USERNAME] = mUsername;
	sd[DISPLAY_NAME] = mDisplayName;
	sd[LEGACY_FIRST_NAME] = mLegacyFirstName;
	sd[LEGACY_LAST_NAME] = mLegacyLastName;
	sd[IS_DISPLAY_NAME_DEFAULT] = mIsDisplayNameDefault;
	sd[DISPLAY_NAME_EXPIRES] = LLDate(mExpires);
	sd[DISPLAY_NAME_NEXT_UPDATE] = LLDate(mNextUpdate);
	return sd;
}

void LLAvatarName::fromLLSD(const LLSD& sd)
{
	mUsername = sd[USERNAME].asString();
	mDisplayName = sd[DISPLAY_NAME].asString();
	mLegacyFirstName = sd[LEGACY_FIRST_NAME].asString();
	mLegacyLastName = sd[LEGACY_LAST_NAME].asString();
	mIsDisplayNameDefault = sd[IS_DISPLAY_NAME_DEFAULT].asBoolean();
	LLDate expires = sd[DISPLAY_NAME_EXPIRES];
	mExpires = expires.secondsSinceEpoch();
	LLDate next_update = sd[DISPLAY_NAME_NEXT_UPDATE];
	mNextUpdate = next_update.secondsSinceEpoch();
}

//std::string LLAvatarName::getCompleteName() const
// [SL:KB] - Patch: DisplayNames-AgentLinkShowUsernames | Checked: 2011-04-17 (Catznip-3.0.0a) | Modified: Catznip-2.6.0a
std::string LLAvatarName::getCompleteName(EShowUsername eShowUsername) const
// [/SL:KB]
{
	std::string name;
//	if (mUsername.empty() || mIsDisplayNameDefault)
	// If the display name feature is off
	// OR this particular display name is defaulted (i.e. based on user name),
	// then display only the easier to read instance of the person's name.
// [SL:KB] - Patch: DisplayNames-AgentLinkShowUsernames | Checked: 2011-04-17 (Catznip-3.0.0a) | Modified: Catznip-2.6.0a
	if ( (mUsername.empty()) || 
		 ((SHOW_NEVER == s_eShowUsername) || ((SHOW_MISMATCH == s_eShowUsername) && (mIsDisplayNameDefault))) )
// [/SL:KB]
	{
		name = mDisplayName;
	}
	else
	{
		name = mDisplayName + " (" + mUsername + ")";
	}
	return name;
}

std::string LLAvatarName::getLegacyName() const
{
	if (mLegacyFirstName.empty() && mLegacyLastName.empty()) // display names disabled?
	{
		return mDisplayName;
	}

	std::string name;
	name.reserve( mLegacyFirstName.size() + 1 + mLegacyLastName.size() );
	name = mLegacyFirstName;
	name += " ";
	name += mLegacyLastName;
	return name;
}
