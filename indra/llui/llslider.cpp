/**
 * @file llslider.cpp
 * @brief LLSlider base class
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

#include "linden_common.h"

#include "llslider.h"
#include "llui.h"

#include "llgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"         // for the MASK constants
#include "llcontrol.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLSlider> r1("slider_bar");
//FIXME: make this into an unregistered template so that code constructed sliders don't
// have ambigious template lookup problem

LLSlider::Params::Params()
:	orientation ("orientation", std::string ("horizontal")),
	track_color("track_color"),
	thumb_outline_color("thumb_outline_color"),
	thumb_center_color("thumb_center_color"),
	thumb_image("thumb_image"),
	thumb_image_pressed("thumb_image_pressed"),
	thumb_image_disabled("thumb_image_disabled"),
	track_image_horizontal("track_image_horizontal"),
	track_image_vertical("track_image_vertical"),
	track_highlight_horizontal_image("track_highlight_horizontal_image"),
	track_highlight_vertical_image("track_highlight_vertical_image"),
//	//BD - Track Difference Visualization
	track_change_horizontal_image("track_change_horizontal_image"),
	track_change_vertical_image("track_change_vertical_image"),
	mouse_down_callback("mouse_down_callback"),
	mouse_up_callback("mouse_up_callback"),
	//BD - UI Improvements
	apply_immediately("apply_immediately", true)
{}

LLSlider::LLSlider(const LLSlider::Params& p)
:	LLF32UICtrl(p),
	mMouseOffset( 0 ),
	mOrientation ((p.orientation() == "horizontal") ? HORIZONTAL : VERTICAL),
	mTrackColor(p.track_color()),
	mThumbOutlineColor(p.thumb_outline_color()),
	mThumbCenterColor(p.thumb_center_color()),
	mThumbImage(p.thumb_image),
	mThumbImagePressed(p.thumb_image_pressed),
	mThumbImageDisabled(p.thumb_image_disabled),
	mTrackImageHorizontal(p.track_image_horizontal),
	mTrackImageVertical(p.track_image_vertical),
	mTrackHighlightHorizontalImage(p.track_highlight_horizontal_image),
	mTrackHighlightVerticalImage(p.track_highlight_vertical_image),
//	//BD - Track Difference Visualization
	mTrackChangeHorizontalImage(p.track_change_horizontal_image),
	mTrackChangeVerticalImage(p.track_change_vertical_image),
	mMouseDownSignal(NULL),
	mMouseUpSignal(NULL),
	//BD - UI Improvements
	mRightMousePressed(false),
	mApplyImmediately(p.apply_immediately)
{
    mViewModel->setValue(p.initial_value);
	updateThumbRect();
	mDragStartThumbRect = mThumbRect;
	setControlName(p.control_name, NULL);
	setValue(getValueF32());
	mOriginalValue = getValueF32();
	
	if (p.mouse_down_callback.isProvided())
	{
		setMouseDownCallback(initCommitCallback(p.mouse_down_callback));
	}
	if (p.mouse_up_callback.isProvided())
	{
		setMouseUpCallback(initCommitCallback(p.mouse_up_callback));
	}
}

LLSlider::~LLSlider()
{
    delete mMouseDownSignal;
    delete mMouseUpSignal;
}

//BD - UI Improvements
void LLSlider::setValue(F32 value, bool from_event, bool precision_override, bool overdrive)
{
	//BD - Allow overdriving sliders if numbers are entered directly.
	if(!overdrive)
		value = llclamp( value, mMinValue, mMaxValue );

	//BD - UI Improvements
	if (!precision_override)
	{
		// Round to nearest increment (bias towards rounding down)
		value -= mMinValue;
		value += mIncrement / 2.0001f;
		value -= fmod(value, mIncrement);
		value += mMinValue;
	}

    if (!from_event && getValueF32() != value)
    {
        setControlValue(value);
    }

    LLF32UICtrl::setValue(value);
    updateThumbRect();
}

void LLSlider::updateThumbRect()
{
	const S32 DEFAULT_THUMB_SIZE = 16;
	//BD - Prevent the slider display from clipping outside the slider width.
	F32 t = llclamp((getValueF32() - mMinValue) / (mMaxValue - mMinValue), 0.f, 1.f);

	S32 thumb_width = mThumbImage ? mThumbImage->getWidth() : DEFAULT_THUMB_SIZE;
	S32 thumb_height = mThumbImage ? mThumbImage->getHeight() : DEFAULT_THUMB_SIZE;

	if ( mOrientation == HORIZONTAL )
	{
		S32 left_edge = (thumb_width / 2);
		S32 right_edge = getRect().getWidth() - (thumb_width / 2);

		S32 x = left_edge + S32( t * (right_edge - left_edge) );
		mThumbRect.mLeft = x - (thumb_width / 2);
		mThumbRect.mRight = mThumbRect.mLeft + thumb_width;
		mThumbRect.mBottom = getLocalRect().getCenterY() - (thumb_height / 2);
		mThumbRect.mTop = mThumbRect.mBottom + thumb_height;
	}
	else
	{
		S32 top_edge = (thumb_height / 2);
		S32 bottom_edge = getRect().getHeight() - (thumb_height / 2);

		S32 y = top_edge + S32( t * (bottom_edge - top_edge) );
		mThumbRect.mLeft = getLocalRect().getCenterX() - (thumb_width / 2);
		mThumbRect.mRight = mThumbRect.mLeft + thumb_width;
		mThumbRect.mBottom = y  - (thumb_height / 2);
		mThumbRect.mTop = mThumbRect.mBottom + thumb_height;
	}
}


void LLSlider::setValueAndCommit(F32 value)
{
    F32 old_value = getValueF32();
    setValue(value);

	if (getValueF32() != old_value
		&& mApplyImmediately)
	{
		onCommit();
	}
}


void LLSlider::onMouseCaptureLost()
{
	//BD - UI Improvements
	setValueAndCommit(getValueF32());
	onCommit();
}

bool LLSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		LLVector2 delta = LLVector2((F32)x - mInitPos.mV[VX], (F32)y - mInitPos.mV[VY]);
		if (mask == MASK_CONTROL)
		{
			delta *= 0.1;
		}
		F32 t = 0;
		LLVector2 new_pos = LLVector2(mInitPos.mV[VX] + delta.mV[VX], mInitPos.mV[VY] + delta.mV[VY]);
		if ( mOrientation == HORIZONTAL )
		{
			S32 thumb_half_width = mThumbImage->getWidth()/2;
			//BD - UI Improvements
			S32 left_edge = thumb_half_width;
			S32 right_edge = getRect().getWidth() - (thumb_half_width);
			t = F32(new_pos.mV[VX] - left_edge) / (right_edge - left_edge);
		}
		else // mOrientation == VERTICAL
		{
			S32 thumb_half_height = mThumbImage->getHeight()/2;
			S32 top_edge = thumb_half_height;
			S32 bottom_edge = getRect().getHeight() - (thumb_half_height);
			t = F32(new_pos.mV[VY] - top_edge) / (bottom_edge - top_edge);
		}

		//BD - UI Improvements
		F32 val = llclamp(t * (mMaxValue - mMinValue) + mMinValue, mMinValue, mMaxValue);
		if ((mOrientation == HORIZONTAL && delta.mV[VX] != 0)
			|| mOrientation == VERTICAL && delta.mV[VY] != 0)
		{
			setValueAndCommit(val);
		}

		getWindow()->setCursor(UI_CURSOR_ARROW);
		// _LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" << LL_ENDL;
	}
	else
	{
		//BD - Right Mouse down will interrupt hover but will not immediately fire left mouse up.
		//     If we previously fire right mouse down, cancel out and revert our value.
		if (mRightMousePressed)
		{
			gFocusMgr.setMouseCapture(NULL);
			setValueAndCommit(mOriginalValue);
			mRightMousePressed = false;
		}

		getWindow()->setCursor(UI_CURSOR_ARROW);
		// _LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)" << LL_ENDL;		
	}
	return true;
}

bool LLSlider::handleMouseUp(S32 x, S32 y, MASK mask)
{
    bool handled = false;

	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);

		if (mMouseUpSignal)
			(*mMouseUpSignal)(this, getValueF32());

        handled = true;
        make_ui_sound("UISndClickRelease");
    }
    else
    {
        handled = true;
    }

    return handled;
}

bool LLSlider::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// only do sticky-focus on non-chrome widgets
	if (!getIsChrome())
	{
		setFocus(true);
	}
	if (mMouseDownSignal)
		(*mMouseDownSignal)( this, getValueF32() );

	// Find the offset of the actual mouse location from the center of the thumb.
	if (mThumbRect.pointInRect(x, y))
	{
		mInitPos = LLVector2((F32)x, (F32)y);
	}

	// No handler needed for focus lost since this class has no state that depends on it.
	gFocusMgr.setMouseCapture(this);

	mOriginalValue = getValueF32();

	// Start dragging the thumb
	mDragStartThumbRect = mThumbRect;

	make_ui_sound("UISndClick");

	return true;
}

//BD - UI Improvements
bool LLSlider::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	//BD - Right Mouse down will interrupt hover but will not immediately fire left mouse up.
	//     If we previously fire right mouse down, cancel out and revert out value.
	if (mRightMousePressed)
	{
		gFocusMgr.setMouseCapture(NULL);
		mRightMousePressed = false;
		setValueAndCommit(mOriginalValue);
	}

	return true;
}

//BD - UI Improvements
bool LLSlider::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// only do sticky-focus on non-chrome widgets
	if (!getIsChrome())
	{
		setFocus(true);
	}

	if ((MASK_SHIFT | MASK_CONTROL) & mask)
	{
		LLControlVariable* control = this->getControlVariable();
		if (control)
		{
			control->resetToDefault(true);
			make_ui_sound("UISndClick");
			return true;
		}
	}
	else
	{
		mRightMousePressed = true;
	}

	return true;
}

bool LLSlider::handleKeyHere(KEY key, MASK mask)
{
	bool handled = false;
	switch(key)
	{
	case KEY_DOWN:
		if (mask == MASK_SHIFT)
		{
			//BD - Allow changing the increment value.
			//     We should probably do something to indicate that the increment has changed.
			setIncrement(mIncrement / 10);
			onCommit();
			handled = true;
		}
		break;
	case KEY_LEFT:
		setValueAndCommit(getValueF32() - getIncrement());
		onCommit();
		handled = true;
		break;
	case KEY_UP:
		if (mask == MASK_SHIFT)
		{
			//BD - Allow changing the increment value.
			//     We should probably do something to indicate that the increment has changed.
			setIncrement(llmin(mIncrement * 10, mMaxValue));
			onCommit();
			handled = true;
		}
		break;
	case KEY_RIGHT:
		setValueAndCommit(getValueF32() + getIncrement());
		onCommit();
		handled = true;
		break;
	default:
		break;
	}
	return handled;
}

//BD - UI Improvements
bool LLSlider::handleScrollWheel(S32 x, S32 y, S32 clicks, MASK mask)
{
	//BD - Only allow using scrollwheel when holding down CTRL.
	//     But allow using it on both horizontal and vertical sliders.
	if (MASK_CONTROL & mask)
	{
		F32 increment = getIncrement();
		if (MASK_SHIFT & mask)
		{
			increment *= 10;
		}
		F32 new_val = getValueF32() - clicks * increment;
		setValueAndCommit(new_val);
		return true;
	}
	return LLF32UICtrl::handleScrollWheel(x,y,clicks,mask);
}

void LLSlider::draw()
{
	F32 alpha = getDrawContext().mAlpha;

	// since thumb image might still be decoding, need thumb to accomodate image size
	updateThumbRect();

	// Draw background and thumb.

	// drawing solids requires texturing be disabled
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Track
	LLPointer<LLUIImage>& trackImage = ( mOrientation == HORIZONTAL )
		? mTrackImageHorizontal
		: mTrackImageVertical;

	LLPointer<LLUIImage>& trackHighlightImage = ( mOrientation == HORIZONTAL )
		? mTrackHighlightHorizontalImage
		: mTrackHighlightVerticalImage;

//	//BD - Track Difference Visualization
	LLPointer<LLUIImage>& trackChangeImage = (mOrientation == HORIZONTAL)
		? mTrackChangeHorizontalImage
		: mTrackChangeVerticalImage;

	LLRect track_rect;
	LLRect highlight_rect;
//	//BD - Track Difference Visualization
	LLRect change_rect;

	if ( mOrientation == HORIZONTAL )
	{
		track_rect.set(mThumbImage->getWidth() / 2,
					   getLocalRect().getCenterY() + (trackImage->getHeight() / 2), 
					   getRect().getWidth() - mThumbImage->getWidth() / 2,
					   getLocalRect().getCenterY() - (trackImage->getHeight() / 2) );
		highlight_rect.set(track_rect.mLeft, track_rect.mTop, mThumbRect.getCenterX(), track_rect.mBottom);
//		//BD - Track Difference Visualization
		change_rect.set(mThumbRect.getCenterX(), track_rect.mTop, track_rect.mRight, track_rect.mBottom);
	}
	else
	{
		track_rect.set(getLocalRect().getCenterX() - (trackImage->getWidth() / 2),
					   getRect().getHeight(),
					   getLocalRect().getCenterX() + (trackImage->getWidth() / 2),
					   0);
		highlight_rect.set(track_rect.mLeft, mThumbRect.getCenterY(), track_rect.mRight, track_rect.mBottom);
//		//BD - Track Difference Visualization
		change_rect.set(track_rect.mLeft, track_rect.mTop, track_rect.mRight, mThumbRect.getCenterY());
	}

	trackImage->draw(track_rect, LLColor4::white % alpha);
	trackHighlightImage->draw(highlight_rect, LLColor4::white % alpha);
//	//BD - Track Difference Visualization
	trackChangeImage->draw(change_rect, LLColor4::white % alpha);

	// Thumb
	if (hasFocus())
	{
		// Draw focus highlighting.
		mThumbImage->drawBorder(mThumbRect, gFocusMgr.getFocusColor() % alpha, gFocusMgr.getFocusFlashWidth());
	}

	if( hasMouseCapture() ) // currently clicking on slider
	{
		// Show ghost where thumb was before dragging began.
		if (mThumbImage.notNull())
		{
			mThumbImage->draw(mDragStartThumbRect, mThumbCenterColor.get() % (0.3f * alpha));
		}
		if (mThumbImagePressed.notNull())
		{
			mThumbImagePressed->draw(mThumbRect, mThumbOutlineColor % alpha);
		}
	}
	else if (!isInEnabledChain())
	{
		if (mThumbImageDisabled.notNull())
		{
			mThumbImageDisabled->draw(mThumbRect, mThumbCenterColor % alpha);
		}
	}
	else
	{
		if (mThumbImage.notNull())
		{
			mThumbImage->draw(mThumbRect, mThumbCenterColor % alpha);
		}
	}
	
	LLUICtrl::draw();
}

boost::signals2::connection LLSlider::setMouseDownCallback( const commit_signal_t::slot_type& cb )
{
    if (!mMouseDownSignal) mMouseDownSignal = new commit_signal_t();
    return mMouseDownSignal->connect(cb);
}

boost::signals2::connection LLSlider::setMouseUpCallback(   const commit_signal_t::slot_type& cb )
{
    if (!mMouseUpSignal) mMouseUpSignal = new commit_signal_t();
    return mMouseUpSignal->connect(cb);
}
