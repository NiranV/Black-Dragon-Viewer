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

	void updateCamera();

	//BD - Camera and Agent Recorder Commands
	//     Export		- Save the current list out as xml/txt file.
	//     Import		- Load any existing xml/txt file and add everything to the list.
	//     Override		- Toggle whether the current record list should be looped.
	//     Loop			- Toggle whether the current record list should be looped.
	//     Start		- Start the playback of all records.
	//     Stop			- Stop the playback of all records.
	//     Record		- Start/Stop recording.
	//     Refresh		- Refresh the current list.
	//     Delete		- Delete all currently selected records.
	//     Waypoint		- Add a default waypoint with the current position and rotation.
	//     Position		- Save the current camera position into the selected record.
	//     Rotation		- Save the current camera rotation into the selected record.
	//     ModPosCamera - Configure the currently selected record's camera position.
	//     ModRotCamera - Configure the currently selected record's camera rotation.
	//     ModPosTarget - Configure the currently selected record's camera target.
	//     ModTime		- Configure the currently selected record's time.
	//     ModFoVCamera - Configrue the currently selected record's field of view (camera angle).
	void onExport();
	void onImport();

	void toggleOverride(LLUICtrl* ctrl, const LLSD& param);
	void toggleLoop(LLUICtrl* ctrl, const LLSD& param);

	void onStartStop(LLUICtrl* ctrl, const LLSD& param);
	void onRecord(LLUICtrl* ctrl, const LLSD& param);
	void onRefresh();

	void onDelete();
	void addWaypoint();
	void savePosition();
	void saveRotation();

	void onCameraPos(LLUICtrl* ctrl, const LLSD& param);
	void onCameraRot(LLUICtrl* ctrl, const LLSD& param);
	void onTargetPos(LLUICtrl* ctrl, const LLSD& param);
	void onTime(LLUICtrl* ctrl, const LLSD& param);
	void onCameraAngle(LLUICtrl* ctrl, const LLSD& param);

	//void onCommand(LLUICtrl* ctrl, const LLSD& param);
	//void onRefreshRecorder();
	void onRecorderSelection();

	//void onActionsExport();
	//void onActionsImport();

	void onRecorderControlsRefresh();

	void copyToClipboard();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	LLScrollListCtrl*				mRecorderScroll;

	LLVector3 mXAxis;
	LLVector3 mYAxis;
	LLVector3 mZAxis;
	LLVector3 mOrigin;
	LLQuaternion mRotation;
	F32 mView;
	F32 mPrevValue[3];
	LLAgentPilot::Action mCurrentAction;
};

#endif
