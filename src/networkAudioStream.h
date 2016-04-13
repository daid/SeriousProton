#ifndef NETWORK_AUDIOSTREAM_H
#define NETWORK_AUDIOSTREAM_H

#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>

class NetworkAudioStream: public sf::SoundStream
{
    public:
        NetworkAudioStream();

        void receivedSamplesFromNetwork(const std::vector<sf::Int16>& samples);
    protected:
        // Inherited functions
        virtual bool onGetData(sf::SoundStream::Chunk& data);
        virtual void onSeek(sf::Time timeOffset);

        //Members
        unsigned int sample_rate;
        sf::Mutex              samples_lock;
        std::vector<sf::Int16> samples;
        std::vector<sf::Int16> playing_samples;
};

#endif //NETWORK_AUDIOSTREAM_H
