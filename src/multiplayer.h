#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <SFML/Network.hpp>
#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <bitset>
#include "Updatable.h"
#include "stringImproved.h"

class MultiplayerObject;


#define REGISTER_MULTIPLAYER_ENUM(type) \
    static inline sf::Packet& operator << (sf::Packet& packet, const type& e) { return packet << int8_t(e); } \
    static inline sf::Packet& operator >> (sf::Packet& packet, type& mw) { int8_t tmp; packet >> tmp; mw = type(tmp); return packet; }


#define REGISTER_MULTIPLAYER_CLASS(className, name) MultiplayerClassListItem MultiplayerClassListItem ## className(name, createMultiplayerObject<className>);

template<typename T> static inline sf::Packet& operator << (sf::Packet& packet, const sf::Vector2<T>& v)
{
    return packet << v.x << v.y;
}
template<typename T> static inline sf::Packet& operator >> (sf::Packet& packet, sf::Vector2<T>& v)
{
    return packet >> v.x >> v.y;
}
template<typename T> static inline sf::Packet& operator << (sf::Packet& packet, const sf::Vector3<T>& v)
{
    return packet << v.x << v.y << v.z;
}
template<typename T> static inline sf::Packet& operator >> (sf::Packet& packet, sf::Vector3<T>& v)
{
    return packet >> v.x >> v.y >> v.z;
}
template<typename T1, typename T2> static inline sf::Packet& operator << (sf::Packet& packet, const std::pair<T1, T2>& pair)
{
    return packet << pair.first << pair.second;
}
template<typename T1, typename T2> static inline sf::Packet& operator >> (sf::Packet& packet, std::pair<T1, T2>& pair)
{
    return packet >> pair.first >> pair.second;
}

static inline sf::Packet& operator << (sf::Packet& packet, const sf::Color& c) { return packet << c.r << c.g << c.b << c.a; } \
static inline sf::Packet& operator >> (sf::Packet& packet, sf::Color& c) { packet >> c.r >> c.g >> c.b >> c.a; return packet; }

template <typename T> struct multiplayerReplicationFunctions
{
    static bool isChanged(void* data, void* prev_data_ptr);
    static void sendData(void* data, sf::Packet& packet)
    {
        T* ptr = (T*)data;
        packet << *ptr;
    }
    static void receiveData(void* data, sf::Packet& packet)
    {
        T* ptr = (T*)data;
        packet >> *ptr;
    }

    static bool isChangedVector(void* data, void* prev_data_ptr)
    {
        std::vector<T>* ptr = (std::vector<T>*)data;
        std::vector<T>* prev_data = *(std::vector<T>**)prev_data_ptr;
        bool changed = false;
        if (prev_data->size() != ptr->size())
        {
            changed = true;
        }else{
            for(unsigned int n=0; n<ptr->size(); n++)
            {
                if ((*prev_data)[n] != (*ptr)[n])
                {
                    changed = true;
                    break;
                }
            }
        }
        if (changed)
        {
            prev_data->resize(ptr->size());
            for(unsigned int n=0; n<ptr->size(); n++)
               (*prev_data)[n] = (*ptr)[n];
            return true;
        }
        return false;
    }
    static void sendDataVector(void* data, sf::Packet& packet)
    {
        std::vector<T>* ptr = (std::vector<T>*)data;
        uint16_t count = ptr->size();
        packet << count;
        for(unsigned int n=0; n<count; n++)
            packet << (*ptr)[n];
    }
    static void receiveDataVector(void* data, sf::Packet& packet)
    {
        std::vector<T>* ptr = (std::vector<T>*)data;
        uint16_t count;
        packet >> count;
        ptr->resize(count);
        for(unsigned int n=0; n<count; n++)
            packet >> (*ptr)[n];
    }
    static void cleanupVector(void* prev_data_ptr)
    {
        std::vector<T>* prev_data = *(std::vector<T>**)prev_data_ptr;
        delete prev_data;
    }
};

template <typename T>
bool multiplayerReplicationFunctions<T>::isChanged(void* data, void* prev_data_ptr)
{
    T* ptr = (T*)data;
    T* prev_data = (T*)prev_data_ptr;
    if (*ptr != *prev_data)
    {
        *prev_data = *ptr;
        return true;
    }
    return false;
}

template <> bool multiplayerReplicationFunctions<string>::isChanged(void* data, void* prev_data_ptr);

namespace replication
{
    class ControlBlock;

    /*! a replicated item - base class that may be anything. */
    class Item
    {
    public:
        /*! Allow fine-grained access control to parts of the internal.
        * 
        * Used to grant access to some classes, while still being to guarantee class invariants.
        */
        class Key final {
            friend class ControlBlock;
            constexpr Key() = default;
        };

        explicit Item(ControlBlock& controller);

        virtual void send(const ControlBlock&, sf::Packet&) = 0;
        virtual void receive(const ControlBlock&, sf::Packet&) = 0;

        // 
        ControlBlock& getController(Key) const;

        uint16_t getId(Key) const;
        void setId(Key, uint16_t id);
    protected:
        ControlBlock& getController() const;
    private:
        ControlBlock& controller;
        uint16_t replication_id{};
    };

    class ControlBlock
    {
    public:
        class Key final {
            friend class ReplicatedBase;
            constexpr Key() = default;
        };

        void add(Item& item);
        bool isDirty(const Item& item) const;
        void setDirty(const Item& item);
        size_t send(sf::Packet&, bool everything);
        bool handles(uint16_t net_id) const;
        void receive(uint16_t net_id, sf::Packet&);
    private:
        static constexpr uint16_t controlled_flag{ 0x8000 };

        bool isDirty(size_t) const;
        void setDirty(size_t);
        void resetDirty();
        std::vector<size_t> dirty;
        std::vector<Item*> items;
    };
} // ns replication

//In between class that handles all the nasty synchronization of objects between server and client.
//I'm assuming that it should be a pure virtual class though.
class MultiplayerObject : public virtual PObject
{
    constexpr static int32_t noId = 0xFFFFFFFF;
    int32_t multiplayerObjectId;
    bool replicated;
    bool on_server;
    string multiplayerClassIdentifier;

    struct MemberReplicationInfo
    {
#ifdef DEBUG
        const char* name;
#endif
        void* ptr;
        uint64_t prev_data;
        float update_delay;
        float update_timeout;

        bool(*isChangedFunction)(void* data, void* prev_data_ptr);
        void(*sendFunction)(void* data, sf::Packet& packet);
        void(*receiveFunction)(void* data, sf::Packet& packet);
        void(*cleanupFunction)(void* prev_data_ptr);
    };
    std::vector<MemberReplicationInfo> memberReplicationInfo;
    std::unique_ptr<replication::ControlBlock> replicationControl;
public:
    MultiplayerObject(string multiplayerClassIdentifier);
    virtual ~MultiplayerObject();

    bool isServer() { return on_server; }
    bool isClient() { return !on_server; }

#ifdef DEBUG
#define STRINGIFY(n) #n
#define registerMemberReplication(member, ...) registerMemberReplication_(STRINGIFY(member), member , ## __VA_ARGS__ )
#define F_PARAM const char* name,
#else
#define registerMemberReplication(member, ...) registerMemberReplication_(member , ## __VA_ARGS__ )
#define F_PARAM
#endif
    template <typename T> void registerMemberReplication_(F_PARAM T* member, float update_delay = 0.0)
    {
        assert(!replicated);
        assert(memberReplicationInfo.size() < 0xFFFF);
        MemberReplicationInfo info;
#ifdef DEBUG
        info.name = name;
#endif
        info.ptr = member;
        static_assert(
                std::is_same<T, string>::value ||
                (
                        std::is_default_constructible<T>::value &&
                        std::is_trivially_destructible<T>::value &&
                        sizeof(T) <= 8
                ),
                "T must be a string or must be a default constructible, trivially destructible and with a size of at most 64bit"
        );
        init_prev_data<T>(info);
        info.update_delay = update_delay;
        info.update_timeout = 0.0;
        info.isChangedFunction = &multiplayerReplicationFunctions<T>::isChanged;
        info.sendFunction = &multiplayerReplicationFunctions<T>::sendData;
        info.receiveFunction = &multiplayerReplicationFunctions<T>::receiveData;
        info.cleanupFunction = NULL;
        memberReplicationInfo.push_back(info);
#ifdef DEBUG
        if (multiplayerReplicationFunctions<T>::isChanged(member, &info.prev_data))
        {
        }
#endif
    }

    template <typename T> void registerMemberReplication_(F_PARAM std::vector<T>* member, float update_delay = 0.0)
    {
        assert(!replicated);
        assert(memberReplicationInfo.size() < 0xFFFF);
        MemberReplicationInfo info;
#ifdef DEBUG
        info.name = name;
#endif
        info.ptr = member;
        info.prev_data = reinterpret_cast<std::uint64_t>(new std::vector<T>);
        info.update_delay = update_delay;
        info.update_timeout = 0.0;
        info.isChangedFunction = &multiplayerReplicationFunctions<T>::isChangedVector;
        info.sendFunction = &multiplayerReplicationFunctions<T>::sendDataVector;
        info.receiveFunction = &multiplayerReplicationFunctions<T>::receiveDataVector;
        info.cleanupFunction = &multiplayerReplicationFunctions<T>::cleanupVector;
        memberReplicationInfo.push_back(info);
    }

    void registerMemberReplication_(F_PARAM sf::Vector3f* member, float update_delay = 0.0)
    {
        registerMemberReplication(&member->x, update_delay);
        registerMemberReplication(&member->y, update_delay);
        registerMemberReplication(&member->z, update_delay);
    }

    void updateMemberReplicationUpdateDelay(void* data, float update_delay)
    {
        for(unsigned int n=0; n<memberReplicationInfo.size(); n++)
            if (memberReplicationInfo[n].ptr == data)
                memberReplicationInfo[n].update_delay = update_delay;
    }

    void forceMemberReplicationUpdate(void* data)
    {
        for(unsigned int n=0; n<memberReplicationInfo.size(); n++)
            if (memberReplicationInfo[n].ptr == data)
                memberReplicationInfo[n].update_timeout = 0.0;
    }

    void registerCollisionableReplication(float object_significant_range = -1);

    int32_t getMultiplayerId() { return multiplayerObjectId; }
    const string& getMultiplayerClassIdentifier() { return multiplayerClassIdentifier; }
    void sendClientCommand(sf::Packet& packet);//Send a command from the client to the server.
    void broadcastServerCommand(sf::Packet& packet);//Send a command from the server to all clients.

    virtual void onReceiveClientCommand(int32_t client_id, sf::Packet& packet) {} //Got data from a client, handle it.
    virtual void onReceiveServerCommand(sf::Packet& packet) {} //Got data from a server, handle it.
    replication::ControlBlock& getReplicationController() const { return *replicationControl; }
private:
    friend class GameServer;
    friend class GameClient;
    friend class MultiplayerStaticReplicationBase;

    template <typename T>
    static inline
    typename std::enable_if<!std::is_same<T, string>::value>::type
    init_prev_data(MemberReplicationInfo& info) {
        new (&info.prev_data) T{};
    }

    template <typename T>
    static inline
    typename std::enable_if<std::is_same<T, string>::value>::type
    init_prev_data(MemberReplicationInfo& info) {
        info.prev_data = 0;
    }
};

namespace replication
{
    template<typename T>
    class Field : public Item
    {
    public:
        template<typename... Args>
        Field(MultiplayerObject* parent, Args&&... params)
            : Item{ parent->getReplicationController() }
            , value(std::forward<Args>(params)...)
        {}

        const T& get() const { return value; }
        operator const T& () const { return get(); }

        Field& operator =(const Field& other)
        {
            return *this = other.get();
        }

        // RMW ops.
        template<typename U>
        Field& operator=(const U& new_value)
        {
            if (value != new_value)
            {
                value = new_value;
                getController().setDirty(*this);
            }
            return *this;
        }

        template<typename U>
        Field& operator-=(U&& update)
        {
            auto previous = value;
            value -= std::forward<U>(update);
            if (previous != value)
                getController().setDirty(*this);

            return *this;
        }

        template<typename U>
        Field& operator+=(U&& update)
        {
            auto previous = value;
            value += std::forward<U>(update);
            if (previous != value)
                getController().setDirty(*this);

            return *this;
        }

        template<typename U>
        Field& operator*=(U&& update)
        {
            auto previous = value;
            value *= std::forward<U>(update);
            if (previous != value)
                getController().setDirty(*this);

            return *this;
        }

        template<typename U>
        Field& operator/=(U&& update)
        {
            auto previous = value;
            value /= std::forward<U>(update);
            if (previous != value)
                getController().setDirty(*this);

            return *this;
        }

        void send(const ControlBlock&, sf::Packet& packet) override
        {
            packet << get();
        }

        void receive(const ControlBlock&, sf::Packet& packet) override
        {
            packet >> *this;
        }
    private:
        T value;
    };

    template<typename T>
    sf::Packet& operator >> (sf::Packet& packet, Field<T>& field)
    {
        T value{};
        packet >> value;
        field = value;
        return packet;
    }
} // ns replication

typedef MultiplayerObject* (*CreateMultiplayerObjectFunction)();

class MultiplayerClassListItem;
extern MultiplayerClassListItem* multiplayerClassListStart;

class MultiplayerClassListItem
{
public:
    string name;
    CreateMultiplayerObjectFunction func;
    MultiplayerClassListItem* next;

    MultiplayerClassListItem(string name, CreateMultiplayerObjectFunction func)
    {
        this->name = name;
        this->func = func;
        this->next = multiplayerClassListStart;
        multiplayerClassListStart = this;
    }
};

template<class T> MultiplayerObject* createMultiplayerObject()
{
    return new T();
}

#endif//MULTIPLAYER_H
