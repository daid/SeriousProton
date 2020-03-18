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
    struct KeyConfig
    {
        sf::Keyboard::Key key;
        int target_identifier;
    };
    std::vector<KeyConfig> keys;
    int active_key_index = -1;
    sf::Mutex sample_buffer_mutex;
    std::vector<sf::Int16> sample_buffer;
    OpusEncoder* encoder = nullptr;
public:
    NetworkAudioRecorder();
    virtual ~NetworkAudioRecorder();

    void addKeyActivation(sf::Keyboard::Key key, int target_identifier);

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
