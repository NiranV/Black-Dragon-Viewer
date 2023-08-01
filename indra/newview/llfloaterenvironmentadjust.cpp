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

#include "llfloaterenvironmentadjust.h"

#include "llnotificationsutil.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llcombobox.h"
#include "llcolorswatch.h"
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
#include "lltrans.h"
#include "llflyoutcombobtn.h"


//=========================================================================
namespace
{
    const S32 FLOATER_ENVIRONMENT_UPDATE(-2);


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

	const std::string   PANEL_SKY_SUN_LAYOUT("sun_layout");
	const std::string   PANEL_SKY_MOON_LAYOUT("moon_layout");

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

	const std::string	FIELD_REFLECTION_PROBE_AMBIANCE("probe_ambiance");

	const std::string	ACTION_SAVELOCAL("save_as_local_setting");
	const std::string	ACTION_SAVEAS("save_as_new_settings");

	const std::string   BTN_RESET("reset");
	const std::string   BTN_SAVE("save");
	const std::string   BTN_DELETE("delete");
	const std::string   BTN_IMPORT("import");

	const std::string   EDITOR_NAME("sky_preset_combo");

	const std::string	BUTTON_NAME_COMMIT("btn_commit");
	const std::string	BUTTON_NAME_FLYOUT("btn_flyout");
	const std::string	XML_FLYOUTMENU_FILE("menu_save_settings_adjust.xml");

	const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
	const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
	const F32 SLIDER_SCALE_GLOW_R(20.0f);
	const F32 SLIDER_SCALE_GLOW_B(-5.0f);
	const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);
}

//=========================================================================
LLFloaterEnvironmentAdjust::LLFloaterEnvironmentAdjust(const LLSD &key):
    LLFloater(key)
{}

LLFloaterEnvironmentAdjust::~LLFloaterEnvironmentAdjust()
{}

//-------------------------------------------------------------------------
BOOL LLFloaterEnvironmentAdjust::postBuild()
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

    getChild<LLUICtrl>(BTN_RESET)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonReset(); });
	getChild<LLUICtrl>(BTN_DELETE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonDelete(); });
	getChild<LLUICtrl>(BTN_IMPORT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonImport(); });

	mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BTN_SAVE, BUTTON_NAME_FLYOUT, XML_FLYOUTMENU_FILE, false);
	mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

	mNameCombo = getChild<LLComboBox>(EDITOR_NAME);
	mNameCombo->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSelectPreset(); });

    getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setCommitCallback([this](LLUICtrl*, const LLSD&) { onReflectionProbeAmbianceChanged(); });

    refresh();
    return TRUE;
}

void LLFloaterEnvironmentAdjust::onOpen(const LLSD& key)
{
    if (!mLiveSky)
    {
        LLEnvironment::instance().saveBeaconsState();
    }
    captureCurrentEnvironment();

    mEventConnection = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32 version){ onEnvironmentUpdated(env, version); });

    LLFloater::onOpen(key);
    refresh();

	gDragonLibrary.loadPresetsFromDir(mNameCombo, "skies");
	gDragonLibrary.addInventoryPresets(mNameCombo, mLiveSky);
}

void LLFloaterEnvironmentAdjust::onClose(bool app_quitting)
{
	if (!mLiveSky)
	{
		LLEnvironment::instance().revertBeaconsState();
		mLiveSky.reset();
	}
    mEventConnection.disconnect();
    LLFloater::onClose(app_quitting);
}


//-------------------------------------------------------------------------
void LLFloaterEnvironmentAdjust::refresh()
{
	if (!mLiveSky)
	{
		setAllChildrenEnabled(FALSE);
		return;
	}

	setEnabled(TRUE);
	setAllChildrenEnabled(TRUE);

	//BD - Atmosphere
	mAmbientLight->set(mLiveSky->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
	mBlueHorizon->set(mLiveSky->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
	mBlueDensity->set(mLiveSky->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
	mHazeHorizon->setValue(mLiveSky->getHazeHorizon());
	mHazeDensity->setValue(mLiveSky->getHazeDensity());
	mSceneGamma->setValue(mLiveSky->getGamma());

	F32 density_mult = mLiveSky->getDensityMultiplier();
	density_mult /= SLIDER_SCALE_DENSITY_MULTIPLIER;
	mDensityMult->setValue(density_mult);
	mDistanceMult->setValue(mLiveSky->getDistanceMultiplier());
	mMaxAltitude->setValue(mLiveSky->getMaxY());

	mMoisture->setValue(mLiveSky->getSkyMoistureLevel());
	mDroplet->setValue(mLiveSky->getSkyDropletRadius());
	mIceLevel->setValue(mLiveSky->getSkyIceLevel());

	static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", true);
	getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->setValue(mLiveSky->getReflectionProbeAmbiance(should_auto_adjust));

	//BD - Sun & Moon
	LLColor3 glow(mLiveSky->getGlow());
	// takes 40 - 0.2 range -> 0 - 1.99 UI range
	mGlowSize->setValue(2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
	mGlowFocus->setValue(glow.mV[2] / SLIDER_SCALE_GLOW_B);
	mStarBrightness->setValue(mLiveSky->getStarBrightness());
	mSunMoonColor->set(mLiveSky->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);
	mSunScale->setValue(mLiveSky->getSunScale());
	mSunImage->setValue(mLiveSky->getSunTextureId());
	mMoonImage->setValue(mLiveSky->getMoonTextureId());
	mMoonScale->setValue(mLiveSky->getMoonScale());
	mMoonBrightness->setValue(mLiveSky->getMoonBrightness());

	F32 pitch, yaw, roll = 0.f;
	mLiveSky->getSunRotation().getEulerAngles(&roll, &pitch, &yaw);
	mSunPositionX->setValue(pitch / F_PI);
	mSunPositionY->setValue(yaw / F_PI);
	mPreviousSunRot = LLVector3(roll, pitch, yaw);

	mLiveSky->getMoonRotation().getEulerAngles(&roll, &pitch, &yaw);
	mMoonPositionX->setValue(pitch / F_PI);
	mMoonPositionY->setValue(yaw / F_PI);
	mPreviousMoonRot = LLVector3(roll, pitch, yaw);

	//BD - Clouds
	mCloudColor->set(mLiveSky->getCloudColor());
	mCloudCoverage->setValue(mLiveSky->getCloudShadow());
	mCloudScale->setValue(mLiveSky->getCloudScale());
	mCloudVariance->setValue(mLiveSky->getCloudVariance());

	LLVector2 cloudScroll(mLiveSky->getCloudScrollRate());
	mCloudScrollX->setValue(cloudScroll[0]);
	mCloudScrollY->setValue(cloudScroll[1]);
	LLEnvironment &environment(LLEnvironment::instance());
	mCloudScrollX->setEnabled(!environment.mCloudScrollXLocked);
	mCloudScrollY->setEnabled(!environment.mCloudScrollYLocked);
	mCloudScrollLockX->setValue(environment.mCloudScrollXLocked);
	mCloudScrollLockY->setValue(environment.mCloudScrollYLocked);

	mCloudImage->setValue(mLiveSky->getCloudNoiseTextureId());

	LLVector3 cloudDensity(mLiveSky->getCloudPosDensity1().getValue());
	mCloudDensityX->setValue(cloudDensity[0]);
	mCloudDensityY->setValue(cloudDensity[1]);
	mCloudDensityD->setValue(cloudDensity[2]);

	LLVector3 cloudDetail(mLiveSky->getCloudPosDensity2().getValue());
	mCloudDetailX->setValue(cloudDetail[0]);
	mCloudDetailY->setValue(cloudDetail[1]);
	mCloudDetailD->setValue(cloudDetail[2]);

	updateGammaLabel();
}


void LLFloaterEnvironmentAdjust::captureCurrentEnvironment()
{
    LLEnvironment &environment(LLEnvironment::instance());
    bool updatelocal(false);

    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {   // We have a full day cycle in the local environment.  Freeze the sky
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)->buildClone();
            updatelocal = true;
        }
        else
        {   // otherwise we can just use the sky.
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
        }
    }
    else
    {
        mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true)->buildClone();
        updatelocal = true;
    }

    if (updatelocal)
    {
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveSky, FLOATER_ENVIRONMENT_UPDATE);
    }
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    environment.updateEnvironment(LLEnvironment::TRANSITION_INSTANT);

	mSunImageChanged = false;
	mCloudImageChanged = false;
	mMoonImageChanged = false;
}

void LLFloaterEnvironmentAdjust::onButtonReset()
{
    LLNotificationsUtil::add("PersonalSettingsConfirmReset", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            //this->closeFloater();
            LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
            LLEnvironment::instance().updateEnvironment();
        }
    }); 
}


void LLFloaterEnvironmentAdjust::onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version)
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


//BD - Atmosphere
void LLFloaterEnvironmentAdjust::onAmbientLightChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setAmbientColor(LLColor3(mAmbientLight->get() * SLIDER_SCALE_SUN_AMBIENT));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onBlueHorizonChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setBlueHorizon(LLColor3(mBlueHorizon->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onBlueDensityChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setBlueDensity(LLColor3(mBlueDensity->get() * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onHazeHorizonChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setHazeHorizon(mHazeHorizon->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onHazeDensityChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setHazeDensity(mHazeDensity->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSceneGammaChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setGamma(mSceneGamma->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onDensityMultipChanged()
{
	if (!mLiveSky) return;
	F32 density_mult = mDensityMult->getValue().asReal();
	density_mult *= SLIDER_SCALE_DENSITY_MULTIPLIER;
	mLiveSky->setDensityMultiplier(density_mult);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onDistanceMultipChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setDistanceMultiplier(mDistanceMult->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onMaxAltChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setMaxY(mMaxAltitude->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onMoistureLevelChanged()
{
	if (!mLiveSky) return;
	F32 moisture_level = mMoisture->getValue().asReal();
	mLiveSky->setSkyMoistureLevel(moisture_level);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onDropletRadiusChanged()
{
	if (!mLiveSky) return;
	F32 droplet_radius = mDroplet->getValue().asReal();
	mLiveSky->setSkyDropletRadius(droplet_radius);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onIceLevelChanged()
{
	if (!mLiveSky) return;
	F32 ice_level = mIceLevel->getValue().asReal();
	mLiveSky->setSkyIceLevel(ice_level);
	mLiveSky->update();
}


//BD - Sun & Moon
void LLFloaterEnvironmentAdjust::onSunMoonColorChanged()
{
	if (!mLiveSky) return;
	LLColor3 color(mSunMoonColor->get());

	color *= SLIDER_SCALE_SUN_AMBIENT;

	mLiveSky->setSunlightColor(color);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onGlowChanged()
{
	if (!mLiveSky) return;
	LLColor3 glow(mGlowSize->getValue().asReal(), 0.0f, mGlowFocus->getValue().asReal());

	// takes 0 - 1.99 UI range -> 40 -> 0.2 range
	glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R;
	glow.mV[2] *= SLIDER_SCALE_GLOW_B;

	mLiveSky->setGlow(glow);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onStarBrightnessChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setStarBrightness(mStarBrightness->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSunRotationChanged()
{
	if (!mLiveSky) return;

	LLQuaternion rot;
	LLQuaternion delta;
	rot.setAngleAxis(mSunPositionX->getValue().asReal() * F_PI, 0, 1, 0);
	delta.setAngleAxis(mSunPositionY->getValue().asReal() * F_PI, 0, 0, 1);
	rot *= delta;

	mLiveSky->setSunRotation(rot);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSunScaleChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setSunScale((mSunScale->getValue().asReal()));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onSunImageChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setSunTextureId(mSunImage->getValue().asUUID());
	mLiveSky->update();
	mSunImageChanged = true;
}

void LLFloaterEnvironmentAdjust::onMoonRotationChanged()
{
	if (!mLiveSky) return;

	LLQuaternion rot;
	LLQuaternion delta;
	rot.setAngleAxis(mMoonPositionX->getValue().asReal() * F_PI, 0, 1, 0);
	delta.setAngleAxis(mMoonPositionY->getValue().asReal() * F_PI, 0, 0, 1);
	rot *= delta;

	mLiveSky->setMoonRotation(rot);
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onMoonImageChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setMoonTextureId(mMoonImage->getValue().asUUID());
	mLiveSky->update();
	mMoonImageChanged = true;
}

void LLFloaterEnvironmentAdjust::onMoonScaleChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setMoonScale((mMoonScale->getValue().asReal()));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onMoonBrightnessChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setMoonBrightness((mMoonBrightness->getValue().asReal()));
	mLiveSky->update();
}



//BD - Clouds
void LLFloaterEnvironmentAdjust::onCloudColorChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setCloudColor(LLColor3(mCloudColor->get()));
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onCloudCoverageChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setCloudShadow(mCloudCoverage->getValue().asReal());
	mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::onCloudScaleChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setCloudScale(mCloudScale->getValue().asReal());
}

void LLFloaterEnvironmentAdjust::onCloudVarianceChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setCloudVariance(mCloudVariance->getValue().asReal());
}

void LLFloaterEnvironmentAdjust::onCloudScrollChanged()
{
	if (!mLiveSky) return;
	LLVector2 scroll(mCloudScrollX->getValue().asReal(), mCloudScrollY->getValue().asReal());
	mLiveSky->setCloudScrollRate(scroll);
}

void LLFloaterEnvironmentAdjust::onCloudMapChanged()
{
	if (!mLiveSky) return;
	mLiveSky->setCloudNoiseTextureId(mCloudImage->getValue().asUUID());
	mCloudImageChanged = true;
}

void LLFloaterEnvironmentAdjust::onCloudDensityChanged()
{
	if (!mLiveSky) return;
	LLColor3 density(mCloudDensityX->getValue().asReal(), mCloudDensityY->getValue().asReal(), mCloudDensityD->getValue().asReal());
	mLiveSky->setCloudPosDensity1(density);
}

void LLFloaterEnvironmentAdjust::onCloudDetailChanged()
{
	if (!mLiveSky) return;
	LLColor3 detail(mCloudDetailX->getValue().asReal(),	mCloudDetailY->getValue().asReal(),	mCloudDetailD->getValue().asReal());
	mLiveSky->setCloudPosDensity2(detail);
}

void LLFloaterEnvironmentAdjust::onCloudScrollXLocked(bool lock)
{
	if (!mLiveSky) return;
	LLEnvironment::instance().pauseCloudScrollX(lock);
	refresh();
}

void LLFloaterEnvironmentAdjust::onCloudScrollYLocked(bool lock)
{
	if (!mLiveSky) return;
	LLEnvironment::instance().pauseCloudScrollY(lock);
	refresh();
}

void LLFloaterEnvironmentAdjust::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
	std::string ctrl_action = ctrl->getName();

	std::string local_desc;
	LLSettingsBase::ptr_t setting_clone;
	bool is_local = false; // because getString can be empty
	if (mLiveSky)
	{
		setting_clone = mLiveSky->buildClone();
		if (LLLocalBitmapMgr::getInstance()->isLocal(mLiveSky->getSunTextureId()))
		{
			local_desc = LLTrans::getString("EnvironmentSun");
			is_local = true;
		}
		else if (LLLocalBitmapMgr::getInstance()->isLocal(mLiveSky->getMoonTextureId()))
		{
			local_desc = LLTrans::getString("EnvironmentMoon");
			is_local = true;
		}
		else if (LLLocalBitmapMgr::getInstance()->isLocal(mLiveSky->getCloudNoiseTextureId()))
		{
			local_desc = LLTrans::getString("EnvironmentCloudNoise");
			is_local = true;
		}
		else if (LLLocalBitmapMgr::getInstance()->isLocal(mLiveSky->getBloomTextureId()))
		{
			local_desc = LLTrans::getString("EnvironmentBloom");
			is_local = true;
		}
	}

	if (is_local)
	{
		LLSD args;
		args["FIELD"] = local_desc;
		LLNotificationsUtil::add("WLLocalTextureFixedBlock", args);
		return;
	}

	if (ctrl_action == ACTION_SAVELOCAL)
	{
		onButtonSave();
	}
	else if (ctrl_action == ACTION_SAVEAS)
	{
		LLSD args;
		args["DESC"] = mLiveSky->getName();
		LLNotificationsUtil::add("SaveSettingAs", args, LLSD(), boost::bind(&LLFloaterEnvironmentAdjust::onSaveAsCommit, this, _1, _2, setting_clone));
	}
	/*else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
		(ctrl_action == ACTION_APPLY_PARCEL) ||
		(ctrl_action == ACTION_APPLY_REGION))
	{
		doApplyEnvironment(ctrl_action, setting_clone);
	}*/
	else
	{
		LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
	}
}

void LLFloaterEnvironmentAdjust::onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsBase::ptr_t &settings)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		std::string settings_name = response["message"].asString();

		LLInventoryObject::correctInventoryName(settings_name);
		if (settings_name.empty())
		{
			// Ideally notification should disable 'OK' button if name won't fit our requirements,
			// for now either display notification, or use some default name
			settings_name = "Unnamed";
		}

		doApplyCreateNewInventory(settings_name, settings);
	}
}

void LLFloaterEnvironmentAdjust::onReflectionProbeAmbianceChanged()
{
    if (!mLiveSky) return;
    F32 ambiance = getChild<LLUICtrl>(FIELD_REFLECTION_PROBE_AMBIANCE)->getValue().asReal();
    mLiveSky->setReflectionProbeAmbiance(ambiance);

    updateGammaLabel();
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::updateGammaLabel()
{
    if (!mLiveSky) return;

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", true);
    F32 ambiance = mLiveSky->getReflectionProbeAmbiance(should_auto_adjust);
    if (ambiance != 0.f)
    {
        childSetValue("scene_gamma_label", getString("hdr_string"));
    }
    else
    {
        childSetValue("scene_gamma_label", getString("brightness_string"));
    }
}

void LLFloaterEnvironmentAdjust::doApplyCreateNewInventory(std::string settings_name, const LLSettingsBase::ptr_t &settings)
{
	LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
	// This method knows what sort of settings object to create.
	LLSettingsVOBase::createInventoryItem(settings, parent_id, settings_name,
		[this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
}

void LLFloaterEnvironmentAdjust::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
	LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

	if (inventory_id.isNull() || !results["success"].asBoolean())
	{
		LLNotificationsUtil::add("CantCreateInventory");
		return;
	}
}

//BD - Windlight Stuff
//=====================================================================================================
void LLFloaterEnvironmentAdjust::onButtonSave()
{
	if (!mLiveSky) return;

	LLSettingsSky::ptr_t sky = mLiveSky->buildClone();

	gDragonLibrary.savePreset(mNameCombo->getValue(), sky);
	gDragonLibrary.loadPresetsFromDir(mNameCombo, "skies");
	gDragonLibrary.addInventoryPresets(mNameCombo, sky);
}

void LLFloaterEnvironmentAdjust::onButtonDelete()
{
	if (!mLiveSky) return;
	gDragonLibrary.deletePreset(mNameCombo->getValue(), "skies");
	gDragonLibrary.loadPresetsFromDir(mNameCombo, "skies");
	gDragonLibrary.addInventoryPresets(mNameCombo, mLiveSky);
}

void LLFloaterEnvironmentAdjust::onButtonImport()
{   // Load a a legacy Windlight XML from disk.
	LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterEnvironmentAdjust::loadSkySettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false);
}

void LLFloaterEnvironmentAdjust::loadSkySettingFromFile(const std::vector<std::string>& filenames)
{
	if (!mLiveSky) return;
	if (filenames.size() < 1) return;
	std::string filename = filenames[0];
	gDragonLibrary.loadPreset(filename, mLiveSky);
	refresh();
}

void LLFloaterEnvironmentAdjust::onSelectPreset()
{
	if (!mLiveSky) return;
	gDragonLibrary.onSelectPreset(mNameCombo, mLiveSky);
	refresh();
}