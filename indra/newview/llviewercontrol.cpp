/**
 * @file llviewercontrol.cpp
 * @brief Viewer configuration
 * @author Richard Nelson
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

#include "llviewercontrol.h"

// Library includes
#include "llwindow.h"   // getGamma()

// For Listeners
#include "llaudioengine.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llconsole.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolterrain.h"
#include "llflexibleobject.h"
#include "llfeaturemanager.h"
#include "llviewershadermgr.h"

#include "llsky.h"
#include "llvieweraudio.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvoiceclient.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llvlcomposition.h"
#include "pipeline.h"
#include "llviewerjoystick.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llkeyboard.h"
#include "llerrorcontrol.h"
#include "llappviewer.h"
#include "llvosurfacepatch.h"
#include "llvowlsky.h"
#include "llrender.h"
#include "llnavigationbar.h"
#include "llnotificationsutil.h"
#include "llfloatertools.h"
#include "llpaneloutfitsinventory.h"
#include "llpanellogin.h"
#include "llspellcheck.h"
#include "llslurl.h"
#include "llstartup.h"
#include "llperfstats.h"


//BD - Includes we need for special features
#include "llvoavatar.h"
#include "llviewerregion.h"
#include "lldrawpoolwlsky.h"
#include "llfloatersnapshot.h"
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "llviewerobjectlist.h"
#include "bdsidebar.h"
#include "bdfunctions.h"
#include "bdstatus.h"
#include "llheadrotmotion.h"

#if LL_DARWIN
#include "llwindowmacosx.h"
#endif

// Third party library includes
#include <boost/algorithm/string.hpp>

#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
bool                gHackGodmode = false;
#endif

// Should you contemplate changing the name "Global", please first grep for
// that string literal. There are at least a couple other places in the C++
// code that assume the LLControlGroup named "Global" is gSavedSettings.
LLControlGroup gSavedSettings("Global");    // saved at end of session
LLControlGroup gSavedPerAccountSettings("PerAccount"); // saved at end of session
LLControlGroup gCrashSettings("CrashSettings"); // saved at end of session
LLControlGroup gWarningSettings("Warnings"); // persists ignored dialogs/warnings

std::string gLastRunVersion;
//BD - Freeze World
std::vector<LLAnimPauseRequest>	mAvatarPauseHandles;

extern bool gResizeScreenTexture;
extern bool gResizeShadowTexture;
extern bool gDebugGL;
////////////////////////////////////////////////////////////////////////////
// Listeners

static bool handleRenderAvatarMouselookChanged(const LLSD& newvalue)
{
    LLVOAvatar::sVisibleInFirstPerson = newvalue.asBoolean();
    return true;
}

static bool handleRenderFarClipChanged(const LLSD& newvalue)
{
    if (LLStartUp::getStartupState() >= STATE_STARTED)
    {
        F32 draw_distance = (F32)newvalue.asReal();
    gAgentCamera.mDrawDistance = draw_distance;
    LLWorld::getInstance()->setLandFarClip(draw_distance);
    return true;
    }
    return false;
}

static bool handleTerrainScaleChanged(const LLSD& newvalue)
{
    F64 scale = newvalue.asReal();
    if (scale != 0.0)
    {
        LLDrawPoolTerrain::sDetailScale = F32(1.0 / scale);
    }
    return true;
}

static bool handlePBRTerrainScaleChanged(const LLSD& newvalue)
{
    F64 scale = newvalue.asReal();
    if (scale != 0.0)
    {
        LLDrawPoolTerrain::sPBRDetailScale = F32(1.0 / scale);
    }
    return true;
}

static bool handleDebugAvatarJointsChanged(const LLSD& newvalue)
{
    std::string new_string = newvalue.asString();
    LLJoint::setDebugJointNames(new_string);
    return true;
}

static bool handleAvatarHoverOffsetChanged(const LLSD& newvalue)
{
    if (isAgentAvatarValid())
    {
        gAgentAvatarp->setHoverIfRegionEnabled();
    }
    return true;
}


static bool handleSetShaderChanged(const LLSD& newvalue)
{
    // changing shader level may invalidate existing cached bump maps, as the shader type determines the format of the bump map it expects - clear and repopulate the bump cache
    gBumpImageList.destroyGL();
    gBumpImageList.restoreGL();

    if (gPipeline.isInit())
    {
        // ALM depends onto atmospheric shaders, state might have changed
        LLPipeline::refreshCachedSettings();
    }

    // else, leave terrain detail as is
    LLViewerShaderMgr::instance()->setShaders();
    return true;
}

static bool handleRenderPerfTestChanged(const LLSD& newvalue)
{
       bool status = !newvalue.asBoolean();
       if (!status)
       {
               gPipeline.clearRenderTypeMask(LLPipeline::RENDER_TYPE_WL_SKY,
                                                                         LLPipeline::RENDER_TYPE_TERRAIN,
                                                                         LLPipeline::RENDER_TYPE_GRASS,
                                                                         LLPipeline::RENDER_TYPE_TREE,
                                                                         LLPipeline::RENDER_TYPE_WATER,
                                                                         LLPipeline::RENDER_TYPE_PASS_GRASS,
                                                                         LLPipeline::RENDER_TYPE_HUD,
                                                                         LLPipeline::RENDER_TYPE_CLOUDS,
                                                                         LLPipeline::RENDER_TYPE_HUD_PARTICLES,
                                                                         LLPipeline::END_RENDER_TYPES);
               gPipeline.setRenderDebugFeatureControl(LLPipeline::RENDER_DEBUG_FEATURE_UI, false);
       }
       else
       {
               gPipeline.setRenderTypeMask(LLPipeline::RENDER_TYPE_WL_SKY,
                                                                         LLPipeline::RENDER_TYPE_TERRAIN,
                                                                         LLPipeline::RENDER_TYPE_GRASS,
                                                                         LLPipeline::RENDER_TYPE_TREE,
                                                                         LLPipeline::RENDER_TYPE_WATER,
                                                                         LLPipeline::RENDER_TYPE_PASS_GRASS,
                                                                         LLPipeline::RENDER_TYPE_HUD,
                                                                         LLPipeline::RENDER_TYPE_CLOUDS,
                                                                         LLPipeline::RENDER_TYPE_HUD_PARTICLES,
                                                                         LLPipeline::END_RENDER_TYPES);
               gPipeline.setRenderDebugFeatureControl(LLPipeline::RENDER_DEBUG_FEATURE_UI, true);
       }

       return true;
}

bool handleRenderTransparentWaterChanged(const LLSD& newvalue)
{
    if (gPipeline.isInit())
    {
        gPipeline.updateRenderTransparentWater();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        LLViewerShaderMgr::instance()->setShaders();
    }
    LLWorld::getInstance()->updateWaterObjects();
    return true;
}

static bool handleShadowsResized(const LLSD& newvalue)
{
    gPipeline.requestResizeShadowTexture();
    return true;
}

static bool handleWindowResized(const LLSD& newvalue)
{
    gPipeline.requestResizeScreenTexture();
    return true;
}

static bool handleReleaseGLBufferChanged(const LLSD& newvalue)
{
    if (gPipeline.isInit())
    {
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
    }
    return true;
}

static bool handleEnableEmissiveChanged(const LLSD& newvalue)
{
    return handleReleaseGLBufferChanged(newvalue) && handleSetShaderChanged(newvalue);
}

static bool handleDisableVintageMode(const LLSD& newvalue)
{
    gSavedSettings.setBOOL("RenderEnableEmissiveBuffer", newvalue.asBoolean());
    gSavedSettings.setBOOL("RenderHDREnabled", newvalue.asBoolean());
    return true;
}

static bool handleEnableHDR(const LLSD& newvalue)
{
    gPipeline.mReflectionMapManager.reset();
    gPipeline.mHeroProbeManager.reset();
    return handleReleaseGLBufferChanged(newvalue) && handleSetShaderChanged(newvalue);
}

static bool handleLUTBufferChanged(const LLSD& newvalue)
{
    if (gPipeline.isInit())
    {
        gPipeline.releaseLUTBuffers();
        gPipeline.createLUTBuffers();
    }
    return true;
}

static bool handleAnisotropicChanged(const LLSD& newvalue)
{
    LLImageGL::sGlobalUseAnisotropic = newvalue.asBoolean();
    LLImageGL::dirtyTexOptions();
    return true;
}

static bool handleVSyncChanged(const LLSD& newvalue)
{
    LLPerfStats::tunables.vsyncEnabled = newvalue.asBoolean();
    if (gViewerWindow && gViewerWindow->getWindow())
    {
        gViewerWindow->getWindow()->toggleVSync(newvalue.asBoolean());

        if (newvalue.asBoolean())
        {
            U32 current_target = gSavedSettings.getU32("TargetFPS");
            gSavedSettings.setU32("TargetFPS", std::min((U32)gViewerWindow->getWindow()->getRefreshRate(), current_target));
        }
    }

    return true;
}

static bool handleVolumeLODChanged(const LLSD& newvalue)
{
    LLVOVolume::sLODFactor = llclamp((F32) newvalue.asReal(), 0.01f, MAX_LOD_FACTOR);
    LLVOVolume::sDistanceFactor = 1.f-LLVOVolume::sLODFactor * 0.1f;
    return true;
}

static bool handleAvatarLODChanged(const LLSD& newvalue)
{
    LLVOAvatar::sLODFactor = llclamp((F32) newvalue.asReal(), 0.f, MAX_AVATAR_LOD_FACTOR);
    return true;
}

static bool handleAvatarPhysicsLODChanged(const LLSD& newvalue)
{
    LLVOAvatar::sPhysicsLODFactor = llclamp((F32) newvalue.asReal(), 0.f, MAX_AVATAR_LOD_FACTOR);
    return true;
}

static bool handleTerrainLODChanged(const LLSD& newvalue)
{
    LLVOSurfacePatch::sLODFactor = (F32)newvalue.asReal();
    //sqaure lod factor to get exponential range of [0,4] and keep
    //a value of 1 in the middle of the detail slider for consistency
    //with other detail sliders (see panel_preferences_graphics1.xml)
    LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor;
    return true;
}

static bool handleTreeLODChanged(const LLSD& newvalue)
{
    LLVOTree::sTreeFactor = (F32) newvalue.asReal();
    return true;
}

static bool handleFlexLODChanged(const LLSD& newvalue)
{
    LLVolumeImplFlexible::sUpdateFactor = (F32) newvalue.asReal();
    return true;
}

static bool handleGammaChanged(const LLSD& newvalue)
{
    F32 gamma = (F32) newvalue.asReal();
    if (gamma == 0.0f)
    {
        gamma = 1.0f; // restore normal gamma
    }
    if (gViewerWindow && gViewerWindow->getWindow() && gamma != gViewerWindow->getWindow()->getGamma())
    {
        // Only save it if it's changed
        if (!gViewerWindow->getWindow()->setGamma(gamma))
        {
            LL_WARNS() << "setGamma failed!" << LL_ENDL;
        }
    }

    return true;
}

const F32 MAX_USER_FOG_RATIO = 10.f;
const F32 MIN_USER_FOG_RATIO = 0.5f;

static bool handleFogRatioChanged(const LLSD& newvalue)
{
    F32 fog_ratio = llmax(MIN_USER_FOG_RATIO, llmin((F32) newvalue.asReal(), MAX_USER_FOG_RATIO));
    gSky.setFogRatio(fog_ratio);
    return true;
}

static bool handleMaxPartCountChanged(const LLSD& newvalue)
{
    LLViewerPartSim::setMaxPartCount(newvalue.asInteger());
    return true;
}

static bool handleChatFontSizeChanged(const LLSD& newvalue)
{
    if(gConsole)
    {
        gConsole->setFontSize(newvalue.asInteger());
    }
    return true;
}

static bool handleConsoleMaxLinesChanged(const LLSD& newvalue)
{
    if(gConsole)
    {
        gConsole->setMaxLines(newvalue.asInteger());
    }
    return true;
}

static void handleAudioVolumeChanged(const LLSD& newvalue)
{
    audio_update_volume(true);
}

static bool handleJoystickChanged(const LLSD& newvalue)
{
	gJoystick->setCameraNeedsUpdate(true);
	gJoystick->refreshAxesMapping();
	return true;
}

static bool handleUseOcclusionChanged(const LLSD& newvalue)
{
    LLPipeline::sUseOcclusion = (newvalue.asBoolean()
        && LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") && !gUseWireframe) ? 2 : 0;
    return true;
}

static bool handleUploadBakedTexOldChanged(const LLSD& newvalue)
{
    LLPipeline::sForceOldBakedUpload = newvalue.asBoolean();
    return true;
}


static bool handleWLSkyDetailChanged(const LLSD&)
{
    if (gSky.mVOWLSkyp.notNull())
    {
        gSky.mVOWLSkyp->updateGeometry(gSky.mVOWLSkyp->mDrawable);
    }
    return true;
}

static bool handleRepartition(const LLSD&)
{
    if (gPipeline.isInit())
    {
        gOctreeMaxCapacity = gSavedSettings.getU32("OctreeMaxNodeCapacity");
        gOctreeMinSize = gSavedSettings.getF32("OctreeMinimumNodeSize");
        gObjectList.repartitionObjects();
    }
    return true;
}

static bool handleRenderDynamicLODChanged(const LLSD& newvalue)
{
    LLPipeline::sDynamicLOD = newvalue.asBoolean();
    return true;
}

static bool handleReflectionProbeDetailChanged(const LLSD& newvalue)
{
    if (gPipeline.isInit())
    {
        LLPipeline::refreshCachedSettings();
        gPipeline.mReflectionMapManager.reset();
        gPipeline.mHeroProbeManager.reset();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
        LLViewerShaderMgr::instance()->setShaders();
    }
    return true;
}

#if LL_DARWIN
static bool handleAppleUseMultGLChanged(const LLSD& newvalue)
{
    if (gGLManager.mInited)
    {
        LLWindowMacOSX::setUseMultGL(newvalue.asBoolean());
    }
    return true;
}
#endif

static bool handleHeroProbeResolutionChanged(const LLSD &newvalue)
{
    if (gPipeline.isInit())
    {
        LLPipeline::refreshCachedSettings();
        gPipeline.mHeroProbeManager.reset();
        gPipeline.releaseGLBuffers();
        gPipeline.createGLBuffers();
    }
    return true;
}

static bool handleRenderDebugPipelineChanged(const LLSD& newvalue)
{
    gDebugPipeline = newvalue.asBoolean();
    return true;
}

static bool handleRenderResolutionDivisorChanged(const LLSD&)
{
    gResizeScreenTexture = true;
    return true;
}

static bool handleDebugViewsChanged(const LLSD& newvalue)
{
    LLView::sDebugRects = newvalue.asBoolean();
    return true;
}

static bool handleLogFileChanged(const LLSD& newvalue)
{
    std::string log_filename = newvalue.asString();
    LLFile::remove(log_filename);
    LLError::logToFile(log_filename);
    LL_INFOS() << "Logging switched to " << log_filename << LL_ENDL;
    return true;
}

bool handleHideGroupTitleChanged(const LLSD& newvalue)
{
    gAgent.setHideGroupTitle(newvalue);
    return true;
}

bool handleEffectColorChanged(const LLSD& newvalue)
{
    gAgent.setEffectColor(LLColor4(newvalue));
    return true;
}

bool handleHighResSnapshotChanged(const LLSD& newvalue)
{
    // High Res Snapshot active, must uncheck RenderUIInSnapshot
    if (newvalue.asBoolean())
    {
        gSavedSettings.setBOOL( "RenderUIInSnapshot", false);
    }
    return true;
}

bool handleVoiceClientPrefsChanged(const LLSD& newvalue)
{
    if (LLVoiceClient::instanceExists())
    {
        LLVoiceClient::getInstance()->updateSettings();
    }
    return true;
}

bool handleVelocityInterpolate(const LLSD& newvalue)
{
    LLMessageSystem* msg = gMessageSystem;
    if ( newvalue.asBoolean() )
    {
        msg->newMessageFast(_PREHASH_VelocityInterpolateOn);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gAgent.sendReliableMessage();
        LL_INFOS() << "Velocity Interpolation On" << LL_ENDL;
    }
    else
    {
        msg->newMessageFast(_PREHASH_VelocityInterpolateOff);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gAgent.sendReliableMessage();
        LL_INFOS() << "Velocity Interpolation Off" << LL_ENDL;
    }
    return true;
}

bool handleForceShowGrid(const LLSD& newvalue)
{
	//LLPanelLogin::updateLocationSelectorsVisibility();
	return true;
}

bool handleLoginLocationChanged()
{
    /*
     * This connects the default preference setting to the state of the login
     * panel if it is displayed; if you open the preferences panel before
     * logging in, and change the default login location there, the login
     * panel immediately changes to match your new preference.
     */
    std::string new_login_location = gSavedSettings.getString("LoginLocation");
    LL_DEBUGS("AppInit")<<new_login_location<<LL_ENDL;
    LLStartUp::setStartSLURL(LLSLURL(new_login_location));
    return true;
}

bool handleSpellCheckChanged()
{
    if (gSavedSettings.getBOOL("SpellCheck"))
    {
        std::list<std::string> dict_list;
        std::string dict_setting = gSavedSettings.getString("SpellCheckDictionary");
        boost::split(dict_list, dict_setting, boost::is_any_of(std::string(",")));
        if (!dict_list.empty())
        {
            LLSpellChecker::setUseSpellCheck(dict_list.front());
            dict_list.pop_front();
            LLSpellChecker::instance().setSecondaryDictionaries(dict_list);
            return true;
        }
    }
    LLSpellChecker::setUseSpellCheck(LLStringUtil::null);
    return true;
}

bool toggle_agent_pause(const LLSD& newvalue)
{
    if ( newvalue.asBoolean() )
    {
        send_agent_pause();
    }
    else
    {
        send_agent_resume();
    }
    return true;
}

bool toggle_show_object_render_cost(const LLSD& newvalue)
{
	LLFloaterTools::sShowObjectCost = newvalue.asBoolean();
	return true;
}

//BD
/////////////////////////////////////////////////////////////////////////////


static bool handleAvatarRotateThresholdFast(const LLSD& newvalue)
{
	gDragonLibrary.mAvatarRotateThresholdFast = (F32)newvalue.asReal();
	return true;
}

static bool handleAvatarRotateThresholdSlow(const LLSD& newvalue)
{
	gDragonLibrary.mAvatarRotateThresholdSlow = (F32)newvalue.asReal();
	return true;
}

static bool handleAvatarRotateThresholdMouselook(const LLSD& newvalue)
{
	gDragonLibrary.mAvatarRotateThresholdMouselook = (F32)newvalue.asReal();
	return true;
}

static bool handleMovementRotationSpeed(const LLSD& newvalue)
{
	gDragonLibrary.mMovementRotationSpeed = (F32)newvalue.asReal();
	return true;
}

static bool handleAllowWalkingBackwards(const LLSD& newvalue)
{
	gDragonLibrary.mAllowWalkingBackwards = newvalue.asBoolean();
	return true;
}

static bool handleHighlightChanged(const LLSD& newvalue)
{
	LLSelectMgr::sRenderHighlightType = newvalue.asInteger();
	return true;
}

static bool handleSelectionUpdateChanged(const LLSD& newvalue)
{
	LLSelectMgr::sSelectionUpdate = newvalue.asInteger();
	return true;
}

//BD - Freeze World
bool toggle_freeze_world(const LLSD& newvalue)
{
	bool val = newvalue.asBoolean();
	if (val)
	{
		// freeze all avatars
		for (auto avatarp : LLCharacter::sInstances)
		{
			mAvatarPauseHandles.push_back(avatarp->requestPause());
		}
	}
	else // turning off freeze world mode, either temporarily or not.
	{
		// thaw all avatars
		mAvatarPauseHandles.clear();
	}

	// freeze/thaw everything else
	gSavedSettings.setBOOL("FreezeTime", val);
	gDragonLibrary.mUseFreezeWorld = val;
	
	//BD
	gDragonStatus->setWorldFrozen(val);

	return true;
}

//BD - Catznip's Borderless Window Mode
static bool handleFullscreenWindow(const LLSD& newvalue)
{
	if ((gViewerWindow) && (gViewerWindow->canFullscreenWindow()))
		gViewerWindow->setFullscreenWindow(newvalue.asBoolean());
	return true;
}

//BD - Make attached lights and particles available everywhere without extra coding.
static bool handleRenderAttachedParticlesChanged(const LLSD& newvalue)
{
	LLPipeline::sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");
	return true;
}

static bool handleRenderOtherAttachedLightsChanged(const LLSD& newvalue)
{
	LLPipeline::sRenderOtherAttachedLights = gSavedSettings.getBOOL("RenderOtherAttachedLights");
	return true;
}

static bool handleRenderOwnAttachedLightsChanged(const LLSD& newvalue)
{
	LLPipeline::sRenderOwnAttachedLights = gSavedSettings.getBOOL("RenderOwnAttachedLights");
	return true;
}

static bool handleRenderDeferredLightsChanged(const LLSD& newvalue)
{
	LLPipeline::sRenderDeferredLights = gSavedSettings.getBOOL("RenderDeferredLights");
	return true;
}

//BD - Camera
static bool handleFreeDoFChanged(const LLSD& newvalue)
{
	gDragonStatus->setFreeDoF(newvalue.asBoolean());
	return true;
}

static bool handleLockDoFChanged(const LLSD& newvalue)
{
	gDragonStatus->setLockedDoF(newvalue.asBoolean());
	return true;
}

//BD - Always-on Mouse-steering.
static bool handleMouseSteeringChanged(const LLSD& newvalue)
{
	if (!gAgentCamera.isInitialized())
		return false;
	gAgentCamera.mThirdPersonSteeringMode = newvalue.asBoolean();

	//BD - Whenever steering is off and we trigger this we will
	//     show the cursor here because it would be too hacky in camera
	//     cpp itself
	if (!newvalue.asBoolean())
	{
		LLToolCamera::getInstance()->setMouseCapture(false);
		gViewerWindow->showCursor();
	}

	return true;
}

//BD - Invert mouse pitch in third person.
static bool handleInvertMouse(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mMouseInvert = newvalue.asBoolean();
	return true;
}

//BD - Follow a specified joint's movement.
static bool handleFollowJoint(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mFollowJoint = newvalue.asInteger();
	return true;
}

//BD - Camera position smoothing.
static bool handleCameraSmoothing(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mCameraPositionSmoothing = (F32)newvalue.asReal();
	return true;
}

static bool handleEyeConstrainsChanged(const LLSD& newvalue)
{
	if (!gAgentAvatarp)
		return false;
	LLEyeMotion* eye_rot_motion = (LLEyeMotion*)gAgentAvatarp->getMotionController().findMotion(ANIM_AGENT_EYE);
	if (eye_rot_motion)
		eye_rot_motion->setEyeConstrains(newvalue.asInteger());
	return true;
}

static bool handleHeadConstrainsChanged(const LLSD& newvalue)
{
	if (!gAgentAvatarp)
		return false;
	LLHeadRotMotion* head_rot_motion = (LLHeadRotMotion*)gAgentAvatarp->getMotionController().findMotion(ANIM_AGENT_HEAD_ROT);
	if (head_rot_motion)
		head_rot_motion->setHeadConstrains(newvalue.asInteger());
	return true;
}

//BD - Water Height
static bool handleWaterHeightChanged(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgent.getRegion()->setWaterHeightLocal((F32)newvalue.asReal());
	return true;
}

//BD - Impostor Toggle
static bool handleRenderImpostorsChanged(const LLSD& newvalue)
{
	gPipeline.RenderImpostors = newvalue.asBoolean();
	return true;
}

//BD - Cinematic Camera
static bool handleCinematicCamera(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mCinematicCamera = newvalue.asBoolean();
	return true;
}

//BD - Camera Rolling
static bool handleCameraMaxRoll(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mCameraMaxRoll = (F32)newvalue.asReal();
	return true;
}

static bool handleCameraMaxRollSitting(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mCameraMaxRollSitting = (F32)newvalue.asReal();
	return true;
}

static bool handleAllowCameraFlip(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mAllowCameraFlipOnSit = newvalue.asBoolean();
	return true;
}

//BD - Realistic Mouselook
static bool handleRealisticMouselook(const LLSD& newvalue)
{
	if (gAgentCamera.isInitialized())
		gAgentCamera.mRealisticMouselook = newvalue.asBoolean();
	return true;
}

//BD - Change control scheme on the fly.
bool handleKeyboardLayoutChanged(const LLSD& newvalue)
{
	LLAppViewer::loadKeyboardlayout();
	return true;
}

//BD - Give UseEnvironmentFromRegion a purpose and make it able to
//     switch between Region/Fixed Windlight from everywhere via UI.
/*static bool handleUseRegioLight(const LLSD& newvalue)
{
	LLEnvManagerNew& envmgr = LLEnvManagerNew::instance();
	gSavedSettings.setBOOL("UseEnvironmentFromRegion" , newvalue.asBoolean());
	bool fixed = gSavedSettings.getBOOL("UseEnvironmentFromRegion");
	//BD
	envmgr.setUseCustomSkySettings(false);

	if (fixed)
	{
		envmgr.setUseRegionSettings(true);
	}
	else
	{
		envmgr.setUseRegionSettings(false);
	}
	return true;
}*/

/*static bool handleWaterResolutionChanged(const LLSD& newvalue)
{
	gPipeline.handleReflectionChanges();
	return true;
}*/

static bool handleShadowMapsChanged(const LLSD& newvalue)
{
	if (STATE_STARTED == LLStartUp::getStartupState())
	{
		//BD - Force refresh the pipeline cached values, otherwise they lag behind.
		gPipeline.RenderShadowResolution = gSavedSettings.getVector4("RenderShadowResolution");
		gPipeline.RenderProjectorShadowResolution = gSavedSettings.getVector2("RenderProjectorShadowResolution");
		gPipeline.allocateShadowBuffer();
		return true;
	}
	else
		return false;
}

/*static bool handleDepthOfFieldChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	return LLViewerShaderMgr::instance()->loadShadersDOF(success);
}

static bool handleSpotlightsChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	return LLViewerShaderMgr::instance()->loadShadersSpotlights(success);
}

static bool handleSSAOChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	return LLViewerShaderMgr::instance()->loadShadersSSAO(success);
}

static bool handleBlurLightChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	success = LLViewerShaderMgr::instance()->loadShadersBlurLight(success);
	return LLViewerShaderMgr::instance()->loadShadersSSAO(success);
}

static bool handleSSRChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	return LLViewerShaderMgr::instance()->loadShadersSSR(success);
}

static bool handleGodraysChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	success = LLViewerShaderMgr::instance()->loadShadersDOF(success);
	return LLViewerShaderMgr::instance()->loadShadersGodrays(success);
}

static bool handleEnvironmentMapChanged(const LLSD& newvalue)
{
	bool success = gPipeline.sRenderDeferred;
	success = LLViewerShaderMgr::instance()->loadShadersSSR(success);
	return LLViewerShaderMgr::instance()->loadShadersMaterials(success);
}*/

static bool handleTimeFactorChanged(const LLSD& newvalue)
{
	if (!gAgentAvatarp)
		return false;
	if (gSavedSettings.getBOOL("SlowMotionAnimation"))
	{
		gAgentAvatarp->setAnimTimeFactor(gSavedSettings.getF32("SlowMotionTimeFactor"));
	}
	return true;
}

static bool handleFullbrightChanged(const LLSD& newvalue)
{
	if (!gSavedSettings.getBOOL("RenderEnableFullbright"))
	{
		gObjectList.killAllFullbrights();
	}
	return true;
}

static bool handleAlphaChanged(const LLSD& newvalue)
{
	if (!gSavedSettings.getBOOL("RenderEnableAlpha"))
	{
		gObjectList.killAllAlphas();
	}
	return true;
}

/*static bool handleCloudNoiseChanged(const LLSD& newvalue)
{
	LLDrawPoolWLSky::loadCloudNoise();
	return true;
}*/

//BD - Machinima Sidebar
static bool handleMachinimaSidebar(const LLSD& newvalue)
{
	if (gSideBar)
	{
		gSideBar->refreshGraphicControls();
		return true;
	}
	return false;
}

//BD - Rotation Speed Customisation
static bool handleAvatarPitchMultiplier(const LLSD& newvalue)
{
	gAgent.setPitchMultiplier((F32)newvalue.asReal());
	return true;
}

static bool handleAvatarYawMultiplier(const LLSD& newvalue)
{
	gAgent.setYawMultiplier((F32)newvalue.asReal());
	return true;
}

void handleTargetFPSChanged(const LLSD& newValue)
{
    const auto targetFPS = gSavedSettings.getU32("TargetFPS");

    U32 frame_rate_limit = gViewerWindow->getWindow()->getRefreshRate();
    if(LLPerfStats::tunables.vsyncEnabled && (targetFPS > frame_rate_limit))
    {
        gSavedSettings.setU32("TargetFPS", std::min(frame_rate_limit, targetFPS));
    }
    else
    {
        LLPerfStats::tunables.userTargetFPS = targetFPS;
    }
}

void handleAutoTuneLockChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getBOOL("AutoTuneLock");
    LLPerfStats::tunables.userAutoTuneLock = newval;

    gSavedSettings.setBOOL("AutoTuneFPS", newval);
}

void handleAutoTuneFPSChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getBOOL("AutoTuneFPS");
    LLPerfStats::tunables.userAutoTuneEnabled = newval;
    if(newval && LLPerfStats::renderAvatarMaxART_ns == 0) // If we've enabled autotune we override "unlimited" to max
    {
        gSavedSettings.setF32("RenderAvatarMaxART", (F32)log10(LLPerfStats::ART_UNLIMITED_NANOS-1000));//triggers callback to update static var
    }
}

void handleRenderAvatarMaxARTChanged(const LLSD& newValue)
{
    LLPerfStats::tunables.updateRenderCostLimitFromSettings();
}

void handleUserTargetDrawDistanceChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getF32("AutoTuneRenderFarClipTarget");
    LLPerfStats::tunables.userTargetDrawDistance = newval;
}

void handleUserMinDrawDistanceChanged(const LLSD &newValue)
{
    const auto newval = gSavedSettings.getF32("AutoTuneRenderFarClipMin");
    LLPerfStats::tunables.userMinDrawDistance = newval;
}

void handleUserTargetReflectionsChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getS32("UserTargetReflections");
    LLPerfStats::tunables.userTargetReflections = newval;
}

void handlePerformanceStatsEnabledChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getBOOL("PerfStatsCaptureEnabled");
    LLPerfStats::StatsRecorder::setEnabled(newval);
}
void handleUserImpostorByDistEnabledChanged(const LLSD& newValue)
{
    bool auto_tune_newval = false;
    S32 mode = gSavedSettings.getS32("RenderAvatarComplexityMode");
    if (mode != LLVOAvatar::AV_RENDER_ONLY_SHOW_FRIENDS)
    {
        auto_tune_newval = gSavedSettings.getBOOL("AutoTuneImpostorByDistEnabled");
    }
    LLPerfStats::tunables.userImpostorDistanceTuningEnabled = auto_tune_newval;
}
void handleUserImpostorDistanceChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getF32("AutoTuneImpostorFarAwayDistance");
    LLPerfStats::tunables.userImpostorDistance = newval;
}
void handleFPSTuningStrategyChanged(const LLSD& newValue)
{
    const auto newval = gSavedSettings.getU32("TuningFPSStrategy");
    LLPerfStats::tunables.userFPSTuningStrategy = newval;
}

void handleLocalTerrainChanged(const LLSD& newValue)
{
    for (U32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        const auto setting = gSavedSettings.getString(std::string("LocalTerrainAsset") + std::to_string(i + 1));
        const LLUUID materialID(setting);
        gLocalTerrainMaterials.setDetailAssetID(i, materialID);

        // *NOTE: The GLTF spec allows for different texture infos to have their texture transforms set independently, but as a simplification, this debug setting only updates all the transforms in-sync (i.e. only one texture transform per terrain material).
        LLGLTFMaterial::TextureTransform transform;
        const std::string prefix = std::string("LocalTerrainTransform") + std::to_string(i + 1);
        transform.mScale.mV[VX] = gSavedSettings.getF32(prefix + "ScaleU");
        transform.mScale.mV[VY] = gSavedSettings.getF32(prefix + "ScaleV");
        transform.mRotation = gSavedSettings.getF32(prefix + "Rotation") * DEG_TO_RAD;
        transform.mOffset.mV[VX] = gSavedSettings.getF32(prefix + "OffsetU");
        transform.mOffset.mV[VY] = gSavedSettings.getF32(prefix + "OffsetV");
        LLPointer<LLGLTFMaterial> mat_override = new LLGLTFMaterial();
        for (U32 info = 0; info < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++info)
        {
            mat_override->mTextureTransform[info] = transform;
        }
        if (*mat_override == LLGLTFMaterial::sDefault)
        {
            gLocalTerrainMaterials.setMaterialOverride(i, nullptr);
        }
        else
        {
            gLocalTerrainMaterials.setMaterialOverride(i, mat_override);
        }
        const bool paint_enabled = gSavedSettings.getBOOL("LocalTerrainPaintEnabled");
        gLocalTerrainMaterials.setPaintType(paint_enabled ? TERRAIN_PAINT_TYPE_PBR_PAINTMAP : TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE);
    }
}
////////////////////////////////////////////////////////////////////////////

LLPointer<LLControlVariable> setting_get_control(LLControlGroup& group, const std::string& setting)
{
    LLPointer<LLControlVariable> cntrl_ptr = group.getControl(setting);
    if (cntrl_ptr.isNull())
    {
        LLError::LLUserWarningMsg::showMissingFiles();
        LL_ERRS() << "Unable to set up setting listener for " << setting
            << "." << LL_ENDL;
    }
    return cntrl_ptr;
}

void setting_setup_signal_listener(LLControlGroup& group, const std::string& setting, std::function<void(const LLSD& newvalue)> callback)
{
    setting_get_control(group, setting)->getSignal()->connect([callback](LLControlVariable* control, const LLSD& new_val, const LLSD& old_val)
    {
        callback(new_val);
    });
}

void setting_setup_signal_listener(LLControlGroup& group, const std::string& setting, std::function<void()> callback)
{
    setting_get_control(group, setting)->getSignal()->connect([callback](LLControlVariable* control, const LLSD& new_val, const LLSD& old_val)
    {
        callback();
    });
}

void settings_setup_listeners()
{
	//BD - Todo
	/*gSavedSettings.getControl("FirstPersonAvatarVisible")->getSignal()->connect(boost::bind(&handleRenderAvatarMouselookChanged, _2));
	gSavedSettings.getControl("RenderFarClip")->getSignal()->connect(boost::bind(&handleRenderFarClipChanged, _2));
	gSavedSettings.getControl("RenderTerrainDetail")->getSignal()->connect(boost::bind(&handleTerrainDetailChanged, _2));
	gSavedSettings.getControl("OctreeStaticObjectSizeFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeDistanceFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeMaxNodeCapacity")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeAlphaDistanceFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeAttachmentSizeFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("RenderMaxTextureIndex")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderUseTriStrips")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderUIBuffer")->getSignal()->connect(boost::bind(&handleWindowResized, _2));
	gSavedSettings.getControl("RenderDepthOfField")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderFSAASamples")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderSpecularResX")->getSignal()->connect(boost::bind(&handleLUTBufferChanged, _2));
	gSavedSettings.getControl("RenderSpecularResY")->getSignal()->connect(boost::bind(&handleLUTBufferChanged, _2));
	gSavedSettings.getControl("RenderSpecularExponent")->getSignal()->connect(boost::bind(&handleLUTBufferChanged, _2));
	gSavedSettings.getControl("RenderAnisotropic")->getSignal()->connect(boost::bind(&handleAnisotropicChanged, _2));
	//gSavedSettings.getControl("RenderShadowResolutionScale")->getSignal()->connect(boost::bind(&handleShadowsResized, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderGlowResolutionPow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderVolumeLODFactor")->getSignal()->connect(boost::bind(&handleVolumeLODChanged, _2));
	gSavedSettings.getControl("RenderAvatarLODFactor")->getSignal()->connect(boost::bind(&handleAvatarLODChanged, _2));
	gSavedSettings.getControl("RenderAvatarPhysicsLODFactor")->getSignal()->connect(boost::bind(&handleAvatarPhysicsLODChanged, _2));
	gSavedSettings.getControl("RenderTerrainLODFactor")->getSignal()->connect(boost::bind(&handleTerrainLODChanged, _2));
	gSavedSettings.getControl("RenderTreeLODFactor")->getSignal()->connect(boost::bind(&handleTreeLODChanged, _2));
	gSavedSettings.getControl("RenderFlexTimeFactor")->getSignal()->connect(boost::bind(&handleFlexLODChanged, _2));
	gSavedSettings.getControl("RenderGamma")->getSignal()->connect(boost::bind(&handleGammaChanged, _2));
	gSavedSettings.getControl("RenderFogRatio")->getSignal()->connect(boost::bind(&handleFogRatioChanged, _2));
	gSavedSettings.getControl("RenderMaxPartCount")->getSignal()->connect(boost::bind(&handleMaxPartCountChanged, _2));
	gSavedSettings.getControl("RenderDynamicLOD")->getSignal()->connect(boost::bind(&handleRenderDynamicLODChanged, _2));
	gSavedSettings.getControl("RenderDebugTextureBind")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaNonDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderObjectBump")->getSignal()->connect(boost::bind(&handleRenderBumpChanged, _2));
	gSavedSettings.getControl("RenderMaxVBOSize")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
    gSavedSettings.getControl("RenderVSyncEnable")->getSignal()->connect(boost::bind(&handleVSyncChanged, _2));
	gSavedSettings.getControl("RenderDeferredNoise")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderDebugGL")->getSignal()->connect(boost::bind(&handleRenderDebugGLChanged, _2));
	gSavedSettings.getControl("RenderDebugPipeline")->getSignal()->connect(boost::bind(&handleRenderDebugPipelineChanged, _2));
	gSavedSettings.getControl("RenderResolutionDivisor")->getSignal()->connect(boost::bind(&handleRenderResolutionDivisorChanged, _2));
	gSavedSettings.getControl("RenderDeferred")->getSignal()->connect(boost::bind(&handleRenderDeferredChanged, _2));
	gSavedSettings.getControl("RenderShadowDetail")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderDeferredSSAO")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderPerformanceTest")->getSignal()->connect(boost::bind(&handleRenderPerfTestChanged, _2));
	gSavedSettings.getControl("TextureMemory")->getSignal()->connect(boost::bind(&handleVideoMemoryChanged, _2));
	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&handleChatFontSizeChanged, _2));
	gSavedSettings.getControl("ChatPersistTime")->getSignal()->connect(boost::bind(&handleChatPersistTimeChanged, _2));
	gSavedSettings.getControl("ConsoleMaxLines")->getSignal()->connect(boost::bind(&handleConsoleMaxLinesChanged, _2));
	gSavedSettings.getControl("UploadBakedTexOld")->getSignal()->connect(boost::bind(&handleUploadBakedTexOldChanged, _2));
	gSavedSettings.getControl("UseOcclusion")->getSignal()->connect(boost::bind(&handleUseOcclusionChanged, _2));
	gSavedSettings.getControl("AudioLevelMaster")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelSFX")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelDoppler")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelRolloff")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelUnderwaterRolloff")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("RenderVBOEnable")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderUseVAO")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderVBOMappingDisable")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderUseStreamVBO")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderPreferStreamDraw")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("WLSkyDetail")->getSignal()->connect(boost::bind(&handleWLSkyDetailChanged, _2));
	gSavedSettings.getControl("JoystickAxis0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("DebugViews")->getSignal()->connect(boost::bind(&handleDebugViewsChanged, _2));
	gSavedSettings.getControl("UserLogFile")->getSignal()->connect(boost::bind(&handleLogFileChanged, _2));
	gSavedSettings.getControl("RenderHideGroupTitle")->getSignal()->connect(boost::bind(handleHideGroupTitleChanged, _2));
	gSavedSettings.getControl("HighResSnapshot")->getSignal()->connect(boost::bind(handleHighResSnapshotChanged, _2));
	gSavedSettings.getControl("EnableVoiceChat")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PTTCurrentlyEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkButton")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkToggle")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceEarLocation")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceInputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceOutputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("AudioLevelMic")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("LipSyncEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));	
	gSavedSettings.getControl("VelocityInterpolate")->getSignal()->connect(boost::bind(&handleVelocityInterpolate, _2));
	gSavedSettings.getControl("QAMode")->getSignal()->connect(boost::bind(&show_debug_menus));
	gSavedSettings.getControl("UseDebugMenus")->getSignal()->connect(boost::bind(&show_debug_menus));
	gSavedSettings.getControl("AgentPause")->getSignal()->connect(boost::bind(&toggle_agent_pause, _2));
	gSavedSettings.getControl("ShowObjectRenderingCost")->getSignal()->connect(boost::bind(&toggle_show_object_render_cost, _2));
	gSavedSettings.getControl("ForceShowGrid")->getSignal()->connect(boost::bind(&handleForceShowGrid, _2));
	gSavedSettings.getControl("SpellCheck")->getSignal()->connect(boost::bind(&handleSpellCheckChanged));
	gSavedSettings.getControl("SpellCheckDictionary")->getSignal()->connect(boost::bind(&handleSpellCheckChanged));
	gSavedSettings.getControl("LoginLocation")->getSignal()->connect(boost::bind(&handleLoginLocationChanged));
	gSavedSettings.getControl("DebugAvatarJoints")->getCommitSignal()->connect(boost::bind(&handleDebugAvatarJointsChanged, _2));
	gSavedSettings.getControl("RenderAutoMuteByteLimit")->getSignal()->connect(boost::bind(&handleRenderAutoMuteByteLimitChanged, _2));
	gSavedPerAccountSettings.getControl("AvatarHoverOffsetZ")->getCommitSignal()->connect(boost::bind(&handleAvatarHoverOffsetChanged, _2));*/
	
	//BD - Special Debugs and handles
	//BD - Misc
	gSavedSettings.getControl("RenderHighlightType")->getSignal()->connect(boost::bind(&handleHighlightChanged, _2));
	gSavedSettings.getControl("FastSelectionUpdates")->getSignal()->connect(boost::bind(&handleSelectionUpdateChanged, _2));

	//BD - Windlight
	//gSavedSettings.getControl("UseEnvironmentFromRegion")->getSignal()->connect(boost::bind(&handleUseRegioLight, _2));
	//gSavedSettings.getControl("CloudNoiseImageName")->getSignal()->connect(boost::bind(&handleCloudNoiseChanged, _2));
	gSavedSettings.getControl("RenderWaterLightReflections")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));

	//BD - System
	gSavedSettings.getControl("SlowMotionTimeFactor")->getSignal()->connect(boost::bind(&handleTimeFactorChanged, _2));
	//gSavedSettings.getControl("SystemMemory")->getSignal()->connect(boost::bind(&handleVideoMemoryChanged, _2));
//	//BD - Catznip's Borderless Window Mode
	gSavedSettings.getControl("FullScreenWindow")->getSignal()->connect(boost::bind(&handleFullscreenWindow, _2));
	
	//BD - Machinima
	gSavedSettings.getControl("UseFreezeWorld")->getSignal()->connect(boost::bind(&toggle_freeze_world, _2));
	gSavedSettings.getControl("MachinimaSidebar")->getSignal()->connect(boost::bind(&handleMachinimaSidebar, _2));

	//BD - Rendering (Lights)
	//gSavedSettings.getControl("RenderSpotLightReflections")->getSignal()->connect(boost::bind(&handleSpotlightsChanged, _2));
	//gSavedSettings.getControl("RenderSpotLightImages")->getSignal()->connect(boost::bind(&handleSpotlightsChanged, _2));
	gSavedSettings.getControl("RenderOtherAttachedLights")->getSignal()->connect(boost::bind(&handleRenderOtherAttachedLightsChanged, _2));
	gSavedSettings.getControl("RenderOwnAttachedLights")->getSignal()->connect(boost::bind(&handleRenderOwnAttachedLightsChanged, _2));
	gSavedSettings.getControl("RenderDeferredLights")->getSignal()->connect(boost::bind(&handleRenderDeferredLightsChanged, _2));

	//BD - Rendering (Quality)
	gSavedSettings.getControl("RenderTerrainScale")->getSignal()->connect(boost::bind(&handleTerrainScaleChanged, _2));
	//gSavedSettings.getControl("RenderWaterRefResolution")->getSignal()->connect(boost::bind(&handleWaterResolutionChanged, _2));
	//gSavedSettings.getControl("RenderNormalMapScale")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderProjectorShadowResolution")->getSignal()->connect(boost::bind(&handleShadowMapsChanged, _2));
	gSavedSettings.getControl("RenderShadowResolution")->getSignal()->connect(boost::bind(&handleShadowMapsChanged, _2));
	gSavedSettings.getControl("RenderDepthOfFieldHighQuality")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	//gSavedSettings.getControl("RenderDeferredEnvironmentMap")->getSignal()->connect(boost::bind(&handleEnvironmentMapChanged, _2));
	gSavedSettings.getControl("RenderHighPrecisionNormals")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));

	//BD - Rendering (Main Toggles)
	//gSavedSettings.getControl("RenderScreenSpaceReflections")->getSignal()->connect(boost::bind(&handleSSRChanged, _2));
	//gSavedSettings.getControl("RenderSSRRoughness")->getSignal()->connect(boost::bind(&handleSSRChanged, _2));
	gSavedSettings.getControl("RenderGodrays")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	//gSavedSettings.getControl("RenderDeferredBlurLight")->getSignal()->connect(boost::bind(&handleBlurLightChanged, _2));
	gSavedSettings.getControl("RenderDeferredBlurLight")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderShadowDetail")->getSignal()->connect(boost::bind(&handleShadowMapsChanged, _2));
	//gSavedSettings.getControl("RenderDeferredSSAO")->getSignal()->connect(boost::bind(&handleSSAOChanged, _2));
	//gSavedSettings.getControl("RenderDepthOfField")->getSignal()->connect(boost::bind(&handleGodraysChanged, _2));
//	//BD - Motion Blur
	//gSavedSettings.getControl("RenderMotionBlur")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
//	//BD - Depth of Field
	gSavedSettings.getControl("RenderDepthOfFieldFront")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));

	//BD - Rendering (General)
	gSavedSettings.getControl("RenderAttachedParticles")->getSignal()->connect(boost::bind(&handleRenderAttachedParticlesChanged, _2));
	gSavedSettings.getControl("RenderEnableFullbright")->getSignal()->connect(boost::bind(&handleFullbrightChanged, _2));
	gSavedSettings.getControl("RenderEnableAlpha")->getSignal()->connect(boost::bind(&handleAlphaChanged, _2));
	gSavedSettings.getControl("RenderGodraysDirectional")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderWaterHeightFudge")->getSignal()->connect(boost::bind(&handleWaterHeightChanged, _2));
	gSavedSettings.getControl("RenderImpostors")->getSignal()->connect(boost::bind(&handleRenderImpostorsChanged, _2));

	//BD - Camera
	gSavedSettings.getControl("InvertMouseThirdPerson")->getSignal()->connect(boost::bind(&handleInvertMouse, _2));
	gSavedSettings.getControl("CameraFollowJoint")->getSignal()->connect(boost::bind(&handleFollowJoint, _2));
	gSavedSettings.getControl("CameraPositionSmoothing")->getSignal()->connect(boost::bind(&handleCameraSmoothing, _2));
	gSavedSettings.getControl("PitchFromMousePosition")->getSignal()->connect(boost::bind(&handleEyeConstrainsChanged, _2));
	gSavedSettings.getControl("YawFromMousePosition")->getSignal()->connect(boost::bind(&handleHeadConstrainsChanged, _2));
	gSavedSettings.getControl("CameraFreeDoFFocus")->getSignal()->connect(boost::bind(&handleFreeDoFChanged, _2));
	gSavedSettings.getControl("CameraDoFLocked")->getSignal()->connect(boost::bind(&handleLockDoFChanged, _2));
//	//BD - Third Person Steering Mode
	gSavedSettings.getControl("EnableThirdPersonSteering")->getSignal()->connect(boost::bind(&handleMouseSteeringChanged, _2));
//	//BD - Camera Rolling
	gSavedSettings.getControl("CameraMaxRoll")->getSignal()->connect(boost::bind(&handleCameraMaxRoll, _2));
	gSavedSettings.getControl("CameraMaxRollSitting")->getSignal()->connect(boost::bind(&handleCameraMaxRollSitting, _2));
	gSavedSettings.getControl("AllowCameraFlipOnSit")->getSignal()->connect(boost::bind(&handleAllowCameraFlip, _2));
//	//BD - Cinematic Head Tracking
	gSavedSettings.getControl("UseCinematicCamera")->getSignal()->connect(boost::bind(&handleCinematicCamera, _2));
//	//BD - Realistic Mouselook
	gSavedSettings.getControl("UseRealisticMouselook")->getSignal()->connect(boost::bind(&handleRealisticMouselook, _2));

//	//BD - Movement
	gSavedSettings.getControl("AvatarRotateThresholdFast")->getSignal()->connect(boost::bind(&handleAvatarRotateThresholdFast, _2));
	gSavedSettings.getControl("AvatarRotateThresholdSlow")->getSignal()->connect(boost::bind(&handleAvatarRotateThresholdSlow, _2));
	gSavedSettings.getControl("AvatarRotateThresholdMouselook")->getSignal()->connect(boost::bind(&handleAvatarRotateThresholdMouselook, _2));
	gSavedSettings.getControl("MovementRotationSpeed")->getSignal()->connect(boost::bind(&handleMovementRotationSpeed, _2));
	gSavedSettings.getControl("AllowWalkingBackwards")->getSignal()->connect(boost::bind(&handleAllowWalkingBackwards, _2));

	gSavedSettings.getControl("AvatarPitchMultiplier")->getSignal()->connect(boost::bind(&handleAvatarPitchMultiplier, _2));
	gSavedSettings.getControl("AvatarYawMultiplier")->getSignal()->connect(boost::bind(&handleAvatarYawMultiplier, _2));
//	//BD

    setting_setup_signal_listener(gSavedSettings, "FirstPersonAvatarVisible", handleRenderAvatarMouselookChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFarClip", handleRenderFarClipChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainScale", handleTerrainScaleChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainPBRScale", handlePBRTerrainScaleChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainPBRDetail", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainPBRPlanarSampleCount", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainPBRTriplanarBlendFactor", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "OctreeStaticObjectSizeFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeDistanceFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeMaxNodeCapacity", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeAlphaDistanceFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "OctreeAttachmentSizeFactor", handleRepartition);
    setting_setup_signal_listener(gSavedSettings, "RenderMaxTextureIndex", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderUIBuffer", handleWindowResized);
    setting_setup_signal_listener(gSavedSettings, "RenderDepthOfField", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFSAAType", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularResX", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularResY", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderSpecularExponent", handleLUTBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAnisotropic", handleAnisotropicChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderShadowResolutionScale", handleShadowsResized);
    setting_setup_signal_listener(gSavedSettings, "RenderGlow", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGlow", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGlowResolutionPow", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGlowHDR", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderEnableEmissiveBuffer", handleEnableEmissiveChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDisableVintageMode", handleDisableVintageMode);
    setting_setup_signal_listener(gSavedSettings, "RenderHDREnabled", handleEnableHDR);
    setting_setup_signal_listener(gSavedSettings, "RenderGlowNoise", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGammaFull", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVolumeLODFactor", handleVolumeLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarComplexityMode", handleUserImpostorByDistEnabledChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarLODFactor", handleAvatarLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarPhysicsLODFactor", handleAvatarPhysicsLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTerrainLODFactor", handleTerrainLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderTreeLODFactor", handleTreeLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFlexTimeFactor", handleFlexLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderGamma", handleGammaChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderFogRatio", handleFogRatioChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderMaxPartCount", handleMaxPartCountChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDynamicLOD", handleRenderDynamicLODChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderVSyncEnable", handleVSyncChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDeferredNoise", handleReleaseGLBufferChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDebugPipeline", handleRenderDebugPipelineChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderResolutionDivisor", handleRenderResolutionDivisorChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderReflectionProbeLevel", handleReflectionProbeDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderReflectionProbeDetail", handleReflectionProbeDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderReflectionsEnabled", handleReflectionProbeDetailChanged);
#if LL_DARWIN
    setting_setup_signal_listener(gSavedSettings, "RenderAppleUseMultGL", handleAppleUseMultGLChanged);
#endif
    setting_setup_signal_listener(gSavedSettings, "RenderScreenSpaceReflections", handleReflectionProbeDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderMirrors", handleReflectionProbeDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderHeroProbeResolution", handleHeroProbeResolutionChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderShadowDetail", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderDeferredSSAO", handleSetShaderChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderPerformanceTest", handleRenderPerfTestChanged);
    setting_setup_signal_listener(gSavedSettings, "ChatFontSize", handleChatFontSizeChanged);
    setting_setup_signal_listener(gSavedSettings, "ConsoleMaxLines", handleConsoleMaxLinesChanged);
    setting_setup_signal_listener(gSavedSettings, "UploadBakedTexOld", handleUploadBakedTexOldChanged);
    setting_setup_signal_listener(gSavedSettings, "UseOcclusion", handleUseOcclusionChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMaster", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelSFX", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelUI", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelAmbient", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMusic", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMedia", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelVoice", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteAudio", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteMusic", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteMedia", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteVoice", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteAmbient", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "MuteUI", handleAudioVolumeChanged);
    setting_setup_signal_listener(gSavedSettings, "WLSkyDetail", handleWLSkyDetailChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "JoystickAxis6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisScale6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "FlycamAxisDeadZone6", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "AvatarAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisScale5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone0", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone1", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone2", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone3", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone4", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "BuildAxisDeadZone5", handleJoystickChanged);
    setting_setup_signal_listener(gSavedSettings, "DebugViews", handleDebugViewsChanged);
    setting_setup_signal_listener(gSavedSettings, "UserLogFile", handleLogFileChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderHideGroupTitle", handleHideGroupTitleChanged);
    setting_setup_signal_listener(gSavedSettings, "HighResSnapshot", handleHighResSnapshotChanged);
    setting_setup_signal_listener(gSavedSettings, "EnableVoiceChat", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PTTCurrentlyEnabled", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PushToTalkButton", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "PushToTalkToggle", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceEarLocation", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceEchoCancellation", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceAutomaticGainControl", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceNoiseSuppressionLevel", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceInputAudioDevice", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VoiceOutputAudioDevice", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "AudioLevelMic", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "LipSyncEnabled", handleVoiceClientPrefsChanged);
    setting_setup_signal_listener(gSavedSettings, "VelocityInterpolate", handleVelocityInterpolate);
    setting_setup_signal_listener(gSavedSettings, "QAMode", show_debug_menus);
    setting_setup_signal_listener(gSavedSettings, "UseDebugMenus", show_debug_menus);
    setting_setup_signal_listener(gSavedSettings, "AgentPause", toggle_agent_pause);
    //setting_setup_signal_listener(gSavedSettings, "ShowNavbarNavigationPanel", toggle_show_navigation_panel);
    //setting_setup_signal_listener(gSavedSettings, "ShowMiniLocationPanel", toggle_show_mini_location_panel);
    setting_setup_signal_listener(gSavedSettings, "ShowObjectRenderingCost", toggle_show_object_render_cost);
    setting_setup_signal_listener(gSavedSettings, "ForceShowGrid", handleForceShowGrid);
    setting_setup_signal_listener(gSavedSettings, "RenderTransparentWater", handleRenderTransparentWaterChanged);
    setting_setup_signal_listener(gSavedSettings, "SpellCheck", handleSpellCheckChanged);
    setting_setup_signal_listener(gSavedSettings, "SpellCheckDictionary", handleSpellCheckChanged);
    setting_setup_signal_listener(gSavedSettings, "LoginLocation", handleLoginLocationChanged);
    setting_setup_signal_listener(gSavedSettings, "DebugAvatarJoints", handleDebugAvatarJointsChanged);

    setting_setup_signal_listener(gSavedSettings, "TargetFPS", handleTargetFPSChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneFPS", handleAutoTuneFPSChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneLock", handleAutoTuneLockChanged);
    setting_setup_signal_listener(gSavedSettings, "RenderAvatarMaxART", handleRenderAvatarMaxARTChanged);
    setting_setup_signal_listener(gSavedSettings, "PerfStatsCaptureEnabled", handlePerformanceStatsEnabledChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneRenderFarClipTarget", handleUserTargetDrawDistanceChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneRenderFarClipMin", handleUserMinDrawDistanceChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneImpostorFarAwayDistance", handleUserImpostorDistanceChanged);
    setting_setup_signal_listener(gSavedSettings, "AutoTuneImpostorByDistEnabled", handleUserImpostorByDistEnabledChanged);
    setting_setup_signal_listener(gSavedSettings, "TuningFPSStrategy", handleFPSTuningStrategyChanged);
    {
        setting_setup_signal_listener(gSavedSettings, "LocalTerrainPaintEnabled", handleLocalTerrainChanged);
        const char* transform_suffixes[] = {
            "ScaleU",
            "ScaleV",
            "Rotation",
            "OffsetU",
            "OffsetV"
        };
        for (U32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
        {
            const auto asset_setting_name = std::string("LocalTerrainAsset") + std::to_string(i + 1);
            setting_setup_signal_listener(gSavedSettings, asset_setting_name, handleLocalTerrainChanged);
            for (const char* ts : transform_suffixes)
            {
                const auto transform_setting_name = std::string("LocalTerrainTransform") + std::to_string(i + 1) + ts;
                setting_setup_signal_listener(gSavedSettings, transform_setting_name, handleLocalTerrainChanged);
            }
        }
    }
    setting_setup_signal_listener(gSavedSettings, "TerrainPaintBitDepth", handleSetShaderChanged);

    setting_setup_signal_listener(gSavedPerAccountSettings, "AvatarHoverOffsetZ", handleAvatarHoverOffsetChanged);
}

#if TEST_CACHED_CONTROL

#define DECL_LLCC(T, V) static LLCachedControl<T> mySetting_##T("TestCachedControl"#T, V)
DECL_LLCC(U32, (U32)666);
DECL_LLCC(S32, (S32)-666);
DECL_LLCC(F32, (F32)-666.666);
DECL_LLCC(bool, true);
DECL_LLCC(bool, false);
static LLCachedControl<std::string> mySetting_string("TestCachedControlstring", "Default String Value");
DECL_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
DECL_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
DECL_LLCC(LLRect, LLRect(0, 0, 100, 500));
DECL_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
DECL_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
DECL_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));

LLSD test_llsd = LLSD()["testing1"] = LLSD()["testing2"];
DECL_LLCC(LLSD, test_llsd);

void test_cached_control()
{
#define do { TEST_LLCC(T, V) if((T)mySetting_##T != V) LL_ERRS() << "Fail "#T << LL_ENDL; } while(0)
    TEST_LLCC(U32, 666);
    TEST_LLCC(S32, (S32)-666);
    TEST_LLCC(F32, (F32)-666.666);
    TEST_LLCC(bool, true);
    TEST_LLCC(bool, false);
    if((std::string)mySetting_string != "Default String Value") LL_ERRS() << "Fail string" << LL_ENDL;
    TEST_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
    TEST_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
    TEST_LLCC(LLRect, LLRect(0, 0, 100, 500));
    TEST_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
    TEST_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
    TEST_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));
//There's no LLSD comparsion for LLCC yet. TEST_LLCC(LLSD, test_llsd);
}
#endif // TEST_CACHED_CONTROL

