#include "networkAudioStream.h"
#include "logging.h"

#include <array>
#include <opus.h>

NetworkAudioStream::NetworkAudioStream()
{
    sample_rate = 48000;
    
    //Reserve 10 seconds of playback in our buffers.
    samples.reserve(sample_rate * 10);
    playing_samples.reserve(sample_rate * 10);
    
    initialize(1, sample_rate);  //initialize the sound stream on 1 channel with 48000Hz. This needs to match the data in NetworkAudioRecorder

    int error = 0;
    decoder = opus_decoder_create(48000, 1, &error);
}

bool NetworkAudioStream::onGetData(sf::SoundStream::Chunk& data)
{
    sf::Lock lock(samples_lock);    //Get exclusive access to the samples vector.
    //Copy all new samples to the playback buffer. And clear our sample buffer.
    playing_samples = std::move(samples);
    samples.clear();
    
    data.samples = playing_samples.data();
    data.sampleCount = playing_samples.size();
    
    //Stop playback if the buffer is empty.
    return data.sampleCount > 0;
}

void NetworkAudioStream::onSeek(sf::Time /*timeOffset*/)
{
    //We cannot seek in the network audio stream.
}

void NetworkAudioStream::receivedPacketFromNetwork(const unsigned char* packet, int packet_size)
{
    std::array<int16_t, 2880> samples_buffer;
    int sample_count = opus_decode(decoder, packet, packet_size, samples_buffer.data(), static_cast<int>(samples_buffer.size()), 0);
    if (sample_count > 0)
    {
        sf::Lock lock(samples_lock);
        this->samples.insert(this->samples.end(), samples_buffer.begin(), samples_buffer.begin() + sample_count);
    }
    if (this->samples.size() > sample_rate / 10 && getStatus() == sf::SoundSource::Stopped) //Start playback when there is 0.1 second of data
    {
        play();
    }
}

void NetworkAudioStream::finalize()
{
    if (this->samples.size() > 0 && getStatus() == sf::SoundSource::Stopped)
    {
        play();
    }
}

bool NetworkAudioStream::isFinished()
{
    return getStatus() == sf::SoundSource::Stopped && this->samples.size() == 0;
}

void NetworkAudioStreamManager::start(int32_t id)
{
    streams[id] = std::unique_ptr<NetworkAudioStream>(new NetworkAudioStream());
}

void NetworkAudioStreamManager::receivedPacketFromNetwork(int32_t id, const unsigned char* packet, int packet_size)
{
    auto it = streams.find(id);
    if (it == streams.end())
        return;
    it->second->receivedPacketFromNetwork(packet, packet_size);
}

void NetworkAudioStreamManager::stop(int32_t id)
{
    auto it = streams.find(id);
    if (it == streams.end())
        return;
    it->second->finalize();
}
