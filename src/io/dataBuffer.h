#ifndef SP2_IO_DATABUFFER_H
#define SP2_IO_DATABUFFER_H

#include <stringImproved.h>
#include <vectorUtils.h>
#include <string.h>


namespace sp {
namespace io {

class DataBuffer
{
public:
    DataBuffer()
    : read_index(0)
    {
    }
    
    DataBuffer(DataBuffer&& b)
    : buffer(std::move(b.buffer)), read_index(b.read_index)
    {
    }
    
    template<typename... ARGS> explicit DataBuffer(const ARGS&... args)
    : DataBuffer()
    {
        write(args...);
    }
    
    void operator=(std::vector<uint8_t>&& data)
    {
         buffer = std::move(data);
         read_index = 0;
    }

    void clear()
    {
        buffer.clear();
        read_index = 0;
    }
    
    const void* getData() const
    {
        return buffer.data();
    }
    
    unsigned int getDataSize() const
    {
        return buffer.size();
    }

    void appendRaw(const void* ptr, size_t size)
    {
        buffer.resize(buffer.size() + size);
        memcpy(buffer.data() + buffer.size() - size, ptr, size);
    }
    
    template<typename T, typename... ARGS> void write(const T& value, const ARGS&... args)
    {
        write(value);
        write(args...);
    }
    
    void write(bool b)
    {
        buffer.push_back(b ? 1 : 0);
    }
    
    void write(uint8_t i)
    {
        buffer.push_back(i);
    }

    void write(int8_t i)
    {
        buffer.push_back(i);
    }

    void write(int16_t i)
    {
        writeVLQs(i);
    }

    void write(int32_t i)
    {
        writeVLQs(i);
    }

    void write(uint16_t i)
    {
        writeVLQu(i);
    }
    
    void write(uint32_t i)
    {
        writeVLQu(i);
    }

    void write(const float f)
    {
        size_t idx = buffer.size();
        buffer.resize(idx + sizeof(f));
        memcpy(&buffer[idx], &f, sizeof(f));
    }

    void write(const double f)
    {
        size_t idx = buffer.size();
        buffer.resize(idx + sizeof(f));
        memcpy(&buffer[idx], &f, sizeof(f));
    }

    template<typename T> void write(const sf::Vector2<T>& v)
    {
        write(v.x);
        write(v.y);
    }

    template<typename T> void write(const sf::Vector3<T>& v)
    {
        write(v.x);
        write(v.y);
        write(v.z);
    }

    void write(const string& s)
    {
        write(uint32_t(s.length()));
        size_t idx = buffer.size();
        buffer.resize(idx + s.length());
        memcpy(&buffer[idx], &s[0], s.length());
    }

    void write(const char* s)
    {
        uint32_t len = strlen(s);
        write(len);
        size_t idx = buffer.size();
        buffer.resize(idx + len);
        memcpy(&buffer[idx], s, len);
    }

    template<class T, class=typename std::enable_if<std::is_enum<T>::value>::type>
    void write(T enum_value) { write(uint16_t(enum_value)); }

    void write(const DataBuffer& other)
    {
        size_t idx = buffer.size();
        buffer.resize(idx + other.buffer.size());
        memcpy(&buffer[idx], other.buffer.data(), other.buffer.size());
    }
    
    template<typename T, typename... ARGS> void read(T& value, ARGS&... args)
    {
        read(value);
        read(args...);
    }

    void read(bool& b)
    {
        if (read_index >= buffer.size()) { b = false; return; }
        b = buffer[read_index++];
    }
    
    void read(uint8_t& i)
    {
        if (read_index >= buffer.size()) { i = 0; return; }
        i = buffer[read_index++];
    }

    void read(int8_t& i)
    {
        if (read_index >= buffer.size()) { i = 0; return; }
        i = buffer[read_index++];
    }

    void read(int16_t& i)
    {
        i = readVLQs();
    }
    void read(int32_t& i)
    {
        i = readVLQs();
    }
    
    void read(uint16_t& i)
    {
        i = readVLQu();
    }
    
    void read(uint32_t& i)
    {
        i = readVLQu();
    }

    void read(float& f)
    {
        if (read_index + sizeof(f) > buffer.size()) { f = 0; return; }
        memcpy(&f, &buffer[read_index], sizeof(f));
        read_index += sizeof(f);
    }

    void read(double& f)
    {
        if (read_index + sizeof(f) > buffer.size()) { f = 0; return; }
        memcpy(&f, &buffer[read_index], sizeof(f));
        read_index += sizeof(f);
    }

    template<typename T> void read(sf::Vector2<T>& v)
    {
        read(v.x);
        read(v.y);
    }

    template<typename T> void read(sf::Vector3<T>& v)
    {
        read(v.x);
        read(v.y);
        read(v.z);
    }

    void read(string& s)
    {
        uint32_t len = 0;
        read(len);
        if (read_index + len > buffer.size()) return;
        s.assign(reinterpret_cast<char*>(&buffer[read_index]), len);
        read_index += len;
    }

    template<class T, class=typename std::enable_if<std::is_enum<T>::value>::type>
    void read(T& enum_value) { uint16_t v=0; read(v); enum_value = T(v); }

    size_t available()
    {
        return buffer.size() - read_index;
    }

    DataBuffer& operator <<(bool data) { write(data); return *this; }
    DataBuffer& operator <<(int8_t data) { write(data); return *this; }
    DataBuffer& operator <<(uint8_t data) { write(data); return *this; }
    DataBuffer& operator <<(int16_t data) { write(data); return *this; }
    DataBuffer& operator <<(uint16_t data) { write(data); return *this; }
    DataBuffer& operator <<(int32_t data) { write(data); return *this; }
    DataBuffer& operator <<(uint32_t data) { write(data); return *this; }
    DataBuffer& operator <<(float data) { write(data); return *this; }
    DataBuffer& operator <<(double data) { write(data); return *this; }
    DataBuffer& operator <<(const string& data) { write(data); return *this; }

    DataBuffer& operator >>(bool& data) { read(data); return *this; }
    DataBuffer& operator >>(int8_t& data) { read(data); return *this; }
    DataBuffer& operator >>(uint8_t& data) { read(data); return *this; }
    DataBuffer& operator >>(int16_t& data) { read(data); return *this; }
    DataBuffer& operator >>(uint16_t& data) { read(data); return *this; }
    DataBuffer& operator >>(int32_t& data) { read(data); return *this; }
    DataBuffer& operator >>(uint32_t& data) { read(data); return *this; }
    DataBuffer& operator >>(float& data) { read(data); return *this; }
    DataBuffer& operator >>(double& data) { read(data); return *this; }
    DataBuffer& operator >>(string& data) { read(data); return *this; }
private:
    void writeVLQu(uint32_t v) {
        if (v >= (1 << 28))
            buffer.push_back((v >> 28) | 0x80);
        if (v >= (1 << 21))
            buffer.push_back((v >> 21) | 0x80);
        if (v >= (1 << 14))
            buffer.push_back((v >> 14) | 0x80);
        if (v >= (1 << 7))
            buffer.push_back((v >> 7) | 0x80);
        buffer.push_back((v & 0x7F));
    }

    void writeVLQs(int32_t v) {
        if (v < 0)
            writeVLQu((uint32_t(-v) << 1) | 1);
        else
            writeVLQu((uint32_t(v) << 1));
    }

    uint32_t readVLQu()
    {
        uint32_t result{0};
        uint8_t u;
        do
        {
            result <<= 7;
            if (read_index >= buffer.size()) { return result; }
            u = buffer[read_index++];
            result |= u & 0x7F;
        } while(u & 0x80);
        return result;
    }

    int32_t readVLQs()
    {
        uint32_t v = readVLQu();
        if (v & 1) return -int32_t(v >> 1);
        return int32_t(v >> 1);
    }

    std::vector<uint8_t> buffer;
    size_t read_index;
};

}//namespace io
}//namespace sp


#endif//SP2_IO_DATABUFFER_H
