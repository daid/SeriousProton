#ifndef NETWORK_AUDIO_RECORDER_H
#define NETWORK_AUDIO_RECORDER_H

#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <stdint.h>
#include <list>

#include "Updatable.h"


struct OpusEncoder;
class NetworkAudioRecorder : private sf::SoundRecorder, public Updatable
{
private:
    sf::Keyboard::Key activation_key;
    sf::Mutex sample_buffer_mutex;
    std::vector<sf::Int16> sample_buffer;
    int32_t target_identifier;
    OpusEncoder* encoder = nullptr;
public:
    NetworkAudioRecorder();
    virtual ~NetworkAudioRecorder();

    void setKeyActivation(sf::Keyboard::Key key);

protected:
    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sample_count) override;

public:
    virtual void update(float delta) override;

private:
    void startSending();
    bool sendAudioPacket();
    void finishSending();
};

#endif //NETWORK_VOICE_RECORDER_H
