/** 
 * @file lldrawpoolpbropaque.cpp
 * @brief LLDrawPoolGLTFPBR class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "lldrawpool.h"
#include "lldrawpoolpbropaque.h"
#include "llviewershadermgr.h"
#include "pipeline.h"

static const U32 gltf_render_types[] = { LLPipeline::RENDER_TYPE_PASS_GLTF_PBR, LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK };

LLDrawPoolGLTFPBR::LLDrawPoolGLTFPBR() :
    LLRenderPass(POOL_GLTF_PBR)
{
}

S32 LLDrawPoolGLTFPBR::getNumDeferredPasses()
{
    return 1;
}

void LLDrawPoolGLTFPBR::renderDeferred(S32 pass)
{
    llassert(!LLPipeline::sRenderingHUDs);

    for (U32 type : gltf_render_types)
    {
        gDeferredPBROpaqueProgram.bind();
        pushGLTFBatches(type);

        gDeferredPBROpaqueProgram.bind(true);
        pushRiggedGLTFBatches(type + 1);
    }
}

S32 LLDrawPoolGLTFPBR::getNumPostDeferredPasses()
{
    return 1;
}

void LLDrawPoolGLTFPBR::renderPostDeferred(S32 pass)
{
    if (LLPipeline::sRenderingHUDs)
    {
        gHUDPBROpaqueProgram.bind();
        for (U32 type : gltf_render_types)
        {
            pushGLTFBatches(type);
        }
    }
    else
    {
        gGL.setColorMask(false, true);
        gPBRGlowProgram.bind();
        pushGLTFBatches(LLRenderPass::PASS_GLTF_GLOW);

        gPBRGlowProgram.bind(true);
        pushRiggedGLTFBatches(LLRenderPass::PASS_GLTF_GLOW_RIGGED);

        gGL.setColorMask(true, false);
    }
}

