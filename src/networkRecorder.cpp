#include <string.h>
#include <cmath>

#include "networkRecorder.h"
#include "multiplayer.h"
#include "multiplayer_client.h"
#include "multiplayer_internal.h"
#include "input.h"
#include "logging.h"

NetworkAudioRecorder::NetworkAudioRecorder()
{
    mode = None;
    samplerate = 44100;
    target_identifier = 0;
    LOG(INFO) << "Using \"" << getDefaultDevice() << "\" at " << samplerate << "Hz for voice communication";
}

NetworkAudioRecorder::~NetworkAudioRecorder()
{
    stop();
}

void NetworkAudioRecorder::setVoiceActivation(float trigger_level)
{
    activation_key = sf::Keyboard::Unknown;
    mode = VoiceActivation;

    start(samplerate);
}

void NetworkAudioRecorder::setKeyActivation(sf::Keyboard::Key key)
{
    stop();
    
    activation_key = key;
    mode = KeyActivation;
}


/// Called from a seperate thread, be sure to watch for thread safety!
bool NetworkAudioRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sample_count)
{
    bool add_samples = false;
    
    if (mode == VoiceActivation)
    {
        int64_t rms_squared = 0;
        for(unsigned int n=0; n<sample_count; n++)
        {
            rms_squared += samples[n] * samples[n];
        }
        double rms = std::sqrt(rms_squared / sample_count) / std::numeric_limits<sf::Int16>::max();
        LOG(INFO) << "Network recorder RMS: " << rms;
    }
    
    if (mode == KeyActivation)
    {
        add_samples = true;
    }
    
    if (add_samples)
    {
        //Add samples to the sample buffer. The update function (which is run from the main thread) will handle sending of the actual audio packet.
        sample_buffer_mutex.lock();
        unsigned int old_size = sample_buffer.size();
        sample_buffer.resize(old_size + sample_count);
        memcpy(&sample_buffer[old_size], samples, sizeof(sf::Int16) * sample_count);
        sample_buffer_mutex.unlock();
    }
    return true;
}

void NetworkAudioRecorder::update(float delta)
{
    switch(mode)
    {
    case None:
        break;
    case VoiceActivation:
        break;
    case KeyActivation:
        if (InputHandler::keyboardIsPressed(activation_key))
        {
            record_start_time.restart();
            start(samplerate);
        }
        if (InputHandler::keyboardIsReleased(activation_key))
        {
            stop();
        }
        break;
    }
    
    sample_buffer_mutex.lock();
    if (sample_buffer.size() > 0)
    {
        if (game_client)
        {
            sf::Packet audio_packet;
            audio_packet << CMD_CLIENT_AUDIO_COMM;
            audio_packet << target_identifier;
            audio_packet << uint32_t(sample_buffer.size());
            for(unsigned int n=0; n<sample_buffer.size(); n++)
            {
                audio_packet << sample_buffer[n];
            }
            
            game_client->sendPacket(audio_packet);
        }
        else if (game_server)
        {
            game_server->gotAudioPacket(0, target_identifier, sample_buffer);
        }
        sample_buffer.clear();
    }
    sample_buffer_mutex.unlock();
}
