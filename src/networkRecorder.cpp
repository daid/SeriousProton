#include "networkRecorder.h"

bool NetworkRecorder::onStart()
{
    if (socket.connect(host, port) == sf::Socket::Done)
    {
        std::cout << "Connected to server " << host << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}

void NetworkRecorder::setCommands(const int16_t _voice_command, const int16_t _open_voice,int16_t _close_voice)
{
    voice_command= _voice_command;
    open_voice = _open_voice;
    close_voice = _close_voice;
}


bool NetworkRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sample_count)
{
    // Pack the audio samples into a network packet

    //TODO: Register command for streaming blah.

    sf::Packet packet;
    //static const int16_t CMD_SEND_VOICE_COMM = 0x0017; //Hardcoded and pretty nasty.
    //packet <<
    //sf::Packet packet;
    //packet << audioData;
    //packet.append(samples, sampleCount * sizeof(sf::Int16));

    // Send the audio packet to the server
    //return m_socket.send(packet) == sf::Socket::Done;
}

void NetworkRecorder::onStop()
{
    //Stop sending
}
