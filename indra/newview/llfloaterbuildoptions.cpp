/** 
 * @file llfloaterbuildoptions.cpp
 * @brief LLFloaterBuildOptions class implementation
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

/**
 * Panel for setting global object-editing options, specifically
 * grid size and spacing.
 */ 

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuildoptions.h"
#include "lluictrlfactory.h"

#include "llcombobox.h"
#include "llselectmgr.h"
#include "llviewercontrol.h"

#include "llfloatertools.h"
#include "llfloaterreg.h"

//
// Methods
//

void commit_grid_mode(LLUICtrl *ctrl)
{
	LLComboBox* combo = (LLComboBox*)ctrl;

	LLSelectMgr::getInstance()->setGridMode((EGridMode)combo->getCurrentIndex());
}

LLFloaterBuildOptions::LLFloaterBuildOptions(const LLSD& key)
	: LLFloater(key),
	mComboGridMode(NULL)
{
	mCommitCallbackRegistrar.add("BuildTool.toggleLightRadius", boost::bind(&LLFloaterBuildOptions::toggleLightRadius, this, _1));
	mCommitCallbackRegistrar.add("BuildTool.toggleSelectSurrounding", boost::bind(&LLFloaterBuildOptions::toggleSelectSurrounding, this, _1));
	mCommitCallbackRegistrar.add("BuildTool.gridMode", boost::bind(&commit_grid_mode, _1));
}

LLFloaterBuildOptions::~LLFloaterBuildOptions()
{}

BOOL LLFloaterBuildOptions::postBuild()
{
	mComboGridMode = getChild<LLComboBox>("combobox grid mode");

	return TRUE;
}

// virtual
void LLFloaterBuildOptions::onOpen(const LLSD& key)
{
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	refreshGridMode();
}

// virtual
void LLFloaterBuildOptions::onClose(bool app_quitting)
{
	mObjectSelection = NULL;
}

// static
void LLFloaterBuildOptions::setGridMode(S32 mode)
{
	LLFloaterBuildOptions* options_floater = LLFloaterReg::getTypedInstance<LLFloaterBuildOptions>("build_options");
	if (!options_floater || !options_floater->mComboGridMode)
	{
		return;
	}

	options_floater->mComboGridMode->setCurrentByIndex(mode);
}

//static
void LLFloaterBuildOptions::refreshGridMode()
{
	LLFloaterBuildOptions* options_floater = LLFloaterReg::getTypedInstance<LLFloaterBuildOptions>("build_options");
	if (!options_floater || !options_floater->mComboGridMode)
	{
		return;
	}

	LLComboBox* combo = options_floater->mComboGridMode;
	S32 index = combo->getCurrentIndex();
	combo->removeall();

	switch (options_floater->mObjectSelection->getSelectType())
	{
	case SELECT_TYPE_HUD:
		combo->add(options_floater->getString("grid_screen_text"));
		combo->add(options_floater->getString("grid_local_text"));
		break;
	case SELECT_TYPE_WORLD:
		combo->add(options_floater->getString("grid_world_text"));
		combo->add(options_floater->getString("grid_local_text"));
		combo->add(options_floater->getString("grid_reference_text"));
		break;
	case SELECT_TYPE_ATTACHMENT:
		combo->add(options_floater->getString("grid_attachment_text"));
		combo->add(options_floater->getString("grid_local_text"));
		combo->add(options_floater->getString("grid_reference_text"));
		break;
	}

	combo->setCurrentByIndex(index);
}

void LLFloaterBuildOptions::toggleLightRadius(LLUICtrl* ctrl)
{
	// TomY TODO merge these
	LLSelectMgr::sRenderLightRadius = ctrl->getValue();
}

void LLFloaterBuildOptions::toggleSelectSurrounding(LLUICtrl* ctrl)
{
	// TomY TODO merge these
	LLSelectMgr::sRectSelectInclusive = ctrl->getValue();
}