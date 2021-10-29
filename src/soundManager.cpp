#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "vectorUtils.h"
#include "resources.h"
#include "soundManager.h"
#include "random.h"

SoundManager* soundManager;

SoundManager::SoundManager()
{
    master_sound_volume = 1.0f;
    music_volume = 1.0f;
    positional_sound_enabled = false;
    music_channel.mode = None;
}

SoundManager::~SoundManager()
{
}

void SoundManager::playMusic(string name)
{
    music_set.clear();
    startMusic(name, true);
}

void SoundManager::playMusicSet(const std::vector<string>& filenames)
{
    music_set = filenames;
    if (music_set.size() > 0)
        startMusic(music_set[irandom(0, static_cast<int>(music_set.size()) - 1)], false);
    else
        stopMusic();
}

void SoundManager::stopMusic()
{
    music_set.clear();
    music_channel.music.stop();
}

void SoundManager::setMusicVolume(float volume)
{
    if (music_volume != volume)
    {
        music_volume = volume;
        if (music_channel.mode == None && music_channel.music.isPlaying())
            music_channel.music.setVolume(music_volume);
    }
}

float SoundManager::getMusicVolume()
{
    return music_volume;
}

void SoundManager::stopSound(int index)
{
    if (index < 0 || index >= int(active_sound_list.size()))
        return;
    auto& sound = active_sound_list[index];
    if (sound.playback.isPlaying())
    {
        sound.playback.stop();
    }
}

void SoundManager::setMasterSoundVolume(float volume)
{
    if (volume != master_sound_volume * 100.0f)
    {
        master_sound_volume = std::clamp(volume / 100.0f, 0.0f, 1.0f);
    }
}

float SoundManager::getMasterSoundVolume()
{
    return master_sound_volume * 100.0f;
}

void SoundManager::setSoundVolume(int index, float volume)
{
    auto& sound = active_sound_list[index];
    if (sound.playback.isPlaying())
    {
        sound.volume = std::clamp(volume / 100.0f, 0.0f, 1.0f);
        updateChannelVolume(sound);
    }
}

void SoundManager::setSoundPitch(int index, float pitch)
{
    if (index < 0 || index >= int(active_sound_list.size()))
        return;
    auto& sound = active_sound_list[index];
    if (sound.playback.isPlaying())
    {
        // Bound pitch to 0.0f or greater.
        pitch = std::max(0.0f, pitch);
        sound.playback.setPitch(pitch);
    }
}

int SoundManager::playSound(string name, float pitch, float volume, bool loop)
{
    auto data = sound_map[name];
    if (data == nullptr)
        data = loadSound(name);

    // Return the sound's index in activeSoundList[].
    // Returns -1 if the list was full of playing sounds.
    return playSoundData(data, pitch, volume, loop);
}

void SoundManager::setListenerPosition(glm::vec2 position, float angle)
{
    //auto listen_vector = vec2FromAngle(angle);
    positional_sound_enabled = true;
    listener_position = glm::vec3(position.x, position.y, 0);
}

void SoundManager::disablePositionalSound()
{
    positional_sound_enabled = false;
}

int SoundManager::playSound(string name, glm::vec2 position, float min_distance, float attenuation, float pitch, float volume, bool loop)
{
    if (!positional_sound_enabled)
        return -1;
    auto* data = sound_map[name];
    if (data == nullptr)
        data = loadSound(name);
    if (data->getChannelCount() > 1)
        LOG(WARNING) << name << ": Used as positional sound but has more than 1 channel.";

    for(unsigned int n = 0; n < active_sound_list.size(); n++)
    {
        auto& sound = active_sound_list[n];
        if (!sound.playback.isPlaying())
        {
            sound.positional = true;
            sound.position = position;
            sound.min_distance = min_distance;
            sound.attenuation = attenuation;
            sound.volume = std::clamp(volume / 100.0f, 0.0f, 1.0f);
            sound.playback.setPitch(pitch);
            updateChannelVolume(sound);
            sound.playback.play(*data, loop);
            return int(n);
        }
    }

    // No room in activeSoundList; return -1.
    return -1;
}

int SoundManager::playSoundData(sp::audio::Sound* data, float pitch, float volume, bool loop)
{
    for(unsigned int n = 0; n < active_sound_list.size(); n++)
    {
        auto& sound = active_sound_list[n];
        if (!sound.playback.isPlaying())
        {
            sound.positional = false;
            sound.volume = std::clamp(volume / 100.0f, 0.0f, 1.0f);
            updateChannelVolume(sound);
            sound.playback.setPitch(pitch);
            sound.playback.play(*data, loop);
            return int(n);
        }
    }

    // No room in activeSoundList; return -1.
    return -1;
}

sp::audio::Sound* SoundManager::loadSound(const string& name)
{
    auto data = sound_map[name];
    if (data)
        return data;

    data = new sp::audio::Sound(name);

    if (data->getChannelCount() == 0)
    {
        LOG(Warning, "Failed to load sound: ", name);
        sound_map[name] = data;
        return data;
    }

    LOG(Info, "Loaded: ", name, " of ", data->getDuration(), " seconds");
    sound_map[name] = data;
    return data;
}

void SoundManager::updateChannelVolume(SoundChannel& channel)
{
    if (channel.positional)
    {
        auto distance = glm::length(listener_position - channel.position);
        distance = std::max(channel.min_distance, distance);
        float gain = channel.min_distance / (channel.min_distance + channel.attenuation * (distance - channel.min_distance));
        channel.playback.setVolume(channel.volume * master_sound_volume * gain);
    }
    else
    {
        channel.playback.setVolume(channel.volume * master_sound_volume);
    }
}

void SoundManager::startMusic(const string& name, bool loop)
{
    if (name.empty())
        return;
    
    if (music_channel.music.isPlaying())
    {
        music_channel.next_stream = name;
        music_channel.mode = FadeOut;
        music_channel.fade_delay = fade_music_time;
    }else{
        music_channel.mode = FadeIn;
        music_channel.fade_delay = fade_music_time;

        music_channel.music.setVolume(0);
        music_channel.music.open(name, loop);
    }
}

void SoundManager::updateTick()
{
    float delta = clock.restart();
    updateChannel(music_channel, delta);
    
    if (music_set.size() > 0)
    {
        if (music_channel.music.isPlaying() && music_channel.mode == None)
        {
            if (music_channel.next_stream.empty())
            {
                /* TODO
                if (music_channel.music.getPlayingOffset() > music_channel.music.getDuration() - sf::seconds(fade_music_time))
                {
                    startMusic(getResourceStream(music_set[irandom(0, static_cast<int>(music_set.size()) - 1)]), false);
                }
                */
            }
        }
    }

    for(auto& channel : active_sound_list)
    {
        if (channel.positional && channel.playback.isPlaying() && positional_sound_enabled)
        {
            updateChannelVolume(channel);
        }
    }
}

void SoundManager::updateChannel(MusicChannel& channel, float delta)
{
    switch(channel.mode)
    {
    case None:
        break;
    case FadeIn:
        channel.fade_delay -= delta;
        if (channel.fade_delay > 0.f)
        {
            channel.music.setVolume(music_volume * (1.f - (channel.fade_delay / fade_music_time)));
        }else{
            channel.music.setVolume(music_volume);
            channel.mode = None;
        }
        break;
    case FadeOut:
        channel.fade_delay -= delta;
        if (channel.fade_delay > 0.f)
        {
            channel.music.setVolume(music_volume * (channel.fade_delay / fade_music_time));
        }else{
            channel.music.stop();
            channel.mode = None;
            if (!channel.next_stream.empty())
            {
                channel.music.setVolume(0);
                channel.music.open(channel.next_stream, false);
                channel.next_stream.clear();
                channel.mode = FadeIn;
                channel.fade_delay = fade_music_time;
            }
        }
        break;
    }
}
