#ifndef NETWORK_RECORDER_H
#define NETWORK_RECORDER_H
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>

class NetworkRecorder: public sf::SoundRecorder
{
    public:
        NetworkRecorder(const sf::IpAddress& _host, unsigned short _port) : host(_host), port(_port);
    protected:

        // Inherited functions
        virtual bool onStart();
        virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sample_count);
        virtual void onStop();

        //Members
        sf::IpAddress  host;   // Address of the remote host
        unsigned short port;   // Remote port
        sf::TcpSocket  socket; // Socket used to communicate with the server
};

#endif //NETWORKRECORDER_H
