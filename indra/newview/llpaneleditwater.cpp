/**
* @file llpaneleditwater.cpp
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

#include "llpaneleditwater.h"

#include "llslider.h"
#include "lltexturectrl.h"
#include "llcolorswatch.h"
#include "llxyvector.h"
#include "llviewercontrol.h"

//BD
#include "llagent.h"
#include "llviewerregion.h"

namespace
{
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

	const std::string   BTN_DEFAULT_WATER_HEIGHT("default_water_height");

	//BD - Image
	const std::string   FIELD_WATER_NORMAL_MAP("water_normal_map");

	const std::string   FIELD_WATER_WAVE1_X("water_wave1_x");
	const std::string   FIELD_WATER_WAVE1_Y("water_wave1_y");
	const std::string   FIELD_WATER_WAVE2_X("water_wave2_x");
	const std::string   FIELD_WATER_WAVE2_Y("water_wave2_y");
}

static LLPanelInjector<LLPanelSettingsWaterMainTab> t_settings_water_settings("panel_settings_water_settings");
//BD - Image Tab
static LLPanelInjector<LLPanelSettingsWaterSecondaryTab> t_settings_water_image("panel_settings_water_image");

//==========================================================================
LLPanelSettingsWater::LLPanelSettingsWater() :
	LLSettingsEditPanel(),
	mWaterSettings()
{

}


//==========================================================================
LLPanelSettingsWaterMainTab::LLPanelSettingsWaterMainTab() :
	LLPanelSettingsWater(),
	mClrFogColor(nullptr)
{
}


BOOL LLPanelSettingsWaterMainTab::postBuild()
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

	getChild<LLUICtrl>(BTN_DEFAULT_WATER_HEIGHT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDefaultWaterHeight(); });

	refresh();

	return TRUE;
}

//==========================================================================
void LLPanelSettingsWaterMainTab::refresh()
{
	if (!mWaterSettings)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setAllChildrenEnabled(getCanChangeSettings());
	mClrFogColor->set(mWaterSettings->getWaterFogColor());
	mFogDensity->setValue(mWaterSettings->getWaterFogDensity());
	mUnderwaterMod->setValue(mWaterSettings->getFogMod());
	LLVector3 vect3 = mWaterSettings->getNormalScale();
	mWaterNormX->setValue(vect3[0]);
	mWaterNormY->setValue(vect3[1]);
	mWaterNormZ->setValue(vect3[2]);
	mFresnelScale->setValue(mWaterSettings->getFresnelScale());
	mFresnelOffset->setValue(mWaterSettings->getFresnelOffset());
	mScaleAbove->setValue(mWaterSettings->getScaleAbove());
	mScaleBelow->setValue(mWaterSettings->getScaleBelow());
	mBlurMult->setValue(mWaterSettings->getBlurMultiplier());
}

//==========================================================================

void LLPanelSettingsWaterMainTab::onFogColorChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setWaterFogColor(LLColor3(mClrFogColor->get()));
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFogDensityChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setWaterFogDensity(mFogDensity->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFogUnderWaterChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setFogMod(mUnderwaterMod->getValue().asReal());
	setIsDirty();
}


void LLPanelSettingsWaterMainTab::onNormalScaleChanged()
{
	if (!mWaterSettings) return;
	LLVector3 vect(mWaterNormX->getValue().asReal(), mWaterNormY->getValue().asReal(), mWaterNormZ->getValue().asReal());
	mWaterSettings->setNormalScale(vect);
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFresnelScaleChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setFresnelScale(mFresnelScale->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFresnelOffsetChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setFresnelOffset(mFresnelOffset->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onScaleAboveChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setScaleAbove(mScaleAbove->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onScaleBelowChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setScaleBelow(mScaleBelow->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onBlurMultipChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setBlurMultiplier(mBlurMult->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsWaterMainTab::onDefaultWaterHeight()
{
	if (!mWaterSettings) return;
	F32 water_height = gAgent.getRegion()->getOriginalWaterHeight();
	gAgent.getRegion()->setWaterHeightLocal(water_height);
	gSavedSettings.setF32("RenderWaterHeightFudge", water_height);
}


//BD - Image Tab
//==========================================================================
LLPanelSettingsWaterSecondaryTab::LLPanelSettingsWaterSecondaryTab() :
	LLPanelSettingsWater(),
	mTxtNormalMap(nullptr)
{
}


BOOL LLPanelSettingsWaterSecondaryTab::postBuild()
{
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

	refresh();

	return TRUE;
}

//==========================================================================
void LLPanelSettingsWaterSecondaryTab::refresh()
{
	if (!mWaterSettings)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setAllChildrenEnabled(getCanChangeSettings());
	mTxtNormalMap->setValue(mWaterSettings->getNormalMapID());
	LLVector2 vect2 = mWaterSettings->getWave1Dir() * -1.0; // Flip so that north and east are +
	mWave1X->setValue(vect2.mV[VX]);
	mWave1Y->setValue(vect2.mV[VY]);
	vect2 = mWaterSettings->getWave2Dir() * -1.0; // Flip so that north and east are +
	mWave2X->setValue(vect2.mV[VX]);
	mWave2Y->setValue(vect2.mV[VY]);
}

//==========================================================================

void LLPanelSettingsWaterSecondaryTab::onNormalMapChanged()
{
	if (!mWaterSettings) return;
	mWaterSettings->setNormalMapID(mTxtNormalMap->getImageAssetID());
	setIsDirty();
}

void LLPanelSettingsWaterSecondaryTab::onLargeWaveChanged()
{
	if (!mWaterSettings) return;
	LLVector2 vect = LLVector2::zero;
	vect.mV[VX] = mWave1X->getValue().asReal();
	vect.mV[VY] = mWave1Y->getValue().asReal();
	vect *= -1.0; // Flip so that north and east are -
	mWaterSettings->setWave1Dir(vect);
	setIsDirty();
}

void LLPanelSettingsWaterSecondaryTab::onSmallWaveChanged()
{
	if (!mWaterSettings) return;
	LLVector2 vect = LLVector2::zero;
	vect.mV[VX] = mWave2X->getValue().asReal();
	vect.mV[VY] = mWave2Y->getValue().asReal();
	vect *= -1.0; // Flip so that north and east are -
	mWaterSettings->setWave2Dir(vect);
	setIsDirty();
}