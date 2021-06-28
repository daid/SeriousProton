#ifndef NETWORK_AUDIO_RECORDER_H
#define NETWORK_AUDIO_RECORDER_H

#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <stdint.h>
#include <list>
#include <mutex>

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
    std::mutex sample_buffer_mutex;
    std::vector<sf::Int16> sample_buffer;
    OpusEncoder* encoder = nullptr;
    int samples_till_stop = -1;
public:
    NetworkAudioRecorder();
    virtual ~NetworkAudioRecorder();

    void addKeyActivation(sf::Keyboard::Key key, int target_identifier);

protected:
    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sample_count) override;

public:
    virtual void update(float delta) override;

private:
    static constexpr int frame_size = 2880;

    void startSending();
    bool sendAudioPacket();
    void finishSending();
};

#endif //NETWORK_VOICE_RECORDER_H
