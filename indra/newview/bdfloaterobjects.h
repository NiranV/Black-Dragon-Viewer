/**
*
* Copyright (C) 2018, NiranV Dean
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


#ifndef BD_FLOATER_OBJECTS_H
#define BD_FLOATER_OBJECTS_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llsafehandle.h"
#include "llselectmgr.h"

class BDFloaterObjects :
	public LLFloater
{
	friend class LLFloaterReg;
private:
	BDFloaterObjects(const LLSD& key);
	/*virtual*/	~BDFloaterObjects();
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	void onObjectRefresh();
	void onObjectCommand(LLUICtrl* ctrl, const LLSD& param);
	void onLightCommand(LLUICtrl* ctrl, const LLSD& param);
	void onAlphaCommand(LLUICtrl* ctrl, const LLSD& param);

	void onSelectEntry(S32 mode);
	void onSelectObject(S32 mode);

	std::array<LLScrollListCtrl*, 3> mObjectsScroll;

	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

#endif
