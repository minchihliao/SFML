////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/AudioDevice.hpp>
#include <SFML/Audio/SoundStream.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/Sleep.hpp>

#include <miniaudio.h>

#include <algorithm>
#include <limits>
#include <ostream>
#include <vector>

#include <cassert>
#include <cstring>


namespace sf
{
struct SoundStream::Impl
{
    Impl(SoundStream* ownerPtr) : owner(ownerPtr)
    {
        // Set this object up as a miniaudio data source
        ma_data_source_config config = ma_data_source_config_init();

        static constexpr ma_data_source_vtable vtable{read, seek, getFormat, getCursor, getLength, setLooping, 0};

        config.vtable = &vtable;

        if (auto result = ma_data_source_init(&config, &dataSourceBase); result != MA_SUCCESS)
            err() << "Failed to initialize audio data source: " << ma_result_description(result) << std::endl;

        // Initialize sound structure and set default settings
        initialize();

        ma_sound_set_pitch(&sound, 1.f);
        ma_sound_set_pan(&sound, 0.f);
        ma_sound_set_volume(&sound, 1.f);
        ma_sound_set_spatialization_enabled(&sound, MA_TRUE);
        ma_sound_set_position(&sound, 0.f, 0.f, 0.f);
        ma_sound_set_direction(&sound, 0.f, 0.f, -1.f);
        ma_sound_set_cone(&sound, sf::degrees(360).asRadians(), sf::degrees(360).asRadians(), 0.f); // inner = 360 degrees, outer = 360 degrees, gain = 0
        ma_sound_set_directional_attenuation_factor(&sound, 1.f);
        ma_sound_set_velocity(&sound, 0.f, 0.f, 0.f);
        ma_sound_set_doppler_factor(&sound, 1.f);
        ma_sound_set_positioning(&sound, ma_positioning_absolute);
        ma_sound_set_min_distance(&sound, 1.f);
        ma_sound_set_max_distance(&sound, std::numeric_limits<float>::max());
        ma_sound_set_min_gain(&sound, 0.f);
        ma_sound_set_max_gain(&sound, 1.f);
        ma_sound_set_rolloff(&sound, 1.f);
    }

    ~Impl()
    {
        ma_sound_uninit(&sound);

        ma_data_source_uninit(&dataSourceBase);
    }

    void initialize()
    {
        // Initialize the sound
        auto* engine = static_cast<ma_engine*>(priv::AudioDevice::getEngine());

        assert(engine != nullptr);

        ma_sound_config soundConfig;

        soundConfig                      = ma_sound_config_init();
        soundConfig.pDataSource          = this;
        soundConfig.pEndCallbackUserData = this;
        soundConfig.endCallback          = [](void* userData, ma_sound* soundPtr)
        {
            // Seek back to the start of the sound when it finishes playing
            auto& impl     = *static_cast<Impl*>(userData);
            impl.streaming = true;

            if (auto result = ma_sound_seek_to_pcm_frame(soundPtr, 0); result != MA_SUCCESS)
                err() << "Failed to seek sound to frame 0: " << ma_result_description(result) << std::endl;
        };

        if (auto result = ma_sound_init_ex(engine, &soundConfig, &sound); result != MA_SUCCESS)
            err() << "Failed to initialize sound: " << ma_result_description(result) << std::endl;

        // Because we are providing a custom data source, we have to provide the channel map ourselves
        if (!channelMap.empty())
        {
            soundChannelMap.clear();

            for (auto channel : channelMap)
            {
                switch (channel)
                {
                    case SoundChannel::Unspecified:
                        soundChannelMap.push_back(MA_CHANNEL_NONE);
                        break;
                    case SoundChannel::Mono:
                        soundChannelMap.push_back(MA_CHANNEL_MONO);
                        break;
                    case SoundChannel::FrontLeft:
                        soundChannelMap.push_back(MA_CHANNEL_FRONT_LEFT);
                        break;
                    case SoundChannel::FrontRight:
                        soundChannelMap.push_back(MA_CHANNEL_FRONT_RIGHT);
                        break;
                    case SoundChannel::FrontCenter:
                        soundChannelMap.push_back(MA_CHANNEL_FRONT_CENTER);
                        break;
                    case SoundChannel::FrontLeftOfCenter:
                        soundChannelMap.push_back(MA_CHANNEL_FRONT_LEFT_CENTER);
                        break;
                    case SoundChannel::FrontRightOfCenter:
                        soundChannelMap.push_back(MA_CHANNEL_FRONT_RIGHT_CENTER);
                        break;
                    case SoundChannel::LowFrequencyEffects:
                        soundChannelMap.push_back(MA_CHANNEL_LFE);
                        break;
                    case SoundChannel::BackLeft:
                        soundChannelMap.push_back(MA_CHANNEL_BACK_LEFT);
                        break;
                    case SoundChannel::BackRight:
                        soundChannelMap.push_back(MA_CHANNEL_BACK_RIGHT);
                        break;
                    case SoundChannel::BackCenter:
                        soundChannelMap.push_back(MA_CHANNEL_BACK_CENTER);
                        break;
                    case SoundChannel::SideLeft:
                        soundChannelMap.push_back(MA_CHANNEL_SIDE_LEFT);
                        break;
                    case SoundChannel::SideRight:
                        soundChannelMap.push_back(MA_CHANNEL_SIDE_RIGHT);
                        break;
                    case SoundChannel::TopCenter:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_CENTER);
                        break;
                    case SoundChannel::TopFrontLeft:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_FRONT_LEFT);
                        break;
                    case SoundChannel::TopFrontRight:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_FRONT_RIGHT);
                        break;
                    case SoundChannel::TopFrontCenter:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_FRONT_CENTER);
                        break;
                    case SoundChannel::TopBackLeft:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_BACK_LEFT);
                        break;
                    case SoundChannel::TopBackRight:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_BACK_RIGHT);
                        break;
                    case SoundChannel::TopBackCenter:
                        soundChannelMap.push_back(MA_CHANNEL_TOP_BACK_CENTER);
                        break;
                    default:
                        break;
                }
            }

            sound.engineNode.spatializer.pChannelMapIn = soundChannelMap.data();
        }
        else
        {
            sound.engineNode.spatializer.pChannelMapIn = nullptr;
        }
    }

    void reinitialize()
    {
        // Save and re-apply settings
        auto pitch                        = ma_sound_get_pitch(&sound);
        auto pan                          = ma_sound_get_pan(&sound);
        auto volume                       = ma_sound_get_volume(&sound);
        auto spatializationEnabled        = ma_sound_is_spatialization_enabled(&sound);
        auto position                     = ma_sound_get_position(&sound);
        auto direction                    = ma_sound_get_direction(&sound);
        auto directionalAttenuationFactor = ma_sound_get_directional_attenuation_factor(&sound);
        auto velocity                     = ma_sound_get_velocity(&sound);
        auto dopplerFactor                = ma_sound_get_doppler_factor(&sound);
        auto positioning                  = ma_sound_get_positioning(&sound);
        auto minDistance                  = ma_sound_get_min_distance(&sound);
        auto maxDistance                  = ma_sound_get_max_distance(&sound);
        auto minGain                      = ma_sound_get_min_gain(&sound);
        auto maxGain                      = ma_sound_get_max_gain(&sound);
        auto rollOff                      = ma_sound_get_rolloff(&sound);

        float innerAngle;
        float outerAngle;
        float outerGain;
        ma_sound_get_cone(&sound, &innerAngle, &outerAngle, &outerGain);

        ma_sound_uninit(&sound);

        initialize();

        ma_sound_set_pitch(&sound, pitch);
        ma_sound_set_pan(&sound, pan);
        ma_sound_set_volume(&sound, volume);
        ma_sound_set_spatialization_enabled(&sound, spatializationEnabled);
        ma_sound_set_position(&sound, position.x, position.y, position.z);
        ma_sound_set_direction(&sound, direction.x, direction.y, direction.z);
        ma_sound_set_directional_attenuation_factor(&sound, directionalAttenuationFactor);
        ma_sound_set_velocity(&sound, velocity.x, velocity.y, velocity.z);
        ma_sound_set_doppler_factor(&sound, dopplerFactor);
        ma_sound_set_positioning(&sound, positioning);
        ma_sound_set_min_distance(&sound, minDistance);
        ma_sound_set_max_distance(&sound, maxDistance);
        ma_sound_set_min_gain(&sound, minGain);
        ma_sound_set_max_gain(&sound, maxGain);
        ma_sound_set_rolloff(&sound, rollOff);

        ma_sound_set_cone(&sound, innerAngle, outerAngle, outerGain);
    }

    static ma_result read(ma_data_source* dataSource, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead)
    {
        auto& impl  = *static_cast<Impl*>(dataSource);
        auto* owner = impl.owner;

        // Try to fill our buffer with new samples if the source is still willing to stream data
        if (impl.sampleBuffer.empty() && impl.streaming)
        {
            Chunk chunk;

            impl.streaming = owner->onGetData(chunk);

            if (chunk.samples && chunk.sampleCount)
            {
                impl.sampleBuffer.assign(chunk.samples, chunk.samples + chunk.sampleCount);
                impl.sampleBufferCursor = 0;
            }
        }

        // Push the samples to miniaudio
        if (!impl.sampleBuffer.empty())
        {
            // Determine how many frames we can read
            *framesRead = std::min<ma_uint64>(frameCount,
                                              (impl.sampleBuffer.size() - impl.sampleBufferCursor) / impl.channelCount);

            const auto sampleCount = *framesRead * impl.channelCount;

            // Copy the samples to the output
            std::memcpy(framesOut,
                        impl.sampleBuffer.data() + impl.sampleBufferCursor,
                        static_cast<std::size_t>(sampleCount) * sizeof(impl.sampleBuffer[0]));

            impl.sampleBufferCursor += static_cast<std::size_t>(sampleCount);
            impl.samplesProcessed += sampleCount;

            if (impl.sampleBufferCursor >= impl.sampleBuffer.size())
            {
                impl.sampleBuffer.clear();
                impl.sampleBufferCursor = 0;

                // If we are looping and at the end of the loop, set the cursor back to the beginning of the loop
                if (!impl.streaming && impl.loop)
                {
                    if (auto seekPositionAfterLoop = owner->onLoop(); seekPositionAfterLoop != NoLoop)
                    {
                        impl.streaming        = true;
                        impl.samplesProcessed = static_cast<std::uint64_t>(seekPositionAfterLoop);
                    }
                }
            }
        }
        else
        {
            *framesRead = 0;
        }

        return MA_SUCCESS;
    }

    static ma_result seek(ma_data_source* dataSource, ma_uint64 frameIndex)
    {
        auto& impl  = *static_cast<Impl*>(dataSource);
        auto* owner = impl.owner;

        impl.streaming = true;
        impl.sampleBuffer.clear();
        impl.sampleBufferCursor = 0;
        impl.samplesProcessed   = frameIndex * impl.channelCount;

        if (impl.sampleRate != 0)
        {
            owner->onSeek(seconds(static_cast<float>(frameIndex / impl.sampleRate)));
        }
        else
        {
            owner->onSeek(Time::Zero);
        }

        return MA_SUCCESS;
    }

    static ma_result getFormat(ma_data_source* dataSource,
                               ma_format*      format,
                               ma_uint32*      channels,
                               ma_uint32*      sampleRate,
                               ma_channel*,
                               size_t)
    {
        const auto& impl = *static_cast<const Impl*>(dataSource);

        // If we don't have valid values yet, initialize with defaults so sound creation doesn't fail
        *format     = ma_format_s16;
        *channels   = impl.channelCount ? impl.channelCount : 1;
        *sampleRate = impl.sampleRate ? impl.sampleRate : 44100;

        return MA_SUCCESS;
    }

    static ma_result getCursor(ma_data_source* dataSource, ma_uint64* cursor)
    {
        auto& impl = *static_cast<Impl*>(dataSource);
        *cursor    = impl.channelCount ? impl.samplesProcessed / impl.channelCount : 0;

        return MA_SUCCESS;
    }

    static ma_result getLength(ma_data_source*, ma_uint64* length)
    {
        *length = 0;

        return MA_NOT_IMPLEMENTED;
    }

    static ma_result setLooping(ma_data_source* dataSource, ma_bool32 looping)
    {
        static_cast<Impl*>(dataSource)->loop = (looping == MA_TRUE);

        return MA_SUCCESS;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    ma_data_source_base dataSourceBase{}; //!< The struct that makes this object a miniaudio data source (must be first member)
    SoundStream* const      owner;        //!< Owning SoundStream object
    std::vector<ma_channel> soundChannelMap; //!< The map of position in sample frame to sound channel (miniaudio channels)
    ma_sound                sound{};         //!< The sound
    std::vector<std::int16_t> sampleBuffer;         //!< Our temporary sample buffer
    std::size_t               sampleBufferCursor{}; //!< The current read position in the temporary sample buffer
    std::uint64_t             samplesProcessed{};   //!< Number of samples processed since beginning of the stream
    unsigned int              channelCount{};       //!< Number of channels (1 = mono, 2 = stereo, ...)
    unsigned int              sampleRate{};         //!< Frequency (samples / second)
    std::vector<SoundChannel> channelMap{};         //!< The map of position in sample frame to sound channel
    bool                      loop{};               //!< Loop flag (true to loop, false to play once)
    bool                      streaming{true};      //!< True if we are still streaming samples from the source
};


////////////////////////////////////////////////////////////
SoundStream::SoundStream() : m_impl(std::make_unique<Impl>(this))
{
}


////////////////////////////////////////////////////////////
SoundStream::~SoundStream() = default;


////////////////////////////////////////////////////////////
void SoundStream::initialize(unsigned int channelCount, unsigned int sampleRate, const std::vector<SoundChannel>& channelMap)
{
    m_impl->channelCount     = channelCount;
    m_impl->sampleRate       = sampleRate;
    m_impl->channelMap       = channelMap;
    m_impl->samplesProcessed = 0;

    m_impl->reinitialize();
}


////////////////////////////////////////////////////////////
void SoundStream::play()
{
    if (auto result = ma_sound_start(&m_impl->sound); result != MA_SUCCESS)
        err() << "Failed to start playing sound: " << ma_result_description(result) << std::endl;
}


////////////////////////////////////////////////////////////
void SoundStream::pause()
{
    if (auto result = ma_sound_stop(&m_impl->sound); result != MA_SUCCESS)
        err() << "Failed to stop playing sound: " << ma_result_description(result) << std::endl;
}


////////////////////////////////////////////////////////////
void SoundStream::stop()
{
    if (auto result = ma_sound_stop(&m_impl->sound); result != MA_SUCCESS)
    {
        err() << "Failed to stop playing sound: " << ma_result_description(result) << std::endl;
    }
    else
    {
        setPlayingOffset(Time::Zero);
    }
}


////////////////////////////////////////////////////////////
unsigned int SoundStream::getChannelCount() const
{
    return m_impl->channelCount;
}


////////////////////////////////////////////////////////////
unsigned int SoundStream::getSampleRate() const
{
    return m_impl->sampleRate;
}


////////////////////////////////////////////////////////////
std::vector<SoundChannel> SoundStream::getChannelMap() const
{
    return m_impl->channelMap;
}


////////////////////////////////////////////////////////////
SoundStream::Status SoundStream::getStatus() const
{
    return SoundSource::getStatus();
}


////////////////////////////////////////////////////////////
void SoundStream::setPlayingOffset(Time timeOffset)
{
    if (m_impl->sampleRate == 0)
        return;

    ma_uint32 sampleRate{};

    if (auto result = ma_sound_get_data_format(&m_impl->sound, nullptr, nullptr, &sampleRate, nullptr, 0);
        result != MA_SUCCESS)
        err() << "Failed to get sound data format: " << ma_result_description(result) << std::endl;

    const auto frameIndex = static_cast<ma_uint64>(timeOffset.asSeconds() * static_cast<float>(sampleRate));

    if (auto result = ma_sound_seek_to_pcm_frame(&m_impl->sound, frameIndex); result != MA_SUCCESS)
        err() << "Failed to seek sound to pcm frame: " << ma_result_description(result) << std::endl;

    m_impl->streaming = true;
    m_impl->sampleBuffer.clear();
    m_impl->sampleBufferCursor = 0;
    m_impl->samplesProcessed   = frameIndex * m_impl->channelCount;

    onSeek(seconds(static_cast<float>(frameIndex / m_impl->sampleRate)));
}


////////////////////////////////////////////////////////////
Time SoundStream::getPlayingOffset() const
{
    if (m_impl->channelCount == 0 || m_impl->sampleRate == 0)
        return {};

    auto cursor = 0.f;

    if (auto result = ma_sound_get_cursor_in_seconds(&m_impl->sound, &cursor); result != MA_SUCCESS)
    {
        err() << "Failed to get sound cursor: " << ma_result_description(result) << std::endl;
        return {};
    }

    return seconds(cursor);
}


////////////////////////////////////////////////////////////
void SoundStream::setLoop(bool loop)
{
    ma_sound_set_looping(&m_impl->sound, loop ? MA_TRUE : MA_FALSE);
}


////////////////////////////////////////////////////////////
bool SoundStream::getLoop() const
{
    return ma_sound_is_looping(&m_impl->sound) == MA_TRUE;
}


////////////////////////////////////////////////////////////
std::int64_t SoundStream::onLoop()
{
    onSeek(Time::Zero);
    return 0;
}


////////////////////////////////////////////////////////////
void* SoundStream::getSound() const
{
    return &m_impl->sound;
}

} // namespace sf
