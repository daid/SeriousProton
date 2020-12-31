#include <i18n.h>
#include <resources.h>

#if defined(_MSC_VER)
#include <cstdlib>
static inline uint32_t bswap32(uint32_t value)
{
    return _byteswap_ulong(value);
}
#elif defined(__GNUC__)
static inline uint32_t bswap32(uint32_t value)
{
    return __builtin_bswap32(value);
}
#else
#error "Unknown compiler - need a byteswap function!"
#endif

static constexpr uint32_t mo_file_magic = 0x950412de;
static constexpr uint32_t mo_file_magic_swapped = 0xde120495;
struct MoHeader
{
    uint32_t version;
    uint32_t count;
    uint32_t offset_origonal;
    uint32_t offset_translated;
};

static i18n::Catalogue main_catalogue;

const string& tr(const string& input)
{
    return main_catalogue.tr(input);
}

const string& tr(const string& context, const string& input)
{
    return main_catalogue.tr(context, input);
}

namespace i18n {

bool load(const string& resource_name)
{
    return main_catalogue.load(resource_name);
}

void reset()
{
    main_catalogue = Catalogue();
}

const string& Catalogue::tr(const string& input)
{
    const auto it = entries.find(input);
    if (it != entries.end())
        return it->second;
    return input;
}

const string& Catalogue::tr(const string& context, const string& input)
{
    const auto cit = context_entries.find(context);
    if (cit == context_entries.end())
        return input;
    const auto it = cit->second.find(input);
    if (it == cit->second.end())
        return input;
    return it->second;
}

bool Catalogue::load(const string& resource_name)
{
    auto stream = getResourceStream(resource_name);
    if (!stream)
        return false;
    uint32_t magic;
    if (stream->read(&magic, sizeof(magic)) != sizeof(magic))
        return false;
    if (magic == mo_file_magic || magic == mo_file_magic_swapped)
    {
        bool swap = magic == mo_file_magic_swapped;
        MoHeader header;
        if (stream->read(&header, sizeof(header)) != sizeof(header))
            return false;
        if (swap)
        {
            header.version = bswap32(header.version);
            header.count = bswap32(header.count);
            header.offset_origonal = bswap32(header.offset_origonal);
            header.offset_translated = bswap32(header.offset_translated);
        }
        std::vector<uint32_t> length_offset_origonal;
        std::vector<uint32_t> length_offset_translated;
        length_offset_origonal.resize(header.count * 2);
        length_offset_translated.resize(header.count * 2);
        stream->seek(header.offset_origonal);
        if (stream->read(length_offset_origonal.data(), length_offset_origonal.size() * sizeof(uint32_t)) != int(length_offset_origonal.size() * sizeof(uint32_t)))
            return false;
        stream->seek(header.offset_translated);
        if (stream->read(length_offset_translated.data(), length_offset_translated.size() * sizeof(uint32_t)) != int(length_offset_translated.size() * sizeof(uint32_t)))
            return false;
        if (swap)
        {
            for(auto& n : length_offset_origonal)
                n = bswap32(n);
            for(auto& n : length_offset_translated)
                n = bswap32(n);
        }
        for(size_t n=0; n<header.count; n++)
        {
            string origonal;
            string translated;
            origonal.resize(length_offset_origonal[n*2]);
            translated.resize(length_offset_translated[n*2]);
            stream->seek(length_offset_origonal[n*2+1]);
            stream->read(&origonal[0], length_offset_origonal[n*2]);
            stream->seek(length_offset_translated[n*2+1]);
            stream->read(&translated[0], length_offset_translated[n*2]);

            if (!origonal.empty())
            {
                int context_index = origonal.find("\x04");
                if (context_index > -1)
                    context_entries[origonal.substr(0, context_index)][origonal.substr(context_index + 1)] = translated;
                else
                    entries[origonal] = translated;
            }
        }
        return true;
    }

    if (resource_name.endswith(".po"))
    {
        string origonal;
        string translated;
        string context;
        string* target = &origonal;

        stream->seek(0);
        while(stream->tell() != stream->getSize())
        {
            string line = stream->readLine().strip();
            string line_contents;
            if (line.endswith("\""))
            {
                if (target == &translated && !line.startswith("\""))
                {
                    if (!origonal.empty() && !translated.empty())
                    {
                        if (!context.empty())
                        {
                            context_entries[context][origonal] = translated;
                            context = "";
                        }
                        else
                        {
                            entries[origonal] = translated;
                        }
                    }
                }
                if (line.startswith("msgid \""))
                {
                    origonal = "";
                    target = &origonal;
                    line_contents = line.substr(7, -1);
                }
                if (line.startswith("msgctxt \""))
                {
                    context = "";
                    target = &context;
                    line_contents = line.substr(9, -1);
                }
                if (line.startswith("msgstr \""))
                {
                    translated = "";
                    target = &translated;
                    line_contents = line.substr(8, -1);
                }
                if (line.startswith("\""))
                {
                    line_contents = line.substr(1, -1);
                }
                for(size_t n=0; n<line_contents.size(); n++)
                {
                    if (line_contents[n] == '\\')
                    {
                        switch(line_contents[++n])
                        {
                        case 'n': *target += '\n'; break;
                        case 'r': *target += '\r'; break;
                        case 't': *target += '\t'; break;
                        default: *target += line_contents[n]; break;
                        }
                    }
                    else
                    {
                        *target += line_contents[n];
                    }
                }
            }
        }
        if (!origonal.empty() && !translated.empty())
        {
            if (!context.empty())
                context_entries[context][origonal] = translated;
            else
                entries[origonal] = translated;
        }
        return true;
    }

    return false;
}

}//!namespace i18n
