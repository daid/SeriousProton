#include "audio/source.h"
#include "logging.h"

#include <mutex>
#include <array>
#include <string.h>

namespace sp {
namespace audio {


static std::recursive_mutex source_list_mutex;
static Source* source_list_start = nullptr;

/*
class MySFMLStream : public sf::SoundStream
{
public:
    MySFMLStream()
    {
        initialize(2, 44100);
    }

    virtual bool onGetData(Chunk& data) override
    {
        data.samples = buffer.data();
        data.sampleCount = buffer.size();
        std::lock_guard<std::recursive_mutex> guard(source_list_mutex);
        Source::onAudioCallback(buffer.data(), buffer.size());
        return true;
    }

    virtual void onSeek(sf::Time timeOffset) override
    {
    }
private:
    std::array<int16_t, 1024> buffer;
};
static MySFMLStream* sfml_stream;
*/

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
#warning SDL2 TODO
    //sfml_stream = new MySFMLStream();
    //sfml_stream->play();
}

void Source::onAudioCallback(int16_t* stream, int sample_count)
{
    memset(stream, 0, sample_count * sizeof(int16_t));
    for(Source* source = source_list_start; source; source = source->next)
        source->onMixSamples(stream, sample_count);
}

}//namespace audio
}//namespace sp
