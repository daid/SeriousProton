#include <string.h>
#include <cmath>

#include "networkRecorder.h"
#include "multiplayer.h"
#include "multiplayer_client.h"
#include "multiplayer_internal.h"
#include "input.h"
#include "logging.h"

#include <SDL.h>
#include <opus.h>

static SDL_AudioDeviceID record_device_id;
static NetworkAudioRecorder* active_recorder;

NetworkAudioRecorder::NetworkAudioRecorder()
{
    if (record_device_id == 0)
    {
        SDL_AudioSpec want, obtained;
        memset(&want, 0, sizeof(want));
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.samples = 4410;
        want.channels = 2;
        want.callback = &NetworkAudioRecorder::SDLCallback;
        record_device_id = SDL_OpenAudioDevice(nullptr, true, &want, &obtained, false);
    }
    active_recorder = this;
}

NetworkAudioRecorder::~NetworkAudioRecorder()
{
    SDL_PauseAudioDevice(record_device_id, 1);

    if (encoder)
    {
        opus_encoder_destroy(encoder);
    }
    active_recorder = nullptr;
}

void NetworkAudioRecorder::addKeyActivation(int key, int target_identifier)
{
    keys.push_back({key, target_identifier});
}

/// Called from a seperate thread, be sure to watch for thread safety!
void NetworkAudioRecorder::SDLCallback(void* userdata, uint8_t* stream, int len)
{
    active_recorder->onProcessSamples(reinterpret_cast<int16_t*>(stream), len / 2);
}

void NetworkAudioRecorder::onProcessSamples(const int16_t* samples, std::size_t sample_count)
{
    //Add samples to the sample buffer. The update function (which is run from the main thread) will handle sending of the actual audio packet.
    sample_buffer_mutex.lock();
    auto old_size = sample_buffer.size();
    sample_buffer.resize(old_size + sample_count);
    memcpy(&sample_buffer[old_size], samples, sizeof(int16_t) * sample_count);
    sample_buffer_mutex.unlock();
}

void NetworkAudioRecorder::update(float /*delta*/)
{
    for(size_t idx=0; idx<keys.size(); idx++)
    {
        if (InputHandler::keyboardIsPressed(keys[idx].key) && active_key_index == -1)
        {
            if (active_key_index == -1)
            {
                samples_till_stop = -1;
                active_key_index = static_cast<int>(idx);
                SDL_PauseAudioDevice(record_device_id, 0);
                startSending();
            } else if (idx == size_t(active_key_index))
            {
                samples_till_stop = -1;
            }
        }
    }
    while(sendAudioPacket())
    {
    }
    if (active_key_index != -1)
    {
        if (InputHandler::keyboardIsReleased(keys[active_key_index].key))
        {
            samples_till_stop = 44100 / 2;
        }
    }
    if (samples_till_stop == 0)
    {
        SDL_PauseAudioDevice(record_device_id, 1);
        finishSending();
        active_key_index = -1;
        samples_till_stop = -1;
    }
}

void NetworkAudioRecorder::startSending()
{
    int error = 0;
    encoder = opus_encoder_create(44100, 1, OPUS_APPLICATION_VOIP, &error);
    if (!encoder)
    {
        LOG(ERROR) << "Failed to create opus encoder:" << error;
    }

    if (game_client)
    {
        sp::io::DataBuffer audio_packet;
        audio_packet << CMD_AUDIO_COMM_START << game_client->getClientId() << int32_t(keys[active_key_index].target_identifier);
        game_client->sendPacket(audio_packet);
    }
    else if (game_server)
    {
        game_server->startAudio(0, keys[active_key_index].target_identifier);
    }
}

bool NetworkAudioRecorder::sendAudioPacket()
{
    bool result = false;
    std::lock_guard<std::mutex> guard(sample_buffer_mutex);
    if (sample_buffer.size() >= frame_size)
    {
        unsigned char packet_buffer[4096];
        int packet_size = 0;
        if (encoder)
            packet_size = opus_encode(encoder, sample_buffer.data(), frame_size, packet_buffer, sizeof(packet_buffer));
        if (game_client)
        {
            sp::io::DataBuffer audio_packet;
            audio_packet << CMD_AUDIO_COMM_DATA << game_client->getClientId();
            audio_packet.appendRaw(packet_buffer, packet_size);

            game_client->sendPacket(audio_packet);
        }
        else if (game_server)
        {
            game_server->gotAudioPacket(0, packet_buffer, packet_size);
        }
        sample_buffer.erase(sample_buffer.begin(), sample_buffer.begin() + frame_size);
        result = true;

        if (samples_till_stop > -1)
        {
            samples_till_stop = std::max(0, samples_till_stop - frame_size);
        }
    }
    return result;
}

void NetworkAudioRecorder::finishSending()
{
    while(sendAudioPacket())
    {
    }

    {
        std::lock_guard<std::mutex> guard(sample_buffer_mutex);
        while(sample_buffer.size() < frame_size)
            sample_buffer.push_back(0);
    }
    
    sendAudioPacket();
    opus_encoder_destroy(encoder);
    encoder = nullptr;

    if (game_client)
    {
        sp::io::DataBuffer audio_packet;
        audio_packet << CMD_AUDIO_COMM_STOP << game_client->getClientId();
        game_client->sendPacket(audio_packet);
    }
    else if (game_server)
    {
        game_server->stopAudio(0);
    }
}
