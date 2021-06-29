#ifndef SP2_AUDIO_MUSIC_H
#define SP2_AUDIO_MUSIC_H

#include "audio/source.h"
#include "stringImproved.h"
#include <string_view>

namespace sp {
namespace audio {

class Music : public Source
{
public:
    bool open(const string& name, bool loop);

    void setVolume(float volume); //range: 0-100
protected:
    virtual void onMixSamples(int16_t* stream, int sample_count) override;

private:
    unsigned int sample_rate;
    unsigned int length;
    float volume = 1.0f;
    std::vector<uint8_t> file_data;
    bool loop = false;
    void* vorbis = nullptr;
};

}//namespace audio
}//namespace sp

#endif//SP2_AUDIO_MUSIC_H
