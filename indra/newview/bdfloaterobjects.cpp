/** 
 * 
 * Copyright (C) 2019, NiranV Dean
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

#include "bdfloaterobjects.h"

#include "llagent.h"
#include "llviewerobjectlist.h"
#include "llvovolume.h"
#include "llvoavatar.h"
#include "llmaterial.h"
#include "llselectmgr.h"
#include "pipeline.h"

BDFloaterObjects::BDFloaterObjects(const LLSD& key)
	:	LLFloater(key)
{
	//BD - Refresh the avatar list.
	mCommitCallbackRegistrar.add("ObjectList.Refresh", boost::bind(&BDFloaterObjects::onObjectRefresh, this));

	mCommitCallbackRegistrar.add("ObjectList.Object", boost::bind(&BDFloaterObjects::onObjectCommand, this, _1, _2));
	mCommitCallbackRegistrar.add("ObjectList.Light", boost::bind(&BDFloaterObjects::onLightCommand, this, _1, _2));
	mCommitCallbackRegistrar.add("ObjectList.Alpha", boost::bind(&BDFloaterObjects::onAlphaCommand, this, _1, _2));
}

BDFloaterObjects::~BDFloaterObjects()
{
}

BOOL BDFloaterObjects::postBuild()
{
	mObjectsScroll = { { this->getChild<LLScrollListCtrl>("object_scroll", true),
						this->getChild<LLScrollListCtrl>("light_object_scroll", true),
						this->getChild<LLScrollListCtrl>("alpha_object_scroll", true) } };
	//BD - Objects
	mObjectsScroll[0]->setCommitOnSelectionChange(TRUE);
	mObjectsScroll[0]->setCommitCallback(boost::bind(&BDFloaterObjects::onSelectEntry, this, 0));
	mObjectsScroll[0]->setDoubleClickCallback(boost::bind(&BDFloaterObjects::onSelectObject, this, 0));
	//BD - Light Sources
	mObjectsScroll[1]->setCommitOnSelectionChange(TRUE);
	mObjectsScroll[1]->setCommitCallback(boost::bind(&BDFloaterObjects::onSelectEntry, this, 1));
	mObjectsScroll[1]->setDoubleClickCallback(boost::bind(&BDFloaterObjects::onSelectObject, this, 1));
	//BD - Alphas
	mObjectsScroll[2]->setCommitOnSelectionChange(TRUE);
	mObjectsScroll[2]->setCommitCallback(boost::bind(&BDFloaterObjects::onSelectEntry, this, 2));
	mObjectsScroll[2]->setDoubleClickCallback(boost::bind(&BDFloaterObjects::onSelectObject, this, 2));

	return TRUE;
}

void BDFloaterObjects::draw()
{
	LLFloater::draw();
}

void BDFloaterObjects::onOpen(const LLSD& key)
{
}

void BDFloaterObjects::onClose(bool app_quitting)
{
}

////////////////////////////////
//BD - Motions
////////////////////////////////
void BDFloaterObjects::onObjectRefresh()
{
	//BD - Clear all scrolls first.
	mObjectsScroll[0]->clearRows();
	mObjectsScroll[1]->clearRows();
	mObjectsScroll[2]->clearRows();


	LLViewerObjectList::vobj_list_t objects = gObjectList.getAllObjects();
	for (LLViewerObject* objectp : objects)
	{
		if (!objectp->isRoot())
			continue;

		LLPointer<LLDrawable> drawable = objectp->mDrawable;
		if (!drawable)
			continue;

		LLSD row;
		bool attachment = objectp->isAttachment();
		//BD - Check if this object is an attachment.
		if (attachment)
		{
			//BD - Check if this attachment's owner is us.
			LLVOAvatar* avatar = objectp->getAvatarAncestor();
			if (avatar && avatar->isSelf())
			{
				//BD - We skip this attachment because if we change the alpha mode
				//     on our own attachments we might end up permanently changing
				//     them and that's something we really don't want.
				continue;
			}
		}

		//BD - Check the owner, if we are the owner, skip. We don't want to change
		//     our own items accidentally.
		//     Technically this check makes the one above unnecessary.
		if (gAgent.getID() == objectp->mOwnerID)
		{
			continue;
		}

		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = objectp->mID;
		row["columns"][0]["font"]["style"] = "BOLD";
		row["columns"][1]["column"] = "distance";
		row["columns"][1]["value"] = "???";
		row["columns"][2]["column"] = "attachment";
		row["columns"][2]["value"] = attachment ? "yes" : "no";
		mObjectsScroll[0]->addSeparator(ADD_BOTTOM);
		mObjectsScroll[0]->addElement(row);

		if (drawable->isLight())
		{
			row["columns"][1]["column"] = "light";
			row["columns"][1]["value"] = "true";
			row["columns"][2]["column"] = "shadow";
			row["columns"][2]["value"] = drawable->hasShadow() ? "true" : "false";
			mObjectsScroll[1]->addSeparator(ADD_BOTTOM);
			mObjectsScroll[1]->addElement(row);
		}

		std::string alpha = "none";
		for (S32 i = 0; i < drawable->getNumFaces(); i++)
		{
			LLFace* face = drawable->getFace(i);
			const LLTextureEntry* te = face->getTextureEntry();
			if (te)
			{
				const LLMaterialPtr mat = te->getMaterialParams();
				if (mat)
				{
					U8 mask = mat->getDiffuseAlphaMode();
					if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_BLEND
						|| mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_DEFAULT)
					{
						row["columns"][1]["column"] = "old_alpha";
						row["columns"][1]["value"] = "blending";
						row["columns"][2]["column"] = "alpha";
						row["columns"][2]["value"] = "blending";
						mObjectsScroll[2]->addSeparator(ADD_BOTTOM);
						mObjectsScroll[2]->addElement(row);
						break;
					}
					else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_MASK)
					{
						row["columns"][1]["column"] = "old_alpha";
						row["columns"][1]["value"] = "masking";
						row["columns"][2]["column"] = "alpha";
						row["columns"][2]["value"] = "masking";
						mObjectsScroll[2]->addSeparator(ADD_BOTTOM);
						mObjectsScroll[2]->addElement(row);
						break;
					}
					else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_EMISSIVE)
					{
						row["columns"][1]["column"] = "old_alpha";
						row["columns"][1]["value"] = "emissive";
						row["columns"][2]["column"] = "alpha";
						row["columns"][2]["value"] = "emissive";
						mObjectsScroll[2]->addSeparator(ADD_BOTTOM);
						mObjectsScroll[2]->addElement(row);
						break;
					}
				}
			}
		}

		for (LLViewerObject* link : objectp->getChildren())
		{
			if (link->isRoot())
				continue;

			LLPointer<LLDrawable> drawable = link->mDrawable;
			if (!drawable)
				continue;

			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = link->getID();
			row["columns"][0]["font"]["style"] = "Italic";
			if (drawable->isLight())
			{
				row["columns"][1]["column"] = "light";
				row["columns"][1]["value"] = "true";
				row["columns"][2]["column"] = "shadow";
				row["columns"][2]["value"] = drawable->hasShadow() ? "true" : "false";
				mObjectsScroll[1]->addElement(row);
			}

			std::string alpha = "none";
			for (S32 i = 0; i < drawable->getNumFaces(); i++)
			{
				LLFace* face = drawable->getFace(i);
				const LLTextureEntry* te = face->getTextureEntry();
				if (te)
				{
					const LLMaterialPtr mat = te->getMaterialParams();
					if (mat)
					{
						U8 mask = mat->getDiffuseAlphaMode();
						if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_BLEND
							|| mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_DEFAULT)
						{
							row["columns"][1]["column"] = "old_alpha";
							row["columns"][1]["value"] = "blending";
							row["columns"][2]["column"] = "alpha";
							row["columns"][2]["value"] = "blending";
							mObjectsScroll[2]->addElement(row);
							break;
						}
						else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_MASK)
						{
							row["columns"][1]["column"] = "old_alpha";
							row["columns"][1]["value"] = "masking";
							row["columns"][2]["column"] = "alpha";
							row["columns"][2]["value"] = "masking";
							mObjectsScroll[2]->addElement(row);
							break;
						}
						else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_EMISSIVE)
						{
							row["columns"][1]["column"] = "old_alpha";
							row["columns"][1]["value"] = "emissive";
							row["columns"][2]["column"] = "alpha";
							row["columns"][2]["value"] = "emissive";
							mObjectsScroll[2]->addElement(row);
							break;
						}
					}
				}
			}

			row["columns"][1]["column"] = "distance";
			row["columns"][1]["value"] = "???";
			row["columns"][2]["column"] = "attachment";
			row["columns"][2]["value"] = attachment ? "yes" : "no";
			row["columns"][3]["column"] = "parent";
			row["columns"][3]["value"] = objectp->getID();
			mObjectsScroll[0]->addElement(row);
		}
	}

	//BD - Make sure we don't have a scrollbar unless we need it.
	mObjectsScroll[0]->updateLayout();
	mObjectsScroll[1]->updateLayout();
	mObjectsScroll[2]->updateLayout();

	//BD - Disable sorting to prevent users from breaking groups.
	//mObjectsScroll[0]->mAllowSorting = false;
	//mObjectsScroll[1]->mAllowSorting = false;
	//mObjectsScroll[2]->mAllowSorting = false;
}

void BDFloaterObjects::onObjectCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		for (LLScrollListItem* element : mObjectsScroll[0]->getAllSelected())
		{
			LLViewerObject* objectp = gObjectList.findObject(element->getColumn(0)->getValue().asUUID());
			if (!objectp)
				continue;

			LLPointer<LLDrawable> drawable = objectp->mDrawable;
			if (!drawable)
				continue;

			LLVOVolume* volobjp = (LLVOVolume*)objectp;
			if (!volobjp)
				continue;

			if (param.asString() == "fullbright")
			{
				gObjectList.setFullbright(objectp, ctrl->getValue().asBoolean());
			}

			if (param.asString() == "dealpha")
			{
				gObjectList.setAlpha(objectp, false);
			}
			else if (param.asString() == "alpha")
			{
				gObjectList.setAlpha(objectp, true);
			}

			if (param.asString() == "derender")
			{
				gObjectList.killObject(objectp, true);
			}
		}
	}
}

void BDFloaterObjects::onLightCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		for (LLScrollListItem* element : mObjectsScroll[1]->getAllSelected())
		{
			LLViewerObject* objectp = gObjectList.findObject(element->getColumn(0)->getValue().asUUID());
			if (!objectp)
				continue;

			LLPointer<LLDrawable> drawable = objectp->mDrawable;
			if (!drawable)
				continue;

			LLVOVolume* volobjp = (LLVOVolume*)objectp;
			if (!volobjp)
				continue;

			if (param.asString() == "toggle_light")
			{
				volobjp->setIsLight(ctrl->getValue().asBoolean());
				element->getColumn(1)->setValue(ctrl->getValue().asBoolean() ? "true" : "false");
			}

			if (param.asString() == "set_shadow"
				&& volobjp->isLightSpotlight())
			{
				volobjp->setHasShadow(ctrl->getValue().asBoolean());
				element->getColumn(2)->setValue(ctrl->getValue().asBoolean() ? "true" : "false");
			}

			if (param.asString() == "dealpha")
			{
				gObjectList.setAlpha(objectp, false);
			}
			else if (param.asString() == "alpha")
			{
				gObjectList.setAlpha(objectp, true);
			}
		}
	}
}

void BDFloaterObjects::onAlphaCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		for (LLScrollListItem* element : mObjectsScroll[2]->getAllSelected())
		{
			LLViewerObject* objectp = gObjectList.findObject(element->getColumn(0)->getValue().asUUID());
			if (!objectp)
				continue;

			LLPointer<LLDrawable> drawable = objectp->mDrawable;
			if (!drawable)
				continue;

			LLVOVolume* volobjp = (LLVOVolume*)objectp;
			if (!volobjp)
				continue;

			if (param.asString() == "dealpha")
			{
				gObjectList.setAlpha(objectp, false);
			}
			else if (param.asString() == "alpha")
			{
				gObjectList.setAlpha(objectp, true);
			}
			else if (param.asString() == "set_none")
			{
				gObjectList.setAlphaMode(objectp, LLMaterial::DIFFUSE_ALPHA_MODE_NONE, false);
				element->getColumn(2)->setValue("none");
			}
			else if (param.asString() == "set_sorting")
			{
				gObjectList.setAlphaMode(objectp, LLMaterial::DIFFUSE_ALPHA_MODE_BLEND, false);
				element->getColumn(2)->setValue("blending");
			}
			else if (param.asString() == "set_masking")
			{
				gObjectList.setAlphaMode(objectp, LLMaterial::DIFFUSE_ALPHA_MODE_MASK, false);
				element->getColumn(2)->setValue("masking");
			}
			else if (param.asString() == "set_emissive")
			{
				gObjectList.setAlphaMode(objectp, LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE, false);
				element->getColumn(2)->setValue("emissive");
			}

			if (param.asString() == "reset")
			{
				for (S32 i = 0; i < drawable->getNumFaces(); i++)
				{
					const LLTextureEntry* tep = drawable->getTextureEntry(i);
					if (tep)
					{
						LLMaterialID material_id = tep->getMaterialID();
						//BD - This causes the alpha mask manipulation to be reverted due to a false pending material update.
						objectp->setTEMaterialID(i, material_id);
					}
				}
			}
		}
	}
}

void BDFloaterObjects::onSelectObject(S32 mode)
{
	for (LLScrollListItem* item : mObjectsScroll[mode]->getAllSelected())
	{
		if (item)
		{
			LLViewerObject* vobject = gObjectList.findObject(item->getColumn(0)->getValue());
			if (vobject && !vobject->isDead())
			{
				LLSelectMgr::instance().deselectAll();
				mObjectSelection = LLSelectMgr::instance().selectObjectOnly(vobject);

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

void BDFloaterObjects::onSelectEntry(S32 mode)
{
	LLScrollListItem* item = mObjectsScroll[mode]->getFirstSelected();
	{
		if (item)
		{
			LLViewerObject* vobject = gObjectList.findObject(item->getColumn(0)->getValue());
			if (vobject && !vobject->isDead())
			{
				LLPointer<LLDrawable> drawable = vobject->mDrawable;
				if (drawable)
				{
					getChild<LLButton>("set_shadows")->setToggleState(drawable->hasShadow());
					getChild<LLButton>("set_light")->setToggleState(drawable->isLight());
				}
			}
		}
	}
}

////////////////////////////////
//BD - Misc Functions
////////////////////////////////