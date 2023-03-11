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

#include "bdfloaterposer.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llclipboard.h"
#include "lldatapacker.h"
#include "lldiriterator.h"
#include "llfilepicker.h"
#include "llfilesystem.h"
#include "llkeyframemotion.h"
#include "llnotificationsutil.h"
#include "llmenugl.h"
#include "llmenubutton.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llviewerjointattachment.h"
#include "llviewerjoint.h"
#include "llvoavatarself.h"
#include "llwindowwin32.h"
#include "pipeline.h"

//BD - Animesh Support
#include "llcontrolavatar.h"

//BD - Black Dragon specifics
#include "bdanimator.h"
#include "bdfunctions.h"
#include "bdposingmotion.h"
#include "bdstatus.h"


BDFloaterPoser::BDFloaterPoser(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Save our current pose as XML or ANIM file to be used or uploaded later.
	mCommitCallbackRegistrar.add("Pose.Save", boost::bind(&BDFloaterPoser::onClickPoseSave, this, _2));
	//BD - Start our custom pose.
	mCommitCallbackRegistrar.add("Pose.Start", boost::bind(&BDFloaterPoser::onPoseStart, this));
	//BD - Load the current pose and export all its values into the UI so we can alter them.
	mCommitCallbackRegistrar.add("Pose.Load", boost::bind(&BDFloaterPoser::onPoseLoad, this));
	//BD - Import ANIM file to the poser.
	mCommitCallbackRegistrar.add("Pose.Import", boost::bind(&BDFloaterPoser::onPoseImport, this));
	//BD - Delete the currently selected Pose.
	mCommitCallbackRegistrar.add("Pose.Delete", boost::bind(&BDFloaterPoser::onPoseDelete, this));
	//BD - Change a pose's blend type and time.
	mCommitCallbackRegistrar.add("Pose.Set", boost::bind(&BDFloaterPoser::onPoseSet, this, _1, _2));
	//BD - Extend or collapse the floater's pose list.
	mCommitCallbackRegistrar.add("Pose.Layout", boost::bind(&BDFloaterPoser::onUpdateLayout, this));
	//BD - Set the desired pose interpolation type.
	mCommitCallbackRegistrar.add("Pose.Interpolation", boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));
	//BD - Include some menu interactions. Sadly necessary.
	mCommitCallbackRegistrar.add("Pose.Menu", boost::bind(&BDFloaterPoser::onPoseMenuAction, this, _2));

	//BD - Change a bone's rotation.
	mCommitCallbackRegistrar.add("Joint.Set", boost::bind(&BDFloaterPoser::onJointSet, this, _1, _2));
	//BD - Change a bone's position.
	mCommitCallbackRegistrar.add("Joint.PosSet", boost::bind(&BDFloaterPoser::onJointPosSet, this, _1, _2));
	//BD - Change a bone's scale.
	mCommitCallbackRegistrar.add("Joint.SetScale", boost::bind(&BDFloaterPoser::onJointScaleSet, this, _1, _2));
	//BD - Add or remove a joint state to or from the pose (enable/disable our overrides).
	mCommitCallbackRegistrar.add("Joint.ChangeState", boost::bind(&BDFloaterPoser::onJointChangeState, this));
	//BD - Reset all selected bone rotations and positions.
	mCommitCallbackRegistrar.add("Joint.ResetJointFull", boost::bind(&BDFloaterPoser::onJointRotPosScaleReset, this));
	//BD - Reset all selected bone rotations back to 0,0,0.
	mCommitCallbackRegistrar.add("Joint.ResetJointRotation", boost::bind(&BDFloaterPoser::onJointRotationReset, this));
	//BD - Reset all selected bones positions back to their default.
	mCommitCallbackRegistrar.add("Joint.ResetJointPosition", boost::bind(&BDFloaterPoser::onJointPositionReset, this));
	//BD - Reset all selected bones scales back to their default.
	mCommitCallbackRegistrar.add("Joint.ResetJointScale", boost::bind(&BDFloaterPoser::onJointScaleReset, this));
	//BD - Reset all selected bone rotations back to the initial rotation.
	mCommitCallbackRegistrar.add("Joint.RevertJointRotation", boost::bind(&BDFloaterPoser::onJointRotationRevert, this));
	//BD - Recapture all bones either all or just disabled ones.
	mCommitCallbackRegistrar.add("Joint.Recapture", boost::bind(&BDFloaterPoser::onJointRecapture, this));
	//BD - Mirror the current bone's rotation to match what the other body side's rotation should be.
	mCommitCallbackRegistrar.add("Joint.Mirror", boost::bind(&BDFloaterPoser::onJointRecapture, this));
	//BD - Copy and mirror the other body side's bone rotation.
	mCommitCallbackRegistrar.add("Joint.Symmetrize", boost::bind(&BDFloaterPoser::onJointRecapture, this));

	//BD - Toggle Mirror Mode on/off.
	mCommitCallbackRegistrar.add("Joint.ToggleMirror", boost::bind(&BDFloaterPoser::toggleMirrorMode, this, _1));
	//BD - Toggle Easy Rotation on/off.
	mCommitCallbackRegistrar.add("Joint.EasyRotations", boost::bind(&BDFloaterPoser::toggleEasyRotations, this, _1));
	//BD - Flip pose (mirror).
	mCommitCallbackRegistrar.add("Joint.FlipPose", boost::bind(&BDFloaterPoser::onFlipPose, this));

	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Poser.RefreshAvatars", boost::bind(&BDFloaterPoser::onAvatarsRefresh, this));
	//BD - Export the current pose as animation.
	//mCommitCallbackRegistrar.add("Poser.Export", boost::bind(&BDFloaterPoser::onPoseExport, this));
	//BD - Toggle between "Live" and "Creation" mode.
	mCommitCallbackRegistrar.add("Poser.ToggleMode", boost::bind(&BDFloaterPoser::onModeChange, this));

	//BD - Add a new entry to the animation creator.
	mCommitCallbackRegistrar.add("Anim.Add", boost::bind(&BDFloaterPoser::onAnimAdd, this, _2));
	//BD - Move the selected entry one row up.
	mCommitCallbackRegistrar.add("Anim.Move", boost::bind(&BDFloaterPoser::onAnimMove, this, _2));
	//BD - Remove an entry in the animation creator.
	mCommitCallbackRegistrar.add("Anim.Delete", boost::bind(&BDFloaterPoser::onAnimDelete, this));
	//BD - Save the currently build list as animation.
	//mCommitCallbackRegistrar.add("Anim.Save", boost::bind(&BDFloaterPoser::onAnimSave, this));
	//BD - Play the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Play", boost::bind(&BDFloaterPoser::onAnimPlay, this));
	//BD - Stop the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Stop", boost::bind(&BDFloaterPoser::onAnimStop, this));
	//BD - Change the value for a wait entry.
	mCommitCallbackRegistrar.add("Anim.Set", boost::bind(&BDFloaterPoser::onAnimSet, this));
}

BDFloaterPoser::~BDFloaterPoser()
{
}

BOOL BDFloaterPoser::postBuild()
{
	//BD - Posing
	mJointScrolls = { { this->getChild<LLScrollListCtrl>("joints_scroll", true),
						this->getChild<LLScrollListCtrl>("cv_scroll", true),
						this->getChild<LLScrollListCtrl>("attach_scroll", true) } };

	mJointScrolls[JOINTS]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[JOINTS]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));
	mJointScrolls[JOINTS]->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onJointChangeState, this));

	//BD - Collision Volumes
	mJointScrolls[COLLISION_VOLUMES]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[COLLISION_VOLUMES]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	//BD - Attachment Bones
	mJointScrolls[ATTACHMENT_BONES]->setCommitOnSelectionChange(TRUE);
	mJointScrolls[ATTACHMENT_BONES]->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	mPoseScroll = this->getChild<LLScrollListCtrl>("poses_scroll", true);
	mPoseScroll->setCommitOnSelectionChange(TRUE);
	mPoseScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onPoseControlsRefresh, this));
	mPoseScroll->setDoubleClickCallback(boost::bind(&BDFloaterPoser::onPoseLoad, this));
	mPoseScroll->setRightMouseDownCallback(boost::bind(&BDFloaterPoser::onPoseScrollRightMouse, this, _1, _2, _3));

	mRotationSliders = { { getChild<LLUICtrl>("Rotation_X"), getChild<LLUICtrl>("Rotation_Y"), getChild<LLUICtrl>("Rotation_Z") } };
	mPositionSliders = { { getChild<LLSliderCtrl>("Position_X"), getChild<LLSliderCtrl>("Position_Y"), getChild<LLSliderCtrl>("Position_Z") } };
	mScaleSliders = { { getChild<LLSliderCtrl>("Scale_X"), getChild<LLSliderCtrl>("Scale_Y"), getChild<LLSliderCtrl>("Scale_Z") } };

	mJointTabs = getChild<LLTabContainer>("joints_tabs");
	mJointTabs->setCommitCallback(boost::bind(&BDFloaterPoser::onJointControlsRefresh, this));

	//BD - Animesh
	mAvatarScroll = this->getChild<LLScrollListCtrl>("avatar_scroll", true);
	mAvatarScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAvatarsSelect, this));

	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimEditorScroll->setCommitCallback(boost::bind(&BDFloaterPoser::onAnimControlsRefresh, this));

	//BD - Misc
	mDelayRefresh = false;

	mMirrorMode = false;
	mEasyRotations = true;

	mStartPosingBtn = getChild<LLButton>("activate");
	mLoadPosesBtn = getChild<LLMenuButton>("load_poses");
	mSavePosesBtn = getChild<LLButton>("save_poses");

	//BD - Poser Menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar pose_reg;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	pose_reg.add("Pose.Menu", boost::bind(&BDFloaterPoser::onPoseLoadSelective, this, _2));
	LLToggleableMenu* context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_poser_poses.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (context_menu)
	{
		mPosesMenuHandle = context_menu->getHandle();
		mLoadPosesBtn->setMenu(context_menu, LLMenuButton::MP_BOTTOM_LEFT);
	}

	/*pose_reg.add("Pose.Save", boost::bind(&BDFloaterPoser::onPoseSaveSelective, this, _2));
	enable_registrar.add("Pose.OnEnable", boost::bind(&BDFloaterPoser::onSaveMenuEnable, this, _2));
	LLToggleableMenu* save_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_poser_save.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (save_menu)
	{
		mPosesMenuHandle = save_menu->getHandle();
		getChild<LLMenuButton>("export_poses")->setMenu(save_menu, LLMenuButton::MP_BOTTOM_LEFT);
	}*/

	//BD - Poser Right Click Menu
	pose_reg.add("Joints.Menu", boost::bind(&BDFloaterPoser::onJointContextMenuAction, this, _2));
	enable_registrar.add("Joints.OnEnable", boost::bind(&BDFloaterPoser::onJointContextMenuEnable, this, _2));
	LLContextMenu* joint_menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_poser_joints.xml",
		gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mJointScrolls[JOINTS]->setContextMenu(joint_menu);

	//BD - Experimental
	/*mTimeSlider = getChild<LLMultiSliderCtrl>("time_slider");
	mKeySlider = getChild<LLMultiSliderCtrl>("key_slider");

	mTimeSlider->addSlider();
	addSliderKey(0.f, BDPoseKey(std::string("Default")));
	mTimeSlider->setCommitCallback(boost::bind(&BDFloaterPoser::onTimeSliderMoved, this));
	mKeySlider->setCommitCallback(boost::bind(&BDFloaterPoser::onKeyTimeMoved, this));
	getChild<LLButton>("add_key")->setClickedCallback(boost::bind(&BDFloaterPoser::onAddKey, this));
	getChild<LLButton>("delete_key")->setClickedCallback(boost::bind(&BDFloaterPoser::onDeleteKey, this));*/

	return TRUE;
}

void BDFloaterPoser::draw()
{
	if (gDragonAnimator.getIsPlaying())
	{
		LLScrollListItem* item = mAvatarScroll->getFirstSelected();
		if (item)
		{
			LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
			if (avatar || !avatar->isDead())
			{
				S32 current_index = avatar->getCurrentActionIndex();
				if (mAnimEditorScroll->getItemCount() != 0)
				{
					mAnimEditorScroll->selectNthItem(current_index);
				}
			}
		}
	}

	LLFloater::draw();
}

void BDFloaterPoser::onOpen(const LLSD& key)
{
	//BD - Check whether we should delay the default value collection or fire it immediately.
	mDelayRefresh = !gAgentAvatarp->isFullyLoaded();
	if (!mDelayRefresh)
	{
		onCollectDefaults();
	}

	//BD - We first fill the avatar list because the creation controls require it.
	onAvatarsRefresh();
	onCreationControlsRefresh();
	onJointRefresh();
	onPoseRefresh();
	onUpdateLayout();
}

void BDFloaterPoser::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mJointScrolls[JOINTS]->clearRows();
	mJointScrolls[COLLISION_VOLUMES]->clearRows();
	mJointScrolls[ATTACHMENT_BONES]->clearRows();
	mAvatarScroll->clearRows();
}

////////////////////////////////
//BD - Poses
////////////////////////////////
void BDFloaterPoser::onPoseRefresh()
{
	mPoseScroll->clearRows();
	std::string dir = gDirUtilp->getExpandedFilename(LL_PATH_POSES, "");
	std::string file;
	LLDirIterator dir_iter(dir, "*.xml");
	while (dir_iter.next(file))
	{
		std::string path = gDirUtilp->add(dir, file);
		std::string name = gDirUtilp->getBaseFileName(LLURI::unescape(path), true);

		LLSD row;
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = name;

		llifstream infile;
		infile.open(path);
		if (!infile.is_open())
		{
			LL_WARNS("Posing") << "Skipping: Cannot read file in: " << path << LL_ENDL;
			continue;
		}

		LLSD data;
		if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(data, infile))
		{
			LL_WARNS("Posing") << "Skipping: Failed to parse pose file: " << path << LL_ENDL;
			continue;
		}

		if (!infile.eof())
		{
			row["columns"][1]["column"] = "time";
			row["columns"][1]["value"] = data["time"];
			row["columns"][2]["column"] = "type";
			row["columns"][2]["value"] = data["type"];
		}
		mPoseScroll->addElement(row);
	}
	onJointControlsRefresh();
}

bool BDFloaterPoser::onClickPoseSave(const LLSD& param)
{
	if (param.asString() == "xml")
	{
		//BD - Values don't matter when not editing.
		if (onPoseSave(2, 0.1f, false))
		{
			LLNotificationsUtil::add("PoserExportXMLSuccess");
			return true;
		}
	}
	else if (param.asString() == "anim")
	{
		if (onPoseExport())
		{
			LLNotificationsUtil::add("PoserExportANIMSuccess");
			return true;
		}
	}
	return false;
}

bool BDFloaterPoser::onPoseSave(S32 type, F32 time, bool editing)
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item)
	{
		LL_WARNS("Posing") << "No avatar selected." << LL_ENDL;
		return false;
	}

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead())
	{
		LL_WARNS("Posing") << "Couldn't find avatar, dead?" << LL_ENDL;
		return false;
	}

	//BD - First and foremost before we do anything, check if the folder exists.
	std::string pathname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");
	if (!gDirUtilp->fileExists(pathname))
	{
		LL_WARNS("Posing") << "Couldn't find folder: " << pathname << " - creating one." << LL_ENDL;
		LLFile::mkdir(pathname);
	}

	std::string filename;
	if (editing)
	{
		LLScrollListItem* item = mPoseScroll->getFirstSelected();
		if (item)
		{
			filename = item->getColumn(0)->getValue().asString();
		}
	}
	else
	{
		filename = getChild<LLUICtrl>("pose_name")->getValue().asString();
	}

	if (filename.empty())
	{
		return false;
	}

	std::string full_path = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(filename) + ".xml");
	LLSD record;
	S32 line = 0;

	if (editing)
	{
		llifstream infile;

		infile.open(full_path);
		if (!infile.is_open())
		{
			LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
			return false;
		}

		LLSD old_record;
		S32 version = 0;
		//BD - Read the pose and save it into an LLSD so we can rewrite it later.
		while (!infile.eof())
		{
			if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(old_record, infile))
			{
				LL_WARNS("Posing") << "Failed to parse while rewrtiting file: " << filename << LL_ENDL;
				return false;
			}

			//BD - Check whether this is a new version or an older one and import them accordingly.
			if (old_record.has("version"))
			{
				version = old_record["version"]["value"];
				//BD - Version 2, simply take as is.
				if (version == 2)
				{
					record = old_record;
				}
			}
			else
			{
				//BD - Pre version 2, we have to import it manually as lines.
				if (line != 0)
				{
					record[line] = old_record;
				}
				++line;
			}
		}

		if (version == 2)
		{
			//BD - Change the header here.
			record["version"]["type"] = type;
			//BD - If we are using spherical linear interpolation we need to clamp the values 
			//     between 0.001f and 1.f otherwise unexpected things might happen.
			if (type == 2)
			{
				time = llclamp(time, 0.001f, 1.0f);
			}
			record["version"]["time"] = time;
		}
		else
		{
			//BD - Change the header here.
			record[0]["type"] = type;
			//BD - If we are using spherical linear interpolation we need to clamp the values 
			//     between 0.001f and 1.f otherwise unexpected things might happen.
			if (type == 2)
			{
				time = llclamp(time, 0.001f, 1.0f);
			}
			record[0]["time"] = time;
		}

		infile.close();

		LLScrollListItem* item = mPoseScroll->getFirstSelected();
		if (item)
		{
			item->getColumn(1)->setValue(time);
			item->getColumn(2)->setValue(type);
		}
	}
	else
	{
		//BD - Create the header first.
		S32 type = getChild<LLUICtrl>("interpolation_type")->getValue();
		F32 time = getChild<LLUICtrl>("interpolation_time")->getValue().asReal();
		//BD - If we are using spherical linear interpolation we need to clamp the values 
		//     between 0.001f and 1.f otherwise unexpected things might happen.
		if (type == 2)
		{
			time = llclamp(time, 0.001f, 1.0f);
		}
		S32 version = 2;
		record["version"]["value"] = version;
		record["version"]["time"] = time;
		record["version"]["type"] = type;

		//BD - Now create the rest.
		for (S32 it = 0; it < 3; ++it)
		{
			for (auto element : mJointScrolls[it]->getAllData())
			{
				LLVector3 vec3;
				LLJoint* joint = (LLJoint*)element->getUserdata();
				if (joint)
				{
					std::string bone_name = joint->getName();
					record[bone_name] = joint->getName();
					joint->getTargetRotation().getEulerAngles(&vec3.mV[VX], &vec3.mV[VZ], &vec3.mV[VY]);
					record[bone_name]["rotation"] = vec3.getValue();

					//BD - We could just check whether position information is available since only joints
					//     which can have their position changed will have position information but we
					//     want this to be a minefield for crashes.
					//     Bones that can support position
					//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
					//     as well as all attachment bones and collision volumes.
					if (joint->mHasPosition || it > JOINTS)
					{
						vec3 = it > JOINTS ? joint->getPosition() : joint->getTargetPosition();
						record[bone_name]["position"] = vec3.getValue();
					}

					vec3 = joint->getScale();
					record[bone_name]["scale"] = vec3.getValue();

					if (it == JOINTS)
					{
						//BD - Save the enabled state per preset so we can switch bones on and off
						//     on demand inbetween poses additionally to globally.
						BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
						if (motion)
						{
							LLPose* pose = motion->getPose();
							if (pose)
							{
								if (pose->findJointState(joint))
								{
									record[bone_name]["enabled"] = true;
								}
								else
								{
									record[bone_name]["enabled"] = false;
								}
							}
						}
					}
				}
			}
		}
	}
	
	llofstream file;
	file.open(full_path.c_str());
	//BD - Now lets actually write the file, whether it is writing a new one
	//     or just rewriting the previous one with a new header.
	LLSDSerialize::toPrettyXML(record, file);
	file.close();

	if (!editing)
	{
		onPoseRefresh();
	}

	//BD - Flash the poses button to give the user a visual cue where it went.
	getChild<LLButton>("extend")->setFlashing(true, true);
	return true;
}

void BDFloaterPoser::onPoseLoad()
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (!item) return;

	std::string pose_name = item->getColumn(0)->getValue().asString();

	gDragonAnimator.loadPose(pose_name);
	onJointRefresh();
}

void BDFloaterPoser::onPoseLoadSelective(const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (!item) return;

	std::string pose_name = item->getColumn(0)->getValue().asString();

	S32 load_type = 0;
	if (param.asString() == "rotation")
		load_type |= ROTATIONS;
	else if (param.asString() == "position")
		load_type |= POSITIONS;
	else if (param.asString() == "scale")
		load_type |= SCALES;
	else if (param.asString() == "rot_pos")
		load_type |= ROTATIONS | POSITIONS;
	else if (param.asString() == "rot_scale")
		load_type |= ROTATIONS | SCALES;
	else if (param.asString() == "pos_scale")
		load_type |= POSITIONS | SCALES;
	else if (param.asString() == "all")
		load_type |= ROTATIONS | POSITIONS | SCALES;

	gDragonAnimator.loadPose(pose_name, load_type);
	onJointRefresh();
}

/*void BDFloaterPoser::onPoseSaveSelective(const LLSD& param)
{
	if (param.asString() == "xml")
	{
		if (onClickPoseSave())
			LLNotificationsUtil::add("PoserExportXMLSuccess");
	}
	else if (param.asString() == "anim")
	{
		if (onPoseExport())
			LLNotificationsUtil::add("PoserExportANIMSuccess");
	}
}*/

void BDFloaterPoser::onPoseStart()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (!motion || motion->isStopped())
	{
		avatar->setPosing();
		if (avatar->isSelf())
		{
			//BD - Grab our current defaults to revert to them when stopping the Poser.
			if(gAgentAvatarp->isFullyLoaded())
				onCollectDefaults();

			gAgent.stopFidget();
			gDragonStatus->setPosing(true);
		}
		avatar->startMotion(ANIM_BD_POSING_MOTION);
	}
	else
	{
		//BD - Reset everything, all rotations, positions and scales of all bones.
		onJointRotPosScaleReset();

		//BD - Clear posing when we're done now that we've safely endangered getting spaghetified.
		avatar->clearPosing();
		avatar->stopMotion(ANIM_BD_POSING_MOTION);

		if (avatar->isSelf())
		{
			gDragonStatus->setPosing(false);
		}
	}
	//BD - Wipe the joint list.
	onJointRefresh();

	onPoseControlsRefresh();
}

void BDFloaterPoser::onPoseDelete()
{
	for (auto item : mPoseScroll->getAllSelected())
	{
		std::string filename = item->getColumn(0)->getValue().asString();
		std::string dirname = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "poses");

		if (gDirUtilp->deleteFilesInDir(dirname, gDragonLibrary.escapeString(filename) + ".xml") < 1)
		{
			LL_WARNS("Posing") << "Cannot remove file: " << filename << LL_ENDL;
		}
	}
	onPoseRefresh();
}

//BD - We use this function to edit an already saved pose's interpolation time or type.
//     Both are combined into one function.
void BDFloaterPoser::onPoseSet(LLUICtrl* ctrl, const LLSD& param)
{
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		S32 type;
		F32 time;
		//BD - We either edit the interpolation time here or if we don't we automatically assume 
		//     we are trying to edit the interpolation type.
		if (param.asString() == "time")
		{
			//BD - Get the new interpolation time from the time widget and read the type from
			//     the list as we are not going to change it anyway.
			time = getChild<LLUICtrl>("interp_time")->getValue().asReal();
			type = item->getColumn(2)->getValue().asInteger();
		}
		else
		{
			//BD - Do the opposite here.
			time = item->getColumn(1)->getValue().asReal();
			type = getChild<LLUICtrl>("interp_type")->getValue().asInteger();
		}

		onPoseSave(type, time, true);
	}
}

void BDFloaterPoser::onPoseControlsRefresh()
{
	bool is_playing = gDragonAnimator.getIsPlaying();
	LLScrollListItem* item = mPoseScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("interp_time")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("interp_type")->setValue(item->getColumn(2)->getValue());
	}
	getChild<LLUICtrl>("interp_time")->setEnabled(bool(item));
	getChild<LLUICtrl>("interp_type")->setEnabled(bool(item));
	getChild<LLUICtrl>("delete_poses")->setEnabled(bool(item));
	getChild<LLUICtrl>("add_entry")->setEnabled(!is_playing && item);
	mLoadPosesBtn->setEnabled(bool(item) && gDragonAnimator.mLiveMode);
}

void BDFloaterPoser::onPoseMenuAction(const LLSD& param)
{
	onPoseLoadSelective(param);
}

void BDFloaterPoser::onPoseScrollRightMouse(LLUICtrl* ctrl, S32 x, S32 y)
{
	mPoseScroll->selectItemAt(x, y, MASK_NONE);
	if (mPoseScroll->getFirstSelected())
	{
		LLToggleableMenu* poses_menu = mPosesMenuHandle.get();
		if (poses_menu)
		{
			poses_menu->buildDrawLabels();
			poses_menu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(mPoseScroll, poses_menu, x, y);
		}
	}
}

bool BDFloaterPoser::onPoseExport()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (!motion)
		return false;

	LLPose* pose = motion->getPose();
	if (!pose)
		return false;

	std::string motion_name = getChild<LLUICtrl>("export_name")->getValue().asString();
	if (motion_name.empty())
		return false;

	LLKeyframeMotion* temp_motion = NULL;
	LLAssetID mMotionID;
	LLTransactionID	mTransactionID;

	//BD - To make this work we'll first need a unique UUID for this animation.
	mTransactionID.generate();
	mMotionID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());

	temp_motion = (LLKeyframeMotion*)gAgentAvatarp->createMotion(mMotionID);

	if (!temp_motion)
		return false;

	LLKeyframeMotion::JointMotionList* joint_motion_list = new LLKeyframeMotion::JointMotionList;
	std::vector<LLPointer<LLJointState>> joint_states;

	if (joint_motion_list == NULL)
		return false;

	if (!joint_motion_list->mJointMotionArray.empty())
		joint_motion_list->mJointMotionArray.clear();
	joint_motion_list->mJointMotionArray.reserve(134);
	joint_states.clear();
	joint_states.reserve(134);

	joint_motion_list->mBasePriority = (LLJoint::JointPriority)getChild<LLUICtrl>("base_priority")->getValue().asInteger();
	joint_motion_list->mDuration = 1.f;
	joint_motion_list->mEaseInDuration = getChild<LLUICtrl>("ease_in")->getValue().asReal();
	joint_motion_list->mEaseOutDuration = getChild<LLUICtrl>("ease_out")->getValue().asReal();
	joint_motion_list->mEmoteName = "";
	joint_motion_list->mHandPose = (LLHandMotion::eHandPose)getChild<LLUICtrl>("hand_poses")->getValue().asInteger();
	joint_motion_list->mLoop = true;
	joint_motion_list->mLoopInPoint = 0.0f;
	joint_motion_list->mLoopOutPoint = 1.0f;
	joint_motion_list->mMaxPriority = LLJoint::ADDITIVE_PRIORITY;

	for (S32 i = 0; i <= ATTACHMENT_BONES; i++)
	{
		for (auto item : mJointScrolls[i]->getAllData())
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				//BD - Skip this bone if its not enabled. Allows us to selectively include bones.
				LLPointer<LLJointState> state = pose->findJointState(joint);
				if (!state)
					continue;

				//BD - Create a new joint motion and add it to the pile.
				LLKeyframeMotion::JointMotion* joint_motion = new LLKeyframeMotion::JointMotion;
				joint_motion_list->mJointMotionArray.push_back(joint_motion);

				//BD - Fill out joint motion with relevant basic data.
				joint_motion->mJointName = joint->getName();
				joint_motion->mPriority = LLJoint::HIGHEST_PRIORITY;

				//BD - Create the basic joint state for this joint and add it to the joint states.
				LLPointer<LLJointState> joint_state = new LLJointState;
				joint_states.push_back(joint_state);
				joint_state->setJoint(joint); // note: can accept NULL
				joint_state->setUsage(0);

				//BD - Start with filling general rotation data in.
				joint_motion->mRotationCurve.mNumKeys = 1;
				joint_motion->mRotationCurve.mInterpolationType = LLKeyframeMotion::IT_LINEAR;

				//BD - Create a rotation key and put it into the rotation curve.
				LLKeyframeMotion::RotationKey rotation_key = LLKeyframeMotion::RotationKey(1.f, joint->getTargetRotation());
				joint_motion->mRotationCurve.mKeys[0] = rotation_key;
				joint_state->setUsage(joint_state->getUsage() | LLJointState::ROT);

				//BD - Fill general position data in.
				joint_motion->mPositionCurve.mNumKeys = joint->mHasPosition ? 1 : 0;
				joint_motion->mPositionCurve.mInterpolationType = LLKeyframeMotion::IT_LINEAR;

				LLKeyframeMotion::PositionKey position_key;
				if (joint->mHasPosition)
				{
					//BD - Create a position key and put it into the position curve.
					LLKeyframeMotion::PositionKey position_key = LLKeyframeMotion::PositionKey(1.f, joint->getTargetPosition());
					joint_motion->mPositionCurve.mKeys[0] = position_key;
					joint_state->setUsage(joint_state->getUsage() | LLJointState::POS);
				}

				temp_motion->addJointState(joint_state);

				//BD - Start with filling general scale data in.
				joint_motion->mScaleCurve.mNumKeys = 1;
				joint_motion->mScaleCurve.mInterpolationType = LLKeyframeMotion::IT_LINEAR;

				//BD - Create a scale key and put it into the scale curve.
				LLKeyframeMotion::ScaleKey scale_key = LLKeyframeMotion::ScaleKey(1.f, joint->getScale());
				joint_motion->mScaleCurve.mKeys[0] = scale_key;

				joint_motion->mUsage = joint_state->getUsage();

				//BD - We do not use constraints so we just leave them out here.
				//     Should we ever add them we'd do so here.
				//joint_motion_list->mConstraints.push_front(constraint);

				//BD - Get the pelvis's bounding box and add it.
				if (joint->getJointNum() == 0)
				{
					joint_motion_list->mPelvisBBox.addPoint(position_key.mPosition);
				}
			}
		}
	}

	temp_motion->setJointMotionList(joint_motion_list);
	return temp_motion->dumpToFile(motion_name);
}

void BDFloaterPoser::onPoseImport()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	//BD - Create a new motion from an anim file from disk.
	//     Let us pick the file we want to preview with the filepicker.
	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getOpenFile(LLFilePicker::FFLOAD_ANIM))
	{
		std::string outfilename = picker.getFirstFile().c_str();
		S32 file_size;
		BOOL success = FALSE;
		LLAPRFile infile;
		LLKeyframeMotion* temp_motion = NULL;
		LLAssetID mMotionID;
		LLTransactionID	mTransactionID;

		//BD - To make this work we'll first need a unique UUID for this animation.
		mTransactionID.generate();
		mMotionID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
		temp_motion = (LLKeyframeMotion*)avatar->createMotion(mMotionID);

		//BD - Find and open the file, we'll need to write it temporarily into the VFS pool.
		infile.open(outfilename, LL_APR_RB, NULL, &file_size);
		if (infile.getFileHandle())
		{
			U8 *anim_data;
			S32 anim_file_size;

			LLFileSystem file(mMotionID, LLAssetType::AT_ANIMATION, LLFileSystem::WRITE);
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = infile.read(copy_buf, buf_size)))
			{
				file.write(copy_buf, file_size);
			}

			//BD - Now that we wrote the temporary file, find it and use it to set the size
			//     and buffer into which we will unpack the .anim file into.
			LLFileSystem* anim_file = new LLFileSystem(mMotionID, LLAssetType::AT_ANIMATION);
			anim_file_size = anim_file->getSize();
			anim_data = new U8[anim_file_size];
			anim_file->read(anim_data, anim_file_size);

			//BD - Cleanup everything we don't need anymore.
			delete anim_file;
			anim_file = NULL;

			//BD - Use the datapacker now to actually deserialize and unpack the animation
			//     into our temporary motion so we can use it after we added it into the list.
			LLDataPackerBinaryBuffer dp(anim_data, anim_file_size);
			success = temp_motion && temp_motion->deserialize(dp, mMotionID);

			//BD - Cleanup the rest.
			delete[]anim_data;
		}
		infile.close();

		//BD - Now write an entry with all given information into our list so we can use it.
		if (success)
		{
			LLKeyframeMotion::JointMotionList* joint_list = temp_motion->getJointMotionList();
			for (U32 i = 0; i < joint_list->getNumJointMotions(); i++)
			{
				LLKeyframeMotion::JointMotion* joint_motion = joint_list->getJointMotion(i);
				LLJoint* joint = avatar->getJoint(joint_motion->mJointName);
				if (joint)
				{

					LLKeyframeMotion::RotationCurve rot_curve = joint_motion->mRotationCurve;
					LLKeyframeMotion::PositionCurve pos_curve = joint_motion->mPositionCurve;
					LLKeyframeMotion::ScaleCurve scale_curve = joint_motion->mScaleCurve;

					//BD - Currently we only support the first frame of a motion (for poses)
					//     Later when we have a full animation editor we can extend this to import
					//     all keyframes.
					//for (auto rot_key : rot_curve.mKeys)
					{
						LLKeyframeMotion::RotationKey rot_key = rot_curve.mKeys[0];
						LLQuaternion rot = rot_key.mRotation;
							
						joint->setTargetRotation(rot);
					}

					//for (auto pos_key : pos_curve.mKeys)
					{
						//if (joint->mHasPosition)
						{
							LLKeyframeMotion::PositionKey pos_key = pos_curve.mKeys[0];
							LLVector3 pos = pos_key.mPosition;
							joint->setTargetPosition(pos);
						}
					}

					//for (auto scale_key : scale_curve.mKeys)
					{
						LLKeyframeMotion::ScaleKey scale_key = scale_curve.mKeys[0];
						LLVector3 scale = scale_key.mScale;

						joint->setScale(scale);
					}

					//BD - We support per bone priority. Question is does SL?
					//LLJoint::JointPriority priority = joint_motion->mPriority;
				}
			}
		}
	}

	onJointRefresh();
}

////////////////////////////////
//BD - Joints
////////////////////////////////
void BDFloaterPoser::onJointRefresh()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	//BD - Getting collision volumes and attachment points.
	std::vector<std::string> cv_names, attach_names;
	avatar->getSortedJointNames(1, cv_names);
	avatar->getSortedJointNames(2, attach_names);

	bool is_posing = avatar->getPosing();
	mJointScrolls[JOINTS]->clearRows();
	mJointScrolls[COLLISION_VOLUMES]->clearRows();
	mJointScrolls[ATTACHMENT_BONES]->clearRows();

	LLVector3 rot;
	LLVector3 pos;
	LLVector3 scale;
	LLJoint* joint;
	for (S32 i = 0; (joint = avatar->getCharacterJoint(i)); ++i)
	{
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLSD row;
		const std::string joint_name = joint->getName();
		//BD - Show some categories to make it a bit easier finding out which
		//     bone belongs where and what they might be for those who can't use
		//     bone names.
		if (joint->mJointNum == 0 ||	//mPelvis
			joint->mJointNum == 8 ||	//mHead
			joint->mJointNum == 58 ||	//mCollarLeft
			joint->mJointNum == 77 ||	//mCollarRight
			joint->mJointNum == 96 ||	//mWingsRoot
			joint->mJointNum == 107 ||	//mHipRight
			joint->mJointNum == 112 ||	//mHipLeft
			joint->mJointNum == 117 ||	//mTail1
			joint->mJointNum == 123)	//mGroin
		{
			row["columns"][COL_ICON]["column"] = "icon";
			row["columns"][COL_ICON]["type"] = "icon";
			row["columns"][COL_ICON]["value"] = getString("icon_category");
			row["columns"][COL_NAME]["column"] = "joint";
			row["columns"][COL_NAME]["value"] = getString("title_" + joint_name);
			LLScrollListItem* element = mJointScrolls[JOINTS]->addElement(row);
			element->setEnabled(FALSE);
		}

		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = joint_name;

		if (is_posing)
		{
			//BD - Bone Rotations
			joint->getTargetRotation().getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
			row["columns"][COL_ROT_X]["column"] = "x";
			row["columns"][COL_ROT_X]["value"] = ll_round(rot.mV[VX], 0.001f);
			row["columns"][COL_ROT_Y]["column"] = "y";
			row["columns"][COL_ROT_Y]["value"] = ll_round(rot.mV[VY], 0.001f);
			row["columns"][COL_ROT_Z]["column"] = "z";
			row["columns"][COL_ROT_Z]["value"] = ll_round(rot.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		//BD - We could just check whether position information is available since only joints
		//     which can have their position changed will have position information but we
		//     want this to be a minefield for crashes.
		//     Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (joint->mHasPosition)
		{
			if (is_posing)
			{
				pos = joint->getTargetPosition();
				row["columns"][COL_POS_X]["column"] = "pos_x";
				row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
				row["columns"][COL_POS_Y]["column"] = "pos_y";
				row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
				row["columns"][COL_POS_Z]["column"] = "pos_z";
				row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);
			}
		}

		LLScrollListItem* item = mJointScrolls[JOINTS]->addElement(row);
		item->setUserdata(joint);

		//BD - We need to check if we are posing or not, simply set all bones to deactivated
		//     when we are not posed otherwise they will remain on "enabled" state. This behavior
		//     could be confusing to the user, this is due to how animations work.
		if (is_posing)
		{
			BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					// BD - We do check here for the joint_state because otherwise we end up with the toggle
					//      state not appearing properly toggled/untoggled when we first refresh after firing
					//      up the poser. At the same time this is used to refresh all bone states when we
					//      load a pose.
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
		else
		{
			((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::NORMAL);
		}
	}

	//BD - We add an empty line into both lists and include an icon to automatically resize
	//     the list row heights to sync up with the height in our joint list. We remove it
	//     immediately after anyway.
	LLSD test_row;
	test_row["columns"][COL_ICON]["column"] = "icon";
	test_row["columns"][COL_ICON]["type"] = "icon";
	test_row["columns"][COL_ICON]["value"] = getString("icon_category");
	mJointScrolls[COLLISION_VOLUMES]->addElement(test_row);
	mJointScrolls[COLLISION_VOLUMES]->deleteSingleItem(0);
	mJointScrolls[ATTACHMENT_BONES]->addElement(test_row);
	mJointScrolls[ATTACHMENT_BONES]->deleteSingleItem(0);

	//BD - Collision Volumes
	for (auto name : cv_names)
	{
		joint = avatar->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint) continue;

		LLSD row;
		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = name;

		if (is_posing)
		{
			//BD - Bone Positions
			pos = joint->getPosition();
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		LLScrollListItem* item = mJointScrolls[COLLISION_VOLUMES]->addElement(row);
		item->setUserdata(joint);
	}

	//BD - Attachment Bones
	for (auto name : attach_names)
	{
		joint = avatar->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint) continue;

		LLSD row;
		row["columns"][COL_ICON]["column"] = "icon";
		row["columns"][COL_ICON]["type"] = "icon";
		row["columns"][COL_ICON]["value"] = getString("icon_bone");
		row["columns"][COL_NAME]["column"] = "joint";
		row["columns"][COL_NAME]["value"] = name;

		if (is_posing)
		{
			//BD - Bone Positions
			pos = joint->getPosition();
			row["columns"][COL_POS_X]["column"] = "pos_x";
			row["columns"][COL_POS_X]["value"] = ll_round(pos.mV[VX], 0.001f);
			row["columns"][COL_POS_Y]["column"] = "pos_y";
			row["columns"][COL_POS_Y]["value"] = ll_round(pos.mV[VY], 0.001f);
			row["columns"][COL_POS_Z]["column"] = "pos_z";
			row["columns"][COL_POS_Z]["value"] = ll_round(pos.mV[VZ], 0.001f);

			//BD - Bone Scales
			scale = joint->getScale();
			row["columns"][COL_SCALE_X]["column"] = "scale_x";
			row["columns"][COL_SCALE_X]["value"] = ll_round(scale.mV[VX], 0.001f);
			row["columns"][COL_SCALE_Y]["column"] = "scale_y";
			row["columns"][COL_SCALE_Y]["value"] = ll_round(scale.mV[VY], 0.001f);
			row["columns"][COL_SCALE_Z]["column"] = "scale_z";
			row["columns"][COL_SCALE_Z]["value"] = ll_round(scale.mV[VZ], 0.001f);
		}

		LLScrollListItem* item = mJointScrolls[ATTACHMENT_BONES]->addElement(row);
		item->setUserdata(joint);
	}

	onJointControlsRefresh();
}

void BDFloaterPoser::onJointControlsRefresh()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	bool can_position = false;
	bool is_pelvis = false;
	bool is_posing = (avatar->isFullyLoaded() && avatar->getPosing());
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			if (index == 0)
			{
				mRotationSliders[VX]->setValue(item->getColumn(COL_ROT_X)->getValue());
				mRotationSliders[VY]->setValue(item->getColumn(COL_ROT_Y)->getValue());
				mRotationSliders[VZ]->setValue(item->getColumn(COL_ROT_Z)->getValue());
			}

			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			//     as well as all attachment bones and collision volumes.
			if (joint->mHasPosition || index > JOINTS)
			{
				can_position = true;
				is_pelvis = (joint->mJointNum == 0);

				mPositionSliders[VX]->setValue(item->getColumn(COL_POS_X)->getValue());
				mPositionSliders[VY]->setValue(item->getColumn(COL_POS_Y)->getValue());
				mPositionSliders[VZ]->setValue(item->getColumn(COL_POS_Z)->getValue());
			}
			else
			{
				//BD - If we didn't select a positionable bone kill all values. We might
				//     end up with numbers that are too big for the min/max values when
				//     changed below.
				mPositionSliders[VX]->setValue(0.000f);
				mPositionSliders[VY]->setValue(0.000f);
				mPositionSliders[VZ]->setValue(0.000f);
			}

			//BD - Bone Scales
			mScaleSliders[VX]->setValue(item->getColumn(COL_SCALE_X)->getValue());
			mScaleSliders[VY]->setValue(item->getColumn(COL_SCALE_Y)->getValue());
			mScaleSliders[VZ]->setValue(item->getColumn(COL_SCALE_Z)->getValue());

			BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
			if (motion)
			{
				//BD - If we don't use our default spherical interpolation, set it once.
				motion->setInterpolationTime(0.25f);
				motion->setInterpolationType(2);

				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					getChild<LLButton>("toggle_bone")->setValue(joint_state.notNull());
				}
			}
		}
	}

	getChild<LLButton>("toggle_bone")->setEnabled(item && is_posing && index == JOINTS);
	getChild<LLButton>("activate")->setValue(is_posing);
	getChild<LLUICtrl>("pose_name")->setEnabled(is_posing);
	getChild<LLUICtrl>("save_poses")->setEnabled(is_posing);
	getChild<LLUICtrl>("joints_tabs")->setEnabled(is_posing);

	//BD - Depending on which interpolation type the user selects we want the time editor to show
	//     a different label, since spherical and linear both have different min/max numbers and
	//     work differently with them.
	//     Spherical: 0.001 - 1.0.
	//     Linear: 0.0 - infinite.
	getChild<LLUICtrl>("interpolation_type")->setEnabled(is_posing);
	S32 interp_type = getChild<LLUICtrl>("interpolation_type")->getValue();
	getChild<LLUICtrl>("interpolation_time")->setEnabled(is_posing && interp_type != 0);
	const std::string label_time = interp_type == 1 ? "linear_time" : "spherical_time";
	getChild<LLLineEditor>("interpolation_time")->setLabel(getString(label_time));

	//BD - Enable position tabs whenever positions are available, scales are always enabled
	//     unless we are editing attachment bones, rotations on the other hand are only
	//     enabled when editing joints.
	LLTabContainer* modifier_tabs = getChild<LLTabContainer>("modifier_tabs");
	modifier_tabs->setEnabled(item && is_posing);
	modifier_tabs->enableTabButton(0, (item && is_posing && index == JOINTS));
	modifier_tabs->enableTabButton(1, (item && is_posing && can_position));
	modifier_tabs->enableTabButton(2, (item && is_posing && index != ATTACHMENT_BONES));
	
	S32 tab_idx = modifier_tabs->getCurrentPanelIndex();
	//BD - Swap out of "Position" tab when it's not available.
	if (!can_position && tab_idx == 1)
	{
		modifier_tabs->selectTab(0);
	}
	//BD - Swap out of "Scale" and "Rotation" tabs when they are not available.
	if ((index != COLLISION_VOLUMES && index != JOINTS && tab_idx == 2)
		|| (index != JOINTS && tab_idx == 0))
	{
		modifier_tabs->selectTab(1);
	}

	F32 max_val = is_pelvis ? 20.f : 1.0f;
	mPositionSliders[VX]->setMaxValue(max_val);
	mPositionSliders[VY]->setMaxValue(max_val);
	mPositionSliders[VZ]->setMaxValue(max_val);
	mPositionSliders[VX]->setMinValue(-max_val);
	mPositionSliders[VY]->setMinValue(-max_val);
	mPositionSliders[VZ]->setMinValue(-max_val);

	//BD - Change our animator's target, make sure it is always up-to-date.
	gDragonAnimator.mTargetAvatar = avatar;
}

void BDFloaterPoser::onJointSet(LLUICtrl* ctrl, const LLSD& param)
{
	//BD - Rotations are only supported by joints so far, everything
	//     else snaps back instantly.
	LLScrollListItem* item = mJointScrolls[JOINTS]->getFirstSelected();
	if (!item)
		return;

	LLJoint* joint = (LLJoint*)item->getUserdata();
	if (!joint)
		return;

	//BD - Neat yet quick and direct way of rotating our bones.
	//     No more need to include bone rotation orders.
	F32 val = ctrl->getValue().asReal();
	S32 axis = param.asInteger();
	LLScrollListCell* cell[3] = { item->getColumn(COL_ROT_X), item->getColumn(COL_ROT_Y), item->getColumn(COL_ROT_Z) };
	LLQuaternion rot_quat = joint->getTargetRotation();
	LLMatrix3 rot_mat;
	F32 old_value;
	F32 new_value;
	LLVector3 vec3;

	old_value = cell[axis]->getValue().asReal();
	cell[axis]->setValue(ll_round(val, 0.001f));
	new_value = val - old_value;
	vec3.mV[axis] = new_value;
	rot_mat = LLMatrix3(vec3.mV[VX], vec3.mV[VY], vec3.mV[VZ]);
	rot_quat = LLQuaternion(rot_mat)*rot_quat;
	joint->setTargetRotation(rot_quat);
	if (!mEasyRotations)
	{
		rot_quat.getEulerAngles(&vec3.mV[VX], &vec3.mV[VY], &vec3.mV[VZ]);
		S32 i = 0;
		while (i < 3)
		{
			if (i != axis)
			{
				cell[i]->setValue(ll_round(vec3.mV[i], 0.001f));
				mRotationSliders[i]->setValue(item->getColumn(i + 2)->getValue());
			}
			++i;
		}
	}
	
	//BD - If we are in Mirror mode, try to find the opposite bone of our currently
	//     selected one, for now this simply means we take the name and replace "Left"
	//     with "Right" and vise versa since all bones are conveniently that way.
	//     TODO: Do this when creating the joint list so we don't try to find the joint
	//     over and over again.
	if (mMirrorMode)
	{
		LLJoint* mirror_joint = nullptr;
		std::string mirror_joint_name = joint->getName();
		S32 idx = joint->getName().find("Left");
		if (idx != -1)
			mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");

		idx = joint->getName().find("Right");
		if (idx != -1)
			mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

		if (mirror_joint_name != joint->getName())
		{
			mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);
		}

		if (mirror_joint)
		{
			//BD - For the opposite joint we invert X and Z axis, everything else is directly applied
			//     exactly like we do it in our currently selected joint.
			if (axis != 1)
				val = -val;

			LLQuaternion inv_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
			mirror_joint->setTargetRotation(inv_quat);

			//BD - We also need to find the opposite joint's list entry and change its values to reflect
			//     the new ones, doing this here is still better than causing a complete refresh.
			LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
			if (item2)
			{
				LLScrollListCell* cell2[3] = { item2->getColumn(COL_ROT_X), item2->getColumn(COL_ROT_Y), item2->getColumn(COL_ROT_Z) };
				S32 i = 0;
				while (i < 3)
				{
					cell2[i]->setValue(ll_round(item->getColumn(i + 2)->getValue(), 0.001f));
					++i;
				}
			}
		}
	}
}

void BDFloaterPoser::onJointPosSet(LLUICtrl* ctrl, const LLSD& param)
{
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			//BD - We could just check whether position information is available since only joints
			//     which can have their position changed will have position information but we
			//     want this to be a minefield for crashes.
			//     Bones that can support position
			//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
			//     as well as all attachment bones and collision volumes.
			if (joint->mHasPosition || index > JOINTS)
			{
				F32 val = ctrl->getValue().asReal();
				LLScrollListCell* cell[3] = { item->getColumn(COL_POS_X), item->getColumn(COL_POS_Y), item->getColumn(COL_POS_Z) };
				LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
								   F32(cell[VY]->getValue().asReal()),
								   F32(cell[VZ]->getValue().asReal()) };

				S32 dir = param.asInteger();
				vec3.mV[dir] = val;
				cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
				joint->setTargetPosition(vec3);
			}
		}
	}
}

void BDFloaterPoser::onJointScaleSet(LLUICtrl* ctrl, const LLSD& param)
{
	S32 index = mJointTabs->getCurrentPanelIndex();
	LLScrollListItem* item = mJointScrolls[index]->getFirstSelected();

	if (item)
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			F32 val = ctrl->getValue().asReal();
			LLScrollListCell* cell[3] = { item->getColumn(COL_SCALE_X), item->getColumn(COL_SCALE_Y), item->getColumn(COL_SCALE_Z) };
			LLVector3 vec3 = { F32(cell[VX]->getValue().asReal()),
							   F32(cell[VY]->getValue().asReal()),
							   F32(cell[VZ]->getValue().asReal()) };

			S32 dir = param.asInteger();
			vec3.mV[dir] = val;
			cell[dir]->setValue(ll_round(vec3.mV[dir], 0.001f));
			joint->setScale(vec3);
		}
	}
}

void BDFloaterPoser::onJointChangeState()
{
	BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		for (auto item : mJointScrolls[JOINTS]->getAllSelected())
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					if (joint_state)
					{
						motion->removeJointState(joint_state);
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::NORMAL);
					}
					else
					{
						motion->addJointToState(joint);
						((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);
					}
				}
			}
		}
	}
	onJointControlsRefresh();
}

//BD - We use this to reset everything at once.
void BDFloaterPoser::onJointRotPosScaleReset()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	for (S32 it = 0; it < 3; ++it)
	{
		//BD - We use this bool to determine whether or not we'll be in need for a full skeleton
		//     reset and to prevent checking for it every single time.
		for (auto item : mJointScrolls[it]->getAllData())
		{
			if (item)
			{
				LLJoint* joint = (LLJoint*)item->getUserdata();
				if (joint)
				{
					//BD - Resetting rotations first if there are any.
					if (it == JOINTS)
					{
						LLQuaternion quat;
						LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
						LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
						LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

						col_rot_x->setValue(0.000f);
						col_rot_y->setValue(0.000f);
						col_rot_z->setValue(0.000f);

						quat.setEulerAngles(0, 0, 0);
						joint->setTargetRotation(quat);

						//BD - If we are in Mirror mode, try to find the opposite bone of our currently
						//     selected one, for now this simply means we take the name and replace "Left"
						//     with "Right" and vise versa since all bones are conveniently that way.
						//     TODO: Do this when creating the joint list so we don't try to find the joint
						//     over and over again.
						if (mMirrorMode)
						{
							LLJoint* mirror_joint = nullptr;
							std::string mirror_joint_name = joint->getName();
							S32 idx = joint->getName().find("Left");
							if (idx != -1)
								mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");

							idx = joint->getName().find("Right");
							if (idx != -1)
								mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

							if (mirror_joint_name != joint->getName())
							{
								mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);
							}

							if (mirror_joint)
							{
								//BD - We also need to find the opposite joint's list entry and change its values to reflect
								//     the new ones, doing this here is still better than causing a complete refresh.
								LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
								if (item2)
								{
									col_rot_x = item2->getColumn(COL_ROT_X);
									col_rot_y = item2->getColumn(COL_ROT_Y);
									col_rot_z = item2->getColumn(COL_ROT_Z);

									col_rot_x->setValue(0.000f);
									col_rot_y->setValue(0.000f);
									col_rot_z->setValue(0.000f);

									mirror_joint->setTargetRotation(quat);
								}
							}
						}
					}

					//BD - Resetting positions next.
					//BD - We could just check whether position information is available since only joints
					//     which can have their position changed will have position information but we
					//     want this to be a minefield for crashes.
					//     Bones that can support position
					//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
					//     as well as all attachment bones and collision volumes.
					if (joint->mHasPosition || it > JOINTS)
					{
						LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
						LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
						LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
						LLVector3 pos = mDefaultPositions[joint->getName()];

						col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
						col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
						col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
						joint->setTargetPosition(pos);
					}

					//BD - Resetting scales last.
					LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
					LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
					LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
					LLVector3 scale = mDefaultScales[joint->getName()];

					col_scale_x->setValue(ll_round(scale.mV[VX], 0.001f));
					col_scale_y->setValue(ll_round(scale.mV[VY], 0.001f));
					col_scale_z->setValue(ll_round(scale.mV[VZ], 0.001f));
					joint->setScale(scale);
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Used to reset rotations only.
void BDFloaterPoser::onJointRotationReset()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We do support resetting bone rotations for everyone however.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLQuaternion quat;
				LLScrollListCell* col_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_z = item->getColumn(COL_ROT_Z);

				col_x->setValue(0.000f);
				col_y->setValue(0.000f);
				col_z->setValue(0.000f);

				quat.setEulerAngles(0, 0, 0);
				joint->setTargetRotation(quat);

				//BD - If we are in Mirror mode, try to find the opposite bone of our currently
				//     selected one, for now this simply means we take the name and replace "Left"
				//     with "Right" and vise versa since all bones are conveniently that way.
				//     TODO: Do this when creating the joint list so we don't try to find the joint
				//     over and over again.
				if (mMirrorMode)
				{
					LLJoint* mirror_joint = nullptr;
					std::string mirror_joint_name = joint->getName();
					S32 idx = joint->getName().find("Left");
					if (idx != -1)
						mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");

					idx = joint->getName().find("Right");
					if (idx != -1)
						mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

					if (mirror_joint_name != joint->getName())
					{
						mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);
					}

					if (mirror_joint)
					{
						//BD - We also need to find the opposite joint's list entry and change its values to reflect
						//     the new ones, doing this here is still better than causing a complete refresh.
						LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
						if (item2)
						{
							col_x = item2->getColumn(COL_ROT_X);
							col_y = item2->getColumn(COL_ROT_Y);
							col_z = item2->getColumn(COL_ROT_Z);

							col_x->setValue(0.000f);
							col_y->setValue(0.000f);
							col_z->setValue(0.000f);

							mirror_joint->setTargetRotation(quat);
						}
					}
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Used to reset positions, this is very tricky hence why it was separated.
//     It causes the avatar to flinch for a second which doesn't look as nice as resetting
//     rotations does.
void BDFloaterPoser::onJointPositionReset()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones positions for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - When resetting positions, we don't use interpolation for now, it looks stupid.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	//BD - We use this bool to prevent going through attachment override reset every single time.
	//bool has_reset = false;
	for (auto item : mJointScrolls[index]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				//BD - We could just check whether position information is available since only joints
				//     which can have their position changed will have position information but we
				//     want this to be a minefield for crashes.
				//     Bones that can support position
				//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
				//     as well as all attachment bones and collision volumes.
				if (joint->mHasPosition || index > JOINTS)
				{
					LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
					LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
					LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);
					LLVector3 pos = mDefaultPositions[joint->getName()];

					col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
					col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
					col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
					joint->setTargetPosition(pos);
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Used to reset scales only.
void BDFloaterPoser::onJointScaleReset()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We don't support resetting bones scales for anyone else yet.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead() || !avatar->isSelf()) return;

	S32 index = mJointTabs->getCurrentPanelIndex();

	//BD - Clear all attachment bone scale changes we've done, they are not automatically
	//     reverted.
	for (auto item : mJointScrolls[index]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
				LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
				LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
				LLVector3 scale = mDefaultScales[joint->getName()];

				col_scale_x->setValue(ll_round(scale.mV[VX], 0.001f));
				col_scale_y->setValue(ll_round(scale.mV[VY], 0.001f));
				col_scale_z->setValue(ll_round(scale.mV[VZ], 0.001f));
				joint->setScale(scale);
			}
		}
	}
	onJointControlsRefresh();
}

//BD - Used to revert rotations only.
void BDFloaterPoser::onJointRotationRevert()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	//BD - We do support reverting bone rotations for everyone however.
	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	//BD - While editing rotations, make sure we use a bit of spherical linear interpolation 
	//     to make movements smoother.
	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		//BD - If we don't use our default spherical interpolation, set it once.
		motion->setInterpolationTime(0.25f);
		motion->setInterpolationType(2);
	}

	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				//BD - Reverting rotations first if there are any.
				LLQuaternion quat = mDefaultRotations[joint->getName()];
				LLVector3 rot;
				quat.getEulerAngles(&rot.mV[VX], &rot.mV[VY], &rot.mV[VZ]);
				LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

				col_rot_x->setValue(rot.mV[VX]);
				col_rot_y->setValue(rot.mV[VY]);
				col_rot_z->setValue(rot.mV[VZ]);

				joint->setTargetRotation(quat);

				//BD - If we are in Mirror mode, try to find the opposite bone of our currently
				//     selected one, for now this simply means we take the name and replace "Left"
				//     with "Right" and vise versa since all bones are conveniently that way.
				//     TODO: Do this when creating the joint list so we don't try to find the joint
				//     over and over again.
				if (mMirrorMode)
				{
					LLJoint* mirror_joint = nullptr;
					std::string mirror_joint_name = joint->getName();
					S32 idx = joint->getName().find("Left");
					if (idx != -1)
						mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");

					idx = joint->getName().find("Right");
					if (idx != -1)
						mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

					if (mirror_joint_name != joint->getName())
					{
						mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);
					}

					if (mirror_joint)
					{
						//BD - We also need to find the opposite joint's list entry and change its values to reflect
						//     the new ones, doing this here is still better than causing a complete refresh.
						LLScrollListItem* item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);
						if (item2)
						{
							col_rot_x = item2->getColumn(COL_ROT_X);
							col_rot_y = item2->getColumn(COL_ROT_Y);
							col_rot_z = item2->getColumn(COL_ROT_Z);

							col_rot_x->setValue(rot.mV[VX]);
							col_rot_y->setValue(rot.mV[VY]);
							col_rot_z->setValue(rot.mV[VZ]);

							mirror_joint->setTargetRotation(quat);
						}
					}
				}
			}
		}
	}

	onJointControlsRefresh();
}

//BD - Flip our pose (mirror it)
void BDFloaterPoser::onFlipPose()
{
	LLVOAvatar* avatar = gDragonAnimator.mTargetAvatar;
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	LLJoint* joint = nullptr;
	bool flipped[134] = { false };

	for (S32 i = 0; (joint = avatar->getCharacterJoint(i)); ++i)
	{
		//BD - Skip if we already flipped this bone.
		if (flipped[i]) continue;

		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLVector3 rot, mirror_rot;
		LLQuaternion rot_quat, mirror_rot_quat;
		std::string joint_name = joint->getName();
		std::string mirror_joint_name = joint->getName();
		//BD - Attempt to find the "right" version of this bone first, we assume we always
		//     end up with the "left" version of a bone first.
		S32 idx = joint->getName().find("Left");
		if (idx != -1)
			mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
		//BD - Attempt to find the "right" version of this bone first, this is necessary
		//     because there are a couple bones starting with the "right" bone.
		idx = joint->getName().find("Right");
		if (idx != -1)
			mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

		LLJoint* mirror_joint = nullptr;
		if (mirror_joint_name != joint->getName())
			mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);

		//BD - Collect the joint and mirror joint entries and their cells, we need them later.
		LLScrollListItem* item1 = mJointScrolls[JOINTS]->getItemByLabel(joint_name, FALSE, COL_NAME);
		LLScrollListItem* item2 = nullptr;

		//BD - Get the rotation of our current bone and that of the mirror bone (if available).
		//     Flip our current bone's rotation and apply it to the mirror bone (if available).
		//     Flip the mirror bone's rotation (if available) and apply it to our current bone.
		//     If the mirror bone does not exist, flip the current bone rotation and use that.
		rot_quat = joint->getTargetRotation();
		LLQuaternion inv_rot_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
		inv_rot_quat.getEulerAngles(&rot[VX], &rot[VY], &rot[VZ]);

		if (mirror_joint)
		{
			mirror_rot_quat = mirror_joint->getTargetRotation();
			LLQuaternion inv_mirror_rot_quat = LLQuaternion(-mirror_rot_quat.mQ[VX], mirror_rot_quat.mQ[VY], -mirror_rot_quat.mQ[VZ], mirror_rot_quat.mQ[VW]);
			inv_mirror_rot_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
			mirror_joint->setTargetRotation(inv_rot_quat);
			joint->setTargetRotation(inv_mirror_rot_quat);

			item2 = mJointScrolls[JOINTS]->getItemByLabel(mirror_joint_name, FALSE, COL_NAME);

			//BD - Make sure we flag this bone as flipped so we skip it next time we iterate over it.
			flipped[mirror_joint->getJointNum()] = true;
		}
		else
		{
			joint->setTargetRotation(inv_rot_quat);
		}

		S32 axis = 0;
		while (axis <= 2)
		{
			//BD - Now flip the list entry values.
			if (item1)
			{
				if (mirror_joint)
					item1->getColumn(axis + 2)->setValue(ll_round(mirror_rot[axis], 0.001f));
				else
					item1->getColumn(axis + 2)->setValue(ll_round(rot[axis], 0.001f));
			}

			//BD - Now flip the mirror joint list entry values.
			if (item2)
				item2->getColumn(axis + 2)->setValue(ll_round(rot[axis], 0.001f));

			++axis;
		}
		flipped[i] = true;
	}
}

//BD - Flip our pose (mirror it)
void BDFloaterPoser::onJointRecapture()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	if (!(avatar->getRegion() == gAgent.getRegion())) return;

	LLQuaternion rot;
	LLVector3 pos;

	BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
	if (motion)
	{
		LLPose* pose = motion->getPose();
		if (pose)
		{
			for (auto item : mJointScrolls[JOINTS]->getAllData())
			{
				if (item)
				{
					LLJoint* joint = (LLJoint*)item->getUserdata();
					if (joint)
					{
						// BD - Check for the joint state (whether a bone is enabled or not)
						//      If not proceed.
						LLPointer<LLJointState> joint_state = pose->findJointState(joint);
						if (!joint_state)
						{
							//BD - First gather the current rotation and position.
							rot = joint->getRotation();
							pos = joint->getPosition();

							//BD - Now, re-add the joint state and enable changing the pose.
							motion->addJointToState(joint);
							((LLScrollListText*)item->getColumn(COL_NAME))->setFontStyle(LLFontGL::BOLD);

							//BD - Apply the newly collected rotation and position to the pose.
							joint->setTargetRotation(rot);
							joint->setTargetPosition(pos);

							//BD - Get all columns and fill in the new values.
							LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
							LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
							LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

							LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
							LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
							LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);

							LLVector3 euler_rot;
							rot.getEulerAngles(&euler_rot.mV[VX], &euler_rot.mV[VY], &euler_rot.mV[VZ]);
							col_rot_x->setValue(ll_round(euler_rot.mV[VX], 0.001f));
							col_rot_y->setValue(ll_round(euler_rot.mV[VY], 0.001f));
							col_rot_z->setValue(ll_round(euler_rot.mV[VZ], 0.001f));

							col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
							col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
							col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
						}
					}
				}
			}
		}
	}
}

//BD - Poser Utility Functions
void BDFloaterPoser::onJointPasteRotation()
{
	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
			LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
			LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

			LLVector3 euler_rot;
			LLQuaternion rot = (LLQuaternion)mClipboard["rot"];

			joint->setTargetRotation(rot);

			rot.getEulerAngles(&euler_rot.mV[VX], &euler_rot.mV[VY], &euler_rot.mV[VZ]);
			col_rot_x->setValue(ll_round(euler_rot.mV[VX], 0.001f));
			col_rot_y->setValue(ll_round(euler_rot.mV[VY], 0.001f));
			col_rot_z->setValue(ll_round(euler_rot.mV[VZ], 0.001f));
		}
	}
}

void BDFloaterPoser::onJointPastePosition()
{
	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			LLScrollListCell* col_pos_x = item->getColumn(COL_POS_X);
			LLScrollListCell* col_pos_y = item->getColumn(COL_POS_Y);
			LLScrollListCell* col_pos_z = item->getColumn(COL_POS_Z);

			LLVector3 pos = (LLVector3)mClipboard["pos"];

			joint->setTargetPosition(pos);

			col_pos_x->setValue(ll_round(pos.mV[VX], 0.001f));
			col_pos_y->setValue(ll_round(pos.mV[VY], 0.001f));
			col_pos_z->setValue(ll_round(pos.mV[VZ], 0.001f));
		}
	}
}

void BDFloaterPoser::onJointPasteScale()
{
	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			LLScrollListCell* col_scale_x = item->getColumn(COL_SCALE_X);
			LLScrollListCell* col_scale_y = item->getColumn(COL_SCALE_Y);
			LLScrollListCell* col_scale_z = item->getColumn(COL_SCALE_Z);
			LLVector3 scale = (LLVector3)mClipboard["scale"];

			joint->setScale(scale);

			col_scale_x->setValue(ll_round(scale.mV[VX], 0.001f));
			col_scale_y->setValue(ll_round(scale.mV[VY], 0.001f));
			col_scale_z->setValue(ll_round(scale.mV[VZ], 0.001f));
		}
	}
}

void BDFloaterPoser::onJointMirror()
{
	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			LLVector3 euler_rot;
			LLQuaternion rot_quat = joint->getTargetRotation();

			//BD - Simply mirror the current bone's rotation like we'd do if we pressed the mirror
			//     button without a mirror bone available.
			LLQuaternion inv_rot_quat = LLQuaternion(-rot_quat.mQ[VX], rot_quat.mQ[VY], -rot_quat.mQ[VZ], rot_quat.mQ[VW]);
			inv_rot_quat.getEulerAngles(&euler_rot[VX], &euler_rot[VY], &euler_rot[VZ]);
			joint->setTargetRotation(inv_rot_quat);

			LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
			LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
			LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

			col_rot_x->setValue(ll_round(euler_rot.mV[VX], 0.001f));
			col_rot_y->setValue(ll_round(euler_rot.mV[VY], 0.001f));
			col_rot_z->setValue(ll_round(euler_rot.mV[VZ], 0.001f));
		}
	}
}

void BDFloaterPoser::onJointSymmetrize()
{
	for (auto item : mJointScrolls[JOINTS]->getAllSelected())
	{
		LLJoint* joint = (LLJoint*)item->getUserdata();
		if (joint)
		{
			std::string joint_name = joint->getName();
			std::string mirror_joint_name = joint->getName();
			//BD - Attempt to find the "right" version of this bone, if we can't find it try
			//     the left version.
			S32 idx = joint->getName().find("Left");
			if (idx != -1)
				mirror_joint_name.replace(idx, mirror_joint_name.length(), "Right");
			idx = joint->getName().find("Right");
			if (idx != -1)
				mirror_joint_name.replace(idx, mirror_joint_name.length(), "Left");

			LLJoint* mirror_joint = nullptr;
			if (mirror_joint_name != joint->getName())
				mirror_joint = gDragonAnimator.mTargetAvatar->mRoot->findJoint(mirror_joint_name);

			//BD - Get the rotation of the mirror bone (if available).
			//     Flip the mirror bone's rotation (if available) and apply it to our current bone.
			if (mirror_joint)
			{
				LLVector3 mirror_rot;
				LLQuaternion mirror_rot_quat;
				mirror_rot_quat = mirror_joint->getTargetRotation();
				LLQuaternion inv_mirror_rot_quat = LLQuaternion(-mirror_rot_quat.mQ[VX], mirror_rot_quat.mQ[VY], -mirror_rot_quat.mQ[VZ], mirror_rot_quat.mQ[VW]);
				inv_mirror_rot_quat.getEulerAngles(&mirror_rot[VX], &mirror_rot[VY], &mirror_rot[VZ]);
				joint->setTargetRotation(inv_mirror_rot_quat);

				LLScrollListCell* col_rot_x = item->getColumn(COL_ROT_X);
				LLScrollListCell* col_rot_y = item->getColumn(COL_ROT_Y);
				LLScrollListCell* col_rot_z = item->getColumn(COL_ROT_Z);

				col_rot_x->setValue(ll_round(mirror_rot.mV[VX], 0.001f));
				col_rot_y->setValue(ll_round(mirror_rot.mV[VY], 0.001f));
				col_rot_z->setValue(ll_round(mirror_rot.mV[VZ], 0.001f));
			}
		}
	}
}

void BDFloaterPoser::onJointCopyTransforms()
{
	LLScrollListItem* item = mJointScrolls[JOINTS]->getFirstSelected();
	LLJoint* joint = (LLJoint*)item->getUserdata();
	if (joint)
	{
		mClipboard["rot"] = joint->getTargetRotation().getValue();
		mClipboard["pos"] = joint->getTargetPosition().getValue();
		mClipboard["scale"] = joint->getScale().getValue();
		LL_INFOS("Posing") << "Copied all transforms " << LL_ENDL;
	}
}

bool BDFloaterPoser::onJointContextMenuEnable(const LLSD& param)
{
	std::string action = param.asString();
	if (action == "clipboard")
	{
		return mClipboard.has("rot");
	}
	if (action == "enable_bone")
	{
		LLScrollListItem* item = mAvatarScroll->getFirstSelected();
		if (!item) return false;

		LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
		if (!avatar || avatar->isDead()) return false;

		item = mJointScrolls[JOINTS]->getFirstSelected();
		if (item)
		{
			LLJoint* joint = (LLJoint*)item->getUserdata();
			if (joint)
			{
				BDPosingMotion* motion = (BDPosingMotion*)avatar->findMotion(ANIM_BD_POSING_MOTION);
				LLPose* pose = motion->getPose();
				if (pose)
				{
					LLPointer<LLJointState> joint_state = pose->findJointState(joint);
					return joint_state;
				}
			}
		}
	}
	return false;
}

void BDFloaterPoser::onJointContextMenuAction(const LLSD& param)
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string action = param.asString();
	if (action == "copy_transforms")
	{
		onJointCopyTransforms();
	}
	else if (action == "paste_rot")
	{
		onJointPasteRotation();
	}
	else if (action == "paste_pos")
	{
		onJointPastePosition();
	}
	else if (action == "paste_scale")
	{
		onJointPasteScale();
	}
	else if (action == "paste_rot_pos")
	{
		onJointPasteRotation();
		onJointPastePosition();
	}
	else if (action == "paste_rot_scale")
	{
		onJointPasteRotation();
		onJointPasteScale();
	}
	else if (action == "paste_pos_scale")
	{
		onJointPastePosition();
		onJointPasteScale();
	}
	else if (action == "paste_all")
	{
		onJointPasteRotation();
		onJointPastePosition();
		onJointPasteScale();
	}
	else if (action == "symmetrize")
	{
		onJointSymmetrize();
	}
	else if (action == "mirror")
	{
		onJointMirror();
	}
	else if (action == "recapture")
	{
		onJointRecapture();
	}
	else if (action == "enable_bone")
	{
		onJointChangeState();
	}
	else if (action == "enable_override")
	{

	}
	else if (action == "enable_offset")
	{

	}
	else if (action == "reset_rot")
	{
		onJointRotationReset();
	}
	else if (action == "reset_pos")
	{
		onJointPositionReset();
	}
	else if (action == "reset_scale")
	{
		onJointScaleReset();
	}
	else if (action == "reset_all")
	{
		//BD - We do all 3 here because the combined function resets all bones regardless of
		//     our selection, these only reset the selected ones.
		onJointRotationReset();
		onJointPositionReset();
		onJointScaleReset();
	}
}

//BD - This is used to collect all default values at the beginning to revert to later on.
void BDFloaterPoser::onCollectDefaults()
{
	LLQuaternion rot;
	LLVector3 pos;
	LLVector3 scale;
	LLJoint* joint;

	//BD - Getting collision volumes and attachment points.
	std::vector<std::string> joint_names, cv_names, attach_names;
	gAgentAvatarp->getSortedJointNames(0, joint_names);
	gAgentAvatarp->getSortedJointNames(1, cv_names);
	gAgentAvatarp->getSortedJointNames(2, attach_names);

	mDefaultRotations.clear();
	mDefaultScales.clear();
	mDefaultPositions.clear();

	for (auto name : joint_names)
	{
		joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		LLSD row;

		rot = joint->getTargetRotation();
		mDefaultRotations.insert(std::pair<std::string, LLQuaternion>(name, rot));

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		scale = joint->getScale();
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));

		//BD - We could just check whether position information is available since only joints
		//     which can have their position changed will have position information but we
		//     want this to be a minefield for crashes.
		//     Bones that can support position
		//     0, 9-37, 39-43, 45-59, 77, 97-107, 110, 112, 115, 117-121, 125, 128-129, 132
		if (joint->mHasPosition)
		{
			//BD - We always get the values but we don't write them out as they are not relevant for the
			//     user yet but we need them to establish default values we revert to later on.
			pos = joint->getPosition();
			mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		}
	}

	//BD - Collision Volumes
	for (auto name : cv_names)
	{
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		pos = joint->getPosition();
		scale = joint->getScale();
		mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));
	}

	//BD - Attachment Bones
	for (auto name : attach_names)
	{
		LLJoint* joint = gAgentAvatarp->getJoint(name);
		//BD - Nothing? Invalid? Skip, when we hit the end we'll break out anyway.
		if (!joint)	continue;

		//BD - We always get the values but we don't write them out as they are not relevant for the
		//     user yet but we need them to establish default values we revert to later on.
		pos = joint->getPosition();
		scale = joint->getScale();

		mDefaultPositions.insert(std::pair<std::string, LLVector3>(name, pos));
		mDefaultScales.insert(std::pair<std::string, LLVector3>(name, scale));
	}
}

////////////////////////////////
//BD - Animations
////////////////////////////////
void BDFloaterPoser::onAnimAdd(const LLSD& param)
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying()) return;

	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	LLScrollListItem* pose = mPoseScroll->getFirstSelected();
	std::string name = param.asString();
	S32 location = mAnimEditorScroll->getFirstSelectedIndex() + 1;
	F32 time = 0.f;
	BD_EActionType type;
	if (param.asString() == "Repeat")
	{
		type = BD_EActionType::REPEAT;
	}
	else if (param.asString() == "Wait")
	{
		type = BD_EActionType::WAIT;
		time = 1.f;
	}
	else
	{
		name = pose->getColumn(0)->getValue().asString();
		type = BD_EActionType::POSE;
	}
	//BD - We'll be adding the avatar into a list in here to keep track of all
	//     avatars we've added actions for.
	gDragonAnimator.onAddAction(avatar, name, type, time, location);
	onAnimListWrite();

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(location);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimListWrite()
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	mAnimEditorScroll->clearRows();
	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (auto action : avatar->mAnimatorActions)
	{
		LLSD row;
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = action.mPoseName;
		if (action.mType == BD_EActionType::WAIT)
		{
			row["columns"][1]["column"] = "time";
			row["columns"][1]["value"] = action.mTime;
			row["columns"][2]["column"] = "type";
			row["columns"][2]["value"] = action.mType;
		}
		mAnimEditorScroll->addElement(row);
	}

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();
}

void BDFloaterPoser::onAnimMove(const LLSD& param)
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying())	return;

	//BD - Don't allow moving if we don't have anything selected either. (Crashfix)
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 new_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 old_index = new_index;

	//BD - Don't allow moving down when we are already at the bottom, this would
	//     create another entry.
	//     Don't allow going up when we are on the first entry already either, this
	//     would select everything in the list.
	//     Don't allow moving if there's nothing to move. (Crashfix)
	if (((new_index + 1) >= item_count && param.asString() == "Down")
		|| (new_index == 0 && param.asString() == "Up")
		|| item_count == 0)
	{
		return;
	}

	//BD - Move up, otherwise move the entry down. No other option.
	if (param.asString() == "Up")
	{
		--new_index;
	}
	else
	{
		++new_index;
	}

	Action action = avatar->mAnimatorActions[old_index];
	gDragonAnimator.onDeleteAction(avatar, old_index);
	gDragonAnimator.onAddAction(avatar, action, new_index);
	onAnimListWrite();

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(new_index);

	//BD - Update our controls when we move items, the move and delete buttons
	//     should enable now that we might have selected something.
	onAnimControlsRefresh();
}

void BDFloaterPoser::onAnimDelete()
{
	//BD - Don't allow changing the list while it's currently playing, this might
	//     cause a crash when the animator is past the null check and we delete an
	//     entry between null checking and trying to read its values. 
	if (gDragonAnimator.getIsPlaying())	return;

	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::vector<LLScrollListItem*> items = mAnimEditorScroll->getAllSelected();
	while (!items.empty())
	{
		//BD - We'll delete the avatar from our list if necessary when no more actions
		//     are saved in a given avatar.
		gDragonAnimator.onDeleteAction(avatar, mAnimEditorScroll->getFirstSelectedIndex());
		items.erase(items.begin());
	}
	onAnimListWrite();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();

	//BD - Update our controls when we delete items. Most of them should
	//     disable now that nothing is selected anymore.
	onAnimControlsRefresh();
}

void BDFloaterPoser::loadPoseRotations(std::string name, LLVector3 *rotations)
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			if (pose["rotation"].isDefined())
			{
				rotations[joint_num].setValue(pose["rotation"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::loadPosePositions(std::string name, LLVector3 *positions)
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			//BD - Position information is only ever written when it is actually safe to do.
			//     It's safe to assume that IF information is available it's safe to apply.
			if (pose["position"].isDefined())
			{
				positions[joint_num].setValue(pose["position"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::loadPoseScales(std::string name, LLVector3 *scales)
{
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (!item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	std::string filename;
	if (!name.empty())
	{
		filename = gDirUtilp->getExpandedFilename(LL_PATH_POSES, gDragonLibrary.escapeString(name) + ".xml");
	}

	LLSD pose;
	llifstream infile;
	infile.open(filename);
	if (!infile.is_open())
	{
		LL_WARNS("Posing") << "Cannot find file in: " << filename << LL_ENDL;
		return;
	}

	while (!infile.eof())
	{
		S32 count = LLSDSerialize::fromXML(pose, infile);
		if (count == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("Posing") << "Failed to parse file: " << filename << LL_ENDL;
			return;
		}

		LLJoint* joint = avatar->getJoint(pose["bone"].asString());
		if (joint)
		{
			S32 joint_num = joint->getJointNum();
			//BD - Bone Scales
			if (pose["scale"].isDefined())
			{
				scales[joint_num].setValue(pose["scale"]);
			}
		}
	}
	infile.close();
	return;
}

void BDFloaterPoser::onAnimSet()
{
	LLScrollListItem* av_item = mAvatarScroll->getFirstSelected();
	if (!av_item) return;

	LLVOAvatar* avatar = (LLVOAvatar*)av_item->getUserdata();
	if (!avatar || avatar->isDead()) return;

	F32 value = getChild<LLUICtrl>("anim_time")->getValue().asReal();
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	Action action = avatar->mAnimatorActions[selected_index];
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		if (action.mType == BD_EActionType::WAIT)
		{
			avatar->mAnimatorActions[mAnimEditorScroll->getFirstSelectedIndex()].mTime = value;
			LLScrollListCell* column = item->getColumn(1);
			column->setValue(value);
		}
	}
}

void BDFloaterPoser::onAnimPlay()
{
	//BD - We start the animator frametimer here and set the expiry time to 0.0
	//     to force the animator to start immediately when hitting the "Start" button.
	gDragonAnimator.startPlayback();

	//BD - Update our controls when we start the animator. Most of them should
	//     disable now.
	onAnimControlsRefresh();
	onPoseControlsRefresh();
}

void BDFloaterPoser::onAnimStop()
{
	//BD - We only need to stop it here because the .start function resets the timer 
	//     automatically.
	gDragonAnimator.stopPlayback();

	//BD - Update our controls when we stop the animator. Most of them should
	//     enable again.
	onAnimControlsRefresh();
	onPoseControlsRefresh();
}

void BDFloaterPoser::onAnimControlsRefresh()
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 index = mAnimEditorScroll->getFirstSelectedIndex();

	bool is_playing = gDragonAnimator.getIsPlaying();
	bool selected = index != -1;
	bool is_wait = false;

	//BD - Don't.
	if (!is_playing)
	{
		LLScrollListItem* item = mAvatarScroll->getFirstSelected();
		if (item)
		{
			LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
			if (avatar || !avatar->isDead())
			{
				//BD - Crashfix.
				if (selected)
				{
					is_wait = avatar->mAnimatorActions[index].mType == BD_EActionType::WAIT;
				}
			}
		}
	}

	getChild<LLUICtrl>("anim_time")->setEnabled(!is_playing && selected && is_wait);
	getChild<LLUICtrl>("delete_entry")->setEnabled(!is_playing && selected);
	getChild<LLUICtrl>("move_up")->setEnabled(!is_playing && selected && index != 0);
	getChild<LLUICtrl>("move_down")->setEnabled(!is_playing && selected && !((index + 1) >= item_count));
	getChild<LLUICtrl>("add_repeat")->setEnabled(!is_playing);
	getChild<LLUICtrl>("add_wait")->setEnabled(!is_playing);
	getChild<LLUICtrl>("play_anim")->setEnabled(!is_playing && (item_count > 0));
	getChild<LLUICtrl>("stop_anim")->setEnabled(is_playing);
}


////////////////////////////////
//BD - Misc Functions
////////////////////////////////

void BDFloaterPoser::onUpdateLayout()
{
	if (!this->isMinimized())
	{
		bool animator_expanded = getChild<LLButton>("animator")->getValue();
		bool poses_expanded = getChild<LLButton>("extend")->getValue() || animator_expanded;
		getChild<LLUICtrl>("poses_layout")->setVisible(poses_expanded);
		getChild<LLUICtrl>("animator_layout")->setVisible(animator_expanded);
		getChild<LLUICtrl>("poser_layout")->setVisible(!animator_expanded);

		S32 collapsed_width = getChild<LLPanel>("min_panel")->getRect().getWidth();
		S32 expanded_width = getChild<LLPanel>("max_panel")->getRect().getWidth();
		S32 floater_width = (poses_expanded && !animator_expanded) ? expanded_width : collapsed_width;
		this->reshape(floater_width, this->getRect().getHeight());

		if (animator_expanded)
		{
			//BD - Refresh our animation list
			onAnimListWrite();
		}
	}
}

void BDFloaterPoser::onModeChange()
{
	gDragonAnimator.mLiveMode = !gDragonAnimator.mLiveMode;
	
	//BD - Recreate our avatar list first to weed out stale avatar/animesh references.
	onAvatarsRefresh();
	//BD - Refresh all our special export controls right away and select an avatar.
	//     We need this to prevent crashes and make the following widget refreshes work
	//     without user input.
	onCreationControlsRefresh();

	if (!gDragonAnimator.mLiveMode)
	{
		//BD - If we aren't posing yet, start Pose mode right now.
		BDPosingMotion* motion = (BDPosingMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
		if (!motion || motion->isStopped())
		{
			gAgent.stopFidget();
			gDragonStatus->setPosing(true);
			gAgentAvatarp->startMotion(ANIM_BD_POSING_MOTION);
			gAgentAvatarp->setPosing();
		}

		//BD - Leaving "Live Edit" mode means we have to go into T pose to allow exporting.
		onJointRotPosScaleReset();
	}
	
	onJointRefresh();
	onPoseControlsRefresh();
}

void BDFloaterPoser::onCreationControlsRefresh()
{
	//BD - Find our own avatar in the list and select it.
	//     We do this to properly fill the joint list and have our own avatar selected
	//     when we switch out of creation mode.
	mAvatarScroll->deselectAllItems();
	for (auto item : mAvatarScroll->getAllData())
	{
		LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
		if (avatar && avatar->isSelf())
		{
			item->setSelected(true);
		}
	}

	getChild<LLUICtrl>("toggle_mode")->setValue(gDragonAnimator.mLiveMode);
	setTitle(gDragonAnimator.mLiveMode ? getString("live_mode_str") : getString("creation_mode_str"));
	getChild<LLUICtrl>("poser_options_layout")->setVisible(gDragonAnimator.mLiveMode);
	getChild<LLUICtrl>("live_panel")->setVisible(gDragonAnimator.mLiveMode);
	getChild<LLUICtrl>("create_panel")->setVisible(!gDragonAnimator.mLiveMode);
	getChild<LLUICtrl>("recapture_bones")->setEnabled(gDragonAnimator.mLiveMode);
}

bool BDFloaterPoser::onSaveMenuEnable(const LLSD& param)
{
	std::string action = param.asString();
	if (action == "anim")
	{
		return !gDragonAnimator.mLiveMode;
	}
	return false;
}

//BD - Animesh
void BDFloaterPoser::onAvatarsSelect()
{
	//BD - Whenever we select an avatar in the list, check if the selected Avatar is still
	//     valid and/or if new avatars have become valid for posing.
	onAvatarsRefresh();

	//BD - Now that we selected an avatar we can refresh the joint list to have all bones
	//     mapped to that avatar so we can immediately start posing them or continue doing so.
	//     This will automatically invoke a onJointControlsRefresh()
	onJointRefresh();

	//BD - Now that we support animating multiple avatars we also need to refresh all controls
	//     and animation/pose lists for them when we switch to make it as easy as possible to
	//     quickly switch back and forth and make edits.
	onUpdateLayout();

	onPoseControlsRefresh();
	onAnimControlsRefresh();

	//BD - Disable the Start Posing button if we haven't loaded yet.
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (item)
	{
		LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
		if (avatar && avatar->isSelf())
			mStartPosingBtn->setEnabled(gAgentAvatarp->isFullyLoaded());
	}

	mStartPosingBtn->setFlashing(true, true);
}

void BDFloaterPoser::onAvatarsRefresh()
{
	//BD - Flag all items first, we're going to unflag them when they are valid.
	for (LLScrollListItem* item : mAvatarScroll->getAllData())
	{
		if (item)
		{
			item->setFlagged(TRUE);
		}
	}

	bool create_new = true;
	for (LLCharacter* character : LLCharacter::sInstances)
	{
		create_new = true;
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(character);
		if (avatar && !avatar->isControlAvatar()
			/*&& avatar->isSelf()*/)
		{
			LLUUID uuid = avatar->getID();
			for (LLScrollListItem* item : mAvatarScroll->getAllData())
			{
				if (avatar == item->getUserdata())
				{
					item->setFlagged(FALSE);
					//BD - When we refresh it might happen that we don't have a name for someone
					//     yet, when this happens the list entry won't be purged and rebuild as
					//     it will be updated with this part, so we have to update the name in
					//     case it was still being resolved last time we refreshed and created the
					//     initial list entry. This prevents the name from missing forever.
					if (item->getColumn(1)->getValue().asString().empty())
					{
						LLAvatarName av_name;
						LLAvatarNameCache::get(uuid, &av_name);
						item->getColumn(1)->setValue(av_name.getDisplayName());
					}

					create_new = false;
					break;
				}
			}

			if (create_new)
			{
				LLAvatarName av_name;
				LLAvatarNameCache::get(uuid, &av_name);

				LLSD row;
				row["columns"][0]["column"] = "icon";
				row["columns"][0]["type"] = "icon";
				row["columns"][0]["value"] = getString("icon_category");
				row["columns"][1]["column"] = "name";
				row["columns"][1]["value"] = av_name.getDisplayName();
				row["columns"][2]["column"] = "uuid";
				row["columns"][2]["value"] = avatar->getID();
				row["columns"][3]["column"] = "control_avatar";
				row["columns"][3]["value"] = false;
				LLScrollListItem* item = mAvatarScroll->addElement(row);
				item->setUserdata(avatar);

				//BD - We're just here to find ourselves, break out immediately when we are done.
				//break;
			}
		}
	}

	//BD - Animesh Support
	//     Search through all control avatars.
	for (auto character : LLControlAvatar::sInstances)
	{
		create_new = true;
		LLControlAvatar* avatar = dynamic_cast<LLControlAvatar*>(character);
		if (avatar && !avatar->isDead() && (avatar->getRegion() == gAgent.getRegion()))
		{
			LLUUID uuid = avatar->getID();
			for (LLScrollListItem* item : mAvatarScroll->getAllData())
			{
				if (item)
				{
					if (avatar == item->getUserdata())
					{
						//BD - Avatar is still valid unflag it from removal.
						item->setFlagged(FALSE);
						create_new = false;
						break;
					}
				}
			}

			if (create_new)
			{
				//BD - Avatar was not listed yet, create a new entry.
				LLSD row;
				row["columns"][0]["column"] = "icon";
				row["columns"][0]["type"] = "icon";
				row["columns"][0]["value"] = getString("icon_object");
				row["columns"][1]["column"] = "name";
				row["columns"][1]["value"] = avatar->getFullname();
				row["columns"][2]["column"] = "uuid";
				row["columns"][2]["value"] = avatar->getID();
				row["columns"][3]["column"] = "control_avatar";
				row["columns"][3]["value"] = true;
				LLScrollListItem* item = mAvatarScroll->addElement(row);
				item->setUserdata(avatar);
			}
		}
	}

	//BD - Now safely delete all items so we can start adding the missing ones.
	mAvatarScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAvatarScroll->updateLayout();
}


////////////////////////////////
//BD - Experimental Functions
////////////////////////////////

/*void BDFloaterPoser::onAnimEdit(LLUICtrl* ctrl, const LLSD& param)
{
	getChild<LLMultiSliderCtrl>("key_slider")->clear();
	LLUUID id = LLUUID("cd9b0386-b26d-e860-0114-d879ee12a777");

	//LLKeyframeMotion* motion = (LLKeyframeMotion*)gAgentAvatarp->findMotion(id);
	typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;

	LLKeyframeMotion::JointMotionList* jointmotion_list;
	jointmotion_list = LLKeyframeDataCache::getKeyframeData(id);
	S32 i = 0;

	if (!jointmotion_list)
	{
		return;
	}

	LLScrollListItem* item = mJointScrolls[JOINTS]->getLastSelectedItem();
	if (!item)
	{
		return;
	}

	LLJoint* joint = (LLJoint*)item->getUserdata();
	i = joint->mJointNum;

	LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->getJointMotion(i);
	LLKeyframeMotion::RotationCurve rot_courve = joint_motion->mRotationCurve;
	LLKeyframeMotion::RotationKey rot_key;
	LLQuaternion rotation;
	F32 time;
	LLKeyframeMotion::RotationCurve::key_map_t keys = rot_courve.mKeys;

	for (LLKeyframeMotion::RotationCurve::key_map_t::const_iterator iter = keys.begin();
		iter != keys.end(); ++iter)
	{
		time = iter->first;
		rot_key = iter->second;

		//const std::string& sldr_name = getChild<LLMultiSliderCtrl>("key_slider")->addSlider(time);
		getChild<LLMultiSliderCtrl>("key_slider")->addSlider(time);
		//getChild<LLMultiSliderCtrl>("key_slider")->getSliderValue();
	}
}

void BDFloaterPoser::onAddKey()
{
	//S32 max_sliders = 60;

	//if ((S32)mSliderToKey.size() >= max_sliders)
	//{
	//	LLSD args;
	//	args["MAX"] = max_sliders;
	//	//LLNotificationsUtil::add("DayCycleTooManyKeyframes", args, LLSD(), LLNotificationFunctorRegistry::instance().DONOTHING);
	//	return;
	//}

	// add the slider key
	std::string key_val = mPoseScroll->getFirstSelected()->getColumn(0)->getValue().asString();
	BDPoseKey pose_params(key_val);

	F32 time = mTimeSlider->getCurSliderValue();
	addSliderKey(time, pose_params);
}

void BDFloaterPoser::addSliderKey(F32 time, BDPoseKey keyframe)
{
	// make a slider
	const std::string& sldr_name = mKeySlider->addSlider(time);
	if (sldr_name.empty())
	{
		return;
	}

	// set the key
	SliderKey newKey(keyframe, mKeySlider->getCurSliderValue());

	// add to map
	mSliderToKey.insert(std::pair<std::string, SliderKey>(sldr_name, newKey));
}

void BDFloaterPoser::onTimeSliderMoved()
{
}

void BDFloaterPoser::onKeyTimeMoved()
{
	if (mKeySlider->getValue().size() == 0)
	{
		return;
	}

	// make sure we have a slider
	const std::string& cur_sldr = mKeySlider->getCurSlider();
	if (cur_sldr == "")
	{
		return;
	}

	F32 time24 = mKeySlider->getCurSliderValue();

	// check to see if a key exists
	BDPoseKey key = mSliderToKey[cur_sldr].keyframe;
	//// _LL_DEBUGS() << "Setting key time: " << time24 << LL_ENDL;
	mSliderToKey[cur_sldr].time = time24;

	// if it exists, turn on check box
	//mSkyPresetsCombo->selectByValue(key.toStringVal());

	//mTimeCtrl->setTime24(time24);
}

void BDFloaterPoser::onKeyTimeChanged()
{
	// if no keys, skipped
	if (mSliderToKey.size() == 0)
	{
		return;
	}

	F32 time24 = getChild<LLTimeCtrl>("time_control")->getTime24();

	const std::string& cur_sldr = mKeySlider->getCurSlider();
	mKeySlider->setCurSliderValue(time24, TRUE);
	F32 time = mKeySlider->getCurSliderValue() / 60;

	// now set the key's time in the sliderToKey map
	//// _LL_DEBUGS() << "Setting key time: " << time << LL_ENDL;
	mSliderToKey[cur_sldr].time = time;

	//applyTrack();
}

void BDFloaterPoser::onDeleteKey()
{
	if (mSliderToKey.size() == 0)
	{
		return;
	}

	// delete from map
	const std::string& sldr_name = mKeySlider->getCurSlider();
	std::map<std::string, SliderKey>::iterator mIt = mSliderToKey.find(sldr_name);
	mSliderToKey.erase(mIt);

	mKeySlider->deleteCurSlider();

	if (mSliderToKey.size() == 0)
	{
		return;
	}

	const std::string& name = mKeySlider->getCurSlider();
	//mSkyPresetsCombo->selectByValue(mSliderToKey[name].keyframe.toStringVal());
	F32 time24 = mSliderToKey[name].time;

	getChild<LLTimeCtrl>("time_control")->setTime24(time24);
}

void BDFloaterPoser::onAnimSetValue(LLUICtrl* ctrl, const LLSD& param)
{
	F32 val = ctrl->getValue().asReal();
	mKeySlider->setValue(val);
}*/
