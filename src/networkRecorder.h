#ifndef NETWORK_AUDIO_RECORDER_H
#define NETWORK_AUDIO_RECORDER_H

#include <stdint.h>
#include <list>
#include <mutex>

#include "Updatable.h"


struct OpusEncoder;
class NetworkAudioRecorder : public Updatable
{
private:
    struct KeyConfig
    {
        int key;
        int target_identifier;
    };
    std::vector<KeyConfig> keys;
    int active_key_index = -1;
    std::mutex sample_buffer_mutex;
    std::vector<int16_t> sample_buffer;
    OpusEncoder* encoder = nullptr;
    int samples_till_stop = -1;
public:
    NetworkAudioRecorder();
    virtual ~NetworkAudioRecorder();

    void addKeyActivation(int key, int target_identifier);

public:
    virtual void update(float delta) override;

private:
    static void SDLCallback(void* userdata, uint8_t* stream, int len);
    void onProcessSamples(const int16_t* samples, std::size_t sample_count);
    static constexpr int frame_size = 2880;

    void startSending();
    bool sendAudioPacket();
    void finishSending();
};

#endif //NETWORK_VOICE_RECORDER_H
