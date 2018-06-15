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

#include "llviewerprecompiledheaders.h"

#include "bdfloateranimations.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llbvhloader.h"
#include "llcallingcard.h"
#include "lldatapacker.h"
#include "llfilepicker.h"
#include "llkeyframemotion.h"
#include "llvfile.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"

BDFloaterAnimations::BDFloaterAnimations(const LLSD& key)
	:	LLFloater(key)
{
	//BD - This is the global command from which on we decide what to do. We had them all separately before.
	//     "Stop" - Stops an animation complete, essentially just reset animation.
	//     "Freeze" - Pauses an animation just like if we set its playback speed to 0.0.
	//     "Set" - Changes the animation playback speed to the desired value.
	//     "Restart" - Restarts the animation, could be used as animation sync.
	//     "Save" - Save a motion to be copied to other avatars.
	//     "Load" - Copy a motion to a selected avatar and make it play it.
	//     "Remove" - Remove all selected motions from our list.
	//     "Create" - Create a new motion via a given UUID.
	mCommitCallbackRegistrar.add("Motion.Command", boost::bind(&BDFloaterAnimations::onMotionCommand, this, _1, _2));

	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("Motion.Refresh", boost::bind(&BDFloaterAnimations::onMotionRefresh, this));

	/*
	//BD - Add a new entry to the animation creator.
	mCommitCallbackRegistrar.add("Anim.Add", boost::bind(&BDFloaterAnimations::onAnimAdd, this, _2));
	//BD - Move the selected entry one row up.
	mCommitCallbackRegistrar.add("Anim.Move", boost::bind(&BDFloaterAnimations::onAnimMove, this, _2));
	//BD - Remove an entry in the animation creator.
	mCommitCallbackRegistrar.add("Anim.Delete", boost::bind(&BDFloaterAnimations::onAnimDelete, this));
	//BD - Save the currently build list as animation.
	mCommitCallbackRegistrar.add("Anim.Save", boost::bind(&BDFloaterAnimations::onAnimSave, this));
	//BD - Play the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Play", boost::bind(&BDFloaterAnimations::onAnimPlay, this));
	//BD - Stop the current animator queue.
	mCommitCallbackRegistrar.add("Anim.Stop", boost::bind(&BDFloaterAnimations::onAnimStop, this));
	//BD - Change the value for a wait entry.
	mCommitCallbackRegistrar.add("Anim.Set", boost::bind(&BDFloaterAnimations::onAnimSet, this));
	*/
}

BDFloaterAnimations::~BDFloaterAnimations()
{
}

BOOL BDFloaterAnimations::postBuild()
{
	//BD - Motions
	mAvatarScroll = getChild<LLScrollListCtrl>("other_avatars_scroll");
	mMotionScroll = getChild<LLScrollListCtrl>("motions_scroll");

	/*
	//BD - Animations
	mAnimEditorScroll = this->getChild<LLScrollListCtrl>("anim_editor_scroll", true);
	mAnimEditorScroll->setCommitCallback(boost::bind(&BDFloaterAnimations::onAnimControlsRefresh, this));
	mAnimScrollIndex = 0;
	*/

	return TRUE;
}

void BDFloaterAnimations::draw()
{
	/*
	//BD - This only works while the window is visible, hiding the UI will stop it.
	if (mAnimPlayTimer.getStarted() &&
		mAnimPlayTimer.getElapsedTimeF32() > mExpiryTime)
	{
		mAnimPlayTimer.stop();
		if (mAnimEditorScroll->getItemCount() != 0)
		{
			mAnimEditorScroll->selectNthItem(mAnimScrollIndex);
			LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
			if (item)
			{
				//BD - We can't use Wait or Restart as label, need to fix this.
				std::string label = item->getColumn(0)->getValue().asString();
				if (label == "Wait")
				{
					//BD - Do nothing?
					mExpiryTime = item->getColumn(1)->getValue().asReal();
					mAnimScrollIndex++;
				}
				else if (label == "Repeat")
				{
					mAnimScrollIndex = 0;
					mExpiryTime = 0.0f;
				}
				else
				{
					mExpiryTime = 0.0f;
					onPoseLoad(label);
					mAnimScrollIndex++;
				}
			}
		}

		if (mAnimEditorScroll->getItemCount() != mAnimScrollIndex)
		{
			mAnimPlayTimer.start();
		}
	}
	*/
	LLFloater::draw();
}

void BDFloaterAnimations::onOpen(const LLSD& key)
{
	//BD - Make these trigger only when opening the actual tab.
	onMotionRefresh();
}

void BDFloaterAnimations::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mAvatarScroll->clearRows();
	mMotionScroll->clearRows();
}

////////////////////////////////
//BD - Motions
////////////////////////////////
void BDFloaterAnimations::onMotionRefresh()
{
	//BD - Flag all items first, we're going to unflag them when they are valid.
	std::vector<LLScrollListItem*> items = mAvatarScroll->getAllData();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			item->setFlagged(TRUE);
		}
	}

	bool create_new;
	LLAvatarTracker& at = LLAvatarTracker::instance();
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*iter);
		if (avatar)
		{
			LLUUID uuid = avatar->getID();
			create_new = true;
			std::vector<LLScrollListItem*> items = mAvatarScroll->getAllData();
			for (std::vector<LLScrollListItem*>::iterator it = items.begin();
				it != items.end(); ++it)
			{
				LLScrollListItem* item = (*it);
				if (avatar == item->getUserdata())
				{
					item->setFlagged(FALSE);

					//BD - When we refresh it might happen that we don't have a name for someone
					//     yet, when this happens the list entry won't be purged and rebuild as
					//     it will be updated with this part, so we have to update the name in
					//     case it was still being resolved last time we refreshed and created the
					//     initial list entry. This prevents the name from missing forever.
					if (item->getColumn(0)->getValue().asString().empty())
					{
						LLAvatarName av_name;
						LLAvatarNameCache::get(uuid, &av_name);
						item->getColumn(0)->setValue(av_name.getDisplayName());
					}

					//BD - Show if we got the permission to animate them and save their motions.
					std::string str = "-";
					const LLRelationship* relation = at.getBuddyInfo(uuid);
					if (relation)
					{
						str = relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "Yes" : "No";
					}
					else
					{
						str = uuid == gAgentID ? "Yes" : "-";
					}
					item->getColumn(3)->setValue(str);

					F32 value = avatar->getMotionController().getTimeFactor();
					item->getColumn(2)->setValue(value);
					create_new = false;
					break;
				}
			}

			if (create_new)
			{
				F32 value = avatar->getMotionController().getTimeFactor();
				LLAvatarName av_name;
				LLAvatarNameCache::get(uuid, &av_name);
				const LLRelationship* relation = at.getBuddyInfo(uuid);

				std::string str = "-";
				if (relation)
				{
					str = relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS) ? "Yes" : "No";
				}
				else
				{
					str = uuid == gAgentID ? "Yes" : "-";
				}

				LLSD row;
				row["columns"][0]["column"] = "name";
				row["columns"][0]["value"] = av_name.getDisplayName();
				row["columns"][1]["column"] = "uuid";
				row["columns"][1]["value"] = uuid.asString();
				row["columns"][2]["column"] = "value";
				row["columns"][2]["value"] = value;
				//BD - Show if we got the permission to animate them and save their motions.
				row["columns"][3]["column"] = "permission";
				row["columns"][3]["value"] = str;
				LLScrollListItem* element = mAvatarScroll->addElement(row);
				element->setUserdata(avatar);
			}
		}
	}

	//BD - Now safely delete all items so we can start adding the missing ones.
	mAvatarScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAvatarScroll->updateLayout();
}

void BDFloaterAnimations::onMotionCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		//BD - First lets get all selected characters, we'll need them for all
		//     the things we're going to do.
		std::vector<LLScrollListItem*> items = mAvatarScroll->getAllSelected();
		for (std::vector<LLScrollListItem*>::iterator item = items.begin();
			item != items.end(); ++item)
		{
			LLScrollListItem* element = (*item);
			LLVOAvatar* avatar = (LLVOAvatar*)element->getUserdata();

			if (!avatar->isDead())
			{
				if (param.asString() == "Freeze")
				{
					if (avatar->getMotionController().isPaused())
					{
						avatar->getMotionController().unpauseAllMotions();
					}
					else
					{
						mAvatarPauseHandles.push_back(avatar->requestPause());
					}
				}
				else if (param.asString() == "Set")
				{
					avatar->getMotionController().setTimeFactor(getChild<LLUICtrl>("anim_factor")->getValue().asReal());
					onMotionRefresh();
				}
				else if (param.asString() == "Restart")
				{
					LLMotionController::motion_list_t motions = avatar->getMotionController().getActiveMotions();
					for (LLMotionController::motion_list_t::iterator it = motions.begin();
						it != motions.end(); ++it)
					{
						LLKeyframeMotion* motion = (LLKeyframeMotion*)*it;
						if (motion)
						{
							LLUUID motion_id = motion->getID();
							avatar->stopMotion(motion_id, 1);
							avatar->startMotion(motion_id, 0.0f);
						}
					}
				}
				else if (param.asString() == "Stop")
				{
					//BD - Special case to prevent the posing animation from becoming stuck.
					if (avatar->getID() == gAgentID)
					{
						gAgentAvatarp->stopMotion(ANIM_BD_POSING_MOTION);
						gAgent.clearPosing();
					}

					avatar->deactivateAllMotions();
				}
				else if (param.asString() == "Remove")
				{
					mMotionScroll->deleteSelectedItems();

					//BD - Make sure we don't have a scrollbar unless we need it.
					mMotionScroll->updateLayout();
				}
				else if (param.asString() == "Save")
				{
					LLAvatarTracker& at = LLAvatarTracker::instance();
					//BD - Only allow saving motions whose object mod rights we have.
					const LLRelationship* relation = at.getBuddyInfo(avatar->getID());
					if (relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS)
						|| avatar->getID() == gAgentID)
					{
						LLMotionController::motion_list_t motions = avatar->getMotionController().getActiveMotions();
						for (LLMotionController::motion_list_t::iterator it = motions.begin();
							it != motions.end(); ++it)
						{
							LLKeyframeMotion* motion = (LLKeyframeMotion*)*it;
							if (motion)
							{
								LLUUID motion_id = motion->getID();
								LLSD row;
								LLAvatarName av_name;
								LLAvatarNameCache::get(avatar->getID(), &av_name);
								row["columns"][0]["column"] = "name";
								row["columns"][0]["value"] = av_name.getDisplayName();
								row["columns"][1]["column"] = "uuid";
								row["columns"][1]["value"] = motion_id.asString();
								row["columns"][2]["column"] = "priority";
								row["columns"][2]["value"] = motion->getPriority();
								row["columns"][3]["column"] = "duration";
								row["columns"][3]["value"] = motion->getDuration();
								row["columns"][4]["column"] = "blend type";
								row["columns"][4]["value"] = motion->getBlendType();
								row["columns"][5]["column"] = "loop";
								row["columns"][5]["value"] = motion->getLoop();
								row["columns"][6]["column"] = "easein";
								row["columns"][6]["value"] = motion->getEaseInDuration();
								row["columns"][7]["column"] = "easeout";
								row["columns"][7]["value"] = motion->getEaseOutDuration();
								if (motion->getName().empty() &&
									motion->getDuration() > 0.0f)
								{
									mMotionScroll->addElement(row);
								}
							}

							//BD - Will be using this later to export our finished animations.
							/*if (kf_motion && kf_motion->getName().empty() &&
								kf_motion->getDuration() > 0.0f)
							{
								kf_motion->dumpToFile("");

								//S32 anim_file_size = kf_motion->getFileSize();
								//U8* anim_data = new U8[anim_file_size];

								//LLDataPackerBinaryBuffer dp(anim_data, anim_file_size);
								//kf_motion->deserialize(dp, true);
							}*/
						}
					}
				}
				else if (param.asString() == "Load")
				{
					//BD - Only allow animating people whose object mod rights we have.
					//const LLRelationship* relation = at.getBuddyInfo(character->getID());
					//if (relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS)
					//	|| character->getID() == gAgentID)
					//if (character->getID() == gAgentID)
					//{
						//BD - Only allow animating ourselves, as per Oz Linden.
						LLScrollListItem* item = mMotionScroll->getFirstSelected();
						if (item)
						{
							LLUUID motion_id = item->getColumn(1)->getValue().asUUID();
							if (!motion_id.isNull())
							{
								gAgentAvatarp->createMotion(motion_id);
								gAgentAvatarp->stopMotion(motion_id, 1);
								gAgentAvatarp->startMotion(motion_id, 0.0f);
							}
						}
					//}
				}
			}
		}

		//BD - Create a new motion from an anim file from disk.
		//     This is essentially like having animation preview inworld on your avatar without
		//     having to upload it. LocalBitmap, more like LocalMotion.
		if (param.asString() == "Create")
		{
			//BD - Let us pick the file we want to preview with the filepicker.
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
				temp_motion = (LLKeyframeMotion*)gAgentAvatarp->createMotion(mMotionID);

				//BD - Find and open the file, we'll need to write it temporarily into the VFS pool.
				infile.open(outfilename, LL_APR_RB, NULL, &file_size);
				if (infile.getFileHandle())
				{
					U8 *anim_data;
					S32 anim_file_size;

					LLVFile file(gVFS, mMotionID, LLAssetType::AT_ANIMATION, LLVFile::WRITE);
					file.setMaxSize(file_size);
					const S32 buf_size = 65536;
					U8 copy_buf[buf_size];
					while ((file_size = infile.read(copy_buf, buf_size)))
					{
						file.write(copy_buf, file_size);
					}

					//BD - Now that we wrote the temporary file, find it and use it to set the size
					//     and buffer into which we will unpack the .anim file into.
					LLVFile* anim_file = new LLVFile(gVFS, mMotionID, LLAssetType::AT_ANIMATION);
					anim_file_size = anim_file->getSize();
					anim_data = new U8[anim_file_size];
					anim_file->read(anim_data, anim_file_size);

					//BD - Cleanup everything we don't need anymore.
					delete anim_file;
					anim_file = NULL;

					//BD - Use the datapacker now to actually deserialize and unpack the animation
					//     into our temporary motion so we can use it after we added it into the list.
					LLDataPackerBinaryBuffer dp(anim_data, anim_file_size);
					success = temp_motion && temp_motion->deserialize(dp, mMotionID, true);

					//BD - Cleanup the rest.
					delete[]anim_data;
				}

				//BD - Now write an entry with all given information into our list so we can use it.
				if (success)
				{
					LLUUID motion_id = temp_motion->getID();
					LLSD row;
					LLAvatarName av_name;
					LLAvatarNameCache::get(gAgentID, &av_name);
					row["columns"][0]["column"] = "name";
					row["columns"][0]["value"] = av_name.getDisplayName();
					row["columns"][1]["column"] = "uuid";
					row["columns"][1]["value"] = motion_id.asString();
					row["columns"][2]["column"] = "priority";
					row["columns"][2]["value"] = temp_motion->getPriority();
					row["columns"][3]["column"] = "duration";
					row["columns"][3]["value"] = temp_motion->getDuration();
					row["columns"][4]["column"] = "blend type";
					row["columns"][4]["value"] = temp_motion->getBlendType();
					row["columns"][5]["column"] = "loop";
					row["columns"][5]["value"] = temp_motion->getLoop();
					row["columns"][6]["column"] = "easein";
					row["columns"][6]["value"] = temp_motion->getEaseInDuration();
					row["columns"][7]["column"] = "easeout";
					row["columns"][7]["value"] = temp_motion->getEaseOutDuration();
					if (temp_motion->getName().empty() &&
						temp_motion->getDuration() > 0.0f)
					{
						mMotionScroll->addElement(row);
					}
				}
			}
		}
	}
}

/*
////////////////////////////////
//BD - Animations
////////////////////////////////
void BDFloaterAnimations::onAnimAdd(const LLSD& param)
{
	//BD - This is going to be a really dirty way of inserting elements inbetween others
	//     until i can figure out a better way.
	S32 selected_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 index = 0;
	std::vector<LLScrollListItem*> old_items = mAnimEditorScroll->getAllData();
	std::vector<LLScrollListItem*> new_items;
	LLScrollListItem* new_item;
	LLSD row;

	if (param.asString() == "Repeat")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Repeat";
	}
	else if (param.asString() == "Wait")
	{
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = "Wait";
		row["columns"][1]["column"] = "time";
		row["columns"][1]["value"] = 1.f;
	}
	else
	{
		LLScrollListItem* pose = mPoseScroll->getFirstSelected();
		if (pose)
		{
			//BD - Replace with list fill.
			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = pose->getColumn(0)->getValue().asString();
		}
		else
		{
			//BD - We should return here since when we dont add anything we don't need to do the rest.
			//     This MIGHT actually cause the issues users are seeing that i cannot reproduce.
			LL_WARNS("Animator") << "No scroll list item was selected or found, return." << LL_ENDL;
			return;
		}
	}

	new_item = mAnimEditorScroll->addElement(row);

	if (!new_item)
	{
		LL_WARNS("Animator") << "Item we wrote is invalid, abort." << LL_ENDL;
		new_item->setFlagged(true);
		mAnimEditorScroll->deleteFlaggedItems();
		return;
	}

	//BD - We can skip this on first add.
	if (selected_index != -1)
	{
		//BD - Let's go through all entries in the list and copy them into a new
		//     list in our new desired order, flag the old ones for removal while
		//     we do this.
		for (std::vector<LLScrollListItem*>::iterator it = old_items.begin();
			it != old_items.end(); ++it)
		{
			LLScrollListItem* item = (*it);
			if (item)
			{
				new_items.push_back(item);
			}

			if (index == selected_index)
			{
				new_items.push_back(new_item);
			}
			index++;
		}

		onAnimListWrite(new_items);
	}

	//BD - Select added entry and make it appear as nothing happened.
	//     In case of nothing being selected yet, select the first entry.
	mAnimEditorScroll->selectNthItem(selected_index + 1);

	//BD - Update our controls when we add items, the move and delete buttons
	//     should enable now that we selected something.
	onAnimControlsRefresh();
}

void BDFloaterAnimations::onAnimListWrite(std::vector<LLScrollListItem*> item_list)
{
	//BD - Now go through the new list we created, read them out and add them
	//     to our list in the new desired order.
	for (std::vector<LLScrollListItem*>::iterator it = item_list.begin();
		it != item_list.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			item->setFlagged(true);

			LLSD row;
			LLScrollListCell* column = item->getColumn(0);
			if (column)
			{
				std::string value_str = column->getValue().asString();
				row["columns"][0]["column"] = "name";
				row["columns"][0]["value"] = value_str;
				if (value_str == "Wait")
				{
					row["columns"][1]["column"] = "time";
					row["columns"][1]["value"] = item->getColumn(1)->getValue().asString();
				}
				LLScrollListItem* new_item = mAnimEditorScroll->addElement(row);
				new_item->setFlagged(false);
			}
		}
		else
		{
			LL_WARNS("Animator") << "Invalid element or nothing in list." << LL_ENDL;
		}
	}

	//BD - Delete all flagged items now and we'll end up with a new list order.
	mAnimEditorScroll->deleteFlaggedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();
}

void BDFloaterAnimations::onAnimMove(const LLSD& param)
{
	std::vector<LLScrollListItem*> old_items = mAnimEditorScroll->getAllData();
	std::vector<LLScrollListItem*> new_items;
	S32 new_index = mAnimEditorScroll->getFirstSelectedIndex();
	S32 index = 0;
	bool skip = false;

	//BD - Don't allow moving down when we are already at the bottom, this would
	//     create another entry.
	//     Don't allow going up when we are on the first entry already either, this
	//     would select everything in the list.
	//     Don't allow moving if there's nothing to move. (Crashfix)
	if (((new_index + 1) >= old_items.size() && param.asString() == "Down")
		|| (new_index == 0 && param.asString() == "Up")
		|| old_items.empty())
	{
		LL_WARNS("Animator") << "We are already at the top/bottom!" << LL_ENDL;
		return;
	}

	LLScrollListItem* cur_item = mAnimEditorScroll->getFirstSelected();

	//BD - Don't allow moving if we don't have anything selected either. (Crashfix)
	if (!cur_item)
	{
		LL_WARNS("Animator") << "Nothing selected to move, abort." << LL_ENDL;
		return;
	}

	//BD - Move up, otherwise move the entry down. No other option.
	if (param.asString() == "Up")
	{
		new_index--;
	}
	else
	{
		mAnimEditorScroll->selectNthItem(new_index + 1);
	}

	cur_item = mAnimEditorScroll->getFirstSelected();
	cur_item->setFlagged(true);

	//BD - Let's go through all entries in the list and copy them into a new
	//     list in our new desired order, flag the old ones for removal while
	//     we do this.
	//     For both up and down this works exactly the same, we made sure of that.
	for (std::vector<LLScrollListItem*>::iterator it = old_items.begin();
		it != old_items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			item->setFlagged(true);
			if (!skip)
			{
				if (index == new_index)
				{
					new_items.push_back(cur_item);
					skip = true;
				}

				new_items.push_back(item);
			}
			else
			{
				skip = false;
			}
		}
		index++;
	}

	onAnimListWrite(new_items);

	if (param.asString() == "Up")
	{
		mAnimEditorScroll->selectNthItem(new_index);
	}
	else
	{
		mAnimEditorScroll->selectNthItem(new_index + 1);
	}
}

void BDFloaterAnimations::onAnimDelete()
{
	mAnimEditorScroll->deleteSelectedItems();

	//BD - Make sure we don't have a scrollbar unless we need it.
	mAnimEditorScroll->updateLayout();

	//BD - Update our controls when we delete items. Most of them should
	//     disable now that nothing is selected anymore.
	onAnimControlsRefresh();
}

void BDFloaterAnimations::onAnimSave()
{
	//BD - Work in Progress exporter.
	//LLKeyframeMotion* motion = (LLKeyframeMotion*)gAgentAvatarp->findMotion(ANIM_BD_POSING_MOTION);
	//LLKeyframeMotion::JointMotionList* jointmotion_list;
	//jointmotion_list = LLKeyframeDataCache::getKeyframeData(ANIM_BD_POSING_MOTION);
	//typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;
	//LLKeyframeMotion::JointMotion* joint_motion = jointmotion_list->getJointMotion(0);
	//LLKeyframeMotion::RotationCurve rot_courve = joint_motion->mRotationCurve;
	//LLKeyframeMotion::RotationKey rot_key = rot_courve.mLoopInKey;
	//LLQuaternion rotation = rot_key.mRotation;
	//F32 time = rot_key.mTime;
}

void BDFloaterAnimations::onAnimSet()
{
	F32 value = getChild<LLUICtrl>("anim_time")->getValue().asReal();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		LLScrollListCell* column = item->getColumn(1);
		if (item->getColumn(0)->getValue().asString() == "Wait")
		{
			column->setValue(value);
		}
	}
}

void BDFloaterAnimations::onAnimPlay()
{
	mExpiryTime = 0.0f;
	mAnimPlayTimer.start();
}

void BDFloaterAnimations::onAnimStop()
{
	mAnimPlayTimer.stop();
}

void BDFloaterAnimations::onAnimControlsRefresh()
{
	S32 item_count = mAnimEditorScroll->getItemCount();
	S32 index = mAnimEditorScroll->getFirstSelectedIndex();
	LLScrollListItem* item = mAnimEditorScroll->getFirstSelected();
	if (item)
	{
		getChild<LLUICtrl>("delete_poses")->setEnabled(true);
		getChild<LLUICtrl>("move_up")->setEnabled(index != 0);
		getChild<LLUICtrl>("move_down")->setEnabled(!((index + 1) >= item_count));

		if (item->getColumn(0)->getValue().asString() == "Wait")
		{
			getChild<LLUICtrl>("anim_time")->setEnabled(true);
		}
		else
		{
			getChild<LLUICtrl>("anim_time")->setEnabled(false);
		}
	}
	else
	{
		getChild<LLUICtrl>("delete_poses")->setEnabled(false);
		getChild<LLUICtrl>("anim_time")->setEnabled(false);
		getChild<LLUICtrl>("move_up")->setEnabled(false);
		getChild<LLUICtrl>("move_down")->setEnabled(false);
	}
}
*/

////////////////////////////////
//BD - Misc Functions
////////////////////////////////