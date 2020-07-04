/** 
 * @file llfloatersettingsdebug.cpp
 * @brief floater for debugging internal viewer settings
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
#include "llfloatersettingsdebug.h"
#include "llfloater.h"
#include "lluictrlfactory.h"
//#include "llfirstuse.h"
#include "llcombobox.h"
// [RLVa:KB] - Patch: RLVa-2.1.0
#include "llsdserialize.h"
// [/RLVa:KB]
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"


LLFloaterSettingsDebug::LLFloaterSettingsDebug(const LLSD& key) 
:	LLFloater(key)
{
	mCommitCallbackRegistrar.add("SettingSelect",	boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this,_1));
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

	std::string key = getKey().asString();
	if (key == "all" || key == "base")
	{
		gSavedSettings.applyToAll(&func);
	}
	if (key == "all" || key == "account")
	{
		gSavedPerAccountSettings.applyToAll(&func);
	}

	mSettingsCombo->sortByName();
	mSettingsCombo->updateSelection();
	return TRUE;
}

void LLFloaterSettingsDebug::draw()
{
	LLControlVariable* controlp = (LLControlVariable*)mSettingsCombo->getCurrentUserdata();
	updateControl(controlp);

	LLFloater::draw();
}

//static 
void LLFloaterSettingsDebug::onSettingSelect(LLUICtrl* ctrl)
{
	LLComboBox* combo_box = (LLComboBox*)ctrl;
	LLControlVariable* controlp = (LLControlVariable*)combo_box->getCurrentUserdata();

	updateControl(controlp);
}

void LLFloaterSettingsDebug::onCommitSettings()
{
	LLControlVariable* controlp = (LLControlVariable*)mSettingsCombo->getCurrentUserdata();

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
	case TYPE_RECT:
		rect.mLeft = mValX->getValue().asInteger();
		rect.mRight = mValY->getValue().asInteger();
		rect.mBottom = mValZ->getValue().asInteger();
		rect.mTop = mValW->getValue().asInteger();
	  case TYPE_QUAT:
		quat.mQ[VX] = getChild<LLUICtrl>("val_spinner_1")->getValue().asReal();
		quat.mQ[VY] = getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
		quat.mQ[VZ] = getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
		quat.mQ[VS] = getChild<LLUICtrl>("val_spinner_4")->getValue().asReal();;
		controlp->set(quat.getValue());
		break;
	  case TYPE_RECT:
		rect.mLeft = getChild<LLUICtrl>("val_spinner_1")->getValue().asInteger();
		rect.mRight = getChild<LLUICtrl>("val_spinner_2")->getValue().asInteger();
		rect.mBottom = getChild<LLUICtrl>("val_spinner_3")->getValue().asInteger();
		rect.mTop = getChild<LLUICtrl>("val_spinner_4")->getValue().asInteger();
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
}

// static
void LLFloaterSettingsDebug::onClickDefault()
{
	LLControlVariable* controlp = (LLControlVariable*)mSettingsCombo->getCurrentUserdata();

	if (controlp)
	{
		controlp->resetToDefault(true);
		updateControl(controlp);
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

	mValX->setVisible(FALSE);
	mValY->setVisible(FALSE);
	mValZ->setVisible(FALSE);
	mColor->setVisible(FALSE);
	mText->setVisible(FALSE);
	mComment->setText(LLStringUtil::null);
//	//BD - Vector4
	mValW->setVisible(FALSE);

	if (controlp)
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

		eControlType type = controlp->type();

		//hide combo box only for non booleans, otherwise this will result in the combo box closing every frame
		mBool->setVisible(type == TYPE_BOOLEAN);
		

		mComment->setText(controlp->getComment());
		mValX->setMaxValue(F32_MAX);
		mValY->setMaxValue(F32_MAX);
		mValZ->setMaxValue(F32_MAX);
		mValX->setMinValue(-F32_MAX);
		mValY->setMinValue(-F32_MAX);
		mValZ->setMinValue(-F32_MAX);
//		//BD - Vector4
		mValW->setMaxValue(F32_MAX);
		mValW->setMinValue(-F32_MAX);
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
			  spinner1->setVisible(TRUE);
			  spinner1->setLabel(std::string("X"));
			  spinner2->setVisible(TRUE);
			  spinner2->setLabel(std::string("Y"));
			  spinner3->setVisible(TRUE);
			  spinner3->setLabel(std::string("Z"));
			  spinner4->setVisible(TRUE);
			  spinner4->setLabel(std::string("S"));
			  if (!spinner1->hasFocus())
			  {
				  spinner1->setPrecision(4);
				  spinner1->setValue(q.mQ[VX]);
			  }
			  if (!spinner2->hasFocus())
			  {
				  spinner2->setPrecision(4);
				  spinner2->setValue(q.mQ[VY]);
			  }
			  if (!spinner3->hasFocus())
			  {
				  spinner3->setPrecision(4);
				  spinner3->setValue(q.mQ[VZ]);
			  }
			  if (!spinner4->hasFocus())
			  {
				  spinner4->setPrecision(4);
				  spinner4->setValue(q.mQ[VS]);
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
		  //BD - LLSD
		  case TYPE_LLSD:
		  {
			  //BD - We do nothing here, we just want it to fill the comment properly.
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
			  }
			  break;
// [/RLVa:KB]
		  default:
			mComment->setText(std::string("unknown"));
			break;
		}
	}

}
