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

#include "bdfloatercomplexity.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llcharacter.h"
#include "lldrawable.h"
#include "llmeshrepository.h"
#include "llselectmgr.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvoavatar.h"
#include "llviewerjointattachment.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvovolume.h"
#include "llface.h"
//BD - Animesh Support
//#include "llcontrolavatar.h"

BDFloaterComplexity::BDFloaterComplexity(const LLSD& key)
	:	LLFloater(key),
	mObjectSelection(NULL)
{
	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("ARC.AvatarRefresh", boost::bind(&BDFloaterComplexity::onAvatarsRefresh, this));
	//BD - Trigger ARC calculation.
	mCommitCallbackRegistrar.add("ARC.Refresh", boost::bind(&BDFloaterComplexity::calcARC, this));
}

BDFloaterComplexity::~BDFloaterComplexity()
{
}

BOOL BDFloaterComplexity::postBuild()
{
	//BD - Complexity
	mAvatarScroll = this->getChild<LLScrollListCtrl>("arc_avs_scroll", true);
	mAvatarScroll->setCommitOnSelectionChange(TRUE);
	mAvatarScroll->setCommitCallback(boost::bind(&BDFloaterComplexity::calcARC, this));
	mARCScroll = this->getChild<LLScrollListCtrl>("arc_scroll", true);
	mARCScroll->setCommitOnSelectionChange(TRUE);
	mARCScroll->setCommitCallback(boost::bind(&BDFloaterComplexity::onSelectEntry, this));
	mARCScroll->setDoubleClickCallback(boost::bind(&BDFloaterComplexity::onSelectAttachment, this));
	mTotalCost = getChild<LLTextBox>("arc_count");
	mTotalTriangleCount = getChild<LLTextBox>("tri_count");
	mTotalVerticeCount = getChild<LLTextBox>("vert_count");
	mTextureCost = getChild<LLTextBox>("memory_count");

	return TRUE;
}

void BDFloaterComplexity::draw()
{
	LLFloater::draw();
}

void BDFloaterComplexity::onOpen(const LLSD& key)
{
	//BD - Shameless copy from bdfloateranimations.cpp
	onAvatarsRefresh();
}

void BDFloaterComplexity::onClose(bool app_quitting)
{
	//BD - Doesn't matter because we destroy the window and rebuild it every time we open it anyway.
	mARCScroll->clearRows();
}

////////////////////////////////
//BD - Complexity
////////////////////////////////
//BD - Shameless copy from bdfloateranimations.cpp
void BDFloaterComplexity::onAvatarsRefresh()
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
		if (avatar && !avatar->isControlAvatar())
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
					if (item->getColumn(0)->getValue().asString().empty())
					{
						LLAvatarName av_name;
						LLAvatarNameCache::get(uuid, &av_name);
						item->getColumn(0)->setValue(av_name.getDisplayName());
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
				row["columns"][0]["column"] = "name";
				row["columns"][0]["value"] = av_name.getDisplayName();
				row["columns"][1]["column"] = "uuid";
				row["columns"][1]["value"] = uuid.asString();
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

void BDFloaterComplexity::calcARC()
{
	//BD - Do an avatar refresh real quick before we continue.
	onAvatarsRefresh();

	//BD - First, clear the scroll.
	mARCScroll->clearRows();

	LLVOVolume::texture_cost_t textures;
	S32Bytes texture_memory;

	U32 cost = 0;
	U32 objects = 0;
	U32 texture_count = 0;
	U32 faces = 0;
	U32 particles = 0;
	U32 lights = 0;
	U32 projectors = 0;
	U32 media = 0;
	U32 animeshs = 0;
	//BD - We need U64 here, i saw F32 exploding with the insane triangle counts some people got.
	//     Since this the absolute total count of vertices/triangles they might also go beyond
	//     4.294.967.295 and explode too hence why we use 64bits here.
	U64 vertices = 0;
	U64 triangles = 0;

	//BD - Null Check hell.
	//     This thing is extremely fragile, crashes instantly the moment something is off even
	//     a tiny bit, we need to be absolutely sure what we are doing here.
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (item)
	{
		LLVOAvatar* avatar = (LLVOAvatar*)item->getUserdata();
		if (avatar && (!avatar->isDead() || avatar->getControlAvatar()))
		{
			//BD - Make sure that we initialize any not-yet-initialized attachment points if any.
			//     Overkill #1
			avatar->initAttachmentPoints(!avatar->isSelf());

			//BD - Check if attachment points are still empty. Bail out if they are for whatever
			//     reason.
			//     Overkill #2
			LLVOAvatar::attachment_map_t attachments = avatar->mAttachmentPoints;
			if (attachments.empty())
			{
				return;
			}

			//BD - We are getting super paranoid here.
			//     Overkill #3
			if (avatar->mDrawable.isNull())
			{
				return;
			}

			//BD - We crashed here before. Checking invalid avatars does not work and does not
			//     crash. Could it be that an avatar or its attachment map could become invalid
			//     while we are still iterating through it? How would we go about fixing that?
			for (auto iter : avatar->mAttachmentPoints)
			{
				LLViewerJointAttachment* attachment = iter.second;
				if (!attachment)
				{
					continue;
				}

				for (LLViewerObject* attached_object : attachment->mAttachedObjects)
				{
					if (attached_object && !attached_object->isDead() && attached_object->mDrawable.notNull())
					{
						textures.clear();
						const LLDrawable* drawable = attached_object->mDrawable;
						if (drawable)
						{
							LLVOVolume* volume = drawable->getVOVolume();
							if (volume)
							{
								U32 attachment_final_cost = 0;
								U32 attachment_total_cost = 0;
								U32 attachment_volume_cost = 0;
								U32 attachment_texture_cost = 0;
								U32 attachment_base_cost = 0;
								U64 attachment_total_triangles = 0.f;
								U64 attachment_total_vertices = 0.f;
								S32Bytes attachment_memory_usage;

								U32 flexible_cost = 0;
								U32 faces_cost = 0;
								U32 particle_cost = 0;
								U32 light_cost = 0;
								U32 projector_cost = 0;
								U32 alpha_cost = 0;
								U32 rigged_cost = 0;
								U32 animesh_cost = 0;
								U32 media_cost = 0;
								U32 bump_cost = 0;
								U32 shiny_cost = 0;
								U32 glow_cost = 0;
								U32 animated_cost = 0;
								U32 face_count = 0;
								U32 link_count = 0;


								checkObject(volume, textures,
									attachment_volume_cost, attachment_base_cost,
									attachment_total_triangles, attachment_total_vertices);

								//BD - Get all necessary data.
								volume->getRenderCostValues(flexible_cost, particle_cost, light_cost, projector_cost,
									alpha_cost, rigged_cost, animesh_cost, media_cost, bump_cost, shiny_cost,
									glow_cost, animated_cost);

								attachment_total_cost = attachment_volume_cost;

								link_count++;
								face_count = volume->getNumFaces();

								for (LLViewerObject* child_obj : volume->getChildren())
								{
									LLVOVolume *child = dynamic_cast<LLVOVolume*>(child_obj);
									if (child)
									{
										checkObject(child, textures,
											attachment_volume_cost, attachment_base_cost,
											attachment_total_triangles, attachment_total_vertices);

										child->getRenderCostValues(flexible_cost, particle_cost, light_cost, projector_cost,
											alpha_cost, rigged_cost, animesh_cost, media_cost, bump_cost, shiny_cost,
											glow_cost, animated_cost);

										attachment_total_cost += attachment_volume_cost;
										link_count++;
										face_count += child->getNumFaces();
										if (child->isParticleSource())
											particles++;
										if (child->isAnimatedObject())
											animeshs++;
										if (child->getIsLight())
											lights++;
										if (child->getHasShadow())
											projectors++;
										//BD - This isn't accurate, it only counts objects with media, not media faces.
										if (child->hasMedia())
											media++;
									}
								}

								//BD - Count the texture impact and memory usage here now that we got all textures collected.
								for (auto volume_texture : textures)
								{
									LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(volume_texture.first);
									if (texture)
									{
										attachment_memory_usage += (texture->getTextureMemory() / 1024);
									}
									attachment_texture_cost += volume_texture.second;
									++texture_count;
								}

								//BD - Final results.
								//     Do not add HUDs to this.
								if (!attached_object->isHUDAttachment())
								{
									texture_memory += attachment_memory_usage;
									attachment_final_cost = attachment_total_cost + attachment_texture_cost;
									cost += attachment_final_cost;
									vertices += attachment_total_vertices;
									triangles += attachment_total_triangles;
									faces += face_count;
									objects += link_count;
									if (volume->isParticleSource())
										particles++;
									if (volume->isAnimatedObject())
										animeshs++;
									if (volume->getIsLight())
										lights++;
									if (volume->getHasShadow())
										projectors++;
									//BD - This isn't accurate, it only counts objects with media, not media faces.
									if (volume->hasMedia())
										media++;
								}

								//BD - Write our results into the list.
								//     Note that most of these values are actually not shown in the list, they are
								//     added regardless so we have a storage for each and every attachment that we
								//     can read all values from. This also makes cleaning up everything a lot easier.
								LLSD row;
								row["columns"][0]["column"] = "name";
								row["columns"][0]["value"] = volume->getAttachmentItemName().empty() ? volume->getAttachmentItemID().asString() : volume->getAttachmentItemName();
								row["columns"][1]["column"] = "arc";
								row["columns"][1]["value"] = LLSD::Integer(attachment_final_cost);
								row["columns"][2]["column"] = "triangles";
								row["columns"][2]["value"] = LLSD::Integer(attachment_total_triangles);
								row["columns"][3]["column"] = "vertices";
								row["columns"][3]["value"] = LLSD::Integer(attachment_total_vertices);
								row["columns"][4]["column"] = "flexible";
								row["columns"][4]["value"] = LLSD::Integer(flexible_cost);
								row["columns"][5]["column"] = "faces";
								row["columns"][5]["value"] = LLSD::Integer(faces_cost);
								row["columns"][6]["column"] = "particle";
								row["columns"][6]["value"] = LLSD::Integer(particle_cost);
								row["columns"][7]["column"] = "light";
								row["columns"][7]["value"] = LLSD::Integer(light_cost);
								row["columns"][8]["column"] = "projector";
								row["columns"][8]["value"] = LLSD::Integer(projector_cost);
								row["columns"][9]["column"] = "alpha";
								row["columns"][9]["value"] = LLSD::Integer(alpha_cost);
								row["columns"][10]["column"] = "bumpmap";
								row["columns"][10]["value"] = LLSD::Integer(bump_cost);
								row["columns"][11]["column"] = "shiny";
								row["columns"][11]["value"] = LLSD::Integer(shiny_cost);
								row["columns"][12]["column"] = "glow";
								row["columns"][12]["value"] = LLSD::Integer(glow_cost);
								row["columns"][13]["column"] = "animated";
								row["columns"][13]["value"] = LLSD::Integer(animated_cost);
								row["columns"][14]["column"] = "rigged";
								row["columns"][14]["value"] = LLSD::Integer(rigged_cost);
								row["columns"][15]["column"] = "animesh";
								row["columns"][15]["value"] = LLSD::Integer(animesh_cost);
								row["columns"][16]["column"] = "media";
								row["columns"][16]["value"] = LLSD::Integer(media_cost);
								row["columns"][17]["column"] = "base_arc";
								row["columns"][17]["value"] = LLSD::Integer(attachment_base_cost);
								row["columns"][18]["column"] = "memory";
								row["columns"][18]["value"] = attachment_memory_usage.value();
								row["columns"][19]["column"] = "uuid";
								row["columns"][19]["value"] = volume->getAttachmentItemID();
								row["columns"][20]["column"] = "memory_arc";
								row["columns"][20]["value"] = LLSD::Integer(attachment_texture_cost);
								row["columns"][21]["column"] = "total_arc";
								row["columns"][21]["value"] = LLSD::Integer(attachment_total_cost);
								row["columns"][22]["column"] = "links";
								row["columns"][22]["value"] = LLSD::Integer(link_count);
								row["columns"][23]["column"] = "faces";
								row["columns"][23]["value"] = LLSD::Integer(face_count);
								LLScrollListItem* element = mARCScroll->addElement(row);
								element->setUserdata(attached_object);
							}
						}
					}
				}
			}
		}
	}

	//BD - Show our final total counts at the bottom.
	mTotalVerticeCount->setValue(LLSD::Integer(vertices));
	mTotalTriangleCount->setValue(LLSD::Integer(triangles));
	mTotalCost->setValue(LLSD::Integer(cost));
	mTextureCost->setValue(texture_memory.value());

	
	//BD - Fill in the numbers for all performance categories.
	getChild<LLUICtrl>("complexity_count")->setTextArg("[COMPLEXITY_COUNT]", llformat("%d", LLSD::Integer(cost)));
	getChild<LLUICtrl>("poly_count")->setTextArg("[TRI_COUNT]", llformat("%d", LLSD::Integer(triangles)));
	getChild<LLUICtrl>("texture_mem_count")->setTextArg("[TEX_MEM_COUNT]", llformat("%.1f", F32(texture_memory.value()) / 1024.f));
	getChild<LLUICtrl>("tex_count")->setTextArg("[TEX_COUNT]", llformat("%.d", texture_count));
	getChild<LLUICtrl>("object_count")->setTextArg("[OBJ_COUNT]", llformat("%d", objects));
	getChild<LLUICtrl>("face_count")->setTextArg("[FACE_COUNT]", llformat("%d", faces));
	getChild<LLUICtrl>("particle_count")->setTextArg("[PART_COUNT]", llformat("%d", particles));
	getChild<LLUICtrl>("light_count")->setTextArg("[LIGHT_COUNT]", llformat("%d", lights));
	getChild<LLUICtrl>("projector_count")->setTextArg("[PROJ_COUNT]", llformat("%d", projectors));
	getChild<LLUICtrl>("media_count")->setTextArg("[MEDIA_COUNT]", llformat("%d", media));
	getChild<LLUICtrl>("animesh_count")->setTextArg("[ANIMESH_COUNT]", llformat("%d", animeshs));

	//BD - Now color the report depending on their estimated "goodness"
	F32 red = 1.0f;
	F32 final_red = 0.0f;

	red = 1.0f * (F32(cost) / 300000.f);
	final_red += red;
	rateAvatarGroup("complexity", red);

	red = 1.0f * (F32(triangles) / 160000.f);
	final_red += red;
	rateAvatarGroup("poly", red);

	red = 1.0f * (F32(texture_memory.value()) / (80.f * 1024.f));
	final_red += red;
	rateAvatarGroup("texture_mem", red);

	red = 1.0f * (F32(texture_count) / 64.f);
	final_red += red;
	rateAvatarGroup("tex", red);

	red = 1.0f * (F32(objects) / 80.f);
	final_red += red;
	rateAvatarGroup("object", red);

	red = 1.0f * (F32(faces) / 320.f);
	final_red += red;
	rateAvatarGroup("face", red);

	red = 1.0f * F32(particles);
	final_red += red;
	rateAvatarGroup("particle", red);

	red = 1.0f * (F32(lights) / 32.f);
	final_red += red;
	rateAvatarGroup("light", red);

	red = 1.0f * F32(projectors);
	final_red += red;
	rateAvatarGroup("projector", red);

	red = 1.0f * F32(media);
	final_red += red;
	rateAvatarGroup("media", red);

	red = 1.0f * F32(animeshs);
	final_red += red;
	rateAvatarGroup("animesh", red);

	final_red /= 11.f;
	getChild<LLUICtrl>("final_rating")->setColor(LLColor3(final_red, 1.0f - final_red, 0.0f));
	std::string str;
	//BD - 90%+ is perfect
	if (final_red <= 0.1f)
		str = getString("perfect");
	//BD - 80% - 90% is still very good
	else if (final_red <= 0.2f)
		str = getString("verygood");
	//BD - 65% - 80% is still good
	else if (final_red <= 0.35f)
		str = getString("good");
	//BD - 35% - 65% is okay
	else if (final_red <= 0.65f)
		str = getString("ok");
	//BD - 20% - 35% is bad
	else if (final_red <= 0.8f)
		str = getString("bad");
	//BD - less than 20% is very bad
	else
		str = getString("verybad");

	getChild<LLUICtrl>("final_verdict")->setTextArg("[VERDICT]", llformat("%s", str.c_str()));

}

void BDFloaterComplexity::rateAvatarGroup(std::string type, F32 rating)
{
	getChild<LLUICtrl>(type + "_rating")->setColor(LLColor3(rating, 1.0f - rating, 0.0f));
	LLUICtrl* ctrl = getChild<LLUICtrl>(type + "_short");
	ctrl->setValue(getString("short"));
	
	//BD - Fill in the performance group we are about to rate.
	std::string str = getString("s_" + type);
	ctrl->setTextArg("[GROUP]", str);

	//BD - Now figure out and fill in the rating of the above group.
	//     0% - 33% is good
	if (rating <= 0.33f)
		str = getString("s_0");
	//BD - 34% - 50% is okay
	else if (rating <= 0.5f)
		str = getString("s_1");
	//BD - 51% - 100% is bad
	else
		str = getString("s_2");
	ctrl->setTextArg("[STATUS]", str);
}

//BD - This function calculates any input object and spits out everything we need to know about it.
//     We do this here so we don't have to have it twice above.
void BDFloaterComplexity::checkObject(LLVOVolume* vovolume, LLVOVolume::texture_cost_t &textures,
									  U32 &volume_cost, U32 &base_cost, U64 &total_triangles, U64 &total_vertices)
{
	//BD - Check all the easy costs and counts first.
	volume_cost = vovolume->getRenderCost(textures);
	base_cost += vovolume->mRenderComplexityBase;
	total_triangles += vovolume->getHighLODTriangleCount64();
	total_vertices += vovolume->getNumVertices();
}

void BDFloaterComplexity::onSelectEntry()
{
	LLScrollListItem* item = mARCScroll->getFirstSelected();
	if (item)
	{
		//BD - Lets write up all the easy information first.
		getChild<LLUICtrl>("label_final_arc")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("label_polygons")->setValue(item->getColumn(2)->getValue());
		getChild<LLUICtrl>("label_vertices")->setValue(item->getColumn(3)->getValue());
		getChild<LLUICtrl>("label_vram")->setValue(item->getColumn(18)->getValue());

		//BD - Write down all ARC values.
		getChild<LLUICtrl>("label_flexi")->setValue(item->getColumn(4)->getValue());
		getChild<LLUICtrl>("label_faces")->setValue(item->getColumn(5)->getValue());
		getChild<LLUICtrl>("label_particles")->setValue(item->getColumn(6)->getValue());
		getChild<LLUICtrl>("label_light")->setValue(item->getColumn(7)->getValue());
		getChild<LLUICtrl>("label_projector")->setValue(item->getColumn(8)->getValue());
		getChild<LLUICtrl>("label_alpha")->setValue(item->getColumn(9)->getValue());
		getChild<LLUICtrl>("label_bump")->setValue(item->getColumn(10)->getValue());
		getChild<LLUICtrl>("label_shiny")->setValue(item->getColumn(11)->getValue());
		getChild<LLUICtrl>("label_glow")->setValue(item->getColumn(12)->getValue());
		getChild<LLUICtrl>("label_animated")->setValue(item->getColumn(13)->getValue());
		getChild<LLUICtrl>("label_rigged")->setValue(item->getColumn(14)->getValue());
		getChild<LLUICtrl>("label_animesh")->setValue(item->getColumn(15)->getValue());
		getChild<LLUICtrl>("label_media")->setValue(item->getColumn(16)->getValue());
		getChild<LLUICtrl>("label_base_arc")->setValue(item->getColumn(17)->getValue());
		getChild<LLUICtrl>("label_uuid")->setValue(item->getColumn(19)->getValue());
		getChild<LLUICtrl>("label_texture_arc")->setValue(item->getColumn(20)->getValue());
		getChild<LLUICtrl>("label_total_arc")->setValue(item->getColumn(21)->getValue());
	}
}

void BDFloaterComplexity::onSelectAttachment()
{
	for (LLScrollListItem* item : mARCScroll->getAllSelected())
	{
		if (item)
		{
			LLViewerObject* vobject = (LLViewerObject*)item->getUserdata();
			if (vobject && !vobject->isDead())
			{
				LLSelectMgr::instance().deselectAll();
				mObjectSelection = LLSelectMgr::instance().selectObjectAndFamily(vobject, FALSE, TRUE);

				// Mark this as a transient selection
				struct SetTransient : public LLSelectedNodeFunctor
				{
					bool apply(LLSelectNode* node)
					{
						node->setTransient(TRUE);
						return true;
					}
				} functor;
				mObjectSelection->applyToNodes(&functor);
			}
		}
	}
}