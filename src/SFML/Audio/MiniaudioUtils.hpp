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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/SoundChannel.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/Time.hpp>

#include <miniaudio.h>


namespace sf::priv
{
////////////////////////////////////////////////////////////
[[nodiscard]] ma_channel soundChannelToMiniaudioChannel(sf::SoundChannel soundChannel);


////////////////////////////////////////////////////////////
void initializeSoundWithDefaultSettings(ma_sound& sound);


////////////////////////////////////////////////////////////
template <typename F>
void reinitializeMiniaudioSound(ma_sound& sound, F initializeFn)
{
    // Save and re-apply settings
    const float          pitch                        = ma_sound_get_pitch(&sound);
    const float          pan                          = ma_sound_get_pan(&sound);
    const float          volume                       = ma_sound_get_volume(&sound);
    const ma_bool32      spatializationEnabled        = ma_sound_is_spatialization_enabled(&sound);
    const ma_vec3f       position                     = ma_sound_get_position(&sound);
    const ma_vec3f       direction                    = ma_sound_get_direction(&sound);
    const float          directionalAttenuationFactor = ma_sound_get_directional_attenuation_factor(&sound);
    const ma_vec3f       velocity                     = ma_sound_get_velocity(&sound);
    const float          dopplerFactor                = ma_sound_get_doppler_factor(&sound);
    const ma_positioning positioning                  = ma_sound_get_positioning(&sound);
    const float          minDistance                  = ma_sound_get_min_distance(&sound);
    const float          maxDistance                  = ma_sound_get_max_distance(&sound);
    const float          minGain                      = ma_sound_get_min_gain(&sound);
    const float          maxGain                      = ma_sound_get_max_gain(&sound);
    const float          rollOff                      = ma_sound_get_rolloff(&sound);

    float innerAngle;
    float outerAngle;
    float outerGain;
    ma_sound_get_cone(&sound, &innerAngle, &outerAngle, &outerGain);

    ma_sound_uninit(&sound);

    initializeFn();

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


////////////////////////////////////////////////////////////
template <typename F>
void initializeMiniaudioSound(const ma_data_source_vtable& vtable, ma_data_source_base& dataSourceBase, ma_sound& sound, F initializeFn)
{
    // Set this object up as a miniaudio data source
    ma_data_source_config config = ma_data_source_config_init();
    config.vtable                = &vtable;

    if (ma_result result = ma_data_source_init(&config, &dataSourceBase); result != MA_SUCCESS)
        err() << "Failed to initialize audio data source: " << ma_result_description(result) << std::endl;

    // Initialize sound structure and set default settings
    initializeFn();
    initializeSoundWithDefaultSettings(sound);
}


////////////////////////////////////////////////////////////
Time getMiniaudioPlayingOffset(ma_sound& sound);


////////////////////////////////////////////////////////////
ma_uint64 getMiniaudioFrameIndex(ma_sound& sound, Time timeOffset);

} // namespace sf::priv
