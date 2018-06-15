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

BDFloaterCamera::BDFloaterCamera(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Recorder.Command", boost::bind(&BDFloaterCamera::onCommand, this, _1, _2));
}

BDFloaterCamera::~BDFloaterCamera()
{
}

BOOL BDFloaterCamera::postBuild()
{
	//BD - Complexity
	mRecorderScroll = this->getChild<LLScrollListCtrl>("recorder_scroll", true);
	mRecorderScroll->setCommitOnSelectionChange(TRUE);
	mRecorderScroll->setCommitCallback(boost::bind(&BDFloaterCamera::onRecorderSelection, this));

	return TRUE;
}

void BDFloaterCamera::draw()
{
	//BD - We should do a refresh every second or so if we're still playing or recording.
	LLFloater::draw();
}

void BDFloaterCamera::onOpen(const LLSD& key)
{
	onRefreshRecorder();
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
void BDFloaterCamera::onCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		if (param.asString() == "Record")
		{
			//BD - Start the recorder.
			if (!gAgentPilot.isRecording())
			{
				gAgentPilot.startRecord();
			}
			else
			{
				gAgentPilot.stopRecord();
				onRefreshRecorder();
			}
		}

		if (param.asString() == "Playback")
		{
			//BD - Start the recorder.
			if (!gAgentPilot.isPlaying())
			{
				gAgentPilot.setNumRuns(-1);
				gAgentPilot.startPlayback();
			}
			else
			{
				gAgentPilot.stopPlayback();
			}
		}

		if (param.asString() == "Loop")
		{
			gAgentPilot.setLoop(ctrl->getValue());
		}

		if (param.asString() == "Refresh")
		{
			onRefreshRecorder();
		}

		if (param.asString() == "Save Camera")
		{
			gAgentPilot.addAction(LLAgentPilot::STRAIGHT);
			onActionsExport();
			//BD - We can do custom autopilot-less actions here.
			//LLAgentPilot::Action action;
			//action.mType = action_type;
			//action.mTarget = gAgent.getPositionGlobal();
			//action.mTime = mTimer.getElapsedTimeF32();
			//LLViewerCamera *cam = LLViewerCamera::getInstance();
			//action.mCameraView = cam->getView();
			//action.mCameraOrigin = cam->getOrigin();
			//action.mCameraXAxis = cam->getXAxis();
			//action.mCameraYAxis = cam->getYAxis();
			//action.mCameraZAxis = cam->getZAxis();
			//mLastRecordTime = (F32)action.mTime;
			//LLAgentPilot::mActions.push_back(action);
		}

		if (param.asString() == "Load Camera")
		{
		}

		if (param.asString() == "Save Position")
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
			onActionsExport();
		}

		if (param.asString() == "Save Rotation")
		{
			LLScrollListItem* item = mRecorderScroll->getFirstSelected();
			LLScrollListCell* column_4 = item->getColumn(4);
			LLScrollListCell* column_5 = item->getColumn(5);
			LLScrollListCell* column_6 = item->getColumn(6);
			LLScrollListCell* column_7 = item->getColumn(7);
			LLScrollListCell* column_8 = item->getColumn(8);
			LLScrollListCell* column_9 = item->getColumn(9);
			LLScrollListCell* column_10 = item->getColumn(10);
			LLScrollListCell* column_11 = item->getColumn(11);
			LLScrollListCell* column_12 = item->getColumn(12);

			LLViewerCamera *cam = LLViewerCamera::getInstance();

			LLVector3 rotation = cam->getXAxis();
			column_4->setValue(rotation.mV[VX]);
			column_5->setValue(rotation.mV[VY]);
			column_6->setValue(rotation.mV[VZ]);
			rotation = cam->getYAxis();
			column_7->setValue(rotation.mV[VX]);
			column_8->setValue(rotation.mV[VY]);
			column_9->setValue(rotation.mV[VZ]);
			rotation = cam->getZAxis();
			column_10->setValue(rotation.mV[VX]);
			column_11->setValue(rotation.mV[VY]);
			column_12->setValue(rotation.mV[VZ]);
			onActionsExport();
		}

		if (param.asString() == "Remove")
		{
			mRecorderScroll->deleteSelectedItems();
			onActionsExport();
		}

		if (param.asString() == "Export")
		{
			onActionsExport();
		}

		if (param.asString() == "Import")
		{
			gAgentPilot.load();
		}

		onRecorderControlsRefresh();
	}
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
		getChild<LLUICtrl>("Rotation_X_X")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_X_Y")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_X_Z")->setEnabled(true);

		getChild<LLUICtrl>("Rotation_Y_X")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_Y_Y")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_Y_Z")->setEnabled(true);

		getChild<LLUICtrl>("Rotation_Z_X")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_Z_Y")->setEnabled(true);
		getChild<LLUICtrl>("Rotation_Z_Z")->setEnabled(true);

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

void BDFloaterCamera::onRefreshRecorder()
{
	mRecorderScroll->clearRows();
	S32 count = 0;
	std::vector<LLAgentPilot::Action> actions = gAgentPilot.getActions();
	for (std::vector<LLAgentPilot::Action>::iterator iter = actions.begin();
		iter != actions.end(); ++iter)
	{
		LLAgentPilot::Action action = *iter;
		{
			LLVector3 pos = action.mCameraOrigin;
			LLVector3 rot_x = action.mCameraXAxis;
			LLVector3 rot_y = action.mCameraYAxis;
			LLVector3 rot_z = action.mCameraZAxis;
			LLVector3d target = action.mTarget;

			LLSD row;
			row["columns"][0]["column"] = "number";
			row["columns"][0]["value"] = count;
			row["columns"][1]["column"] = "pos_x";
			row["columns"][1]["value"] = pos.mV[VX];
			row["columns"][2]["column"] = "pos_y";
			row["columns"][2]["value"] = pos.mV[VY];
			row["columns"][3]["column"] = "pos_z";
			row["columns"][3]["value"] = pos.mV[VZ];
			row["columns"][4]["column"] = "rot_x_x";
			row["columns"][4]["value"] = rot_x.mV[VX];
			row["columns"][5]["column"] = "rot_x_y";
			row["columns"][5]["value"] = rot_x.mV[VY];
			row["columns"][6]["column"] = "rot_x_z";
			row["columns"][6]["value"] = rot_x.mV[VZ];
			row["columns"][7]["column"] = "rot_y_x";
			row["columns"][7]["value"] = rot_y.mV[VX];
			row["columns"][8]["column"] = "rot_y_y";
			row["columns"][8]["value"] = rot_y.mV[VY];
			row["columns"][9]["column"] = "rot_y_z";
			row["columns"][9]["value"] = rot_y.mV[VZ];
			row["columns"][10]["column"] = "rot_z_x";
			row["columns"][10]["value"] = rot_z.mV[VX];
			row["columns"][11]["column"] = "rot_z_y";
			row["columns"][11]["value"] = rot_z.mV[VY];
			row["columns"][12]["column"] = "rot_z_z";
			row["columns"][12]["value"] = rot_z.mV[VZ];
			row["columns"][13]["column"] = "target_x";
			row["columns"][13]["value"] = target.mdV[VX];
			row["columns"][14]["column"] = "target_y";
			row["columns"][14]["value"] = target.mdV[VY];
			row["columns"][15]["column"] = "target_z";
			row["columns"][15]["value"] = target.mdV[VZ];
			row["columns"][16]["column"] = "view";
			row["columns"][16]["value"] = action.mCameraView;
			row["columns"][17]["column"] = "type";
			row["columns"][17]["value"] = action.mType;
			row["columns"][18]["column"] = "time";
			row["columns"][18]["value"] = action.mTime;
			mRecorderScroll->addElement(row);
			count++;
		}
	}
}

void BDFloaterCamera::onRecorderSelection()
{
	LLScrollListItem* item = mRecorderScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("Rotation_X_X")->setValue(item->getColumn(4)->getValue());
		getChild<LLUICtrl>("Rotation_X_Y")->setValue(item->getColumn(5)->getValue());
		getChild<LLUICtrl>("Rotation_X_Z")->setValue(item->getColumn(6)->getValue());

		getChild<LLUICtrl>("Rotation_Y_X")->setValue(item->getColumn(7)->getValue());
		getChild<LLUICtrl>("Rotation_Y_Y")->setValue(item->getColumn(8)->getValue());
		getChild<LLUICtrl>("Rotation_Y_Z")->setValue(item->getColumn(9)->getValue());

		getChild<LLUICtrl>("Rotation_Z_X")->setValue(item->getColumn(10)->getValue());
		getChild<LLUICtrl>("Rotation_Z_Y")->setValue(item->getColumn(11)->getValue());
		getChild<LLUICtrl>("Rotation_Z_Z")->setValue(item->getColumn(12)->getValue());

		getChild<LLUICtrl>("Target_X")->setValue(item->getColumn(13)->getValue());
		getChild<LLUICtrl>("Target_Y")->setValue(item->getColumn(14)->getValue());
		getChild<LLUICtrl>("Target_Z")->setValue(item->getColumn(15)->getValue());

		getChild<LLUICtrl>("Camera_View")->setValue(item->getColumn(16)->getValue());
		getChild<LLUICtrl>("Action_Type")->setValue(item->getColumn(17)->getValue());
		getChild<LLUICtrl>("Transition_Time")->setValue(item->getColumn(18)->getValue());

		getChild<LLUICtrl>("save_rotation_btn")->setEnabled(true);
		getChild<LLUICtrl>("save_position_btn")->setEnabled(true);
		getChild<LLUICtrl>("remove_btn")->setEnabled(true);
	}
}

void BDFloaterCamera::onActionsExport()
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

			action.mCameraXAxis = LLVector3(item->getColumn(4)->getValue().asReal(),
											item->getColumn(5)->getValue().asReal(),
											item->getColumn(6)->getValue().asReal());

			action.mCameraYAxis = LLVector3(item->getColumn(7)->getValue().asReal(),
											item->getColumn(8)->getValue().asReal(),
											item->getColumn(9)->getValue().asReal());

			action.mCameraZAxis = LLVector3(item->getColumn(10)->getValue().asReal(),
											item->getColumn(11)->getValue().asReal(),
											item->getColumn(12)->getValue().asReal());

			action.mTarget = LLVector3d(item->getColumn(13)->getValue().asReal(),
											item->getColumn(14)->getValue().asReal(),
											item->getColumn(15)->getValue().asReal());

			action.mCameraView = item->getColumn(16)->getValue().asReal();
			action.mType = LLAgentPilot::EActionType::STRAIGHT;
			action.mTime = item->getColumn(18)->getValue().asReal();

			actions.push_back(action);
		}
	}

	gAgentPilot.setActions(actions);
}