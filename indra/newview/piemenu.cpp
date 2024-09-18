/**
 * @file piemenu.cpp
 * @brief Pie menu class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2011, Zi Ree @ Second Life
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

#include "piemenu.h"
#include "pieslice.h"
#include "pieseparator.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

// Register the pie menu globally as child widget
static LLDefaultChildRegistry::Register<PieMenu> r1("pie_menu");

// Register all possible child widgets a pie menu can have
static PieChildRegistry::Register<PieMenu> pie_r1("pie_menu");
static PieChildRegistry::Register<PieSlice> pie_r2("pie_slice");
static PieChildRegistry::Register<PieSeparator> pie_r3("pie_separator");

const S32 PIE_INNER_SIZE = 70;             // radius of the inner pie circle
const S32 PIE_OUTER_SIZE = 136;             // radius of the outer pie circle
const F32 PIE_MAX_SLICES_F = F32(PIE_MAX_SLICES);

PieMenu::PieMenu(const LLMenuGL::Params& p) :
	LLMenuGL(p),
	mCurrentSegment(-1),
	mCurrentAngle(0.0f),
	mInsidePie(false),
	mInsideCenter(true),
	mSnapSegment(-1),
	mTopMenu(true)
{
	LL_DEBUGS("Pie") << "PieMenu::PieMenu()" << LL_ENDL;

	mBGImage = LLRender2D::getInstance()->getUIImage("PieMenu_Background");
	mCenterImage = LLRender2D::getInstance()->getUIImage("PieMenu_Center");
	mBackImage = LLRender2D::getInstance()->getUIImage("PieMenu_Back");

	// Radius, so we need this *2
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, FALSE);
	
	//BD - Set up the font for the menu.
	//     Always use small font.
	mFont = LLFontGL::getFont(LLFontDescriptor("SansSerif", "Small", LLFontGL::NORMAL));
	
	// Set slices pointer to our own slices
	mSlices = &mMySlices;
	
	// This will be the first click on the menu
	mFirstClick = true;

	// Clean initialisation
	mSlice = NULL;

}

bool PieMenu::addChild(LLView* child, S32 tab_group)
{
	// Don't add invalid slices
	if (!child)
		return false;

	// Add a new slice to the menu
	mSlices->push_back(child);

	// Tell the view that our menu has changed and reshape it back to correct size
	LLUICtrl::addChild(child);
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, FALSE);

	return true;
}

void PieMenu::removeChild(LLView* child)
{
	// Remove a slice from the menu
	slice_list_t::iterator found_it = std::find(mSlices->begin(), mSlices->end(), child);

	if (found_it != mSlices->end())
	{
		mSlices->erase(found_it);
	}

	// Tell the view that our menu has changed and reshape it back to correct size
	LLUICtrl::removeChild(child);
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, FALSE);
}

bool PieMenu::handleHover(S32 x, S32 y, MASK mask)
{
	// Initially, the current segment is marked as invalid
	mCurrentSegment = -1;

	// Move mouse coordinates to be relative to the pie center
	LLVector2 mouseVector(x - PIE_OUTER_SIZE, y - PIE_OUTER_SIZE);

	// Get the distance from the center point
	F32 distance = mouseVector.length();

	mInsidePie = false;

	// Check if our mouse pointer is within the pie slice area
	if (distance > PIE_INNER_SIZE && distance < PIE_OUTER_SIZE)
	{
		// Get the angle of the mouse pointer from the center in radians
		F32 angle = acos(mouseVector.mV[VX] / distance);
		// If the mouse is below the middle of the pie, reverse the angle
		if (mouseVector.mV[VY] < 0.f)
		{
			angle = F_TWO_PI - angle;
		}

		mCurrentAngle = angle;
		if (mInsideCenter)
		{
			mInsideCenter = false;
			mInsideCenterTimer.stop();
		}
		mInsidePie = true;

		// Rotate the angle slightly so the slices' centers are aligned correctly
		angle += F_PI / PIE_MAX_SLICES_F;

		// Calculate slice number from the angle
		mCurrentSegment = (S32) (PIE_MAX_SLICES_F * angle / F_TWO_PI) % PIE_MAX_SLICES;
	}
	//BD - This is the first opening click or we are hovering our mouse somewhere around the
	//     center of the pie menu.
	else if (distance < PIE_INNER_SIZE * 0.7f)
	{
		if (!mInsideCenter)
		{
			mInsideCenter = true;
			if (!mInsideCenterTimer.getStarted())
			{
				mInsideCenterTimer.reset();
				mInsideCenterTimer.start();
			}
		}
		mInsidePie = false;
	}

	return true;
}

void PieMenu::show(S32 x, S32 y, LLView* spawning_view)
{
	// If the menu is already there, do nothing
	if (getVisible()) 
		return;

	// Get the 3D view rectangle
	LLRect screen = LLMenuGL::sMenuContainer->getMenuRect();

	make_ui_sound("UISndPieMenuAppear");

	LL_DEBUGS("Pie") << "PieMenu::show(): " << x << " " << y << LL_ENDL;

	// Make sure the menu is always the correct size
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, FALSE);

	// Check if the pie menu is out of bounds and move it accordingly
	if (x - PIE_OUTER_SIZE < 0)
		x = PIE_OUTER_SIZE;
	else if (x + PIE_OUTER_SIZE > screen.getWidth())
		x = screen.getWidth() - PIE_OUTER_SIZE;

	if (y - PIE_OUTER_SIZE < screen.mBottom)
		y = PIE_OUTER_SIZE + screen.mBottom;
	else if (y + PIE_OUTER_SIZE - screen.mBottom > screen.getHeight())
		y = screen.getHeight() - PIE_OUTER_SIZE + screen.mBottom;

	// Move the mouse pointer into the center of the menu
	LLUI::getInstance()->setMousePositionLocal(getParent(), x, y);

	// Set our drawing origin to the center of the menu
	setOrigin(x - PIE_OUTER_SIZE, y - PIE_OUTER_SIZE);

	// Grab mouse control
	gFocusMgr.setMouseCapture(this);

	// This was the first click for the menu
	mFirstClick = true;

	// Set up the slices pointer to the menu's own slices
	mSlices = &mMySlices;

	// Reset enable update checks for slices
	for (slice_list_t::iterator it = mSlices->begin(); it != mSlices->end(); ++it)
	{
		PieSlice* resetSlice = dynamic_cast<PieSlice*>(*it);
		if (resetSlice)
		{
			resetSlice->resetUpdateEnabledCheck();
		}
	}

	// Cleanup
	mCurrentSegment = -1;
	mSnapSegment = -1;
	mSlice = NULL;
	mOldSlice = NULL;
	mTopMenu = true;

	// Draw the menu on screen
	setVisible(TRUE);
	LLView::setVisible(TRUE);
}

void PieMenu::hide()
{
	// If the menu is already hidden, do nothing
	if (!getVisible()) 
		return;

	// Make a sound when hiding
	make_ui_sound("UISndPieMenuHide");

	LL_DEBUGS("Pie") << "Clearing selections" << LL_ENDL;

	mCurrentSegment = -1;
	mSnapSegment = -1;
	mInsidePie = false;
	mInsideCenter = true;
	mSlices = &mMySlices;

	// Safety in case the timer was still running
	mPopupTimer.stop();
	mInsideCenterTimer.stop();

	LLView::setVisible(FALSE);
}

void PieMenu::setVisible(bool visible)
{
	// Hide the menu if needed
	if (!visible)
	{
		hide();
		// Clear all menus and temporary selections
		sMenuContainer->hideMenus();
	}
}

void PieMenu::draw()
{
	S32 num = 0;
	F32 cur_segment_angle = mCurrentAngle;
	F32 pie_half = (F_PI / 8.0);
	F32 scale_factor = getScaleFactor();
	F32 cur_angle = 0.0f;

	LLColor4 selectedColor = LLUIColorTable::instance().getColor("PieMenuSelectedColor");
	LLColor4 textColor = LLUIColorTable::instance().getColor("PieMenuTextColor");
	LLColor4 bgColor = LLColor4(1, 1, 1, scale_factor);
	LLColor4 borderColor = LLUIColorTable::instance().getColor("PieMenuBgColor") % 0.5f;
	
	LLRect rect = getRect();
	LLVector2 scale = gViewerWindow->getDisplayScale();
	slice_list_t::iterator cur_item_iter = mSlices->begin();

	bool wasAutohide = false;
	bool found_selection = false;

	// Save the current OpenGL drawing matrix so we can freely modify it
	gGL.pushMatrix();
	// Move origin point to the center of our rectangle
	gGL.translatef(rect.getWidth() / 2.f * scale.mV[VX], rect.getHeight() / 2.f * scale.mV[VY], 0.f);

	textColor.mV[VW] = scale_factor;
	cur_segment_angle += F_PI / PIE_MAX_SLICES_F;
	cur_segment_angle = fmodf(PIE_MAX_SLICES_F * cur_segment_angle / F_TWO_PI, 1.0f);
	cur_angle = mCurrentAngle;

	mSlice = NULL;
	// Draw background image
	mBGImage->draw(-(mBGImage->getWidth() / 2), -(mBGImage->getHeight() / 2), mBGImage->getWidth(), mBGImage->getHeight(), bgColor);

	//BD - We are inside the pie menu area, either render the free selection or snap to an entry.
	if (mInsidePie)
	{
		//BD - Default to free selection.
		F32 new_angle = cur_angle;
		F32 diff = mOldAngle - cur_angle;
		//BD - Use the currently highlighted pie slice if we are close to its center.
		if (cur_segment_angle > 0.25f && cur_segment_angle < 0.75f && mSnapSegment >= 0)
		{
			cur_angle = (F_PI / 4.0 * (F32)mCurrentSegment);
			diff = mOldAngle - cur_angle;
		}

		//BD - Make sure we accomodate and fix the sudden wrap when we approach F_TWO_PI
		if (diff > F_PI)
			mOldAngle -= F_TWO_PI;
		else if (diff < -F_PI)
			mOldAngle += F_TWO_PI;

		new_angle = lerp(mOldAngle, cur_angle, 0.5f);
		gl_washer_segment_2d(PIE_OUTER_SIZE - 2, PIE_INNER_SIZE + 2, new_angle - pie_half + 0.02, new_angle + pie_half - 0.02, 8, selectedColor, borderColor);
		mOldAngle = new_angle;
		//BD - Why? hover() only works while we actually hover the pie menu's bounding box
		//     so as soon as we move outside the bounding box we are no longer checking if
		//     we are inside any area of the pie menu (obviously) this also means we can't
		//     check if we are outside of the bounding box (or generally around the outer
		//     area of the pie menu to stop drawing the selection. So instead we simply set
		//     the inside bool to false after we are done since between each draw() we have
		//     to go through a hover() IF we are actually hovering the pie menu, it will be
		//     set to true if necessary.
		mInsidePie = false;
	}
	else if (mInsideCenter && !mTopMenu)
	{
		F32 outside_factor = getOutsideFactor();
		LLColor4 color = LLColor4(1, 1, 1, outside_factor);

		mCenterImage->draw( -(mCenterImage->getWidth() / 2) * outside_factor,
							-(mCenterImage->getHeight() / 2) * outside_factor,
							mCenterImage->getWidth() * outside_factor,
							mCenterImage->getHeight() * outside_factor,
							color);
		mBackImage->draw(	-(mBackImage->getWidth() / 2),
							-(mBackImage->getHeight() / 2),
							mBackImage->getWidth(),
							mBackImage->getHeight(),
							color);
	}

	while (num < PIE_MAX_SLICES)	// do this until the menu is full
	{
		// Standard item text color
		LLColor4 itemColor = textColor;
		std::string label[3];
		int label_count = 0;

		// Iterate through the list of slices
		if (cur_item_iter != mSlices->end())
		{
			LLView* item = (*cur_item_iter);
			// Check if this is a submenu or a normal click slice
			PieSlice* currentSlice = dynamic_cast<PieSlice*>(item);
			PieMenu* currentSubmenu = dynamic_cast<PieMenu*>(item);

			// Advance internally to the next slice item
			cur_item_iter++;

			// In case it is regular click slice
			if (currentSlice)
			{
				// Get the slice label and tell the slice to check if it's supposed to be visible
				label[0] = currentSlice->getLabel();
				currentSlice->updateVisible();
				// Disable it if it's not visible, pie slices never really disappear
				BOOL slice_visible = currentSlice->getVisible();
				currentSlice->setEnabled(slice_visible);
				if (!slice_visible)
				{
					LL_DEBUGS("Pie") << label[0] << " is not visible" << LL_ENDL;
					currentSlice->setEnabled(false);
				}

				// If the current slice is the start of an autohide chain, clear out previous chains
				if (currentSlice->getStartAutohide())
				{
					wasAutohide = false;
				}

				// Check if the current slice is part of an autohide chain
				if (currentSlice->getAutohide())
				{
					// If the previous item already won the autohide, skip this item
					if (wasAutohide) 
						continue;

					// Look at the next item in the pie
					if (cur_item_iter != mSlices->end())
					{
						//BD - Make sure we aren't iterating to an invalid piece
						LLView* lookAhead = (*cur_item_iter);
						if (lookAhead) // THIS WAS MISSING
						{
							// Check if this is a normal click slice
							PieSlice* lookSlice = dynamic_cast<PieSlice*>(lookAhead);
							if (lookSlice)
							{
								// If the next item is part of the current autohide chain as well ...
								if (lookSlice->getAutohide() && !lookSlice->getStartAutohide())
								{
									// ... it's visible and it's enabled, skip the current one.
									// the first visible and enabled item in autohide chains wins
									// this is useful for Sit/Stand toggles
									lookSlice->updateEnabled();
									lookSlice->updateVisible();

									if (lookSlice->getVisible() && lookSlice->getEnabled())	
										continue;

									// This item won the autohide contest
									wasAutohide = true;
								}
							}
						}
					}
				}
				else
				{
					// Reset autohide chain
					wasAutohide = false;
				}

				// Check if the slice is currently enabled
				currentSlice->updateEnabled();
				if (!currentSlice->getEnabled())
				{
					LL_DEBUGS("Pie") << label[0] << " is disabled" << LL_ENDL;
					itemColor %= 0.3f;
				}
			}
			else if (currentSubmenu)
			{
				label[0] = currentSubmenu->getLabel();

				S32 image_idx = S32(num / 2);
				std::string image_name = llformat("PieMenu_SubMenu_%i", image_idx);

				mSubMenuImage = LLRender2D::getInstance()->getUIImage(image_name);
				mSubMenuImage->draw(-(mSubMenuImage->getWidth() / 2),
									-(mSubMenuImage->getHeight() / 2), 
									mSubMenuImage->getWidth(), 
									mSubMenuImage->getHeight(),
									itemColor);
			}

			// If it's a slice or submenu, the mouse pointer is over the same segment as our counter and the item is enabled
			if ((currentSlice || currentSubmenu) && (mCurrentSegment == num) && item->getEnabled())
			{
				// Memorize the currently highlighted slice for later
				mSlice = item;
				// If we highlighted a different slice than before, we must play a sound
				if (mOldSlice != mSlice)
				{
					// Get the appropriate UI sound and play it
					std::string soundName = llformat("UISndPieMenuSliceHighlight%d", num);
					make_ui_sound(soundName.c_str());

					// Remember which slice we highlighted last, so we only play the sound once
					mOldSlice = mSlice;
				}

				mSnapSegment = mCurrentSegment;
				found_selection = true;
			}
			else if (!found_selection)
			{
				mSnapSegment = -1;
			}
		}

		//BD - Count through our linebreaks and write down the labels in each line.
		while (true)
		{
			std::size_t pos = label[label_count].find(";");
			if (pos != std::string::npos)
			{
				label[label_count + 1] = label[label_count].substr(pos + 1);
				label[label_count] = label[label_count].substr(0, pos);
				label_count++;
			}
			else
				break;
		}

		// Draw the slice labels around the center
		for (S32 i = 0; i <= label_count; i++)
		{
			//BD - Calculate good place for each slice's label.
			F32 angle = (F_PI / 2.0f) - ((F_TWO_PI / PIE_MAX_SLICES_F)) * (num % PIE_MAX_SLICES);
			S32 x = S32(sin(angle) * 102);
			S32 y = S32(cos(angle) * 105);

			//BD - Draw the labels for each slice if they defined them.
			if (!label[i].empty())
			{
				mFont->renderUTF8(label[i], 0, x, y + (7 * label_count) - (14 * i), itemColor,
					LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT);
			}
		}

		// Next slice
		++num;
	}

	LL_INFOS("Pie") << mSnapSegment << " (" << mCurrentSegment << ") " << cur_segment_angle << LL_ENDL;

	// Restore OpenGL drawing matrix
	gGL.popMatrix();

	// Give control back to the UI library
	LLView::draw();
}

bool PieMenu::appendContextSubMenu(PieMenu* menu)
{
	LL_DEBUGS("Pie") << "PieMenu::appendContextSubMenu()" << LL_ENDL;
	if (!menu)
		return false;

	LL_DEBUGS("Pie") << "PieMenu::appendContextSubMenu() appending " << menu->getLabel() << " to " << getLabel() << LL_ENDL;

	// Add the submenu to the list of items
	mSlices->push_back(menu);
	// Tell the view that our menu has changed
	LLUICtrl::addChild(menu);

	return true;
}

bool PieMenu::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// Left and right mouse buttons both do the same thing currently
	return handleMouseButtonUp(x, y, mask);
}

bool PieMenu::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// Left and right mouse buttons both do the same thing currently
	return handleMouseButtonUp(x, y, mask);
}

// left and right mouse buttons both do the same thing currently
bool PieMenu::handleMouseButtonUp(S32 x, S32 y, MASK mask)
{
	// If this was the first click and no slice is highlighted (no borderless click), start the popup timer
	if (mFirstClick && !mSlice)
	{
		mFirstClick = false;
		mPopupTimer.start();
	}
	else
	{
		// Default to invisible
		bool visible = false;

		// Get the current selected slice and check if this is a regular click slice
		PieSlice* currentSlice = dynamic_cast<PieSlice*>(mSlice);
		if (currentSlice)
		{
			// If so, click it and make a sound
			make_ui_sound("UISndClickRelease");
			currentSlice->onCommit();
		}
		else
		{
			// Check if this was a submenu
			PieMenu* currentSubmenu = dynamic_cast<PieMenu*>(mSlice);
			if (currentSubmenu)
			{
				// If so, remember we clicked the menu already at least once
				mFirstClick = false;
				// Swap out the list of items for the ones in the submenu
				mSlices = &currentSubmenu->mMySlices;
				// Reset enable update checks for slices
				for (slice_list_t::iterator it = mSlices->begin(); it != mSlices->end(); ++it)
				{
					PieSlice* resetSlice = dynamic_cast<PieSlice*>(*it);

					if (resetSlice)
						resetSlice->resetUpdateEnabledCheck();
				}
				// The menu stays visible
				visible = true;

				// Restart the effect timers
				mPopupTimer.reset();
				mPopupTimer.start();

				// Make a sound
				make_ui_sound("UISndPieMenuAppear");

				//BD - We are navigating down into the menu hirarchy.
				mTopMenu = false;
			}
			//BD - Otherwise we clicked outside of the actual pie area.
			else
			{
				//BD - Go back to the top-level menu (for now) if we are not yet.
				//     Otherwise simply close the menu.
				if (!mTopMenu)
				{
					mSlices = &mMySlices;

					// Reset enable update checks for slices
					for (slice_list_t::iterator it = mSlices->begin(); it != mSlices->end(); ++it)
					{
						PieSlice* resetSlice = dynamic_cast<PieSlice*>(*it);

						if (resetSlice)
							resetSlice->resetUpdateEnabledCheck();
					}
					// The menu stays visible
					visible = true;

					// Restart the effect timers
					mPopupTimer.reset();
					mPopupTimer.start();

					// Make a sound
					make_ui_sound("UISndPieMenuAppear");

					//BD - We are leaving our submenu and returning back to the topmost.
					mTopMenu = true;
				}
			}
		}
		// Show or hide the menu, as needed
		setVisible(visible);
	}
	// Release mouse capture after the first click if we still have it grabbed
	if (hasMouseCapture())
		gFocusMgr.setMouseCapture(NULL);

	// Give control back to the system
	return LLView::handleMouseUp(x, y, mask);
}

F32 PieMenu::getScaleFactor()
{
	// Initialize pie scale factor for fade effect
	F32 factor = 1.f;

	// Set the fade if this was the first click on the menu
	if (mFirstClick)
	{
		factor = 0.0;
	}
	// Otherwise check if the fade timer is still running
	else if (mPopupTimer.getStarted())
	{
		// If the timer ran past the fade time, stop the timer and set the alpha to 1.0
		F32 elapsedTime = mPopupTimer.getElapsedTimeF32();
		F32 popuptime = gSavedSettings.getF32("PieMenuPopupTime");
		if (elapsedTime > popuptime)
		{
			factor = 1.0;
			mPopupTimer.stop();
		}
		// Otherwise calculate the alpha factor to make the menu fade over time
		else
		{
			factor = 0.0 - (0.0 - 1.0)*elapsedTime / popuptime;
		}
	}

	return factor;
}

F32 PieMenu::getOutsideFactor()
{
	// Initialize pie scale factor for popup effect
	F32 factor = 1.0f;

	if (mInsideCenterTimer.getStarted())
	{
		// If the timer ran past the popup time, stop the timer and set the size to 1.0
		F32 elapsedTime = mInsideCenterTimer.getElapsedTimeF32();
		F32 popuptime = 0.25;

		if (elapsedTime > popuptime)
		{
			factor = 1.0;
			mInsideCenterTimer.stop();
		}
		// Otherwise calculate the size factor to make the button shrink over time
		else
		{
			factor = elapsedTime / popuptime;
		}
	}

	return factor;
}
