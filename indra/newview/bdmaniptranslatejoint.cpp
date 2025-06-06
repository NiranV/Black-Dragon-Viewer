/**
 * @file llmaniptranslate.cpp
 * @brief BDManipTranslateJoint class implementation
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

/**
 * Positioning tool
 */

#include "llviewerprecompiledheaders.h"

#include "bdmaniptranslatejoint.h"

#include "llgl.h"
#include "llrender.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llbbox.h"
#include "llbox.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llcylinder.h"
#include "lldrawable.h"
#include "llfloatertools.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llhudrender.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llrendersphere.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewerjoint.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llui.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "lltrans.h"

const S32 NUM_AXES = 3;
const S32 MOUSE_DRAG_SLOP = 2;       // pixels
const F32 SELECTED_ARROW_SCALE = 1.3f;
const F32 MANIPULATOR_HOTSPOT_START = 0.2f;
const F32 MANIPULATOR_HOTSPOT_END = 1.2f;
const F32 SNAP_GUIDE_SCREEN_SIZE = 0.7f;
const F32 MIN_PLANE_MANIP_DOT_PRODUCT = 0.25f;
const F32 PLANE_TICK_SIZE = 0.4f;
const F32 MANIPULATOR_SCALE_HALF_LIFE = 0.07f;
const F32 SNAP_ARROW_SCALE = 0.7f;

static LLPointer<LLViewerTexture> sGridTex = NULL ;

const LLManip::EManipPart MANIPULATOR_IDS[9] =
{
    LLManip::LL_X_ARROW,
    LLManip::LL_Y_ARROW,
    LLManip::LL_Z_ARROW,
    LLManip::LL_X_ARROW,
    LLManip::LL_Y_ARROW,
    LLManip::LL_Z_ARROW,
    LLManip::LL_YZ_PLANE,
    LLManip::LL_XZ_PLANE,
    LLManip::LL_XY_PLANE
};

const U32 ARROW_TO_AXIS[4] =
{
    VX,
    VX,
    VY,
    VZ
};

// Sort manipulator handles by their screen-space projection
struct ClosestToCamera
{
    bool operator()(const BDManipTranslateJoint::ManipulatorHandle& a,
                    const BDManipTranslateJoint::ManipulatorHandle& b) const
    {
        return a.mEndPosition.mV[VZ] < b.mEndPosition.mV[VZ];
    }
};

BDManipTranslateJoint::BDManipTranslateJoint( LLToolComposite* composite )
:   LLManipTranslate(composite ),
    mLastHoverMouseX(-1),
    mLastHoverMouseY(-1),
    mMouseOutsideSlop(false),
    mCopyMadeThisDrag(false),
    mMouseDownX(-1),
    mMouseDownY(-1),
    mAxisArrowLength(50),
    mConeSize(0),
    mArrowLengthMeters(0.f),
    mGridSizeMeters(1.f),
    mPlaneManipOffsetMeters(0.f),
    mUpdateTimer(),
    mSnapOffsetMeters(0.f),
    mSubdivisions(10.f),
    mInSnapRegime(false),
    mArrowScales(1.f, 1.f, 1.f),
    mPlaneScales(1.f, 1.f, 1.f),
    mPlaneManipPositions(1.f, 1.f, 1.f, 1.f)
{
    if (sGridTex.isNull())
    {
        restoreGL();
    }
}

//static
U32 BDManipTranslateJoint::getGridTexName()
{
    if(sGridTex.isNull())
    {
        restoreGL() ;
    }

    return sGridTex.isNull() ? 0 : sGridTex->getTexName() ;
}

//static
void BDManipTranslateJoint::destroyGL()
{
    if (sGridTex)
    {
        sGridTex = NULL ;
    }
}

//static
void BDManipTranslateJoint::restoreGL()
{
    //generate grid texture
    U32 rez = 512;
    U32 mip = 0;

    destroyGL() ;
    sGridTex = LLViewerTextureManager::getLocalTexture() ;
    if(!sGridTex->createGLTexture())
    {
        sGridTex = NULL ;
        return ;
    }

    GLuint* d = new GLuint[rez*rez];

    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, sGridTex->getTexName(), true);
    gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_TRILINEAR);

    while (rez >= 1)
    {
        for (U32 i = 0; i < rez*rez; i++)
        {
            d[i] = 0x00FFFFFF;
        }

        U32 subcol = 0xFFFFFFFF;
        if (rez >= 4)
        {   //large grain grid
            for (U32 i = 0; i < rez; i++)
            {
                if (rez <= 16)
                {
                    if (rez == 16)
                    {
                        subcol = 0xA0FFFFFF;
                    }
                    else if (rez == 8)
                    {
                        subcol = 0x80FFFFFF;
                    }
                    else
                    {
                        subcol = 0x40FFFFFF;
                    }
                }
                else
                {
                    subcol = 0xFFFFFFFF;
                }
                d[i         *rez+ 0      ] = subcol;
                d[0         *rez+ i      ] = subcol;
                if (rez >= 32)
                {
                    d[i         *rez+ (rez-1)] = subcol;
                    d[(rez-1)   *rez+ i      ] = subcol;
                }

                if (rez >= 64)
                {
                    subcol = 0xFFFFFFFF;

                    if (i > 0 && i < (rez-1))
                    {
                        d[i         *rez+ 1      ] = subcol;
                        d[i         *rez+ (rez-2)] = subcol;
                        d[1         *rez+ i      ] = subcol;
                        d[(rez-2)   *rez+ i      ] = subcol;
                    }
                }
            }
        }

        subcol = 0x50A0A0A0;
        if (rez >= 128)
        { //small grain grid
            for (U32 i = 8; i < rez; i+=8)
            {
                for (U32 j = 2; j < rez-2; j++)
                {
                    d[i *rez+ j] = subcol;
                    d[j *rez+ i] = subcol;
                }
            }
        }
        if (rez >= 64)
        { //medium grain grid
            if (rez == 64)
            {
                subcol = 0x50A0A0A0;
            }
            else
            {
                subcol = 0xA0D0D0D0;
            }

            for (U32 i = 32; i < rez; i+=32)
            {
                U32 pi = i-1;
                for (U32 j = 2; j < rez-2; j++)
                {
                    d[i     *rez+ j] = subcol;
                    d[j     *rez+ i] = subcol;

                    if (rez > 128)
                    {
                        d[pi    *rez+ j] = subcol;
                        d[j     *rez+ pi] = subcol;
                    }
                }
            }
        }
        LLImageGL::setManualImage(GL_TEXTURE_2D, mip, GL_RGBA, rez, rez, GL_RGBA, GL_UNSIGNED_BYTE, d);
        rez = rez >> 1;
        mip++;
    }
    delete [] d;
}


BDManipTranslateJoint::~BDManipTranslateJoint()
{
}

/**
 * @brief Computes the natural axes for the bone associated with the joint.
 *
 * This function calculates a set of orthogonal axes that represent the natural
 * orientation of the bone. It uses the joint's end point and a reference vector
 * to determine these axes, with the Z-axis along the joint/bone and the Y axis
 * perpendicular and in the plan of the world vertical, except when the joint is vertical
 * in which case the X-axis is used. You can provide a custom reference vector by setting
 * an entry in the sReferenceUpVectors map (joints like thumbs need this ideally).
 *
 * @return BoneAxes A struct containing the computed natural X, Y, and Z axes for the bone.
 */
BDManipTranslateJoint::BoneAxes BDManipTranslateJoint::computeBoneAxes() const
{
    BoneAxes axes;

    // Use 0,0,0 as local start for joint.
    LLVector3 joint_local_pos(0.f, 0.f, 0.f);

    // Transform the local endpoint (mEnd) into world space.
    LLVector3 localEnd = mJoint->getEnd();

    axes.naturalZ = localEnd - joint_local_pos;
    axes.naturalZ.normalize();

    // Choose a reference vector. We'll use world up (0,1,0) as the default,
    // but check for an override.
    LLVector3 reference(0.f, 0.f, 1.f);
    std::string jointName = mJoint->getName();
    /*auto iter = sReferenceUpVectors.find(jointName);
    if (iter != sReferenceUpVectors.end())
    {
        reference = iter->second;
    }*/

    // However, if the bone is nearly vertical relative to world up, then world up may be almost co-linear with naturalZ.
    if (std::fabs(axes.naturalZ * reference) > 0.99f)
    {
        // Use an alternate reference (+x)
        reference = LLVector3(1.f, 0.f, 0.f);
    }

    // Now, we want the naturalY to be the projection of the chosen reference onto the plane
    // between natrualZ and rference.
    //   naturalY = reference - (naturalZ dot reference)*naturalZ (I think)
    axes.naturalY = reference - (axes.naturalZ * (axes.naturalZ * reference));
    axes.naturalY.normalize();

    // Compute naturalX as the cross product of naturalY and naturalZ.
    axes.naturalX = axes.naturalY % axes.naturalZ;
    axes.naturalX.normalize();

    return axes;
}

void BDManipTranslateJoint::setJoint(LLJoint* joint)
{
    mJoint = joint;

    // Save initial rotation as baseline for delta rotation
    if (mJoint)
    {
        if (mJoint->getJointNum() >= 134)
        {
            mSavedJointPos = mJoint->getPosition();
        }
        else
        {
            mSavedJointPos = mJoint->getTargetPosition();
        }
        mBoneAxes = computeBoneAxes();
    }
}

void BDManipTranslateJoint::setAvatar(LLVOAvatar* avatar)
{
    mAvatar = avatar;
}

void BDManipTranslateJoint::handleSelect()
{
    if (mJoint)
    {
        if (mJoint->getJointNum() >= 134)
        {
            mSavedJointPos = mJoint->getPosition();
        }
        else
        {
            mSavedJointPos = mJoint->getTargetPosition();
        }
    }
}

bool BDManipTranslateJoint::handleMouseDown(S32 x, S32 y, MASK mask)
{
    bool    handled = false;

    // didn't click in any UI object, so must have clicked in the world
    if( (mHighlightedPart == LL_X_ARROW ||
         mHighlightedPart == LL_Y_ARROW ||
         mHighlightedPart == LL_Z_ARROW ||
         mHighlightedPart == LL_YZ_PLANE ||
         mHighlightedPart == LL_XZ_PLANE ||
         mHighlightedPart == LL_XY_PLANE ) )
    {
        handled = handleMouseDownOnPart( x, y, mask );
    }

    return handled;
}

// Assumes that one of the arrows on an object was hit.
bool BDManipTranslateJoint::handleMouseDownOnPart( S32 x, S32 y, MASK mask )
{
    highlightManipulators(x, y);
    S32 hit_part = mHighlightedPart;

    if( (hit_part != LL_X_ARROW) &&
        (hit_part != LL_Y_ARROW) &&
        (hit_part != LL_Z_ARROW) &&
        (hit_part != LL_YZ_PLANE) &&
        (hit_part != LL_XZ_PLANE) &&
        (hit_part != LL_XY_PLANE) )
    {
        return true;
    }

    mHelpTextTimer.reset();
    sNumTimesHelpTextShown++;

    LLSelectMgr::getInstance()->getGrid(mGridOrigin, mGridRotation, mGridScale);

    LLSelectMgr::getInstance()->enableSilhouette(false);

    // we just started a drag, so save initial object positions
    if (mJoint)
    {
        if (mJoint->getJointNum() >= 134)
        {
            mSavedJointPos = mJoint->getPosition();
        }
        else
        {
            mSavedJointPos = mJoint->getTargetPosition();
        }
    }
    else
    {
        //BD - Didn't find the joint...oh well
        LL_WARNS() << "Trying to translate an unselected joint" << LL_ENDL;
        return false;
    }

    mManipPart = (EManipPart)hit_part;
    mMouseDownX = x;
    mMouseDownY = y;
    mMouseOutsideSlop = false;

    LLVector3       axis;

    // Compute unit vectors for arrow hit and a plane through that vector
    bool axis_exists = getManipAxis(nullptr, mManipPart, axis);
    getManipNormal(nullptr, mManipPart, mManipNormal);

    //LLVector3 select_center_agent = gAgent.getPosAgentFromGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal());
    // TomY: The above should (?) be identical to the below
    LLVector3 select_center_agent = getPivotPoint();
    mSubdivisions = getSubdivisionLevel(select_center_agent, axis_exists ? axis : LLVector3::z_axis, getMinGridScale());

    // if we clicked on a planar manipulator, recenter mouse cursor
    if (mManipPart >= LL_YZ_PLANE && mManipPart <= LL_XY_PLANE)
    {
        LLCoordGL mouse_pos;
        if (!LLViewerCamera::getInstance()->projectPosAgentToScreen(select_center_agent, mouse_pos))
        {
            // mouse_pos may be nonsense
            LL_WARNS() << "Failed to project object center to screen" << LL_ENDL;
        }
        else if (gSavedSettings.getBOOL("SnapToMouseCursor"))
        {
            LLUI::getInstance()->setMousePositionScreen(mouse_pos.mX, mouse_pos.mY);
            x = mouse_pos.mX;
            y = mouse_pos.mY;
        }
    }

    LLVector3d object_start_global = gAgent.getPosGlobalFromAgent(getPivotPoint());
    getMousePointOnPlaneGlobal(mDragCursorStartGlobal, x, y, object_start_global, mManipNormal);
    mDragSelectionStartGlobal = object_start_global;
    mCopyMadeThisDrag = false;

    // Route future Mouse messages here preemptively.  (Release on mouse up.)
    setMouseCapture( true );

    return true;
}

bool BDManipTranslateJoint::handleHover(S32 x, S32 y, MASK mask)
{
    // Translation tool only works if mouse button is down.
    // Bail out if mouse not down.
    if( !hasMouseCapture() )
    {
        LL_DEBUGS("UserInput") << "hover handled by BDManipTranslateJoint (inactive)" << LL_ENDL;
        // Always show cursor
        // gViewerWindow->setCursor(UI_CURSOR_ARROW);
        gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);

        highlightManipulators(x, y);
        return true;
    }

    // Handle auto-rotation if necessary.
    LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
    const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
    const S32 ROTATE_H_MARGIN = world_rect.getWidth() / 20;
    const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
    bool rotated = false;

    // ...build mode moves camera about focus point
    if (x < ROTATE_H_MARGIN)
    {
        gAgentCamera.cameraOrbitAround(rotate_angle);
        rotated = true;
    }
    else if (x > world_rect.getWidth() - ROTATE_H_MARGIN)
    {
        gAgentCamera.cameraOrbitAround(-rotate_angle);
        rotated = true;
    }

    // Suppress processing if mouse hasn't actually moved.
    // This may cause problems if the camera moves outside of the
    // rotation above.
    if( x == mLastHoverMouseX && y == mLastHoverMouseY && !rotated)
    {
        LL_DEBUGS("UserInput") << "hover handled by BDManipTranslateJoint (mouse unmoved)" << LL_ENDL;
        gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
        return true;
    }
    mLastHoverMouseX = x;
    mLastHoverMouseY = y;

    // Suppress if mouse hasn't moved past the initial slop region
    // Reset once we start moving
    if( !mMouseOutsideSlop )
    {
        if (abs(mMouseDownX - x) < MOUSE_DRAG_SLOP && abs(mMouseDownY - y) < MOUSE_DRAG_SLOP )
        {
            LL_DEBUGS("UserInput") << "hover handled by BDManipTranslateJoint (mouse inside slop)" << LL_ENDL;
            gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
            return true;
        }
        else
        {
            // ...just went outside the slop region
            mMouseOutsideSlop = true;
        }
    }

    // Throttle updates to 10 per second.

    LLVector3       axis_f;
    LLVector3      axis_d;

    // pick the first object to constrain to grid w/ common origin
    // this is so we don't screw up groups
    if(!mJoint)
    {
        LL_WARNS() << "No morejoint" << LL_ENDL;
        gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
        return true;
    }

    // Compute unit vectors for arrow hit and a plane through that vector
    bool axis_exists = getManipAxis(nullptr, mManipPart, axis_f);        // TODO: move this

    axis_d.setVec(axis_f);

    LLVector3 current_pos_local = getPivotPoint();
    LLVector3d current_pos_global = gAgent.getPosGlobalFromAgent(getPivotPoint());

    mSubdivisions = getSubdivisionLevel(getPivotPoint(), axis_f, getMinGridScale());

    // Project the cursor onto that plane
    LLVector3 relative_move;
    getMousePointOnPlaneAgent(relative_move, x, y, current_pos_local, mManipNormal);
    relative_move -= current_pos_local;

    // You can't move more than some distance from your original mousedown point.
    if (gSavedSettings.getBOOL("LimitDragDistance"))
    {
        F32 max_drag_distance = gSavedSettings.getF32("MaxDragDistance");

        if (relative_move.magVecSquared() > max_drag_distance * max_drag_distance)
        {
            LL_DEBUGS("UserInput") << "hover handled by BDManipTranslateJoint (too far)" << LL_ENDL;
            gViewerWindow->setCursor(UI_CURSOR_NOLOCKED);
            return true;
        }
    }

    F64 axis_magnitude = relative_move * axis_d;                    // dot product
    LLVector3 cursor_point_snap_line;

    F64 off_axis_magnitude;

    getMousePointOnPlaneAgent(cursor_point_snap_line, x, y, current_pos_local, mSnapOffsetAxis % axis_f);
    off_axis_magnitude = axis_exists ? llabs((cursor_point_snap_line - current_pos_local) * mSnapOffsetAxis) : 0.f;

    // Clamp to arrow direction
    // *FIX: does this apply anymore?
    if (!axis_exists)
    {
        axis_magnitude = relative_move.normVec();
        axis_d.setVec(relative_move);
        axis_d.normVec();
        axis_f.setVec(axis_d);
    }

    LLVector3 clamped_relative_move = (F32)axis_magnitude * axis_d; // scalar multiply
    LLVector3 clamped_relative_move_f = (F32)axis_magnitude * axis_f; // scalar multiply

    // calculate local version of relative move
    LLQuaternion objWorldRotation = mJoint->getWorldRotation();
    objWorldRotation.transQuat();

    LLVector3 old_position_local = mJoint->getTargetPosition();
    LLVector3 new_position_local = mSavedJointPos + (clamped_relative_move_f * objWorldRotation);

    //RN: I forget, but we need to do this because of snapping which doesn't often result
    // in position changes even when the mouse moves
    mJoint->setTargetPosition(new_position_local);
    if (mJoint->getJointNum() >= 134)
    {
        mSavedJointPos = mJoint->getPosition();
    }
    else
    {
        mSavedJointPos = mJoint->getTargetPosition();
    }

    gAgentCamera.clearFocusObject();

    LL_DEBUGS("UserInput") << "hover handled by BDManipTranslateJoint (active)" << LL_ENDL;
    gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
    return true;
}

void BDManipTranslateJoint::highlightManipulators(S32 x, S32 y)
{
    mHighlightedPart = LL_NO_PART;

    //LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
    LLMatrix4 projMatrix = LLViewerCamera::getInstance()->getProjection();
    LLMatrix4 modelView = LLViewerCamera::getInstance()->getModelview();

    LLVector3 object_position = getPivotPoint();

    LLVector3 grid_origin;
    LLVector3 grid_scale;
    LLQuaternion grid_rotation;

    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

    LLVector3 relative_camera_dir;

    LLMatrix4 transform;

    relative_camera_dir = (object_position - LLViewerCamera::getInstance()->getOrigin()) * ~grid_rotation;
    relative_camera_dir.normVec();

    transform.initRotTrans(grid_rotation, LLVector4(object_position));
    transform *= modelView;
    transform *= projMatrix;

    S32 numManips = 0;

    // edges
    mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 0.f, 0.f, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 0.f, 0.f, 1.f);

    mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 0.f, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 0.f, 1.f);

    mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 1.f);

    mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 0.f, 0.f, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 0.f, 0.f, 1.f);

    mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 0.f, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 0.f, 1.f);

    mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 1.f);
    mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 1.f);

    S32 num_arrow_manips = numManips;

    // planar manipulators
    bool planar_manip_yz_visible = false;
    bool planar_manip_xz_visible = false;
    bool planar_manip_xy_visible = false;

    mManipulatorVertices[numManips] = LLVector4(0.f, mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    mManipulatorVertices[numManips] = LLVector4(0.f, mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    if (llabs(relative_camera_dir.mV[VX]) > MIN_PLANE_MANIP_DOT_PRODUCT)
    {
        planar_manip_yz_visible = true;
    }

    mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 0.f, mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 0.f, mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    if (llabs(relative_camera_dir.mV[VY]) > MIN_PLANE_MANIP_DOT_PRODUCT)
    {
        planar_manip_xz_visible = true;
    }

    mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 0.f, 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 0.f, 1.f);
    mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
    if (llabs(relative_camera_dir.mV[VZ]) > MIN_PLANE_MANIP_DOT_PRODUCT)
    {
        planar_manip_xy_visible = true;
    }

    // Project up to 9 manipulators to screen space 2*X, 2*Y, 2*Z, 3*planes
    std::vector<ManipulatorHandle> projected_manipulators;
    projected_manipulators.reserve(9);

    for (S32 i = 0; i < num_arrow_manips; i+= 2)
    {
        LLVector4 projected_start = mManipulatorVertices[i] * transform;
        projected_start = projected_start * (1.f / projected_start.mV[VW]);

        LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
        projected_end = projected_end * (1.f / projected_end.mV[VW]);

        ManipulatorHandle projected_manip(
                LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]),
                LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]),
                MANIPULATOR_IDS[i / 2],
                10.f); // 10 pixel hotspot for arrows
        projected_manipulators.push_back(projected_manip);
    }

    if (planar_manip_yz_visible)
    {
        S32 i = num_arrow_manips;
        LLVector4 projected_start = mManipulatorVertices[i] * transform;
        projected_start = projected_start * (1.f / projected_start.mV[VW]);

        LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
        projected_end = projected_end * (1.f / projected_end.mV[VW]);

        ManipulatorHandle projected_manip(
                LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]),
                LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]),
                MANIPULATOR_IDS[i / 2],
                20.f); // 20 pixels for planar manipulators
        projected_manipulators.push_back(projected_manip);
    }

    if (planar_manip_xz_visible)
    {
        S32 i = num_arrow_manips + 2;
        LLVector4 projected_start = mManipulatorVertices[i] * transform;
        projected_start = projected_start * (1.f / projected_start.mV[VW]);

        LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
        projected_end = projected_end * (1.f / projected_end.mV[VW]);

        ManipulatorHandle projected_manip(
                LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]),
                LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]),
                MANIPULATOR_IDS[i / 2],
                20.f); // 20 pixels for planar manipulators
        projected_manipulators.push_back(projected_manip);
    }

    if (planar_manip_xy_visible)
    {
        S32 i = num_arrow_manips + 4;
        LLVector4 projected_start = mManipulatorVertices[i] * transform;
        projected_start = projected_start * (1.f / projected_start.mV[VW]);

        LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
        projected_end = projected_end * (1.f / projected_end.mV[VW]);

        ManipulatorHandle projected_manip(
                LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]),
                LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]),
                MANIPULATOR_IDS[i / 2],
                20.f); // 20 pixels for planar manipulators
        projected_manipulators.push_back(projected_manip);
    }

    LLVector2 manip_start_2d;
    LLVector2 manip_end_2d;
    LLVector2 manip_dir;
    LLRect world_view_rect = gViewerWindow->getWorldViewRectScaled();
    F32 half_width = (F32)world_view_rect.getWidth() / 2.f;
    F32 half_height = (F32)world_view_rect.getHeight() / 2.f;
    LLVector2 mousePos((F32)x - half_width, (F32)y - half_height);
    LLVector2 mouse_delta;

    // Keep order consistent with insertion via stable_sort
    std::stable_sort( projected_manipulators.begin(),
        projected_manipulators.end(),
        ClosestToCamera() );

    std::vector<ManipulatorHandle>::iterator it = projected_manipulators.begin();
    for ( ; it != projected_manipulators.end(); ++it)
    {
        ManipulatorHandle& manipulator = *it;
        {
            manip_start_2d.setVec(manipulator.mStartPosition.mV[VX] * half_width, manipulator.mStartPosition.mV[VY] * half_height);
            manip_end_2d.setVec(manipulator.mEndPosition.mV[VX] * half_width, manipulator.mEndPosition.mV[VY] * half_height);
            manip_dir = manip_end_2d - manip_start_2d;

            mouse_delta = mousePos - manip_start_2d;

            F32 manip_length = manip_dir.normVec();

            F32 mouse_pos_manip = mouse_delta * manip_dir;
            F32 mouse_dist_manip_squared = mouse_delta.magVecSquared() - (mouse_pos_manip * mouse_pos_manip);

            if (mouse_pos_manip > 0.f &&
                mouse_pos_manip < manip_length &&
                mouse_dist_manip_squared < manipulator.mHotSpotRadius * manipulator.mHotSpotRadius)
            {
                mHighlightedPart = manipulator.mManipID;
                break;
            }
        }
    }
}

F32 BDManipTranslateJoint::getMinGridScale()
{
    F32 scale;
    switch (mManipPart)
    {
    case LL_NO_PART:
    default:
        scale = 1.f;
        break;
    case LL_X_ARROW:
        scale = mGridScale.mV[VX];
        break;
    case LL_Y_ARROW:
        scale = mGridScale.mV[VY];
        break;
    case LL_Z_ARROW:
        scale = mGridScale.mV[VZ];
        break;
    case LL_YZ_PLANE:
        scale = llmin(mGridScale.mV[VY], mGridScale.mV[VZ]);
        break;
    case LL_XZ_PLANE:
        scale = llmin(mGridScale.mV[VX], mGridScale.mV[VZ]);
        break;
    case LL_XY_PLANE:
        scale = llmin(mGridScale.mV[VX], mGridScale.mV[VY]);
        break;
    }

    return scale;
}


bool BDManipTranslateJoint::handleMouseUp(S32 x, S32 y, MASK mask)
{
    // first, perform normal processing in case this was a quick-click
    handleHover(x, y, mask);

    if(hasMouseCapture())
    {
        // make sure arrow colors go back to normal
        mManipPart = LL_NO_PART;

        mInSnapRegime = false;
    }

    return LLManip::handleMouseUp(x, y, mask);
}


void BDManipTranslateJoint::render()
{
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        renderGuidelines();
    }
    {
        //LLGLDisable gls_stencil(GL_STENCIL_TEST);
        renderTranslationHandles();
        renderSnapGuides();
    }
    gGL.popMatrix();

    renderText();
}

void BDManipTranslateJoint::renderSnapGuides()
{
    if (!gSavedSettings.getBOOL("SnapEnabled"))
    {
        return;
    }

    F32 max_subdivisions = sGridMaxSubdivisionLevel;//(F32)gSavedSettings.getS32("GridSubdivision");
    F32 line_alpha = gSavedSettings.getF32("GridOpacity");

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    LLGLDepthTest gls_depth(GL_TRUE);
    LLGLDisable gls_cull(GL_CULL_FACE);
    LLVector3 translate_axis;

    if (mManipPart == LL_NO_PART)
    {
        return;
    }

    if(!mJoint)
    {
        return;
    }

    updateGridSettings();

    F32 smallest_grid_unit_scale = getMinGridScale() / max_subdivisions;
    LLVector3 grid_origin;
    LLVector3 grid_scale;
    LLQuaternion grid_rotation;

    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);
    LLVector3 saved_selection_center = getSavedPivotPoint(); //LLSelectMgr::getInstance()->getSavedBBoxOfSelection().getCenterAgent();
    LLVector3 selection_center = getPivotPoint();

    //pick appropriate projection plane for snap rulers according to relative camera position
    if (mManipPart >= LL_X_ARROW && mManipPart <= LL_Z_ARROW)
    {
        LLVector3 normal;
        LLColor4 inner_color;
        LLManip::EManipPart temp_manip = mManipPart;
        switch (mManipPart)
        {
        case LL_X_ARROW:
            normal.setVec(1,0,0);
            inner_color.setVec(0,1,1,line_alpha);
            mManipPart = LL_YZ_PLANE;
            break;
        case LL_Y_ARROW:
            normal.setVec(0,1,0);
            inner_color.setVec(1,0,1,line_alpha);
            mManipPart = LL_XZ_PLANE;
            break;
        case LL_Z_ARROW:
            normal.setVec(0,0,1);
            inner_color.setVec(1,1,0,line_alpha);
            mManipPart = LL_XY_PLANE;
            break;
        default:
            break;
        }

        highlightIntersection(normal, selection_center, grid_rotation, inner_color);
        mManipPart = temp_manip;
        getManipAxis(nullptr, mManipPart, translate_axis);

        LLVector3 at_axis_abs;
        {
            at_axis_abs = saved_selection_center - LLViewerCamera::getInstance()->getOrigin();
            at_axis_abs.normVec();

            at_axis_abs = at_axis_abs * ~grid_rotation;
        }
        at_axis_abs.abs();

        if (at_axis_abs.mV[VX] > at_axis_abs.mV[VY] && at_axis_abs.mV[VX] > at_axis_abs.mV[VZ])
        {
            if (mManipPart == LL_Y_ARROW)
            {
                mSnapOffsetAxis = LLVector3::z_axis;
            }
            else if (mManipPart == LL_Z_ARROW)
            {
                mSnapOffsetAxis = LLVector3::y_axis;
            }
            else if (at_axis_abs.mV[VY] > at_axis_abs.mV[VZ])
            {
                mSnapOffsetAxis = LLVector3::z_axis;
            }
            else
            {
                mSnapOffsetAxis = LLVector3::y_axis;
            }
        }
        else if (at_axis_abs.mV[VY] > at_axis_abs.mV[VZ])
        {
            if (mManipPart == LL_X_ARROW)
            {
                mSnapOffsetAxis = LLVector3::z_axis;
            }
            else if (mManipPart == LL_Z_ARROW)
            {
                mSnapOffsetAxis = LLVector3::x_axis;
            }
            else if (at_axis_abs.mV[VX] > at_axis_abs.mV[VZ])
            {
                mSnapOffsetAxis = LLVector3::z_axis;
            }
            else
            {
                mSnapOffsetAxis = LLVector3::x_axis;
            }
        }
        else
        {
            if (mManipPart == LL_X_ARROW)
            {
                mSnapOffsetAxis = LLVector3::y_axis;
            }
            else if (mManipPart == LL_Y_ARROW)
            {
                mSnapOffsetAxis = LLVector3::x_axis;
            }
            else if (at_axis_abs.mV[VX] > at_axis_abs.mV[VY])
            {
                mSnapOffsetAxis = LLVector3::y_axis;
            }
            else
            {
                mSnapOffsetAxis = LLVector3::x_axis;
            }
        }

        mSnapOffsetAxis = mSnapOffsetAxis * grid_rotation;

        F32 guide_size_meters;

        {
            LLVector3 cam_to_selection = getPivotPoint() - LLViewerCamera::getInstance()->getOrigin();
            F32 current_range = cam_to_selection.normVec();
            guide_size_meters = SNAP_GUIDE_SCREEN_SIZE * gViewerWindow->getWorldViewHeightRaw() * current_range / LLViewerCamera::getInstance()->getPixelMeterRatio();

            F32 fraction_of_fov = mAxisArrowLength / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
            F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
            F32 offset_at_camera = tan(apparent_angle) * 1.5f;
            F32 range = dist_vec(gAgent.getPosAgentFromGlobal(mSavedJointPosGlobal), LLViewerCamera::getInstance()->getOrigin());
            mSnapOffsetMeters = range * offset_at_camera;
        }

        LLVector3 tick_start;
        LLVector3 tick_end;

        // how far away from grid origin is the selection along the axis of translation?
        F32 dist_grid_axis = (selection_center - mGridOrigin) * translate_axis;
        // find distance to nearest smallest grid unit
        F32 offset_nearest_grid_unit = fmodf(dist_grid_axis, smallest_grid_unit_scale);
        // how many smallest grid units are we away from largest grid scale?
        S32 sub_div_offset = ll_round(fmodf(dist_grid_axis - offset_nearest_grid_unit, getMinGridScale() / sGridMinSubdivisionLevel) / smallest_grid_unit_scale);
        S32 num_ticks_per_side = llmax(1, llfloor(0.5f * guide_size_meters / smallest_grid_unit_scale));

        LLGLDepthTest gls_depth(GL_FALSE);

        for (S32 pass = 0; pass < 3; pass++)
        {
            LLColor4 line_color = setupSnapGuideRenderPass(pass);
            LLGLDepthTest gls_depth(pass != 1);

            gGL.begin(LLRender::LINES);
            {
                LLVector3 line_start = selection_center + (mSnapOffsetMeters * mSnapOffsetAxis) + (translate_axis * (guide_size_meters * 0.5f + offset_nearest_grid_unit));
                LLVector3 line_end = selection_center + (mSnapOffsetMeters * mSnapOffsetAxis) - (translate_axis * (guide_size_meters * 0.5f + offset_nearest_grid_unit));
                LLVector3 line_mid = (line_start + line_end) * 0.5f;

                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA] * 0.2f);
                gGL.vertex3fv(line_start.mV);
                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA]);
                gGL.vertex3fv(line_mid.mV);
                gGL.vertex3fv(line_mid.mV);
                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA] * 0.2f);
                gGL.vertex3fv(line_end.mV);

                line_start.setVec(selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) + (translate_axis * guide_size_meters * 0.5f));
                line_end.setVec(selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) - (translate_axis * guide_size_meters * 0.5f));
                line_mid = (line_start + line_end) * 0.5f;

                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA] * 0.2f);
                gGL.vertex3fv(line_start.mV);
                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA]);
                gGL.vertex3fv(line_mid.mV);
                gGL.vertex3fv(line_mid.mV);
                gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA] * 0.2f);
                gGL.vertex3fv(line_end.mV);

                for (S32 i = -num_ticks_per_side; i <= num_ticks_per_side; i++)
                {
                    tick_start = selection_center + (translate_axis * (smallest_grid_unit_scale * (F32)i - offset_nearest_grid_unit));

                    //No need check this condition to prevent tick position scaling (FIX MAINT-5207/5208)
                    //F32 cur_subdivisions = getSubdivisionLevel(tick_start, translate_axis, getMinGridScale());
                    /*if (fmodf((F32)(i + sub_div_offset), (max_subdivisions / cur_subdivisions)) != 0.f)
                    {
                        continue;
                    }*/

                    // add in off-axis offset
                    tick_start += (mSnapOffsetAxis * mSnapOffsetMeters);

                    F32 tick_scale = 1.f;
                    for (F32 division_level = max_subdivisions; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
                    {
                        if (fmodf((F32)(i + sub_div_offset), division_level) == 0.f)
                        {
                            break;
                        }
                        tick_scale *= 0.7f;
                    }

//                  S32 num_ticks_to_fade = is_sub_tick ? num_ticks_per_side / 2 : num_ticks_per_side;
//                  F32 alpha = line_alpha * (1.f - (0.8f *  ((F32)llabs(i) / (F32)num_ticks_to_fade)));

                    tick_end = tick_start + (mSnapOffsetAxis * mSnapOffsetMeters * tick_scale);

                    gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA]);
                    gGL.vertex3fv(tick_start.mV);
                    gGL.vertex3fv(tick_end.mV);

                    tick_start = selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) +
                        (translate_axis * (getMinGridScale() / (F32)(max_subdivisions) * (F32)i - offset_nearest_grid_unit));
                    tick_end = tick_start - (mSnapOffsetAxis * mSnapOffsetMeters * tick_scale);

                    gGL.vertex3fv(tick_start.mV);
                    gGL.vertex3fv(tick_end.mV);
                }
            }
            gGL.end();

            if (mInSnapRegime)
            {
                LLVector3 line_start = selection_center - mSnapOffsetAxis * mSnapOffsetMeters;
                LLVector3 line_end = selection_center + mSnapOffsetAxis * mSnapOffsetMeters;

                gGL.begin(LLRender::LINES);
                {
                    gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA]);

                    gGL.vertex3fv(line_start.mV);
                    gGL.vertex3fv(line_end.mV);
                }
                gGL.end();

                // draw snap guide arrow
                gGL.begin(LLRender::TRIANGLES);
                {
                    gGL.color4f(line_color.mV[VRED], line_color.mV[VGREEN], line_color.mV[VBLUE], line_color.mV[VALPHA]);

                    LLVector3 arrow_dir;
                    LLVector3 arrow_span = translate_axis;

                    arrow_dir = -mSnapOffsetAxis;
                    gGL.vertex3fv((line_start + arrow_dir * mConeSize * SNAP_ARROW_SCALE).mV);
                    gGL.vertex3fv((line_start + arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
                    gGL.vertex3fv((line_start - arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);

                    arrow_dir = mSnapOffsetAxis;
                    gGL.vertex3fv((line_end + arrow_dir * mConeSize * SNAP_ARROW_SCALE).mV);
                    gGL.vertex3fv((line_end + arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
                    gGL.vertex3fv((line_end - arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
                }
                gGL.end();
            }
        }

        sub_div_offset = ll_round(fmod(dist_grid_axis - offset_nearest_grid_unit, getMinGridScale() * 32.f) / smallest_grid_unit_scale);

        LLVector2 screen_translate_axis(llabs(translate_axis * LLViewerCamera::getInstance()->getLeftAxis()), llabs(translate_axis * LLViewerCamera::getInstance()->getUpAxis()));
        screen_translate_axis.normVec();

        S32 tick_label_spacing = ll_round(screen_translate_axis * sTickLabelSpacing);

        // render tickmark values
        for (S32 i = -num_ticks_per_side; i <= num_ticks_per_side; i++)
        {
            LLVector3 tick_pos = selection_center + (translate_axis * ((smallest_grid_unit_scale * (F32)i) - offset_nearest_grid_unit));
            F32 alpha = line_alpha * (1.f - (0.5f *  ((F32)llabs(i) / (F32)num_ticks_per_side)));

            F32 tick_scale = 1.f;
            for (F32 division_level = max_subdivisions; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
            {
                if (fmodf((F32)(i + sub_div_offset), division_level) == 0.f)
                {
                    break;
                }
                tick_scale *= 0.7f;
            }

            if (fmodf((F32)(i + sub_div_offset), (max_subdivisions / getSubdivisionLevel(tick_pos, translate_axis, getMinGridScale(), tick_label_spacing))) == 0.f)
            {
                F32 snap_offset_meters;

                if (mSnapOffsetAxis * LLViewerCamera::getInstance()->getUpAxis() > 0.f)
                {
                    snap_offset_meters = mSnapOffsetMeters;
                }
                else
                {
                    snap_offset_meters = -mSnapOffsetMeters;
                }
                LLVector3 text_origin = selection_center +
                        (translate_axis * ((smallest_grid_unit_scale * (F32)i) - offset_nearest_grid_unit)) +
                            (mSnapOffsetAxis * snap_offset_meters * (1.f + tick_scale));

                LLVector3 tick_offset = (tick_pos - mGridOrigin) * ~mGridRotation;
                F32 offset_val = 0.5f * tick_offset.mV[ARROW_TO_AXIS[mManipPart]] / getMinGridScale();
                EGridMode grid_mode = LLSelectMgr::getInstance()->getGridMode();
                F32 text_highlight = 0.8f;
                if(i - ll_round(offset_nearest_grid_unit / smallest_grid_unit_scale) == 0 && mInSnapRegime)
                {
                    text_highlight = 1.f;
                }

                if (grid_mode == GRID_MODE_WORLD)
                {
                    // rescale units to meters from multiple of grid scale
                    offset_val *= 2.f * grid_scale[ARROW_TO_AXIS[mManipPart]];
                    renderTickValue(text_origin, offset_val, std::string("m"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
                }
                else
                {
                    renderTickValue(text_origin, offset_val, std::string("x"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
                }
            }
        }
        
    }
    else
    {
        // render gridlines for planar snapping

        F32 u = 0, v = 0;
        LLColor4 inner_color;
        LLVector3 normal;
        LLVector3 grid_center = selection_center - grid_origin;
        F32 usc = 1;
        F32 vsc = 1;

        grid_center *= ~grid_rotation;

        switch (mManipPart)
        {
        case LL_YZ_PLANE:
            u = grid_center.mV[VY];
            v = grid_center.mV[VZ];
            usc = grid_scale.mV[VY];
            vsc = grid_scale.mV[VZ];
            inner_color.setVec(0,1,1,line_alpha);
            normal.setVec(1,0,0);
            break;
        case LL_XZ_PLANE:
            u = grid_center.mV[VX];
            v = grid_center.mV[VZ];
            usc = grid_scale.mV[VX];
            vsc = grid_scale.mV[VZ];
            inner_color.setVec(1,0,1,line_alpha);
            normal.setVec(0,1,0);
            break;
        case LL_XY_PLANE:
            u = grid_center.mV[VX];
            v = grid_center.mV[VY];
            usc = grid_scale.mV[VX];
            vsc = grid_scale.mV[VY];
            inner_color.setVec(1,1,0,line_alpha);
            normal.setVec(0,0,1);
            break;
        default:
            break;
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        highlightIntersection(normal, selection_center, grid_rotation, inner_color);

        gGL.pushMatrix();

        F32 x,y,z,angle_radians;
        grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);
        gGL.translatef(selection_center.mV[VX], selection_center.mV[VY], selection_center.mV[VZ]);
        gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);

        F32 sz = mGridSizeMeters;
        F32 tiles = sz;

        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.pushMatrix();
        usc = 1.0f/usc;
        vsc = 1.0f/vsc;

        while (usc > vsc*4.0f)
        {
            usc *= 0.5f;
        }
        while (vsc > usc * 4.0f)
        {
            vsc *= 0.5f;
        }

        gGL.scalef(usc, vsc, 1.0f);
        gGL.translatef(u, v, 0);

        float a = line_alpha;

        {
            //draw grid behind objects
            LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

            {
                //LLGLDisable stencil(GL_STENCIL_TEST);
                {
                    LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GREATER);
                    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, getGridTexName());
                    gGL.flush();
                    gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
                    renderGrid(u,v,tiles,0.9f, 0.9f, 0.9f,a*0.15f);
                    gGL.flush();
                    gGL.setSceneBlendType(LLRender::BT_ALPHA);
                }

                {
                    //draw black overlay
                    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                    renderGrid(u,v,tiles,0.0f, 0.0f, 0.0f,a*0.16f);

                    //draw grid top
                    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, getGridTexName());
                    renderGrid(u,v,tiles,1,1,1,a);

                    gGL.popMatrix();
                    gGL.matrixMode(LLRender::MM_MODELVIEW);
                    gGL.popMatrix();
                }

                {
                    LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
                    renderGuidelines();
                }

                {
                    LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GREATER);
                    gGL.flush();

                    switch (mManipPart)
                    {
                      case LL_YZ_PLANE:
                        renderGuidelines(false, true, true);
                        break;
                      case LL_XZ_PLANE:
                        renderGuidelines(true, false, true);
                        break;
                      case LL_XY_PLANE:
                        renderGuidelines(true, true, false);
                        break;
                      default:
                        break;
                    }
                    gGL.flush();
                }
            }
        }
    }
}

void BDManipTranslateJoint::renderGrid(F32 x, F32 y, F32 size, F32 r, F32 g, F32 b, F32 a)
{
    F32 d = size*0.5f;

    for (F32 xx = -size-d; xx < size+d; xx += d)
    {
        gGL.begin(LLRender::TRIANGLE_STRIP);
        for (F32 yy = -size-d; yy < size+d; yy += d)
        {
            float dx, dy, da;

            dx = xx; dy = yy;
            da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
            gGL.texCoord2f(dx, dy);
            renderGridVert(dx,dy,r,g,b,da);

            dx = xx+d; dy = yy;
            da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
            gGL.texCoord2f(dx, dy);
            renderGridVert(dx,dy,r,g,b,da);

            dx = xx; dy = yy+d;
            da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
            gGL.texCoord2f(dx, dy);
            renderGridVert(dx,dy,r,g,b,da);

            dx = xx+d; dy = yy+d;
            da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
            gGL.texCoord2f(dx, dy);
            renderGridVert(dx,dy,r,g,b,da);
        }
        gGL.end();
    }
}

void BDManipTranslateJoint::highlightIntersection(LLVector3 normal,
                                             LLVector3 selection_center,
                                             LLQuaternion grid_rotation,
                                             LLColor4 inner_color)
{

}

void BDManipTranslateJoint::renderText()
{
    if (mJoint)
    {
        LLVector3 pos = getPivotPoint();
        renderXYZ(pos);
    }
}

void BDManipTranslateJoint::renderTranslationHandles()
{
    LLVector3 grid_origin;
    LLVector3 grid_scale;
    LLQuaternion grid_rotation;
    LLGLDepthTest gls_depth(GL_FALSE);

    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);
    LLVector3 at_axis;
    {
        at_axis = LLViewerCamera::getInstance()->getAtAxis() * ~grid_rotation;
    }

    if (at_axis.mV[VX] > 0.f)
    {
        mPlaneManipPositions.mV[VX] = 1.f;
    }
    else
    {
        mPlaneManipPositions.mV[VX] = -1.f;
    }

    if (at_axis.mV[VY] > 0.f)
    {
        mPlaneManipPositions.mV[VY] = 1.f;
    }
    else
    {
        mPlaneManipPositions.mV[VY] = -1.f;
    }

    if (at_axis.mV[VZ] > 0.f)
    {
        mPlaneManipPositions.mV[VZ] = 1.f;
    }
    else
    {
        mPlaneManipPositions.mV[VZ] = -1.f;
    }

    if (!mJoint) return;

    LLVector3 selection_center = getPivotPoint();

    // Drag handles
    {
        LLVector3 camera_pos_agent = gAgentCamera.getCameraPositionAgent();
        F32 range = dist_vec(camera_pos_agent, selection_center);
        F32 range_from_agent = dist_vec(gAgent.getPositionAgent(), selection_center);

        // Don't draw handles if you're too far away
        if (gSavedSettings.getBOOL("LimitSelectDistance"))
        {
            if (range_from_agent > gSavedSettings.getF32("MaxSelectDistance"))
            {
                return;
            }
        }

        if (range > 0.001f)
        {
            // range != zero
            F32 fraction_of_fov = mAxisArrowLength / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
            F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
            mArrowLengthMeters = range * tan(apparent_angle);
        }
        else
        {
            // range == zero
            mArrowLengthMeters = 1.0f;
        }
    }
    //Assume that UI scale factor is equivalent for X and Y axis
    F32 ui_scale_factor = LLUI::getScaleFactor().mV[VX];
    mArrowLengthMeters *= ui_scale_factor;

    mPlaneManipOffsetMeters = mArrowLengthMeters * 1.8f;
    mGridSizeMeters = gSavedSettings.getF32("GridDrawSize");
    mConeSize = mArrowLengthMeters / 4.f;

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    {
        gGL.translatef(selection_center.mV[VX], selection_center.mV[VY], selection_center.mV[VZ]);

        F32 angle_radians, x, y, z;
        grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);

        gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);

        LLQuaternion invRotation = grid_rotation;
        invRotation.conjQuat();

        LLVector3 relative_camera_dir;

        {
            relative_camera_dir = (selection_center - LLViewerCamera::getInstance()->getOrigin()) * invRotation;
        }
        relative_camera_dir.normVec();

        {
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            LLGLDisable cull_face(GL_CULL_FACE);

            LLColor4 color1;
            LLColor4 color2;

            // update manipulator sizes
            for (S32 index = 0; index < 3; index++)
            {
                if (index == mManipPart - LL_X_ARROW || index == mHighlightedPart - LL_X_ARROW)
                {
                    mArrowScales.mV[index] = lerp(mArrowScales.mV[index], SELECTED_ARROW_SCALE, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                    mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], 1.f, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                }
                else if (index == mManipPart - LL_YZ_PLANE || index == mHighlightedPart - LL_YZ_PLANE)
                {
                    mArrowScales.mV[index] = lerp(mArrowScales.mV[index], 1.f, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                    mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], SELECTED_ARROW_SCALE, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                }
                else
                {
                    mArrowScales.mV[index] = lerp(mArrowScales.mV[index], 1.f, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                    mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], 1.f, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
                }
            }

            if ((mManipPart == LL_NO_PART || mManipPart == LL_YZ_PLANE) && llabs(relative_camera_dir.mV[VX]) > MIN_PLANE_MANIP_DOT_PRODUCT)
            {
                // render YZ plane manipulator
                gGL.pushMatrix();
                gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);
                gGL.translatef(0.f, mPlaneManipOffsetMeters, mPlaneManipOffsetMeters);
                gGL.scalef(mPlaneScales.mV[VX], mPlaneScales.mV[VX], mPlaneScales.mV[VX]);
                if (mHighlightedPart == LL_YZ_PLANE)
                {
                    color1.setVec(0.f, 1.f, 0.f, 1.f);
                    color2.setVec(0.f, 0.f, 1.f, 1.f);
                }
                else
                {
                    color1.setVec(0.f, 1.f, 0.f, 0.6f);
                    color2.setVec(0.f, 0.f, 1.f, 0.6f);
                }
                gGL.begin(LLRender::TRIANGLES);
                {
                    gGL.color4fv(color1.mV);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f));
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));

                    gGL.color4fv(color2.mV);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
                }
                gGL.end();

                LLUI::setLineWidth(3.0f);
                gGL.begin(LLRender::LINES);
                {
                    gGL.color4f(0.f, 0.f, 0.f, 0.3f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f);

                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f,  mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f,  mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
                }
                gGL.end();
                LLUI::setLineWidth(1.0f);
                gGL.popMatrix();
            }

            if ((mManipPart == LL_NO_PART || mManipPart == LL_XZ_PLANE) && llabs(relative_camera_dir.mV[VY]) > MIN_PLANE_MANIP_DOT_PRODUCT)
            {
                // render XZ plane manipulator
                gGL.pushMatrix();
                gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);
                gGL.translatef(mPlaneManipOffsetMeters, 0.f, mPlaneManipOffsetMeters);
                gGL.scalef(mPlaneScales.mV[VY], mPlaneScales.mV[VY], mPlaneScales.mV[VY]);
                if (mHighlightedPart == LL_XZ_PLANE)
                {
                    color1.setVec(0.f, 0.f, 1.f, 1.f);
                    color2.setVec(1.f, 0.f, 0.f, 1.f);
                }
                else
                {
                    color1.setVec(0.f, 0.f, 1.f, 0.6f);
                    color2.setVec(1.f, 0.f, 0.f, 0.6f);
                }

                gGL.begin(LLRender::TRIANGLES);
                {
                    gGL.color4fv(color1.mV);
                    gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), 0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));

                    gGL.color4fv(color2.mV);
                    gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
                    gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f),   0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f));
                    gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f),   0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
                }
                gGL.end();

                LLUI::setLineWidth(3.0f);
                gGL.begin(LLRender::LINES);
                {
                    gGL.color4f(0.f, 0.f, 0.f, 0.3f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f);

                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f,   0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
                    gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f,   0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
                }
                gGL.end();
                LLUI::setLineWidth(1.0f);

                gGL.popMatrix();
            }

            if ((mManipPart == LL_NO_PART || mManipPart == LL_XY_PLANE) && llabs(relative_camera_dir.mV[VZ]) > MIN_PLANE_MANIP_DOT_PRODUCT)
            {
                // render XY plane manipulator
                gGL.pushMatrix();
                gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);

/*                            Y
                              ^
                              v1
                              |  \
                              |<- v0
                              |  /| \
                              v2__v__v3 > X
*/
                LLVector3 v0,v1,v2,v3;

                gGL.translatef(mPlaneManipOffsetMeters, mPlaneManipOffsetMeters, 0.f);
                v0 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f);
                v1 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), 0.f);
                v2 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), 0.f);
                v3 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), 0.f);

                gGL.scalef(mPlaneScales.mV[VZ], mPlaneScales.mV[VZ], mPlaneScales.mV[VZ]);
                if (mHighlightedPart == LL_XY_PLANE)
                {
                    color1.setVec(1.f, 0.f, 0.f, 1.f);
                    color2.setVec(0.f, 1.f, 0.f, 1.f);
                }
                else
                {
                    color1.setVec(0.8f, 0.f, 0.f, 0.6f);
                    color2.setVec(0.f, 0.8f, 0.f, 0.6f);
                }

                gGL.begin(LLRender::TRIANGLES);
                {
                    gGL.color4fv(color1.mV);
                    gGL.vertex3fv(v0.mV);
                    gGL.vertex3fv(v1.mV);
                    gGL.vertex3fv(v2.mV);

                    gGL.color4fv(color2.mV);
                    gGL.vertex3fv(v2.mV);
                    gGL.vertex3fv(v3.mV);
                    gGL.vertex3fv(v0.mV);
                }
                gGL.end();

                LLUI::setLineWidth(3.0f);
                gGL.begin(LLRender::LINES);
                {
                    gGL.color4f(0.f, 0.f, 0.f, 0.3f);
                    LLVector3 v12 = (v1 + v2) * .5f;
                    gGL.vertex3fv(v0.mV);
                    gGL.vertex3fv(v12.mV);
                    gGL.vertex3fv(v12.mV);
                    gGL.vertex3fv((v12 + (v0-v12)*.3f + (v2-v12)*.3f).mV);
                    gGL.vertex3fv(v12.mV);
                    gGL.vertex3fv((v12 + (v0-v12)*.3f + (v1-v12)*.3f).mV);

                    LLVector3 v23 = (v2 + v3) * .5f;
                    gGL.vertex3fv(v0.mV);
                    gGL.vertex3fv(v23.mV);
                    gGL.vertex3fv(v23.mV);
                    gGL.vertex3fv((v23 + (v0-v23)*.3f + (v3-v23)*.3f).mV);
                    gGL.vertex3fv(v23.mV);
                    gGL.vertex3fv((v23 + (v0-v23)*.3f + (v2-v23)*.3f).mV);
                }
                gGL.end();
                LLUI::setLineWidth(1.0f);

                gGL.popMatrix();
            }
        }
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        // Since we draw handles with depth testing off, we need to draw them in the
        // proper depth order.

        // Copied from LLDrawable::updateGeometry
        LLVector3 pos_agent     = mJoint->getWorldPosition();
        LLVector3 camera_agent  = gAgentCamera.getCameraPositionAgent();
        LLVector3 headPos       = pos_agent - camera_agent;

        LLVector3 orientWRTHead    = headPos * invRotation;

        // Find nearest vertex
        U32 nearest = (orientWRTHead.mV[0] < 0.0f ? 1 : 0) +
            (orientWRTHead.mV[1] < 0.0f ? 2 : 0) +
            (orientWRTHead.mV[2] < 0.0f ? 4 : 0);

        // opposite faces on Linden cubes:
        // 0 & 5
        // 1 & 3
        // 2 & 4

        // Table of order to draw faces, based on nearest vertex
        static U32 face_list[8][NUM_AXES*2] = {
            { 2,0,1, 4,5,3 }, // v6  F201 F453
            { 2,0,3, 4,5,1 }, // v7  F203 F451
            { 4,0,1, 2,5,3 }, // v5  F401 F253
            { 4,0,3, 2,5,1 }, // v4  F403 F251
            { 2,5,1, 4,0,3 }, // v2  F251 F403
            { 2,5,3, 4,0,1 }, // v3  F253 F401
            { 4,5,1, 2,0,3 }, // v1  F451 F203
            { 4,5,3, 2,0,1 }, // v0  F453 F201
        };
        static const EManipPart which_arrow[6] = {
            LL_Z_ARROW,
            LL_X_ARROW,
            LL_Y_ARROW,
            LL_X_ARROW,
            LL_Y_ARROW,
            LL_Z_ARROW};

        // draw arrows for deeper faces first, closer faces last
        LLVector3 camera_axis;
        {
            camera_axis.setVec(mJoint->getWorldPosition());
        }

        for (U32 i = 0; i < NUM_AXES*2; i++)
        {
            U32 face = face_list[nearest][i];

            LLVector3 arrow_axis;
            getManipAxis(nullptr, which_arrow[face], arrow_axis);

            renderArrow(which_arrow[face],
                        mManipPart,
                        (face >= 3) ? -mConeSize : mConeSize,
                        (face >= 3) ? -mArrowLengthMeters : mArrowLengthMeters,
                        mConeSize,
                        false);
        }
    }
    gGL.popMatrix();
}


void BDManipTranslateJoint::renderArrow(S32 which_arrow, S32 selected_arrow, F32 box_size, F32 arrow_size, F32 handle_size, bool reverse_direction)
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    LLGLEnable gls_blend(GL_BLEND);

    for (S32 pass = 1; pass <= 2; pass++)
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, pass == 1 ? GL_LEQUAL : GL_GREATER);
        gGL.pushMatrix();

        S32 index = 0;

        index = ARROW_TO_AXIS[which_arrow];

        // assign a color for this arrow
        LLColor4 color;  // black
        if (which_arrow == selected_arrow || which_arrow == mHighlightedPart)
        {
            color.mV[index] = (pass == 1) ? 1.f : 0.5f;
        }
        else if (selected_arrow != LL_NO_PART)
        {
            color.mV[VALPHA] = 0.f;
        }
        else
        {
            color.mV[index] = pass == 1 ? .8f : .35f ;          // red, green, or blue
            color.mV[VALPHA] = 0.6f;
        }
        gGL.color4fv( color.mV );

        LLVector3 vec;

        {
            LLUI::setLineWidth(2.0f);
            gGL.begin(LLRender::LINES);
                vec.mV[index] = box_size;
                gGL.vertex3f(vec.mV[0], vec.mV[1], vec.mV[2]);

                vec.mV[index] = arrow_size;
                gGL.vertex3f(vec.mV[0], vec.mV[1], vec.mV[2]);
            gGL.end();
            LLUI::setLineWidth(1.0f);
        }

        gGL.translatef(vec.mV[0], vec.mV[1], vec.mV[2]);
        gGL.scalef(handle_size, handle_size, handle_size);

        F32 rot = 0.0f;
        LLVector3 axis;

        switch(which_arrow)
        {
        case LL_X_ARROW:
            rot = reverse_direction ? -90.0f : 90.0f;
            axis.mV[1] = 1.0f;
            break;
        case LL_Y_ARROW:
            rot = reverse_direction ? 90.0f : -90.0f;
            axis.mV[0] = 1.0f;
            break;
        case LL_Z_ARROW:
            rot = reverse_direction ? 180.0f : 0.0f;
            axis.mV[0] = 1.0f;
            break;
        default:
            LL_ERRS() << "renderArrow called with bad arrow " << which_arrow << LL_ENDL;
            break;
        }

        gGL.diffuseColor4fv(color.mV);
        gGL.rotatef(rot, axis.mV[0], axis.mV[1], axis.mV[2]);
        gGL.scalef(mArrowScales.mV[index], mArrowScales.mV[index], mArrowScales.mV[index] * 1.5f);

        gCone.render();

        gGL.popMatrix();
    }
}

void BDManipTranslateJoint::renderGridVert(F32 x_trans, F32 y_trans, F32 r, F32 g, F32 b, F32 alpha)
{
    gGL.color4f(r, g, b, alpha);
    switch (mManipPart)
    {
    case LL_YZ_PLANE:
        gGL.vertex3f(0, x_trans, y_trans);
        break;
    case LL_XZ_PLANE:
        gGL.vertex3f(x_trans, 0, y_trans);
        break;
    case LL_XY_PLANE:
        gGL.vertex3f(x_trans, y_trans, 0);
        break;
    default:
        gGL.vertex3f(0,0,0);
        break;
    }
}

//BD - Overrides
LLVector3 BDManipTranslateJoint::getPivotPoint()
{
    if (mJoint)
    {
        return mJoint->getWorldPosition();
    }

    return LLVector3::zero;
}
