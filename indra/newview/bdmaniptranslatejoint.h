/**
 * @file llmaniptranslate.h
 * @brief BDManipTranslateJoint class definition
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

#ifndef BD_MANIPTRANSLATEJOINT_H
#define BD_MANIPTRANSLATEJOINT_H

#include "llmaniptranslate.h"
#include "lltimer.h"
#include "v4math.h"
#include "llquaternion.h"

class LLJoint;
class LLVOAvatar;  // or LLVOAvatarSelf, etc.

class BDManipTranslateJoint : public LLManipTranslate
{
    // Used for overriding the natural "up" direction of a joint.
    // if no override is set then the world up direction is used unless the joint is vertical in which case we pick an arbitrary normal
    //static std::unordered_map<std::string, LLVector3> sReferenceUpVectors;

    struct BoneAxes
    {
        LLVector3 naturalX;
        LLVector3 naturalY;
        LLVector3 naturalZ;
    };
    BoneAxes computeBoneAxes() const;

public:
    class ManipulatorHandle
    {
    public:
        LLVector3   mStartPosition;
        LLVector3   mEndPosition;
        EManipPart  mManipID;
        F32         mHotSpotRadius;

        ManipulatorHandle(LLVector3 start_pos, LLVector3 end_pos, EManipPart id, F32 radius):mStartPosition(start_pos), mEndPosition(end_pos), mManipID(id), mHotSpotRadius(radius){}
    };


    BDManipTranslateJoint( LLToolComposite* composite );
    virtual ~BDManipTranslateJoint();

    static std::string getManipPartString(EManipPart part);

    static  U32     getGridTexName() ;
    static  void    destroyGL();
    static  void    restoreGL();
    bool    handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool    handleMouseUp(S32 x, S32 y, MASK mask)override;
    bool    handleHover(S32 x, S32 y, MASK mask)override;
    void    render() override;
    void    handleSelect() override;

    void    highlightManipulators(S32 x, S32 y) override;
    bool    handleMouseDownOnPart(S32 x, S32 y, MASK mask) override;

    //BD - Overrides
    LLVector3 getPivotPoint();

    // Called to designate which joint we are going to manipulate.
    void setJoint(LLJoint* joint);

    void setAvatar(LLVOAvatar* avatar);

protected:
    enum EHandleType {
        HANDLE_CONE,
        HANDLE_BOX,
        HANDLE_SPHERE
    };

    void        renderArrow(S32 which_arrow, S32 selected_arrow, F32 box_size, F32 arrow_size, F32 handle_size, bool reverse_direction);
    void        renderTranslationHandles();
    void        renderText();
    void        renderSnapGuides();
    void        renderGrid(F32 x, F32 y, F32 size, F32 r, F32 g, F32 b, F32 a);
    void        renderGridVert(F32 x_trans, F32 y_trans, F32 r, F32 g, F32 b, F32 alpha);
    void        highlightIntersection(LLVector3 normal,
                                     LLVector3 selection_center,
                                     LLQuaternion grid_rotation,
                                     LLColor4 inner_color);
    F32         getMinGridScale();

    LLJoint* mJoint = nullptr;
    BoneAxes     mBoneAxes;
    LLVOAvatar* mAvatar = nullptr;
    // We'll store the joint's original rotation for reference
    LLVector3 mSavedJointPos;
    LLVector3d mSavedJointPosGlobal;
    LLJoint* mHighlightedJoint = nullptr;
    F32       mHighlightedPartDistance = 0.f;
    LLVector3 mInitialIntersection;  // The initial point on the manipulator’s sphere (in agent space)

private:
    S32         mLastHoverMouseX;
    S32         mLastHoverMouseY;
    bool        mMouseOutsideSlop;      // true after mouse goes outside slop region
    bool        mCopyMadeThisDrag;
    S32         mMouseDownX;
    S32         mMouseDownY;
    F32         mAxisArrowLength;       // pixels
    F32         mConeSize;              // meters, world space
    F32         mArrowLengthMeters;     // meters
    F32         mGridSizeMeters;
    F32         mPlaneManipOffsetMeters;
    LLVector3   mManipNormal;
    LLVector3  mDragCursorStartLocal;
    LLVector3d  mDragCursorStartGlobal;
    LLVector3d  mDragSelectionStartGlobal;
    LLTimer     mUpdateTimer;
    LLVector4   mManipulatorVertices[18];
    F32         mSnapOffsetMeters;
    LLVector3   mSnapOffsetAxis;
    LLQuaternion mGridRotation;
    LLVector3   mGridOrigin;
    LLVector3   mGridScale;
    F32         mSubdivisions;
    bool        mInSnapRegime;
    LLVector3   mArrowScales;
    LLVector3   mPlaneScales;
    LLVector4   mPlaneManipPositions;
};

#endif
