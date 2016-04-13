#include "networkAudioStream.h"

NetworkAudioStream::NetworkAudioStream()
{
    sample_rate = 44100;
    
    //Reserve 10 seconds of playback in our buffers.
    samples.reserve(sample_rate * 10);
    playing_samples.reserve(sample_rate * 10);
    
    initialize(1, sample_rate);  //initialize the sound stream on 1 channel with 44100Hz. This needs to match the data in NetworkAudioRecorder
}

bool NetworkAudioStream::onGetData(sf::SoundStream::Chunk& data)
{
    sf::Lock lock(samples_lock);    //Get exclusive access to the samples vector.
    //Copy all new samples to the playback buffer. And clear our sample buffer.
    playing_samples = samples;
    samples.clear();
    
    data.samples = playing_samples.data();
    data.sampleCount = playing_samples.size();
    
    //Stop playback if the buffer is empty.
    return data.sampleCount > 0;
}

void NetworkAudioStream::onSeek(sf::Time timeOffset)
{
    //We cannot seek in the network audio stream.
}

void NetworkAudioStream::receivedSamplesFromNetwork(const std::vector<sf::Int16>& samples)
{
    sf::Lock lock(samples_lock);
    this->samples.insert(this->samples.end(), samples.begin(), samples.end());
    
    if (this->samples.size() > sample_rate && getStatus() == sf::SoundSource::Stopped) //Start playback when there is 1 second of data
    {
        play();
    }
}
