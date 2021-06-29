#ifndef SP2_AUDIO_SOUND_H
#define SP2_AUDIO_SOUND_H

#include <stdint.h>
#include <glm/vec3.hpp>
#include "audio/source.h"
#include "stringImproved.h"

namespace sp {
namespace audio {

class Sound;
class SoundPlayback : public Source
{
public:
    void play(const Sound& sound, bool loop);

    void setVolume(float volume);
    void setPitch(float pitch);
protected:
    virtual void onMixSamples(int16_t* stream, int sample_count) override;

private:
    const Sound* sound = nullptr;
    bool loop = false;
    float pitch = 1.0f;
    float volume = 1.0f;
    float index = 0.0f;
};

class Sound
{
public:
    Sound(const string& resource_name);

    float getDuration();
    int getChannelCount();

private:
    int channels = 0;
    int samplerate = 0;
    std::vector<int16_t> samples;

    friend class SoundPlayback;
};

}//namespace audio
}//namespace sp

#endif//SP2_AUDIO_SOUND_H
