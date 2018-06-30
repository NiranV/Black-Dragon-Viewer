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
	mAvatarScroll->setDoubleClickCallback(boost::bind(&BDFloaterComplexity::calcARC, this));
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

	bool create_new;
	for (LLCharacter* character : LLCharacter::sInstances)
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(character);
		if (avatar)
		{
			LLUUID uuid = avatar->getID();
			create_new = true;
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
	//BD - First, clear the scroll.
	mARCScroll->clearRows();

	LLVOVolume::texture_cost_t textures;
	S32Bytes texture_memory;

	U32 cost = 0;
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
		if (avatar && !avatar->isDead())
		{
			for (auto iter : avatar->mAttachmentPoints)
			{
				LLViewerJointAttachment* attachment = iter.second;
				if (!attachment)
				{
					continue;
				}

				for (LLViewerObject* attached_object : attachment->mAttachedObjects)
				{
					if (attached_object && !attached_object->isHUDAttachment() && attached_object->mDrawable.notNull())
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

								S32 flexibles = 0;
								S32 lights = 0;
								S32 projectors = 0;
								S32 alphas = 0;
								S32 rigged = 0;
								S32 medias = 0;

								bool is_flexible = false;
								bool has_particles = false;
								bool is_light = false;
								bool is_projector = false;
								bool is_alpha = false;
								bool has_bump = false;
								bool has_shiny = false;
								bool has_glow = false;
								bool is_animated = false;
								bool is_rigged = false;
								bool has_media = false;

								checkObject(volume, textures, is_flexible, has_particles, is_light, is_projector,
									is_alpha, has_bump, has_shiny, has_glow, is_animated, is_rigged, has_media,
									flexibles, lights, projectors, alphas, rigged, medias,
									attachment_volume_cost, attachment_total_cost, attachment_base_cost,
									attachment_total_triangles, attachment_total_vertices);

								for (LLViewerObject* child_obj : volume->getChildren())
								{
									LLVOVolume *child = dynamic_cast<LLVOVolume*>(child_obj);
									if (child)
									{
										checkObject(child, textures, is_flexible, has_particles, is_light, is_projector,
											is_alpha, has_bump, has_shiny, has_glow, is_animated, is_rigged, has_media,
											flexibles, lights, projectors, alphas, rigged, medias,
											attachment_volume_cost, attachment_total_cost, attachment_base_cost,
											attachment_total_triangles, attachment_total_vertices);
									}
								}

								//BD - Count the texture impact and memory usage here now that we got all textures colelcted.
								for (auto volume_texture : textures)
								{
									LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(volume_texture.first);
									if (texture)
									{
										attachment_memory_usage += (texture->getTextureMemory() / 1024);
									}
									attachment_texture_cost += volume_texture.second;
								}

								//BD - Final results.
								texture_memory += attachment_memory_usage;
								attachment_final_cost = attachment_volume_cost + attachment_texture_cost;
								cost += attachment_final_cost;
								vertices += attachment_total_vertices;
								triangles += attachment_total_triangles;

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
								row["columns"][4]["value"] = is_flexible;
								row["columns"][5]["column"] = "particle";
								row["columns"][5]["value"] = has_particles;
								row["columns"][6]["column"] = "light";
								row["columns"][6]["value"] = is_light;
								row["columns"][7]["column"] = "projector";
								row["columns"][7]["value"] = is_projector;
								row["columns"][8]["column"] = "alpha";
								row["columns"][8]["value"] = is_alpha;
								row["columns"][9]["column"] = "bumpmap";
								row["columns"][9]["value"] = has_bump;
								row["columns"][10]["column"] = "shiny";
								row["columns"][10]["value"] = has_shiny;
								row["columns"][11]["column"] = "glow";
								row["columns"][11]["value"] = has_glow;
								row["columns"][12]["column"] = "animated";
								row["columns"][12]["value"] = is_animated;
								row["columns"][13]["column"] = "rigged";
								row["columns"][13]["value"] = is_rigged;
								row["columns"][14]["column"] = "media";
								row["columns"][14]["value"] = has_media;
								row["columns"][15]["column"] = "base_arc";
								row["columns"][15]["value"] = LLSD::Integer(attachment_base_cost);
								row["columns"][16]["column"] = "memory";
								row["columns"][16]["value"] = attachment_memory_usage.value();
								row["columns"][17]["column"] = "uuid";
								row["columns"][17]["value"] = volume->getAttachmentItemID();
								row["columns"][18]["column"] = "lights";
								row["columns"][18]["value"] = lights;
								row["columns"][19]["column"] = "projectors";
								row["columns"][19]["value"] = projectors;
								row["columns"][20]["column"] = "alphas";
								row["columns"][20]["value"] = alphas;
								row["columns"][21]["column"] = "rigs";
								row["columns"][21]["value"] = rigged;
								row["columns"][22]["column"] = "flexis";
								row["columns"][22]["value"] = flexibles;
								row["columns"][23]["column"] = "medias";
								row["columns"][23]["value"] = medias;
								row["columns"][24]["column"] = "memory_arc";
								row["columns"][24]["value"] = LLSD::Integer(attachment_texture_cost);
								row["columns"][25]["column"] = "total_arc";
								row["columns"][25]["value"] = LLSD::Integer(attachment_total_cost);
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
}

//BD - This function calculates any input object and spits out everything we need to know about it.
//     We do this here so we don't have to have it twice above.
bool BDFloaterComplexity::checkObject(LLVOVolume* vovolume, LLVOVolume::texture_cost_t &textures,
									  bool &is_flexible, bool &has_particles, bool &is_light, bool &is_projector, bool &is_alpha,
									  bool &has_bump, bool &has_shiny, bool &has_glow, bool &is_animated, bool &is_rigged, bool &has_media,
									  S32 &flexibles, /*S32 particles,*/ S32 &lights, S32 &projectors, S32 &alphas, S32 &rigged, S32 &medias,
									  U32 &volume_cost, U32 &total_cost, U32 &base_cost, U64 &total_triangles, U64 &total_vertices)
{
	//BD - Check all the easy costs and counts first.
	volume_cost += vovolume->getRenderCost(textures);
	total_cost += vovolume->mRenderComplexityTotal;
	base_cost += vovolume->mRenderComplexityBase;
	total_triangles += vovolume->getHighLODTriangleCount64();
	total_vertices += vovolume->getNumVertices();

	//BD - Check each object and count each feature being used.
	//     TODO: Remove all booleans, we can just check the count if we need to,
	//     though booleans end up nicely as "true" or "false" strings, which would
	//     need additional effort if we don't do it with booleans. Ugh.
	if (vovolume->getIsLight())
	{
		lights++;
		is_light = true;
	}
	if (vovolume->isLightSpotlight())
	{
		projectors++;
		is_projector = true;
	}
	if (vovolume->isFlexible())
	{
		flexibles++;
		is_flexible = true;
	}
	if (!is_rigged)
	{
		LLVolumeParams volume_params;
		volume_params = vovolume->getVolume()->getParams();
		if (gMeshRepo.getSkinInfo(volume_params.getSculptID(), vovolume))
		{
			is_rigged = true;
			rigged++;
		}
	}
	if (vovolume->isParticleSource())
	{
		has_particles = true;
	}
	if (vovolume->mIsAnimated)
	{
		is_animated = true;
	}

	//BD - We iterate through faces here because below checks need to be checked
	//     on each surface and are no global object specific features, they are face
	//     specific.
	for (S32 i = 0; (i < vovolume->getNumFaces()); i++)
	{
		LLTextureEntry* te = vovolume->getTE(i);
		if (te)
		{
			if (!has_glow
				&& te->getGlow())
			{
				has_glow = true;
			}

			LLMaterialPtr mat = te->getMaterialParams();
			if (mat)
			{
				if (!has_bump
					&& (te->getBumpmap()
					|| mat->getNormalID().notNull()))
				{
					has_bump = true;
				}

				if (!has_shiny
					&& (te->getShiny()
					|| mat->getSpecularID().notNull()))
				{
					has_shiny = true;
				}

				if (te->getColor().mV[VW] < 1.f
					|| mat->getDiffuseAlphaMode() == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_BLEND)
				{
					is_alpha = true;
					alphas++;
				}
			}

			if (te->hasMedia())
			{
				has_media = true;
				medias++;
			}
		}
	}
	return true;
}

void BDFloaterComplexity::onSelectEntry()
{
	LLScrollListItem* item = mARCScroll->getFirstSelected();
	if (item)
	{
		//BD - Lets write up all the easy information first.
		//getChild<LLUICtrl>("label_name")->setValue(item->getColumn(0)->getValue());
		getChild<LLUICtrl>("label_total_arc")->setValue(item->getColumn(25)->getValue());
		getChild<LLUICtrl>("label_final_arc")->setValue(item->getColumn(1)->getValue());
		getChild<LLUICtrl>("label_polygons")->setValue(item->getColumn(2)->getValue());
		getChild<LLUICtrl>("label_vertices")->setValue(item->getColumn(3)->getValue());
		getChild<LLUICtrl>("label_base_arc")->setValue(item->getColumn(15)->getValue());
		getChild<LLUICtrl>("label_texture_memory")->setValue(item->getColumn(16)->getValue());
		getChild<LLUICtrl>("label_texture_arc")->setValue(item->getColumn(24)->getValue());
		getChild<LLUICtrl>("label_uuid")->setValue(item->getColumn(17)->getValue());

		//BD - Now lets show all multiplicators of features enabled.
		getChild<LLUICtrl>("panel_bump")->setVisible(item->getColumn(9)->getValue().asBoolean());
		getChild<LLUICtrl>("label_bump")->setValue(item->getColumn(9)->getValue());
		getChild<LLUICtrl>("panel_shiny")->setVisible(item->getColumn(10)->getValue().asBoolean());
		getChild<LLUICtrl>("label_shiny")->setValue(item->getColumn(10)->getValue());
		getChild<LLUICtrl>("panel_glow")->setVisible(item->getColumn(11)->getValue().asBoolean());
		getChild<LLUICtrl>("label_glow")->setValue(item->getColumn(11)->getValue());
		getChild<LLUICtrl>("panel_animated")->setVisible(item->getColumn(12)->getValue().asBoolean());
		getChild<LLUICtrl>("label_animated")->setValue(item->getColumn(12)->getValue());
		
		//BD - Now show the amount of objects that use a certain feature.
		getChild<LLUICtrl>("panel_flexi")->setVisible(item->getColumn(4)->getValue().asBoolean());
		getChild<LLUICtrl>("label_flexi")->setValue(item->getColumn(22)->getValue());
		getChild<LLUICtrl>("panel_particles")->setVisible(item->getColumn(5)->getValue().asBoolean());
		getChild<LLUICtrl>("label_particles")->setValue(item->getColumn(5)->getValue());
		getChild<LLUICtrl>("panel_light")->setVisible(item->getColumn(6)->getValue().asBoolean());
		getChild<LLUICtrl>("label_light")->setValue(item->getColumn(18)->getValue());
		getChild<LLUICtrl>("panel_projector")->setVisible(item->getColumn(7)->getValue().asBoolean());
		getChild<LLUICtrl>("label_projector")->setValue(item->getColumn(19)->getValue());
		getChild<LLUICtrl>("panel_alpha")->setVisible(item->getColumn(8)->getValue().asBoolean());
		getChild<LLUICtrl>("label_alpha")->setValue(item->getColumn(20)->getValue());
		getChild<LLUICtrl>("panel_media")->setVisible(item->getColumn(14)->getValue().asBoolean());
		getChild<LLUICtrl>("label_media")->setValue(item->getColumn(23)->getValue());
		getChild<LLUICtrl>("panel_rigged")->setVisible(item->getColumn(13)->getValue().asBoolean());
		getChild<LLUICtrl>("label_rigged")->setValue(item->getColumn(21)->getValue());
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