#include "audio/source.h"
#include "logging.h"

#include <SDL.h>
#include <mutex>
#include <array>
#include <string.h>

namespace sp {
namespace audio {


static std::recursive_mutex source_list_mutex;
static Source* source_list_start = nullptr;

static SDL_AudioDeviceID audio_device;

class MySDLAudioInterface {
public:
    static void Callback(void* userdata, uint8_t* stream, int len)
    {
        Source::onAudioCallback(reinterpret_cast<int16_t*>(stream), len/2);
    }
};

Source::~Source()
{
    stop();
}

void Source::start()
{
    std::lock_guard<std::recursive_mutex> guard(source_list_mutex);
    if (active)
        return;

    active = true;
    next = source_list_start;
    if (next)
        next->previous = this;
    previous = nullptr;
    source_list_start = this;
}

bool Source::isPlaying()
{
    return active;
}

void Source::stop()
{
    std::lock_guard<std::recursive_mutex> guard(source_list_mutex);
    if (!active)
        return;

    active = false;
    if (source_list_start == this)
    {
        source_list_start = next;
        if (source_list_start)
            source_list_start->previous = nullptr;
    }
    else
    {
        previous->next = next;
    }
    if (next)
        next->previous = previous;
}

void Source::startAudioSystem()
{
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 2048;
    want.callback = &MySDLAudioInterface::Callback;
    audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (audio_device == 0)
    {
        LOG(Error, "Failed to open audio device: ", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(audio_device, 0);
    }
}

void Source::onAudioCallback(int16_t* stream, int sample_count)
{
    memset(stream, 0, sample_count * sizeof(int16_t));
    for(Source* source = source_list_start; source; source = source->next)
        source->onMixSamples(stream, sample_count);
}

}//namespace audio
}//namespace sp
