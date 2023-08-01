/** 
 * @file llfloaterenvironmentadjust.h
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

#ifndef LL_FLOATERENVIRONMENTADJUST_H
#define LL_FLOATERENVIRONMENTADJUST_H

#include "llfloater.h"
#include "llsettingsbase.h"
#include "llsettingssky.h"
#include "llenvironment.h"
#include "llflyoutcombobtn.h"

#include "boost/signals2.hpp"

class LLButton;
class LLLineEditor;
class LLColorSwatchCtrl;
class LLTextureCtrl;
class LLComboBox;

/**
 * Floater container for taking a snapshot of the current environment and making minor adjustments.
 */
class LLFloaterEnvironmentAdjust : public LLFloater
{
    LOG_CLASS(LLFloaterEnvironmentAdjust);

public:
                                LLFloaterEnvironmentAdjust(const LLSD &key);
    virtual                     ~LLFloaterEnvironmentAdjust();


    virtual BOOL                postBuild() override;
    virtual void                onOpen(const LLSD& key) override;
    virtual void                onClose(bool app_quitting) override;

    virtual void                refresh() override;

private:
    void                        captureCurrentEnvironment();
    void                        onWaterMapChanged();

	void                        onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version);

	void						onButtonApply(LLUICtrl *ctrl, const LLSD &data);
	void						onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsBase::ptr_t &settings);
	void						onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results);
	virtual void				doApplyCreateNewInventory(std::string settings_name, const LLSettingsBase::ptr_t &settings);

	//BD - Atmosphere
	void						onAmbientLightChanged();
	void						onBlueHorizonChanged();
	void						onBlueDensityChanged();
	void						onHazeHorizonChanged();
	void						onHazeDensityChanged();
	void						onSceneGammaChanged();
	void						onDensityMultipChanged();
	void						onDistanceMultipChanged();
	void						onMaxAltChanged();
	void						onMoistureLevelChanged();
	void						onDropletRadiusChanged();
	void						onIceLevelChanged();
	void                        updateGammaLabel();

	//BD - Sun & Moon
	void						onSunMoonColorChanged();
	void						onGlowChanged();
	void						onStarBrightnessChanged();
	void						onSunRotationChanged();
	void						onSunScaleChanged();
	void						onSunImageChanged();
	void						onMoonRotationChanged();
	void						onMoonScaleChanged();
	void						onMoonBrightnessChanged();
	void						onMoonImageChanged();
	void                        onReflectionProbeAmbianceChanged();

	//BD - Clouds
	void						onCloudColorChanged();
	void						onCloudCoverageChanged();
	void						onCloudScaleChanged();
	void						onCloudVarianceChanged();
	void						onCloudScrollChanged();
	void						onCloudMapChanged();
	void						onCloudDensityChanged();
	void						onCloudDetailChanged();
	void						onCloudScrollXLocked(bool lock);
	void						onCloudScrollYLocked(bool lock);

	//BD - Windlight Stuff
	void						onButtonReset();
	void						onButtonSave();
	void						onButtonDelete();
	void						onButtonImport();
	void						onSelectPreset();

	void						loadSkySettingFromFile(const std::vector<std::string>& filenames);

    LLSettingsSky::ptr_t        mLiveSky;
    LLEnvironment::connection_t mEventConnection;

	bool						mSunImageChanged;
	bool						mCloudImageChanged;
	bool						mMoonImageChanged;

	//BD - Atmosphere
	LLColorSwatchCtrl*			mAmbientLight;
	LLColorSwatchCtrl*			mBlueHorizon;
	LLColorSwatchCtrl*			mBlueDensity;
	LLUICtrl*					mHazeHorizon;
	LLUICtrl*					mHazeDensity;
	LLUICtrl*					mSceneGamma;
	LLUICtrl*					mDensityMult;
	LLUICtrl*					mDistanceMult;
	LLUICtrl*					mMaxAltitude;
	LLUICtrl*					mMoisture;
	LLUICtrl*					mDroplet;
	LLUICtrl*					mIceLevel;

	//BD - Sun & Moon
	LLColorSwatchCtrl*			mSunMoonColor;
	LLUICtrl*					mGlowFocus;
	LLUICtrl*					mGlowSize;
	LLUICtrl*					mStarBrightness;
	LLUICtrl*					mSunPositionX;
	LLUICtrl*					mSunPositionY;
	LLTextureCtrl*				mSunImage;
	LLUICtrl*					mSunScale;
	LLUICtrl*					mMoonPositionX;
	LLUICtrl*					mMoonPositionY;
	LLTextureCtrl*				mMoonImage;
	LLUICtrl*					mMoonScale;
	LLUICtrl*					mMoonBrightness;

	//BD - Clouds
	LLColorSwatchCtrl*			mCloudColor;
	LLUICtrl*					mCloudCoverage;
	LLUICtrl*					mCloudScale;
	LLUICtrl*					mCloudVariance;
	LLUICtrl*					mCloudScrollX;
	LLUICtrl*					mCloudScrollY;
	LLTextureCtrl*				mCloudImage;
	LLUICtrl*					mCloudDensityX;
	LLUICtrl*					mCloudDensityY;
	LLUICtrl*					mCloudDensityD;
	LLUICtrl*					mCloudDetailX;
	LLUICtrl*					mCloudDetailY;
	LLUICtrl*					mCloudDetailD;
	LLUICtrl*					mCloudScrollLockX;
	LLUICtrl*					mCloudScrollLockY;

	LLComboBox*					mNameCombo;
	LLFlyoutComboBtnCtrl *      mFlyoutControl;

	LLVector3					mPreviousMoonRot;
	LLVector3					mPreviousSunRot;
};

#endif // LL_FLOATERFIXEDENVIRONMENT_H
