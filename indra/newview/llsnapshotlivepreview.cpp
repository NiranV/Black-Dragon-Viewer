/**
* @file llsnapshotlivepreview.cpp
* @brief Implementation of llsnapshotlivepreview
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llfilesystem.h"
#include "llcombobox.h"
#include "llfloaterperms.h"
#include "llfloaterreg.h"
#include "llimagefilter.h"
#include "llimagefiltersmanager.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "lllandmarkactions.h"
#include "lllocalcliprect.h"
#include "llresmgr.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "llsnapshotlivepreview.h"
#include "lltoolfocus.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"   // upload_new_resource()
#include "llviewerstats.h"
#include "llviewertexturelist.h"
#include "llwindow.h"
#include "llworld.h"
#include <boost/filesystem.hpp>

S32 BORDER_WIDTH = 6;
S32 TOP_PANEL_HEIGHT = 30;

constexpr S32 MAX_TEXTURE_SIZE = 2048 ; //max upload texture size 2048 * 2048

std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;
LLPointer<LLImageFormatted> LLSnapshotLivePreview::sSaveLocalImage = nullptr;

LLSnapshotLivePreview::LLSnapshotLivePreview (const LLSnapshotLivePreview::Params& p)
    :   LLView(p),
    mColor(1.f, 0.f, 0.f, 0.5f),
    mCurImageIndex(0),
    mPreviewImage(NULL),
    mThumbnailImage(NULL) ,
    mBigThumbnailImage(NULL) ,
	mThumbnailWidth(0),
	mThumbnailHeight(0),
    mThumbnailSubsampled(false),
	mPreviewImageEncoded(NULL),
	mFormattedImage(NULL),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mDataSize(0),
	mSnapshotType(LLSnapshotModel::SNAPSHOT_POSTCARD),
	mSnapshotFormat(LLSnapshotModel::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat"))),
	mSnapshotUpToDate(false),
	mCameraPos(LLViewerCamera::getInstance()->getOrigin()),
	mCameraRot(LLViewerCamera::getInstance()->getQuaternion()),
	mSnapshotActive(false),
	mSnapshotBufferType(LLSnapshotModel::SNAPSHOT_TYPE_COLOR),
    mFilterName(""),
    mAllowRenderUI(true),
    mAllowFullScreenPreview(true),
    mViewContainer(NULL)
{
    setSnapshotQuality(gSavedSettings.getS32("SnapshotQuality"));
    mSnapshotDelayTimer.setTimerExpirySec(0.0f);
    mSnapshotDelayTimer.start();
    //  gIdleCallbacks.addFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
    sList.insert(this);
    setFollowsAll();
    mWidth[0] = gViewerWindow->getWindowWidthRaw();
    mWidth[1] = gViewerWindow->getWindowWidthRaw();
    mHeight[0] = gViewerWindow->getWindowHeightRaw();
    mHeight[1] = gViewerWindow->getWindowHeightRaw();
    mImageScaled[0] = false;
    mImageScaled[1] = false;

    mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
    mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;
    mThumbnailUpdateLock = false ;
    mThumbnailUpToDate   = false ;
    mBigThumbnailUpToDate = false ;

    mForceUpdateSnapshot = false;
}

LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
    // delete images
    mPreviewImage = NULL;
    mPreviewImageEncoded = NULL;
    mFormattedImage = NULL;

    //  gIdleCallbacks.deleteFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
    sList.erase(this);
    sSaveLocalImage = NULL;
}

void LLSnapshotLivePreview::setMaxImageSize(S32 size)
{
    mMaxImageSize = llmin(size,(S32)(MAX_SNAPSHOT_IMAGE_SIZE));
}

LLViewerTexture* LLSnapshotLivePreview::getCurrentImage()
{
    return mViewerImage[mCurImageIndex];
}

F32 LLSnapshotLivePreview::getImageAspect()
{
    if (!getCurrentImage())
    {
        return 0.f;
    }
    // mKeepAspectRatio) == gSavedSettings.getBOOL("KeepAspectForSnapshot"))
    return (mKeepAspectRatio ? ((F32)getRect().getWidth()) / ((F32)getRect().getHeight()) : ((F32)getWidth()) / ((F32)getHeight()));
}

void LLSnapshotLivePreview::updateSnapshot(bool new_snapshot, bool new_thumbnail, F32 delay)
{
    LL_DEBUGS("Snapshot") << "updateSnapshot: mSnapshotUpToDate = " << getSnapshotUpToDate() << LL_ENDL;

    // Update snapshot if requested.
    if (new_snapshot)
    {
        if (getSnapshotUpToDate())
        {
            S32 old_image_index = mCurImageIndex;
            mCurImageIndex = (mCurImageIndex + 1) % 2; 
            setSize(mWidth[old_image_index], mHeight[old_image_index]);	
        }
        mSnapshotUpToDate = false;

        // Update snapshot source rect depending on whether we keep the aspect ratio.
        LLRect& rect = mImageRect[mCurImageIndex];
        rect.set(0, getRect().getHeight(), getRect().getWidth(), 0);

        F32 image_aspect_ratio = ((F32)getWidth()) / ((F32)getHeight());
        F32 window_aspect_ratio = ((F32)getRect().getWidth()) / ((F32)getRect().getHeight());

        if (mKeepAspectRatio)//gSavedSettings.getBOOL("KeepAspectForSnapshot"))
        {
            if (image_aspect_ratio > window_aspect_ratio)
            {
                // trim off top and bottom
                S32 new_height = ll_round((F32)getRect().getWidth() / image_aspect_ratio);
                rect.mBottom += (getRect().getHeight() - new_height) / 2;
                rect.mTop -= (getRect().getHeight() - new_height) / 2;
            }
            else if (image_aspect_ratio < window_aspect_ratio)
            {
                // trim off left and right
                S32 new_width = ll_round((F32)getRect().getHeight() * image_aspect_ratio);
                rect.mLeft += (getRect().getWidth() - new_width) / 2;
                rect.mRight -= (getRect().getWidth() - new_width) / 2;
            }
        }

		//BD
		mSnapshotDelayTimer.setTimerExpirySec(delay);
		mSnapshotDelayTimer.start();
        
		mPosTakenGlobal = gAgentCamera.getCameraPositionGlobal();

        // Tell the floater container that the snapshot is in the process of updating itself
        if (mViewContainer)
        {
            mViewContainer->notify(LLSD().with("snapshot-updating", true));
        }
    }

    // Update thumbnail if requested.
    if (new_thumbnail)
    {
        mThumbnailUpToDate = false ;
        mBigThumbnailUpToDate = false;
    }
}

// Return true if the quality has been changed, false otherwise
bool LLSnapshotLivePreview::setSnapshotQuality(S32 quality, bool set_by_user)
{
    llclamp(quality, 0, 100);
    if (quality != mSnapshotQuality)
    {
        mSnapshotQuality = quality;
        if (set_by_user)
        {
            gSavedSettings.setS32("SnapshotQuality", quality);
        }
        mFormattedImage = NULL;     // Invalidate the already formatted image if any
        return true;
    }
    return false;
}

void LLSnapshotLivePreview::drawPreviewRect(S32 offset_x, S32 offset_y, LLColor4 alpha_color)
{
    F32 line_width ;
    glGetFloatv(GL_LINE_WIDTH, &line_width) ;
    glLineWidth(2.0f * line_width) ;
    LLColor4 color(0.0f, 0.0f, 0.0f, 1.0f) ;
    gl_rect_2d( mPreviewRect.mLeft + offset_x, mPreviewRect.mTop + offset_y,
        mPreviewRect.mRight + offset_x, mPreviewRect.mBottom + offset_y, color, false ) ;
    glLineWidth(line_width) ;

    //draw four alpha rectangles to cover areas outside of the snapshot image
    if(!mKeepAspectRatio)
    {
        S32 dwl = 0, dwr = 0 ;
        if(mThumbnailWidth > mPreviewRect.getWidth())
        {
            dwl = (mThumbnailWidth - mPreviewRect.getWidth()) >> 1 ;
            dwr = mThumbnailWidth - mPreviewRect.getWidth() - dwl ;

            gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y,
                mPreviewRect.mLeft + offset_x, mPreviewRect.mBottom + offset_y, alpha_color, true ) ;
            gl_rect_2d( mPreviewRect.mRight + offset_x, mPreviewRect.mTop + offset_y,
                mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y, alpha_color, true ) ;
        }

        if(mThumbnailHeight > mPreviewRect.getHeight())
        {
            S32 dh = (mThumbnailHeight - mPreviewRect.getHeight()) >> 1 ;
            gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mBottom + offset_y ,
                mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y - dh, alpha_color, true ) ;

            dh = mThumbnailHeight - mPreviewRect.getHeight() - dh ;
            gl_rect_2d( mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y + dh,
                mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mTop + offset_y, alpha_color, true ) ;
        }
    }
}

//BD - Called when the world is frozen.
void LLSnapshotLivePreview::draw()
{
	// Do nothing.
}

/*virtual*/
void LLSnapshotLivePreview::reshape(S32 width, S32 height, bool called_from_parent)
{
	LLRect old_rect = getRect();
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		// _LL_DEBUGS("Window", "Snapshot") << "window reshaped, updating thumbnail" << LL_ENDL;
		if (mViewContainer && mViewContainer->isInVisibleChain())
		{
			// We usually resize only on window reshape, so give it a chance to redraw, assign delay
			updateSnapshot(
				true, // new snapshot is needed
				false, // thumbnail will be updated either way.
				0.0f); // shutter delay.
		}
	}
}

bool LLSnapshotLivePreview::setThumbnailImageSize()
{
    if (getWidth() < 10 || getHeight() < 10)
    {
        return false ;
    }
    S32 width  = (mThumbnailSubsampled ? mPreviewImage->getWidth()  : gViewerWindow->getWindowWidthRaw());
    S32 height = (mThumbnailSubsampled ? mPreviewImage->getHeight() : gViewerWindow->getWindowHeightRaw()) ;

    F32 aspect_ratio = ((F32)width) / ((F32)height);

    // UI size for thumbnail
    S32 max_width  = mThumbnailPlaceholderRect.getWidth();
    S32 max_height = mThumbnailPlaceholderRect.getHeight();

    if (aspect_ratio > (F32)max_width / (F32)max_height)
    {
        // image too wide, shrink to width
        mThumbnailWidth = max_width;
        mThumbnailHeight = ll_round((F32)max_width / aspect_ratio);
    }
    else
    {
        // image too tall, shrink to height
        mThumbnailHeight = max_height;
        mThumbnailWidth = ll_round((F32)max_height * aspect_ratio);
    }

    if (mThumbnailWidth > width || mThumbnailHeight > height)
    {
        return false ;//if the window is too small, ignore thumbnail updating.
    }

    S32 left = 0 , top = mThumbnailHeight, right = mThumbnailWidth, bottom = 0 ;
    if (!mKeepAspectRatio)
    {
        F32 ratio_x = (F32)getWidth()  / width ;
        F32 ratio_y = (F32)getHeight() / height ;

        if (ratio_x > ratio_y)
        {
            top = (S32)(top * ratio_y / ratio_x) ;
        }
        else
        {
            right = (S32)(right * ratio_x / ratio_y) ;
        }
        left = (S32)((mThumbnailWidth - right) * 0.5f) ;
        bottom = (S32)((mThumbnailHeight - top) * 0.5f) ;
        top += bottom ;
        right += left ;
    }
    mPreviewRect.set(left - 1, top + 1, right + 1, bottom - 1) ;

    return true ;
}

void LLSnapshotLivePreview::generateThumbnailImage(bool force_update)
{
    if(mThumbnailUpdateLock) //in the process of updating
    {
        return ;
    }
    if(getThumbnailUpToDate() && !force_update)//already updated
    {
        return ;
    }
    if(getWidth() < 10 || getHeight() < 10)
    {
        return ;
    }

    ////lock updating
    mThumbnailUpdateLock = true ;

    if(!setThumbnailImageSize())
    {
        mThumbnailUpdateLock = false ;
        mThumbnailUpToDate = true ;
        return ;
    }

    // Invalidate the big thumbnail when we regenerate the small one
    mBigThumbnailUpToDate = false;

    if(mThumbnailImage)
    {
        resetThumbnailImage() ;
    }

    LLPointer<LLImageRaw> raw = new LLImageRaw;

	//BD
	if (raw)
	{
		raw->resize( mPreviewImage->getWidth(),
                     mPreviewImage->getHeight(),
                     mPreviewImage->getComponents());
        raw->copy(mPreviewImage);

        // Scale to the thumbnail size
        if (!raw->scale(mThumbnailWidth, mThumbnailHeight))
        {
            raw = NULL ;
        }
    }
    else
    {
        // The thumbnail is a screen view with screen grab positioning preview
        if(!gViewerWindow->thumbnailSnapshot(raw,
                                         mThumbnailWidth, mThumbnailHeight,
                                         mAllowRenderUI && gSavedSettings.getBOOL("RenderUIInSnapshot"),
                                         gSavedSettings.getBOOL("RenderHUDInSnapshot"),
                                         false,
                                         gSavedSettings.getBOOL("RenderSnapshotNoPost"),
                                         mSnapshotBufferType) )
        {
            raw = NULL ;
        }
    }

    if (raw)
    {
        // Filter the thumbnail
        // Note: filtering needs to be done *before* the scaling to power of 2 or the effect is distorted
        if (getFilter() != "")
        {
            std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
            if (filter_path != "")
            {
                LLImageFilter filter(filter_path);
                filter.executeFilter(raw);
            }
            else
            {
                LL_WARNS("Snapshot") << "Couldn't find a path to the following filter : " << getFilter() << LL_ENDL;
            }
        }
        // Scale to a power of 2 so it can be mapped to a texture
        raw->expandToPowerOfTwo();
        mThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), false);
        mThumbnailUpToDate = true ;
    }

    //unlock updating
    mThumbnailUpdateLock = false ;
}

LLViewerTexture* LLSnapshotLivePreview::getBigThumbnailImage()
{
	if (mThumbnailUpdateLock) //in the process of updating
	{
		return NULL;
	}
	if (mBigThumbnailUpToDate && mBigThumbnailImage)//already updated
	{
		return mBigThumbnailImage;
	}
    
	LLPointer<LLImageRaw> raw = new LLImageRaw;
    
	if (raw)
	{
        // The big thumbnail is a new filtered version of the preview (used in SL Share previews, i.e. Flickr, Twitter, Facebook)
        mBigThumbnailWidth = mPreviewImage->getWidth();
        mBigThumbnailHeight = mPreviewImage->getHeight();
        raw->resize( mBigThumbnailWidth,
                     mBigThumbnailHeight,
                     mPreviewImage->getComponents());
        raw->copy(mPreviewImage);

        // Filter
        // Note: filtering needs to be done *before* the scaling to power of 2 or the effect is distorted
        if (getFilter() != "")
        {
            std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
            if (filter_path != "")
            {
                LLImageFilter filter(filter_path);
                filter.executeFilter(raw);
            }
            else
            {
                LL_WARNS("Snapshot") << "Couldn't find a path to the following filter : " << getFilter() << LL_ENDL;
            }
        }
        // Scale to a power of 2 so it can be mapped to a texture
        raw->expandToPowerOfTwo();
        mBigThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), false);
        mBigThumbnailUpToDate = true ;
    }

    return mBigThumbnailImage ;
}

// Called often. Checks whether it's time to grab a new snapshot and if so, does it.
// Returns true if new snapshot generated, false otherwise.
//static
bool LLSnapshotLivePreview::onIdle( void* snapshot_preview )
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)snapshot_preview;
	if (previewp->getWidth() == 0 || previewp->getHeight() == 0)
	{
		LL_WARNS("Snapshot") << "Incorrect dimensions: " << previewp->getWidth() << "x" << previewp->getHeight() << LL_ENDL;
		return false;
	}

	//BD
	if (previewp->mForceUpdateSnapshot)
	{
		previewp->updateSnapshot(
			true, // whether a new snapshot is needed or merely invalidate the existing one
			false, // or if 1st arg is false, whether to produce a new thumbnail image.
			0.f); // shutter delay if 1st arg is true.
		previewp->mForceUpdateSnapshot = false;
	}

	//BD - see if it's time yet to snap the shot and bomb out otherwise.
	previewp->mSnapshotActive = 
		(previewp->mSnapshotDelayTimer.getStarted() &&	previewp->mSnapshotDelayTimer.hasExpired())
		&& !LLToolCamera::getInstance()->hasMouseCapture(); // don't take snapshots while ALT-zoom active
	if (!previewp->mSnapshotActive && previewp->getSnapshotUpToDate() && previewp->getThumbnailUpToDate())
	{
		return false;
	}

    static LLCachedControl<bool> auto_snapshot(gSavedSettings, "AutoSnapshot", false);
    static LLCachedControl<bool> freeze_time(gSavedSettings, "FreezeTime", false);
    static LLCachedControl<bool> use_freeze_frame(gSavedSettings, "UseFreezeFrame", false);
    static LLCachedControl<bool> render_ui(gSavedSettings, "RenderUIInSnapshot", false);
    static LLCachedControl<bool> render_hud(gSavedSettings, "RenderHUDInSnapshot", false);
    static LLCachedControl<bool> render_no_post(gSavedSettings, "RenderSnapshotNoPost", false);

	// time to produce a snapshot
	if(!previewp->getSnapshotUpToDate())
	{
		// _LL_DEBUGS("Snapshot") << "producing snapshot" << LL_ENDL;
		if (!previewp->mPreviewImage)
		{
			previewp->mPreviewImage = new LLImageRaw;
		}

		previewp->setVisible(false);
		previewp->setEnabled(false);

		previewp->getWindow()->incBusyCount();
		previewp->setImageScaled(false);

		// grab the raw image
		if (gViewerWindow->rawSnapshot(
				previewp->mPreviewImage,
				previewp->getWidth(),
				previewp->getHeight(),
				previewp->mKeepAspectRatio,//gSavedSettings.getBOOL("KeepAspectForSnapshot"),
                previewp->getSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE,
                previewp->mAllowRenderUI && render_ui,
                render_hud,
                false,
                render_no_post,
                previewp->mSnapshotBufferType,
                previewp->getMaxImageSize()))
        {
            // Invalidate/delete any existing encoded image
            previewp->mPreviewImageEncoded = NULL;
            // Invalidate/delete any existing formatted image
            previewp->mFormattedImage = NULL;
            // Update the data size
            previewp->estimateDataSize();

            // The snapshot is updated now...
            previewp->mSnapshotUpToDate = true;
        
			// We need to update the thumbnail though
			previewp->setThumbnailImageSize();
			previewp->generateThumbnailImage(true) ;
		}
		previewp->getWindow()->decBusyCount();
		//BD
		previewp->mSnapshotDelayTimer.stop();
		previewp->mSnapshotActive = false;
		LL_DEBUGS("Snapshot") << "done creating snapshot" << LL_ENDL;
	}
    
	if (!previewp->getThumbnailUpToDate())
	{
		previewp->generateThumbnailImage() ;
	}
    
	// Tell the floater container that the snapshot is updated now
	if (previewp->mViewContainer)
	{
		previewp->mViewContainer->notify(LLSD().with("snapshot-updated", true));
	}

    return true;
}

S32 LLSnapshotLivePreview::getEncodedImageWidth() const
{
    S32 width = getWidth();
    if (getSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
    {
        width = LLImageRaw::biasedDimToPowerOfTwo(width,MAX_TEXTURE_SIZE);
    }
    return width;
}
S32 LLSnapshotLivePreview::getEncodedImageHeight() const
{
    S32 height = getHeight();
    if (getSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
    {
        height = LLImageRaw::biasedDimToPowerOfTwo(height,MAX_TEXTURE_SIZE);
    }
    return height;
}

LLPointer<LLImageRaw> LLSnapshotLivePreview::getEncodedImage()
{
    if (!mPreviewImageEncoded)
    {
        LLImageDataSharedLock lock(mPreviewImage);

        mPreviewImageEncoded = new LLImageRaw;

        mPreviewImageEncoded->resize(
            mPreviewImage->getWidth(),
            mPreviewImage->getHeight(),
            mPreviewImage->getComponents());

        if (getSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
        {
            // We don't store the intermediate formatted image in mFormattedImage in the J2C case
            LL_DEBUGS("Snapshot") << "Encoding new image of format J2C" << LL_ENDL;
            LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
            // Copy the preview
            LLPointer<LLImageRaw> scaled = new LLImageRaw(
                                                          mPreviewImage->getData(),
                                                          mPreviewImage->getWidth(),
                                                          mPreviewImage->getHeight(),
                                                          mPreviewImage->getComponents());
            // Scale it as required by J2C
            scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
            setImageScaled(true);
            // Compress to J2C
            if (formatted->encode(scaled, 0.f))
            {
                // We can update the data size precisely at that point
                mDataSize = formatted->getDataSize();
                // Decompress back
                formatted->decode(mPreviewImageEncoded, 0);
            }
        }
        else
        {
            // Update mFormattedImage if necessary
            lock.unlock();
            getFormattedImage(); // will apply filters to mPreviewImage with a lock
            lock.lock();
            if (getSnapshotFormat() == LLSnapshotModel::SNAPSHOT_FORMAT_BMP)
            {
                // BMP hack : copy instead of decode otherwise decode will crash.
                mPreviewImageEncoded->copy(mPreviewImage);
            }
            else
            {
                // Decode back
                mFormattedImage->decode(mPreviewImageEncoded, 0);
            }
        }
    }
    return mPreviewImageEncoded;
}

bool LLSnapshotLivePreview::createUploadFile(const std::string &out_filename, const S32 max_image_dimentions, const S32 min_image_dimentions)
{
    return LLViewerTextureList::createUploadFile(mPreviewImage, out_filename, max_image_dimentions, min_image_dimentions);
}

// We actually estimate the data size so that we do not require actual compression when showing the preview
// Note : whenever formatted image is computed, mDataSize will be updated to reflect the true size
void LLSnapshotLivePreview::estimateDataSize()
{
    // Compression ratio
    F32 ratio = 1.0;

    if (getSnapshotType() == LLSnapshotModel::SNAPSHOT_TEXTURE)
    {
        ratio = 8.0;    // This is what we shoot for when compressing to J2C
    }
    else
    {
        LLSnapshotModel::ESnapshotFormat format = getSnapshotFormat();
        switch (format)
        {
            case LLSnapshotModel::SNAPSHOT_FORMAT_PNG:
                ratio = 3.0;    // Average observed PNG compression ratio
                break;
            case LLSnapshotModel::SNAPSHOT_FORMAT_JPEG:
                // Observed from JPG compression tests
                ratio = (F32)(110 - mSnapshotQuality) / 2.f;
                break;
            case LLSnapshotModel::SNAPSHOT_FORMAT_BMP:
                ratio = 1.0;    // No compression with BMP
                break;
        }
    }
    mDataSize = (S32)((F32)mPreviewImage->getDataSize() / ratio);
}

LLPointer<LLImageFormatted> LLSnapshotLivePreview::getFormattedImage()
{
    if (!mFormattedImage)
    {
        // Apply the filter to mPreviewImage
        if (getFilter() != "")
        {
            std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
            if (filter_path != "")
            {
                LLImageFilter filter(filter_path);
                filter.executeFilter(mPreviewImage);
            }
            else
            {
                LL_WARNS("Snapshot") << "Couldn't find a path to the following filter : " << getFilter() << LL_ENDL;
            }
        }

        // Create the new formatted image of the appropriate format.
        LLSnapshotModel::ESnapshotFormat format = getSnapshotFormat();
        LL_DEBUGS("Snapshot") << "Encoding new image of format " << format << LL_ENDL;

        switch (format)
        {
            case LLSnapshotModel::SNAPSHOT_FORMAT_PNG:
                mFormattedImage = new LLImagePNG();
                break;
            case LLSnapshotModel::SNAPSHOT_FORMAT_JPEG:
                mFormattedImage = new LLImageJPEG(mSnapshotQuality);
                break;
            case LLSnapshotModel::SNAPSHOT_FORMAT_BMP:
                mFormattedImage = new LLImageBMP();
                break;
        }
        if (mFormattedImage->encode(mPreviewImage, 0))
        {
            // We can update the data size precisely at that point
            mDataSize = mFormattedImage->getDataSize();
        }
    }
    return mFormattedImage;
}

void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
    LL_DEBUGS("Snapshot") << "setSize(" << w << ", " << h << ")" << LL_ENDL;
    setWidth(w);
    setHeight(h);
}

void LLSnapshotLivePreview::setSnapshotFormat(LLSnapshotModel::ESnapshotFormat format)
{
    if (mSnapshotFormat != format)
    {
        mFormattedImage = NULL;     // Invalidate the already formatted image if any
        mSnapshotFormat = format;
    }
}

void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
    w = getWidth();
    h = getHeight();
}

void LLSnapshotLivePreview::saveTexture(bool outfit_snapshot, std::string name)
{
    LLImageDataSharedLock lock(mPreviewImage);

    LL_DEBUGS("Snapshot") << "saving texture: " << mPreviewImage->getWidth() << "x" << mPreviewImage->getHeight() << LL_ENDL;
    // gen a new uuid for this asset
    LLTransactionID tid;
    tid.generate();
    LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

    LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
    LLPointer<LLImageRaw> scaled = new LLImageRaw(mPreviewImage->getData(),
        mPreviewImage->getWidth(),
        mPreviewImage->getHeight(),
        mPreviewImage->getComponents());

    // Apply the filter to mPreviewImage
    if (getFilter() != "")
    {
        std::string filter_path = LLImageFiltersManager::getInstance()->getFilterPath(getFilter());
        if (filter_path != "")
        {
            LLImageFilter filter(filter_path);
            filter.executeFilter(scaled);
        }
        else
        {
            LL_WARNS("Snapshot") << "Couldn't find a path to the following filter : " << getFilter() << LL_ENDL;
        }
    }

    scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
    LL_DEBUGS("Snapshot") << "scaled texture to " << scaled->getWidth() << "x" << scaled->getHeight() << LL_ENDL;

    if (formatted->encode(scaled, 0.0f))
    {
        LLFileSystem fmt_file(new_asset_id, LLAssetType::AT_TEXTURE, LLFileSystem::WRITE);
        fmt_file.write(formatted->getData(), formatted->getDataSize());
        std::string pos_string;
        LLAgentUI::buildLocationString(pos_string, LLAgentUI::LOCATION_FORMAT_FULL);
        std::string who_took_it;
        LLAgentUI::buildFullname(who_took_it);
        S32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost(scaled->getWidth(), scaled->getHeight());
        std::string res_name = outfit_snapshot ? name : "Snapshot : " + pos_string;
        std::string res_desc = outfit_snapshot ? "" : "Taken by " + who_took_it + " at " + pos_string;
        LLFolderType::EType folder_type = outfit_snapshot ? LLFolderType::FT_NONE : LLFolderType::FT_SNAPSHOT_CATEGORY;
        LLInventoryType::EType inv_type = outfit_snapshot ? LLInventoryType::IT_NONE : LLInventoryType::IT_SNAPSHOT;

        LLResourceUploadInfo::ptr_t assetUploadInfo(new LLResourceUploadInfo(
            tid, LLAssetType::AT_TEXTURE, res_name, res_desc, 0,
            folder_type, inv_type,
            PERM_ALL, LLFloaterPerms::getGroupPerms("Uploads"), LLFloaterPerms::getEveryonePerms("Uploads"),
            expected_upload_cost, !outfit_snapshot));

        upload_new_resource(assetUploadInfo);

        gViewerWindow->playSnapshotAnimAndSound();
    }
    else
    {
        LLNotificationsUtil::add("ErrorEncodingSnapshot");
        LL_WARNS("Snapshot") << "Error encoding snapshot" << LL_ENDL;
    }

    add(LLStatViewer::SNAPSHOT, 1);

    mDataSize = 0;
}

void LLSnapshotLivePreview::saveLocal(const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb)
{
    // Update mFormattedImage if necessary
    getFormattedImage();

    // Save the formatted image
    saveLocal(mFormattedImage, success_cb, failure_cb);
}

//Check if failed due to insufficient memory
void LLSnapshotLivePreview::saveLocal(LLPointer<LLImageFormatted> image, const snapshot_saved_signal_t::slot_type& success_cb, const snapshot_saved_signal_t::slot_type& failure_cb)
{
    sSaveLocalImage = image;

    gViewerWindow->saveImageNumbered(sSaveLocalImage, false, success_cb, failure_cb);
}
