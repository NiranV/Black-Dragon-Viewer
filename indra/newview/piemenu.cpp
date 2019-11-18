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

const S32 PIE_INNER_SIZE = 70;			// radius of the inner pie circle
const F32 PIE_POPUP_TIME = (F32)0.325;	// time to shrink from popup size to regular size
const S32 PIE_OUTER_SIZE = 136;			// radius of the outer pie circle

// register the pie menu globally as child widget
static LLDefaultChildRegistry::Register<PieMenu> r1("pie_menu");

// register all possible child widgets a pie menu can have
static PieChildRegistry::Register<PieMenu> pie_r1("pie_menu");
static PieChildRegistry::Register<PieSlice> pie_r2("pie_slice");
static PieChildRegistry::Register<PieSeparator> pie_r3("pie_separator");

// pie slice label text positioning
S32 PIE_X[] = { 102, 72, 0, -72, -102, -72, 0, 72 };
S32 PIE_Y[] = { 0, 73, 105, 73, 0, -73, -105, -73 };

PieMenu::PieMenu(const LLMenuGL::Params& p) :
LLMenuGL(p)
{
	LL_DEBUGS() << "PieMenu::PieMenu()" << LL_ENDL;
	// radius, so we need this *2
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, false);

	//BD - Set up the font for the menu.
	//     Always use small font.
	mFont = LLFontGL::getFont(LLFontDescriptor("SansSerif", "Small", LLFontGL::NORMAL));

	// set slices pointer to our own slices
	mSlices = &mMySlices;
	// this will be the first click on the menu
	mFirstClick = true;

	// clean initialisation
	mSlice = 0;
}

bool PieMenu::addChild(LLView* child, S32 tab_group)
{
	// don't add invalid slices
	if (!child) return false;

	// add a new slice to the menu
	mSlices->push_back(child);

	// tell the view that our menu has changed and reshape it back to correct size
	LLUICtrl::addChild(child);
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, false);

	return true;
}

void PieMenu::removeChild(LLView* child)
{
	// remove a slice from the menu
	slice_list_t::iterator found_it = std::find(mSlices->begin(), mSlices->end(), child);
	if (found_it != mSlices->end())
	{
		mSlices->erase(found_it);
	}

	// tell the view that our menu has changed and reshape it back to correct size
	LLUICtrl::removeChild(child);
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, false);
}

BOOL PieMenu::handleHover(S32 x, S32 y, MASK mask)
{
	// do nothing
	return TRUE;
}

void PieMenu::show(S32 x, S32 y, LLView* spawning_view)
{
	// if the menu is already there, do nothing
	if (getVisible()) return;

	LL_DEBUGS() << "PieMenu::show(): " << x << " " << y << LL_ENDL;

	// make sure the menu is always the correct size
	reshape(PIE_OUTER_SIZE * 2, PIE_OUTER_SIZE * 2, false);

	// remember our center point
	mCenterX = x;
	mCenterY = y;

	// get the 3D view rectangle
	LLRect screen = LLMenuGL::sMenuContainer->getMenuRect();

	// check if the pie menu is out of bounds and move it accordingly
	if (x - PIE_OUTER_SIZE<0)
	{
		x = PIE_OUTER_SIZE;
	}
	else if (x + PIE_OUTER_SIZE > screen.getWidth())
	{
		x = screen.getWidth() - PIE_OUTER_SIZE;
	}

	if (y - PIE_OUTER_SIZE < screen.mBottom)
	{
		y = PIE_OUTER_SIZE + screen.mBottom;
	}
	else if (y + PIE_OUTER_SIZE - screen.mBottom>screen.getHeight())
	{
		y = screen.getHeight() - PIE_OUTER_SIZE + screen.mBottom;
	}

	// move the mouse pointer into the center of the menu
	LLUI::getInstance()->setMousePositionLocal(getParent(), x, y);
	//BD - Set our drawing origin to the center of the menu
	//     Don't take UI Size into account, it breaks the selection area.
	setOrigin(x - PIE_OUTER_SIZE, y - PIE_OUTER_SIZE);
	// grab mouse control
	gFocusMgr.setMouseCapture(this);

	// this was the first click for the menu
	mFirstClick = true;
	// set up the slices pointer to the menu's own slices
	mSlices = &mMySlices;

	// reset enable update checks for slices
	for (slice_list_t::iterator it = mSlices->begin(); it != mSlices->end(); it++)
	{
		PieSlice* resetSlice = dynamic_cast<PieSlice*>(*it);
		if (resetSlice)
		{
			resetSlice->resetUpdateEnabledCheck();
		}
	}

	// cleanup
	mSlice = 0;
	mOldSlice = 0;

	// draw the menu on screen
	setVisible(true);
	LLView::setVisible(true);
}

void PieMenu::hide()
{
	// if the menu is already hidden, do nothing
	if (!getVisible()) return;

	LL_DEBUGS() << "Clearing selections" << LL_ENDL;

	mSlices = &mMySlices;
	if (gSavedSettings.getBOOL("PieMenuPopup"))
	{
		// safety in case the timer was still running
		mPopupTimer.stop();
	}
	LLView::setVisible(FALSE);
}

void PieMenu::setVisible(BOOL visible)
{
	// hide the menu if needed
	if (!visible)
	{
		hide();
		// clear all menus and temporary selections
		sMenuContainer->hideMenus();
	}
}

void PieMenu::draw()
{
	// save the current OpenGL drawing matrix so we can freely modify it
	gGL.pushMatrix();

	// save the widget's rectangle for later use
	LLRect r = getRect();

	// initialize pie scale factor for popup effect
	F32 factor = 1.0;

	// set the popup size if this was the first click on the menu
	if (mFirstClick)
	{
		factor = 0.0;
	}
	// otherwise check if the popup timer is still running
	else if (mPopupTimer.getStarted())
	{
		// if the timer ran past the popup time, stop the timer and set the size to 1.0
		F32 elapsedTime = mPopupTimer.getElapsedTimeF32();
		F32 popuptime = gSavedSettings.getF32("PieMenuPopupTime");
		if (elapsedTime > popuptime)
		{
			factor = 1.0;
			mPopupTimer.stop();
		}
		// otherwise calculate the size factor to make the menu shrink over time
		else
		{
			factor = 0.0 - (0.0 - 1.0)*elapsedTime / popuptime;
		}
	}

	// set up pie menu colors
	LLColor4 lineColor = LLUIColorTable::instance().getColor("PieMenuLineColor");
	LLColor4 selectedColor = LLUIColorTable::instance().getColor("PieMenuSelectedColor");
	LLColor4 textColor = LLUIColorTable::instance().getColor("PieMenuTextColor");
	LLColor4 bgColor = LLUIColorTable::instance().getColor("PieMenuBgColor") % gSavedSettings.getF32("PieMenuOpacity");
	LLColor4 borderColor = bgColor % (1.f - gSavedSettings.getF32("PieMenuFade"));
	// initially, the current segment is marked as invalid
	S32 currentSegment = -1;
	// get the current mouse pointer position local to the pie
	S32 x, y;

	LLUI::getInstance()->getMousePositionLocal(this, &x, &y);
	//BD - move mouse coordinates to be relative to the pie center
	//     Don't take UI size into account, it breaks the selection area.
	LLVector2 mouseVector(x - PIE_OUTER_SIZE, y - PIE_OUTER_SIZE);
	// get the distance from the center point
	F32 distance = mouseVector.length();
	// check if our mouse pointer is within the pie slice area

	if (distance > PIE_INNER_SIZE && (distance < (PIE_OUTER_SIZE*factor) || mFirstClick))
	{
		// get the angle of the mouse pointer from the center in radians
		F32 angle = acos(mouseVector.mV[VX] / distance);
		// if the mouse is below the middle of the pie, reverse the angle
		if (mouseVector.mV[VY] < 0)
		{
			angle = F_PI * 2 - angle;
		}
		// rotate the angle slightly so the slices' centers are aligned correctly
		angle += F_PI / 8;

		// calculate slice number from the angle
		currentSegment = (S32)(8.0*angle / (F_PI * 2.0)) % 8;
	}

	//BD - Take UI size into account for the pie menu visuals.
	//     I don't understand why it was divide by rather than multiply by
	//     in the first place but since the visuals are the only thing being
	//     offsetted we only need to have those take UI size into account to
	//     make the selection area and the visuals match.
	LLVector2 scale = gViewerWindow->getDisplayScale();
	// move origin point to the center of our rectangle
	gGL.translatef(r.getWidth() / 2 * scale.mV[VX], r.getHeight() / 2 * scale.mV[VY], 0.0);

	// set up an item list iterator to point at the beginning of the item list
	slice_list_t::iterator cur_item_iter;
	cur_item_iter = mSlices->begin();

	S32 num = 0;

	//BD - Render the background for each slice.
	F32 segmentStart = F_TWO_PI / 8.0;
	gl_washer_segment_2d(PIE_OUTER_SIZE, PIE_INNER_SIZE, segmentStart, segmentStart + (F_TWO_PI * factor), 42, bgColor, borderColor);

	// clear current slice pointer
	mSlice = 0;
	bool wasAutohide = false;
	// reset our current slice number to 0
	do
	{
		// standard item text color
		LLColor4 itemColor = textColor * factor;

		// clear the label and set up the starting angle to draw in
		std::string label[3];
		int label_count = 0;

		// iterate through the list of slices
		if (cur_item_iter != mSlices->end())
		{
			// get current slice item
			LLView* item = (*cur_item_iter);
			if (!item) continue;

			segmentStart = F_PI / 4.0*(F32)num - F_PI / 8.0;

			// check if this is a submenu or a normal click slice
			PieSlice* currentSlice = dynamic_cast<PieSlice*>(item);
			PieMenu* currentSubmenu = dynamic_cast<PieMenu*>(item);
			// advance internally to the next slice item
			cur_item_iter++;

			// in case it is regular click slice
			if (currentSlice)
			{
				// get the slice label and tell the slice to check if it's supposed to be visible
				label[0] = currentSlice->getLabel();
				currentSlice->updateVisible();
				// disable it if it's not visible, pie slices never really disappear
				if (!currentSlice->getVisible())
				{
					LL_DEBUGS() << label[0] << " is not visible" << LL_ENDL;
					currentSlice->setEnabled(false);
				}

				// if the current slice is the start of an autohide chain, clear out previous chains
				if (currentSlice->getStartAutohide())
				{
					wasAutohide = false;
				}

				// check if the current slice is part of an autohide chain
				if (currentSlice->getAutohide())
				{
					// if the previous item already won the autohide, skip this item
					if (wasAutohide)
					{
						continue;
					}

					// look at the next item in the pie
					LLView* lookAhead = (*cur_item_iter);
					if (lookAhead)
					{
						// check if this is a normal click slice
						PieSlice* lookSlice = dynamic_cast<PieSlice*>(lookAhead);
						if (lookSlice)
						{
							// if the next item is part of the current autohide chain as well ...
							if (lookSlice->getAutohide() && !lookSlice->getStartAutohide())
							{
								// ... it's visible and it's enabled, skip the current one.
								// the first visible and enabled item in autohide chains wins
								// this is useful for Sit/Stand toggles
								lookSlice->updateEnabled();
								lookSlice->updateVisible();

								if (lookSlice->getVisible() && lookSlice->getEnabled())	continue;

								// this item won the autohide contest
								wasAutohide = true;
							}
						}
					}
				}
				else
				{
					// reset autohide chain
					wasAutohide = false;
				}

				// check if the slice is currently enabled
				currentSlice->updateEnabled();
				if (!currentSlice->getEnabled())
				{
					LL_DEBUGS() << label[0] << " is disabled" << LL_ENDL;
					//fade the item color alpha to mark the item as disabled
					itemColor %= (F32)0.3;
				}
			}
			// if it's a submenu just get the label
			else if (currentSubmenu)
			{
				label[0] = currentSubmenu->getLabel();

				//BD - draw the submenu marker
				gl_washer_segment_2d(PIE_OUTER_SIZE * 1.09, PIE_OUTER_SIZE, 
									segmentStart + 0.02, segmentStart + ((F_PI / 4.0 - 0.02) * factor), 
									8, selectedColor, selectedColor);
			}

			// if it's a slice or submenu, the mouse pointer is over the same segment as our counter and the item is enabled
			if ((currentSlice || currentSubmenu) && (currentSegment == num) && item->getEnabled())
			{
				// memorize the currently highlighted slice for later
				mSlice = item;
				// if we highlighted a different slice than before, we must play a sound
				if (mOldSlice != mSlice)
				{
					// get the appropriate UI sound and play it
					char soundNum = '0' + num;
					std::string soundName = "UISndPieMenuSliceHighlight";
					soundName += soundNum;

					// remember which slice we highlighted last, so we only play the sound once
					mOldSlice = mSlice;
				}

				//BD - draw the currently highlighted pie slice
				gl_washer_segment_2d(PIE_OUTER_SIZE, PIE_INNER_SIZE, segmentStart + 0.02, segmentStart + F_PI / 4.0 - 0.02, 8, selectedColor, borderColor);
			}
		}

		for (label_count = 0; label_count < 2; label_count++)
		{
			std::size_t pos = label[label_count].find(";");
			if (pos != std::string::npos)
			{
				label[label_count + 1] = label[label_count].substr(pos + 1);
				label[label_count] = label[label_count].substr(0, pos);
			}
		}

		// draw the slice labels around the center
		if (!label[0].empty())
		{
			mFont->renderUTF8(label[0], 0, PIE_X[num], PIE_Y[num], itemColor,
				LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT);
		}
		if (!label[1].empty())
		{
			mFont->renderUTF8(label[1], 0, PIE_X[num], PIE_Y[num] - 13, itemColor,
				LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT);
		}
		if (!label[2].empty())
		{
			mFont->renderUTF8(label[2], 0, PIE_X[num], PIE_Y[num] + 13, itemColor,
				LLFontGL::HCENTER, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT);
		}
		// next slice
		++num;
	}
	while (num < 8);

	//BD - draw inner and outer circle, outer only if it was not the first click
	F32 circleStart = F_PI / 4.0;
	F32 circleOffset = F_PI / 8.0;

	if (!mFirstClick)
	{
		gl_washer_segment_2d(PIE_OUTER_SIZE, PIE_OUTER_SIZE - 2, circleStart - circleOffset, circleStart - circleOffset + ((F_PI * 2.0) * factor * factor), 64, borderColor, borderColor);
		gl_washer_segment_2d(PIE_OUTER_SIZE * 1.09, PIE_OUTER_SIZE - 2, circleStart * 3 - circleOffset, circleStart * 3 - circleOffset + ((F_PI * 2.0) * factor * factor), 64, borderColor, lineColor);
	}
	gl_washer_segment_2d(PIE_INNER_SIZE + 1, PIE_INNER_SIZE - 1, circleStart * 6 - circleOffset, circleStart * 6 - circleOffset + ((F_PI * 2.0) * factor * factor), 32, borderColor, lineColor);

	// restore OpenGL drawing matrix
	gGL.popMatrix();

	// give control back to the UI library
	LLView::draw();
}

bool PieMenu::appendContextSubMenu(PieMenu* menu)
{
	LL_DEBUGS() << "PieMenu::appendContextSubMenu()" << LL_ENDL;
	if (!menu) return false;

	LL_DEBUGS() << "PieMenu::appendContextSubMenu() appending " << menu->getLabel() << " to " << getLabel() << LL_ENDL;

	// add the submenu to the list of items
	mSlices->push_back(menu);
	// tell the view that our menu has changed
	LLUICtrl::addChild(menu);

	return true;
}

BOOL PieMenu::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// left and right mouse buttons both do the same thing currently
	return handleMouseButtonUp(x, y, mask);
}

BOOL PieMenu::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// left and right mouse buttons both do the same thing currently
	return handleMouseButtonUp(x, y, mask);
}

// left and right mouse buttons both do the same thing currently
bool PieMenu::handleMouseButtonUp(S32 x, S32 y, MASK mask)
{
	// if this was the first click and no slice is highlighted (no borderless click), start the popup timer
	if (!mSlice && mFirstClick)
	{
		mFirstClick = false;
		mPopupTimer.start();
	}
	else
	{
		// default to invisible
		bool visible = false;

		//BD
		if (mSlice)
		{
			// get the current selected slice and check if this is a regular click slice
			PieSlice* currentSlice = dynamic_cast<PieSlice*>(mSlice);
			if (currentSlice)
			{
				// if so, click it and make a sound
				make_ui_sound("UISndClickRelease");
				currentSlice->onCommit();
			}
			else
			{
				// check if this was a submenu
				PieMenu* currentSubmenu = dynamic_cast<PieMenu*>(mSlice);
				if (currentSubmenu)
				{
					// if so, remember we clicked the menu already at least once
					mFirstClick = false;
					// swap out the list of items for the ones in the submenu
					mSlices = &currentSubmenu->mMySlices;
					// reset enable update checks for slices
					for (slice_list_t::iterator it = mSlices->begin(); it != mSlices->end(); it++)
					{
						PieSlice* resetSlice = dynamic_cast<PieSlice*>(*it);
						if (resetSlice)
						{
							resetSlice->resetUpdateEnabledCheck();
						}
					}
					// the menu stays visible
					visible = true;

					// restart the popup timer
					mPopupTimer.reset();
					mPopupTimer.start();
				}
			}
		}

		// show or hide the menu, as needed
		setVisible(visible);
	}
	// release mouse capture after the first click if we still have it grabbed
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}

	// give control back to the system
	return LLView::handleMouseUp(x, y, mask);
}
