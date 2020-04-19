/** 
 * @file llfloaterfixedenvironment.cpp
 * @brief Floaters to create and edit fixed settings for sky and water.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterwateradjust.h"

#include "llnotificationsutil.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lltexturectrl.h"
#include "llvirtualtrackball.h"
#include "llenvironment.h"
#include "llviewercontrol.h"

//BD
#include "bdfunctions.h"
#include "llsdserialize.h"
#include "llsettingsvo.h"
#include "llviewermenufile.h"
#include "llinventorymodel.h"
#include "llinventoryfunctions.h"
#include "llagent.h"


//=========================================================================
namespace
{
    const S32 FLOATER_ENVIRONMENT_UPDATE(-2);


	//BD - Settings
	const std::string   FIELD_WATER_FOG_COLOR("water_fog_color");
	const std::string   FIELD_WATER_FOG_DENSITY("water_fog_density");
	const std::string   FIELD_WATER_UNDERWATER_MOD("water_underwater_mod");

	const std::string   FIELD_WATER_NORMAL_SCALE_X("water_normal_scale_x");
	const std::string   FIELD_WATER_NORMAL_SCALE_Y("water_normal_scale_y");
	const std::string   FIELD_WATER_NORMAL_SCALE_Z("water_normal_scale_z");

	const std::string   FIELD_WATER_FRESNEL_SCALE("water_fresnel_scale");
	const std::string   FIELD_WATER_FRESNEL_OFFSET("water_fresnel_offset");

	const std::string   FIELD_WATER_SCALE_ABOVE("water_scale_above");
	const std::string   FIELD_WATER_SCALE_BELOW("water_scale_below");
	const std::string   FIELD_WATER_BLUR_MULTIP("water_blur_multip");

	//BD - Image
	const std::string   FIELD_WATER_NORMAL_MAP("water_normal_map");

	const std::string   FIELD_WATER_WAVE1_X("water_wave1_x");
	const std::string   FIELD_WATER_WAVE1_Y("water_wave1_y");
	const std::string   FIELD_WATER_WAVE2_X("water_wave2_x");
	const std::string   FIELD_WATER_WAVE2_Y("water_wave2_y");

	const std::string   BTN_RESET("reset");
	const std::string   BTN_SAVE("save");
	const std::string   BTN_DELETE("delete");
	const std::string   BTN_IMPORT("import");

	const std::string   EDITOR_NAME("water_preset_combo");

	const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
	const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
	const F32 SLIDER_SCALE_GLOW_R(20.0f);
	const F32 SLIDER_SCALE_GLOW_B(-5.0f);
	const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);
}

//=========================================================================
LLFloaterWaterAdjust::LLFloaterWaterAdjust(const LLSD &key) :
    LLFloater(key)
{}

LLFloaterWaterAdjust::~LLFloaterWaterAdjust()
{}

//-------------------------------------------------------------------------
BOOL LLFloaterWaterAdjust::postBuild()
{
	//BD - Settings
	mClrFogColor = getChild<LLColorSwatchCtrl>(FIELD_WATER_FOG_COLOR);
	mClrFogColor->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogColorChanged(); });
	mFogDensity = getChild<LLUICtrl>(FIELD_WATER_FOG_DENSITY);
	mFogDensity->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogDensityChanged(); });
	mUnderwaterMod = getChild<LLUICtrl>(FIELD_WATER_UNDERWATER_MOD);
	mUnderwaterMod->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogUnderWaterChanged(); });

	mWaterNormX = getChild<LLUICtrl>(FIELD_WATER_NORMAL_SCALE_X);
	mWaterNormX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });
	mWaterNormY = getChild<LLUICtrl>(FIELD_WATER_NORMAL_SCALE_Y);
	mWaterNormY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });
	mWaterNormZ = getChild<LLUICtrl>(FIELD_WATER_NORMAL_SCALE_Z);
	mWaterNormZ->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });

	mFresnelScale = getChild<LLUICtrl>(FIELD_WATER_FRESNEL_SCALE);
	mFresnelScale->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFresnelScaleChanged(); });
	mFresnelOffset = getChild<LLUICtrl>(FIELD_WATER_FRESNEL_OFFSET);
	mFresnelOffset->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFresnelOffsetChanged(); });
	mScaleAbove = getChild<LLUICtrl>(FIELD_WATER_SCALE_ABOVE);
	mScaleAbove->setCommitCallback([this](LLUICtrl *, const LLSD &) { onScaleAboveChanged(); });
	mScaleBelow = getChild<LLUICtrl>(FIELD_WATER_SCALE_BELOW);
	mScaleBelow->setCommitCallback([this](LLUICtrl *, const LLSD &) { onScaleBelowChanged(); });
	mBlurMult = getChild<LLUICtrl>(FIELD_WATER_BLUR_MULTIP);
	mBlurMult->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlurMultipChanged(); });

	//BD - Image
	mTxtNormalMap = getChild<LLTextureCtrl>(FIELD_WATER_NORMAL_MAP);
	mTxtNormalMap->setDefaultImageAssetID(LLSettingsWater::GetDefaultWaterNormalAssetId());
	mTxtNormalMap->setBlankImageAssetID(LLUUID(gSavedSettings.getString("DefaultBlankNormalTexture")));
	mTxtNormalMap->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalMapChanged(); });

	mWave1X = getChild<LLUICtrl>(FIELD_WATER_WAVE1_X);
	mWave1X->setCommitCallback([this](LLUICtrl *, const LLSD &) { onLargeWaveChanged(); });
	mWave1Y = getChild<LLUICtrl>(FIELD_WATER_WAVE1_Y);
	mWave1Y->setCommitCallback([this](LLUICtrl *, const LLSD &) { onLargeWaveChanged(); });

	mWave2X = getChild<LLUICtrl>(FIELD_WATER_WAVE2_X);
	mWave2X->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSmallWaveChanged(); });
	mWave2Y = getChild<LLUICtrl>(FIELD_WATER_WAVE2_Y);
	mWave2Y->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSmallWaveChanged(); });

	getChild<LLUICtrl>(BTN_RESET)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonReset(); });
	getChild<LLUICtrl>(BTN_DELETE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonDelete(); });
	getChild<LLUICtrl>(BTN_SAVE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonSave(); });
	getChild<LLUICtrl>(BTN_IMPORT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });

	mNameCombo = getChild<LLComboBox>(EDITOR_NAME);
	mNameCombo->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSelectPreset(); });

    refresh();
    return TRUE;
}

void LLFloaterWaterAdjust::onOpen(const LLSD& key)
{
    captureCurrentEnvironment();

    mEventConnection = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32 version){ onEnvironmentUpdated(env, version); });

    LLFloater::onOpen(key);
    refresh();
	gDragonLibrary.loadPresetsFromDir(mNameCombo, "water");
	gDragonLibrary.addInventoryPresets(mNameCombo, mLiveWater);
}

void LLFloaterWaterAdjust::onClose(bool app_quitting)
{
    LLEnvironment::instance().revertBeaconsState();
    mEventConnection.disconnect();
    mLiveWater.reset();
    LLFloater::onClose(app_quitting);
}


//-------------------------------------------------------------------------
void LLFloaterWaterAdjust::refresh()
{
    if (!mLiveWater)
    {
        setAllChildrenEnabled(FALSE);
        return;
    }

    setEnabled(TRUE);
    setAllChildrenEnabled(TRUE);

	//BD - Settings
	mClrFogColor->set(mLiveWater->getWaterFogColor());
	mFogDensity->setValue(mLiveWater->getWaterFogDensity());
	mUnderwaterMod->setValue(mLiveWater->getFogMod());
	LLVector3 vect3 = mLiveWater->getNormalScale();
	mWaterNormX->setValue(vect3[0]);
	mWaterNormY->setValue(vect3[1]);
	mWaterNormZ->setValue(vect3[2]);
	mFresnelScale->setValue(mLiveWater->getFresnelScale());
	mFresnelOffset->setValue(mLiveWater->getFresnelOffset());
	mScaleAbove->setValue(mLiveWater->getScaleAbove());
	mScaleBelow->setValue(mLiveWater->getScaleBelow());
	mBlurMult->setValue(mLiveWater->getBlurMultiplier());

	//BD - Image
	mTxtNormalMap->setValue(mLiveWater->getNormalMapID());
	LLVector2 vect2 = mLiveWater->getWave1Dir() * -1.0; // Flip so that north and east are +
	mWave1X->setValue(vect2.mV[VX]);
	mWave1Y->setValue(vect2.mV[VY]);
	vect2 = mLiveWater->getWave2Dir() * -1.0; // Flip so that north and east are +
	mWave2X->setValue(vect2.mV[VX]);
	mWave2Y->setValue(vect2.mV[VY]);
}


void LLFloaterWaterAdjust::captureCurrentEnvironment()
{
    LLEnvironment &environment(LLEnvironment::instance());
    bool updatelocal(false);

    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {   // We have a full day cycle in the local environment.  Freeze the water
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL)->buildClone();
            updatelocal = true;
        }
        else
        {   // otherwise we can just use the water.
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL);
        }
    }
    else
    {
        mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_PARCEL, true)->buildClone();
        updatelocal = true;
    }
    if (updatelocal)
    {
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveWater, FLOATER_ENVIRONMENT_UPDATE);
    }
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    environment.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);

}

void LLFloaterWaterAdjust::onButtonReset()
{
    LLNotificationsUtil::add("PersonalSettingsConfirmReset", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
			LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
			LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);

			mLiveWater.reset();
        }
    }); 

}

void LLFloaterWaterAdjust::onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version)
{
    if (env == LLEnvironment::ENV_LOCAL)
    {   // a new local environment has been applied
        if (version != FLOATER_ENVIRONMENT_UPDATE)
        {   // not by this floater
            captureCurrentEnvironment();
            refresh();
        }
    }
}


//BD - Settings
void LLFloaterWaterAdjust::onFogColorChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setWaterFogColor(LLColor3(mClrFogColor->get()));
}

void LLFloaterWaterAdjust::onFogDensityChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setWaterFogDensity(mFogDensity->getValue().asReal());
}

void LLFloaterWaterAdjust::onFogUnderWaterChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setFogMod(mUnderwaterMod->getValue().asReal());
}


void LLFloaterWaterAdjust::onNormalScaleChanged()
{
	if (!mLiveWater) return;
	LLVector3 vect(mWaterNormX->getValue().asReal(), mWaterNormY->getValue().asReal(), mWaterNormZ->getValue().asReal());
	mLiveWater->setNormalScale(vect);
}

void LLFloaterWaterAdjust::onFresnelScaleChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setFresnelScale(mFresnelScale->getValue().asReal());
}

void LLFloaterWaterAdjust::onFresnelOffsetChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setFresnelOffset(mFresnelOffset->getValue().asReal());
}

void LLFloaterWaterAdjust::onScaleAboveChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setScaleAbove(mScaleAbove->getValue().asReal());
}

void LLFloaterWaterAdjust::onScaleBelowChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setScaleBelow(mScaleBelow->getValue().asReal());
}

void LLFloaterWaterAdjust::onBlurMultipChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setBlurMultiplier(mBlurMult->getValue().asReal());
}


//BD - Image
void LLFloaterWaterAdjust::onNormalMapChanged()
{
	if (!mLiveWater) return;
	mLiveWater->setNormalMapID(mTxtNormalMap->getImageAssetID());
	LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mLiveWater, FLOATER_ENVIRONMENT_UPDATE);
}

void LLFloaterWaterAdjust::onLargeWaveChanged()
{
	if (!mLiveWater) return;
	LLVector2 vect = LLVector2::zero;
	vect.mV[VX] = mWave1X->getValue().asReal();
	vect.mV[VY] = mWave1Y->getValue().asReal();
	vect *= -1.0; // Flip so that north and east are -
	mLiveWater->setWave1Dir(vect);
}

void LLFloaterWaterAdjust::onSmallWaveChanged()
{
	if (!mLiveWater) return;
	LLVector2 vect = LLVector2::zero;
	vect.mV[VX] = mWave2X->getValue().asReal();
	vect.mV[VY] = mWave2Y->getValue().asReal();
	vect *= -1.0; // Flip so that north and east are -
	mLiveWater->setWave2Dir(vect);
}


//BD - Windlight Stuff
//=====================================================================================================
void LLFloaterWaterAdjust::onButtonSave()
{
	if (!mLiveWater) return;
	gDragonLibrary.savePreset(mNameCombo->getValue(), mLiveWater);
	gDragonLibrary.loadPresetsFromDir(mNameCombo, "water");
	gDragonLibrary.addInventoryPresets(mNameCombo, mLiveWater);
}

void LLFloaterWaterAdjust::onButtonDelete()
{
	if (!mLiveWater) return;
	gDragonLibrary.deletePreset(mNameCombo->getValue(), "water");
	gDragonLibrary.loadPresetsFromDir(mNameCombo, "water");
	gDragonLibrary.addInventoryPresets(mNameCombo, mLiveWater);
}

void LLFloaterWaterAdjust::onButtonImport()
{   // Load a a legacy Windlight XML from disk.
	(new LLFilePickerReplyThread(boost::bind(&LLFloaterWaterAdjust::loadWaterSettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false))->getFile();
}

void LLFloaterWaterAdjust::loadWaterSettingFromFile(const std::vector<std::string>& filenames)
{
	if (filenames.size() < 1) return;
	std::string filename = filenames[0];
	gDragonLibrary.loadPreset(filename, mLiveWater);
	refresh();
}

void LLFloaterWaterAdjust::onSelectPreset()
{
	if (!mLiveWater) return;
	gDragonLibrary.onSelectPreset(mNameCombo, mLiveWater);
	refresh();
}