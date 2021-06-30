#include "networkAudioStream.h"
#include "logging.h"

#include <array>
#include <opus.h>


NetworkAudioStream::NetworkAudioStream()
{
    sample_rate = 44100;
    
    //Reserve 10 seconds of playback in our buffers.
    samples.reserve(sample_rate * 10);
    
    int error = 0;
    decoder = opus_decoder_create(sample_rate, 1, &error);
}

void NetworkAudioStream::onMixSamples(int16_t* stream, int sample_count)
{
    std::lock_guard<std::mutex> guard(samples_lock);    //Get exclusive access to the samples vector.

    //Copy all new samples to the playback buffer. And clear our sample buffer.
    int mix_count = std::min(sample_count / 2, int(samples.size()));
    for(int index=0; index<mix_count; index++) {
        int sample = samples[index];
        mix(stream[index*2+0], sample);
        mix(stream[index*2+1], sample);
    }
    samples.erase(samples.begin(), samples.begin() + mix_count);
    
    //Stop playback if the buffer is empty.
    if (samples.empty())
        stop();
}

void NetworkAudioStream::receivedPacketFromNetwork(const unsigned char* packet, int packet_size)
{
    std::array<int16_t, 2880> samples_buffer;
    int sample_count = opus_decode(decoder, packet, packet_size, samples_buffer.data(), static_cast<int>(samples_buffer.size()), 0);
    if (sample_count > 0)
    {
        std::lock_guard<std::mutex> guard(samples_lock);
        this->samples.insert(this->samples.end(), samples_buffer.begin(), samples_buffer.begin() + sample_count);
    }
    if (this->samples.size() > sample_rate / 10 && !isPlaying()) //Start playback when there is 0.1 second of data
    {
        start();
    }
}

void NetworkAudioStream::finalize()
{
    if (this->samples.size() > 0 && !isPlaying())
    {
        start();
    }
}

bool NetworkAudioStream::isFinished()
{
    return !isPlaying() && this->samples.size() == 0;
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
