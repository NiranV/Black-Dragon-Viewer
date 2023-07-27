/** 
 * @file llfloatersettingsdebug.cpp
 * @brief floater for debugging internal viewer settings
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
#include "llfloatersettingsdebug.h"
#include "llfloater.h"
#include "llfiltereditor.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
// [RLVa:KB] - Patch: RLVa-2.1.0
#include "llsdserialize.h"
// [/RLVa:KB]
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"


LLFloaterSettingsDebug::LLFloaterSettingsDebug(const LLSD& key) 
:   LLFloater(key),
    mSettingList(NULL)
{
	mCommitCallbackRegistrar.add("CommitSettings",	boost::bind(&LLFloaterSettingsDebug::onCommitSettings, this));
	mCommitCallbackRegistrar.add("ClickDefault",	boost::bind(&LLFloaterSettingsDebug::onClickDefault, this));
}

LLFloaterSettingsDebug::~LLFloaterSettingsDebug()
{}

BOOL LLFloaterSettingsDebug::postBuild()
{
	mSettingsCombo = getChild<LLComboBox>("settings_combo");
	mBool = getChild<LLUICtrl>("boolean_combo");
	mText = getChild<LLUICtrl>("val_text");
	mColor = getChild<LLColorSwatchCtrl>("val_color_swatch");
	mValX = getChild<LLSpinCtrl>("val_spinner_1");
	mValY = getChild<LLSpinCtrl>("val_spinner_2");
	mValZ = getChild<LLSpinCtrl>("val_spinner_3");
	mValW = getChild<LLSpinCtrl>("val_spinner_4");
	mDefaultBtn = getChild<LLButton>("default_btn");
	mComment = getChild<LLTextEditor>("comment_text");

	struct f : public LLControlGroup::ApplyFunctor
	{
		LLComboBox* combo;
		f(LLComboBox* c) : combo(c) {}
		virtual void apply(const std::string& name, LLControlVariable* control)
		{
			if (!control->isHiddenFromSettingsEditor())
			{
				combo->add(name, (void*)control);
			}
		}
	} func(mSettingsCombo);

    getChild<LLFilterEditor>("filter_input")->setCommitCallback(boost::bind(&LLFloaterSettingsDebug::setSearchFilter, this, _2));

    mSettingList = getChild<LLScrollListCtrl>("setting_list");
    mSettingList->setCommitOnSelectionChange(TRUE);
    mSettingList->setCommitCallback(boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));

    updateList();

    gSavedSettings.getControl("DebugSettingsHideDefault")->getCommitSignal()->connect(boost::bind(&LLFloaterSettingsDebug::updateList, this, false));

	mSettingsCombo->sortByName();
	mSettingsCombo->updateSelection();
	return TRUE;
}

void LLFloaterSettingsDebug::draw()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        updateControl(controlp);
    }

	LLFloater::draw();
}

void LLFloaterSettingsDebug::onCommitSettings()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (!first_selected)
    {
        return;
    }
    LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();

	if (!controlp)
	{
		return;
	}

	LLVector3 vector;
	LLVector3d vectord;
	LLQuaternion quat;
	LLRect rect;
	LLColor4 col4;
	LLColor3 col3;
	LLColor4U col4U;
	LLColor4 color_with_alpha;
//	//BD - Vector2
	LLVector2 vector2;
//	//BD - Vector4
	LLVector4 vector4;

	switch(controlp->type())
	{		
	case TYPE_U32:
		controlp->set(mValX->getValue());
		break;
	case TYPE_S32:
		controlp->set(mValX->getValue());
		break;
	case TYPE_F32:
		controlp->set(LLSD(mValX->getValue().asReal()));
		break;
	case TYPE_BOOLEAN:
		controlp->set(mBool->getValue());
		break;
	case TYPE_STRING:
		controlp->set(LLSD(getChild<LLUICtrl>("val_text")->getValue().asString()));
		break;
	case TYPE_VEC3:
		vector.mV[VX] = (F32)mValX->getValue().asReal();
		vector.mV[VY] = (F32)mValY->getValue().asReal();
		vector.mV[VZ] = (F32)mValZ->getValue().asReal();
		controlp->set(vector.getValue());
		break;
	case TYPE_VEC3D:
		vectord.mdV[VX] = mValX->getValue().asReal();
		vectord.mdV[VY] = mValY->getValue().asReal();
		vectord.mdV[VZ] = mValZ->getValue().asReal();
		controlp->set(vectord.getValue());
		break;
	  case TYPE_QUAT:
		quat.mQ[VX] = mValX->getValue().asReal();
		quat.mQ[VY] = mValY->getValue().asReal();
		quat.mQ[VZ] = mValZ->getValue().asReal();
		quat.mQ[VS] = mValW->getValue().asReal();
		controlp->set(quat.getValue());
		break;
	  case TYPE_RECT:
		rect.mLeft = mValX->getValue().asInteger();
		rect.mRight = mValY->getValue().asInteger();
		rect.mBottom = mValZ->getValue().asInteger();
		rect.mTop = mValW->getValue().asInteger();
		controlp->set(rect.getValue());
		break;
	case TYPE_COL4:
		col3.setValue(getChild<LLUICtrl>("val_color_swatch")->getValue());
		col4 = LLColor4(col3, (F32)mValW->getValue().asReal());
		controlp->set(col4.getValue());
		break;
	case TYPE_COL3:
		controlp->set(getChild<LLUICtrl>("val_color_swatch")->getValue());
		//col3.mV[VRED] = (F32)floaterp->mValX->getValue().asC();
		//col3.mV[VGREEN] = (F32)floaterp->mValY->getValue().asReal();
		//col3.mV[VBLUE] = (F32)floaterp->mValZ->getValue().asReal();
		//controlp->set(col3.getValue());
		break;
//	//BD - Vector2
	case TYPE_VEC2:
		vector2.mV[VX] = (F32)mValX->getValue().asReal();
		vector2.mV[VY] = (F32)mValY->getValue().asReal();
		controlp->set(vector2.getValue());
		break;
//	//BD - Vector4
	case TYPE_VEC4:
		vector4.mV[VX] = (F32)mValX->getValue().asReal();
		vector4.mV[VY] = (F32)mValY->getValue().asReal();
		vector4.mV[VZ] = (F32)mValZ->getValue().asReal();
		vector4.mV[VW] = (F32)mValW->getValue().asReal();
		controlp->set(vector4.getValue());
		default:
		break;
	}
    updateDefaultColumn(controlp);
}

// static
void LLFloaterSettingsDebug::onClickDefault()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        if (controlp)
        {
            controlp->resetToDefault(true);
            updateDefaultColumn(controlp);
            updateControl(controlp);
        }
    }
}

// we've switched controls, or doing per-frame update, so update spinners, etc.
void LLFloaterSettingsDebug::updateControl(LLControlVariable* controlp)
{
//	//BD - Vector4
	if (!mValX || !mValY || !mValZ || !mValW || !mColor)
	{
		LL_WARNS() << "Could not find all desired controls by name"
			<< LL_ENDL;
		return;
	}

	hideUIControls();

	eControlType type = controlp->type();

	//hide combo box only for non booleans, otherwise this will result in the combo box closing every frame
	mBool->setVisible(type == TYPE_BOOLEAN);

	if (controlp && !isSettingHidden(controlp))
	{
// [RLVa:KB] - Checked: 2011-05-28 (RLVa-1.4.0a) | Modified: RLVa-1.4.0a
		// If "HideFromEditor" was toggled while the floater is open then we need to manually disable access to the control
		// NOTE: this runs per-frame so there's no need to explictly handle onCommitSettings() or onClickDefault()
		bool fEnable = !controlp->isHiddenFromSettingsEditor();
		mValX->setEnabled(fEnable);
		mValY->setEnabled(fEnable);
		mValZ->setEnabled(fEnable);
		mColor->setEnabled(fEnable);
		mText->setEnabled(fEnable);
		mBool->setEnabled(fEnable);
		mDefaultBtn->setEnabled(fEnable);
//		//BD - Vector4
		mValW->setEnabled(fEnable);
// [/RLVa:KB]

		mValX->setMaxValue(F32_MAX);
		mValY->setMaxValue(F32_MAX);
		mValZ->setMaxValue(F32_MAX);
		mValX->setMinValue(-F32_MAX);
		mValY->setMinValue(-F32_MAX);
		mValZ->setMinValue(-F32_MAX);
//		//BD - Vector4
		mValW->setMaxValue(F32_MAX);
		mValW->setMinValue(-F32_MAX);


		getChildView("boolean_combo")->setVisible( type == TYPE_BOOLEAN);
        getChildView("default_btn")->setVisible(true);
        getChildView("setting_name_txt")->setVisible(true);
        getChild<LLTextBox>("setting_name_txt")->setText(controlp->getName());
        getChild<LLTextBox>("setting_name_txt")->setToolTip(controlp->getName());
        mComment->setVisible(true);

        std::string old_text = mComment->getText();
        std::string new_text = controlp->getComment();
        // Don't setText if not nessesary, it will reset scroll
        // This is a debug UI that reads from xml, there might
        // be use cases where comment changes, but not the name
		if (old_text != new_text)
		{
			mComment->setText(controlp->getComment());
		}
        
		if (!mValX->hasFocus())
		{
			mValX->setIncrement(0.1f);
		}
		if (!mValY->hasFocus())
		{
			mValY->setIncrement(0.1f);
		}
		if (!mValZ->hasFocus())
		{
			mValZ->setIncrement(0.1f);
		}
//		//BD - Vector4
		if (!mValW->hasFocus())
		{
			mValZ->setIncrement(0.1f);
		}

		LLSD sd = controlp->get();
		switch(type)
		{
		  case TYPE_U32:
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("value")); // Debug, don't translate
			if (!mValX->hasFocus())
			{
				mValX->setValue(sd);
				mValX->setMinValue((F32)U32_MIN);
				mValX->setMaxValue((F32)U32_MAX);
				mValX->setIncrement(1.f);
				mValX->setPrecision(0);
			}
			break;
		  case TYPE_S32:
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("value")); // Debug, don't translate
			if (!mValX->hasFocus())
			{
				mValX->setValue(sd);
				mValX->setMinValue((F32)S32_MIN);
				mValX->setMaxValue((F32)S32_MAX);
				mValX->setIncrement(1.f);
				mValX->setPrecision(0);
			}
			break;
		  case TYPE_F32:
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("value")); // Debug, don't translate
			if (!mValX->hasFocus())
			{
				mValX->setPrecision(3);
				mValX->setValue(sd);
			}
			break;
		  case TYPE_BOOLEAN:
			  if (!mBool->hasFocus())
			{
				if (sd.asBoolean())
				{
					mBool->setValue(LLSD("true"));
				}
				else
				{
					mBool->setValue(LLSD(""));
				}
			}
			break;
		  case TYPE_STRING:
			  mText->setVisible(TRUE);
			  if (!mText->hasFocus())
			{
				mText->setValue(sd);
			}
			break;
		  case TYPE_VEC3:
		  {
			LLVector3 v;
			v.setValue(sd);
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("X"));
			mValY->setVisible(TRUE);
			mValY->setLabel(std::string("Y"));
			mValZ->setVisible(TRUE);
			mValZ->setLabel(std::string("Z"));
			if (!mValX->hasFocus())
			{
				mValX->setPrecision(3);
				mValX->setValue(v[VX]);
			}
			if (!mValY->hasFocus())
			{
				mValY->setPrecision(3);
				mValY->setValue(v[VY]);
			}
			if (!mValZ->hasFocus())
			{
				mValZ->setPrecision(3);
				mValZ->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_VEC3D:
		  {
			LLVector3d v;
			v.setValue(sd);
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("X"));
			mValY->setVisible(TRUE);
			mValY->setLabel(std::string("Y"));
			mValZ->setVisible(TRUE);
			mValZ->setLabel(std::string("Z"));
			if (!mValX->hasFocus())
			{
				mValX->setPrecision(3);
				mValX->setValue(v[VX]);
			}
			if (!mValY->hasFocus())
			{
				mValY->setPrecision(3);
				mValY->setValue(v[VY]);
			}
			if (!mValZ->hasFocus())
			{
				mValZ->setPrecision(3);
				mValZ->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_QUAT:
		  {
			  LLQuaternion q;
			  q.setValue(sd);
			  mValX->setVisible(TRUE);
			  mValX->setLabel(std::string("X"));
			  mValY->setVisible(TRUE);
			  mValY->setLabel(std::string("Y"));
			  mValZ->setVisible(TRUE);
			  mValZ->setLabel(std::string("Z"));
			  mValW->setVisible(TRUE);
			  mValW->setLabel(std::string("S"));
			  if (!mValX->hasFocus())
			  {
				  mValX->setPrecision(4);
				  mValX->setValue(q.mQ[VX]);
			  }
			  if (!mValY->hasFocus())
			  {
				  mValY->setPrecision(4);
				  mValY->setValue(q.mQ[VY]);
			  }
			  if (!mValZ->hasFocus())
			  {
				  mValZ->setPrecision(4);
				  mValZ->setValue(q.mQ[VZ]);
			  }
			  if (!mValW->hasFocus())
			  {
				  mValW->setPrecision(4);
				  mValW->setValue(q.mQ[VS]);
			  }
			  break;
		  }
		  case TYPE_RECT:
		  {
			LLRect r;
			r.setValue(sd);
			mValX->setVisible(TRUE);
			mValX->setLabel(std::string("Left"));
			mValY->setVisible(TRUE);
			mValY->setLabel(std::string("Right"));
			mValZ->setVisible(TRUE);
			mValZ->setLabel(std::string("Bottom"));
			mValW->setVisible(TRUE);
			mValW->setLabel(std::string("Top"));
			if (!mValX->hasFocus())
			{
				mValX->setPrecision(0);
				mValX->setValue(r.mLeft);
			}
			if (!mValY->hasFocus())
			{
				mValY->setPrecision(0);
				mValY->setValue(r.mRight);
			}
			if (!mValZ->hasFocus())
			{
				mValZ->setPrecision(0);
				mValZ->setValue(r.mBottom);
			}
			if (!mValW->hasFocus())
			{
				mValW->setPrecision(0);
				mValW->setValue(r.mTop);
			}

			mValX->setMinValue((F32)S32_MIN);
			mValX->setMaxValue((F32)S32_MAX);
			mValX->setIncrement(1.f);

			mValY->setMinValue((F32)S32_MIN);
			mValY->setMaxValue((F32)S32_MAX);
			mValY->setIncrement(1.f);

			mValZ->setMinValue((F32)S32_MIN);
			mValZ->setMaxValue((F32)S32_MAX);
			mValZ->setIncrement(1.f);

			mValW->setMinValue((F32)S32_MIN);
			mValW->setMaxValue((F32)S32_MAX);
			mValW->setIncrement(1.f);
			break;
		  }
		  case TYPE_COL4:
		  {
			LLColor4 clr;
			clr.setValue(sd);
			mColor->setVisible(TRUE);
			// only set if changed so color picker doesn't update
			if (clr != LLColor4(mColor->getValue()))
			{
				mColor->set(LLColor4(sd), TRUE, FALSE);
			}
			mValZ->setVisible(TRUE);
			mValZ->setLabel(std::string("Alpha"));
			if (!mValZ->hasFocus())
			{
				mValZ->setPrecision(3);
				mValZ->setMinValue(0.0);
				mValZ->setMaxValue(1.f);
				mValZ->setValue(clr.mV[VALPHA]);
			}
			break;
		  }
		  case TYPE_COL3:
		  {
			LLColor3 clr;
			clr.setValue(sd);
			mColor->setVisible(TRUE);
			mColor->setValue(sd);
			break;
		  }
//		  //BD - Vector2
		  case TYPE_VEC2:
		  {
			  LLVector2 v;
			  v.setValue(sd);
			  mValX->setVisible(TRUE);
			  mValX->setLabel(std::string("X"));
			  mValY->setVisible(TRUE);
			  mValY->setLabel(std::string("Y"));
			  if (!mValX->hasFocus())
			  {
				  mValX->setPrecision(3);
				  mValX->setValue(v[VX]);
			  }
			  if (!mValY->hasFocus())
			  {
				  mValY->setPrecision(3);
				  mValY->setValue(v[VY]);
			  }
			  break;
		  }
//		  //BD - Vector4
		  case TYPE_VEC4:
		  {
			  LLVector4 v;
			  v.setValue(sd);
			  mValX->setVisible(TRUE);
			  mValX->setLabel(std::string("X"));
			  mValY->setVisible(TRUE);
			  mValY->setLabel(std::string("Y"));
			  mValZ->setVisible(TRUE);
			  mValZ->setLabel(std::string("Z"));
//			  //BD - Vector4
			  mValW->setVisible(TRUE);
			  mValW->setLabel(std::string("W"));
			  if (!mValX->hasFocus())
			  {
				  mValX->setPrecision(3);
				  mValX->setValue(v[VX]);
			  }
			  if (!mValY->hasFocus())
			  {
				  mValY->setPrecision(3);
				  mValY->setValue(v[VY]);
			  }
			  if (!mValZ->hasFocus())
			  {
				  mValZ->setPrecision(3);
				  mValZ->setValue(v[VZ]);
			  }
//			  //BD - Vector4
			  if (!mValW->hasFocus())
			  {
				  mValW->setPrecision(3);
				  mValW->setValue(v[VW]);
			  }
			  break;
		  }
// [RLVa:KB] - Patch: RLVa-2.1.0
		  case TYPE_LLSD:
		  {
			  std::ostringstream strLLSD;
			  LLSDSerialize::toPrettyNotation(sd, strLLSD);
			  mComment->setText(strLLSD.str());
			  break;
		  }
// [/RLVa:KB]
		  default:
			mComment->setText(std::string("unknown"));
			break;
		}
	}

}

void LLFloaterSettingsDebug::updateList(bool skip_selection)
{
    std::string last_selected;
    LLScrollListItem* item = mSettingList->getFirstSelected();
    if (item)
    {
        LLScrollListCell* cell = item->getColumn(1);
        if (cell)
        {
            last_selected = cell->getValue().asString();
         }
    }

    mSettingList->deleteAllItems();
    struct f : public LLControlGroup::ApplyFunctor
    {
        LLScrollListCtrl* setting_list;
        LLFloaterSettingsDebug* floater;
        std::string selected_setting;
        bool skip_selection;
        f(LLScrollListCtrl* list, LLFloaterSettingsDebug* floater, std::string setting, bool skip_selection) 
            : setting_list(list), floater(floater), selected_setting(setting), skip_selection(skip_selection) {}
        virtual void apply(const std::string& name, LLControlVariable* control)
        {
            if (!control->isHiddenFromSettingsEditor() && floater->matchesSearchFilter(name) && !floater->isSettingHidden(control))
            {
                LLSD row;

                row["columns"][0]["column"] = "changed_setting";
                row["columns"][0]["value"] = control->isDefault() ? "" : "*";

                row["columns"][1]["column"] = "setting";
                row["columns"][1]["value"] = name;

                LLScrollListItem* item = setting_list->addElement(row, ADD_BOTTOM, (void*)control);
                if (!floater->mSearchFilter.empty() && (selected_setting == name) && !skip_selection)
                {
                    std::string lower_name(name);
                    LLStringUtil::toLower(lower_name);
                    if (LLStringUtil::startsWith(lower_name, floater->mSearchFilter))
                    {
                        item->setSelected(true);
                    }
                }
            }
        }
    } func(mSettingList, this, last_selected, skip_selection);

    std::string key = getKey().asString();
    if (key == "all" || key == "base")
    {
        gSavedSettings.applyToAll(&func);
    }
    if (key == "all" || key == "account")
    {
        gSavedPerAccountSettings.applyToAll(&func);
    }


    if (!mSettingList->isEmpty())
    {
        if (mSettingList->hasSelectedItem())
        {
            mSettingList->scrollToShowSelected();
        }
        else if (!mSettingList->hasSelectedItem() && !mSearchFilter.empty() && !skip_selection)
        {
            if (!mSettingList->selectItemByPrefix(mSearchFilter, false, 1))
            {
                mSettingList->selectFirstItem();
            }
            mSettingList->scrollToShowSelected();
        }
    }
    else
    {
        LLSD row;

        row["columns"][0]["column"] = "changed_setting";
        row["columns"][0]["value"] = "";
        row["columns"][1]["column"] = "setting";
        row["columns"][1]["value"] = "No matching settings.";

        mSettingList->addElement(row);
        hideUIControls();
    }
}

void LLFloaterSettingsDebug::onSettingSelect()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        if (controlp)
        {
            updateControl(controlp);
        }
    }
}

void LLFloaterSettingsDebug::setSearchFilter(const std::string& filter)
{
    if(mSearchFilter == filter)
        return;
    mSearchFilter = filter;
    LLStringUtil::toLower(mSearchFilter);
    updateList();
}

bool LLFloaterSettingsDebug::matchesSearchFilter(std::string setting_name)
{
    // If the search filter is empty, everything passes.
    if (mSearchFilter.empty()) return true;

    LLStringUtil::toLower(setting_name);
    std::string::size_type match_name = setting_name.find(mSearchFilter);

    return (std::string::npos != match_name);
}

bool LLFloaterSettingsDebug::isSettingHidden(LLControlVariable* control)
{
    static LLCachedControl<bool> hide_default(gSavedSettings, "DebugSettingsHideDefault", false);
    return hide_default && control->isDefault();
}

void LLFloaterSettingsDebug::updateDefaultColumn(LLControlVariable* control)
{
    if (isSettingHidden(control))
    {
        hideUIControls();
        updateList(true);
        return;
    }

    LLScrollListItem* item = mSettingList->getFirstSelected();
    if (item)
    {
        LLScrollListCell* cell = item->getColumn(0);
        if (cell)
        {
            std::string is_default = control->isDefault() ? "" : "*";
            cell->setValue(is_default);
        }
    }
}

void LLFloaterSettingsDebug::hideUIControls()
{
	mBool->setVisible(false);
	mValX->setVisible(false);
	mValY->setVisible(false);
	mValZ->setVisible(false);
	mColor->setVisible(false);
	mText->setVisible(false);
	//	//BD - Vector4
	mValW->setVisible(false);
	mDefaultBtn->setVisible(false);
    getChildView("setting_name_txt")->setVisible(false);
    mComment->setVisible(false);
}

