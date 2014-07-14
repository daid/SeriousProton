#include "networkRecorder.h"

NetworkRecorder::NetworkRecorder(const sf::IpAddress& _host, unsigned short _port){}

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

bool NetworkRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sample_count)
{
    // Pack the audio samples into a network packet

    //TODO: Register command for streaming blah.


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
