#include "audio/source.h"
#include "logging.h"

#include <mutex>
#include <string.h>

#include <SFML/Audio/SoundStream.hpp>

namespace sp {
namespace audio {


static std::mutex source_list_mutex;
static Source* source_list_start = nullptr;

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


Source::~Source()
{
    stop();
}

void Source::start()
{
    if (active)
        return;

    std::lock_guard<std::mutex> guard(source_list_mutex);
    active = true;
    next = source_list_start;
    if (next)
        next->previous = this;
    else if (sfml_stream)
        sfml_stream->play();
    previous = nullptr;
    source_list_start = this;
}

bool Source::isPlaying()
{
    return active;
}

void Source::stop()
{
    if (!active)
        return;
    
    std::lock_guard<std::mutex> guard(source_list_mutex);
    active = false;
    if (source_list_start == this)
    {
        source_list_start = next;
        if (source_list_start)
            source_list_start->previous = nullptr;
        else if (sfml_stream)
            sfml_stream->stop();
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
    sfml_stream = new MySFMLStream();

    //Check if any audio source was started before we opened our audio device.
    if (source_list_start)
        sfml_stream->play();
}

void Source::onAudioCallback(int16_t* stream, int sample_count)
{
    memset(stream, 0, sample_count * sizeof(int16_t));
    for(Source* source = source_list_start; source; source = source->next)
        source->onMixSamples(stream, sample_count);
}

}//namespace audio
}//namespace sp
