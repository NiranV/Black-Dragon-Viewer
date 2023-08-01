/**
* @file llpaneleditsky.cpp
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

#include "llpaneleditsky.h"

#include "llslider.h"
#include "llsliderctrl.h"
#include "lltexturectrl.h"
#include "llcolorswatch.h"
#include "llvirtualtrackball.h"
#include "llsettingssky.h"
#include "llenvironment.h"
#include "llatmosphere.h"
#include "llviewercontrol.h"

namespace
{
	//BD - Atmosphere
	const std::string   FIELD_SKY_AMBIENT_LIGHT("ambient_light");
	const std::string   FIELD_SKY_BLUE_HORIZON("blue_horizon");
	const std::string   FIELD_SKY_BLUE_DENSITY("blue_density");
	const std::string   FIELD_SKY_HAZE_HORIZON("haze_horizon");
	const std::string   FIELD_SKY_HAZE_DENSITY("haze_density");
	const std::string   FIELD_SKY_SCENE_GAMMA("scene_gamma");
	const std::string   FIELD_SKY_DENSITY_MULTIP("density_multip");
	const std::string   FIELD_SKY_DISTANCE_MULTIP("distance_multip");
	const std::string   FIELD_SKY_MAX_ALT("max_alt");

	const std::string   FIELD_SKY_DENSITY_MOISTURE_LEVEL("moisture_level");
	const std::string   FIELD_SKY_DENSITY_DROPLET_RADIUS("droplet_radius");
	const std::string   FIELD_SKY_DENSITY_ICE_LEVEL("ice_level");

	//BD - Sun & Moon
	const std::string   FIELD_SKY_SUN_MOON_COLOR("sun_moon_color");
	const std::string   FIELD_SKY_GLOW_FOCUS("glow_focus");
	const std::string   FIELD_SKY_GLOW_SIZE("glow_size");
	const std::string   FIELD_SKY_STAR_BRIGHTNESS("star_brightness");
	const std::string   FIELD_SKY_SUN_POSITION_X("sun_position_x");
	const std::string   FIELD_SKY_SUN_POSITION_Y("sun_position_y");
	const std::string   FIELD_SKY_SUN_IMAGE("sun_image");
	const std::string   FIELD_SKY_SUN_SCALE("sun_scale");
	const std::string   FIELD_SKY_SUN_BEACON("sunbeacon");
	const std::string   FIELD_SKY_MOON_BEACON("moonbeacon");
	const std::string   FIELD_SKY_MOON_POSITION_X("moon_position_x");
	const std::string   FIELD_SKY_MOON_POSITION_Y("moon_position_y");
	const std::string   FIELD_SKY_MOON_IMAGE("moon_image");
	const std::string   FIELD_SKY_MOON_SCALE("moon_scale");
	const std::string   FIELD_SKY_MOON_BRIGHTNESS("moon_brightness");

	//BD - Clouds
	const std::string   FIELD_SKY_CLOUD_COLOR("cloud_color");
	const std::string   FIELD_SKY_CLOUD_COVERAGE("cloud_coverage");
	const std::string   FIELD_SKY_CLOUD_SCALE("cloud_scale");
	const std::string   FIELD_SKY_CLOUD_VARIANCE("cloud_variance");

	const std::string   FIELD_SKY_CLOUD_SCROLL_X("cloud_scroll_x");
	const std::string   FIELD_SKY_CLOUD_SCROLL_Y("cloud_scroll_y");
	const std::string   FIELD_SKY_CLOUD_MAP("cloud_map");
	const std::string   FIELD_SKY_CLOUD_DENSITY_X("cloud_density_x");
	const std::string   FIELD_SKY_CLOUD_DENSITY_Y("cloud_density_y");
	const std::string   FIELD_SKY_CLOUD_DENSITY_D("cloud_density_d");
	const std::string   FIELD_SKY_CLOUD_DETAIL_X("cloud_detail_x");
	const std::string   FIELD_SKY_CLOUD_DETAIL_Y("cloud_detail_y");
	const std::string   FIELD_SKY_CLOUD_DETAIL_D("cloud_detail_d");
	const std::string   FIELD_SKY_CLOUD_LOCK_X("cloud_lock_x");
	const std::string   FIELD_SKY_CLOUD_LOCK_Y("cloud_lock_y");

	//BD - Density
	const std::string   FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL("rayleigh_exponential");
	const std::string   FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE("rayleigh_exponential_scale");
	const std::string   FIELD_SKY_DENSITY_RAYLEIGH_LINEAR("rayleigh_linear");
	const std::string   FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT("rayleigh_constant");
	const std::string   FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE("rayleigh_max_altitude");

	const std::string   FIELD_SKY_DENSITY_MIE_EXPONENTIAL("mie_exponential");
	const std::string   FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE("mie_exponential_scale");
	const std::string   FIELD_SKY_DENSITY_MIE_LINEAR("mie_linear");
	const std::string   FIELD_SKY_DENSITY_MIE_CONSTANT("mie_constant");
	const std::string   FIELD_SKY_DENSITY_MIE_ANISO("mie_aniso_factor");
	const std::string   FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE("mie_max_altitude");

	const std::string   FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL("absorption_exponential");
	const std::string   FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE("absorption_exponential_scale");
	const std::string   FIELD_SKY_DENSITY_ABSORPTION_LINEAR("absorption_linear");
	const std::string   FIELD_SKY_DENSITY_ABSORPTION_CONSTANT("absorption_constant");
	const std::string   FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE("absorption_max_altitude");

	const std::string   FIELD_REFLECTION_PROBE_AMBIANCE("probe_ambiance");

	const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
	const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
	const F32 SLIDER_SCALE_GLOW_R(20.0f);
	const F32 SLIDER_SCALE_GLOW_B(-5.0f);
	const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);
}

static LLPanelInjector<LLPanelSettingsSkyAtmosTab> t_settings_atmos("panel_settings_atmos");
static LLPanelInjector<LLPanelSettingsSkyCloudTab> t_settings_cloud("panel_settings_cloud");
static LLPanelInjector<LLPanelSettingsSkySunMoonTab> t_settings_sunmoon("panel_settings_sunmoon");
static LLPanelInjector<LLPanelSettingsSkyDensityTab> t_settings_density("panel_settings_density");

//==========================================================================
LLPanelSettingsSky::LLPanelSettingsSky() :
	LLSettingsEditPanel(),
	mSkySettings()
{

}


//==========================================================================
LLPanelSettingsSkyAtmosTab::LLPanelSettingsSkyAtmosTab() :
	LLPanelSettingsSky()
{
}


BOOL LLPanelSettingsSkyAtmosTab::postBuild()
{
	//BD - Atmosphere
	mAmbientLight = getChild<LLColorSwatchCtrl>(FIELD_SKY_AMBIENT_LIGHT);
	mAmbientLight->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAmbientLightChanged(); });
	mBlueHorizon = getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_HORIZON);
	mBlueHorizon->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueHorizonChanged(); });
	mBlueDensity = getChild<LLColorSwatchCtrl>(FIELD_SKY_BLUE_DENSITY);
	mBlueDensity->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueDensityChanged(); });
	mHazeHorizon = getChild<LLUICtrl>(FIELD_SKY_HAZE_HORIZON);
	mHazeHorizon->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeHorizonChanged(); });
	mHazeDensity = getChild<LLUICtrl>(FIELD_SKY_HAZE_DENSITY);
	mHazeDensity->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeDensityChanged(); });
	mSceneGamma = getChild<LLUICtrl>(FIELD_SKY_SCENE_GAMMA);
	mSceneGamma->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSceneGammaChanged(); });
	mDensityMult = getChild<LLUICtrl>(FIELD_SKY_DENSITY_MULTIP);
	mDensityMult->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDensityMultipChanged(); });
	mDistanceMult = getChild<LLUICtrl>(FIELD_SKY_DISTANCE_MULTIP);
	mDistanceMult->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDistanceMultipChanged(); });
	mMaxAltitude = getChild<LLUICtrl>(FIELD_SKY_MAX_ALT);
	mMaxAltitude->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMaxAltChanged(); });
	mMoisture = getChild<LLUICtrl>(FIELD_SKY_DENSITY_MOISTURE_LEVEL);
	mMoisture->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoistureLevelChanged(); });
	mDroplet = getChild<LLUICtrl>(FIELD_SKY_DENSITY_DROPLET_RADIUS);
	mDroplet->setCommitCallback([this](LLUICtrl *, const LLSD &) { onDropletRadiusChanged(); });
	mIceLevel = getChild<LLUICtrl>(FIELD_SKY_DENSITY_ICE_LEVEL);
	mIceLevel->setCommitCallback([this](LLUICtrl *, const LLSD &) { onIceLevelChanged(); });

	getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setCommitCallback([this](LLUICtrl*, const LLSD&) { onReflectionProbeAmbianceChanged(); });

	refresh();

	return TRUE;
}

void LLPanelSettingsSkyAtmosTab::refresh()
{
	if (!mSkySettings)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setAllChildrenEnabled(getCanChangeSettings());

	//BD - Atmosphere
	mAmbientLight->set(mSkySettings->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
	mBlueHorizon->set(mSkySettings->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
	mBlueDensity->set(mSkySettings->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
	mHazeHorizon->setValue(mSkySettings->getHazeHorizon());
	mHazeDensity->setValue(mSkySettings->getHazeDensity());
	mSceneGamma->setValue(mSkySettings->getGamma());

	F32 density_mult = mSkySettings->getDensityMultiplier();
	density_mult /= SLIDER_SCALE_DENSITY_MULTIPLIER;
	mDensityMult->setValue(density_mult);
	mDistanceMult->setValue(mSkySettings->getDistanceMultiplier());
	mMaxAltitude->setValue(mSkySettings->getMaxY());

	static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", true);
	F32 rp_ambiance = mSkySettings->getReflectionProbeAmbiance(should_auto_adjust);

	mMoisture->setValue(mSkySettings->getSkyMoistureLevel());
	mDroplet->setValue(mSkySettings->getSkyDropletRadius());
	mIceLevel->setValue(mSkySettings->getSkyIceLevel());
	getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setValue(rp_ambiance);

    updateGammaLabel(should_auto_adjust);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyAtmosTab::onAmbientLightChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setAmbientColor(LLColor3(mAmbientLight->get() * SLIDER_SCALE_SUN_AMBIENT));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onBlueHorizonChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setBlueHorizon(LLColor3(mBlueHorizon->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onBlueDensityChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setBlueDensity(LLColor3(mBlueDensity->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onHazeHorizonChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setHazeHorizon(mHazeHorizon->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onHazeDensityChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setHazeDensity(mHazeDensity->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onSceneGammaChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setGamma(mSceneGamma->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDensityMultipChanged()
{
	if (!mSkySettings) return;
	F32 density_mult = mDensityMult->getValue().asReal();
	density_mult *= SLIDER_SCALE_DENSITY_MULTIPLIER;
	mSkySettings->setDensityMultiplier(density_mult);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDistanceMultipChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setDistanceMultiplier(mDistanceMult->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onMaxAltChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setMaxY(mMaxAltitude->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onMoistureLevelChanged()
{
	if (!mSkySettings) return;
	F32 moisture_level = mMoisture->getValue().asReal();
	mSkySettings->setSkyMoistureLevel(moisture_level);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onDropletRadiusChanged()
{
	if (!mSkySettings) return;
	F32 droplet_radius = mDroplet->getValue().asReal();
	mSkySettings->setSkyDropletRadius(droplet_radius);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onIceLevelChanged()
{
	if (!mSkySettings) return;
	F32 ice_level = mIceLevel->getValue().asReal();
	mSkySettings->setSkyIceLevel(ice_level);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyAtmosTab::onReflectionProbeAmbianceChanged()
{
    if (!mSkySettings) return;
    F32 ambiance = getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->getValue().asReal();

    mSkySettings->setReflectionProbeAmbiance(ambiance);
    mSkySettings->update();
    setIsDirty();

    updateGammaLabel();
}


void LLPanelSettingsSkyAtmosTab::updateGammaLabel(bool auto_adjust)
{
    if (!mSkySettings) return;
    F32 ambiance = mSkySettings->getReflectionProbeAmbiance(auto_adjust);
    if (ambiance != 0.f)
    {
        childSetValue("scene_gamma_label", getString("hdr_string"));
    }
    else
    {
        childSetValue("scene_gamma_label", getString("brightness_string"));
    }

}
//==========================================================================
LLPanelSettingsSkyCloudTab::LLPanelSettingsSkyCloudTab() :
	LLPanelSettingsSky()
{
}

BOOL LLPanelSettingsSkyCloudTab::postBuild()
{
	//BD - Clouds
	mCloudColor = getChild<LLColorSwatchCtrl>(FIELD_SKY_CLOUD_COLOR);
	mCloudColor->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudColorChanged(); });
	mCloudCoverage = getChild<LLUICtrl>(FIELD_SKY_CLOUD_COVERAGE);
	mCloudCoverage->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudCoverageChanged(); });
	mCloudScale = getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCALE);
	mCloudScale->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScaleChanged(); });
	mCloudVariance = getChild<LLUICtrl>(FIELD_SKY_CLOUD_VARIANCE);
	mCloudVariance->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudVarianceChanged(); });

	mCloudScrollX = getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_X);
	mCloudScrollX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScrollChanged(); });
	mCloudScrollY = getChild<LLUICtrl>(FIELD_SKY_CLOUD_SCROLL_Y);
	mCloudScrollY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScrollChanged(); });
	mCloudScrollLockX = getChild<LLUICtrl>(FIELD_SKY_CLOUD_LOCK_X);
	mCloudScrollLockX->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &) { onCloudScrollXLocked(ctrl->getValue()); });
	mCloudScrollLockY = getChild<LLUICtrl>(FIELD_SKY_CLOUD_LOCK_Y);
	mCloudScrollLockY->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &) { onCloudScrollYLocked(ctrl->getValue()); });
	mCloudImage = getChild<LLTextureCtrl>(FIELD_SKY_CLOUD_MAP);
	mCloudImage->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudMapChanged(); });
	mCloudImage->setDefaultImageAssetID(LLSettingsSky::GetDefaultCloudNoiseTextureId());
	mCloudImage->setAllowNoTexture(TRUE);

	mCloudDensityX = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_X);
	mCloudDensityX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
	mCloudDensityY = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_Y);
	mCloudDensityY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
	mCloudDensityD = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DENSITY_D);
	mCloudDensityD->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDensityChanged(); });
	mCloudDetailX = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_X);
	mCloudDetailX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
	mCloudDetailY = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_Y);
	mCloudDetailY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });
	mCloudDetailD = getChild<LLUICtrl>(FIELD_SKY_CLOUD_DETAIL_D);
	mCloudDetailD->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudDetailChanged(); });

	refresh();

	return TRUE;
}

void LLPanelSettingsSkyCloudTab::refresh()
{
	if (!mSkySettings)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setAllChildrenEnabled(getCanChangeSettings());

	//BD - Clouds
	mCloudColor->set(mSkySettings->getCloudColor());
	mCloudCoverage->setValue(mSkySettings->getCloudShadow());
	mCloudScale->setValue(mSkySettings->getCloudScale());
	mCloudVariance->setValue(mSkySettings->getCloudVariance());

	LLVector2 cloudScroll(mSkySettings->getCloudScrollRate());
	mCloudScrollX->setValue(cloudScroll[0]);
	mCloudScrollY->setValue(cloudScroll[1]);
	LLEnvironment &environment(LLEnvironment::instance());
	mCloudScrollX->setEnabled(!environment.mCloudScrollXLocked);
	mCloudScrollY->setEnabled(!environment.mCloudScrollYLocked);
	mCloudScrollLockX->setValue(environment.mCloudScrollXLocked);
	mCloudScrollLockY->setValue(environment.mCloudScrollYLocked);

	mCloudImage->setValue(mSkySettings->getCloudNoiseTextureId());

	LLVector3 cloudDensity(mSkySettings->getCloudPosDensity1().getValue());
	mCloudDensityX->setValue(cloudDensity[0]);
	mCloudDensityY->setValue(cloudDensity[1]);
	mCloudDensityD->setValue(cloudDensity[2]);

	LLVector3 cloudDetail(mSkySettings->getCloudPosDensity2().getValue());
	mCloudDetailX->setValue(cloudDetail[0]);
	mCloudDetailY->setValue(cloudDetail[1]);
	mCloudDetailD->setValue(cloudDetail[2]);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkyCloudTab::onCloudColorChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setCloudColor(LLColor3(mCloudColor->get()));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudCoverageChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setCloudShadow(mCloudCoverage->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScaleChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setCloudScale(mCloudScale->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudVarianceChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setCloudVariance(mCloudVariance->getValue().asReal());
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScrollChanged()
{
	if (!mSkySettings) return;
	LLVector2 scroll(mCloudScrollX->getValue().asReal(), mCloudScrollY->getValue().asReal());
	mSkySettings->setCloudScrollRate(scroll);
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudMapChanged()
{
	if (!mSkySettings) return;
	LLTextureCtrl* ctrl = mCloudImage;
	mSkySettings->setCloudNoiseTextureId(ctrl->getValue().asUUID());
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudDensityChanged()
{
	if (!mSkySettings) return;
	LLColor3 density(mCloudDensityX->getValue().asReal(), mCloudDensityY->getValue().asReal(), mCloudDensityD->getValue().asReal());
	mSkySettings->setCloudPosDensity1(density);
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudDetailChanged()
{
	if (!mSkySettings) return;
	LLColor3 detail(mCloudDetailX->getValue().asReal(), mCloudDetailY->getValue().asReal(), mCloudDetailD->getValue().asReal());
	mSkySettings->setCloudPosDensity2(detail);
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScrollXLocked(bool lock)
{
	LLEnvironment::instance().pauseCloudScrollX(lock);
	refresh();
	setIsDirty();
}

void LLPanelSettingsSkyCloudTab::onCloudScrollYLocked(bool lock)
{
	LLEnvironment::instance().pauseCloudScrollY(lock);
	refresh();
	setIsDirty();
}

//==========================================================================
LLPanelSettingsSkySunMoonTab::LLPanelSettingsSkySunMoonTab() :
	LLPanelSettingsSky()
{
}


BOOL LLPanelSettingsSkySunMoonTab::postBuild()
{
	//BD - Sun & Moon
	mSunMoonColor = getChild<LLColorSwatchCtrl>(FIELD_SKY_SUN_MOON_COLOR);
	mSunMoonColor->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunMoonColorChanged(); });
	mGlowFocus = getChild<LLUICtrl>(FIELD_SKY_GLOW_FOCUS);
	mGlowFocus->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
	mGlowSize = getChild<LLUICtrl>(FIELD_SKY_GLOW_SIZE);
	mGlowSize->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
	mStarBrightness = getChild<LLUICtrl>(FIELD_SKY_STAR_BRIGHTNESS);
	mStarBrightness->setCommitCallback([this](LLUICtrl *, const LLSD &) { onStarBrightnessChanged(); });
	mSunPositionX = getChild<LLUICtrl>(FIELD_SKY_SUN_POSITION_X);
	mSunPositionX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
	mSunPositionY = getChild<LLUICtrl>(FIELD_SKY_SUN_POSITION_Y);
	mSunPositionY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
	mSunImage = getChild<LLTextureCtrl>(FIELD_SKY_SUN_IMAGE);
	mSunImage->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunImageChanged(); });
	mSunScale = getChild<LLUICtrl>(FIELD_SKY_SUN_SCALE);
	mSunScale->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunScaleChanged(); });
	mSunImage->setBlankImageAssetID(LLSettingsSky::GetBlankSunTextureId());
	mSunImage->setDefaultImageAssetID(LLSettingsSky::GetBlankSunTextureId());
	mSunImage->setAllowNoTexture(TRUE);
	mMoonPositionX = getChild<LLUICtrl>(FIELD_SKY_MOON_POSITION_X);
	mMoonPositionX->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
	mMoonPositionY = getChild<LLUICtrl>(FIELD_SKY_MOON_POSITION_Y);
	mMoonPositionY->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
	mMoonImage = getChild<LLTextureCtrl>(FIELD_SKY_MOON_IMAGE);
	mMoonImage->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonImageChanged(); });
	mMoonImage->setDefaultImageAssetID(LLSettingsSky::GetDefaultMoonTextureId());
	mMoonImage->setBlankImageAssetID(LLSettingsSky::GetDefaultMoonTextureId());
	mMoonImage->setAllowNoTexture(TRUE);
	mMoonScale = getChild<LLUICtrl>(FIELD_SKY_MOON_SCALE);
	mMoonScale->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonScaleChanged(); });
	mMoonBrightness = getChild<LLUICtrl>(FIELD_SKY_MOON_BRIGHTNESS);
	mMoonBrightness->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonBrightnessChanged(); });

	mSunBeacon = getChild<LLUICtrl>(FIELD_SKY_SUN_BEACON);
	mMoonBeacon = getChild<LLUICtrl>(FIELD_SKY_MOON_BEACON);

	refresh();

	return TRUE;
}

void LLPanelSettingsSkySunMoonTab::refresh()
{
	if (!mSkySettings || !getCanChangeSettings())
	{
		mSunBeacon->setEnabled(TRUE);
		mMoonBeacon->setEnabled(TRUE);

		if (!mSkySettings)
			return;
	}
	else
	{
		setAllChildrenEnabled(TRUE);
	}

	//BD - Sun & Moon
	LLColor3 glow(mSkySettings->getGlow());
	// takes 40 - 0.2 range -> 0 - 1.99 UI range
	mGlowSize->setValue(2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
	mGlowFocus->setValue(glow.mV[2] / SLIDER_SCALE_GLOW_B);
	mStarBrightness->setValue(mSkySettings->getStarBrightness());
	mSunMoonColor->set(mSkySettings->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);
	mSunScale->setValue(mSkySettings->getSunScale());
	mSunImage->setValue(mSkySettings->getSunTextureId());
	mMoonImage->setValue(mSkySettings->getMoonTextureId());
	mMoonScale->setValue(mSkySettings->getMoonScale());
	mMoonBrightness->setValue(mSkySettings->getMoonBrightness());

	F32 pitch, yaw, roll = 0.f;
	mSkySettings->getSunRotation().getEulerAngles(&roll, &pitch, &yaw);
	mSunPositionX->setValue(pitch / F_PI);
	mSunPositionY->setValue(yaw / F_PI);

	mSkySettings->getMoonRotation().getEulerAngles(&roll, &pitch, &yaw);
	mMoonPositionX->setValue(pitch / F_PI);
	mMoonPositionY->setValue(yaw / F_PI);
}

//-------------------------------------------------------------------------
void LLPanelSettingsSkySunMoonTab::onSunMoonColorChanged()
{
	if (!mSkySettings) return;
	LLColor3 color(mSunMoonColor->get());

	color *= SLIDER_SCALE_SUN_AMBIENT;

	mSkySettings->setSunlightColor(color);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onGlowChanged()
{
	if (!mSkySettings) return;
	LLColor3 glow(mGlowSize->getValue().asReal(), 0.0f, mGlowFocus->getValue().asReal());

	// takes 0 - 1.99 UI range -> 40 -> 0.2 range
	glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R;
	glow.mV[2] *= SLIDER_SCALE_GLOW_B;

	mSkySettings->setGlow(glow);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onStarBrightnessChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setStarBrightness(mStarBrightness->getValue().asReal());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onSunRotationChanged()
{
	if (!mSkySettings) return;

	LLQuaternion rot;
	LLQuaternion delta;
	rot.setAngleAxis(mSunPositionX->getValue().asReal() * F_PI, 0, 1, 0);
	delta.setAngleAxis(mSunPositionY->getValue().asReal() * F_PI, 0, 0, 1);
	rot *= delta;

	mSkySettings->setSunRotation(rot);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onSunScaleChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setSunScale((mSunScale->getValue().asReal()));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onSunImageChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setSunTextureId(mSunImage->getValue().asUUID());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonRotationChanged()
{
	if (!mSkySettings) return;

	LLQuaternion rot;
	LLQuaternion delta;
	rot.setAngleAxis(mMoonPositionX->getValue().asReal() * F_PI, 0, 1, 0);
	delta.setAngleAxis(mMoonPositionY->getValue().asReal() * F_PI, 0, 0, 1);
	rot *= delta;

	mSkySettings->setMoonRotation(rot);
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonImageChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setMoonTextureId(mMoonImage->getValue().asUUID());
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonScaleChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setMoonScale((mMoonScale->getValue().asReal()));
	mSkySettings->update();
	setIsDirty();
}

void LLPanelSettingsSkySunMoonTab::onMoonBrightnessChanged()
{
	if (!mSkySettings) return;
	mSkySettings->setMoonBrightness((mMoonBrightness->getValue().asReal()));
	mSkySettings->update();
	setIsDirty();
}


LLPanelSettingsSkyDensityTab::LLPanelSettingsSkyDensityTab()
{
}

BOOL LLPanelSettingsSkyDensityTab::postBuild()
{
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighExponentialChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighExponentialScaleChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighLinearChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighConstantChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onRayleighMaxAltitudeChanged(); });

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieExponentialChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieExponentialScaleChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieLinearChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieConstantChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieAnisoFactorChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMieMaxAltitudeChanged(); });

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionExponentialChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionExponentialScaleChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionLinearChanged(); });
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionConstantChanged(); });

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAbsorptionMaxAltitudeChanged(); });

	refresh();
	return TRUE;
}

void LLPanelSettingsSkyDensityTab::refresh()
{
	if (!mSkySettings)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setAllChildrenEnabled(getCanChangeSettings());

	// Get first (only) profile layer of each type for editing
	LLSD rayleigh_config = mSkySettings->getRayleighConfig();
	LLSD mie_config = mSkySettings->getMieConfig();
	LLSD absorption_config = mSkySettings->getAbsorptionConfig();

	F32 rayleigh_exponential_term = rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
	F32 rayleigh_exponential_scale = rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
	F32 rayleigh_linear_term = rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
	F32 rayleigh_constant_term = rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
	F32 rayleigh_max_alt = rayleigh_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

	F32 mie_exponential_term = mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
	F32 mie_exponential_scale = mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
	F32 mie_linear_term = mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
	F32 mie_constant_term = mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_CONSTANT_TERM].asReal();
	F32 mie_aniso_factor = mie_config[LLSettingsSky::SETTING_MIE_ANISOTROPY_FACTOR].asReal();
	F32 mie_max_alt = mie_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

	F32 absorption_exponential_term = absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
	F32 absorption_exponential_scale = absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_SCALE_FACTOR].asReal();
	F32 absorption_linear_term = absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_LINEAR_TERM].asReal();
	F32 absorption_constant_term = absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_EXP_TERM].asReal();
	F32 absorption_max_alt = absorption_config[LLSettingsSky::SETTING_DENSITY_PROFILE_WIDTH].asReal();

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->setValue(rayleigh_exponential_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->setValue(rayleigh_exponential_scale);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->setValue(rayleigh_linear_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->setValue(rayleigh_constant_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->setValue(rayleigh_max_alt);

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->setValue(mie_exponential_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->setValue(mie_exponential_scale);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->setValue(mie_linear_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->setValue(mie_constant_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->setValue(mie_aniso_factor);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->setValue(mie_max_alt);

	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->setValue(absorption_exponential_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->setValue(absorption_exponential_scale);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->setValue(absorption_linear_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->setValue(absorption_constant_term);
	getChild<LLUICtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->setValue(absorption_max_alt);
}

void LLPanelSettingsSkyDensityTab::updateProfile()
{
	F32 rayleigh_exponential_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL)->getValueF32();
	F32 rayleigh_exponential_scale = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_EXPONENTIAL_SCALE)->getValueF32();
	F32 rayleigh_linear_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_LINEAR)->getValueF32();
	F32 rayleigh_constant_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_CONSTANT)->getValueF32();
	F32 rayleigh_max_alt = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_RAYLEIGH_MAX_ALTITUDE)->getValueF32();

	F32 mie_exponential_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL)->getValueF32();
	F32 mie_exponential_scale = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_EXPONENTIAL_SCALE)->getValueF32();
	F32 mie_linear_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_LINEAR)->getValueF32();
	F32 mie_constant_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_CONSTANT)->getValueF32();
	F32 mie_aniso_factor = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_ANISO)->getValueF32();
	F32 mie_max_alt = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_MIE_MAX_ALTITUDE)->getValueF32();

	F32 absorption_exponential_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL)->getValueF32();
	F32 absorption_exponential_scale = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_EXPONENTIAL_SCALE)->getValueF32();
	F32 absorption_linear_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_LINEAR)->getValueF32();
	F32 absorption_constant_term = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_CONSTANT)->getValueF32();
	F32 absorption_max_alt = getChild<LLSliderCtrl>(FIELD_SKY_DENSITY_ABSORPTION_MAX_ALTITUDE)->getValueF32();

	LLSD rayleigh_config = LLSettingsSky::createSingleLayerDensityProfile(rayleigh_max_alt, rayleigh_exponential_term, rayleigh_exponential_scale, rayleigh_linear_term, rayleigh_constant_term);
	LLSD mie_config = LLSettingsSky::createSingleLayerDensityProfile(mie_max_alt, mie_exponential_term, mie_exponential_scale, mie_linear_term, mie_constant_term, mie_aniso_factor);
	LLSD absorption_layer = LLSettingsSky::createSingleLayerDensityProfile(absorption_max_alt, absorption_exponential_term, absorption_exponential_scale, absorption_linear_term, absorption_constant_term);

	static LLSD absorption_layer_ozone = LLSettingsSky::createDensityProfileLayer(0.0f, 0.0f, 0.0f, -1.0f / 15000.0f, 8.0f / 3.0f);

	LLSD absorption_config;
	absorption_config.append(absorption_layer);
	absorption_config.append(absorption_layer_ozone);

	mSkySettings->setRayleighConfigs(rayleigh_config);
	mSkySettings->setMieConfigs(mie_config);
	mSkySettings->setAbsorptionConfigs(absorption_config);
	mSkySettings->update();
	setIsDirty();

	if (gAtmosphere)
	{
		AtmosphericModelSettings atmospheric_settings;
		LLEnvironment::getAtmosphericModelSettings(atmospheric_settings, mSkySettings);
		gAtmosphere->configureAtmosphericModel(atmospheric_settings);
	}
}

void LLPanelSettingsSkyDensityTab::onRayleighExponentialChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighExponentialScaleChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighLinearChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighConstantChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onRayleighMaxAltitudeChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieExponentialChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieExponentialScaleChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieLinearChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieConstantChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieAnisoFactorChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onMieMaxAltitudeChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionExponentialChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionExponentialScaleChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionLinearChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionConstantChanged()
{
	updateProfile();
}

void LLPanelSettingsSkyDensityTab::onAbsorptionMaxAltitudeChanged()
{
	updateProfile();
}