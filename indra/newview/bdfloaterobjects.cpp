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

	mCommitCallbackRegistrar.add("ObjectList.Command", boost::bind(&BDFloaterObjects::onObjectCommand, this, _1, _2));
}

BDFloaterObjects::~BDFloaterObjects()
{
}

BOOL BDFloaterObjects::postBuild()
{
	//BD - Motions
	mObjectsScroll = getChild<LLScrollListCtrl>("object_scroll");
	mObjectsScroll->setCommitOnSelectionChange(TRUE);
	mObjectsScroll->setCommitCallback(boost::bind(&BDFloaterObjects::onSelectEntry, this));
	mObjectsScroll->setDoubleClickCallback(boost::bind(&BDFloaterObjects::onSelectObject, this));

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
	LLViewerObjectList::vobj_list_t objects = gObjectList.getAllObjects();

	for (LLViewerObject* objectp : objects)
	{
		LLPointer<LLDrawable> drawable = objectp->mDrawable;
		if (!drawable)
			continue;

		LLSD row;

		//BD - Check if this object is an attachment.
		if (objectp->isAttachment())
		{
			row["columns"][0]["font"]["style"] = "BOLD";
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
		if (gAgent.getID() == objectp->getRootEdit()->mOwnerID)
		{
			continue;
		}

		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = objectp->getID();
		row["columns"][1]["column"] = "shadow";
		if (drawable)
		{
			if (drawable->isLight())
			{
				row["columns"][1]["value"] = drawable->hasShadow() ? "true" : "false";
			}
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
						alpha = "blending";
						break;
					}
					else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_MASK)
					{
						alpha = "masking";
						break;
					}
					else if (mask == LLMaterial::eDiffuseAlphaMode::DIFFUSE_ALPHA_MODE_EMISSIVE)
					{
						alpha = "emissive";
						break;
					}
				}
			}
		}
		row["columns"][2]["column"] = "alpha";
		row["columns"][2]["value"] = alpha;
		mObjectsScroll->addElement(row);
	}

	//BD - Make sure we don't have a scrollbar unless we need it.
	mObjectsScroll->updateLayout();
}

void BDFloaterObjects::onObjectCommand(LLUICtrl* ctrl, const LLSD& param)
{
	if (!param.asString().empty())
	{
		for (LLScrollListItem* element : mObjectsScroll->getAllSelected())
		{
			LLViewerObject* objectp = gObjectList.findObject(element->getColumn(0)->getValue());
			if (!objectp)
				return;

			LLPointer<LLDrawable> drawable = objectp->mDrawable;
			if (!drawable)
				return;

			LLVOVolume* volobjp = (LLVOVolume*)objectp;
			if (!volobjp)
				return;

			if (param.asString() == "set_shadow"
				&& volobjp->isLightSpotlight())
			{
				volobjp->setHasShadow(ctrl->getValue().asBoolean() ? true : false);
				element->getColumn(1)->setValue(ctrl->getValue().asBoolean() ? "true" : "false");
			}

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
				element->getColumn(2)->setValue("sorting");
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

void BDFloaterObjects::onSelectObject()
{
	for (LLScrollListItem* item : mObjectsScroll->getAllSelected())
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

void BDFloaterObjects::onSelectEntry()
{
	LLScrollListItem* item = mObjectsScroll->getFirstSelected();
	{
		if (item)
		{
			LLViewerObject* vobject = gObjectList.findObject(item->getColumn(0)->getValue());
			if (vobject && !vobject->isDead())
			{
				LLPointer<LLDrawable> drawable = vobject->mDrawable;
				if (drawable)
				{
					getChild<LLButton>("shadows")->setToggleState(drawable->hasShadow());
				}
			}
		}
	}
}

////////////////////////////////
//BD - Misc Functions
////////////////////////////////