/**
* @file llpanelface.cpp
* @brief Panel in the tools floater for editing face textures, colors, etc.
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
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
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelface.h"

// library includes
#include "llcalc.h"
#include "llerror.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llagent.h"
#include "llagentdata.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h" // gInventory
#include "llinventorymodelbackgroundfetch.h"
#include "llfloatermediasettings.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llmaterialmgr.h"
#include "llmediactrl.h"
#include "llmediaentry.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "llpanelcontents.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llpluginclassmedia.h"
#include "llviewertexturelist.h"// Update sel manager as to which channel we're editing so it can reflect the correct overlay UI

//
// Constant definitions for comboboxes
// Must match the commbobox definitions in panel_tools_texture.xml
//
const S32 MATMEDIA_MATERIAL = 0;	// Material
const S32 MATMEDIA_MEDIA = 1;		// Media
const S32 MATTYPE_DIFFUSE = 0;		// Diffuse material texture
const S32 MATTYPE_NORMAL = 1;		// Normal map
const S32 MATTYPE_SPECULAR = 2;		// Specular map
const S32 ALPHAMODE_MASK = 2;		// Alpha masking mode
const S32 BUMPY_TEXTURE = 18;		// use supplied normal map
const S32 SHINY_TEXTURE = 4;		// use supplied specular map

BOOST_STATIC_ASSERT(MATTYPE_DIFFUSE == LLRender::DIFFUSE_MAP && MATTYPE_NORMAL == LLRender::NORMAL_MAP && MATTYPE_SPECULAR == LLRender::SPECULAR_MAP);

//
// "Use texture" label for normal/specular type comboboxes
// Filled in at initialization from translated strings
//
std::string USE_TEXTURE;

// Things the UI provides...
//
LLUUID	LLPanelFace::getCurrentNormalMap()			{ return mBumpyTextureCtrl->getImageAssetID(); }
LLUUID	LLPanelFace::getCurrentSpecularMap()		{ return mShinyTextureCtrl->getImageAssetID(); }
U32		LLPanelFace::getCurrentShininess()			{ return mComboShiny->getCurrentIndex(); }
U32		LLPanelFace::getCurrentBumpiness()			{ return mComboBumpy->getCurrentIndex(); }
U8		LLPanelFace::getCurrentDiffuseAlphaMode()	{ return (U8)mComboAlpha->getCurrentIndex(); }
U8		LLPanelFace::getCurrentAlphaMaskCutoff()	{ return (U8)mMaskCutoff->getValue().asInteger(); }
U8		LLPanelFace::getCurrentEnvIntensity()		{ return (U8)mEnvironment->getValue().asInteger(); }
U8		LLPanelFace::getCurrentGlossiness()			{ return (U8)mGlossiness->getValue().asInteger(); }
//BD
F32		LLPanelFace::getCurrentBumpyRot()			{ return mTexRot->getValue().asReal(); }
F32		LLPanelFace::getCurrentBumpyScaleU()		{ return mTexScaleU->getValue().asReal(); }
F32		LLPanelFace::getCurrentBumpyScaleV()		{ return mTexScaleV->getValue().asReal(); }
F32		LLPanelFace::getCurrentBumpyOffsetU()		{ return mTexOffsetU->getValue().asReal(); }
F32		LLPanelFace::getCurrentBumpyOffsetV()		{ return mTexOffsetV->getValue().asReal(); }
F32		LLPanelFace::getCurrentShinyRot()			{ return mTexRot->getValue().asReal(); }
F32		LLPanelFace::getCurrentShinyScaleU()		{ return mTexScaleU->getValue().asReal(); }
F32		LLPanelFace::getCurrentShinyScaleV()		{ return mTexScaleV->getValue().asReal(); }
F32		LLPanelFace::getCurrentShinyOffsetU()		{ return mTexOffsetU->getValue().asReal(); }
F32		LLPanelFace::getCurrentShinyOffsetV()		{ return mTexOffsetV->getValue().asReal(); }

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	childSetCommitCallback("combobox shininess", &LLPanelFace::onCommitShiny, this);
	childSetCommitCallback("combobox bumpiness", &LLPanelFace::onCommitBump, this);
	childSetCommitCallback("combobox alphamode", &LLPanelFace::onCommitAlphaMode, this);
	childSetCommitCallback("TexScaleU", &LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexScaleV", &LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexRot", &LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("rptctrl", &LLPanelFace::onCommitRepeatsPerMeter, this);
	childSetCommitCallback("checkbox planar align", &LLPanelFace::onCommitPlanarAlign, this);
	childSetCommitCallback("TexOffsetU", LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexOffsetV", LLPanelFace::onCommitTextureInfo, this);

	childSetAction("align_media", &LLPanelFace::onClickAutoFix, this);

	setMouseOpaque(FALSE);

	mBtnAlign = getChild<LLButton>("align_media");
	mComboShiny = getChild<LLComboBox>("combobox shininess");
	mComboBumpy = getChild<LLComboBox>("combobox bumpiness");
	mComboAlpha = getChild<LLComboBox>("combobox alphamode");
	mLockDiffuse = getChild<LLUICtrl>("lock_diffuse_check");
	mLockSpec = getChild<LLUICtrl>("lock_spec_check");
	mLockBump = getChild<LLUICtrl>("lock_bump_check");

	mLabelShiny = getChild<LLTextBox>("shininess_label");
	mLabelBumpy = getChild<LLTextBox>("normalmap_label");
	mLabelAlpha = getChild<LLTextBox>("alphamode_label");
	mLabelTexGen = getChild<LLTextBox>("tex gen");

	mCheckAlignPlanar = getChild<LLCheckBoxCtrl>("checkbox planar align");

	mTexScaleU = getChild<LLUICtrl>("TexScaleU");
	mTexScaleV = getChild<LLUICtrl>("TexScaleV");

	mTexOffsetU = getChild<LLUICtrl>("TexOffsetU");
	mTexOffsetV = getChild<LLUICtrl>("TexOffsetV");

	mTexRot = getChild<LLUICtrl>("TexRot");
	mRepeats = getChild<LLUICtrl>("rptctrl");

	mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	mTextureCtrl->setDefaultImageAssetID(LLUUID(gSavedSettings.getString("DefaultObjectTexture")));
	mTextureCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitTexture, this, _2));
	mTextureCtrl->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelTexture, this, _2));
	mTextureCtrl->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectTexture, this, _2));
	mTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
	mTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
	mTextureCtrl->setOnCloseCallback(boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2));

	mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
	mTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mShinyTextureCtrl = getChild<LLTextureCtrl>("shinytexture control");
	mShinyTextureCtrl->setDefaultImageAssetID(LLUUID(gSavedSettings.getString("DefaultObjectSpecularTexture")));
	mShinyTextureCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitSpecularTexture, this, _2));
	mShinyTextureCtrl->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelSpecularTexture, this, _2));
	mShinyTextureCtrl->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectSpecularTexture, this, _2));
	mShinyTextureCtrl->setOnCloseCallback(boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2));
	mShinyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
	mShinyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));

	mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
	mShinyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mBumpyTextureCtrl = getChild<LLTextureCtrl>("bumpytexture control");
	mBumpyTextureCtrl->setDefaultImageAssetID(LLUUID(gSavedSettings.getString("DefaultObjectNormalTexture")));
	mBumpyTextureCtrl->setBlankImageAssetID(LLUUID(gSavedSettings.getString("DefaultBlankNormalTexture")));
	mBumpyTextureCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitNormalTexture, this, _2));
	mBumpyTextureCtrl->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelNormalTexture, this, _2));
	mBumpyTextureCtrl->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectNormalTexture, this, _2));
	mBumpyTextureCtrl->setOnCloseCallback(boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2));
	mBumpyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
	mBumpyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));

	mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
	mBumpyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	mColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitColor, this, _2));
	mColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelColor, this, _2));
	mColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectColor, this, _2));
	mColorSwatch->setCanApplyImmediately(TRUE);

	mColorSwatchShiny = getChild<LLColorSwatchCtrl>("shinycolorswatch");
	mColorSwatchShiny->setCommitCallback(boost::bind(&LLPanelFace::onCommitShinyColor, this, _2));
	mColorSwatchShiny->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelShinyColor, this, _2));
	mColorSwatchShiny->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectShinyColor, this, _2));
	mColorSwatchShiny->setCanApplyImmediately(TRUE);

	//BD
	mColorTransparency = getChild<LLSliderCtrl>("ColorTrans");
	mColorTransparency->setCommitCallback(boost::bind(&LLPanelFace::sendAlpha, this));

	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	mCheckFullbright->setCommitCallback(LLPanelFace::onCommitFullbright, this);

	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	mComboTexGen->setCommitCallback(LLPanelFace::onCommitTexGen, this);

	//BD
	mRadioMaterialType = getChild<LLRadioGroup>("radio mattype");
	mRadioMaterialType->setCommitCallback(LLPanelFace::onCommitMaterialType, this);
	mRadioMaterialType->selectNthItem(MATTYPE_DIFFUSE);

	//BD
	mGlow = getChild<LLSliderCtrl>("glow");
	mGlow->setCommitCallback(boost::bind(&LLPanelFace::sendGlow, this));

	mGlossiness = getChild<LLSliderCtrl>("glossiness");
	mGlossiness->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialGloss, this));

	mEnvironment = getChild<LLSliderCtrl>("environment");
	mEnvironment->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialEnv, this));

    mMenuClipboardColor = getChild<LLMenuButton>("clipboard_color_params_btn");
    mMenuClipboardTexture = getChild<LLMenuButton>("clipboard_texture_params_btn");
    
    mTitleMedia = getChild<LLMediaCtrl>("title_media");
    mTitleMediaText = getChild<LLTextBox>("media_info");

	mMaskCutoff = getChild<LLSliderCtrl>("maskcutoff");
	mMaskCutoff->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialMaskCutoff, this));
	clearCtrls();

	return TRUE;
}

LLPanelFace::LLPanelFace()
	: LLPanel(),
	mIsAlpha(false)
{
    USE_TEXTURE = LLTrans::getString("use_texture");
    mCommitCallbackRegistrar.add("PanelFace.menuDoToSelected", boost::bind(&LLPanelFace::menuDoToSelected, this, _2));
    mEnableCallbackRegistrar.add("PanelFace.menuEnable", boost::bind(&LLPanelFace::menuEnableItem, this, _2));
}

LLPanelFace::~LLPanelFace()
{
    unloadMedia();
}

void LLPanelFace::draw()
{
    updateCopyTexButton();

    // grab media name/title and update the UI widget
    // Todo: move it, it's preferable not to update
    // labels inside draw
    updateMediaTitle();

    LLPanel::draw();
}

void LLPanelFace::sendTexture()
{
	if (!mTextureCtrl->getTentative())
	{
		// we grab the item id first, because we want to do a
		// permissions check in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if (id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
//		//BD - We have to do this to make sure selection a different texture channel
		//     doesnt interfere the diffuse texture picker. If we don't do this our
		//     selected diffuse map will always be applied to the currently selected
		//     texture channel for some reason, this behavior doesnt apply to the
		//     other texture pickers, probably because they got a special function to
		//     set their normal/specular image, this function here seems to be
		//     generalized, taking whatever texture channel is currently selected.
		LLSelectMgr::getInstance()->setTextureChannel(LLRender::DIFFUSE_MAP);
		LLSelectMgr::getInstance()->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump(U32 bumpiness)
{
	if (bumpiness < BUMPY_TEXTURE)
	{
		// _LL_DEBUGS("Materials") << "clearing bumptexture control" << LL_ENDL;
		mBumpyTextureCtrl->clear();
		mBumpyTextureCtrl->setImageAssetID(LLUUID());
	}

	updateBumpyControls(bumpiness == BUMPY_TEXTURE, true);

	LLUUID current_normal_map = mBumpyTextureCtrl->getImageAssetID();

	U8 bump = (U8)bumpiness & TEM_BUMP_MASK;

	// Clear legacy bump to None when using an actual normal map
	//
	if (!current_normal_map.isNull())
		bump = 0;

	// Set the normal map or reset it to null as appropriate
	//
	LLSelectedTEMaterial::setNormalID(this, current_normal_map);

	LLSelectMgr::getInstance()->selectionSetBumpmap( bump, mBumpyTextureCtrl->getImageItemID() );
}

void LLPanelFace::sendTexGen()
{
	U8 tex_gen = (U8)mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	LLSelectMgr::getInstance()->selectionSetTexGen(tex_gen);
}

void LLPanelFace::sendShiny(U32 shininess)
{
	if (shininess < SHINY_TEXTURE)
	{
		mShinyTextureCtrl->clear();
		mShinyTextureCtrl->setImageAssetID(LLUUID());
	}

	LLUUID specmap = getCurrentSpecularMap();

	U8 shiny = (U8)shininess & TEM_SHINY_MASK;
	if (!specmap.isNull())
		shiny = 0;

	LLSelectedTEMaterial::setSpecularID(this, specmap);

	LLSelectMgr::getInstance()->selectionSetShiny( shiny, mShinyTextureCtrl->getImageItemID() );

	updateShinyControls(!specmap.isNull(), true);

}

void LLPanelFace::sendFullbright()
{
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	LLSelectMgr::getInstance()->selectionSetFullbright(fullbright);
}

void LLPanelFace::sendColor()
{
	LLColor4 color = mColorSwatch->get();
	LLSelectMgr::getInstance()->selectionSetColorOnly(color);
}

void LLPanelFace::sendAlpha()
{
	F32 alpha = (100.f - mColorTransparency->getValue().asReal()) / 100.f;
	LLSelectMgr* select_mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle handle = select_mgr->getSelection();
	for (LLObjectSelection::iterator iter = handle->begin(); iter != handle->end(); ++iter)
	{
		LLSelectNode* node = (*iter);
		if (node)
		{
			LLViewerObject* objectp = node->getObject();
			if (objectp)
			{
				for (S32 index = 0; index != objectp->getNumTEs(); ++index)
				{
					LLTextureEntry* te = objectp->getTE(index);
					if (te && te->isSelected())
					{
						LLColor4 prev_color = te->getColor();
						prev_color.mV[VALPHA] = alpha;
						objectp->setTEColor(index, prev_color);
					}
				}
			}
		}
	}
	if (!mColorTransparency->isMouseHeldDown())
		select_mgr->selectionSetAlphaOnly(alpha);
}

void LLPanelFace::sendGlow()
{
	F32 glow = mGlow->getValue().asReal();
	LLSelectMgr* select_mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle handle = select_mgr->getSelection();
	for (LLObjectSelection::iterator iter = handle->begin(); iter != handle->end(); ++iter)
	{
		LLSelectNode* node = (*iter);
		if (node)
		{
			LLViewerObject* objectp = node->getObject();
			if (objectp)
			{
				for (S32 index = 0; index != objectp->getNumTEs(); ++index)
				{
					LLTextureEntry* te = objectp->getTE(index);
					if (te && te->isSelected())
					{
						objectp->setTEGlow(index, glow);
					}
				}
			}
		}
	}
	if (!mGlow->isMouseHeldDown())
		select_mgr->selectionSetGlow(glow);
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;

		bool align_planar = mPanel->mCheckAlignPlanar->get();

		llassert(object);

		if (mPanel->mTexScaleU)
		{
			valid = !mPanel->mTexScaleU->getTentative(); // || !checkFlipScaleS->getTentative();
			if (valid || align_planar)
			{
				value = mPanel->mTexScaleU->getValue().asReal();
				if (mPanel->mComboTexGen &&
					mPanel->mComboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS(te, value);

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, value, te);
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, value, te);
				}
			}
		}

		if (mPanel->mTexScaleV)
		{
			valid = !mPanel->mTexScaleV->getTentative(); // || !checkFlipScaleT->getTentative();
			if (valid || align_planar)
			{
				value = mPanel->mTexScaleV->getValue().asReal();
				//if( checkFlipScaleT->get() )
				//{
				//	value = -value;
				//}
				if (mPanel->mComboTexGen &&
					mPanel->mComboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT(te, value);

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, value, te);
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, value, te);
				}
			}
		}

		if (mPanel->mTexOffsetU)
		{
			valid = !mPanel->mTexOffsetU->getTentative();
			if (valid || align_planar)
			{
				value = mPanel->mTexOffsetU->getValue().asReal();
				object->setTEOffsetS(te, value);

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, value, te);
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, value, te);
				}
			}
		}

		if (mPanel->mTexOffsetV)
		{
			valid = !mPanel->mTexOffsetV->getTentative();
			if (valid || align_planar)
			{
				value = mPanel->mTexOffsetV->getValue().asReal();
				object->setTEOffsetT(te, value);

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, value, te);
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, value, te);
				}
			}
		}

		if (mPanel->mTexRot)
		{
			valid = !mPanel->mTexRot->getTentative();
			if (valid || align_planar)
			{
				value = mPanel->mTexRot->getValue().asReal() * DEG_TO_RAD;
				object->setTERotation(te, value);

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, value, te);
					LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, value, te);
				}
			}
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
};

// Functor that aligns a face to mCenterFace
struct LLPanelFaceSetAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetAlignedTEFunctor(LLPanelFace* panel, LLFace* center_face) :
		mPanel(panel),
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return true;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{
			return true;
		}

		bool set_aligned = true;
		if (facep == mCenterFace)
		{
			set_aligned = false;
		}
		if (set_aligned)
		{
			LLVector2 uv_offset, uv_scale;
			F32 uv_rot;
			set_aligned = facep->calcAlignedPlanarTE(mCenterFace, &uv_offset, &uv_scale, &uv_rot);
			if (set_aligned)
			{
				object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
				object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
				object->setTERotation(te, uv_rot);

				LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, uv_rot, te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, uv_rot, te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
			}
		}
		if (!set_aligned)
		{
			LLPanelFaceSetTEFunctor setfunc(mPanel);
			setfunc.apply(object, te);
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
	LLFace* mCenterFace;
};

// Functor that tests if a face is aligned to mCenterFace
struct LLPanelFaceGetIsAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceGetIsAlignedTEFunctor(LLFace* center_face) :
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return false;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{ //volume face does not exist, can't be aligned
			return false;
		}

		if (facep == mCenterFace)
		{
			return true;
		}

		LLVector2 aligned_st_offset, aligned_st_scale;
		F32 aligned_st_rot;
		if (facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot))
		{
			const LLTextureEntry* tep = facep->getTextureEntry();
			LLVector2 st_offset, st_scale;
			tep->getOffset(&st_offset.mV[VX], &st_offset.mV[VY]);
			tep->getScale(&st_scale.mV[VX], &st_scale.mV[VY]);
			F32 st_rot = tep->getRotation();

            bool eq_offset_x = is_approx_equal_fraction(st_offset.mV[VX], aligned_st_offset.mV[VX], 12);
            bool eq_offset_y = is_approx_equal_fraction(st_offset.mV[VY], aligned_st_offset.mV[VY], 12);
            bool eq_scale_x  = is_approx_equal_fraction(st_scale.mV[VX], aligned_st_scale.mV[VX], 12);
            bool eq_scale_y  = is_approx_equal_fraction(st_scale.mV[VY], aligned_st_scale.mV[VY], 12);
            bool eq_rot      = is_approx_equal_fraction(st_rot, aligned_st_rot, 6);

			// needs a fuzzy comparison, because of fp errors
			if (eq_offset_x && 
				eq_offset_y && 
				eq_scale_x &&
				eq_scale_y &&
				eq_rot)
			{
				return true;
			}
		}
		return false;
	}
private:
	LLFace* mCenterFace;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

void LLPanelFace::sendTextureInfo()
{
	if (mCheckAlignPlanar->getValue().asBoolean())
	{
		LLFace* last_face = NULL;
		bool identical_face = false;
		LLSelectedTE::getFace(last_face, identical_face);
		LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}
	else
	{
		LLPanelFaceSetTEFunctor setfunc(this);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::getState()
{
	updateUI();
}

void LLPanelFace::updateUI(bool force_set_values)
{ //set state of UI to match state of texture entry(ies)  (calls setEnabled, setValue, etc, but NOT setVisible)
	LLSelectMgr* select_mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle selection = select_mgr->getSelection();
	LLViewerObject* objectp = selection->getFirstObject();

	if (objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME
		&& objectp->permModify())
	{
		BOOL editable = objectp->permModify() && !objectp->isPermanentEnforced();

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
		mBtnAlign->setEnabled(editable);

		/*if (mComboMatMedia)
		{
			if (mComboMatMedia->getCurrentIndex() < MATMEDIA_MATERIAL)
			{
                mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
			}
            mComboMatMedia->setEnabled(editable);
		}
		else
		{
			LL_WARNS() << "failed getChild for 'combobox matmedia'" << LL_ENDL;
		}*/

		//BD
		if(mRadioMaterialType)
		{
		    if (mRadioMaterialType->getSelectedIndex() < MATTYPE_DIFFUSE)
		    {
				mRadioMaterialType->selectNthItem(MATTYPE_DIFFUSE);
		    }
		}
		else
		{
			mRadioMaterialType->selectNthItem(MATTYPE_DIFFUSE);
		}
		mRadioMaterialType->setEnabled(editable);
		U32 material_type = mRadioMaterialType->getSelectedIndex();

		updateVisibility();

		bool identical = true;	// true because it is anded below
		bool identical_diffuse = false;
		bool identical_norm = false;
		bool identical_spec = false;

		LLUUID id;
		LLUUID normmap_id;
		LLUUID specmap_id;

		LLColor4 color = LLColor4::white;
		bool		identical_color = false;

		LLSelectedTE::getColor(color, identical_color);
		LLColor4 prev_color = mColorSwatch->get();

		mColorSwatch->setOriginal(color);
		mColorSwatch->set(color, force_set_values || (prev_color != color) || !editable);

		mColorSwatch->setValid(editable);
		mColorSwatch->setEnabled(editable);
		mColorSwatch->setCanApplyImmediately(editable);

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		mColorTransparency->setValue(editable ? transparency : 0);
		mColorTransparency->setEnabled(editable);

		// Specular map
		LLSelectedTEMaterial::getSpecularID(specmap_id, identical_spec);

		U8 shiny = 0;
		bool identical_shiny = false;

		// Shiny
		LLSelectedTE::getShiny(shiny, identical_shiny);
		identical = identical && identical_shiny;

		shiny = specmap_id.isNull() ? shiny : SHINY_TEXTURE;

		mComboShiny->selectNthItem((S32)shiny);

		//BD
		mLockDiffuse->setEnabled(editable);
		mLockSpec->setEnabled(editable && specmap_id.notNull());

		mLabelShiny->setReadOnly(!editable);
		mComboShiny->setEnabled(editable);

		U32 shiny_value = mComboShiny->getCurrentIndex();
		bool show_shinyctrls = (shiny_value == SHINY_TEXTURE); // Use texture
		mGlossiness->setEnabled(editable && show_shinyctrls);
		mEnvironment->setEnabled(editable && show_shinyctrls);
		mColorSwatchShiny->setEnabled(editable && show_shinyctrls);

		mComboShiny->setTentative(!identical_spec);
		mGlossiness->setTentative(!identical_spec);
		mEnvironment->setTentative(!identical_spec);
		mColorSwatchShiny->setTentative(!identical_spec);

		//BD
		if (!editable) mColorSwatchShiny->setOriginal(color);
		if (!editable) mColorSwatchShiny->set(color, TRUE);
		mColorSwatchShiny->setEnabled(editable);
		mColorSwatchShiny->setFallbackImage(LLUI::getUIImage("locked_image.j2c"));
		mColorSwatchShiny->setValid(editable);
		mColorSwatchShiny->setCanApplyImmediately(editable);

		U8 bumpy = 0;
		// Bumpy
		{
			bool identical_bumpy = false;
			LLSelectedTE::getBumpmap(bumpy, identical_bumpy);

			LLUUID norm_map_id = getCurrentNormalMap();

			bumpy = norm_map_id.isNull() ? bumpy : BUMPY_TEXTURE;

			mComboBumpy->selectNthItem((S32)bumpy);

			//BD - Is this even there still? TODO: Remove it.
			mLabelBumpy->setReadOnly(!editable);
			mComboBumpy->setEnabled(editable);
			mComboBumpy->setTentative(!identical_bumpy);
			//BD
			mLockBump->setEnabled(editable && norm_map_id.notNull());
		}

		// Texture
		{
			LLSelectedTE::getTexId(id, identical_diffuse);

			// Normal map
			LLSelectedTEMaterial::getNormalID(normmap_id, identical_norm);

			mIsAlpha = FALSE;
			LLGLenum image_format = GL_RGB;
			bool identical_image_format = false;
			LLSelectedTE::getImageFormat(image_format, identical_image_format);

			mIsAlpha = FALSE;
			switch (image_format)
			{
			case GL_RGBA:
			case GL_ALPHA:
			{
				mIsAlpha = TRUE;
			}
			break;

			case GL_RGB: break;
			default:
			{
				LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
			}
			break;
			}

			if(LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				mBtnAlign->setEnabled(editable);
			}

			// Diffuse Alpha Mode

			// Init to the default that is appropriate for the alpha content of the asset
			//
			U8 alpha_mode = mIsAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			bool identical_alpha_mode = false;

			// See if that's been overridden by a material setting for same...
			//
			LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(alpha_mode, identical_alpha_mode, mIsAlpha);

			//it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
			// Want masking? Want emissive? Tough! You get BLEND!
			alpha_mode = (transparency > 0.f) ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : alpha_mode;

			// ... unless there is no alpha channel in the texture, in which case alpha mode MUST be none
			alpha_mode = mIsAlpha ? alpha_mode : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			mComboAlpha->selectNthItem(alpha_mode);

			updateAlphaControls();

			if (mTextureCtrl)
			{
				if (identical_diffuse)
				{
					mTextureCtrl->setTentative(FALSE);
					mTextureCtrl->setEnabled(editable);
					mTextureCtrl->setImageAssetID(id);
					mLabelAlpha->setReadOnly(!(editable && mIsAlpha && transparency <= 0.f));
					//BD
					mComboAlpha->setEnabled(editable && mIsAlpha && transparency <= 0.f);
					mMaskCutoff->setEnabled(editable && mIsAlpha);
					mTextureCtrl->setBakeTextureEnabled(TRUE);
				}
				else if (id.isNull())
				{
					// None selected
					mTextureCtrl->setTentative(FALSE);
					mTextureCtrl->setEnabled(FALSE);
					mTextureCtrl->setImageAssetID(LLUUID::null);
					mLabelAlpha->setReadOnly(TRUE);
					//BD
					mComboAlpha->setEnabled(FALSE);
					mMaskCutoff->setEnabled(FALSE);
					mTextureCtrl->setBakeTextureEnabled(false);
				}
				else
				{
					// Tentative: multiple selected with different textures
					mTextureCtrl->setTentative(TRUE);
					mTextureCtrl->setEnabled(editable);
					mTextureCtrl->setImageAssetID(id);
					mLabelAlpha->setReadOnly(!(editable && mIsAlpha && transparency <= 0.f));
					//BD
					mComboAlpha->setEnabled(editable && mIsAlpha && transparency <= 0.f);
					mMaskCutoff->setEnabled(editable && mIsAlpha);
					mTextureCtrl->setBakeTextureEnabled(TRUE);
				}
			}

			if (mShinyTextureCtrl)
			{
				mShinyTextureCtrl->setTentative(!identical_spec);
				mShinyTextureCtrl->setEnabled(editable);
				mShinyTextureCtrl->setImageAssetID(specmap_id);
			}

			if (mBumpyTextureCtrl)
			{
				mBumpyTextureCtrl->setTentative(!identical_norm);
				mBumpyTextureCtrl->setEnabled(editable);
				mBumpyTextureCtrl->setImageAssetID(normmap_id);
			}
		}

		// planar align
		bool align_planar = false;
		bool identical_planar_aligned = false;
		{
			align_planar = mCheckAlignPlanar->getValue();

			bool enabled = (editable && isIdenticalPlanarTexgen());
			mCheckAlignPlanar->setValue(align_planar && enabled);
			mCheckAlignPlanar->setEnabled(enabled);
			childSetEnabled("button align textures", enabled && LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1);

			if (align_planar && enabled)
			{
				LLFace* last_face = NULL;
				bool identical_face = false;
				LLSelectedTE::getFace(last_face, identical_face);

				LLPanelFaceGetIsAlignedTEFunctor get_is_aligend_func(last_face);
				// this will determine if the texture param controls are tentative:
				identical_planar_aligned = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&get_is_aligend_func);
			}
		}

		// Needs to be public and before tex scale settings below to properly reflect
		// behavior when in planar vs default texgen modes in the
		// NORSPEC-84 et al
		//
		LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
		bool identical_texgen = true;
		bool identical_planar_texgen = false;

		{
			LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
			identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
		}

		// Texture scale
		{
			bool identical_diff_scale_s = false;
			bool identical_spec_scale_s = false;
			bool identical_norm_scale_s = false;

			identical = align_planar ? identical_planar_aligned : identical;

			F32 diff_scale_s = 1.f;
			F32 spec_scale_s = 1.f;
			F32 norm_scale_s = 1.f;

			LLSelectedTE::getScaleS(diff_scale_s, identical_diff_scale_s);
			LLSelectedTEMaterial::getSpecularRepeatX(spec_scale_s, identical_spec_scale_s);
			LLSelectedTEMaterial::getNormalRepeatX(norm_scale_s, identical_norm_scale_s);

			diff_scale_s = editable ? diff_scale_s : 1.0f;
			diff_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			norm_scale_s = editable ? norm_scale_s : 1.0f;
			norm_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_s = editable ? spec_scale_s : 1.0f;
			spec_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			//BD
			BOOL spec_scale_tentative = !(identical && identical_spec_scale_s);
			BOOL norm_scale_tentative = !(identical && identical_norm_scale_s);
			BOOL diff_scale_tentative = !(identical && identical_diff_scale_s);

			if (material_type == MATTYPE_SPECULAR)
			{
				mTexScaleU->setValue(spec_scale_s);
				mTexScaleU->setEnabled(editable && specmap_id.notNull());
				mTexScaleU->setTentative(LLSD(spec_scale_tentative));
			}
			else if (material_type == MATTYPE_NORMAL)
			{
				mTexScaleU->setValue(norm_scale_s);
				mTexScaleU->setEnabled(editable && normmap_id.notNull());
				mTexScaleU->setTentative(LLSD(norm_scale_tentative));
			}
			else
			{
				mTexScaleU->setValue(diff_scale_s);
				mTexScaleU->setEnabled(editable);
				mTexScaleU->setTentative(LLSD(diff_scale_tentative));
			}
		}

		{
			bool identical_diff_scale_t = false;
			bool identical_spec_scale_t = false;
			bool identical_norm_scale_t = false;

			F32 diff_scale_t = 1.f;
			F32 spec_scale_t = 1.f;
			F32 norm_scale_t = 1.f;

			LLSelectedTE::getScaleT(diff_scale_t, identical_diff_scale_t);
			LLSelectedTEMaterial::getSpecularRepeatY(spec_scale_t, identical_spec_scale_t);
			LLSelectedTEMaterial::getNormalRepeatY(norm_scale_t, identical_norm_scale_t);

			diff_scale_t = editable ? diff_scale_t : 1.0f;
			diff_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			norm_scale_t = editable ? norm_scale_t : 1.0f;
			norm_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_t = editable ? spec_scale_t : 1.0f;
			spec_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			BOOL diff_scale_tentative = !identical_diff_scale_t;
			BOOL norm_scale_tentative = !identical_norm_scale_t;
			BOOL spec_scale_tentative = !identical_spec_scale_t;

			//BD
			if (material_type == MATTYPE_SPECULAR)
			{
				mTexScaleV->setEnabled(editable && specmap_id.notNull());
				mTexScaleV->setValue(spec_scale_t);
				mTexScaleV->setTentative(LLSD(spec_scale_tentative));
			}
			else if (material_type == MATTYPE_NORMAL)
			{
				mTexScaleV->setEnabled(editable && normmap_id.notNull());
				mTexScaleV->setValue(norm_scale_t);
				mTexScaleV->setTentative(LLSD(norm_scale_tentative));
			}
			else
			{
				mTexScaleV->setEnabled(editable);
				mTexScaleV->setValue(diff_scale_t);
				mTexScaleV->setTentative(LLSD(diff_scale_tentative));
			}
		}

		// Texture offset
		{
			bool identical_diff_offset_s = false;
			bool identical_norm_offset_s = false;
			bool identical_spec_offset_s = false;

			F32 diff_offset_s = 0.0f;
			F32 norm_offset_s = 0.0f;
			F32 spec_offset_s = 0.0f;

			LLSelectedTE::getOffsetS(diff_offset_s, identical_diff_offset_s);
			LLSelectedTEMaterial::getNormalOffsetX(norm_offset_s, identical_norm_offset_s);
			LLSelectedTEMaterial::getSpecularOffsetX(spec_offset_s, identical_spec_offset_s);

			BOOL diff_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_s);
			BOOL norm_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_s);
			BOOL spec_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_s);

			//BD
			if (material_type == MATTYPE_SPECULAR)
			{
				mTexOffsetU->setValue(editable ? spec_offset_s : 0.0f);
				mTexOffsetU->setTentative(LLSD(spec_offset_u_tentative));
				mTexOffsetU->setEnabled(editable && specmap_id.notNull());
			}
			else if (material_type == MATTYPE_NORMAL)
			{
				mTexOffsetU->setValue(editable ? norm_offset_s : 0.0f);
				mTexOffsetU->setTentative(LLSD(norm_offset_u_tentative));
				mTexOffsetU->setEnabled(editable && normmap_id.notNull());
			}
			else
			{
				mTexOffsetU->setValue(editable ? diff_offset_s : 0.0f);
				mTexOffsetU->setTentative(LLSD(diff_offset_u_tentative));
				mTexOffsetU->setEnabled(editable);
			}
		}

		{
			bool identical_diff_offset_t = false;
			bool identical_norm_offset_t = false;
			bool identical_spec_offset_t = false;

			F32 diff_offset_t = 0.0f;
			F32 norm_offset_t = 0.0f;
			F32 spec_offset_t = 0.0f;

			LLSelectedTE::getOffsetT(diff_offset_t, identical_diff_offset_t);
			LLSelectedTEMaterial::getNormalOffsetY(norm_offset_t, identical_norm_offset_t);
			LLSelectedTEMaterial::getSpecularOffsetY(spec_offset_t, identical_spec_offset_t);

			BOOL diff_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_t);
			BOOL norm_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_t);
			BOOL spec_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_t);

			//BD
			if (material_type == MATTYPE_SPECULAR)
			{
				mTexOffsetV->setValue(editable ? spec_offset_t : 0.0f);
				mTexOffsetV->setTentative(LLSD(spec_offset_v_tentative));
				mTexOffsetV->setEnabled(editable && specmap_id.notNull());
			}
			else if (material_type == MATTYPE_NORMAL)
			{
				mTexOffsetV->setValue(editable ? norm_offset_t : 0.0f);
				mTexOffsetV->setTentative(LLSD(norm_offset_v_tentative));
				mTexOffsetV->setEnabled(editable && normmap_id.notNull());
			}
			else
			{
				mTexOffsetV->setValue(editable ? diff_offset_t : 0.0f);
				mTexOffsetV->setTentative(LLSD(diff_offset_v_tentative));
				mTexOffsetV->setEnabled(editable);
			}
		}

		// Texture rotation
		{
			bool identical_diff_rotation = false;
			bool identical_norm_rotation = false;
			bool identical_spec_rotation = false;

			F32 diff_rotation = 0.f;
			F32 norm_rotation = 0.f;
			F32 spec_rotation = 0.f;

			LLSelectedTE::getRotation(diff_rotation, identical_diff_rotation);
			LLSelectedTEMaterial::getSpecularRotation(spec_rotation, identical_spec_rotation);
			LLSelectedTEMaterial::getNormalRotation(norm_rotation, identical_norm_rotation);

			BOOL diff_rot_tentative = !(align_planar ? identical_planar_aligned : identical_diff_rotation);
			BOOL norm_rot_tentative = !(align_planar ? identical_planar_aligned : identical_norm_rotation);
			BOOL spec_rot_tentative = !(align_planar ? identical_planar_aligned : identical_spec_rotation);

			F32 diff_rot_deg = diff_rotation * RAD_TO_DEG;
			F32 norm_rot_deg = norm_rotation * RAD_TO_DEG;
			F32 spec_rot_deg = spec_rotation * RAD_TO_DEG;

			//BD
			if (material_type == MATTYPE_SPECULAR)
			{
				mTexRot->setEnabled(editable && specmap_id.notNull());
				mTexRot->setTentative(LLSD(spec_rot_tentative));
				mTexRot->setValue(editable ? spec_rot_deg : 0.0f);
			}
			else if (material_type == MATTYPE_NORMAL)
			{
				mTexRot->setEnabled(editable && normmap_id.notNull());
				mTexRot->setTentative(LLSD(norm_rot_tentative));
				mTexRot->setValue(editable ? norm_rot_deg : 0.0f);
			}
			else
			{
				mTexRot->setEnabled(editable);
				mTexRot->setTentative(diff_rot_tentative);
				mTexRot->setValue(editable ? diff_rot_deg : 0.0f);
			}
		}

		{
			F32 glow = 0.f;
			bool identical_glow = false;
			LLSelectedTE::getGlow(glow, identical_glow);
			mGlow->setValue(glow);
			mGlow->setTentative(!identical_glow);
			mGlow->setEnabled(editable);
		}

		{
			// Maps from enum to combobox entry index
			mComboTexGen->selectNthItem(((S32)selected_texgen) >> 1);

			mComboTexGen->setEnabled(editable);
			mComboTexGen->setTentative(!identical);
			mLabelTexGen->setEnabled(editable);
		}

		{
			U8 fullbright_flag = 0;
			bool identical_fullbright = false;

			LLSelectedTE::getFullbright(fullbright_flag, identical_fullbright);

			mCheckFullbright->setValue((S32)(fullbright_flag != 0));
			mCheckFullbright->setEnabled(editable);
			mCheckFullbright->setTentative(!identical_fullbright);
		}

		// Repeats per meter
		{
			F32 repeats_diff = 1.f;
			F32 repeats_norm = 1.f;
			F32 repeats_spec = 1.f;

			bool identical_diff_repeats = false;
			bool identical_norm_repeats = false;
			bool identical_spec_repeats = false;

			LLSelectedTE::getMaxDiffuseRepeats(repeats_diff, identical_diff_repeats);
			LLSelectedTEMaterial::getMaxNormalRepeats(repeats_norm, identical_norm_repeats);
			LLSelectedTEMaterial::getMaxSpecularRepeats(repeats_spec, identical_spec_repeats);

			S32 index = mComboTexGen ? mComboTexGen->getCurrentIndex() : 0;
			BOOL enabled = editable && (index != 1);
			BOOL identical_repeats = true;
			F32  repeats = 1.0f;

			select_mgr->setTextureChannel(LLRender::eTexIndex(material_type));

			switch (material_type)
			{
			default:
			case MATTYPE_DIFFUSE:
			{
				enabled = editable && !id.isNull();
				identical_repeats = identical_diff_repeats;
				repeats = repeats_diff;
			}
			break;

			case MATTYPE_SPECULAR:
			{
				enabled = (editable && ((shiny == SHINY_TEXTURE) && !specmap_id.isNull()));
				identical_repeats = identical_spec_repeats;
				repeats = repeats_spec;
			}
			break;

			case MATTYPE_NORMAL:
			{
				enabled = (editable && ((bumpy == BUMPY_TEXTURE) && !normmap_id.isNull()));
				identical_repeats = identical_norm_repeats;
				repeats = repeats_norm;
			}
			break;
			}

			BOOL repeats_tentative = !identical_repeats;

			mRepeats->setEnabled(identical_planar_texgen ? FALSE : enabled);
			mRepeats->setValue(editable ? repeats : 1.0f);
			mRepeats->setTentative(LLSD(repeats_tentative));
		}

		// Materials
		{
			LLMaterialPtr material;
			LLSelectedTEMaterial::getCurrent(material, identical);

			if (material && editable)
			{
				// _LL_DEBUGS("Materials") << material->asLLSD() << LL_ENDL;

				U32 alpha_mode = material->getDiffuseAlphaMode();

				if (transparency > 0.f)
				{ //it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
					alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
				}

				if (!mIsAlpha)
				{ // ... unless there is no alpha channel in the texture, in which case alpha mode MUST ebe none
					alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
				}
				mComboAlpha->selectNthItem(alpha_mode);
				mMaskCutoff->setValue(material->getAlphaMaskCutoff());
				updateAlphaControls();

				identical_planar_texgen = isIdenticalPlanarTexgen();

				// Shiny (specular)
				F32 offset_x, offset_y, repeat_x, repeat_y, rot;
				mShinyTextureCtrl->setImageAssetID(material->getSpecularID());

				if (!material->getSpecularID().isNull() && (shiny == SHINY_TEXTURE))
				{
					material->getSpecularOffset(offset_x, offset_y);
					material->getSpecularRepeat(repeat_x, repeat_y);

					if (identical_planar_texgen)
					{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
					}

					rot = material->getSpecularRotation();
					//BD
					if (material_type == MATTYPE_SPECULAR)
					{
						mTexScaleU->setValue(repeat_x);
						mTexScaleV->setValue(repeat_y);
						mTexRot->setValue(rot*RAD_TO_DEG);
						mTexOffsetU->setValue(offset_x);
						mTexOffsetV->setValue(offset_y);
					}
					mGlossiness->setValue(material->getSpecularLightExponent());
					mEnvironment->setValue(material->getEnvironmentIntensity());

					updateShinyControls(!material->getSpecularID().isNull(), true);
				}

				// Assert desired colorswatch color to match material AFTER updateShinyControls
				// to avoid getting overwritten with the default on some UI state changes.
				//
				if (!material->getSpecularID().isNull())
				{
					LLColor4 new_color = material->getSpecularLightColor();
					LLColor4 old_color = mColorSwatchShiny->get();

					mColorSwatchShiny->setOriginal(new_color);
					mColorSwatchShiny->set(new_color, force_set_values || old_color != new_color || !editable);
				}

				// Bumpy (normal)
				mBumpyTextureCtrl->setImageAssetID(material->getNormalID());

				if (!material->getNormalID().isNull())
				{
					material->getNormalOffset(offset_x, offset_y);
					material->getNormalRepeat(repeat_x, repeat_y);

					if (identical_planar_texgen)
					{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
					}

					rot = material->getNormalRotation();
					//BD
					if (material_type == MATTYPE_NORMAL)
					{
						mTexScaleU->setValue(repeat_x);
						mTexScaleV->setValue(repeat_y);
						mTexRot->setValue(rot*RAD_TO_DEG);
						mTexOffsetU->setValue(offset_x);
						mTexOffsetV->setValue(offset_y);
					}

					updateBumpyControls(!material->getNormalID().isNull(), true);
				}
			}
		}
        S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
        BOOL single_volume = (selected_count == 1);
        mMenuClipboardColor->setEnabled(editable && single_volume);

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->setVar(LLCalc::TEX_U_SCALE, mTexScaleU->getValue().asReal());
		calcp->setVar(LLCalc::TEX_V_SCALE, mTexScaleV->getValue().asReal());
		calcp->setVar(LLCalc::TEX_U_OFFSET, mTexRot->getValue().asReal());
		calcp->setVar(LLCalc::TEX_V_OFFSET, mTexOffsetU->getValue().asReal());
		calcp->setVar(LLCalc::TEX_ROTATION, mTexOffsetV->getValue().asReal());
		calcp->setVar(LLCalc::TEX_TRANSPARENCY, mColorTransparency->getValue().asReal());
		calcp->setVar(LLCalc::TEX_GLOW, mGlow->getValue().asReal());
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
		mTextureCtrl->setImageAssetID(LLUUID::null);
		mTextureCtrl->setEnabled(FALSE);  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
		mColorSwatch->setEnabled(FALSE);
		mColorSwatch->setFallbackImage(LLUI::getUIImage("locked_image.j2c"));
		mColorSwatch->setValid(FALSE);
		mColorTransparency->setEnabled(FALSE);
		mRepeats->setEnabled(FALSE);
		mLabelTexGen->setEnabled(FALSE);
		mLockDiffuse->setEnabled(FALSE);
		mLockSpec->setEnabled(FALSE);
		mLockBump->setEnabled(FALSE);

		updateVisibility();

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->clearVar(LLCalc::TEX_U_SCALE);
		calcp->clearVar(LLCalc::TEX_V_SCALE);
		calcp->clearVar(LLCalc::TEX_U_OFFSET);
		calcp->clearVar(LLCalc::TEX_V_OFFSET);
		calcp->clearVar(LLCalc::TEX_ROTATION);
		calcp->clearVar(LLCalc::TEX_TRANSPARENCY);
		calcp->clearVar(LLCalc::TEX_GLOW);
	}
}


void LLPanelFace::updateCopyTexButton()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    mMenuClipboardTexture->setEnabled(objectp && objectp->getPCode() == LL_PCODE_VOLUME && objectp->permModify() 
                                                    && !objectp->isPermanentEnforced() && !objectp->isInventoryPending() 
                                                    && (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1));
    std::string tooltip = (objectp && objectp->isInventoryPending()) ? LLTrans::getString("LoadingContents") : getString("paste_options");
    mMenuClipboardTexture->setToolTip(tooltip);

}

void LLPanelFace::refresh()
{
	// _LL_DEBUGS("Materials") << LL_ENDL;
	getState();
}

void LLPanelFace::refreshMedia()
{
    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
    LLViewerObject* first_object = selected_objects->getFirstObject();

    if (!(first_object
        && first_object->getPCode() == LL_PCODE_VOLUME
        && first_object->permModify()
        ))
    {
        getChildView("add_media")->setEnabled(FALSE);
        mTitleMediaText->clear();
        clearMediaSettings();
        return;
    }

    std::string url = first_object->getRegion()->getCapability("ObjectMedia");
    bool has_media_capability = (!url.empty());

    if (!has_media_capability)
    {
        getChildView("add_media")->setEnabled(FALSE);
        LL_WARNS("LLFloaterToolsMedia") << "Media not enabled (no capability) in this region!" << LL_ENDL;
        clearMediaSettings();
        return;
    }

    BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
        && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
        || LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();
    bool editable = is_nonpermanent_enforced && (first_object->permModify() || selectedMediaEditable());

    // Check modify permissions and whether any selected objects are in
    // the process of being fetched.  If they are, then we're not editable
    if (editable)
    {
        LLObjectSelection::iterator iter = selected_objects->begin();
        LLObjectSelection::iterator end = selected_objects->end();
        for (; iter != end; ++iter)
        {
            LLSelectNode* node = *iter;
            LLVOVolume* object = dynamic_cast<LLVOVolume*>(node->getObject());
            if (NULL != object)
            {
                if (!object->permModify())
                {
                    LL_INFOS("LLFloaterToolsMedia")
                        << "Selection not editable due to lack of modify permissions on object id "
                        << object->getID() << LL_ENDL;

                    editable = false;
                    break;
                }
            }
        }
    }

    // Media settings
    bool bool_has_media = false;
    struct media_functor : public LLSelectedTEGetFunctor<bool>
    {
        bool get(LLViewerObject* object, S32 face)
        {
            LLTextureEntry *te = object->getTE(face);
            if (te)
            {
                return te->hasMedia();
            }
            return false;
        }
    } func;


    // check if all faces have media(or, all dont have media)
    LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo = selected_objects->getSelectedTEValue(&func, bool_has_media);

    const LLMediaEntry default_media_data;

    struct functor_getter_media_data : public LLSelectedTEGetFunctor< LLMediaEntry>
    {
        functor_getter_media_data(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        LLMediaEntry get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return *(object->getTE(face)->getMediaData());
            return mMediaEntry;
        };

        const LLMediaEntry& mMediaEntry;

    } func_media_data(default_media_data);

    LLMediaEntry media_data_get;
    LLFloaterMediaSettings::getInstance()->mMultipleMedia = !(selected_objects->getSelectedTEValue(&func_media_data, media_data_get));

    std::string multi_media_info_str = LLTrans::getString("Multiple Media");
    std::string media_title = "";
    // update UI depending on whether "object" (prim or face) has media
    // and whether or not you are allowed to edit it.

    getChildView("add_media")->setEnabled(editable);
    // IF all the faces have media (or all dont have media)
    if (LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo)
    {
        // TODO: get media title and set it.
        mTitleMediaText->clear();
        // if identical is set, all faces are same (whether all empty or has the same media)
        if (!(LLFloaterMediaSettings::getInstance()->mMultipleMedia))
        {
            // Media data is valid
            if (media_data_get != default_media_data)
            {
                // initial media title is the media URL (until we get the name)
                media_title = media_data_get.getHomeURL();
            }
            // else all faces might be empty. 
        }
        else // there' re Different Medias' been set on on the faces.
        {
            media_title = multi_media_info_str;
        }

        getChildView("delete_media")->setEnabled(bool_has_media && editable);
        // TODO: display a list of all media on the face - use 'identical' flag
    }
    else // not all face has media but at least one does.
    {
        // seleted faces have not identical value
        LLFloaterMediaSettings::getInstance()->mMultipleValidMedia = selected_objects->isMultipleTEValue(&func_media_data, default_media_data);

        if (LLFloaterMediaSettings::getInstance()->mMultipleValidMedia)
        {
            media_title = multi_media_info_str;
        }
        else
        {
            // Media data is valid
            if (media_data_get != default_media_data)
            {
                // initial media title is the media URL (until we get the name)
                media_title = media_data_get.getHomeURL();
            }
        }

        getChildView("delete_media")->setEnabled(TRUE);
    }

    //U32 materials_media = mComboMatMedia->getCurrentIndex();
    //if (materials_media == MATMEDIA_MEDIA)
    {
        // currently displaying media info, navigateTo and update title
        navigateToTitleMedia(media_title);
    }
    /*else
    {
        // Media can be heavy, don't keep it around
        // MAC specific: MAC doesn't support setVolume(0) so if  not
        // unloaded, it might keep playing audio until user closes editor
        unloadMedia();
        mNeedMediaTitle = false;
    }*/

    mTitleMediaText->setText(media_title);

    // load values for media settings
    updateMediaSettings();

    LLFloaterMediaSettings::initValues(mMediaSettings, editable);
}

void LLPanelFace::unloadMedia()
{
    // destroy media source used to grab media title
    if (mTitleMedia)
        mTitleMedia->unloadMediaSource();
}

//////////////////////////////////////////////////////////////////////////////
//
void LLPanelFace::navigateToTitleMedia( const std::string url )
{
	std::string multi_media_info_str = LLTrans::getString("Multiple Media");
	if (url.empty() || multi_media_info_str == url)
	{
		// nothing to show
		mNeedMediaTitle = false;
	}
	else if (mTitleMedia)
	{
		LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
		// check if url changed or if we need a new media source
		if (mTitleMedia->getCurrentNavUrl() != url || media_plugin == NULL)
		{
			mTitleMedia->navigateTo( url );

            LLViewerMediaImpl* impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mTitleMedia->getTextureID());
            if (impl)
            {
                // if it's a page with a movie, we don't want to hear it
                impl->setVolume(0);
            };
		}

		// flag that we need to update the title (even if no request were made)
		mNeedMediaTitle = true;
	}
}

bool LLPanelFace::selectedMediaEditable()
{
    U32 owner_mask_on;
    U32 owner_mask_off;
    U32 valid_owner_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
        &owner_mask_on, &owner_mask_off);
    U32 group_mask_on;
    U32 group_mask_off;
    U32 valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
        &group_mask_on, &group_mask_off);
    U32 everyone_mask_on;
    U32 everyone_mask_off;
    S32 valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
        &everyone_mask_on, &everyone_mask_off);

    bool selected_Media_editable = false;

    // if perms we got back are valid
    if (valid_owner_perms &&
        valid_group_perms &&
        valid_everyone_perms)
    {

        if ((owner_mask_on & PERM_MODIFY) ||
            (group_mask_on & PERM_MODIFY) ||
            (everyone_mask_on & PERM_MODIFY))
        {
            selected_Media_editable = true;
        }
        else
            // user is NOT allowed to press the RESET button
        {
            selected_Media_editable = false;
        };
    };

    return selected_Media_editable;
}

void LLPanelFace::clearMediaSettings()
{
    LLFloaterMediaSettings::clearValues(false);
}

void LLPanelFace::updateMediaSettings()
{
    bool identical(false);
    std::string base_key("");
    std::string value_str("");
    int value_int = 0;
    bool value_bool = false;
    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
    // TODO: (CP) refactor this using something clever or boost or both !!

    const LLMediaEntry default_media_data;

    // controls 
    U8 value_u8 = default_media_data.getControls();
    struct functor_getter_controls : public LLSelectedTEGetFunctor< U8 >
    {
        functor_getter_controls(const LLMediaEntry &entry) : mMediaEntry(entry) {}

        U8 get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getControls();
            return mMediaEntry.getControls();
        };

        const LLMediaEntry &mMediaEntry;

    } func_controls(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_controls, value_u8);
    base_key = std::string(LLMediaEntry::CONTROLS_KEY);
    mMediaSettings[base_key] = value_u8;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // First click (formerly left click)
    value_bool = default_media_data.getFirstClickInteract();
    struct functor_getter_first_click : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_first_click(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getFirstClickInteract();
            return mMediaEntry.getFirstClickInteract();
        };

        const LLMediaEntry &mMediaEntry;

    } func_first_click(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_first_click, value_bool);
    base_key = std::string(LLMediaEntry::FIRST_CLICK_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Home URL
    value_str = default_media_data.getHomeURL();
    struct functor_getter_home_url : public LLSelectedTEGetFunctor< std::string >
    {
        functor_getter_home_url(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::string get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getHomeURL();
            return mMediaEntry.getHomeURL();
        };

        const LLMediaEntry &mMediaEntry;

    } func_home_url(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_home_url, value_str);
    base_key = std::string(LLMediaEntry::HOME_URL_KEY);
    mMediaSettings[base_key] = value_str;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Current URL
    value_str = default_media_data.getCurrentURL();
    struct functor_getter_current_url : public LLSelectedTEGetFunctor< std::string >
    {
        functor_getter_current_url(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::string get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getCurrentURL();
            return mMediaEntry.getCurrentURL();
        };

        const LLMediaEntry &mMediaEntry;

    } func_current_url(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_current_url, value_str);
    base_key = std::string(LLMediaEntry::CURRENT_URL_KEY);
    mMediaSettings[base_key] = value_str;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto zoom
    value_bool = default_media_data.getAutoZoom();
    struct functor_getter_auto_zoom : public LLSelectedTEGetFunctor< bool >
    {

        functor_getter_auto_zoom(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoZoom();
            return mMediaEntry.getAutoZoom();
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_zoom(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_zoom, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_ZOOM_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto play
    //value_bool = default_media_data.getAutoPlay();
    // set default to auto play TRUE -- angela  EXT-5172
    value_bool = true;
    struct functor_getter_auto_play : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_play(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoPlay();
            //return mMediaEntry.getAutoPlay(); set default to auto play TRUE -- angela  EXT-5172
            return true;
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_play(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_play, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_PLAY_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;


    // Auto scale
    // set default to auto scale TRUE -- angela  EXT-5172
    //value_bool = default_media_data.getAutoScale();
    value_bool = true;
    struct functor_getter_auto_scale : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_scale(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoScale();
            // return mMediaEntry.getAutoScale();  set default to auto scale TRUE -- angela  EXT-5172
            return true;
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_scale(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_scale, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_SCALE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto loop
    value_bool = default_media_data.getAutoLoop();
    struct functor_getter_auto_loop : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_loop(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoLoop();
            return mMediaEntry.getAutoLoop();
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_loop(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_loop, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_LOOP_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // width pixels (if not auto scaled)
    value_int = default_media_data.getWidthPixels();
    struct functor_getter_width_pixels : public LLSelectedTEGetFunctor< int >
    {
        functor_getter_width_pixels(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        int get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWidthPixels();
            return mMediaEntry.getWidthPixels();
        };

        const LLMediaEntry &mMediaEntry;

    } func_width_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_width_pixels, value_int);
    base_key = std::string(LLMediaEntry::WIDTH_PIXELS_KEY);
    mMediaSettings[base_key] = value_int;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // height pixels (if not auto scaled)
    value_int = default_media_data.getHeightPixels();
    struct functor_getter_height_pixels : public LLSelectedTEGetFunctor< int >
    {
        functor_getter_height_pixels(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        int get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getHeightPixels();
            return mMediaEntry.getHeightPixels();
        };

        const LLMediaEntry &mMediaEntry;

    } func_height_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_height_pixels, value_int);
    base_key = std::string(LLMediaEntry::HEIGHT_PIXELS_KEY);
    mMediaSettings[base_key] = value_int;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Enable Alt image
    value_bool = default_media_data.getAltImageEnable();
    struct functor_getter_enable_alt_image : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_enable_alt_image(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAltImageEnable();
            return mMediaEntry.getAltImageEnable();
        };

        const LLMediaEntry &mMediaEntry;

    } func_enable_alt_image(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_enable_alt_image, value_bool);
    base_key = std::string(LLMediaEntry::ALT_IMAGE_ENABLE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - owner interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_OWNER);
    struct functor_getter_perms_owner_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_owner_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_OWNER));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_OWNER);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_owner_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_owner_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_OWNER_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - owner control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_OWNER);
    struct functor_getter_perms_owner_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_owner_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_OWNER));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_OWNER);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_owner_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_owner_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_OWNER_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - group interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_GROUP);
    struct functor_getter_perms_group_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_group_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_GROUP));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_GROUP);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_group_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_group_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_GROUP_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - group control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_GROUP);
    struct functor_getter_perms_group_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_group_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_GROUP));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_GROUP);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_group_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_group_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_GROUP_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - anyone interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_ANYONE);
    struct functor_getter_perms_anyone_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_anyone_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_ANYONE));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_ANYONE);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_anyone_interact(default_media_data);
    identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func_perms_anyone_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_ANYONE_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - anyone control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_ANYONE);
    struct functor_getter_perms_anyone_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_anyone_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_ANYONE));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_ANYONE);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_anyone_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_anyone_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_ANYONE_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // security - whitelist enable
    value_bool = default_media_data.getWhiteListEnable();
    struct functor_getter_whitelist_enable : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_whitelist_enable(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWhiteListEnable();
            return mMediaEntry.getWhiteListEnable();
        };

        const LLMediaEntry &mMediaEntry;

    } func_whitelist_enable(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_whitelist_enable, value_bool);
    base_key = std::string(LLMediaEntry::WHITELIST_ENABLE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // security - whitelist URLs
    std::vector<std::string> value_vector_str = default_media_data.getWhiteList();
    struct functor_getter_whitelist_urls : public LLSelectedTEGetFunctor< std::vector<std::string> >
    {
        functor_getter_whitelist_urls(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::vector<std::string> get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWhiteList();
            return mMediaEntry.getWhiteList();
        };

        const LLMediaEntry &mMediaEntry;

    } func_whitelist_urls(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_whitelist_urls, value_vector_str);
    base_key = std::string(LLMediaEntry::WHITELIST_KEY);
    mMediaSettings[base_key].clear();
    std::vector< std::string >::iterator iter = value_vector_str.begin();
    while (iter != value_vector_str.end())
    {
        std::string white_list_url = *iter;
        mMediaSettings[base_key].append(white_list_url);
        ++iter;
    };

    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;
}

void LLPanelFace::updateMediaTitle()
{
    // only get the media name if we need it
    if (!mNeedMediaTitle)
        return;

    // get plugin impl
    LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
    if (media_plugin && mTitleMedia->getCurrentNavUrl() == media_plugin->getNavigateURI())
    {
        // get the media name (asynchronous - must call repeatedly)
        std::string media_title = media_plugin->getMediaName();

        // only replace the title if what we get contains something
        if (!media_title.empty())
        {
            // update the UI widget
            if (mTitleMediaText)
            {
                mTitleMediaText->setText(media_title);

                // stop looking for a title when we get one
                mNeedMediaTitle = false;
            };
        };
    };
}

//
// Static functions
//

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
	return (F32)(object->getTE(face)->getGlow());
}


void LLPanelFace::onCommitColor(const LLSD& data)
{
	sendColor();
}

void LLPanelFace::onCommitShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
}

void LLPanelFace::onCancelColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertColors();
}

void LLPanelFace::onCancelShinyColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertShinyColors();
}

void LLPanelFace::onSelectColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectColors();
	sendColor();
}

void LLPanelFace::onSelectShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
	LLSelectMgr::getInstance()->saveSelectedShinyColors();
}

// static
void LLPanelFace::onCommitMaterialsMedia(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	// Force to default states to side-step problems with menu contents
	// and generally reflecting old state when switching tabs or objects
	//
	self->updateShinyControls(false, true);
	self->updateBumpyControls(false, true);
	self->updateUI();
	self->refreshMedia();
}

void LLPanelFace::updateVisibility()
{
	mRadioMaterialType->setVisible(true);
	mRepeats->setVisible(true);

	// Diffuse texture controls
	updateAlphaControls();
	mTexScaleU->setVisible(true);
	mTexScaleV->setVisible(true);
	mTexRot->setVisible(true);
	mTexOffsetU->setVisible(true);
	mTexOffsetV->setVisible(true);

	// Specular map controls
	updateShinyControls();

	// Normal map controls
	updateBumpyControls();
}

// static
void LLPanelFace::onCommitMaterialType(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	// Force to default states to side-step problems with menu contents
	// and generally reflecting old state when switching tabs or objects
	//
	self->updateShinyControls(false, true);
	self->updateBumpyControls(false, true);
	self->updateUI();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	U32 bumpiness = self->mComboBumpy->getCurrentIndex();

	self->sendBump(bumpiness);
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::updateShinyControls(bool is_setting_texture, bool mess_with_shiny_combobox)
{
	LLUUID shiny_texture_ID = mShinyTextureCtrl->getImageAssetID();
	// _LL_DEBUGS("Materials") << "Shiny texture selected: " << shiny_texture_ID << LL_ENDL;

	if (mess_with_shiny_combobox)
	{
		if (!shiny_texture_ID.isNull() && is_setting_texture)
		{
			if (!mComboShiny->itemExists(USE_TEXTURE))
			{
				mComboShiny->add(USE_TEXTURE);
			}
			mComboShiny->setSimple(USE_TEXTURE);
		}
		else
		{
			if (mComboShiny->itemExists(USE_TEXTURE))
			{
				mComboShiny->remove(SHINY_TEXTURE);
				mComboShiny->selectFirstItem();
			}
		}
	}
	else
	{
		if (shiny_texture_ID.isNull() && mComboShiny && mComboShiny->itemExists(USE_TEXTURE))
		{
			mComboShiny->remove(SHINY_TEXTURE);
			mComboShiny->selectFirstItem();
		}
	}
}

// static
void LLPanelFace::updateBumpyControls(bool is_setting_texture, bool mess_with_combobox)
{
	LLUUID bumpy_texture_ID = mBumpyTextureCtrl->getImageAssetID();
	// _LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;

	if (mess_with_combobox)
	{
		LLUUID bumpy_texture_ID = mBumpyTextureCtrl->getImageAssetID();
		// _LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;

		if (!bumpy_texture_ID.isNull() && is_setting_texture)
		{
			if (!mComboBumpy->itemExists(USE_TEXTURE))
			{
				mComboBumpy->add(USE_TEXTURE);
			}
			mComboBumpy->setSimple(USE_TEXTURE);
		}
		else
		{
			if (mComboBumpy->itemExists(USE_TEXTURE))
			{
				mComboBumpy->remove(BUMPY_TEXTURE);
				mComboBumpy->selectFirstItem();
			}
		}
	}
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	U32 shininess = self->mComboShiny->getCurrentIndex();

	self->sendShiny(shininess);
}

// static
void LLPanelFace::updateAlphaControls()
{
	LLComboBox* comboAlphaMode = getChild<LLComboBox>("combobox alphamode");
	if (!comboAlphaMode)
	{
		return;
	}
	U32 alpha_value = comboAlphaMode->getCurrentIndex();
	bool show_alphactrls = (alpha_value == ALPHAMODE_MASK); // Alpha masking
    
    /*U32 mat_media = MATMEDIA_MATERIAL;
    if (mComboMatMedia)
    {
        mat_media = mComboMatMedia->getCurrentIndex();
    }*/
    
    U32 mat_type = MATTYPE_DIFFUSE;
    LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
    if(radio_mat_type)
    {
        mat_type = radio_mat_type->getSelectedIndex();
    }

    //show_alphactrls = show_alphactrls && (mat_media == MATMEDIA_MATERIAL);
    show_alphactrls = show_alphactrls && (mat_type == MATTYPE_DIFFUSE);
    
	getChildView("label maskcutoff")->setVisible(show_alphactrls);
	getChildView("maskcutoff")->setVisible(show_alphactrls);
}

// static
void LLPanelFace::onCommitAlphaMode(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	LLSelectedTEMaterial::setDiffuseAlphaMode(self, self->getCurrentDiffuseAlphaMode());
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	//BD - Don't commit fullbright changes ever as long as fullbrights
	//     are disabled, we'll end up updating the object with fullbright
	//     off. Keep all changes local to us until we enable fullbrights
	//     again.
	if (gSavedSettings.getBOOL("RenderEnableFullbright"))
	{
		LLPanelFace* self = (LLPanelFace*)userdata;
		self->sendFullbright();
	}
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item)
{
	BOOL accept = TRUE;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
		{
			accept = FALSE;
			break;
		}
	}
	return accept;
}

void LLPanelFace::onCommitTexture(const LLSD& data)
{
	add(LLStatViewer::EDIT_TEXTURE, 1);
	sendTexture();
}

void LLPanelFace::onCancelTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertTextures();
}

void LLPanelFace::onSelectTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectTextures();
	sendTexture();

	LLGLenum image_format;
	bool identical_image_format = false;
	LLSelectedTE::getImageFormat(image_format, identical_image_format);
	U32 alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
	if (mComboAlpha)
	{
		switch (image_format)
		{
		case GL_RGBA:
		case GL_ALPHA:
		{
			alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
		}
		break;

		case GL_RGB: break;
		default:
		{
			LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
		}
		break;
		}

		mComboAlpha->selectNthItem(alpha_mode);
	}
	LLSelectedTEMaterial::setDiffuseAlphaMode(this, getCurrentDiffuseAlphaMode());
}

void LLPanelFace::onCloseTexturePicker(const LLSD& data)
{
	// _LL_DEBUGS("Materials") << data << LL_ENDL;
	updateUI();
}

void LLPanelFace::onCommitSpecularTexture(const LLSD& data)
{
	// _LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onCommitNormalTexture(const LLSD& data)
{
	// _LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

void LLPanelFace::onCancelSpecularTexture(const LLSD& data)
{
	U8 shiny = 0;
	bool identical_shiny = false;
	LLSelectedTE::getShiny(shiny, identical_shiny);
	LLUUID spec_map_id = mShinyTextureCtrl->getImageAssetID();
	shiny = spec_map_id.isNull() ? shiny : SHINY_TEXTURE;
	sendShiny(shiny);
}

void LLPanelFace::onCancelNormalTexture(const LLSD& data)
{
	U8 bumpy = 0;
	bool identical_bumpy = false;
	LLSelectedTE::getBumpmap(bumpy, identical_bumpy);
	LLUUID spec_map_id = mBumpyTextureCtrl->getImageAssetID();
	bumpy = spec_map_id.isNull() ? bumpy : BUMPY_TEXTURE;
	sendBump(bumpy);
}

void LLPanelFace::onSelectSpecularTexture(const LLSD& data)
{
	// _LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onSelectNormalTexture(const LLSD& data)
{
	// _LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to edit existing media settings on a prim or prim face
// TODO: test if there is media on the item and only allow editing if present
void LLPanelFace::onClickBtnEditMedia(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*)userdata;
    self->refreshMedia();
    LLFloaterReg::showInstance("media_settings");
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to delete media from a prim or prim face
void LLPanelFace::onClickBtnDeleteMedia(LLUICtrl* ctrl, void* userdata)
{
    LLNotificationsUtil::add("DeleteMedia", LLSD(), LLSD(), deleteMediaConfirm);
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to add media to a prim or prim face
void LLPanelFace::onClickBtnAddMedia(LLUICtrl* ctrl, void* userdata)
{
    // check if multiple faces are selected
    if (LLSelectMgr::getInstance()->getSelection()->isMultipleTESelected())
    {
        LLPanelFace* self = (LLPanelFace*)userdata;
        self->refreshMedia();
        LLNotificationsUtil::add("MultipleFacesSelected", LLSD(), LLSD(), multipleFacesSelectedConfirm);
    }
    else
    {
        onClickBtnEditMedia(ctrl, userdata);
    }
}

// static
bool LLPanelFace::deleteMediaConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch (option)
    {
    case 0:  // "Yes"
        LLSelectMgr::getInstance()->selectionSetMedia(0, LLSD());
        if (LLFloaterReg::instanceVisible("media_settings"))
        {
            LLFloaterReg::hideInstance("media_settings");
        }
        break;

    case 1:  // "No"
    default:
        break;
    }
    return false;
}

// static
bool LLPanelFace::multipleFacesSelectedConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch (option)
    {
    case 0:  // "Yes"
        LLFloaterReg::showInstance("media_settings");
        break;
    case 1:  // "No"
    default:
        break;
    }
    return false;
}

//static
void LLPanelFace::onCommitMaterialGloss()
{
	LLSelectedTEMaterial::setSpecularLightExponent(this, getCurrentGlossiness());
}

//static
void LLPanelFace::onCommitMaterialEnv()
{
	LLSelectedTEMaterial::setEnvironmentIntensity(this, getCurrentEnvIntensity());
}

//static
void LLPanelFace::onCommitMaterialMaskCutoff()
{
	LLSelectedTEMaterial::setAlphaMaskCutoff(this, getCurrentAlphaMaskCutoff());
}

// static
void LLPanelFace::onCommitTextureInfo(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	llassert_always(self);
	U32 material_type = self->getTextureChannelToEdit();
	bool texture = material_type == MATTYPE_DIFFUSE;
	bool bumpiness = material_type == MATTYPE_NORMAL;
	bool has_norm = self->getCurrentNormalMap().notNull();
	bool shininess = material_type == MATTYPE_SPECULAR;
	bool has_spec = self->getCurrentSpecularMap().notNull();
	bool diffuse_locked = self->mLockDiffuse->getValue().asBoolean();
	bool bump_locked = self->mLockBump->getValue().asBoolean();
	bool spec_locked = self->mLockSpec->getValue().asBoolean();

	//BD - Change this?
	if (ctrl->getName() == "TexScaleU")
	{
		if (texture || diffuse_locked)
		{
			self->sendTextureInfo();
		}

		if ((bumpiness || bump_locked) && has_norm)
		{
			F32 bumpy_scale_u = self->getCurrentBumpyScaleU();
			if (self->isIdenticalPlanarTexgen())
			{
				bumpy_scale_u *= 0.5f;
			}
			LLSelectedTEMaterial::setNormalRepeatX(self, bumpy_scale_u);
		}

		if ((shininess || spec_locked) && has_spec)
		{
			F32 shiny_scale_u = self->getCurrentShinyScaleU();
			if (self->isIdenticalPlanarTexgen())
			{
				shiny_scale_u *= 0.5f;
			}
			LLSelectedTEMaterial::setSpecularRepeatX(self, shiny_scale_u);
		}
	}

	if (ctrl->getName() == "TexScaleV")
	{
		if (texture || diffuse_locked)
		{
			self->sendTextureInfo();
		}

		if ((bumpiness || bump_locked) && has_norm)
		{
			F32 bumpy_scale_v = self->getCurrentBumpyScaleV();
			if (self->isIdenticalPlanarTexgen())
			{
				bumpy_scale_v *= 0.5f;
			}
			LLSelectedTEMaterial::setNormalRepeatY(self, bumpy_scale_v);
		}

		if ((shininess || spec_locked) && has_spec)
		{
			F32 shiny_scale_v = self->getCurrentShinyScaleV();
			if (self->isIdenticalPlanarTexgen())
			{
				shiny_scale_v *= 0.5f;
			}
			LLSelectedTEMaterial::setSpecularRepeatY(self, shiny_scale_v);
		}
	}

	if (ctrl->getName() == "TexRot")
	{
		if (texture || diffuse_locked)
		{
			self->sendTextureInfo();
		}

		if ((bumpiness || bump_locked) && has_norm)
		{
			LLSelectedTEMaterial::setNormalRotation(self, self->getCurrentBumpyRot() * DEG_TO_RAD);
		}

		if ((shininess || spec_locked) && has_spec)
		{
			LLSelectedTEMaterial::setSpecularRotation(self, self->getCurrentShinyRot() * DEG_TO_RAD);
		}
	}

	if (ctrl->getName() == "TexOffsetU")
	{
		if (texture || diffuse_locked)
		{
			self->sendTextureInfo();
		}

		if ((bumpiness || bump_locked) && has_norm)
		{
			LLSelectedTEMaterial::setNormalOffsetX(self, self->getCurrentBumpyOffsetU());
		}

		if ((shininess || spec_locked) && has_spec)
		{
			LLSelectedTEMaterial::setSpecularOffsetX(self, self->getCurrentShinyOffsetU());
		}
	}

	if (ctrl->getName() == "TexOffsetV")
	{
		if (texture || diffuse_locked)
		{
			self->sendTextureInfo();
		}

		if ((bumpiness || bump_locked) && has_norm)
		{
			LLSelectedTEMaterial::setNormalOffsetY(self, self->getCurrentBumpyOffsetV());
		}

		if ((shininess || spec_locked) && has_spec)
		{
			LLSelectedTEMaterial::setSpecularOffsetY(self, self->getCurrentShinyOffsetV());
		}
	}
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onCommitRepeatsPerMeter(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;

	//BD
	U32 material_type = self->mRadioMaterialType->getSelectedIndex();

	F32 repeats_per_meter = self->mRepeats->getValue().asReal();
	
	//LLUICtrl*	repeats_ctrl	= self->getChild<LLUICtrl>("rptctrl");
	//LLRadioGroup* radio_mat_type = self->getChild<LLRadioGroup>("radio_material_type");
	
	//U32 materials_media = self->mComboMatMedia->getCurrentIndex();

	F32 obj_scale_s = 1.0f;
	F32 obj_scale_t = 1.0f;

	bool identical_scale_s = false;
	bool identical_scale_t = false;

	LLSelectedTE::getObjectScaleS(obj_scale_s, identical_scale_s);
	LLSelectedTE::getObjectScaleS(obj_scale_t, identical_scale_t);

	switch (material_type)
	{
	case MATTYPE_DIFFUSE:
	{
		LLSelectMgr::getInstance()->selectionTexScaleAutofit(repeats_per_meter);
	}
	break;

	case MATTYPE_NORMAL:
	{
		LLUICtrl* bumpy_scale_u = self->mTexScaleU;
		LLUICtrl* bumpy_scale_v = self->mTexScaleV;

		bumpy_scale_u->setValue(obj_scale_s * repeats_per_meter);
		bumpy_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setNormalRepeatX(self, obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setNormalRepeatY(self, obj_scale_t * repeats_per_meter);
	}
	break;

	case MATTYPE_SPECULAR:
	{
		LLUICtrl* shiny_scale_u = self->mTexScaleU;
		LLUICtrl* shiny_scale_v = self->mTexScaleV;

		shiny_scale_u->setValue(obj_scale_s * repeats_per_meter);
		shiny_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setSpecularRepeatX(self, obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setSpecularRepeatY(self, obj_scale_t * repeats_per_meter);
	}
	break;

	default:
		llassert(false);
		break;
	}
}

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		viewer_media_t pMediaImpl;

		const LLTextureEntry* tep = object->getTE(te);
		const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
		if (mep)
		{
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());
		}

		if (pMediaImpl.isNull())
		{
			// If we didn't find face media for this face, check whether this face is showing parcel media.
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getID());
		}

		if (pMediaImpl.notNull())
		{
			LLPluginClassMedia *media = pMediaImpl->getMediaPlugin();
			if (media)
			{
				S32 media_width = media->getWidth();
				S32 media_height = media->getHeight();
				S32 texture_width = media->getTextureWidth();
				S32 texture_height = media->getTextureHeight();
				F32 scale_s = (F32)media_width / (F32)texture_width;
				F32 scale_t = (F32)media_height / (F32)texture_height;

				// set scale and adjust offset
				object->setTEScaleS(te, scale_s);
				object->setTEScaleT(te, scale_t);	// don't need to flip Y anymore since QT does this for us now.
				object->setTEOffsetS(te, -(1.0f - scale_s) / 2.0f);
				object->setTEOffsetT(te, -(1.0f - scale_t) / 2.0f);
			}
		}
		return true;
	};
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}


enum EPasteMode
{
    PASTE_COLOR,
    PASTE_TEXTURE
};

struct LLPanelFacePasteTexFunctor : public LLSelectedTEFunctor
{
    LLPanelFacePasteTexFunctor(LLPanelFace* panel, EPasteMode mode) :
        mPanelFace(panel), mMode(mode) {}

    virtual bool apply(LLViewerObject* objectp, S32 te)
    {
        switch (mMode)
        {
        case PASTE_COLOR:
            mPanelFace->onPasteColor(objectp, te);
            break;
        case PASTE_TEXTURE:
            mPanelFace->onPasteTexture(objectp, te);
            break;
        }
        return true;
    }
private:
    LLPanelFace *mPanelFace;
    EPasteMode mMode;
};

struct LLPanelFaceUpdateFunctor : public LLSelectedObjectFunctor
{
    LLPanelFaceUpdateFunctor(bool update_media) : mUpdateMedia(update_media) {}
    virtual bool apply(LLViewerObject* object)
    {
        object->sendTEUpdate();
        if (mUpdateMedia)
        {
            LLVOVolume *vo = dynamic_cast<LLVOVolume*>(object);
            if (vo && vo->hasMedia())
            {
                vo->sendMediaDataUpdate();
            }
        }
        return true;
    }
private:
    bool mUpdateMedia;
};

struct LLPanelFaceNavigateHomeFunctor : public LLSelectedTEFunctor
{
    virtual bool apply(LLViewerObject* objectp, S32 te)
    {
        if (objectp && objectp->getTE(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            const LLMediaEntry *media_data = tep->getMediaData();
            if (media_data)
            {
                if (media_data->getCurrentURL().empty() && media_data->getAutoPlay())
                {
                    viewer_media_t media_impl =
                        LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getMediaData()->getMediaID());
                    if (media_impl)
                    {
                        media_impl->navigateHome();
                    }
                }
            }
        }
        return true;
    }
};

void LLPanelFace::onCopyColor()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        return;
    }

    if (mClipboardParams.has("color"))
    {
        mClipboardParams["color"].clear();
    }
    else
    {
        mClipboardParams["color"] = LLSD::emptyArray();
    }

    std::map<LLUUID, LLUUID> asset_item_map;

    // a way to resolve situations where source and target have different amount of faces
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    mClipboardParams["color_all_tes"] = (num_tes != 1) || (LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool());
    for (S32 te = 0; te < num_tes; ++te)
    {
        if (node->isTESelected(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            if (tep)
            {
                LLSD te_data;

                // asLLSD() includes media
                te_data["te"] = tep->asLLSD(); // Note: includes a lot more than just color/alpha/glow

                mClipboardParams["color"].append(te_data);
            }
        }
    }
}

void LLPanelFace::onPasteColor()
{
    if (!mClipboardParams.has("color"))
    {
        return;
    }

    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        // not supposed to happen
        LL_WARNS() << "Failed to paste color due to missing or wrong selection" << LL_ENDL;
        return;
    }

    bool face_selection_mode = LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
    LLSD &clipboard = mClipboardParams["color"]; // array
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    S32 compare_tes = num_tes;

    if (face_selection_mode)
    {
        compare_tes = 0;
        for (S32 te = 0; te < num_tes; ++te)
        {
            if (node->isTESelected(te))
            {
                compare_tes++;
            }
        }
    }

    // we can copy if single face was copied in edit face mode or if face count matches
    if (!((clipboard.size() == 1) && mClipboardParams["color_all_tes"].asBoolean())
        && compare_tes != clipboard.size())
    {
        LLSD notif_args;
        if (face_selection_mode)
        {
            static std::string reason = getString("paste_error_face_selection_mismatch");
            notif_args["REASON"] = reason;
        }
        else
        {
            static std::string reason = getString("paste_error_object_face_count_mismatch");
            notif_args["REASON"] = reason;
        }
        LLNotificationsUtil::add("FacePasteFailed", notif_args);
        return;
    }

    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();

    LLPanelFacePasteTexFunctor paste_func(this, PASTE_COLOR);
    selected_objects->applyToTEs(&paste_func);

    LLPanelFaceUpdateFunctor sendfunc(false);
    selected_objects->applyToObjects(&sendfunc);
}

void LLPanelFace::onPasteColor(LLViewerObject* objectp, S32 te)
{
    LLSD te_data;
    LLSD &clipboard = mClipboardParams["color"]; // array
    if ((clipboard.size() == 1) && mClipboardParams["color_all_tes"].asBoolean())
    {
        te_data = *(clipboard.beginArray());
    }
    else if (clipboard[te])
    {
        te_data = clipboard[te];
    }
    else
    {
        return;
    }

    LLTextureEntry* tep = objectp->getTE(te);
    if (tep)
    {
        if (te_data.has("te"))
        {
            // Color / Alpha
            if (te_data["te"].has("colors"))
            {
                LLColor4 color = tep->getColor();

                LLColor4 clip_color;
                clip_color.setValue(te_data["te"]["colors"]);

                // Color
                color.mV[VRED] = clip_color.mV[VRED];
                color.mV[VGREEN] = clip_color.mV[VGREEN];
                color.mV[VBLUE] = clip_color.mV[VBLUE];

                // Alpha
                color.mV[VALPHA] = clip_color.mV[VALPHA];

                objectp->setTEColor(te, color);
            }

            // Color/fullbright
            if (te_data["te"].has("fullbright"))
            {
                objectp->setTEFullbright(te, te_data["te"]["fullbright"].asInteger());
            }

            // Glow
            if (te_data["te"].has("glow"))
            {
                objectp->setTEGlow(te, (F32)te_data["te"]["glow"].asReal());
            }
        }
    }
}

void LLPanelFace::onCopyTexture()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        return;
    }

    if (mClipboardParams.has("texture"))
    {
        mClipboardParams["texture"].clear();
    }
    else
    {
        mClipboardParams["texture"] = LLSD::emptyArray();
    }

    std::map<LLUUID, LLUUID> asset_item_map;

    // a way to resolve situations where source and target have different amount of faces
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    mClipboardParams["texture_all_tes"] = (num_tes != 1) || (LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool());
    for (S32 te = 0; te < num_tes; ++te)
    {
        if (node->isTESelected(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            if (tep)
            {
                LLSD te_data;

                // asLLSD() includes media
                te_data["te"] = tep->asLLSD();
                te_data["te"]["shiny"] = tep->getShiny();
                te_data["te"]["bumpmap"] = tep->getBumpmap();
                te_data["te"]["bumpshiny"] = tep->getBumpShiny();
                te_data["te"]["bumpfullbright"] = tep->getBumpShinyFullbright();

                if (te_data["te"].has("imageid"))
                {
                    LLUUID item_id;
                    LLUUID id = te_data["te"]["imageid"].asUUID();
                    bool from_library = get_is_predefined_texture(id);
                    bool full_perm = from_library;

                    if (!full_perm
                        && objectp->permCopy()
                        && objectp->permTransfer()
                        && objectp->permModify())
                    {
                        // If agent created this object and nothing is limiting permissions, mark as full perm
                        // If agent was granted permission to edit objects owned and created by somebody else, mark full perm
                        // This check is not perfect since we can't figure out whom textures belong to so this ended up restrictive
                        std::string creator_app_link;
                        LLUUID creator_id;
                        LLSelectMgr::getInstance()->selectGetCreator(creator_id, creator_app_link);
                        full_perm = objectp->mOwnerID == creator_id;
                    }

                    if (id.notNull() && !full_perm)
                    {
                        std::map<LLUUID, LLUUID>::iterator iter = asset_item_map.find(id);
                        if (iter != asset_item_map.end())
                        {
                            item_id = iter->second;
                        }
                        else
                        {
                            // What this does is simply searches inventory for item with same asset id,
                            // as result it is Hightly unreliable, leaves little control to user, borderline hack
                            // but there are little options to preserve permissions - multiple inventory
                            // items might reference same asset and inventory search is expensive.
                            bool no_transfer = false;
                            if (objectp->getInventoryItemByAsset(id))
                            {
                                no_transfer = !objectp->getInventoryItemByAsset(id)->getIsFullPerm();
                            }
                            item_id = get_copy_free_item_by_asset_id(id, no_transfer);
                            // record value to avoid repeating inventory search when possible
                            asset_item_map[id] = item_id;
                        }
                    }

                    if (item_id.notNull() && gInventory.isObjectDescendentOf(item_id, gInventory.getLibraryRootFolderID()))
                    {
                        full_perm = true;
                        from_library = true;
                    }

                    {
                        te_data["te"]["itemfullperm"] = full_perm;
                        te_data["te"]["fromlibrary"] = from_library; 

                        // If full permission object, texture is free to copy,
                        // but otherwise we need to check inventory and extract permissions
                        //
                        // Normally we care only about restrictions for current user and objects
                        // don't inherit any 'next owner' permissions from texture, so there is
                        // no need to record item id if full_perm==true
                        if (!full_perm && !from_library && item_id.notNull())
                        {
                            LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                            if (itemp)
                            {
                                LLPermissions item_permissions = itemp->getPermissions();
                                if (item_permissions.allowOperationBy(PERM_COPY,
                                    gAgent.getID(),
                                    gAgent.getGroupID()))
                                {
                                    te_data["te"]["imageitemid"] = item_id;
                                    te_data["te"]["itemfullperm"] = itemp->getIsFullPerm();
                                    if (!itemp->isFinished())
                                    {
                                        // needed for dropTextureAllFaces
                                        LLInventoryModelBackgroundFetch::instance().start(item_id, false);
                                    }
                                }
                            }
                        }
                    }
                }

                LLMaterialPtr material_ptr = tep->getMaterialParams();
                if (!material_ptr.isNull())
                {
                    LLSD mat_data;

                    mat_data["NormMap"] = material_ptr->getNormalID();
                    mat_data["SpecMap"] = material_ptr->getSpecularID();

                    mat_data["NormRepX"] = material_ptr->getNormalRepeatX();
                    mat_data["NormRepY"] = material_ptr->getNormalRepeatY();
                    mat_data["NormOffX"] = material_ptr->getNormalOffsetX();
                    mat_data["NormOffY"] = material_ptr->getNormalOffsetY();
                    mat_data["NormRot"] = material_ptr->getNormalRotation();

                    mat_data["SpecRepX"] = material_ptr->getSpecularRepeatX();
                    mat_data["SpecRepY"] = material_ptr->getSpecularRepeatY();
                    mat_data["SpecOffX"] = material_ptr->getSpecularOffsetX();
                    mat_data["SpecOffY"] = material_ptr->getSpecularOffsetY();
                    mat_data["SpecRot"] = material_ptr->getSpecularRotation();

                    mat_data["SpecColor"] = material_ptr->getSpecularLightColor().getValue();
                    mat_data["SpecExp"] = material_ptr->getSpecularLightExponent();
                    mat_data["EnvIntensity"] = material_ptr->getEnvironmentIntensity();
                    mat_data["AlphaMaskCutoff"] = material_ptr->getAlphaMaskCutoff();
                    mat_data["DiffuseAlphaMode"] = material_ptr->getDiffuseAlphaMode();

                    // Replace no-copy textures, destination texture will get used instead if available
                    if (mat_data.has("NormMap"))
                    {
                        LLUUID id = mat_data["NormMap"].asUUID();
                        if (id.notNull() && !get_can_copy_texture(id))
                        {
                            mat_data["NormMap"] = LLUUID(gSavedSettings.getString("DefaultObjectTexture"));
                            mat_data["NormMapNoCopy"] = true;
                        }

                    }
                    if (mat_data.has("SpecMap"))
                    {
                        LLUUID id = mat_data["SpecMap"].asUUID();
                        if (id.notNull() && !get_can_copy_texture(id))
                        {
                            mat_data["SpecMap"] = LLUUID(gSavedSettings.getString("DefaultObjectTexture"));
                            mat_data["SpecMapNoCopy"] = true;
                        }

                    }

                    te_data["material"] = mat_data;
                }

                mClipboardParams["texture"].append(te_data);
            }
        }
    }
}

void LLPanelFace::onPasteTexture()
{
    if (!mClipboardParams.has("texture"))
    {
        return;
    }

    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        // not supposed to happen
        LL_WARNS() << "Failed to paste texture due to missing or wrong selection" << LL_ENDL;
        return;
    }

    bool face_selection_mode = LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
    LLSD &clipboard = mClipboardParams["texture"]; // array
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    S32 compare_tes = num_tes;

    if (face_selection_mode)
    {
        compare_tes = 0;
        for (S32 te = 0; te < num_tes; ++te)
        {
            if (node->isTESelected(te))
            {
                compare_tes++;
            }
        }
    }

    // we can copy if single face was copied in edit face mode or if face count matches
    if (!((clipboard.size() == 1) && mClipboardParams["texture_all_tes"].asBoolean()) 
        && compare_tes != clipboard.size())
    {
        LLSD notif_args;
        if (face_selection_mode)
        {
            static std::string reason = getString("paste_error_face_selection_mismatch");
            notif_args["REASON"] = reason;
        }
        else
        {
            static std::string reason = getString("paste_error_object_face_count_mismatch");
            notif_args["REASON"] = reason;
        }
        LLNotificationsUtil::add("FacePasteFailed", notif_args);
        return;
    }

    bool full_perm_object = true;
    LLSD::array_const_iterator iter = clipboard.beginArray();
    LLSD::array_const_iterator end = clipboard.endArray();
    for (; iter != end; ++iter)
    {
        const LLSD& te_data = *iter;
        if (te_data.has("te") && te_data["te"].has("imageid"))
        {
            bool full_perm = te_data["te"].has("itemfullperm") && te_data["te"]["itemfullperm"].asBoolean();
            full_perm_object &= full_perm;
            if (!full_perm)
            {
                if (te_data["te"].has("imageitemid"))
                {
                    LLUUID item_id = te_data["te"]["imageitemid"].asUUID();
                    if (item_id.notNull())
                    {
                        LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                        if (!itemp)
                        {
                            // image might be in object's inventory, but it can be not up to date
                            LLSD notif_args;
                            static std::string reason = getString("paste_error_inventory_not_found");
                            notif_args["REASON"] = reason;
                            LLNotificationsUtil::add("FacePasteFailed", notif_args);
                            return;
                        }
                    }
                }
                else
                {
                    // Item was not found on 'copy' stage
                    // Since this happened at copy, might be better to either show this
                    // at copy stage or to drop clipboard here
                    LLSD notif_args;
                    static std::string reason = getString("paste_error_inventory_not_found");
                    notif_args["REASON"] = reason;
                    LLNotificationsUtil::add("FacePasteFailed", notif_args);
                    return;
                }
            }
        }
    }

    if (!full_perm_object)
    {
        LLNotificationsUtil::add("FacePasteTexturePermissions");
    }

    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();

    LLPanelFacePasteTexFunctor paste_func(this, PASTE_TEXTURE);
    selected_objects->applyToTEs(&paste_func);

    LLPanelFaceUpdateFunctor sendfunc(true);
    selected_objects->applyToObjects(&sendfunc);

    LLPanelFaceNavigateHomeFunctor navigate_home_func;
    selected_objects->applyToTEs(&navigate_home_func);
}

void LLPanelFace::onPasteTexture(LLViewerObject* objectp, S32 te)
{
    LLSD te_data;
    LLSD &clipboard = mClipboardParams["texture"]; // array
    if ((clipboard.size() == 1) && mClipboardParams["texture_all_tes"].asBoolean())
    {
        te_data = *(clipboard.beginArray());
    }
    else if (clipboard[te])
    {
        te_data = clipboard[te];
    }
    else
    {
        return;
    }

    LLTextureEntry* tep = objectp->getTE(te);
    if (tep)
    {
        if (te_data.has("te"))
        {
            // Texture
            bool full_perm = te_data["te"].has("itemfullperm") && te_data["te"]["itemfullperm"].asBoolean();
            bool from_library = te_data["te"].has("fromlibrary") && te_data["te"]["fromlibrary"].asBoolean();
            if (te_data["te"].has("imageid"))
            {
                const LLUUID& imageid = te_data["te"]["imageid"].asUUID(); //texture or asset id
                LLViewerInventoryItem* itemp_res = NULL;

                if (te_data["te"].has("imageitemid"))
                {
                    LLUUID item_id = te_data["te"]["imageitemid"].asUUID();
                    if (item_id.notNull())
                    {
                        LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                        if (itemp && itemp->isFinished())
                        {
                            // dropTextureAllFaces will fail if incomplete
                            itemp_res = itemp;
                        }
                        else
                        {
                            // Theoretically shouldn't happend, but if it does happen, we
                            // might need to add a notification to user that paste will fail
                            // since inventory isn't fully loaded
                            LL_WARNS() << "Item " << item_id << " is incomplete, paste might fail silently." << LL_ENDL;
                        }
                    }
                }
                // for case when item got removed from inventory after we pressed 'copy'
                // or texture got pasted into previous object
                if (!itemp_res && !full_perm)
                {
                    // Due to checks for imageitemid in LLPanelFace::onPasteTexture() this should no longer be reachable.
                    LL_INFOS() << "Item " << te_data["te"]["imageitemid"].asUUID() << " no longer in inventory." << LL_ENDL;
                    // Todo: fix this, we are often searching same texture multiple times (equal to number of faces)
                    // Perhaps just mPanelFace->onPasteTexture(objectp, te, &asset_to_item_id_map); ? Not pretty, but will work
                    LLViewerInventoryCategory::cat_array_t cats;
                    LLViewerInventoryItem::item_array_t items;
                    LLAssetIDMatches asset_id_matches(imageid);
                    gInventory.collectDescendentsIf(LLUUID::null,
                        cats,
                        items,
                        LLInventoryModel::INCLUDE_TRASH,
                        asset_id_matches);

                    // Extremely unreliable and perfomance unfriendly.
                    // But we need this to check permissions and it is how texture control finds items
                    for (S32 i = 0; i < items.size(); i++)
                    {
                        LLViewerInventoryItem* itemp = items[i];
                        if (itemp && itemp->isFinished())
                        {
                            // dropTextureAllFaces will fail if incomplete
                            LLPermissions item_permissions = itemp->getPermissions();
                            if (item_permissions.allowOperationBy(PERM_COPY,
                                gAgent.getID(),
                                gAgent.getGroupID()))
                            {
                                itemp_res = itemp;
                                break; // first match
                            }
                        }
                    }
                }

                if (itemp_res)
                {
                    if (te == -1) // all faces
                    {
                        LLToolDragAndDrop::dropTextureAllFaces(objectp,
                            itemp_res,
                            from_library ? LLToolDragAndDrop::SOURCE_LIBRARY : LLToolDragAndDrop::SOURCE_AGENT,
                            LLUUID::null);
                    }
                    else // one face
                    {
                        LLToolDragAndDrop::dropTextureOneFace(objectp,
                            te,
                            itemp_res,
                            from_library ? LLToolDragAndDrop::SOURCE_LIBRARY : LLToolDragAndDrop::SOURCE_AGENT,
                            LLUUID::null,
                            0);
                    }
                }
                // not an inventory item or no complete items
                else if (full_perm)
                {
                    // Either library, local or existed as fullperm when user made a copy
                    LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(imageid, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
                    objectp->setTEImage(U8(te), image);
                }
            }

            if (te_data["te"].has("bumpmap"))
            {
                objectp->setTEBumpmap(te, (U8)te_data["te"]["bumpmap"].asInteger());
            }
            if (te_data["te"].has("bumpshiny"))
            {
                objectp->setTEBumpShiny(te, (U8)te_data["te"]["bumpshiny"].asInteger());
            }
            if (te_data["te"].has("bumpfullbright"))
            {
                objectp->setTEBumpShinyFullbright(te, (U8)te_data["te"]["bumpfullbright"].asInteger());
            }

            // Texture map
            if (te_data["te"].has("scales") && te_data["te"].has("scalet"))
            {
                objectp->setTEScale(te, (F32)te_data["te"]["scales"].asReal(), (F32)te_data["te"]["scalet"].asReal());
            }
            if (te_data["te"].has("offsets") && te_data["te"].has("offsett"))
            {
                objectp->setTEOffset(te, (F32)te_data["te"]["offsets"].asReal(), (F32)te_data["te"]["offsett"].asReal());
            }
            if (te_data["te"].has("imagerot"))
            {
                objectp->setTERotation(te, (F32)te_data["te"]["imagerot"].asReal());
            }

            // Media
            if (te_data["te"].has("media_flags"))
            {
                U8 media_flags = te_data["te"]["media_flags"].asInteger();
                objectp->setTEMediaFlags(te, media_flags);
                LLVOVolume *vo = dynamic_cast<LLVOVolume*>(objectp);
                if (vo && te_data["te"].has(LLTextureEntry::TEXTURE_MEDIA_DATA_KEY))
                {
                    vo->syncMediaData(te, te_data["te"][LLTextureEntry::TEXTURE_MEDIA_DATA_KEY], true/*merge*/, true/*ignore_agent*/);
                }
            }
            else
            {
                // Keep media flags on destination unchanged
            }
        }

        if (te_data.has("material"))
        {
            LLUUID object_id = objectp->getID();

            LLSelectedTEMaterial::setAlphaMaskCutoff(this, (U8)te_data["material"]["SpecRot"].asInteger(), te, object_id);

            // Normal
            // Replace placeholders with target's
            if (te_data["material"].has("NormMapNoCopy"))
            {
                LLMaterialPtr material = tep->getMaterialParams();
                if (material.notNull())
                {
                    LLUUID id = material->getNormalID();
                    if (id.notNull())
                    {
                        te_data["material"]["NormMap"] = id;
                    }
                }
            }
            LLSelectedTEMaterial::setNormalID(this, te_data["material"]["NormMap"].asUUID(), te, object_id);
            LLSelectedTEMaterial::setNormalRepeatX(this, (F32)te_data["material"]["NormRepX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalRepeatY(this, (F32)te_data["material"]["NormRepY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalOffsetX(this, (F32)te_data["material"]["NormOffX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalOffsetY(this, (F32)te_data["material"]["NormOffY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalRotation(this, (F32)te_data["material"]["NormRot"].asReal(), te, object_id);

            // Specular
                // Replace placeholders with target's
            if (te_data["material"].has("SpecMapNoCopy"))
            {
                LLMaterialPtr material = tep->getMaterialParams();
                if (material.notNull())
                {
                    LLUUID id = material->getSpecularID();
                    if (id.notNull())
                    {
                        te_data["material"]["SpecMap"] = id;
                    }
                }
            }
            LLSelectedTEMaterial::setSpecularID(this, te_data["material"]["SpecMap"].asUUID(), te, object_id);
            LLSelectedTEMaterial::setSpecularRepeatX(this, (F32)te_data["material"]["SpecRepX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularRepeatY(this, (F32)te_data["material"]["SpecRepY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularOffsetX(this, (F32)te_data["material"]["SpecOffX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularOffsetY(this, (F32)te_data["material"]["SpecOffY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularRotation(this, (F32)te_data["material"]["SpecRot"].asReal(), te, object_id);
            LLColor4 spec_color(te_data["material"]["SpecColor"]);
            LLSelectedTEMaterial::setSpecularLightColor(this, spec_color, te);
            LLSelectedTEMaterial::setSpecularLightExponent(this, (U8)te_data["material"]["SpecExp"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setEnvironmentIntensity(this, (U8)te_data["material"]["EnvIntensity"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setDiffuseAlphaMode(this, (U8)te_data["material"]["SpecRot"].asInteger(), te, object_id);
            if (te_data.has("te") && te_data["te"].has("shiny"))
            {
                objectp->setTEShiny(te, (U8)te_data["te"]["shiny"].asInteger());
            }
        }
    }
}

void LLPanelFace::menuDoToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste
    if (command == "color_paste")
    {
        onPasteColor();
    }
    else if (command == "texture_paste")
    {
        onPasteTexture();
    }
    // copy
    else if (command == "color_copy")
    {
        onCopyColor();
    }
    else if (command == "texture_copy")
    {
        onCopyTexture();
    }
}

bool LLPanelFace::menuEnableItem(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste options
    if (command == "color_paste")
    {
        return mClipboardParams.has("color");
    }
    else if (command == "texture_paste")
    {
        return mClipboardParams.has("texture");
    }
    return false;
}


// static
void LLPanelFace::onCommitPlanarAlign(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*)userdata;
	self->getState();
	self->sendTextureInfo();
}

void LLPanelFace::onTextureSelectionChanged(LLInventoryItem* itemp)
{
	// _LL_DEBUGS("Materials") << "item asset " << itemp->getAssetUUID() << LL_ENDL;
	//BD
	U32 mattype = mRadioMaterialType->getSelectedIndex();
	std::string which_control = "texture control";
	switch (mattype)
	{
	case MATTYPE_SPECULAR:
		which_control = "shinytexture control";
		break;
	case MATTYPE_NORMAL:
		which_control = "bumpytexture control";
		break;
		// no default needed
	}
	// _LL_DEBUGS("Materials") << "control " << which_control << LL_ENDL;
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(which_control);
	if (texture_ctrl)
	{
		LLUUID obj_owner_id;
		std::string obj_owner_name;
		LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

		LLSaleInfo sale_info;
		LLSelectMgr::instance().selectGetSaleInfo(sale_info);

		bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this texture?
		bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this texture?
		bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply texture belong to the agent?
		bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply texture not for sale?

		if (can_copy && can_transfer)
		{
			texture_ctrl->setCanApply(true, true);
			return;
		}

		// if texture has (no-transfer) attribute it can be applied only for object which we own and is not for sale
		texture_ctrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

		if (gSavedSettings.getBOOL("TextureLivePreview"))
		{
			LLNotificationsUtil::add("LivePreviewUnavailable");
		}
	}
}

bool LLPanelFace::isIdenticalPlanarTexgen()
{
	LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
	bool identical_texgen = false;
	LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
	return (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
}

void LLPanelFace::LLSelectedTE::getFace(LLFace*& face_to_return, bool& identical_face)
{
	struct LLSelectedTEGetFace : public LLSelectedTEGetFunctor<LLFace *>
	{
		LLFace* get(LLViewerObject* object, S32 te)
		{
			return (object->mDrawable) ? object->mDrawable->getFace(te) : NULL;
		}
	} get_te_face_func;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_te_face_func, face_to_return, false, (LLFace*)nullptr);
}

void LLPanelFace::LLSelectedTE::getImageFormat(LLGLenum& image_format_to_return, bool& identical_face)
{
	LLGLenum image_format;
	struct LLSelectedTEGetImageFormat : public LLSelectedTEGetFunctor<LLGLenum>
	{
		LLGLenum get(LLViewerObject* object, S32 te_index)
		{
			LLViewerTexture* image = object->getTEImage(te_index);
			return image ? image->getPrimaryFormat() : GL_RGB;
		}
	} get_glenum;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_glenum, image_format);
	image_format_to_return = image_format;
}

void LLPanelFace::LLSelectedTE::getTexId(LLUUID& id, bool& identical)
{
	struct LLSelectedTEGetTexId : public LLSelectedTEGetFunctor<LLUUID>
	{
		LLUUID get(LLViewerObject* object, S32 te_index)
		{
			LLUUID id;
			LLViewerTexture* image = object->getTEImage(te_index);
			if (image)
			{
				id = image->getID();
			}

			if (!id.isNull() && LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				LLTextureEntry *te = object->getTE(te_index);
				if (te)
				{
					LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID(), TEX_LIST_STANDARD) : NULL;
					if (!tex)
					{
						tex = LLViewerFetchedTexture::sDefaultImagep;
					}
					if (tex)
					{
						id = tex->getID();
					}
				}
			}
			return id;
		}
	} func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func, id);
}

void LLPanelFace::LLSelectedTEMaterial::getCurrent(LLMaterialPtr& material_ptr, bool& identical_material)
{
	struct MaterialFunctor : public LLSelectedTEGetFunctor<LLMaterialPtr>
	{
		LLMaterialPtr get(LLViewerObject* object, S32 te_index)
		{
			return object->getTE(te_index)->getMaterialParams();
		}
	} func;
	identical_material = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func, material_ptr);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxSpecularRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxSpecRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getSpecularRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}
			return llmax(repeats_s, repeats_t);
		}

	} max_spec_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&max_spec_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxNormalRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxNormRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getNormalRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}
			return llmax(repeats_s, repeats_t);
		}

	} max_norm_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&max_norm_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(U8& diffuse_alpha_mode, bool& identical, bool diffuse_texture_has_alpha)
{
	struct LLSelectedTEGetDiffuseAlphaMode : public LLSelectedTEGetFunctor<U8>
	{
		LLSelectedTEGetDiffuseAlphaMode() : _isAlpha(false) {}
		LLSelectedTEGetDiffuseAlphaMode(bool diffuse_texture_has_alpha) : _isAlpha(diffuse_texture_has_alpha) {}
		virtual ~LLSelectedTEGetDiffuseAlphaMode() {}

		U8 get(LLViewerObject* object, S32 face)
		{
			U8 diffuse_mode = _isAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			LLTextureEntry* tep = object->getTE(face);
			if (tep)
			{
				LLMaterial* mat = tep->getMaterialParams().get();
				if (mat)
				{
					diffuse_mode = mat->getDiffuseAlphaMode();
				}
			}

			return diffuse_mode;
		}
		bool _isAlpha; // whether or not the diffuse texture selected contains alpha information
	} get_diff_mode(diffuse_texture_has_alpha);
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_diff_mode, diffuse_alpha_mode);
}

void LLPanelFace::LLSelectedTE::getObjectScaleS(F32& scale_s, bool& identical)
{
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[s_axis];
		}

	} scale_s_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&scale_s_func, scale_s);
}

void LLPanelFace::LLSelectedTE::getObjectScaleT(F32& scale_t, bool& identical)
{
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[t_axis];
		}

	} scale_t_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&scale_t_func, scale_t);
}

void LLPanelFace::LLSelectedTE::getMaxDiffuseRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxDiffuseRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			F32 repeats_s = object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
			F32 repeats_t = object->getTE(face)->mScaleT / object->getScale().mV[t_axis];
			return llmax(repeats_s, repeats_t);
		}

	} max_diff_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&max_diff_repeats_func, repeats);
}

LLRender::eTexIndex LLPanelFace::getTextureChannelToEdit()
{
	//BD
	LLRender::eTexIndex channel_to_edit = (LLRender::eTexIndex)mRadioMaterialType->getSelectedIndex();

	channel_to_edit = (channel_to_edit == LLRender::NORMAL_MAP) ? (getCurrentNormalMap().isNull() ? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	channel_to_edit = (channel_to_edit == LLRender::SPECULAR_MAP) ? (getCurrentSpecularMap().isNull() ? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	return channel_to_edit;
}