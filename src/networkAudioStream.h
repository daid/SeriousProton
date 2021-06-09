#ifndef NETWORK_AUDIOSTREAM_H
#define NETWORK_AUDIOSTREAM_H

#include <SFML/Audio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>


struct OpusDecoder;
class NetworkAudioStream: public sf::SoundStream
{
public:
    NetworkAudioStream();

    void receivedPacketFromNetwork(const unsigned char* packet, int packet_size);
    void finalize();
    bool isFinished();
protected:
    // Inherited functions
    virtual bool onGetData(sf::SoundStream::Chunk& data);
    virtual void onSeek(sf::Time timeOffset);

    //Members
    unsigned int sample_rate;
    sf::Mutex              samples_lock;
    std::vector<sf::Int16> samples;
    std::vector<sf::Int16> playing_samples;

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
