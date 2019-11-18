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

#include "llviewerprecompiledheaders.h"

#include "bdfloatercamera.h"
#include "lluictrlfactory.h"
#include "llagent.h"

#include "llagentpilot.h"
#include "llviewercamera.h"
#include "llcombobox.h"

#include "bdfunctions.h"

BDFloaterCamera::BDFloaterCamera(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Camera and Agent Recorder Commands
	//     Export		- Save the current list out as xml/txt file.
	mCommitCallbackRegistrar.add("Recorder.Export", boost::bind(&BDFloaterCamera::onExport, this));
	//     Import		- Load any existing xml/txt file and add everything to the list.
	mCommitCallbackRegistrar.add("Recorder.Import", boost::bind(&BDFloaterCamera::onImport, this));
	//     Override		- Toggle whether the current record list should be looped.
	mCommitCallbackRegistrar.add("Recorder.Override", boost::bind(&BDFloaterCamera::toggleOverride, this, _1, _2));
	//     Loop			- Toggle whether the current record list should be looped.
	mCommitCallbackRegistrar.add("Recorder.Loop", boost::bind(&BDFloaterCamera::toggleLoop, this, _1, _2));
	//     Start		- Start the playback of all records.
	//     Stop			- Stop the playback of all records.
	mCommitCallbackRegistrar.add("Recorder.StartStop", boost::bind(&BDFloaterCamera::onStartStop, this, _1, _2));
	//     Record		- Start/Stop recording.
	mCommitCallbackRegistrar.add("Recorder.Record", boost::bind(&BDFloaterCamera::onRecord, this, _1, _2));
	//     Refresh		- Refresh the current list.
	mCommitCallbackRegistrar.add("Recorder.Refresh", boost::bind(&BDFloaterCamera::onRefresh, this));
	//     Delete		- Delete all currently selected records.
	mCommitCallbackRegistrar.add("Recorder.Delete", boost::bind(&BDFloaterCamera::onDelete, this));
	//     Waypoint		- Add a default waypoint with the current position and rotation.
	mCommitCallbackRegistrar.add("Recorder.Waypoint", boost::bind(&BDFloaterCamera::addWaypoint, this));
	//     Position		- Save the current camera position into the selected record.
	mCommitCallbackRegistrar.add("Recorder.Position", boost::bind(&BDFloaterCamera::savePosition, this));
	//     Rotation		- Save the current camera rotation into the selected record.
	mCommitCallbackRegistrar.add("Recorder.Rotation", boost::bind(&BDFloaterCamera::saveRotation, this));
	//     ModPosCamera - Configure the currently selected record's camera position.
	mCommitCallbackRegistrar.add("Recorder.CamPos", boost::bind(&BDFloaterCamera::onCameraPos, this, _1, _2));
	//     ModRotCamera - Configure the currently selected record's camera rotation.
	mCommitCallbackRegistrar.add("Recorder.CamRot", boost::bind(&BDFloaterCamera::onCameraRot, this, _1, _2));
	//     ModPosTarget - Configure the currently selected record's camera target.
	mCommitCallbackRegistrar.add("Recorder.TargetPos", boost::bind(&BDFloaterCamera::onTargetPos, this, _1, _2));
	//     ModTime		- Configure the currently selected record's time.
	mCommitCallbackRegistrar.add("Recorder.Time", boost::bind(&BDFloaterCamera::onTime, this, _1, _2));
	//     ModFoVCamera - Configrue the currently selected record's field of view (camera angle).
	mCommitCallbackRegistrar.add("Recorder.CamAngle", boost::bind(&BDFloaterCamera::onCameraAngle, this, _1, _2));

	//mCommitCallbackRegistrar.add("Recorder.Command", boost::bind(&BDFloaterCamera::onCommand, this, _1, _2));
}

BDFloaterCamera::~BDFloaterCamera()
{
}

BOOL BDFloaterCamera::postBuild()
{
	mRecorderScroll = this->getChild<LLScrollListCtrl>("recorder_scroll", true);
	mRecorderScroll->setCommitOnSelectionChange(TRUE);
	mRecorderScroll->setCommitCallback(boost::bind(&BDFloaterCamera::onRecorderSelection, this));

	return TRUE;
}

void BDFloaterCamera::draw()
{
	if (gDragonLibrary.getCameraOverride())
		updateCamera();

	//BD - We should do a refresh every second or so if we're still playing or recording.
	LLFloater::draw();
}

void BDFloaterCamera::updateCamera()
{
	LLViewerCamera* viewer_cam = LLViewerCamera::getInstance();
	viewer_cam->setOrigin(mOrigin);
	viewer_cam->mXAxis = mXAxis;
	viewer_cam->mYAxis = mYAxis;
	viewer_cam->mZAxis = mZAxis;
	viewer_cam->setView(mView);
}

void BDFloaterCamera::onOpen(const LLSD& key)
{
	LLViewerCamera* viewer_cam = LLViewerCamera::getInstance();
	if (!gDragonLibrary.getCameraOverride())
	{
		mOrigin = viewer_cam->mOrigin;
		mXAxis = viewer_cam->mXAxis;
		mYAxis = viewer_cam->mYAxis;
		mZAxis = viewer_cam->mZAxis;
		mView = viewer_cam->getView();
	}

	onRefresh();
	//onRefreshRecorder();
	onRecorderControlsRefresh();
}

void BDFloaterCamera::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mRecorderScroll->clearRows();
}

////////////////////////////////
//BD - Recorder
////////////////////////////////
void BDFloaterCamera::onExport()
{
	std::vector<LLScrollListItem*> items = mRecorderScroll->getAllData();
	std::vector<LLAgentPilot::Action> actions;
	//BD - Now lets do it the other way around and look for new things to add.
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin();
		iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item)
		{
			LLAgentPilot::Action action;

			action.mCameraOrigin = LLVector3(item->getColumn(1)->getValue().asReal(),
				item->getColumn(2)->getValue().asReal(),
				item->getColumn(3)->getValue().asReal());

			LLQuaternion quat;
			LLVector3 rot;
			rot.mV[VX] = item->getColumn(4)->getValue().asReal();
			rot.mV[VY] = item->getColumn(5)->getValue().asReal();
			rot.mV[VZ] = item->getColumn(6)->getValue().asReal();
			quat.setEulerAngles(rot.mV[VX], rot.mV[VY], rot.mV[VZ]);
			LLMatrix3 mat = quat.getMatrix3();

			action.mCameraXAxis = mat.getFwdRow();
			action.mCameraYAxis = mat.getLeftRow();
			action.mCameraZAxis = mat.getUpRow();

			action.mTarget = LLVector3d(item->getColumn(7)->getValue().asReal(),
				item->getColumn(8)->getValue().asReal(),
				item->getColumn(9)->getValue().asReal());

			action.mCameraView = item->getColumn(10)->getValue().asReal();
			action.mType = LLAgentPilot::EActionType::STRAIGHT;
			action.mTime = item->getColumn(12)->getValue().asReal();

			actions.push_back(action);
		}
	}

	gAgentPilot.setActions(actions);
}

void BDFloaterCamera::onImport()
{
	gAgentPilot.load();
}

void BDFloaterCamera::toggleOverride(LLUICtrl* ctrl, const LLSD& param)
{ 
	gDragonLibrary.mCameraOverride = ctrl->getValue().asBoolean(); 

	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (!item)
		return;


};

void BDFloaterCamera::toggleLoop(LLUICtrl* ctrl, const LLSD& param)
{
	gAgentPilot.setLoop(ctrl->getValue());
}

void BDFloaterCamera::onStartStop(LLUICtrl* ctrl, const LLSD& param)
{
	bool playing = gAgentPilot.isPlaying();
	bool val = ctrl->getValue().asBoolean();
	//BD - Start the playback.
	if (val)
	{
		if (!playing)
		{
			gAgentPilot.setNumRuns(-1);
			gAgentPilot.startPlayback();
		}
	}
	else
	{
		if (playing)
			gAgentPilot.stopPlayback();
	}
}

void BDFloaterCamera::onRecord(LLUICtrl* ctrl, const LLSD& param)
{
	bool recording = gAgentPilot.isRecording();
	bool val = ctrl->getValue().asBoolean();
	//BD - Start the recorder.
	if (val)
	{
		if (!recording)
			gAgentPilot.startRecord();
	}
	else
	{
		if (recording)
		{
			gAgentPilot.stopRecord();
			onRefresh();
		}
	}
}

void BDFloaterCamera::onRefresh()
{
	mRecorderScroll->clearRows();
	S32 count = 0;
	std::vector<LLAgentPilot::Action> actions = gAgentPilot.getActions();
	for (std::vector<LLAgentPilot::Action>::iterator iter = actions.begin();
		iter != actions.end(); ++iter)
	{
		LLAgentPilot::Action action = *iter;
		{
			mCurrentAction = action;
			LLVector3 pos = action.mCameraOrigin;
			LLVector3 rot_x = action.mCameraXAxis;
			LLVector3 rot_y = action.mCameraYAxis;
			LLVector3 rot_z = action.mCameraZAxis;
			LLVector3d target = action.mTarget;
			LLQuaternion quat = LLQuaternion(rot_x, rot_y, rot_z);
			LLVector3 rotation_euler;
			quat.getEulerAngles(&rotation_euler.mV[VX], &rotation_euler.mV[VY], &rotation_euler.mV[VZ]);
			LLSD row;
			row["columns"][0]["column"] = "number";
			row["columns"][0]["value"] = count;
			row["columns"][1]["column"] = "pos_x";
			row["columns"][1]["value"] = pos.mV[VX];
			row["columns"][2]["column"] = "pos_y";
			row["columns"][2]["value"] = pos.mV[VY];
			row["columns"][3]["column"] = "pos_z";
			row["columns"][3]["value"] = pos.mV[VZ];
			row["columns"][4]["column"] = "rot_x";
			row["columns"][4]["value"] = rotation_euler.mV[VX];
			row["columns"][5]["column"] = "rot_y";
			row["columns"][5]["value"] = rotation_euler.mV[VY];
			row["columns"][6]["column"] = "rot_z";
			row["columns"][6]["value"] = rotation_euler.mV[VZ];
			row["columns"][7]["column"] = "target_x";
			row["columns"][7]["value"] = target.mdV[VX];
			row["columns"][8]["column"] = "target_y";
			row["columns"][8]["value"] = target.mdV[VY];
			row["columns"][9]["column"] = "target_z";
			row["columns"][9]["value"] = target.mdV[VZ];
			row["columns"][10]["column"] = "view";
			row["columns"][10]["value"] = action.mCameraView;
			row["columns"][11]["column"] = "type";
			row["columns"][11]["value"] = action.mType;
			row["columns"][12]["column"] = "time";
			row["columns"][12]["value"] = action.mTime;
			/*LLScrollListItem* item =*/ mRecorderScroll->addElement(row);
			//item->setUserdata(quat);
			count++;
		}
	}
}

void BDFloaterCamera::onDelete()
{
	mRecorderScroll->deleteSelectedItems();
	onExport();
}

void BDFloaterCamera::addWaypoint()
{
	gAgentPilot.addAction(LLAgentPilot::EActionType::STRAIGHT);
	gAgentPilot.save();
	onRefresh();
}

void BDFloaterCamera::savePosition()
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	LLScrollListCell* column_1 = item->getColumn(1);
	LLScrollListCell* column_2 = item->getColumn(2);
	LLScrollListCell* column_3 = item->getColumn(3);

	LLViewerCamera *cam = LLViewerCamera::getInstance();

	LLVector3 origin = cam->getOrigin();
	column_1->setValue(origin.mV[VX]);
	column_2->setValue(origin.mV[VY]);
	column_3->setValue(origin.mV[VZ]);
	onExport();
}

void BDFloaterCamera::saveRotation()
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	LLScrollListCell* column_4 = item->getColumn(4);
	LLScrollListCell* column_5 = item->getColumn(5);
	LLScrollListCell* column_6 = item->getColumn(6);

	LLViewerCamera *cam = LLViewerCamera::getInstance();
	LLQuaternion quat = cam->getQuaternion();
	LLVector3 rotation;
	quat.getEulerAngles(&rotation.mV[VX], &rotation.mV[VY], &rotation.mV[VZ]);
	column_4->setValue(rotation.mV[VX]);
	column_5->setValue(rotation.mV[VY]);
	column_6->setValue(rotation.mV[VZ]);
	onExport();
}

void BDFloaterCamera::onCameraPos(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (!item)
		return;

	LLScrollListCell* column_1 = item->getColumn(1);
	LLScrollListCell* column_2 = item->getColumn(2);
	LLScrollListCell* column_3 = item->getColumn(3);
	F32 val = ctrl->getValue().asReal();
	if (param.asString() == "X")
	{
		mOrigin.mV[VX] = val;
		column_1->setValue(val);
	}
	else if (param.asString() == "Y")
	{
		mOrigin.mV[VY] = val;
		column_2->setValue(val);
	}
	else
	{
		mOrigin.mV[VZ] = val;
		column_3->setValue(val);
	}
}

void BDFloaterCamera::onCameraRot(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (!item)
		return;

	LLViewerCamera *cam = LLViewerCamera::getInstance();
	F32 val = ctrl->getValue().asReal();
	S32 axis = param.asInteger();
	LLScrollListCell* cell[3] = { item->getColumn(4), item->getColumn(5), item->getColumn(6) };
	LLQuaternion rot_quat = cam->getQuaternion();
	LLMatrix3 rot_mat;
	F32 old_value;
	LLVector3 vec3;
	old_value = mPrevValue[axis];
	mPrevValue[axis] = val;
	val -= old_value;
	vec3.mV[axis] = val;
	rot_mat = LLMatrix3(vec3.mV[VX], vec3.mV[VY], vec3.mV[VZ]);
	rot_quat = LLQuaternion(rot_mat)*rot_quat;
	LLMatrix3 mat(rot_quat);

	mXAxis = LLVector3(mat.mMatrix[0]);
	mYAxis = LLVector3(mat.mMatrix[1]);
	mZAxis = LLVector3(mat.mMatrix[2]);

	LLVector3 euler;
	rot_quat.getEulerAngles(&euler.mV[VX], &euler.mV[VY], &euler.mV[VZ]);
	cell[0]->setValue(ll_round(euler.mV[VX], 0.001f));
	cell[1]->setValue(ll_round(euler.mV[VY], 0.001f));
	cell[2]->setValue(ll_round(euler.mV[VZ], 0.001f));

	mCurrentAction.mCameraXAxis = mXAxis;
	mCurrentAction.mCameraYAxis = mYAxis;
	mCurrentAction.mCameraZAxis = mZAxis;
}

void BDFloaterCamera::onTargetPos(LLUICtrl* ctrl, const LLSD& param)
{}
void BDFloaterCamera::onTime(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (!item)
		return;

	LLScrollListCell* column_12 = item->getColumn(12);
	column_12->setValue(ctrl->getValue().asReal());
	mCurrentAction.mTime = ctrl->getValue().asReal();
	//onExport();
}

void BDFloaterCamera::onCameraAngle(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (!item)
		return;

	LLScrollListCell* column_10 = item->getColumn(10);
	column_10->setValue(ctrl->getValue().asReal());
	LLViewerCamera *cam = LLViewerCamera::getInstance();
	cam->setView(ctrl->getValue().asReal());
	mCurrentAction.mCameraView = ctrl->getValue().asReal();
	//onExport();
}

void BDFloaterCamera::onRecorderControlsRefresh()
{
	if (mRecorderScroll->getItemCount() != 0)
	{
		getChild<LLUICtrl>("export_btn")->setEnabled(true);
		getChild<LLUICtrl>("playback_btn")->setEnabled(true);
	}
	else
	{
		getChild<LLUICtrl>("save_rotation_btn")->setEnabled(false);
		getChild<LLUICtrl>("save_position_btn")->setEnabled(false);
		getChild<LLUICtrl>("remove_btn")->setEnabled(false);
		getChild<LLUICtrl>("export_btn")->setEnabled(false);
		getChild<LLUICtrl>("playback_btn")->setEnabled(false);
	}

	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (item)
	{
		LLScrollListCell* column_4 = item->getColumn(4);
		LLScrollListCell* column_5 = item->getColumn(5);
		LLScrollListCell* column_6 = item->getColumn(6);

		getChild<LLUICtrl>("Rotation_X")->setValue(column_4->getValue());
		getChild<LLUICtrl>("Rotation_Y")->setValue(column_5->getValue());
		getChild<LLUICtrl>("Rotation_Z")->setValue(column_6->getValue());

		LLScrollListItem* item = mRecorderScroll->getFirstSelected();
		if (item)
		{
			LLScrollListCell* column_1 = item->getColumn(1);
			LLScrollListCell* column_2 = item->getColumn(2);
			LLScrollListCell* column_3 = item->getColumn(3);

			getChild<LLUICtrl>("Position_X")->setValue(column_1->getValue());
			getChild<LLUICtrl>("Position_Y")->setValue(column_2->getValue());
			getChild<LLUICtrl>("Position_Z")->setValue(column_3->getValue());
		}

		getChild<LLUICtrl>("Target_X")->setEnabled(true);
		getChild<LLUICtrl>("Target_Y")->setEnabled(true);
		getChild<LLUICtrl>("Target_Z")->setEnabled(true);

		getChild<LLUICtrl>("Camera_View")->setEnabled(true);
		getChild<LLUICtrl>("Action_Type")->setEnabled(true);
		getChild<LLUICtrl>("Transition_Time")->setEnabled(true);

		getChild<LLUICtrl>("load_camera")->setEnabled(true);
	}
	else
	{
		getChild<LLUICtrl>("Rotation_X_X")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_X_Y")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_X_Z")->setEnabled(false);

		getChild<LLUICtrl>("Rotation_Y_X")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_Y_Y")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_Y_Z")->setEnabled(false);

		getChild<LLUICtrl>("Rotation_Z_X")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_Z_Y")->setEnabled(false);
		getChild<LLUICtrl>("Rotation_Z_Z")->setEnabled(false);

		getChild<LLUICtrl>("Target_X")->setEnabled(false);
		getChild<LLUICtrl>("Target_Y")->setEnabled(false);
		getChild<LLUICtrl>("Target_Z")->setEnabled(false);

		getChild<LLUICtrl>("Camera_View")->setEnabled(false);
		getChild<LLUICtrl>("Action_Type")->setEnabled(false);
		getChild<LLUICtrl>("Transition_Time")->setEnabled(false);

		getChild<LLUICtrl>("load_camera")->setEnabled(false);
	}

	getChild<LLUICtrl>("loop_btn")->setValue(gAgentPilot.getLoop());
}

void BDFloaterCamera::onRecorderSelection()
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (item)
	{

		getChild<LLUICtrl>("Position_X")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("Position_Y")->setValue(item->getColumn(2)->getValue());
		getChild<LLUICtrl>("Position_Z")->setValue(item->getColumn(3)->getValue());

		getChild<LLUICtrl>("Rotation_X")->setValue(item->getColumn(4)->getValue());
		getChild<LLUICtrl>("Rotation_Y")->setValue(item->getColumn(5)->getValue());
		getChild<LLUICtrl>("Rotation_Z")->setValue(item->getColumn(6)->getValue());

		LLQuaternion quat;
		quat.setEulerAngles(item->getColumn(4)->getValue().asReal(), item->getColumn(5)->getValue().asReal(), item->getColumn(6)->getValue().asReal());
		LLMatrix3 rot_mat(quat);
		mXAxis = LLVector3(rot_mat.mMatrix[0]);
		mYAxis = LLVector3(rot_mat.mMatrix[1]);
		mZAxis = LLVector3(rot_mat.mMatrix[2]);
		mPrevValue[0] = item->getColumn(4)->getValue().asReal();
		mPrevValue[1] = item->getColumn(5)->getValue().asReal();
		mPrevValue[2] = item->getColumn(6)->getValue().asReal();

		getChild<LLUICtrl>("Target_X")->setValue(item->getColumn(7)->getValue());
		getChild<LLUICtrl>("Target_Y")->setValue(item->getColumn(8)->getValue());
		getChild<LLUICtrl>("Target_Z")->setValue(item->getColumn(9)->getValue());

		getChild<LLUICtrl>("Camera_View")->setValue(item->getColumn(10)->getValue());
		getChild<LLUICtrl>("Action_Type")->setValue(item->getColumn(11)->getValue());
		getChild<LLUICtrl>("Transition_Time")->setValue(item->getColumn(12)->getValue());

		getChild<LLUICtrl>("save_rotation_btn")->setEnabled(true);
		getChild<LLUICtrl>("save_position_btn")->setEnabled(true);
		getChild<LLUICtrl>("remove_btn")->setEnabled(true);
	}
}