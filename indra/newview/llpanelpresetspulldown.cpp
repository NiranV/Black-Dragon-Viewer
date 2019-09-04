/** 
 * @file llpanelpresetspulldown.cpp
 * @brief A panel showing a quick way to pick presets
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include "llpanelpresetspulldown.h"

#include "llviewercontrol.h"
#include "llstatusbar.h"

#include "llbutton.h"
#include "lltabcontainer.h"
#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "llpresetsmanager.h"
#include "llsliderctrl.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"

//BD 
#include "lldir.h"
#include "lldiriterator.h"
#include "llsdserialize.h"
#include "bdfunctions.h"

/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseFadeStartTimeSec = 2.0f;
/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseTotalTimeSec = 3.0f;

///----------------------------------------------------------------------------
/// Class LLPanelPresetsPulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelPresetsPulldown::LLPanelPresetsPulldown()
{
	mHoverTimer.stop();

	mCommitCallbackRegistrar.add("Presets.GoGraphicsPrefs", boost::bind(&LLPanelPresetsPulldown::onGraphicsButtonClick, this));
	mCommitCallbackRegistrar.add("Presets.RowClick", boost::bind(&LLPanelPresetsPulldown::onRowClick, this, _2));

	buildFromFile( "panel_presets_pulldown.xml");
}

BOOL LLPanelPresetsPulldown::postBuild()
{
	populatePanel();

	return LLPanel::postBuild();
}

void LLPanelPresetsPulldown::populatePanel()
{
	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_list");
	scroll->clearRows();

	std::string active_preset = gSavedSettings.getString("PresetGraphicActive");

	//BD - Look through our defaults first.
	std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "presets", "graphic");
	std::string file;
	LLDirIterator dir_iter_app(dir, "*.xml");
	while (dir_iter_app.next(file))
	{
		std::string path = gDirUtilp->add(dir, file);
		std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

		LLSD preset;
		llifstream infile;
		infile.open(path);
		if (!infile.is_open())
		{
			//BD - If we can't open and read the file we shouldn't add it because we won't
			//     be able to load it later.
			LL_WARNS("Settings") << "Cannot open file in: " << path << LL_ENDL;
			continue;
		}

		//BD - Camera Preset files only have one single line, so it's either a parse failure
		//     or a success.
		S32 ret = LLSDSerialize::fromXML(preset, infile);

		//BD - We couldn't parse the file, don't bother adding it.
		if (ret == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Settings") << "Failed to parse file: " << path << LL_ENDL;
			continue;
		}

		LLSD row;
		row["columns"][0]["column"] = "preset_name";
		row["columns"][0]["value"] = name;

		//BD
		row["columns"][1]["column"] = "icon";
		row["columns"][1]["type"] = "icon";

		bool is_selected_preset = false;
		if (name == active_preset)
		{
			//BD
			row["columns"][1]["value"] = "Checkbox_On";
			is_selected_preset = true;
		}
		//BD
		else
		{
			row["columns"][1]["value"] = "Checkbox_Off";
		}

		LLScrollListItem* new_item = scroll->addElement(row);
		new_item->setSelected(is_selected_preset);
	}
}

/*virtual*/
void LLPanelPresetsPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}

/*virtual*/
void LLPanelPresetsPulldown::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleMouseDown(x,y,mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleRightMouseDown(x, y, mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLPanel::handleDoubleClick(x, y, mask);
    return TRUE;
}

/*virtual*/
void LLPanelPresetsPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void LLPanelPresetsPulldown::onVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)	
	{
		mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
		populatePanel();
	}
	else
	{
		mHoverTimer.stop();
	}
}

void LLPanelPresetsPulldown::onRowClick(const LLSD& user_data)
{
	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_list");

	if (scroll)
	{
		LLScrollListItem* item = scroll->getFirstSelected();
		if (item)
		{
			std::string name = item->getColumn(1)->getValue().asString();

            LL_DEBUGS() << "selected '" << name << "'" << LL_ENDL;
			//BD
			gSavedSettings.loadPreset(1, name);
			gSavedSettings.setString("PresetGraphicActive", name);
			setVisible(FALSE);
		}
        else
        {
            LL_DEBUGS() << "none selected" << LL_ENDL;
        }
	}
    else
    {
        LL_DEBUGS() << "no scroll" << LL_ENDL;
    }
}

void LLPanelPresetsPulldown::onGraphicsButtonClick()
{
	// close the minicontrol, we're bringing up the big one
	setVisible(FALSE);

	// bring up the prefs floater
	gDragonLibrary.openPreferences("display");
}

//virtual
void LLPanelPresetsPulldown::draw()
{
	F32 alpha = mHoverTimer.getStarted() 
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), sAutoCloseFadeStartTimeSec, sAutoCloseTotalTimeSec, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	LLPanel::draw();

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}
}
