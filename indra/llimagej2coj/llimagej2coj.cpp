/**
 * @file llimagej2coj.cpp
 * @brief This is an implementation of JPEG2000 encode/decode using OpenJPEG.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "linden_common.h"
#include "llimagej2coj.h"

 // this is defined so that we get static linking.
#include "openjpeg.h"

#include "lltimer.h"
//#include "llmemory.h"

// Factory function: see declaration in llimagej2c.cpp
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl()
{
    return new LLImageJ2COJ();
}

std::string LLImageJ2COJ::getEngineInfo() const
{
#ifdef OPENJPEG_VERSION
    return std::string("OpenJPEG: " OPENJPEG_VERSION ", Runtime: ")
        + opj_version();
#else
    return std::string("OpenJPEG runtime: ") + opj_version();
#endif
}

// Return string from message, eliminating final \n if present
static std::string chomp(const char* msg)
{
    // stomp trailing \n
    std::string message = msg;
    if (!message.empty())
    {
        size_t last = message.size() - 1;
        if (message[last] == '\n')
        {
            message.resize(last);
        }
    }
    return message;
}

/**
sample error callback expecting a LLFILE* client object
*/
void error_callback(const char* msg, void*)
{
    LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}
/**
sample warning callback expecting a LLFILE* client object
*/
void warning_callback(const char* msg, void*)
{
    LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}
/**
sample debug callback expecting no client object
*/
void info_callback(const char* msg, void*)
{
    LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}

// Divide a by 2 to the power of b and round upwards
int ceildivpow2(int a, int b)
{
    return (a + (1 << b) - 1) >> b;
}


LLImageJ2COJ::LLImageJ2COJ()
    : LLImageJ2CImpl()
{
}


LLImageJ2COJ::~LLImageJ2COJ()
{
}

bool LLImageJ2COJ::initDecode(LLImageJ2C& base, LLImageRaw& raw_image, int discard_level, int* region)
{
    // No specific implementation for this method in the OpenJpeg case
    return false;
}

bool LLImageJ2COJ::initEncode(LLImageJ2C& base, LLImageRaw& raw_image, int blocks_size, int precincts_size, int levels)
{
    // No specific implementation for this method in the OpenJpeg case
    return false;
}

bool LLImageJ2COJ::decodeImpl(LLImageJ2C& base, LLImageRaw& raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count)
{
    //
    // FIXME: Get the comment field out of the texture
    //

    LLTimer decode_timer;

    opj_dparameters_t parameters;	/* decompression parameters */
    opj_event_mgr_t event_mgr = { };		/* event manager */
    opj_image_t* image = nullptr;

    opj_dinfo_t* dinfo = nullptr;	/* handle to a decompressor */
    opj_cio_t* cio = nullptr;


    /* configure the event callbacks (not required) */
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    /* set decoding parameters to default values */
    opj_set_default_decoder_parameters(&parameters);

    parameters.cp_reduce = base.getRawDiscardLevel();

    /* decode the code-stream */
    /* ---------------------- */

    /* JPEG-2000 codestream */

    /* get a decoder handle */
    dinfo = opj_create_decompress(CODEC_J2K);

    /* catch events using our callbacks and give a local context */
    opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, stderr);

    /* setup the decoder decoding parameters using user parameters */
    opj_setup_decoder(dinfo, &parameters);

    /* open a byte stream */
    cio = opj_cio_open((opj_common_ptr)dinfo, base.getData(), base.getDataSize());

    /* decode the stream and fill the image structure */
    image = opj_decode(dinfo, cio);

    /* close the byte stream */
    opj_cio_close(cio);

    /* free remaining structures */
    if (dinfo)
    {
        opj_destroy_decompress(dinfo);
    }

    // The image decode failed if the return was NULL or the component
    // count was zero.  The latter is just a sanity check before we
    // dereference the array.
    if (!image || !image->numcomps)
    {
        LL_DEBUGS("Texture") << "ERROR -> decodeImpl: failed to decode image!" << LL_ENDL;
        if (image)
        {
            opj_image_destroy(image);
        }
        base.decodeFailed();
        return true; // done
    }

    // sometimes we get bad data out of the cache - check to see if the decode succeeded
    for (S32 i = 0; i < image->numcomps; i++)
    {
        if (image->comps[i].factor != base.getRawDiscardLevel())
        {
            // if we didn't get the discard level we're expecting, fail
            opj_image_destroy(image);
            base.decodeFailed();
            return true;
        }
    }

    if (image->numcomps <= first_channel)
    {
        LL_WARNS() << "trying to decode more channels than are present in image: numcomps: " << image->numcomps << " first_channel: " << first_channel << LL_ENDL;
        opj_image_destroy(image);
        base.decodeFailed();
        return true;
    }

    // Copy image data into our raw image format (instead of the separate channel format

    S32 img_components = image->numcomps;
    S32 channels = img_components - first_channel;
    if (channels > max_channel_count)
        channels = max_channel_count;

    // Component buffers are allocated in an image width by height buffer.
    // The image placed in that buffer is ceil(width/2^factor) by
    // ceil(height/2^factor) and if the factor isn't zero it will be at the
    // top left of the buffer with black filled in the rest of the pixels.
    // It is integer math so the formula is written in ceildivpo2.
    // (Assuming all the components have the same width, height and
    // factor.)
    S32 comp_width = image->comps[0].w;
    S32 f = image->comps[0].factor;
    S32 width = ceildivpow2(image->x1 - image->x0, f);
    S32 height = ceildivpow2(image->y1 - image->y0, f);
    raw_image.resize(width, height, channels);
    U8* rawp = raw_image.getData();
    if (!rawp)
    {
        opj_image_destroy(image);
        base.setLastError("Memory error");
        base.decodeFailed();
        return true; // done
    }

    // first_channel is what channel to start copying from
    // dest is what channel to copy to.  first_channel comes from the
    // argument, dest always starts writing at channel zero.
    for (S32 comp = first_channel, dest = 0; comp < first_channel + channels;
        comp++, dest++)
    {
        if (image->comps[comp].data)
        {
            S32 offset = dest;
            for (S32 y = (height - 1); y >= 0; y--)
            {
                for (S32 x = 0; x < width; x++)
                {
                    rawp[offset] = image->comps[comp].data[y * comp_width + x];
                    offset += channels;
                }
            }
        }
        else // Some rare OpenJPEG versions have this bug.
        {
            LL_DEBUGS("Texture") << "ERROR -> decodeImpl: failed to decode image! (NULL comp data - OpenJPEG bug)" << LL_ENDL;
            opj_image_destroy(image);
            base.decodeFailed();
            return true; // done
        }
    }

    /* free image data structure */
    opj_image_destroy(image);

    return true; // done
}


bool LLImageJ2COJ::encodeImpl(LLImageJ2C& base, const LLImageRaw& raw_image, const char* comment_text, F32 encode_time, bool reversible)
{
    const S32 MAX_COMPS = 5;
    opj_cparameters_t parameters;	/* compression parameters */
    opj_event_mgr_t event_mgr = { };		/* event manager */


    /*
    configure the event callbacks (not required)
    setting of each callback is optional
    */
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);
    parameters.cod_format = 0;
    parameters.cp_disto_alloc = 1;

    if (reversible)
    {
        parameters.tcp_numlayers = 1;
        parameters.tcp_rates[0] = 0.0f;
    }
    else
    {
        parameters.tcp_numlayers = 5;
        parameters.tcp_rates[0] = 1920.0f;
        parameters.tcp_rates[1] = 480.0f;
        parameters.tcp_rates[2] = 120.0f;
        parameters.tcp_rates[3] = 30.0f;
        parameters.tcp_rates[4] = 10.0f;
        parameters.irreversible = 1;
        if (raw_image.getComponents() >= 3)
        {
            parameters.tcp_mct = 1;
        }
    }

    if (!comment_text)
    {
        parameters.cp_comment = (char*)"";
    }
    else
    {
        // Awful hacky cast, too lazy to copy right now.
        parameters.cp_comment = (char*)comment_text;
    }

    //
    // Fill in the source image from our raw image
    //
    OPJ_COLOR_SPACE color_space = CLRSPC_SRGB;
    opj_image_cmptparm_t cmptparm[MAX_COMPS];
    opj_image_t* image = nullptr;
    S32 numcomps = raw_image.getComponents();
    S32 width = raw_image.getWidth();
    S32 height = raw_image.getHeight();

    memset(&cmptparm[0], 0, MAX_COMPS * sizeof(opj_image_cmptparm_t));
    for (S32 c = 0; c < numcomps; c++) {
        cmptparm[c].prec = 8;
        cmptparm[c].bpp = 8;
        cmptparm[c].sgnd = 0;
        cmptparm[c].dx = parameters.subsampling_dx;
        cmptparm[c].dy = parameters.subsampling_dy;
        cmptparm[c].w = width;
        cmptparm[c].h = height;
    }

    /* create the image */
    image = opj_image_create(numcomps, &cmptparm[0], color_space);

    image->x1 = width;
    image->y1 = height;

    S32 i = 0;
    const U8* src_datap = raw_image.getData();
    for (S32 y = height - 1; y >= 0; y--)
    {
        for (S32 x = 0; x < width; x++)
        {
            const U8* pixel = src_datap + (y * width + x) * numcomps;
            for (S32 c = 0; c < numcomps; c++)
            {
                image->comps[c].data[i] = *pixel;
                pixel++;
            }
            i++;
        }
    }



    /* encode the destination image */
    /* ---------------------------- */

    int codestream_length;
    opj_cio_t* cio = nullptr;

    /* get a J2K compressor handle */
    opj_cinfo_t* cinfo = opj_create_compress(CODEC_J2K);

    /* catch events using our callbacks and give a local context */
    opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, stderr);

    /* setup the encoder parameters using the current image and using user parameters */
    opj_setup_encoder(cinfo, &parameters, image);

    /* open a byte stream for writing */
    /* allocate memory for all tiles */
    cio = opj_cio_open((opj_common_ptr)cinfo, nullptr, 0);

    /* encode the image */
    bool bSuccess = opj_encode(cinfo, cio, image, nullptr);
    if (!bSuccess)
    {
        opj_cio_close(cio);
        // _LL_DEBUGS("Texture") << "Failed to encode image." << LL_ENDL;
        return false;
    }
    codestream_length = cio_tell(cio);

    base.copyData(cio->buffer, codestream_length);
    base.updateData(); // set width, height

    /* close and free the byte stream */
    opj_cio_close(cio);

    /* free remaining compression structures */
    opj_destroy_compress(cinfo);


    /* free user parameters structure */
    if (parameters.cp_matrice) free(parameters.cp_matrice);

    /* free image data */
    opj_image_destroy(image);
    return true;
}

inline S32 extractLong4(U8 const* aBuffer, int nOffset)
{
    S32 ret = aBuffer[nOffset] << 24;
    ret += aBuffer[nOffset + 1] << 16;
    ret += aBuffer[nOffset + 2] << 8;
    ret += aBuffer[nOffset + 3];
    return ret;
}

inline S32 extractShort2(U8 const* aBuffer, int nOffset)
{
    S32 ret = aBuffer[nOffset] << 8;
    ret += aBuffer[nOffset + 1];

    return ret;
}

inline bool isSOC(U8 const* aBuffer)
{
    return aBuffer[0] == 0xFF && aBuffer[1] == 0x4F;
}

inline bool isSIZ(U8 const* aBuffer)
{
    return aBuffer[0] == 0xFF && aBuffer[1] == 0x51;
}

bool getMetadataFast(LLImageJ2C& aImage, S32& aW, S32& aH, S32& aComps)
{
    const int J2K_HDR_LEN(42);
    const int J2K_HDR_X1(8);
    const int J2K_HDR_Y1(12);
    const int J2K_HDR_X0(16);
    const int J2K_HDR_Y0(20);
    const int J2K_HDR_NUMCOMPS(40);

    if (aImage.getDataSize() < J2K_HDR_LEN)
        return false;

    U8 const* pBuffer = aImage.getData();

    if (!isSOC(pBuffer) || !isSIZ(pBuffer + 2))
        return false;

    S32 x1 = extractLong4(pBuffer, J2K_HDR_X1);
    S32 y1 = extractLong4(pBuffer, J2K_HDR_Y1);
    S32 x0 = extractLong4(pBuffer, J2K_HDR_X0);
    S32 y0 = extractLong4(pBuffer, J2K_HDR_Y0);
    S32 numComps = extractShort2(pBuffer, J2K_HDR_NUMCOMPS);

    aComps = numComps;
    aW = x1 - x0;
    aH = y1 - y0;

    return true;
}

bool LLImageJ2COJ::getMetadata(LLImageJ2C& base)
{
    //
    // FIXME: We get metadata by decoding the ENTIRE image.
    //

    // Update the raw discard level
    base.updateRawDiscardLevel();

    S32 width(0);
    S32 height(0);
    S32 img_components(0);

    if (getMetadataFast(base, width, height, img_components))
    {
        base.setSize(width, height, img_components);
        return true;
    }

    // Do it the old and slow way, decode the image with openjpeg

    opj_dparameters_t parameters;	/* decompression parameters */
    opj_event_mgr_t event_mgr = { };		/* event manager */
    opj_image_t* image = nullptr;

    opj_dinfo_t* dinfo = nullptr;	/* handle to a decompressor */
    opj_cio_t* cio = nullptr;


    /* configure the event callbacks (not required) */
    event_mgr.error_handler = error_callback;
    event_mgr.warning_handler = warning_callback;
    event_mgr.info_handler = info_callback;

    /* set decoding parameters to default values */
    opj_set_default_decoder_parameters(&parameters);

    // Only decode what's required to get the size data.
    parameters.cp_limit_decoding = LIMIT_TO_MAIN_HEADER;

    //parameters.cp_reduce = mRawDiscardLevel;

    /* decode the code-stream */
    /* ---------------------- */

    /* JPEG-2000 codestream */

    /* get a decoder handle */
    dinfo = opj_create_decompress(CODEC_J2K);

    /* catch events using our callbacks and give a local context */
    opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, stderr);

    /* setup the decoder decoding parameters using user parameters */
    opj_setup_decoder(dinfo, &parameters);

    /* open a byte stream */
    cio = opj_cio_open((opj_common_ptr)dinfo, base.getData(), base.getDataSize());

    /* decode the stream and fill the image structure */
    image = opj_decode(dinfo, cio);

    /* close the byte stream */
    opj_cio_close(cio);

    /* free remaining structures */
    if (dinfo)
    {
        opj_destroy_decompress(dinfo);
    }

    if (!image)
    {
        LL_WARNS() << "ERROR -> getMetadata: failed to decode image!" << LL_ENDL;
        return false;
    }

    // Copy image data into our raw image format (instead of the separate channel format

    img_components = image->numcomps;
    width = image->x1 - image->x0;
    height = image->y1 - image->y0;
    base.setSize(width, height, img_components);

    /* free image data structure */
    opj_image_destroy(image);
    return true;
}
