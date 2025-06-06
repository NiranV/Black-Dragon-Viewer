/**
* @file llpaneleditsky.h
* @brief Panels for sky settings
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

#ifndef LLPANEL_EDIT_SKY_H
#define LLPANEL_EDIT_SKY_H

#include "llpanel.h"
#include "llsettingssky.h"
#include "llfloaterfixedenvironment.h"

//=========================================================================
class LLSlider;
class LLColorSwatchCtrl;
class LLTextureCtrl;

//=========================================================================
class LLPanelSettingsSky : public LLSettingsEditPanel
{
	LOG_CLASS(LLPanelSettingsSky);

public:
	LLPanelSettingsSky();

	virtual void            setSettings(const LLSettingsBase::ptr_t &settings) override { setSky(std::static_pointer_cast<LLSettingsSky>(settings)); }

	LLSettingsSky::ptr_t    getSky() const { return mSkySettings; }
	void                    setSky(const LLSettingsSky::ptr_t &sky) { mSkySettings = sky; clearIsDirty(); refresh(); }

protected:
	LLSettingsSky::ptr_t  mSkySettings;
};

class LLPanelSettingsSkyAtmosTab : public LLPanelSettingsSky
{
	LOG_CLASS(LLPanelSettingsSkyAtmosTab);

public:
	LLPanelSettingsSkyAtmosTab();

    virtual bool            postBuild() override;

protected:
	virtual void            refresh() override;

private:
	void                    onAmbientLightChanged();
	void                    onBlueHorizonChanged();
	void                    onBlueDensityChanged();
	void                    onHazeHorizonChanged();
	void                    onHazeDensityChanged();
	void                    onSceneGammaChanged();
	void                    onDensityMultipChanged();
	void                    onDistanceMultipChanged();
	void                    onMaxAltChanged();
	void                    onReflectionProbeAmbianceChanged();
	void                    updateGammaLabel(bool auto_adjust = false);

	//BD - Atmosphere
	LLColorSwatchCtrl* mAmbientLight;
	LLColorSwatchCtrl* mBlueHorizon;
	LLColorSwatchCtrl* mBlueDensity;
	LLUICtrl* mHazeHorizon;
	LLUICtrl* mHazeDensity;
	LLUICtrl* mSceneGamma;
	LLUICtrl* mDensityMult;
	LLUICtrl* mDistanceMult;
	LLUICtrl* mMaxAltitude;
};

class LLPanelSettingsSkyCloudTab : public LLPanelSettingsSky
{
	LOG_CLASS(LLPanelSettingsSkyCloudTab);

public:
	LLPanelSettingsSkyCloudTab();

    virtual bool            postBuild() override;

protected:
	virtual void            refresh() override;

private:
	void                    onCloudColorChanged();
	void                    onCloudCoverageChanged();
	void                    onCloudScaleChanged();
	void                    onCloudVarianceChanged();
	void                    onCloudScrollChanged();
	void                    onCloudMapChanged();
	void                    onCloudDensityChanged();
	void                    onCloudDetailChanged();
	void					onCloudScrollXLocked(bool lock);
	void					onCloudScrollYLocked(bool lock);

	//BD - Clouds
	LLColorSwatchCtrl* mCloudColor;
	LLUICtrl* mCloudCoverage;
	LLUICtrl* mCloudScale;
	LLUICtrl* mCloudVariance;
	LLUICtrl* mCloudScrollX;
	LLUICtrl* mCloudScrollY;
	LLTextureCtrl* mCloudImage;
	LLUICtrl* mCloudDensityX;
	LLUICtrl* mCloudDensityY;
	LLUICtrl* mCloudDensityD;
	LLUICtrl* mCloudDetailX;
	LLUICtrl* mCloudDetailY;
	LLUICtrl* mCloudDetailD;
	LLUICtrl* mCloudScrollLockX;
	LLUICtrl* mCloudScrollLockY;
};

class LLPanelSettingsSkySunMoonTab : public LLPanelSettingsSky
{
	LOG_CLASS(LLPanelSettingsSkySunMoonTab);

public:
	LLPanelSettingsSkySunMoonTab();

    virtual bool            postBuild() override;

protected:
	virtual void            refresh() override;

private:
	void                    onSunMoonColorChanged();
	void                    onGlowChanged();
	void                    onStarBrightnessChanged();
	void                    onSunRotationChanged();
	void                    onSunScaleChanged();
	void                    onSunImageChanged();
	void                    onMoonRotationChanged();
	void                    onMoonScaleChanged();
	void                    onMoonBrightnessChanged();
	void                    onMoonImageChanged();

	//BD - Sun & Moon
	LLColorSwatchCtrl* mSunMoonColor;
	LLUICtrl* mGlowFocus;
	LLUICtrl* mGlowSize;
	LLUICtrl* mStarBrightness;
	LLUICtrl* mSunPositionX;
	LLUICtrl* mSunPositionY;
	LLTextureCtrl* mSunImage;
	LLUICtrl* mSunScale;
	LLUICtrl* mMoonPositionX;
	LLUICtrl* mMoonPositionY;
	LLTextureCtrl* mMoonImage;
	LLUICtrl* mMoonScale;
	LLUICtrl* mMoonBrightness;
	LLUICtrl* mMoonBeacon;
	LLUICtrl* mSunBeacon;
};

// single subtab of the density settings tab
class LLPanelSettingsSkyDensityTab : public LLPanelSettingsSky
{
	LOG_CLASS(LLPanelSettingsSkyDensityTab);

public:
	LLPanelSettingsSkyDensityTab();

    virtual bool postBuild() override;

protected:
	virtual void refresh() override;

	void onRayleighExponentialChanged();
	void onRayleighExponentialScaleChanged();
	void onRayleighLinearChanged();
	void onRayleighConstantChanged();
	void onRayleighMaxAltitudeChanged();

	void onMieExponentialChanged();
	void onMieExponentialScaleChanged();
	void onMieLinearChanged();
	void onMieConstantChanged();
	void onMieAnisoFactorChanged();
	void onMieMaxAltitudeChanged();

	void onAbsorptionExponentialChanged();
	void onAbsorptionExponentialScaleChanged();
	void onAbsorptionLinearChanged();
	void onAbsorptionConstantChanged();
	void onAbsorptionMaxAltitudeChanged();

	// update the settings for our profile type
	void updateProfile();

private:
	void                    onMoistureLevelChanged();
	void                    onDropletRadiusChanged();
	void                    onIceLevelChanged();

	LLUICtrl* mMoisture;
	LLUICtrl* mDroplet;
	LLUICtrl* mIceLevel;
};
#endif // LLPANEL_EDIT_SKY_H
