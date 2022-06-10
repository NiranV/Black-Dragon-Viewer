/** 
 * @file streamingaudio_fmodstudio.cpp
 * @brief LLStreamingAudio_FMODSTUDIO implementation
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

#include "linden_common.h"
#include "llstreamingaudio_fmodstudio.h"

#include "llsd.h"
#include "llmath.h"
#include "llmutex.h"

#include "fmod.hpp"
#include "fmod_errors.h"

inline bool Check_FMOD_Stream_Error(FMOD_RESULT result, const char *string)
{
	if (result == FMOD_OK)
		return false;
	LL_WARNS("AudioImpl") << string << " Error: " << FMOD_ErrorString(result) << LL_ENDL;
	return true;
}

class LLAudioStreamManagerFMODSTUDIO
{
public:
	LLAudioStreamManagerFMODSTUDIO(FMOD::System *system, FMOD::ChannelGroup *group, const std::string& url);
	FMOD::Channel* startStream();
	bool stopStream(); // Returns true if the stream was successfully stopped.

	const std::string& getURL() 	{ return mInternetStreamURL; }

	FMOD_RESULT getOpenState(FMOD_OPENSTATE& openstate, unsigned int* percentbuffered = nullptr, bool* starving = nullptr, bool* diskbusy = nullptr);
protected:
	FMOD::System* mSystem;
	FMOD::ChannelGroup* mChannelGroup;
	FMOD::Channel* mStreamChannel;
	FMOD::Sound* mInternetStream;
	bool mReady;

	std::string mInternetStreamURL;
};

LLMutex gWaveDataMutex;	//Just to be extra strict.
const U32 WAVE_BUFFER_SIZE = 1024;
U32 gWaveBufferMinSize = 0;
F32 gWaveDataBuffer[WAVE_BUFFER_SIZE] = { 0.f };
U32 gWaveDataBufferSize = 0;

FMOD_RESULT F_CALLBACK waveDataCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
	if (!length || !inchannels)
		return FMOD_OK;
	memcpy(outbuffer, inbuffer, length * inchannels * sizeof(float));

	static std::vector<F32> local_buf;
	if (local_buf.size() < length)
		local_buf.resize(length, 0.f);

	for (U32 i = 0; i < length; ++i)
	{
		F32 total = 0.f;
		for (S32 j = 0; j < inchannels; ++j)
		{
			total += inbuffer[i*inchannels + j];
		}
		local_buf[i] = total / inchannels;
	}

	{
		LLMutexLock lock(&gWaveDataMutex);

		for (U32 i = length; i > 0; --i)
		{
			if (++gWaveDataBufferSize > WAVE_BUFFER_SIZE)
			{
				if (gWaveBufferMinSize)
					memcpy(gWaveDataBuffer + WAVE_BUFFER_SIZE - gWaveBufferMinSize, gWaveDataBuffer, gWaveBufferMinSize * sizeof(float));
				gWaveDataBufferSize = 1 + gWaveBufferMinSize;
			}
			gWaveDataBuffer[WAVE_BUFFER_SIZE - gWaveDataBufferSize] = local_buf[i - 1];
		}
	}
	
	return FMOD_OK;
}

//---------------------------------------------------------------------------
// Internet Streaming
//---------------------------------------------------------------------------
LLStreamingAudio_FMODSTUDIO::LLStreamingAudio_FMODSTUDIO(FMOD::System *system) :
mSystem(system),
mCurrentInternetStreamp(NULL),
mFMODInternetStreamChannelp(NULL),
mGain(1.0f),
mRetryCount(0)
{
	FMOD_RESULT result;

	// Number of milliseconds of audio to buffer for the audio card.
	// Must be larger than the usual Second Life frame stutter time.
	const U32 buffer_seconds = 10;		//sec
	const U32 estimated_bitrate = 128;	//kbit/sec
	result = mSystem->setStreamBufferSize(estimated_bitrate * buffer_seconds * 128/*bytes/kbit*/, FMOD_TIMEUNIT_RAWBYTES);
	Check_FMOD_Stream_Error(result, "FMOD::System::setStreamBufferSize");

	Check_FMOD_Stream_Error(system->createChannelGroup("stream", &mStreamGroup), "FMOD::System::createChannelGroup");

	FMOD_DSP_DESCRIPTION dspdesc = { };
	dspdesc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
	strncpy(dspdesc.name, "Waveform", sizeof(dspdesc.name));
	dspdesc.numoutputbuffers = 1;
	dspdesc.read = &waveDataCallback; //Assign callback.

	Check_FMOD_Stream_Error(system->createDSP(&dspdesc, &mStreamDSP), "FMOD::System::createDSP");
}

LLStreamingAudio_FMODSTUDIO::~LLStreamingAudio_FMODSTUDIO()
{
    if (mCurrentInternetStreamp)
    {
        // Isn't supposed to hapen, stream should be clear by now,
        // and if it does, we are likely going to crash.
        LL_WARNS("FMOD") << "mCurrentInternetStreamp not null on shutdown!" << LL_ENDL;
        stop();
    }

    // Kill dead internet streams, if possible
    killDeadStreams();

    if (!mDeadStreams.empty())
    {
        // LLStreamingAudio_FMODSTUDIO was inited on startup
        // and should be destroyed on shutdown, it should
        // wait for streams to die to not cause crashes or
        // leaks.
        // Ideally we need to wait on some kind of callback
        // to release() streams correctly, but 200 ms should
        // be enough and we can't wait forever.
        LL_INFOS("FMOD") << "Waiting for " << (S32)mDeadStreams.size() << " streams to stop" << LL_ENDL;
        for (S32 i = 0; i < 20; i++)
        {
            const U32 ms_delay = 10;
            ms_sleep(ms_delay); // rude, but not many options here
            killDeadStreams();
            if (mDeadStreams.empty())
            {
                LL_INFOS("FMOD") << "All streams stopped after " << (S32)((i + 1) * ms_delay) << "ms" << LL_ENDL;
                break;
            }
        }
    }

    if (!mDeadStreams.empty())
    {
        LL_WARNS("FMOD") << "Failed to kill some audio streams" << LL_ENDL;
    }
}

void LLStreamingAudio_FMODSTUDIO::killDeadStreams()
{
    std::list<LLAudioStreamManagerFMODSTUDIO *>::iterator iter;
    for (iter = mDeadStreams.begin(); iter != mDeadStreams.end();)
    {
        LLAudioStreamManagerFMODSTUDIO *streamp = *iter;
        if (streamp->stopStream())
        {
            LL_INFOS("FMOD") << "Closed dead stream" << LL_ENDL;
            delete streamp;
            mDeadStreams.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
}

void LLStreamingAudio_FMODSTUDIO::start(const std::string& url)
{
    //if (!mInited)
    //{
    //	LL_WARNS() << "startInternetStream before audio initialized" << LL_ENDL;
    //	return;
    //}

    // "stop" stream but don't clear url, etc. in case url == mInternetStreamURL
    stop();

    if (!url.empty())
    {
        LL_INFOS("FMOD") << "Starting internet stream: " << url << LL_ENDL;
        mCurrentInternetStreamp = new LLAudioStreamManagerFMODSTUDIO(mSystem, url);
        mURL = url;
    }
    else
    {
        LL_INFOS("FMOD") << "Set internet stream to null" << LL_ENDL;
        mURL.clear();
    }

    mRetryCount = 0;
}

enum utf_endian_type_t
{
	UTF16LE,
	UTF16BE,
	UTF16
};

std::string utf16input_to_utf8(unsigned char* input, U32 len, utf_endian_type_t type)
{
	if (type == UTF16)
	{
		type = UTF16BE;	//Default
		if (len > 2)
		{
			//Parse and strip BOM.
			if ((input[0] == 0xFE && input[1] == 0xFF) || 
				(input[0] == 0xFF && input[1] == 0xFE))
			{
				input += 2;
				len -= 2;
				type = input[0] == 0xFE ? UTF16BE : UTF16LE;
			}
		}
	}
	llutf16string out_16((llutf16string::value_type*)input, len / 2);
	if (len % 2)
	{
		out_16.push_back((input)[len - 1] << 8);
	}
	if (type == UTF16BE)
	{
		for (llutf16string::iterator i = out_16.begin(); i < out_16.end(); ++i)
		{
			llutf16string::value_type v = *i;
			*i = ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
		}
	}
	return utf16str_to_utf8str(out_16);
}

void LLStreamingAudio_FMODSTUDIO::update()
{
    // Kill dead internet streams, if possible
    killDeadStreams();

    // Don't do anything if there are no streams playing
    if (!mCurrentInternetStreamp)
    {
        return;
    }

    unsigned int progress;
    bool starving;
    bool diskbusy;
    FMOD_OPENSTATE open_state = mCurrentInternetStreamp->getOpenState(&progress, &starving, &diskbusy);

    if (open_state == FMOD_OPENSTATE_READY)
    {
        // Stream is live

        // start the stream if it's ready
        if (!mFMODInternetStreamChannelp &&
            (mFMODInternetStreamChannelp = mCurrentInternetStreamp->startStream()))
        {
            // Reset volume to previously set volume
            setGain(getGain());
            mFMODInternetStreamChannelp->setPaused(false);
        }
        mRetryCount = 0;
    }
    else if (open_state == FMOD_OPENSTATE_ERROR)
    {
        LL_INFOS("FMOD") << "State: FMOD_OPENSTATE_ERROR"
            << " Progress: " << U32(progress)
            << " Starving: " << S32(starving)
            << " Diskbusy: " << S32(diskbusy) << LL_ENDL;
        if (mRetryCount < 2)
        {
            // Retry
            std::string url = mURL;
            stop(); // might drop mURL, drops mCurrentInternetStreamp

            mRetryCount++;

            if (!url.empty())
            {
                LL_INFOS("FMOD") << "Restarting internet stream: " << url  << ", attempt " << (mRetryCount + 1) << LL_ENDL;
                mCurrentInternetStreamp = new LLAudioStreamManagerFMODSTUDIO(mSystem, url);
                mURL = url;
            }
        }
        else
        {
            stop();
        }
        return;
    }

    if (mFMODInternetStreamChannelp)
    {
        FMOD::Sound *sound = NULL;

        if (mFMODInternetStreamChannelp->getCurrentSound(&sound) == FMOD_OK && sound)
        {
            FMOD_TAG tag;
            S32 tagcount, dirtytagcount;

            if (sound->getNumTags(&tagcount, &dirtytagcount) == FMOD_OK && dirtytagcount)
            {
                for (S32 i = 0; i < tagcount; ++i)
                {
                    if (sound->getTag(NULL, i, &tag) != FMOD_OK)
                        continue;

                    if (tag.type == FMOD_TAGTYPE_FMOD)
                    {
                        if (!strcmp(tag.name, "Sample Rate Change"))
                        {
                            LL_INFOS("FMOD") << "Stream forced changing sample rate to " << *((float *)tag.data) << LL_ENDL;
                            mFMODInternetStreamChannelp->setFrequency(*((float *)tag.data));
                        }
                        continue;
                    }
                }
            }

            if (starving)
            {
                bool paused = false;
                mFMODInternetStreamChannelp->getPaused(&paused);
                if (!paused)
                {
                    LL_INFOS("FMOD") << "Stream starvation detected! Pausing stream until buffer nearly full." << LL_ENDL;
                    LL_INFOS("FMOD") << "  (diskbusy=" << diskbusy << ")" << LL_ENDL;
                    LL_INFOS("FMOD") << "  (progress=" << progress << ")" << LL_ENDL;
                    mFMODInternetStreamChannelp->setPaused(true);
                }
            }
            else if (progress > 80)
            {
                mFMODInternetStreamChannelp->setPaused(false);
            }
        }
    }
}

void LLStreamingAudio_FMODSTUDIO::stop()
{
    if (mFMODInternetStreamChannelp)
    {
        mFMODInternetStreamChannelp->setPaused(true);
        mFMODInternetStreamChannelp->setPriority(0);
        mFMODInternetStreamChannelp = NULL;
    }

    if (mCurrentInternetStreamp)
    {
        LL_INFOS("FMOD") << "Stopping internet stream: " << mCurrentInternetStreamp->getURL() << LL_ENDL;
        if (mCurrentInternetStreamp->stopStream())
        {
            delete mCurrentInternetStreamp;
        }
        else
        {
            LL_WARNS("FMOD") << "Pushing stream to dead list: " << mCurrentInternetStreamp->getURL() << LL_ENDL;
            mDeadStreams.push_back(mCurrentInternetStreamp);
        }
        mCurrentInternetStreamp = NULL;
        //mURL.clear();
    }
}

void LLStreamingAudio_FMODSTUDIO::pause(S32 pauseopt)
{
    if (pauseopt < 0)
    {
        pauseopt = mCurrentInternetStreamp ? 1 : 0;
    }

    if (pauseopt)
    {
        if (mCurrentInternetStreamp)
        {
            LL_INFOS("FMOD") << "Pausing internet stream" << LL_ENDL;
            stop();
        }
    }
    else
    {
        start(getURL());
    }
}


// A stream is "playing" if it has been requested to start.  That
// doesn't necessarily mean audio is coming out of the speakers.
int LLStreamingAudio_FMODSTUDIO::isPlaying()
{
	if (mCurrentInternetStreamp)
	{
		return 1; // Active and playing
	}
	else if (!mURL.empty() || !mPendingURL.empty())
	{
		return 2; // "Paused"
	}
	else
	{
		return 0;
	}
}


F32 LLStreamingAudio_FMODSTUDIO::getGain()
{
	return mGain;
}


std::string LLStreamingAudio_FMODSTUDIO::getURL()
{
	return mURL;
}


void LLStreamingAudio_FMODSTUDIO::setGain(F32 vol)
{
	mGain = vol;

	if (mFMODInternetStreamChannelp)
	{
		vol = llclamp(vol * vol, 0.f, 1.f);	//should vol be squared here?

		Check_FMOD_Stream_Error(mFMODInternetStreamChannelp->setVolume(vol), "FMOD::Channel::setVolume");
	}
}

bool LLStreamingAudio_FMODSTUDIO::hasNewMetaData()
{
	if (mCurrentInternetStreamp && mNewMetadata)
	{
		mNewMetadata = false;
		return true;
	}
	else
	{
		return false;
	}
}

/* virtual */
bool LLStreamingAudio_FMODSTUDIO::getWaveData(float* arr, S32 count, S32 stride/*=1*/)
{
	if (count > (WAVE_BUFFER_SIZE / 2))
		LL_ERRS("AudioImpl") << "Count=" << count << " exceeds WAVE_BUFFER_SIZE/2=" << WAVE_BUFFER_SIZE << LL_ENDL;

	if(!mFMODInternetStreamChannelp || !mCurrentInternetStreamp)
		return false;

	bool muted = false;
	FMOD_RESULT res = mFMODInternetStreamChannelp->getMute(&muted);
	if(res != FMOD_OK || muted)
		return false;
	{
		U32 buff_size;
		{
			LLMutexLock lock(&gWaveDataMutex);
			gWaveBufferMinSize = count;
			buff_size = gWaveDataBufferSize;
			if (!buff_size)
				return false;
			memcpy(arr, gWaveDataBuffer + WAVE_BUFFER_SIZE - buff_size, llmin(U32(count), buff_size) * sizeof(float));
		}
		if (buff_size < U32(count))
			memset(arr + buff_size, 0, (count - buff_size) * sizeof(float));
	}
	return true;
}

LLAudioStreamManagerFMODSTUDIO::LLAudioStreamManagerFMODSTUDIO(FMOD::System *system, FMOD::ChannelGroup *group, const std::string& url) :
	mSystem(system),
	mChannelGroup(group),
	mStreamChannel(nullptr),
	mInternetStream(nullptr),
	mReady(false),
	mInternetStreamURL(url)
{
	FMOD_RESULT result = mSystem->createStream(url.c_str(), FMOD_2D | FMOD_NONBLOCKING | FMOD_IGNORETAGS, nullptr, &mInternetStream);

	if (result != FMOD_OK)
	{
		LL_WARNS("FMOD") << "Couldn't open fmod stream, error "
			<< FMOD_ErrorString(result)
			<< LL_ENDL;
		mReady = false;
		return;
	}

	mReady = true;
}

FMOD::Channel *LLAudioStreamManagerFMODSTUDIO::startStream()
{
    // We need a live and opened stream before we try and play it.
    if (!mInternetStream || getOpenState() != FMOD_OPENSTATE_READY)
    {
        LL_WARNS("FMOD") << "No internet stream to start playing!" << LL_ENDL;
        return NULL;
    }

    if (mStreamChannel)
        return mStreamChannel;	//Already have a channel for this stream.

    mSystem->playSound(mInternetStream, NULL, true, &mStreamChannel);
    return mStreamChannel;
}

bool LLAudioStreamManagerFMODSTUDIO::stopStream()
{
	if (mInternetStream)
	{
		bool close = true;
		FMOD_OPENSTATE open_state;
		if (getOpenState(open_state) == FMOD_OK)
		{
			switch (open_state)
			{
			case FMOD_OPENSTATE_CONNECTING:
				close = false;
				break;
			default:
				close = true;
			}
		}

		if (close && mInternetStream->release() == FMOD_OK)
		{
			mStreamChannel = nullptr;
			mInternetStream = nullptr;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

FMOD_RESULT LLAudioStreamManagerFMODSTUDIO::getOpenState(FMOD_OPENSTATE& state, unsigned int* percentbuffered, bool* starving, bool* diskbusy)
{
	if (!mInternetStream)
		return FMOD_ERR_INVALID_HANDLE;
	FMOD_RESULT result = mInternetStream->getOpenState(&state, percentbuffered, starving, diskbusy);
	Check_FMOD_Stream_Error(result, "FMOD::Sound::getOpenState");
	return result;
}

void LLStreamingAudio_FMODSTUDIO::setBufferSizes(U32 streambuffertime, U32 decodebuffertime)
{
	Check_FMOD_Stream_Error(mSystem->setStreamBufferSize(streambuffertime / 1000 * 128 * 128, FMOD_TIMEUNIT_RAWBYTES), "FMOD::System::setStreamBufferSize");
	FMOD_ADVANCEDSETTINGS settings = { };
	settings.cbSize=sizeof(settings);
	settings.defaultDecodeBufferSize = decodebuffertime;//ms
	Check_FMOD_Stream_Error(mSystem->setAdvancedSettings(&settings), "FMOD::System::setAdvancedSettings");
}

bool LLStreamingAudio_FMODSTUDIO::releaseDeadStreams()
{
	// Kill dead internet streams, if possible
	for (auto iter = mDeadStreams.begin(); iter != mDeadStreams.end();)
	{
		LLAudioStreamManagerFMODSTUDIO *streamp = *iter;
		if (streamp->stopStream())
		{
			LL_INFOS() << "Closed dead stream" << LL_ENDL;
			delete streamp;
			mDeadStreams.erase(iter++);
		}
		else
		{
			++iter;
		}
	}

	return mDeadStreams.empty();
}

void LLStreamingAudio_FMODSTUDIO::cleanupWaveData()
{
	if (mStreamGroup)
	{
		Check_FMOD_Stream_Error(mStreamGroup->release(), "FMOD::ChannelGroup::release");
		mStreamGroup = nullptr;
	}
	
	if(mStreamDSP)
		Check_FMOD_Stream_Error(mStreamDSP->release(), "FMOD::DSP::release");
	mStreamDSP = nullptr;
}
