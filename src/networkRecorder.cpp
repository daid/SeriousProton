#include <string.h>
#include <cmath>

#include "networkRecorder.h"
#include "multiplayer.h"
#include "multiplayer_client.h"
#include "multiplayer_internal.h"
#include "input.h"
#include "logging.h"

#include <opus.h>

NetworkAudioRecorder::NetworkAudioRecorder()
{
    LOG(INFO) << "Using \"" << getDefaultDevice() << "\" for voice communication";
    setProcessingInterval(sf::milliseconds(10));
}

NetworkAudioRecorder::~NetworkAudioRecorder()
{
    stop();

    if (encoder)
    {
        opus_encoder_destroy(encoder);
    }
}

void NetworkAudioRecorder::addKeyActivation(sf::Keyboard::Key key, int target_identifier)
{
    keys.push_back({key, target_identifier});
}


/// Called from a seperate thread, be sure to watch for thread safety!
bool NetworkAudioRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sample_count)
{
    //Add samples to the sample buffer. The update function (which is run from the main thread) will handle sending of the actual audio packet.
    sample_buffer_mutex.lock();
    auto old_size = sample_buffer.size();
    sample_buffer.resize(old_size + sample_count);
    memcpy(&sample_buffer[old_size], samples, sizeof(sf::Int16) * sample_count);
    sample_buffer_mutex.unlock();
    return true;
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
                start(48000);
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
            samples_till_stop = 48000 / 2;
        }
    }
    if (samples_till_stop == 0)
    {
        stop();
        finishSending();
        active_key_index = -1;
        samples_till_stop = -1;
    }
}

void NetworkAudioRecorder::startSending()
{
    int error = 0;
    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);
    if (!encoder)
    {
        LOG(ERROR) << "Failed to create opus encoder:" << error;
    }

    if (game_client)
    {
        sf::Packet audio_packet;
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
    sample_buffer_mutex.lock();
    if (sample_buffer.size() >= frame_size)
    {
        unsigned char packet_buffer[4096];
        int packet_size = 0;
        if (encoder)
            packet_size = opus_encode(encoder, sample_buffer.data(), frame_size, packet_buffer, sizeof(packet_buffer));
        if (game_client)
        {
            sf::Packet audio_packet;
            audio_packet << CMD_AUDIO_COMM_DATA << game_client->getClientId();
            audio_packet.append(packet_buffer, packet_size);

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
    sample_buffer_mutex.unlock();
    return result;
}

void NetworkAudioRecorder::finishSending()
{
    while(sendAudioPacket())
    {
    }
    sample_buffer_mutex.lock();
    while(sample_buffer.size() < frame_size)
        sample_buffer.push_back(0);
    sample_buffer_mutex.unlock();
    sendAudioPacket();
    opus_encoder_destroy(encoder);
    encoder = nullptr;

    if (game_client)
    {
        sf::Packet audio_packet;
        audio_packet << CMD_AUDIO_COMM_STOP << game_client->getClientId();
        game_client->sendPacket(audio_packet);
    }
    else if (game_server)
    {
        game_server->stopAudio(0);
    }
}
