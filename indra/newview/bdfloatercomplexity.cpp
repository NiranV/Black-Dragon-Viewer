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

#include "bdfloatercomplexity.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llcharacter.h"
#include "lldrawable.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewerjointattachment.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llvovolume.h"
#include "llvolume.h"

#include "llselectmgr.h"
#include "llmeshrepository.h"

BDFloaterComplexity::BDFloaterComplexity(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("ARC.Refresh", boost::bind(&BDFloaterComplexity::calcARC, this));
}

BDFloaterComplexity::~BDFloaterComplexity()
{
}

BOOL BDFloaterComplexity::postBuild()
{
	//BD - ARC
	mAvatarScroll = this->getChild<LLScrollListCtrl>("arc_avs_scroll", true);
	//mAvatarScroll->setCommitCallback(boost::bind(&BDFloaterComplexity::onSelectAvatar, this));
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
void BDFloaterComplexity::onAvatarsRefresh()
{
	//BD - First lets go through all things that need updating, if any.
	std::vector<LLScrollListItem*> items = mAvatarScroll->getAllData();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LLScrollListItem* item = (*it);
		if (item)
		{
			//BD - Flag all items for removal by default.
			item->setFlagged(TRUE);

			//BD - Now lets check our list against all current avatars.
			for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
				iter != LLCharacter::sInstances.end(); ++iter)
			{
				LLCharacter* character = (*iter);
				//BD - Check if the character is valid and if its the same.
				if (character && character == item->getUserdata())
				{
					//BD - Remove the removal flag, we're updating it.
					item->setFlagged(FALSE);

					//BD - When we refresh it might happen that we don't have a name for someone
					//     yet, when this happens the list entry won't be purged and rebuild as
					//     it will be updated with this part, so we have to update the name in
					//     case it was still being resolved last time we refreshed and created the
					//     initial list entry. This prevents the name from missing forever.
					if (item->getColumn(0)->getValue().asString().empty())
					{
						LLAvatarName av_name;
						LLAvatarNameCache::get(character->getID(), &av_name);
						item->getColumn(0)->setValue(av_name.asLLSD());
					}
					break;
				}
			}
		}
	}

	//BD - Now safely delete all items so we can start adding the missing ones.
	mAvatarScroll->deleteFlaggedItems();

	//BD - Now lets do it the other way around and look for new things to add.
	items = mAvatarScroll->getAllData();
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLCharacter* character = (*iter);
		if (character)
		{
			LLUUID uuid = character->getID();
			bool skip = false;

			for (std::vector<LLScrollListItem*>::iterator it = items.begin();
				it != items.end(); ++it)
			{
				if ((*it)->getColumn(1)->getValue().asString() == uuid.asString())
				{
					skip = true;
					break;
				}
			}

			if (skip)
			{
				continue;
			}

			LLAvatarName av_name;
			LLAvatarNameCache::get(uuid, &av_name);

			LLSD row;
			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = av_name.getDisplayName();
			row["columns"][1]["column"] = "uuid";
			row["columns"][1]["value"] = uuid.asString();
			LLScrollListItem* element = mAvatarScroll->addElement(row);
			element->setUserdata(character);
		}
	}
}

void BDFloaterComplexity::calcARC()
{
	mARCScroll->clearRows();
	F32 cost = 0;
	//BD - Triangle Count
	F32 vertices = 0;
	F32 triangles = 0;
	LLVOVolume::texture_cost_t textures;
	S32Bytes texture_memory;

	//BD - Null Check hell.
	//     This thing is extremely fragile, crashes instantly the moment something is off even
	//     a tiny bit.
	LLScrollListItem* item = mAvatarScroll->getFirstSelected();
	if (item)
	{
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* character = dynamic_cast<LLVOAvatar*>(*iter);
			LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>((LLVOAvatar*)item->getUserdata());
			if (character && avatar == character)
			{
				for (LLVOAvatar::attachment_map_t::const_iterator attachment_point = character->mAttachmentPoints.begin();
					attachment_point != character->mAttachmentPoints.end();
					++attachment_point)
				{
					LLViewerJointAttachment* attachment = attachment_point->second;
					if (attachment)
					{
						if (!attachment->mAttachedObjects.empty())
						{
							for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
								attachment_iter != attachment->mAttachedObjects.end();
								++attachment_iter)
							{
								LLViewerObject* attached_object = (*attachment_iter);
								if (attached_object && !attached_object->isHUDAttachment())
								{
									textures.clear();
									const LLDrawable* drawable = attached_object->mDrawable;
									if (drawable)
									{
										LLVOVolume* volume = drawable->getVOVolume();
										if (volume)
										{
											F32 attachment_final_cost = 0;
											F32 attachment_total_cost = 0;
											F32 attachment_volume_cost = 0;
											F32 attachment_texture_cost = 0;
											F32 attachment_children_cost = 0;
											F32 attachment_base_cost = 0;
											S32Bytes attachment_memory_usage;

											S32 flexibles = 0;
											//S32 particles = 0;
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

											attachment_volume_cost += volume->getRenderCost(textures);
											attachment_total_cost += volume->mRenderComplexityTotal;
											attachment_base_cost += volume->mRenderComplexityBase;
											//BD - Triangle Count
											F32 attachment_total_triangles = volume->getHighLODTriangleCount();
											F32 attachment_total_vertices = volume->getNumVertices();
											if (volume->getIsLight())
											{
												lights++;
												is_light = true;
												//is_light = volume->getIsLight();
											}
											if (volume->isLightSpotlight())
											{
												projectors++;
												is_projector = true;
												//is_projector = volume->isLightSpotlight();
											}
											if (volume->isFlexible())
											{
												flexibles++;
												is_flexible = true;
												//is_flexible = volume->isFlexible();
											}
											//if (!has_particles)
												has_particles = volume->isParticleSource();
											//is_sculpt = child->isSculpted();
											//if (!is_animated)
												is_animated = volume->mIsAnimated;
											if (!is_rigged)
											{
												LLVolumeParams volume_params;
												volume_params = volume->getVolume()->getParams();
												if (gMeshRepo.getSkinInfo(volume_params.getSculptID(), volume))
												{
													is_rigged = true;
													rigged++;
												}
											}

											for (S32 i = 0; i < volume->getNumFaces(); i++)
											{
												LLTextureEntry* te = volume->getTE(i);
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

														if (/*!is_alpha
															&&*/ (te->getColor().mV[VW] < 1.f
															|| mat->getDiffuseAlphaMode() == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_BLEND))
														{
															is_alpha = true;
															alphas++;
														}
													}

													if (/*!has_media
														&&*/ te->hasMedia())
													{
														has_media = true;
														medias++;
													}
												}
											}

											LLVOVolume::const_child_list_t& children = volume->getChildren();
											for (LLVOVolume::const_child_list_t::const_iterator child_iter = children.begin();
												child_iter != children.end();
												++child_iter)
											{
												LLViewerObject* child_obj = *child_iter;
												LLVOVolume *child = dynamic_cast<LLVOVolume*>(child_obj);
												if (child)
												{
													attachment_children_cost += child->getRenderCost(textures);
													attachment_total_cost += child->mRenderComplexityTotal;
													//BD - Triangle Count
													attachment_total_triangles += child->getHighLODTriangleCount();
													attachment_total_vertices += child->getNumVertices();
													attachment_base_cost += child->mRenderComplexityBase;
													if (child->getIsLight())
													{
														lights++;
														is_light = true;
														//is_light = child->getIsLight();
													}
													if (child->isLightSpotlight())
													{
														projectors++;
														is_projector = true;
														//is_projector = child->isLightSpotlight();
													}
													if (child->isFlexible())
													{
														flexibles++;
														is_flexible = true;
														//is_flexible = child->isFlexible();
													}
													if (!has_particles)
														has_particles = child->isParticleSource();
													//is_sculpt = child->isSculpted();
													if (!is_animated)
														is_animated = child->mIsAnimated;
													if (!is_rigged)
													{
														LLVolumeParams volume_params;
														volume_params = child->getVolume()->getParams();
														if (gMeshRepo.getSkinInfo(volume_params.getSculptID(), child))
														{
															is_rigged = true;
															rigged++;
														}
													}

													for (S32 i = 0; i < child->getNumFaces(); i++)
													{
														LLTextureEntry* te = child->getTE(i);
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

																if (/*!is_alpha
																	&&*/ te->getColor().mV[VW] < 1.f)
																{
																	is_alpha = true;
																	alphas++;
																}
															}

															if (/*!has_media
																&&*/ te->hasMedia())
															{
																has_media = true;
																medias++;
															}
														}
													}
												}
											}

											for (LLVOVolume::texture_cost_t::iterator volume_texture = textures.begin();
												volume_texture != textures.end();
												++volume_texture)
											{
												LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(volume_texture->first);
												if (texture)
												{
													attachment_memory_usage += (texture->getTextureMemory() / 1024);
												}
												// add the cost of each individual texture in the linkset
												attachment_texture_cost += volume_texture->second;
											}
											texture_memory += attachment_memory_usage;

											attachment_final_cost = attachment_volume_cost + attachment_texture_cost + attachment_children_cost;
											// Limit attachment complexity to avoid signed integer flipping of the wearer's ACI
											cost += (U32)llclamp(attachment_final_cost, 0.f, 9999999.f);
											//BD - Triangle Count
											vertices += (U32)llclamp(attachment_total_vertices, 0.f, 9999999.f);
											triangles += (U32)llclamp(attachment_total_triangles, 0.f, 9999999.f);

											LLSD row;
											row["columns"][0]["column"] = "name";
											if (volume->getAttachmentItemName().empty())
											{
												row["columns"][0]["value"] = volume->getAttachmentItemID();
											}
											else
											{
												row["columns"][0]["value"] = volume->getAttachmentItemName();
											}
											row["columns"][1]["column"] = "arc";
											row["columns"][1]["value"] = attachment_final_cost;
											row["columns"][2]["column"] = "triangles";
											row["columns"][2]["value"] = attachment_total_triangles;
											row["columns"][3]["column"] = "vertices";
											row["columns"][3]["value"] = attachment_total_vertices;
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
											row["columns"][15]["value"] = attachment_base_cost;
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
											row["columns"][24]["value"] = attachment_texture_cost;
											row["columns"][25]["column"] = "total_arc";
											row["columns"][25]["value"] = attachment_total_cost;
											LLScrollListItem* element = mARCScroll->addElement(row);
											element->setUserdata(attached_object);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	mTotalVerticeCount->setValue(vertices);
	mTotalTriangleCount->setValue(triangles);
	mTotalCost->setValue(cost);
	mTextureCost->setValue(texture_memory.value());
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
		//if (item->getColumn(4)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_flexi")->setVisible(item->getColumn(4)->getValue().asBoolean());
			getChild<LLUICtrl>("label_flexi")->setValue(item->getColumn(22)->getValue());
		//if (item->getColumn(4)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_particles")->setVisible(item->getColumn(5)->getValue().asBoolean());
			getChild<LLUICtrl>("label_particles")->setValue(item->getColumn(5)->getValue());
		//if (item->getColumn(6)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_light")->setVisible(item->getColumn(6)->getValue().asBoolean());
			getChild<LLUICtrl>("label_light")->setValue(item->getColumn(18)->getValue());
		//if (item->getColumn(7)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_projector")->setVisible(item->getColumn(7)->getValue().asBoolean());
			getChild<LLUICtrl>("label_projector")->setValue(item->getColumn(19)->getValue());
		//if (item->getColumn(8)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_alpha")->setVisible(item->getColumn(8)->getValue().asBoolean());
			getChild<LLUICtrl>("label_alpha")->setValue(item->getColumn(20)->getValue());
		//if (item->getColumn(14)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_media")->setVisible(item->getColumn(14)->getValue().asBoolean());
			getChild<LLUICtrl>("label_media")->setValue(item->getColumn(23)->getValue());
		//if (item->getColumn(13)->getValue().asBoolean())
			getChild<LLUICtrl>("panel_rigged")->setVisible(item->getColumn(13)->getValue().asBoolean());
			getChild<LLUICtrl>("label_rigged")->setValue(item->getColumn(21)->getValue());
	}
}

void BDFloaterComplexity::onSelectAttachment()
{
	std::vector<LLScrollListItem*> items = mARCScroll->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator it = items.begin();
		it != items.end(); ++it)
	{
		LL_WARNS("Posing") << "Trying to select" << LL_ENDL;
		LLScrollListItem* item = (*it);
		LLViewerObject* vobject = (LLViewerObject*)item->getUserdata();
		if (vobject)
		{
			LL_WARNS("Posing") << "Found object" << LL_ENDL;
			LLViewerObject* select_object = gObjectList.findObject(vobject->getID());
			if (select_object && !select_object->isSelected())
			{
				LL_WARNS("Posing") << "In list, trying to select" << LL_ENDL;
				//LLSelectMgr::getInstance()->selectObjectAndFamily(select_object);
				//LLSelectMgr::getInstance()->selectObjectOnly(select_object);
				vobject->setSelected(TRUE);
				LLSelectNode* nodep = LLSelectMgr::getInstance()->getSelection()->findNode(vobject);
				if (nodep)
				{
					// rebuild selection with orphans
					LLSelectMgr::getInstance()->deselectObjectAndFamily(vobject);
					LLSelectMgr::getInstance()->selectObjectAndFamily(vobject);
				}
			}
		}
	}
}