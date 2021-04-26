#include <stdio.h>
#include <string.h>
#include <SFML/Network.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "vectorUtils.h"
#include "resources.h"
#include "soundManager.h"
#include "random.h"

#define MAX_SOUNDS 16

SoundManager* soundManager;

SoundManager::SoundManager()
{
    // By creating a SoundBuffer we force SFML to load the sound subsystem.
    // Else this is done when the first sound is loaded, causing a delay at that
    // point.
    sf::SoundBuffer forceLoadBuffer;

    for(unsigned int n = 0; n < MAX_SOUNDS; n++)
        activeSoundList.push_back(sf::Sound());
    
    master_sound_volume = 100.0;
    music_volume = 100.0;
    positional_sound_enabled = false;
    music_channel.mode = None;
}

SoundManager::~SoundManager()
{
}

void SoundManager::playMusic(string name)
{
    music_set.clear();
    P<ResourceStream> stream = getResourceStream(name);
    if (stream)
        startMusic(stream, true);
}

void SoundManager::playMusicSet(std::vector<string> filenames)
{
    music_set = filenames;
    if (music_set.size() > 0)
        startMusic(getResourceStream(music_set[irandom(0, static_cast<int>(music_set.size()) - 1)]), false);
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
        if (music_channel.mode == None && music_channel.music.getStatus() != sf::Music::Stopped)
            music_channel.music.setVolume(music_volume);
    }
}

float SoundManager::getMusicVolume()
{
    return music_volume;
}

void SoundManager::stopSound(int index)
{
    if (index < 0 || index >= MAX_SOUNDS)
        return;
    sf::Sound& sound = activeSoundList[index];
    if (sound.getStatus() == sf::Sound::Playing)
    {
        sound.setLoop(false);
        sound.stop();
    }
}

void SoundManager::setMasterSoundVolume(float volume)
{
    // Set factor by which sound effect playback is multiplied.
    // Bound volume between 0.0f and 100.0f.
    master_sound_volume = std::max(0.0f, std::min(100.0f, volume));
}

float SoundManager::getMasterSoundVolume()
{
    return master_sound_volume;
}

void SoundManager::setSoundVolume(int index, float volume)
{
    sf::Sound& sound = activeSoundList[index];
    if (sound.getStatus() == sf::Sound::Playing)
    {
        // Bound volume between 0.0f and 100.0f.
        volume = std::max(0.0f, std::min(100.0f, volume));
        sound.setVolume(volume);
    }
}

float SoundManager::getSoundVolume(int index)
{
    sf::Sound& sound = activeSoundList[index];
    if (sound.getStatus() == sf::Sound::Playing)
    {
        return sound.getVolume();
    } else {
        return 0.0f;
    }
}

void SoundManager::setSoundPitch(int index, float pitch)
{
    sf::Sound& sound = activeSoundList[index];
    if (sound.getStatus() == sf::Sound::Playing)
    {
        // Bound pitch to 0.0f or greater.
        pitch = std::max(0.0f, pitch);
        sound.setPitch(pitch);
    }
}

float SoundManager::getSoundPitch(int index)
{
    sf::Sound& sound = activeSoundList[index];
    if (sound.getStatus() == sf::Sound::Playing)
    {
        return sound.getPitch();
    } else {
        return 0.0f;
    }
}

int SoundManager::playSound(string name, float pitch, float volume, bool loop)
{
    sf::SoundBuffer* data = soundMap[name];
    if (data == NULL)
        data = loadSound(name);

    // Return the sound's index in activeSoundList[].
    // Returns -1 if the list was full of playing sounds.
    return playSoundData(data, pitch, volume * (master_sound_volume / 100.f), loop);
}

void SoundManager::setListenerPosition(sf::Vector2f position, float angle)
{
    sf::Vector2f listen_vector = sf::vector2FromAngle(angle);
    sf::Listener::setPosition(position.x, 0, position.y);
    sf::Listener::setDirection(listen_vector.x, 0, listen_vector.y);
    positional_sound_enabled = true;
}

void SoundManager::disablePositionalSound()
{
    positional_sound_enabled = false;
}

int SoundManager::playSound(string name, sf::Vector2f position, float min_distance, float attenuation, float pitch, float volume, bool loop)
{
    if (!positional_sound_enabled)
        return -1;
    sf::SoundBuffer* data = soundMap[name];
    if (data == NULL)
        data = loadSound(name);
    if (data->getChannelCount() > 1)
        LOG(WARNING) << name << ": Used as positional sound but has more than 1 channel.";

    for(unsigned int n = 0; n < activeSoundList.size(); n++)
    {
        sf::Sound& sound = activeSoundList[n];
        if (sound.getStatus() == sf::Sound::Stopped)
        {
            sound.setBuffer(*data);
            sound.setRelativeToListener(false);
            sound.setMinDistance(min_distance);
            sound.setAttenuation(attenuation);
            sound.setPosition(position.x, 0, position.y);
            sound.setPitch(pitch);
            sound.setVolume(volume * (master_sound_volume / 100.f));
            sound.setLoop(loop);
            sound.play();
            return int(n);
        }
    }

    // No room in activeSoundList; return -1.
    return -1;
}

void SoundManager::setTextToSpeachVoice(string name)
{
}

string url_encode(const string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else if (c == ' ') {
            escaped << '+';
        }
        else {
            escaped << '%' << std::setw(2) << ((int) c) << std::setw(0);
        }
    }

    return escaped.str();
}

void SoundManager::playTextToSpeech(string text)
{
    string name = "TTS:" + text;
    sf::SoundBuffer* data = soundMap[name];
    if (data != NULL)
    {
        playSoundData(data, 1.0, 100.0, false);
        return;
    }

    sf::Http http("localhost", 59125);
    sf::Http::Request request("process?INPUT_TEXT=" + url_encode(text) + "&INPUT_TYPE=TEXT&OUTPUT_TYPE=AUDIO&AUDIO=WAVE_FILE&LOCALE=en_US&VOICE=dfki-prudence");
    sf::Http::Response response = http.sendRequest(request);

    sf::Http::Response::Status status = response.getStatus();
    if (status == sf::Http::Response::Ok)
    {
        string wave = response.getBody();
        sf::SoundBuffer* soundbuffer = new sf::SoundBuffer();
        data->loadFromMemory(wave.data(), wave.size());
        soundMap[name] = soundbuffer;
        playSoundData(soundbuffer, 1.f, 100.f, false);
    }
    else
    {
        std::cout << "Error requesting text to speech from Mary server: " << status << std::endl;
    }
}

int SoundManager::playSoundData(sf::SoundBuffer* data, float pitch, float volume, bool loop)
{
    for(unsigned int n = 0; n < activeSoundList.size(); n++)
    {
        sf::Sound& sound = activeSoundList[n];
        if (sound.getStatus() == sf::Sound::Stopped)
        {
            sound.setBuffer(*data);
            sound.setRelativeToListener(true);
            sound.setMinDistance(1);
            sound.setAttenuation(0);
            sound.setPitch(pitch);
            sound.setVolume(volume);
            sound.setPosition(0, 0, 0);
            sound.setLoop(loop);
            sound.play();
            return int(n);
        }
    }

    // No room in activeSoundList; return -1.
    return -1;
}

sf::SoundBuffer* SoundManager::loadSound(string name)
{
    sf::SoundBuffer* data = soundMap[name];
    if (data)
        return data;

    data = new sf::SoundBuffer();

    P<ResourceStream> stream = getResourceStream(name);
    if (!stream) stream = getResourceStream(name + ".wav");
    if (!stream || !data->loadFromStream(**stream))
    {
        LOG(WARNING) << "Failed to load sound: " << name;
        soundMap[name] = data;
        return data;
    }

    LOG(INFO) << "Loaded: " << name << " of " << data->getDuration().asSeconds() << " seconds";
    soundMap[name] = data;
    return data;
}

void SoundManager::startMusic(P<ResourceStream> stream, bool loop)
{
    if (!stream)
        return;
    
    if (music_channel.music.getStatus() == sf::Music::Playing)
    {
        music_channel.next_stream = stream;
        music_channel.mode = FadeOut;
        music_channel.fade_delay = fade_music_time;
    }else{
        music_channel.mode = FadeIn;
        music_channel.fade_delay = fade_music_time;

        music_channel.music.openFromStream(**stream);
        music_channel.stream = stream;
        music_channel.music.setLoop(loop);
        music_channel.music.setVolume(0);
        music_channel.music.play();
    }
}

void SoundManager::updateTick()
{
    float delta = clock.restart().asSeconds();
    updateChannel(music_channel, delta);
    
    if (music_set.size() > 0)
    {
        if (music_channel.music.getStatus() == sf::Music::Playing && music_channel.mode == None)
        {
            if (!music_channel.next_stream)
            {
                if (music_channel.music.getPlayingOffset() > music_channel.music.getDuration() - sf::seconds(fade_music_time))
                {
                    startMusic(getResourceStream(music_set[irandom(0, static_cast<int>(music_set.size()) - 1)]), false);
                }
            }
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
            if (channel.next_stream)
            {
                channel.music.openFromStream(**channel.next_stream);
                channel.stream = channel.next_stream;
                channel.next_stream = nullptr;
                channel.mode = FadeIn;
                channel.fade_delay = fade_music_time;
                channel.music.setVolume(0);
                channel.music.play();
            }
        }
        break;
    }
}
