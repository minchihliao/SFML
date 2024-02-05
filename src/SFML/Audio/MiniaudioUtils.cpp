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
#include <SFML/Audio/MiniaudioUtils.hpp>
#include <SFML/Audio/SoundChannel.hpp>

#include <SFML/System/Angle.hpp>

#include <miniaudio.h>

#include <limits>

#include <cassert>


namespace sf::priv
{
////////////////////////////////////////////////////////////
ma_channel soundChannelToMiniaudioChannel(sf::SoundChannel soundChannel)
{
    switch (soundChannel)
    {
        case SoundChannel::Unspecified:
            return MA_CHANNEL_NONE;
        case SoundChannel::Mono:
            return MA_CHANNEL_MONO;
        case SoundChannel::FrontLeft:
            return MA_CHANNEL_FRONT_LEFT;
        case SoundChannel::FrontRight:
            return MA_CHANNEL_FRONT_RIGHT;
        case SoundChannel::FrontCenter:
            return MA_CHANNEL_FRONT_CENTER;
        case SoundChannel::FrontLeftOfCenter:
            return MA_CHANNEL_FRONT_LEFT_CENTER;
        case SoundChannel::FrontRightOfCenter:
            return MA_CHANNEL_FRONT_RIGHT_CENTER;
        case SoundChannel::LowFrequencyEffects:
            return MA_CHANNEL_LFE;
        case SoundChannel::BackLeft:
            return MA_CHANNEL_BACK_LEFT;
        case SoundChannel::BackRight:
            return MA_CHANNEL_BACK_RIGHT;
        case SoundChannel::BackCenter:
            return MA_CHANNEL_BACK_CENTER;
        case SoundChannel::SideLeft:
            return MA_CHANNEL_SIDE_LEFT;
        case SoundChannel::SideRight:
            return MA_CHANNEL_SIDE_RIGHT;
        case SoundChannel::TopCenter:
            return MA_CHANNEL_TOP_CENTER;
        case SoundChannel::TopFrontLeft:
            return MA_CHANNEL_TOP_FRONT_LEFT;
        case SoundChannel::TopFrontRight:
            return MA_CHANNEL_TOP_FRONT_RIGHT;
        case SoundChannel::TopFrontCenter:
            return MA_CHANNEL_TOP_FRONT_CENTER;
        case SoundChannel::TopBackLeft:
            return MA_CHANNEL_TOP_BACK_LEFT;
        case SoundChannel::TopBackRight:
            return MA_CHANNEL_TOP_BACK_RIGHT;
        default:
            assert(soundChannel == SoundChannel::TopBackCenter);
            return MA_CHANNEL_TOP_BACK_CENTER;
    }
}


////////////////////////////////////////////////////////////
void initializeSoundWithDefaultSettings(ma_sound& sound)
{
    ma_sound_set_pitch(&sound, 1.f);
    ma_sound_set_pan(&sound, 0.f);
    ma_sound_set_volume(&sound, 1.f);
    ma_sound_set_spatialization_enabled(&sound, MA_TRUE);
    ma_sound_set_position(&sound, 0.f, 0.f, 0.f);
    ma_sound_set_direction(&sound, 0.f, 0.f, -1.f);
    ma_sound_set_cone(&sound, degrees(360).asRadians(), degrees(360).asRadians(), 0.f); // inner = 360 degrees, outer = 360 degrees, gain = 0
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


////////////////////////////////////////////////////////////
Time getMiniaudioPlayingOffset(ma_sound& sound)
{
    float cursor = 0.f;

    if (ma_result result = ma_sound_get_cursor_in_seconds(&sound, &cursor); result != MA_SUCCESS)
    {
        err() << "Failed to get sound cursor: " << ma_result_description(result) << std::endl;
        return {};
    }

    return seconds(cursor);
}


////////////////////////////////////////////////////////////
ma_uint64 getMiniaudioFrameIndex(ma_sound& sound, Time timeOffset)
{
    ma_uint32 sampleRate{};

    if (ma_result result = ma_sound_get_data_format(&sound, nullptr, nullptr, &sampleRate, nullptr, 0); result != MA_SUCCESS)
        err() << "Failed to get sound data format: " << ma_result_description(result) << std::endl;

    const auto frameIndex = static_cast<ma_uint64>(timeOffset.asSeconds() * static_cast<float>(sampleRate));

    if (ma_result result = ma_sound_seek_to_pcm_frame(&sound, frameIndex); result != MA_SUCCESS)
        err() << "Failed to seek sound to pcm frame: " << ma_result_description(result) << std::endl;

    return frameIndex;
}

} // namespace sf::priv
