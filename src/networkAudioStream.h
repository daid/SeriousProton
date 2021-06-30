#ifndef NETWORK_AUDIOSTREAM_H
#define NETWORK_AUDIOSTREAM_H

#include <audio/source.h>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <vector>


struct OpusDecoder;
class NetworkAudioStream: public sp::audio::Source
{
public:
    NetworkAudioStream();

    void receivedPacketFromNetwork(const unsigned char* packet, int packet_size);
    void finalize();
    bool isFinished();
protected:
    // Inherited functions
    virtual void onMixSamples(int16_t* stream, int sample_count) override;

    //Members
    unsigned int sample_rate;
    std::mutex             samples_lock;
    std::vector<int16_t>   samples;

    OpusDecoder* decoder = nullptr;
};

class NetworkAudioStreamManager
{
public:
    void start(int32_t id);
    void receivedPacketFromNetwork(int32_t id, const unsigned char* packet, int packet_size);
    void stop(int32_t id);

private:
    std::unordered_map<int32_t, std::unique_ptr<NetworkAudioStream>> streams;
};

#endif //NETWORK_AUDIOSTREAM_H
