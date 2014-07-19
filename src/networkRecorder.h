#ifndef NETWORK_RECORDER_H
#define NETWORK_RECORDER_H
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>

class NetworkRecorder: public sf::SoundRecorder
{
    public:
        NetworkRecorder(const sf::IpAddress& _host = sf::IpAddress("127.0.0.1"), unsigned short _port = 9002) : host(_host), port(_port){};
        void setCommands(const int16_t _voice_command, const int16_t _open_voice,int16_t _close_voice);
    protected:

        // Inherited functions
        virtual bool onStart();
        virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sample_count);
        virtual void onStop();
        int16_t voice_command;
        int16_t open_voice;
        int16_t close_voice;
        //Members
        sf::IpAddress  host;   // Address of the remote host
        unsigned short port;   // Remote port
        sf::TcpSocket  socket; // Socket used to communicate with the server
};

#endif //NETWORKRECORDER_H
