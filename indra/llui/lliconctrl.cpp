/** 
 * @file lliconctrl.cpp
 * @brief LLIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lliconctrl.h"

// Linden library includes 

// Project includes
#include "llcontrol.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"

static LLDefaultChildRegistry::Register<LLIconCtrl> r("icon");

LLIconCtrl::Params::Params()
	: image("image_name"),
	color("color"),
	use_draw_context_alpha("use_draw_context_alpha", true),
	scale_image("scale_image"),
	min_width("min_width", 0),
	min_height("min_height", 0),
	//BD
	repeat_image("repeat_image", false),
	repeats("repeats", 1),
	warn_image("warn_image"),
	default_image("default_image"),
	warns_index("warns_index", -1),
	default_index("default_index", -1),
	selected("selected")
{}

LLIconCtrl::LLIconCtrl(const LLIconCtrl::Params& p)
	: LLUICtrl(p),
	mColor(p.color()),
	mImagep(p.image),
	mUseDrawContextAlpha(p.use_draw_context_alpha),
	mPriority(0),
	mMinWidth(p.min_width),
	mMinHeight(p.min_height),
	mMaxWidth(0),
	mMaxHeight(0),
	//BD
	mDefaultIndex(p.default_index),
	mWarnIndex(p.warns_index),
	mDefaultImagep(p.default_image),
	mWarnImagep(p.warn_image),
	mRepeats(p.repeats),
	mRepeatImage(p.repeat_image)
{
	if (mImagep.notNull())
	{
		LLUICtrl::setValue(mImagep->getName());
	}

	if (mDefaultImagep.notNull())
	{
		LLSD tvalue(mDefaultImagep->getName());
		if (tvalue.isString() && LLUUID::validate(tvalue.asString()))
		{
			//RN: support UUIDs masquerading as strings
			tvalue = LLSD(LLUUID(tvalue.asString()));
		}
		if (tvalue.isUUID())
		{
			mDefaultImagep = LLUI::getUIImageByID(tvalue.asUUID(), mPriority);
		}
		else
		{
			mDefaultImagep = LLUI::getUIImage(tvalue.asString(), mPriority);
		}

		if (mDefaultImagep.notNull()
			&& mDefaultImagep->getImage().notNull()
			&& mMinWidth
			&& mMinHeight)
		{
			S32 desired_draw_width = llmax(mMinWidth, mDefaultImagep->getWidth());
			S32 desired_draw_height = llmax(mMinHeight, mDefaultImagep->getHeight());
			if (mMaxWidth && mMaxHeight)
			{
				desired_draw_width = llmin(desired_draw_width, mMaxWidth);
				desired_draw_height = llmin(desired_draw_height, mMaxHeight);
			}

			mDefaultImagep->getImage()->setKnownDrawSize(desired_draw_width, desired_draw_height);
		}
	}

	if (mWarnImagep.notNull())
	{
		LLSD tvalue(mWarnImagep->getName());
		if (tvalue.isString() && LLUUID::validate(tvalue.asString()))
		{
			//RN: support UUIDs masquerading as strings
			tvalue = LLSD(LLUUID(tvalue.asString()));
		}
		if (tvalue.isUUID())
		{
			mWarnImagep = LLUI::getUIImageByID(tvalue.asUUID(), mPriority);
		}
		else
		{
			mWarnImagep = LLUI::getUIImage(tvalue.asString(), mPriority);
		}

		if (mWarnImagep.notNull()
			&& mWarnImagep->getImage().notNull()
			&& mMinWidth
			&& mMinHeight)
		{
			S32 desired_draw_width = llmax(mMinWidth, mWarnImagep->getWidth());
			S32 desired_draw_height = llmax(mMinHeight, mWarnImagep->getHeight());
			if (mMaxWidth && mMaxHeight)
			{
				desired_draw_width = llmin(desired_draw_width, mMaxWidth);
				desired_draw_height = llmin(desired_draw_height, mMaxHeight);
			}

			mWarnImagep->getImage()->setKnownDrawSize(desired_draw_width, desired_draw_height);
		}
	}
}

LLIconCtrl::~LLIconCtrl()
{
	mImagep = NULL;
}


void LLIconCtrl::draw()
{
	if( mImagep.notNull() )
	{
		F32 alpha = mUseDrawContextAlpha ? getDrawContext().mAlpha : getCurrentTransparency();
		LLRect local_rect = getLocalRect();
		if (mRepeatImage)
		{
			S32 available_width = getRect().getWidth();
			S32 leftover_width = available_width % mRepeats;
			//S32 repeat_width = local_rect.getWidth() / mRepeats;
			S32 last_left = 0;

			F32 new_alpha = alpha * 0.6;
			S32 i = 0;
			while (i < mRepeats)
			{
				S32 consistent_width = llfloor(available_width / mRepeats);
				if (leftover_width > 0)
				{
					++consistent_width;
					--leftover_width;
				}

				LLRect new_rect = local_rect;
				new_rect.mLeft = last_left;
				new_rect.mRight = new_rect.mLeft + consistent_width - 1;
				last_left += consistent_width;
				if (i == mDefaultIndex)
					mDefaultImagep->draw(new_rect, mColor.get() % (i != mSelected ? new_alpha : alpha));
				else if(i >= mWarnIndex)
					mWarnImagep->draw(new_rect, mColor.get() % (i != mSelected ? new_alpha : alpha));
				else
					mImagep->draw(new_rect, mColor.get() % (i != mSelected ? new_alpha : alpha));
				++i;
			}
		}
		else
		{
			mImagep->draw(local_rect, mColor.get() % alpha);
		}
	}

	LLUICtrl::draw();
}

// virtual
// value might be a string or a UUID
void LLIconCtrl::setValue(const LLSD& value)
{
    setValue(value, mPriority);
}

void LLIconCtrl::setValue(const LLSD& value, S32 priority)
{
	LLSD tvalue(value);
	if (value.isString() && LLUUID::validate(value.asString()))
	{
		//RN: support UUIDs masquerading as strings
		tvalue = LLSD(LLUUID(value.asString()));
	}
	LLUICtrl::setValue(tvalue);
	if (tvalue.isUUID())
	{
        mImagep = LLUI::getUIImageByID(tvalue.asUUID(), priority);
	}
	else
	{
        mImagep = LLUI::getUIImage(tvalue.asString(), priority);
	}

	if(mImagep.notNull() 
		&& mImagep->getImage().notNull() 
		&& mMinWidth 
		&& mMinHeight)
	{
        S32 desired_draw_width = llmax(mMinWidth, mImagep->getWidth());
        S32 desired_draw_height = llmax(mMinHeight, mImagep->getHeight());
        if (mMaxWidth && mMaxHeight)
        {
            desired_draw_width = llmin(desired_draw_width, mMaxWidth);
            desired_draw_height = llmin(desired_draw_height, mMaxHeight);
        }

        mImagep->getImage()->setKnownDrawSize(desired_draw_width, desired_draw_height);
	}
}

std::string LLIconCtrl::getImageName() const
{
	if (getValue().isString())
		return getValue().asString();
	else
		return std::string();
}


