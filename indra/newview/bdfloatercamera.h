/**
*
* Copyright (C) 2017, NiranV Dean
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
*/


#ifndef BD_FLOATER_CAMERA_H
#define BD_FLOATER_CAMERA_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llagentpilot.h"

class LLAgentPilot::Action;

class BDFloaterCamera :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterCamera(const LLSD& key);
	/*virtual*/	~BDFloaterCamera();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();

	void onCommand(LLUICtrl* ctrl, const LLSD& param);
	void onRefreshRecorder();
	void onRecorderSelection();

	void onActionsExport();
	//void onActionsImport();

	void onRecorderControlsRefresh();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	//BD - Complexity
	LLScrollListCtrl*				mRecorderScroll;
};

#endif
