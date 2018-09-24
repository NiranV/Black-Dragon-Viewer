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

//BD - Animesh Support
#include "llcontrolavatar.h"

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
}

BDFloaterAnimations::~BDFloaterAnimations()
{
}

BOOL BDFloaterAnimations::postBuild()
{
	//BD - Motions
	mAvatarScroll = getChild<LLScrollListCtrl>("other_avatars_scroll");
	mMotionScroll = getChild<LLScrollListCtrl>("motions_scroll");

	return TRUE;
}

void BDFloaterAnimations::draw()
{
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
	for (LLScrollListItem* item : mAvatarScroll->getAllData())
	{
		if (item)
		{
			item->setFlagged(TRUE);
		}
	}

	bool create_new = true;
	//BD - Animesh Support
	//     Search through Animesh first so that the rest of this function will automatically
	//     skip all animesh entries and simply just check non-Animesh entries.
	//     We do this because the LLCharacter::sInstances now also contains all Animesh instances
	//     as well because they are handled as such internally, which is both good and bad.
	for (LLCharacter* character : LLControlAvatar::sInstances)
	{
		LLControlAvatar* avatar = dynamic_cast<LLControlAvatar*>(character);
		if (avatar)
		{
			LLUUID uuid = avatar->getID();
			for (LLScrollListItem* item : mAvatarScroll->getAllData())
			{
				if (item)
				{
					if (avatar == item->getUserdata())
					{
						item->setFlagged(FALSE);

						LL_INFOS("Animating") << "Animesh added: " << avatar->mIsControlAvatar << LL_ENDL;

						std::string str = "Yes";
						F32 value = avatar->getMotionController().getTimeFactor();
						item->getColumn(2)->setValue(value);
						item->getColumn(3)->setValue(str);
						create_new = false;
						break;
					}
				}
			}

			if (create_new)
			{
				F32 value = avatar->getMotionController().getTimeFactor();

				LLSD row;
				row["columns"][0]["column"] = "name";
				row["columns"][0]["value"] = avatar->getFullname();
				row["columns"][1]["column"] = "uuid";
				row["columns"][1]["value"] = uuid.asString();
				row["columns"][2]["column"] = "value";
				row["columns"][2]["value"] = value;
				//BD - Animesh always has editing permission since these are not actual avatars.
				//     Allow doing whatever we want with them.
				row["columns"][3]["column"] = "permission";
				row["columns"][3]["value"] = "Yes";
				LLScrollListItem* element = mAvatarScroll->addElement(row);
				element->setUserdata(avatar);
			}
		}
	}

	LLAvatarTracker& at = LLAvatarTracker::instance();
	for (LLCharacter* character : LLCharacter::sInstances)
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(character);
		//BD - Don't even bother with control avatars, we already added them.
		LL_INFOS("Animating") << "Tried to add animesh: " << avatar->mIsControlAvatar << LL_ENDL;
		if (avatar && !avatar->mIsControlAvatar)
		{
			LLUUID uuid = avatar->getID();
			for (LLScrollListItem* item : mAvatarScroll->getAllData())
			{
				if (item)
				{
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
		for (LLScrollListItem* element : mAvatarScroll->getAllSelected())
		{
			LLVOAvatar* avatar = (LLVOAvatar*)element->getUserdata();
			//BD - Animesh Support
			bool is_animesh = false;
			LLControlAvatar* animesh = (LLControlAvatar*)element->getUserdata();
			if (animesh)
			{
				is_animesh = animesh->mPlaying;
				LL_INFOS("Animating") << "Animesh added: " << is_animesh << LL_ENDL;
			}

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
					for (LLMotion* motion : motions)
					{
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
					if ((relation && relation->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS))
						|| avatar->getID() == gAgentID || is_animesh)
					{
						LLMotionController::motion_list_t motions = avatar->getMotionController().getActiveMotions();
						for (LLMotion* motion : motions)
						{
							if (motion)
							{
								LLUUID motion_id = motion->getID();
								LLSD row;
								LLAvatarName av_name;
								LLAvatarNameCache::get(avatar->getID(), &av_name);

								//BD - Animesh Support
								row["columns"][0]["column"] = "name";
								row["columns"][0]["value"] = is_animesh ? animesh->getFullname() : av_name.getDisplayName();
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

					//BD - Animesh Support
					if (avatar->getID() == gAgentID || is_animesh)
					{
						//BD - Only allow animating ourselves, as per Oz Linden.
						LLScrollListItem* item = mMotionScroll->getFirstSelected();
						if (item)
						{
							LLUUID motion_id = item->getColumn(1)->getValue().asUUID();
							if (!motion_id.isNull())
							{
								//BD - Animesh Support
								//if (animesh)
								//{
									//signaled_animation_map_t signaled_anims = LLObjectSignaledAnimationMap::instance().getMap()[avatar->getID()];
									//signaled_anims[motion_id] = signaled_anims.size();
									//LLObjectSignaledAnimationMap::instance().getMap()[avatar->getID()] = signaled_anims;
									//animesh->createMotion(motion_id);
									//animesh->stopMotion(motion_id, 1);
									//animesh->startMotion(motion_id, 0.0f);
								//}
								//else
								{
									LL_INFOS("Animating") << "Adding animation to animesh" << LL_ENDL;
									avatar->createMotion(motion_id);
									avatar->stopMotion(motion_id, 1);
									avatar->startMotion(motion_id, 0.0f);
								}
							}
						}
					}
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

////////////////////////////////
//BD - Misc Functions
////////////////////////////////