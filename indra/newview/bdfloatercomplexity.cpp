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
#include "llface.h"
#include "llmeshrepository.h"
#include "llprimitive.h"
#include "llpartdata.h"
#include "llselectmgr.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvoavatar.h"
#include "llviewerjointattachment.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewerpartsource.h"
#include "llvolumemgr.h"
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
								getRenderCostValues(volume, flexible_cost, particle_cost, light_cost, projector_cost,
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

										getRenderCostValues(child, flexible_cost, particle_cost, light_cost, projector_cost,
											alpha_cost, rigged_cost, animesh_cost, media_cost, bump_cost, shiny_cost,
											glow_cost, animated_cost);

										attachment_total_cost += attachment_volume_cost;
										link_count++;
										face_count += child->getNumFaces();
										if (child->isParticleSource())
											particles++;
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
								/*for (auto volume_texture : textures)
								{
									LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(volume_texture);
									if (texture)
									{
										attachment_memory_usage += (texture->getTextureMemory() / 1024);
									}
									//attachment_texture_cost += volume_texture.second;
									++texture_count;
								}*/

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
	total_triangles += getHighLODTriangleCount64(vovolume);
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

void BDFloaterComplexity::getRenderCostValues(LLVOVolume* volume, U32& flexible_cost, U32& particle_cost, U32& light_cost, U32& projector_cost,
	U32& alpha_cost, U32& rigged_cost, U32& animesh_cost, U32& media_cost, U32& bump_cost,
	U32& shiny_cost, U32& glow_cost, U32& animated_cost) const
{
	/*****************************************************************
	* This calculation should not be modified by third party viewers,
	* since it is used to limit rendering and should be uniform for
	* everyone. If you have suggested improvements, submit them to
	* the official viewer for consideration.
	*****************************************************************/

	// Get access to params we'll need at various points.  
	// Skip if this is object doesn't have a volume (e.g. is an avatar).
	BOOL has_volume = (volume != NULL);
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;

	U32 num_triangles = 0;

	//BD - Experimental new ARC
	// per-prim costs
	//BD - Particles need to be punished extremely harsh, they are besides all other features, the single biggest
	//     performance hog in Second Life. Just having them enabled and a tiny bunch around drops the framerate
	//     noticeably.
	static const U32 ARC_PARTICLE_COST = 8; //16
	//BD - Lights are an itchy thing. They don't have any impact if used carefully. They do however have an
	//     increasingly bigger impact above a certain threshold at which they will significantly drop your average
	//     FPS. We should punish them slightly but not too hard otherwise Avatars with a few lights get overpunished.
	static const U32 ARC_LIGHT_COST = 256; //512
	//BD - Projectors have a huge impact, whether or not they cast a shadow or not, multiple of these will make quick
	//     work of any good framerate.
	static const U32 ARC_PROJECTOR_COST = 8192; //16384
	//BD - Media faces have a huge impact on performance, they should never ever be attached and should be used
	//     carefully. Punish them with extreme measure, besides, by default we can only have 6-8 active at any time
	//     those alone will significantly draw resources both RAM and FPS.
	static const U32 ARC_MEDIA_FACE_COST = 50000; //100000 - static cost per media-enabled face 

	// per-prim multipliers
	//BD - Glow has nearly no impact, the impact is already there due to the omnipresent ambient glow Black Dragon
	//     uses, putting up hundreds of glowing prims does nothing, it's a global post processing effect.
	static const F32 ARC_GLOW_MULT = 0.05f;
	//BD - Bump has nearly no impact, it's biggest impact is texture memory which we really shouldn't be including.
	static const F32 ARC_BUMP_MULT = 0.05f;
	//BD - I'm unsure about flexi, on one side its very efficient but if huge amounts of flexi are active at the same
	//     time they can quickly become extremely slow which is hardly ever the case.
	static const F32 ARC_FLEXI_MULT = 0.15f;
	//BD - Shiny has nearly no impact, it's basically a global post process effect.
	static const F32 ARC_SHINY_MULT = 0.05f;
	//BD - Invisible prims are not rendered anymore in Black Dragon.
	//static const F32 ARC_INVISI_COST = 1.0f;
	//BD - Weighted mesh does have quite some impact and it only gets worse with more triangles to transform.
	static const F32 ARC_WEIGHTED_MESH = 2.0f; //4.0

	//BD - Animated textures hit quite hard, not as hard as quick alpha state changes.
	static const F32 ARC_ANIM_TEX_COST = 1.f;
	//BD - Alpha's are bad.
	static const F32 ARC_ALPHA_COST = 1.0f;
	//BD - Alpha's aren't that bad as normal alphas if they are rigged and worn, static ones are evil.
	//     Besides, as long as they are fully invisible Black Dragon won't render them anyway.
	static const F32 ARC_RIGGED_ALPHA_COST = 0.25f;
	//BD - In theory animated mesh are pretty limited and they are rendering wise not different to normal avatars.
	//     Thus they should not be weighted differently, however, since they are just basic dummy avatars with no
	//     super extensive information, relations, name tag and so on they deserve a tiny complexity discount.
	static const F32 ARC_ANIMATED_MESH_COST = -0.05f;

	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 animtex = 0;
	U32 bump = 0;
	U32 weighted_mesh = 0;
	U32 media_faces = 0;

	LLDrawable* drawablep = volume->mDrawable;
	U32 num_faces = drawablep->getNumFaces();

	if (has_volume)
	{
		volume_params = volume->getVolume()->getParams();
		num_triangles = drawablep->getVOVolume()->getHighLODTriangleCount();
	}

	if (num_triangles == 0)
	{
		num_triangles = 4;
	}

	if (volume->isSculpted())
	{
		if (volume->isMesh())
		{
			S32 size = gMeshRepo.getMeshSize(volume_params.getSculptID(), volume->getLOD());
			if (size > 0)
			{
				if (volume->getSkinInfo())
				{
					weighted_mesh = 1;
				}
			}
		}
	}

	for (S32 i = 0; i < num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;
		const LLTextureEntry* te = face->getTextureEntry();

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			alpha = 1;
		}

		if (face->hasMedia())
		{
			media_faces++;
		}

		if (te)
		{
			if (te->getBumpmap())
			{
				bump = 1;
			}
			if (te->getShiny())
			{
				shiny = 1;
			}
			if (te->getGlow() > 0.f)
			{
				glow = 1;
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex = 1;
			}
		}
	}

	//BD - shame currently has the "base" cost of 1 point per 5 triangles, min 2.
	U32 shame = volume->mRenderComplexityBase;

	if (animtex)
	{
		animated_cost += (shame * ARC_ANIM_TEX_COST);
	}

	if (glow)
	{
		glow_cost += (shame * ARC_GLOW_MULT);
	}

	if (bump)
	{
		bump_cost += (shame * ARC_BUMP_MULT);
	}

	if (shiny)
	{
		shiny_cost += (shame * ARC_SHINY_MULT);
	}

	if (weighted_mesh)
	{
		rigged_cost += (shame * ARC_WEIGHTED_MESH);

		if (alpha)
		{
			alpha_cost += (shame * ARC_ALPHA_COST);
		}
	}
	else
	{
		if (alpha)
		{
			alpha_cost += (shame * ARC_RIGGED_ALPHA_COST);
		}
	}

	if (volume->isAnimatedObject())
	{
		animesh_cost += (shame * ARC_ANIMATED_MESH_COST);
	}

	// multiply shame by multipliers
	if (volume->isFlexible())
	{
		flexible_cost += (shame * ARC_FLEXI_MULT);
	}

	// add additional costs
	if (volume->isParticleSource())
	{
		const LLPartSysData* part_sys_data = &(drawablep->getVObj()->getParticleSource()->mPartSysData);
		const LLPartData* part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil(part_data->mMaxAge / part_sys_data->mBurstRate));
		//BD
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;
		particle_cost += num_particles * part_size * ARC_PARTICLE_COST;
	}

	if (volume->getIsLight())
	{
		light_cost += ARC_LIGHT_COST;
	}
	else if (volume->getHasShadow())
	{
		projector_cost += ARC_PROJECTOR_COST;
	}

	if (media_faces)
	{
		media_cost += media_faces * ARC_MEDIA_FACE_COST;
	}
}

//BD - Altered Complexity Calculation
// Returns a base cost and adds textures to passed in set.
// total cost is returned value + 5 * size of the resulting set.
// Cannot include cost of textures, as they may be re-used in linked
// children, and cost should only be increased for unique textures  -Nyx
U32 BDFloaterComplexity::getRenderCost(LLVOVolume* volume, texture_cost& textures) const
{
	/*****************************************************************
	 * This calculation should not be modified by third party viewers,
	 * since it is used to limit rendering and should be uniform for
	 * everyone. If you have suggested improvements, submit them to
	 * the official viewer for consideration.
	 *****************************************************************/

	 // Get access to params we'll need at various points.  
	 // Skip if this is object doesn't have a volume (e.g. is an avatar).
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;

	U32 num_triangles = 0;

	//BD - Experimental new ARC
	// per-prim costs
	//BD - Particles need to be punished extremely harsh, they are besides all other features, the single biggest
	//     performance hog in Second Life. Just having them enabled and a tiny bunch around drops the framerate
	//     noticeably.
	static const U32 ARC_PARTICLE_COST = 4; //16
	//BD - Textures don't directly influence performance impact on a large scale but allocating a lot of textures
	//     and filling the Viewer memory as well as texture memory grinds at the Viewer's overall performance, the
	//     lost performance does not fully recover when leaving the area in question, textures overall have a lingering
	//     performance impact that slowly drives down the Viewer's performance, we should punish them much harder.
	//     Textures are not free after all and not everyone can have 2+GB texture memory for SL.
	static const U32 ARC_TEXTURE_COST = 1.25; //5
	//BD - Lights are an itchy thing. They don't have any impact if used carefully. They do however have an
	//     increasingly bigger impact above a certain threshold at which they will significantly drop your average
	//     FPS. We should punish them slightly but not too hard otherwise Avatars with a few lights get overpunished.
	static const U32 ARC_LIGHT_COST = 128; //512
	//BD - Projectors have a huge impact, whether or not they cast a shadow or not, multiple of these will make quick
	//     work of any good framerate.
	static const U32 ARC_PROJECTOR_COST = 4096; //16384
	//BD - Media faces have a huge impact on performance, they should never ever be attached and should be used
	//     carefully. Punish them with extreme measure, besides, by default we can only have 6-8 active at any time
	//     those alone will significantly draw resources both RAM and FPS.
	static const U32 ARC_MEDIA_FACE_COST = 25000; //100000 - static cost per media-enabled face 

	// per-prim multipliers
	//BD - Glow has nearly no impact, the impact is already there due to the omnipresent ambient glow Black Dragon
	//     uses, putting up hundreds of glowing prims does nothing, it's a global post processing effect.
	static const F32 ARC_GLOW_MULT = 0.05f;
	//BD - Bump has nearly no impact, it's biggest impact is texture memory which we really shouldn't be including.
	static const F32 ARC_BUMP_MULT = 0.05f;
	//BD - I'm unsure about flexi, on one side its very efficient but if huge amounts of flexi are active at the same
	//     time they can quickly become extremely slow which is hardly ever the case.
	static const F32 ARC_FLEXI_MULT = 0.15f;
	//BD - Shiny has nearly no impact, it's basically a global post process effect.
	static const F32 ARC_SHINY_MULT = 0.05f;
	//BD - Invisible prims are not rendered anymore in Black Dragon.
	//static const F32 ARC_INVISI_COST = 1.0f;
	//BD - Weighted mesh does have quite some impact and it only gets worse with more triangles to transform.
	static const F32 ARC_WEIGHTED_MESH = 2.0f; //4.0

	//BD - Animated textures hit quite hard, not as hard as quick alpha state changes.
	static const F32 ARC_ANIM_TEX_COST = 1.f;
	//BD - Alpha's are bad.
	static const F32 ARC_ALPHA_COST = 1.0f;
	//BD - Alpha's aren't that bad as normal alphas if they are rigged and worn, static ones are evil.
	//     Besides, as long as they are fully invisible Black Dragon won't render them anyway.
	static const F32 ARC_RIGGED_ALPHA_COST = 0.25f;
	//BD - In theory animated mesh are pretty limited and they are rendering wise not different to normal avatars.
	//     Thus they should not be weighted differently, however, since they are just basic dummy avatars with no
	//     super extensive information, relations, name tag and so on they deserve a tiny complexity discount.
	static const F32 ARC_ANIMATED_MESH_COST = -0.05f;

	F32 shame = 0;

	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 animtex = 0;
	U32 bump = 0;
	U32 weighted_mesh = 0;
	U32 animated_mesh = 0;
	U32 media_faces = 0;

	LLDrawable* drawablep = volume->mDrawable;
	U32 num_faces = drawablep->getNumFaces();

	if (volume->isMeshFast() && volume->getVolume())
	{
		volume_params = volume->getVolume()->getParams();
		path_params = volume_params.getPathParams();
		profile_params = volume_params.getProfileParams();

		//BD - Punish high triangle counts.
		num_triangles = drawablep->getVOVolume()->getHighLODTriangleCount();
	}

	if (num_triangles <= 0)
	{
		num_triangles = 4;
	}

	if (volume->isSculpted())
	{
		if (volume->isMesh())
		{
			// base cost is dependent on mesh complexity
			// note that 3 is the highest LOD as of the time of this coding.
			S32 size = gMeshRepo.getMeshSize(volume_params.getSculptID(), volume->getLOD());
			if (size > 0)
			{
				if (volume->isRiggedMesh())
				{
					// weighted attachment - 1 point for every 3 bytes
					weighted_mesh = 1;
				}
			}
			else
			{
				// something went wrong - user should know their content isn't render-free
				return 0;
			}

			if (volume->isAnimatedObject())
			{
				animated_mesh = 1;
			}
		}
		else
		{
			const LLSculptParams* sculpt_params = (LLSculptParams*)drawablep->getVObj()->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			LLUUID sculpt_id = sculpt_params->getSculptTexture();
			LLViewerFetchedTexture* texture = LLViewerTextureManager::getFetchedTexture(sculpt_id);
			if (textures.find(texture) == textures.end())
			{
				if (texture)
				{
					//BD - Punish sculpt usage compared to normal prims or the much faster mesh.
					//S32 texture_cost = 256 + (S32)(((ARC_TEXTURE_COST * 2) * (texture->getFullHeight() * texture->getFullWidth())) / 1024);
					//textures.insert(texture_cost::value_type(texture));
					//vovolume->mRenderComplexityTextures += texture_cost;
				}
			}
		}
	}

	for (S32 i = 0; i < num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;
		const LLTextureEntry* te = face->getTextureEntry();

		S32 j = 0;
		while (j < LLRender::NUM_TEXTURE_CHANNELS)
		{
			const LLViewerTexture* img = face->getTexture(j);
			if (img)
			{
				if (textures.find(img) == textures.end())
				{
					S32 texture_cost = 256 + (S32)((ARC_TEXTURE_COST * (img->getFullHeight() * img->getFullWidth())) / 1024);
					//textures.insert(texture_cost::value_type(img));
					volume->mRenderComplexityTextures += texture_cost;
				}
			}
			++j;
		}

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			alpha = 1;
		}

		if (face->hasMedia())
		{
			media_faces++;
		}

		if (te)
		{
			if (te->getBumpmap())
			{
				// bump is a multiplier, don't add per-face
				bump = 1;
			}
			if (te->getShiny())
			{
				// shiny is a multiplier, don't add per-face
				shiny = 1;
				//BD
				volume->setHasShiny(true);
			}
			if (te->getGlow() > 0.f)
			{
				// glow is a multiplier, don't add per-face
				glow = 1;
				//BD
				volume->setHasGlow(true);
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex = 1;
				//BD
				volume->setIsAnimated(true);
			}
		}
	}

	//BD - shame currently has the "base" cost of 1 point per 5 triangles, min 2.
	shame = num_triangles / 10; //5
	shame = shame < 2.f ? 2.f : shame;

	volume->setRenderComplexityBase((S32)shame);
	F32 extra_shame = 0.f;
	if (animtex)
	{
		extra_shame += (shame * ARC_ANIM_TEX_COST);
	}

	if (glow)
	{
		extra_shame += (shame * ARC_GLOW_MULT);
	}

	if (bump)
	{
		extra_shame += (shame * ARC_BUMP_MULT);
	}

	if (shiny)
	{
		extra_shame += (shame * ARC_SHINY_MULT);
	}

	if (weighted_mesh)
	{
		extra_shame += (shame * ARC_WEIGHTED_MESH);

		if (alpha)
		{
			extra_shame += (shame * ARC_ALPHA_COST);
		}
	}
	else
	{
		if (alpha)
		{
			extra_shame += (shame * ARC_RIGGED_ALPHA_COST);
		}
	}

	if (animated_mesh)
	{
		extra_shame += (shame * ARC_ANIMATED_MESH_COST);
	}

	// multiply shame by multipliers
	if (volume->isFlexible())
	{
		extra_shame += (shame * ARC_FLEXI_MULT);
	}

	// Streaming cost for animated objects includes a fixed cost
	// per linkset. Add a corresponding charge here translated into
	// triangles, but not weighted by any graphics properties.
	/*if (isAnimatedObject() && isRootEdit())
	{
		shame += (ANIMATED_OBJECT_BASE_COST / 0.06) * 5.0f;
	}*/

	// add additional costs
	if (volume->isParticleSource())
	{
		const LLPartSysData* part_sys_data = &(drawablep->getVObj()->getParticleSource()->mPartSysData);
		const LLPartData* part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil(part_data->mMaxAge / part_sys_data->mBurstRate));
		//BD
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;
		shame += num_particles * part_size * ARC_PARTICLE_COST;
	}

	shame += extra_shame;

	if (volume->getIsLight())
	{
		shame += ARC_LIGHT_COST;
	}

	if (volume->getHasShadow())
	{
		shame += ARC_PROJECTOR_COST;
	}

	if (media_faces)
	{
		shame += (media_faces * ARC_MEDIA_FACE_COST);
	}

	/*vovolume->mRenderComplexityTotal = (S32)shame;

	if (shame > mRenderComplexity_current)
	{
		mRenderComplexity_current = (S32)shame;
	}*/

	return (U32)shame;
}

U64 BDFloaterComplexity::getHighLODTriangleCount64(LLVOVolume* volume)
{
	U64 ret = 0;
	LLVolume* vol = volume->getVolume();

	if (!volume->isSculpted())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(vol->getParams(), 3);
		ret = ref->getNumTriangles64();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else if (volume->isMesh())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(vol->getParams(), 3);
		if (!ref->isMeshAssetLoaded() || ref->getNumVolumeFaces() == 0)
		{
			gMeshRepo.loadMesh(volume, vol->getParams(), LLModel::LOD_HIGH);
		}
		ret = ref->getNumTriangles64();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else
	{ //default sculpts have a constant number of triangles
		ret = 31 * 2 * 31;  //31 rows of 31 columns of quads for a 32x32 vertex patch
	}

	return ret;
}