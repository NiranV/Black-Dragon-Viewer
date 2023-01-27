/** 
 * @file llfloatertools.cpp
 * @brief The edit tools, including move, position, land, etc.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloatertools.h"

#include "llfontgl.h"
#include "llcoord.h"
//#include "llgl.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llerror.h"
#include "llfloaterbuildoptions.h"
#include "llfloatermediasettings.h"
#include "llfloateropenobject.h"
#include "llfloaterobjectweights.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llmediaentry.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpanelcontents.h"
#include "llpanelface.h"
#include "llpanelland.h"
#include "llpanelobjectinventory.h"
#include "llpanelobject.h"
#include "llpanelvolume.h"
#include "llpanelpermissions.h"
#include "llparcel.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llslider.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltoolbrush.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolgrab.h"
#include "lltoolindividual.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolpipette.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewerregion.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llmeshrepository.h"

//BD - Qarl's Aligning Tool
#include "qtoolalign.h"

// Globals
LLFloaterTools *gFloaterTools = NULL;
bool LLFloaterTools::sShowObjectCost = true;
bool LLFloaterTools::sPreviousFocusOnAvatar = false;

const std::string PANEL_NAMES[LLFloaterTools::PANEL_COUNT] =
{
	std::string("General"), 	// PANEL_GENERAL,
	std::string("Object"), 	// PANEL_OBJECT,
	std::string("Features"),	// PANEL_FEATURES,
	std::string("Texture"),	// PANEL_FACE,
	std::string("Content"),	// PANEL_CONTENTS,
};


// Local prototypes
void commit_grid_mode(LLUICtrl *ctrl);
void commit_select_component(void *data);
void click_show_more(void*);
void click_popup_info(void*);
void click_popup_done(void*);
void click_popup_minimize(void*);
void commit_slider_dozer_force(LLUICtrl *);
void click_apply_to_selection(void*);
void commit_radio_group_focus(LLUICtrl* ctrl);
void commit_radio_group_move(LLUICtrl* ctrl);
void commit_radio_group_edit(LLUICtrl* ctrl);
void commit_radio_group_land(LLUICtrl* ctrl);
void commit_slider_zoom(LLUICtrl *ctrl);

/**
 * Class LLLandImpactsObserver
 *
 * An observer class to monitor parcel selection and update
 * the land impacts data from a parcel containing the selected object.
 */
class LLLandImpactsObserver : public LLParcelObserver
{
public:
	virtual void changed()
	{
		LLFloaterTools* tools_floater = LLFloaterReg::getTypedInstance<LLFloaterTools>("build");
		if(tools_floater)
		{
			tools_floater->updateLandImpacts();
		}
	}
};

//static
void*	LLFloaterTools::createPanelPermissions(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelPermissions = new LLPanelPermissions();
	return floater->mPanelPermissions;
}
//static
void*	LLFloaterTools::createPanelObject(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelObject = new LLPanelObject();
	return floater->mPanelObject;
}

//static
void*	LLFloaterTools::createPanelVolume(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelVolume = new LLPanelVolume();
	return floater->mPanelVolume;
}

//static
void*	LLFloaterTools::createPanelFace(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelFace = new LLPanelFace();
	return floater->mPanelFace;
}

//static
void*	LLFloaterTools::createPanelContents(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents = new LLPanelContents();
	return floater->mPanelContents;
}

//static
void*	LLFloaterTools::createPanelLandInfo(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelLandInfo = new LLPanelLandInfo();
	return floater->mPanelLandInfo;
}

static	const std::string	toolNames[]={
	"ToolCube",
	"ToolPrism",
	"ToolPyramid",
	"ToolTetrahedron",
	"ToolCylinder",
	"ToolHemiCylinder",
	"ToolCone",
	"ToolHemiCone",
	"ToolSphere",
	"ToolHemiSphere",
	"ToolTorus",
	"ToolTube",
	"ToolRing",
	"ToolTree",
	"ToolGrass"};
LLPCode toolData[]={
	LL_PCODE_CUBE,
	LL_PCODE_PRISM,
	LL_PCODE_PYRAMID,
	LL_PCODE_TETRAHEDRON,
	LL_PCODE_CYLINDER,
	LL_PCODE_CYLINDER_HEMI,
	LL_PCODE_CONE,
	LL_PCODE_CONE_HEMI,
	LL_PCODE_SPHERE,
	LL_PCODE_SPHERE_HEMI,
	LL_PCODE_TORUS,
	LLViewerObject::LL_VO_SQUARE_TORUS,
	LLViewerObject::LL_VO_TRIANGLE_TORUS,
	LL_PCODE_LEGACY_TREE,
	LL_PCODE_LEGACY_GRASS};

BOOL	LLFloaterTools::postBuild()
{	
	// Since we constantly show and hide this during drags, don't
	// make sounds on visibility changes.
	setSoundFlags(LLView::SILENT);

	getDragHandle()->setEnabled( !gSavedSettings.getBOOL("ToolboxAutoMove") );

	LLRect rect;
	mBtnFocus			= getChild<LLButton>("button focus");//btn;
	mBtnMove			= getChild<LLButton>("button move");
	mBtnEdit			= getChild<LLButton>("button edit");
	mBtnCreate			= getChild<LLButton>("button create");
	mBtnLand			= getChild<LLButton>("button land" );
	mRadioGroupFocus	= getChild<LLRadioGroup>("focus_radio_group");
	mRadioGroupMove		= getChild<LLRadioGroup>("move_radio_group");
	mRadioGroupEdit		= getChild<LLRadioGroup>("edit_radio_group");
	mBtnGridOptions		= getChild<LLButton>("Options...");
	mBtnLink			= getChild<LLButton>("link_btn");
	mBtnUnlink			= getChild<LLButton>("unlink_btn");
	
	//BD
	mFocusPanel			= getChild<LLPanel>("focus_panel");
	mGrabPanel			= getChild<LLPanel>("grab_panel");
	mEditPanel			= getChild<LLPanel>("select_panel");
	mCreatePanel		= getChild<LLPanel>("create_panel");
	mLandPanel			= getChild<LLPanel>("land_panel");

	mCheckSelectIndividual	= getChild<LLCheckBoxCtrl>("checkbox edit linked parts");	
	mCheckSnapToGrid		= getChild<LLUICtrl>("checkbox snap to grid");
	mCheckStretchUniform	= getChild<LLCheckBoxCtrl>("checkbox uniform");
	mCheckStretchTexture	= getChild<LLCheckBoxCtrl>("checkbox stretch textures");

	//
	// Create Buttons
	//

	for(size_t t=0; t<LL_ARRAY_SIZE(toolNames); ++t)
	{
		LLButton *found = getChild<LLButton>(toolNames[t]);
		if(found)
		{
			found->setClickedCallback(boost::bind(&LLFloaterTools::setObjectType, toolData[t]));
			mButtons.push_back( found );
		}else{
			LL_WARNS() << "Tool button not found! DOA Pending." << LL_ENDL;
		}
	}
	mCheckCopySelection		= getChild<LLCheckBoxCtrl>("checkbox copy selection");
	mCheckSticky			= getChild<LLCheckBoxCtrl>("checkbox sticky");
	mCheckCopyCenters		= getChild<LLCheckBoxCtrl>("checkbox copy centers");
	mCheckCopyRotates		= getChild<LLCheckBoxCtrl>("checkbox copy rotates");

	mRadioGroupLand			= getChild<LLRadioGroup>("land_radio_group");
	mBtnApplyToSelection	= getChild<LLButton>("button apply to selection");
	mSliderDozerSize		= getChild<LLSlider>("slider brush size");
	mSliderDozerForce		= getChild<LLSlider>("slider force");

	mZoomSlider				= getChild<LLSlider>("slider zoom");

	mSliderDozerSize->setValue(gSavedSettings.getF32("LandBrushSize"));
	// the setting stores the actual force multiplier, but the slider is logarithmic, so we convert here
	mSliderDozerForce->setValue(log10(gSavedSettings.getF32("LandBrushForce")));

	mCostTextBorder			= getChild<LLViewBorder>("cost_text_border");
	mSelectionCount			= getChild<LLTextBox>("selection_count");
	mRemainingCapacity		= getChild<LLTextBox>("remaining_capacity");
	mNothingSelected		= getChild<LLTextBox>("selection_empty");

	mNextElement			= getChild<LLButton>("next_part_btn");
	mPrevElement			= getChild<LLButton>("prev_part_btn");

	mMediaInfo				= getChild<LLTextBox>("media_info");
	mMediaAdd				= getChild<LLButton>("add_media");
	mMediaDelete			= getChild<LLButton>("delete_media");
	mMediaAlign				= getChild<LLButton>("align_media");

	mTab					= getChild<LLTabContainer>("Object Info Tabs");

	mStatusText["rotate"] = getString("status_rotate");
	mStatusText["scale"] = getString("status_scale");
	mStatusText["move"] = getString("status_move");
	mStatusText["modifyland"] = getString("status_modifyland");
	mStatusText["camera"] = getString("status_camera");
	mStatusText["grab"] = getString("status_grab");
	mStatusText["place"] = getString("status_place");
	mStatusText["selectland"] = getString("status_selectland");

	sShowObjectCost = gSavedSettings.getBOOL("ShowObjectRenderingCost");
	
	return TRUE;
}

// Create the popupview with a dummy center.  It will be moved into place
// during LLViewerWindow's per-frame hover processing.
LLFloaterTools::LLFloaterTools(const LLSD& key)
:	LLFloater(key),
	mBtnFocus(NULL),
	mBtnMove(NULL),
	mBtnEdit(NULL),
	mBtnCreate(NULL),
	mBtnLand(NULL),
	//BD
	//mTextStatus(NULL),

	mRadioGroupFocus(NULL),
	mRadioGroupMove(NULL),
	mRadioGroupEdit(NULL),

	mCheckSelectIndividual(NULL),

	mCheckSnapToGrid(NULL),
	mBtnGridOptions(NULL),
	//mComboGridMode(NULL),
	mCheckStretchUniform(NULL),
	mCheckStretchTexture(NULL),

	mBtnRotateLeft(NULL),
	mBtnRotateReset(NULL),
	mBtnRotateRight(NULL),

	mBtnLink(NULL),
	mBtnUnlink(NULL),

	//BD
	mFocusPanel(NULL),
	mGrabPanel(NULL),
	mEditPanel(NULL),
	mCreatePanel(NULL),
	mLandPanel(NULL),

	mZoomSlider(NULL),

	mBtnDelete(NULL),
	mBtnDuplicate(NULL),
	mBtnDuplicateInPlace(NULL),

	mCheckSticky(NULL),
	mCheckCopySelection(NULL),
	mCheckCopyCenters(NULL),
	mCheckCopyRotates(NULL),
	mRadioGroupLand(NULL),
	mSliderDozerSize(NULL),
	mSliderDozerForce(NULL),
	mBtnApplyToSelection(NULL),

	mTab(NULL),
	mPanelPermissions(NULL),
	mPanelObject(NULL),
	mPanelVolume(NULL),
	mPanelContents(NULL),
	mPanelFace(NULL),
	mPanelLandInfo(NULL),

	mCostTextBorder(NULL),
	mTabLand(NULL),

	mSelectionCount(NULL),
	mRemainingCapacity(NULL),
	mNothingSelected(NULL),
	mMediaInfo(NULL),

	//BD - Next / Previous Element
	mNextElement(NULL),
	mPrevElement(NULL),

	mMediaAdd(NULL),
	mMediaDelete(NULL),
	mMediaAlign(NULL),

	mLandImpactsObserver(NULL),

	mDirty(TRUE),
	mHasSelection(TRUE)
{
	gFloaterTools = this;

	setAutoFocus(FALSE);
	mFactoryMap["General"] = LLCallbackMap(createPanelPermissions, this);		//LLPanelPermissions
	mFactoryMap["Object"] = LLCallbackMap(createPanelObject, this);				//LLPanelObject
	mFactoryMap["Features"] = LLCallbackMap(createPanelVolume, this);			//LLPanelVolume
	mFactoryMap["Texture"] = LLCallbackMap(createPanelFace, this);				//LLPanelFace
	mFactoryMap["Contents"] = LLCallbackMap(createPanelContents, this);			//LLPanelContents
	mFactoryMap["land info panel"] = LLCallbackMap(createPanelLandInfo, this);	//LLPanelLandInfo
	
	mCommitCallbackRegistrar.add("BuildTool.setTool",			boost::bind(&LLFloaterTools::setTool,this, _2));
	mCommitCallbackRegistrar.add("BuildTool.commitZoom",		boost::bind(&commit_slider_zoom, _1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioFocus",	boost::bind(&commit_radio_group_focus, _1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioMove",	boost::bind(&commit_radio_group_move,_1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioEdit",	boost::bind(&commit_radio_group_edit,_1));

	mCommitCallbackRegistrar.add("BuildTool.selectComponent",	boost::bind(&commit_select_component, this));
	mCommitCallbackRegistrar.add("BuildTool.gridOptions",		boost::bind(&LLFloaterTools::onClickGridOptions,this));
	mCommitCallbackRegistrar.add("BuildTool.applyToSelection",	boost::bind(&click_apply_to_selection, this));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioLand",	boost::bind(&commit_radio_group_land,_1));
	mCommitCallbackRegistrar.add("BuildTool.LandBrushForce",	boost::bind(&commit_slider_dozer_force,_1));

	mCommitCallbackRegistrar.add("BuildTool.NextPart",			boost::bind(&LLFloaterTools::onSelectElement, this, _1, _2));
	mCommitCallbackRegistrar.add("BuildTool.PrevPart",			boost::bind(&LLFloaterTools::onSelectElement, this, _1, _2));

	mCommitCallbackRegistrar.add("BuildTool.LinkObjects",		boost::bind(&LLSelectMgr::linkObjects, LLSelectMgr::getInstance()));
	mCommitCallbackRegistrar.add("BuildTool.UnlinkObjects",		boost::bind(&LLSelectMgr::unlinkObjects, LLSelectMgr::getInstance()));

	mLandImpactsObserver = new LLLandImpactsObserver();
	LLViewerParcelMgr::getInstance()->addObserver(mLandImpactsObserver);
}

LLFloaterTools::~LLFloaterTools()
{
	// children automatically deleted
	gFloaterTools = NULL;

	LLViewerParcelMgr::getInstance()->removeObserver(mLandImpactsObserver);
	delete mLandImpactsObserver;
}

//BD
/*void LLFloaterTools::setStatusText(const std::string& text)
{
	std::map<std::string, std::string>::iterator iter = mStatusText.find(text);
	if (iter != mStatusText.end())
	{
		mTextStatus->setText(iter->second);
	}
	else
	{
		mTextStatus->setText(text);
	}
}*/

void LLFloaterTools::refresh()
{
	const S32 INFO_WIDTH = getRect().getWidth();
	const S32 INFO_HEIGHT = 384;

	LLRect object_info_rect(0, 0, INFO_WIDTH, -INFO_HEIGHT);
	LLSelectMgr* select_mgr = LLSelectMgr::getInstance();

	BOOL all_volume = select_mgr->selectionAllPCode( LL_PCODE_VOLUME );

	S32 idx_features = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FEATURES]);
	S32 idx_face = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FACE]);
	S32 idx_contents = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_CONTENTS]);
	S32 selected_index = mTab->getCurrentPanelIndex();

	if (!all_volume && (selected_index == idx_features || selected_index == idx_face ||
		selected_index == idx_contents))
	{
		mTab->selectFirstTab();
	}

	mTab->enableTabButton(idx_features, all_volume);
	mTab->enableTabButton(idx_face, all_volume);
	mTab->enableTabButton(idx_contents, all_volume);

	// Refresh object and prim count labels
	LLLocale locale(LLLocale::USER_LOCALE);
#if 0
	if (!gMeshRepo.meshRezEnabled())
	{		
		std::string obj_count_string;
		LLResMgr* res_mgr = LLResMgr::getInstance();
		res_mgr->getIntegerString(obj_count_string, LLSelectMgr::getInstance()->getSelection()->getRootObjectCount());
		mSelectionCount->setTextArg("[OBJ_COUNT]", obj_count_string);
		std::string prim_count_string;
		res_mgr->getIntegerString(prim_count_string, LLSelectMgr::getInstance()->getSelection()->getObjectCount());
		mSelectionCount->setTextArg("[PRIM_COUNT]", prim_count_string);

		// calculate selection rendering cost
		/*if (sShowObjectCost)
		{
			std::string prim_cost_string;
			S32 render_cost = select_mgr->getSelection()->getSelectedObjectRenderCost();
			res_mgr->getIntegerString(prim_cost_string, render_cost);
			getChild<LLUICtrl>("RenderingCost")->setTextArg("[COUNT]", prim_cost_string);
		}*/
		
		// disable the object and prim counts if nothing selected
		//bool have_selection = ! LLSelectMgr::getInstance()->getSelection()->isEmpty();
		//getChildView("obj_count")->setEnabled(have_selection);
		//getChildView("prim_count")->setEnabled(have_selection);
		//getChildView("RenderingCost")->setEnabled(have_selection && sShowObjectCost);
	}
	else
#endif
	{
		//BD - Selected Face / Link index.
		bool is_link_select = mCheckSelectIndividual->getValue().asBoolean();

		//LLObjectSelectionHandle selection = select_mgr->getSelection();
		F32 link_cost = mObjectSelection->getSelectedLinksetCost();
		S32 link_count = mObjectSelection->getRootObjectCount();

		//BD - Selected Face / Link index.
		S32 link_index = -1;
		S32 face_index = -1;
		bool is_face_select = LLToolMgr::getInstance()->getCurrentToolset()->getSelectedTool() == LLToolFace::getInstance();
		if (is_link_select || is_face_select)
		{
			link_count = mObjectSelection->getObjectCount();

			LLViewerObject* selected_object = mObjectSelection->getFirstObject();
			if (selected_object)
			{
				if (selected_object && selected_object->getRootEdit())
				{
					LLViewerObject::child_list_t children = selected_object->getRootEdit()->getChildren();
					children.push_front(selected_object->getRootEdit());	// need root in the list too

					S32 i = 0;
					bool selected = false;
					for (LLViewerObject::child_list_t::iterator iter = children.begin(); iter != children.end(); ++iter)
					{
						if ((*iter)->isSelected())
						{
							link_index = selected ? -2 : i;
							selected = true;
						}
						++i;
					}
				}

				if (is_face_select)
				{
					bool selected = false;
					for (S32 i = 0; i < selected_object->getNumTEs(); i++)
					{
						LLTextureEntry* te = selected_object->getTE(i);
						if (te && te->isSelected())
						{
							face_index = selected ? -2 : i;
							selected = true;
						}
					}
				}
			}
		}

		LLCrossParcelFunctor func;
		if (mObjectSelection->applyToRootObjects(&func, true))
		{
			// Selection crosses parcel bounds.
			// We don't display remaining land capacity in this case.
			const LLStringExplicit empty_str("");
			mRemainingCapacity->setTextArg("[CAPACITY_STRING]", empty_str);
		}
		else
		{
			LLViewerObject* selected_object = mObjectSelection->getFirstObject();
			if (selected_object)
			{
				// Select a parcel at the currently selected object's position.
				LLViewerParcelMgr::getInstance()->selectParcelAt(selected_object->getPositionGlobal());
			}
			else
			{
				LL_WARNS() << "Failed to get selected object" << LL_ENDL;
			}
		}

		LLStringUtil::format_map_t selection_args;
		selection_args["OBJ_COUNT"] = llformat("%.1d", link_count);
		selection_args["LAND_IMPACT"] = llformat("%.1d", (S32)link_cost);

		//BD - Selected Face / Link index.
		selection_args["FACE_IDX"] = llformat("%.1d", face_index);
		selection_args["LINK_IDX"] = llformat("%.1d", link_index);

		std::ostringstream selection_info;

		selection_info << getString(is_link_select ? "status_selectlinkcount" : "status_selectcount", selection_args);

		//BD - Selected Face / Link index.
		if (link_index >= 0)
			selection_info << getString("status_selectlink", selection_args);

		if (face_index >= 0)
			selection_info << getString("status_selectface", selection_args);

		mSelectionCount->setText(selection_info.str());

		bool have_selection = !mObjectSelection->isEmpty();
		mSelectionCount->setVisible(have_selection);
		mRemainingCapacity->setVisible(have_selection);
		mNothingSelected->setVisible(!have_selection);

		mNextElement->setEnabled(have_selection && (is_link_select || is_face_select));
		mPrevElement->setEnabled(have_selection && (is_link_select || is_face_select));
	}


	// Refresh child tabs
	mPanelPermissions->refresh();
	mPanelObject->refresh();
	mPanelVolume->refresh();
	mPanelFace->refresh();
    mPanelFace->refreshMedia();
	mPanelContents->refresh();
	mPanelLandInfo->refresh();

	// Refresh the advanced weights floater
	LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::findTypedInstance<LLFloaterObjectWeights>("object_weights");
	if(object_weights_floater && object_weights_floater->getVisible())
	{
		object_weights_floater->refresh();
	}
}

void LLFloaterTools::draw()
{
    BOOL has_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();
    if(!has_selection && (mHasSelection != has_selection))
    {
        mDirty = TRUE;
    }
    mHasSelection = has_selection;

    if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}

	//	mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	LLFloater::draw();
}

void LLFloaterTools::dirty()
{
	mDirty = TRUE; 
	LLFloaterOpenObject* instance = LLFloaterReg::findTypedInstance<LLFloaterOpenObject>("openobject");
	if (instance) instance->dirty();
}

// Clean up any tool state that should not persist when the
// floater is closed.
void LLFloaterTools::resetToolState()
{
	gCameraBtnZoom = TRUE;
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;

	gGrabBtnSpin = FALSE;
	gGrabBtnVertical = FALSE;
}

void LLFloaterTools::updatePopup(LLCoordGL center, MASK mask)
{
	LLTool *tool = LLToolMgr::getInstance()->getCurrentTool();

	// HACK to allow seeing the buttons when you have the app in a window.
	// Keep the visibility the same as it 
	if (tool == gToolNull)
	{
		return;
	}

	if (isMinimized())
	{	// SL looks odd if we draw the tools while the window is minimized
		return;
	}
	
	// Focus buttons
	BOOL focus_visible = (tool == LLToolCamera::getInstance());

	mFocusPanel->setVisible(focus_visible);
	mBtnFocus->setToggleState(focus_visible);

	if (focus_visible)
	{
		if (!gCameraBtnOrbit &&
			!gCameraBtnPan &&
			!(mask == MASK_ORBIT) &&
			!(mask == (MASK_ORBIT | MASK_ALT)) &&
			!(mask == MASK_PAN) &&
			!(mask == (MASK_PAN | MASK_ALT)))
		{
			mRadioGroupFocus->setValue("radio zoom");
		}
		else if (gCameraBtnOrbit ||
			(mask == MASK_ORBIT) ||
			(mask == (MASK_ORBIT | MASK_ALT)))
		{
			mRadioGroupFocus->setValue("radio orbit");
		}
		else if (gCameraBtnPan ||
			(mask == MASK_PAN) ||
			(mask == (MASK_PAN | MASK_ALT)))
		{
			mRadioGroupFocus->setValue("radio pan");
		}

		// multiply by correction factor because volume sliders go [0, 0.5]
		mZoomSlider->setValue(gAgentCamera.getCameraZoomFraction() * 0.5f);
	}

	// Move buttons
	BOOL move_visible = (tool == LLToolGrab::getInstance());

	mGrabPanel->setVisible(move_visible);
	mBtnMove->setToggleState(move_visible);

	if (move_visible)
	{
		// HACK - highlight buttons for next click
		mRadioGroupMove->setVisible(move_visible);
		if (!(gGrabBtnSpin ||
			gGrabBtnVertical ||
			(mask == MASK_VERTICAL) ||
			(mask == MASK_SPIN)))
		{
			mRadioGroupMove->setValue("radio move");
		}
		else if ((mask == MASK_VERTICAL) ||
			(gGrabBtnVertical && (mask != MASK_SPIN)))
		{
			mRadioGroupMove->setValue("radio lift");
		}
		else if ((mask == MASK_SPIN) ||
			(gGrabBtnSpin && (mask != MASK_VERTICAL)))
		{
			mRadioGroupMove->setValue("radio spin");
		}
	}

	// Edit buttons
	BOOL edit_visible = tool == LLToolCompTranslate::getInstance() ||
						tool == LLToolCompRotate::getInstance() ||
						tool == LLToolCompScale::getInstance() ||
						tool == LLToolFace::getInstance() ||
						tool == LLToolIndividual::getInstance() ||
						tool == LLToolPipette::getInstance() ||
//						//BD - Qarl's Aligning Tool
						tool == QToolAlign::getInstance();

	mEditPanel->setVisible(edit_visible);
	mBtnEdit->setToggleState( edit_visible );

	if (edit_visible)
	{
		mBtnLink->setEnabled(LLSelectMgr::instance().enableLinkObjects());
		mBtnUnlink->setEnabled(LLSelectMgr::instance().enableUnlinkObjects());

		if (tool == LLToolCompTranslate::getInstance())
		{
			mRadioGroupEdit->setValue("radio position");
		}
		else if (tool == LLToolCompRotate::getInstance())
		{
			mRadioGroupEdit->setValue("radio rotate");
		}
		else if (tool == LLToolCompScale::getInstance())
		{
			mRadioGroupEdit->setValue("radio stretch");
		}
		else if (tool == LLToolFace::getInstance())
		{
			mRadioGroupEdit->setValue("radio select face");
		}
		//	//BD - Qarl's Aligning Tool
		else if (tool == QToolAlign::getInstance())
		{
			mRadioGroupEdit->setValue("radio align");
		}
	}

	//BD - Refresh grid options
	LLFloaterBuildOptions* options_floater = LLFloaterReg::getTypedInstance<LLFloaterBuildOptions>("build_options");
	if (options_floater)
	{
		options_floater->refreshGridMode();
	}

	// Create buttons
	BOOL create_visible = (tool == LLToolCompCreate::getInstance());

	mCreatePanel->setVisible(create_visible);
	mBtnCreate->setToggleState(tool == LLToolCompCreate::getInstance());

	if (create_visible)
	{
		if ( mCheckCopySelection->getValue())
		{
			// don't highlight any placer button
			for (std::vector<LLButton*>::size_type i = 0; i < mButtons.size(); i++)
			{
				mButtons[i]->setToggleState(FALSE);
			}
		}
		else
		{
			// Highlight the correct placer button
			for (S32 t = 0; t < (S32)mButtons.size(); t++)
			{
				LLPCode pcode = LLToolPlacer::getObjectType();
				LLPCode button_pcode = toolData[t];
				BOOL state = (pcode == button_pcode);
				mButtons[t]->setToggleState(state);
			}
		}
	}

	// Land buttons
	BOOL land_visible = (tool == LLToolBrushLand::getInstance() || tool == LLToolSelectLand::getInstance());

	mLandPanel->setVisible(land_visible);
	mCostTextBorder->setVisible(!land_visible);

	mBtnLand->setToggleState(land_visible);
	
	if (land_visible)
	{
		S32 dozer_mode = gSavedSettings.getS32("RadioLandBrushAction");
		switch(dozer_mode)
		{
		case 0:
			mRadioGroupLand->setValue("radio select land");
			break;
		case 1:
			mRadioGroupLand->setValue("radio flatten");
			break;
		case 2:
			mRadioGroupLand->setValue("radio raise");
			break;
		case 3:
			mRadioGroupLand->setValue("radio lower");
			break;
		case 4:
			mRadioGroupLand->setValue("radio smooth");
			break;
		case 5:
			mRadioGroupLand->setValue("radio noise");
			break;
		case 6:
			mRadioGroupLand->setValue("radio revert");
			break;
		default:
			break;
		}
	}

	mBtnApplyToSelection->setEnabled( land_visible && !LLViewerParcelMgr::getInstance()->selectionEmpty() && tool != LLToolSelectLand::getInstance());

	bool have_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();

	mSelectionCount->setVisible(!land_visible && have_selection);
	mRemainingCapacity->setVisible(!land_visible && have_selection);
	mNothingSelected->setVisible(!land_visible && !have_selection);
	
	mTab->setVisible(!land_visible);
	mPanelLandInfo->setVisible(land_visible);
}


// virtual
BOOL LLFloaterTools::canClose()
{
	// don't close when quitting, so camera will stay put
	return !LLApp::isExiting();
}

// virtual
void LLFloaterTools::onOpen(const LLSD& key)
{
	mParcelSelection = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	
	std::string panel = key.asString();
	if (!panel.empty())
	{
		mTab->selectTabByName(panel);
	}

	LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
	if (tool == LLToolCompInspect::getInstance()
		|| tool == LLToolDragAndDrop::getInstance())
	{
		// Something called floater up while it was supressed (during drag n drop, inspect),
		// so it won't be getting any layout or visibility updates, update once
		// further updates will come from updateLayout()
		LLCoordGL select_center_screen;
		MASK	mask = gKeyboard->currentMask(TRUE);
		updatePopup(select_center_screen, mask);
	}
	
	//gMenuBarView->setItemVisible("BuildTools", TRUE);
}

// virtual
void LLFloaterTools::onClose(bool app_quitting)
{
	gJoystick->moveAvatar(false);

	// destroy media source used to grab media title
	mPanelFace->unloadMedia();

    // Different from handle_reset_view in that it doesn't actually 
	//   move the camera if EditCameraMovement is not set.
	gAgentCamera.resetView(gSavedSettings.getBOOL("EditCameraMovement"));
	
	// exit component selection mode
	LLSelectMgr::getInstance()->promoteSelectionToRoot();
	gSavedSettings.setBOOL("EditLinkedParts", FALSE);

	gViewerWindow->showCursor();

	resetToolState();

	mParcelSelection = NULL;
	mObjectSelection = NULL;

	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	// Switch back to basic toolset
	tool_mgr->setCurrentToolset(gBasicToolset);
	// we were already in basic toolset, using build tools
	// so manually reset tool to default (pie menu tool)
	tool_mgr->getCurrentToolset()->selectFirstTool();

	//gMenuBarView->setItemVisible("BuildTools", FALSE);
	LLFloaterReg::hideInstance("media_settings");

	// hide the advanced object weights floater
	LLFloaterReg::hideInstance("object_weights");

	// prepare content for next call
	mPanelContents->clearContents();

	if(sPreviousFocusOnAvatar)
	{
		sPreviousFocusOnAvatar = false;
		gAgentCamera.setAllowChangeToFollow(TRUE);
	}
}

void click_popup_info(void*)
{
}

void click_popup_done(void*)
{
	handle_reset_view();
}

void commit_radio_group_move(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio move")
	{
		gGrabBtnVertical = FALSE;
		gGrabBtnSpin = FALSE;
	}
	else if (selected == "radio lift")
	{
		gGrabBtnVertical = TRUE;
		gGrabBtnSpin = FALSE;
	}
	else if (selected == "radio spin")
	{
		gGrabBtnVertical = FALSE;
		gGrabBtnSpin = TRUE;
	}
}

void commit_radio_group_focus(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio zoom")
	{
		gCameraBtnZoom = TRUE;
		gCameraBtnOrbit = FALSE;
		gCameraBtnPan = FALSE;
	}
	else if (selected == "radio orbit")
	{
		gCameraBtnZoom = FALSE;
		gCameraBtnOrbit = TRUE;
		gCameraBtnPan = FALSE;
	}
	else if (selected == "radio pan")
	{
		gCameraBtnZoom = FALSE;
		gCameraBtnOrbit = FALSE;
		gCameraBtnPan = TRUE;
	}
}

void commit_slider_zoom(LLUICtrl *ctrl)
{
	// renormalize value, since max "volume" level is 0.5 for some reason
	F32 zoom_level = (F32)ctrl->getValue().asReal() * 2.f; // / 0.5f;
	gAgentCamera.setCameraZoomFraction(zoom_level);
}

void commit_slider_dozer_force(LLUICtrl *ctrl)
{
	// the slider is logarithmic, so we exponentiate to get the actual force multiplier
	F32 dozer_force = pow(10.f, (F32)ctrl->getValue().asReal());
	gSavedSettings.setF32("LandBrushForce", dozer_force);
}

void click_apply_to_selection(void*)
{
	LLToolBrushLand::getInstance()->modifyLandInSelectionGlobal();
}

void commit_radio_group_edit(LLUICtrl *ctrl)
{
	S32 show_owners = gSavedSettings.getBOOL("ShowParcelOwners");

	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio position")
	{
		LLFloaterTools::setEditTool( LLToolCompTranslate::getInstance() );
	}
	else if (selected == "radio rotate")
	{
		LLFloaterTools::setEditTool( LLToolCompRotate::getInstance() );
	}
	else if (selected == "radio stretch")
	{
		LLFloaterTools::setEditTool( LLToolCompScale::getInstance() );
	}
	else if (selected == "radio select face")
	{
		LLFloaterTools::setEditTool( LLToolFace::getInstance() );
	}
//	//BD - Qarl's Aligning Tool
	else if (selected == "radio align")
	{
		LLFloaterTools::setEditTool( QToolAlign::getInstance() );
	}

	gSavedSettings.setBOOL("ShowParcelOwners", show_owners);
}

void commit_radio_group_land(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	S32 selected = group->getValue().asInteger();
	if (selected == 0)
	{
		LLFloaterTools::setEditTool( LLToolSelectLand::getInstance() );
	}
	else
	{
		LLFloaterTools::setEditTool( LLToolBrushLand::getInstance() );
	}
}

void commit_select_component(void *data)
{
	LLFloaterTools* floaterp = (LLFloaterTools*)data;

	//forfeit focus
	if (gFocusMgr.childHasKeyboardFocus(floaterp))
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}

	BOOL select_individuals = floaterp->mCheckSelectIndividual->getValue();
	floaterp->dirty();

	if (select_individuals)
	{
		LLSelectMgr::getInstance()->demoteSelectionToIndividuals();
	}
	else
	{
		LLSelectMgr::getInstance()->promoteSelectionToRoot();
	}
}

// static 
void LLFloaterTools::setObjectType( LLPCode pcode )
{
	LLToolPlacer::setObjectType( pcode );
	gSavedSettings.setBOOL("CreateToolCopySelection", FALSE);
	gFocusMgr.setMouseCapture(NULL);
}

void LLFloaterTools::onClickGridOptions()
{
	LLFloater* floaterp = LLFloaterReg::showInstance("build_options");
	// position floater next to build tools, not over
	floaterp->setShape(gFloaterView->findNeighboringPosition(this, floaterp), true);
}

// static
void LLFloaterTools::setEditTool(void* tool_pointer)
{
	LLTool *tool = (LLTool *)tool_pointer;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( tool );
}

void LLFloaterTools::setTool(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	if(control_name == "Focus")
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool((LLTool *) LLToolCamera::getInstance() );
	else if (control_name == "Move" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *)LLToolGrab::getInstance() );
	else if (control_name == "Edit" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolCompTranslate::getInstance());
	else if (control_name == "Create" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolCompCreate::getInstance());
	else if (control_name == "Land" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolSelectLand::getInstance());
	else
		LL_WARNS()<<" no parameter name "<<control_name<<" found!! No Tool selected!!"<< LL_ENDL;
}

void LLFloaterTools::onFocusReceived()
{
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLFloater::onFocusReceived();
}


void LLFloaterTools::updateLandImpacts()
{
	LLParcel *parcel = mParcelSelection->getParcel();
	if (!parcel)
	{
		return;
	}

	S32 rezzed_prims = parcel->getSimWidePrimCount();
	S32 total_capacity = parcel->getSimWideMaxPrimCapacity();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if (region)
	{
		S32 max_tasks_per_region = (S32)region->getMaxTasks();
		total_capacity = llmin(total_capacity, max_tasks_per_region);
	}
	std::string remaining_capacity_str = "";

	bool show_mesh_cost = gMeshRepo.meshRezEnabled();
	if (show_mesh_cost)
	{
		LLStringUtil::format_map_t remaining_capacity_args;
		remaining_capacity_args["LAND_CAPACITY"] = llformat("%d", total_capacity - rezzed_prims);
		remaining_capacity_str = getString("status_remaining_capacity", remaining_capacity_args);
	}

	mRemainingCapacity->setTextArg("[CAPACITY_STRING]", remaining_capacity_str);

	// Update land impacts info in the weights floater
	LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::findTypedInstance<LLFloaterObjectWeights>("object_weights");
	if(object_weights_floater)
	{
		object_weights_floater->updateLandImpacts(parcel);
	}
}

//BD - Shameless plug from llviewermenu.cpp
// Cycle selection through linked children or/and faces in selected object.
// FIXME: Order of children list is not always the same as sim's idea of link order. This may confuse
// resis. Need link position added to sim messages to address this.
void LLFloaterTools::onSelectElement(LLUICtrl* ctrl, const LLSD& userdata)
{
	bool cycle_faces = LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
	bool cycle_linked = gSavedSettings.getBOOL("EditLinkedParts");

	if (!cycle_faces && !cycle_linked)
	{
		// Nothing to do
		return;
	}

	bool fwd = (userdata.asString() == "next");
	bool prev = (userdata.asString() == "previous");
	bool ifwd = (userdata.asString() == "includenext");
	bool iprev = (userdata.asString() == "includeprevious");

	LLViewerObject* to_select = NULL;
	bool restart_face_on_part = !cycle_faces;
	S32 new_te = 0;

	if (cycle_faces)
	{
		// Cycle through faces of current selection, if end is reached, swithc to next part (if present)
		LLSelectNode* nodep = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
		if (!nodep)
			return;
		to_select = nodep->getObject();
		if (!to_select)
			return;

		S32 te_count = to_select->getNumTEs();
		S32 selected_te = nodep->getLastOperatedTE();

		if (fwd || ifwd)
		{
			if (selected_te < 0)
			{
				new_te = 0;
			}
			else if (selected_te + 1 < te_count)
			{
				// select next face
				new_te = selected_te + 1;
			}
			else
			{
				// restart from first face on next part
				restart_face_on_part = true;
			}
		}
		else if (prev || iprev)
		{
			if (selected_te > te_count)
			{
				new_te = te_count - 1;
			}
			else if (selected_te - 1 >= 0)
			{
				// select previous face
				new_te = selected_te - 1;
			}
			else
			{
				// restart from last face on next part
				restart_face_on_part = true;
			}
		}
	}

	S32 object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
	if (cycle_linked && object_count && restart_face_on_part)
	{
		LLViewerObject* selected = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
		if (selected && selected->getRootEdit())
		{
			LLViewerObject::child_list_t children = selected->getRootEdit()->getChildren();
			children.push_front(selected->getRootEdit());	// need root in the list too

			for (LLViewerObject::child_list_t::iterator iter = children.begin(); iter != children.end(); ++iter)
			{
				if ((*iter)->isSelected())
				{
					if (object_count > 1 && (fwd || prev))	// multiple selection, find first or last selected if not include
					{
						to_select = *iter;
						if (fwd)
						{
							// stop searching if going forward; repeat to get last hit if backward
							break;
						}
					}
					else if ((object_count == 1) || (ifwd || iprev))	// single selection or include
					{
						if (fwd || ifwd)
						{
							++iter;
							while (iter != children.end() && ((*iter)->isAvatar() || (ifwd && (*iter)->isSelected())))
							{
								++iter;	// skip sitting avatars and selected if include
							}
						}
						else // backward
						{
							iter = (iter == children.begin() ? children.end() : iter);
							--iter;
							while (iter != children.begin() && ((*iter)->isAvatar() || (iprev && (*iter)->isSelected())))
							{
								--iter;	// skip sitting avatars and selected if include
							}
						}
						iter = (iter == children.end() ? children.begin() : iter);
						to_select = *iter;
						break;
					}
				}
			}
		}
	}

	if (to_select)
	{
		if (gFocusMgr.childHasKeyboardFocus(gFloaterTools))
		{
			gFocusMgr.setKeyboardFocus(NULL);	// force edit toolbox to commit any changes
		}
		if (fwd || prev)
		{
			LLSelectMgr::getInstance()->deselectAll();
		}
		if (cycle_faces)
		{
			if (restart_face_on_part)
			{
				if (fwd || ifwd)
				{
					new_te = 0;
				}
				else
				{
					new_te = to_select->getNumTEs() - 1;
				}
			}
			LLSelectMgr::getInstance()->addAsIndividual(to_select, new_te, FALSE);
		}
		else
		{
			LLSelectMgr::getInstance()->selectObjectOnly(to_select);
		}
		return;
	}
	return;
};
