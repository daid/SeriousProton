#include <i18n.h>
#include <resources.h>
#include <unordered_map>

static constexpr uint32_t mo_file_magic = 0x950412de;
static constexpr uint32_t mo_file_magic_swapped = 0xde120495;
struct MoHeader
{
    uint32_t version;
    uint32_t count;
    uint32_t offset_origonal;
    uint32_t offset_translated;
};

static std::unordered_map<string, string> translations;

//Translate a string with a loaded translation.
// If no translation was loaded, return the origonal string unmodified.
// There functions are not in the i18n namespace to prevent very long identifiers.
const string& tr(const string& input)
{
    auto it = translations.find(input);
    if (it != translations.end())
        return it->second;
    return input;
}

const string& tr(const char* context, const string& input)
{
    return tr(input);
}

namespace i18n {

bool load(const string& resource_name)
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
            header.version = __builtin_bswap32(header.version);
            header.count = __builtin_bswap32(header.count);
            header.offset_origonal = __builtin_bswap32(header.offset_origonal);
            header.offset_translated = __builtin_bswap32(header.offset_translated);
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
                n = __builtin_bswap32(n);
            for(auto& n : length_offset_translated)
                n = __builtin_bswap32(n);
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

            if (origonal.find("\x04") > -1)
                origonal = origonal.substr(origonal.find("\x04") + 1);

            if (!origonal.empty())
                translations[origonal] = translated;
        }
        return true;
    }

    if (resource_name.endswith(".po"))
    {
        string origonal;
        string translated;
        string* target = &origonal;

        stream->seek(0);
        while(stream->tell() != stream->getSize())
        {
            string line = stream->readLine().strip();
            string line_contents;
            if (line.endswith("\""))
            {
                if (line.startswith("msgid \""))
                {
                    if (!origonal.empty() && !translated.empty())
                        translations[origonal] = translated;
                    origonal = "";
                    target = &origonal;
                    line_contents = line.substr(7, -1);
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
            translations[origonal] = translated;
        return true;
    }

    return false;
}

void reset()
{
    translations.clear();
}

}//!namespace i18n
