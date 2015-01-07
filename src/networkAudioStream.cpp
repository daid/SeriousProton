#include "networkAudioStream.h"

NetworkAudioStream::NetworkAudioStream()
{
    offset = 0;
    has_finished = false;
    initialize(1, 44100);  //soundstream magic
}

void NetworkAudioStream::startListening(unsigned short port)
{
    if(!has_finished)
    {
        if(listener.listen(port) != sf::Socket::Done);
            return;
        if (listener.accept(client) != sf::Socket::Done)
            return;
        play(); //Inherited from soundstream
        receiveLoop();
    } else
    {
        play();
    }
}

bool NetworkAudioStream::onGetData(sf::SoundStream::Chunk& data)
{
    return false;
}
void NetworkAudioStream::onSeek(sf::Time timeOffset)
{

}

void NetworkAudioStream::receiveLoop()
{

}
