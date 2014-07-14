#ifndef NETWORK_AUDIOSTREAM_H
#define NETWORK_AUDIOSTREAM_H
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>

class NetworkAudioStream: public sf::SoundStream
{
    public:
        NetworkAudioStream();
        void startListening(unsigned short port);
    protected:

        // Inherited functions
        virtual bool onGetData(sf::SoundStream::Chunk& data);
        virtual void onSeek(sf::Time timeOffset);

        void receiveLoop();

        //Members
        sf::TcpListener        listener;
        sf::TcpSocket          client;
        sf::Mutex              mutex;
        std::vector<sf::Int16> samples;
        std::vector<sf::Int16> temp_buffer;
        std::size_t            offset;
        bool                   has_finished;
};

#endif //NETWORK_AUDIOSTREAM_H
