#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <io/dataBuffer.h>
#include <stdint.h>
#include "Updatable.h"
#include "stringImproved.h"
#include "ecs/entity.h"
#include "multiplayer_internal.h"

class MultiplayerObject;


#define REGISTER_MULTIPLAYER_ENUM(type) \
    static inline sp::io::DataBuffer& operator << (sp::io::DataBuffer& packet, const type& e) { return packet << int8_t(e); } \
    static inline sp::io::DataBuffer& operator >> (sp::io::DataBuffer& packet, type& mw) { int8_t tmp; packet >> tmp; mw = type(tmp); return packet; }


#define REGISTER_MULTIPLAYER_CLASS(className, name) MultiplayerClassListItem MultiplayerClassListItem ## className(name, createMultiplayerObject<className>);

template<typename T, glm::qualifier Q> static inline sp::io::DataBuffer& operator << (sp::io::DataBuffer& packet, const glm::vec<2, T, Q>& v)
{
    return packet << v.x << v.y;
}
template<typename T, glm::qualifier Q> static inline sp::io::DataBuffer& operator >> (sp::io::DataBuffer& packet, glm::vec<2, T, Q>& v)
{
    return packet >> v.x >> v.y;
}
template<typename T, glm::qualifier Q> static inline sp::io::DataBuffer& operator << (sp::io::DataBuffer& packet, const glm::vec<3, T, Q>& v)
{
    return packet << v.x << v.y << v.z;
}
template<typename T, glm::qualifier Q> static inline sp::io::DataBuffer& operator >> (sp::io::DataBuffer& packet, glm::vec<3, T, Q>& v)
{
    return packet >> v.x >> v.y >> v.z;
}
template<typename T1, typename T2> static inline sp::io::DataBuffer& operator << (sp::io::DataBuffer& packet, const std::pair<T1, T2>& pair)
{
    return packet << pair.first << pair.second;
}
template<typename T1, typename T2> static inline sp::io::DataBuffer& operator >> (sp::io::DataBuffer& packet, std::pair<T1, T2>& pair)
{
    return packet >> pair.first >> pair.second;
}

static inline sp::io::DataBuffer& operator << (sp::io::DataBuffer& packet, const glm::u8vec4& c) { return packet << c.r << c.g << c.b << c.a; } \
static inline sp::io::DataBuffer& operator >> (sp::io::DataBuffer& packet, glm::u8vec4& c) { packet >> c.r >> c.g >> c.b >> c.a; return packet; }

template <typename T> struct multiplayerReplicationFunctions
{
    static bool isChanged(void* data, void* prev_data_ptr);
    static void sendData(void* data, sp::io::DataBuffer& packet)
    {
        T* ptr = (T*)data;
        packet << *ptr;
    }
    static void receiveData(void* data, sp::io::DataBuffer& packet)
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
    static void sendDataVector(void* data, sp::io::DataBuffer& packet)
    {
        std::vector<T>* ptr = (std::vector<T>*)data;
        uint16_t count = ptr->size();
        packet << count;
        for(unsigned int n=0; n<count; n++)
            packet << (*ptr)[n];
    }
    static void receiveDataVector(void* data, sp::io::DataBuffer& packet)
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

template <> void multiplayerReplicationFunctions<sp::ecs::Entity>::sendData(void* data, sp::io::DataBuffer& packet);
template <> void multiplayerReplicationFunctions<sp::ecs::Entity>::receiveData(void* data, sp::io::DataBuffer& packet);

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
        void(*sendFunction)(void* data, sp::io::DataBuffer& packet);
        void(*receiveFunction)(void* data, sp::io::DataBuffer& packet);
        void(*cleanupFunction)(void* prev_data_ptr);
    };
    std::vector<MemberReplicationInfo> memberReplicationInfo;
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
    template <typename T> void registerMemberReplication_(F_PARAM T* member, float update_delay = 0.0f)
    {
        SDL_assert(!replicated);
        SDL_assert(memberReplicationInfo.size() < 0xFFFF);
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
        info.update_timeout = 0.0f;
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

    template <typename T> void registerMemberReplication_(F_PARAM std::vector<T>* member, float update_delay = 0.0f)
    {
        SDL_assert(!replicated);
        SDL_assert(memberReplicationInfo.size() < 0xFFFF);
        MemberReplicationInfo info;
#ifdef DEBUG
        info.name = name;
#endif
        info.ptr = member;
        info.prev_data = reinterpret_cast<std::uint64_t>(new std::vector<T>);
        info.update_delay = update_delay;
        info.update_timeout = 0.0f;
        info.isChangedFunction = &multiplayerReplicationFunctions<T>::isChangedVector;
        info.sendFunction = &multiplayerReplicationFunctions<T>::sendDataVector;
        info.receiveFunction = &multiplayerReplicationFunctions<T>::receiveDataVector;
        info.cleanupFunction = &multiplayerReplicationFunctions<T>::cleanupVector;
        memberReplicationInfo.push_back(info);
    }

    void registerMemberReplication_(F_PARAM glm::vec3* member, float update_delay = 0.0f)
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
                memberReplicationInfo[n].update_timeout = 0.0f;
    }

    void registerCollisionableReplication(float object_significant_range = -1);

    int32_t getMultiplayerId() { return multiplayerObjectId; }
    const string& getMultiplayerClassIdentifier() { return multiplayerClassIdentifier; }
    void sendClientCommand(sp::io::DataBuffer& packet);//Send a command from the client to the server.
    void broadcastServerCommand(sp::io::DataBuffer& packet);//Send a command from the server to all clients.

    virtual void onReceiveClientCommand(int32_t client_id, sp::io::DataBuffer& packet) {} //Got data from a client, handle it.
    virtual void onReceiveServerCommand(sp::io::DataBuffer& packet) {} //Got data from a server, handle it.
private:
    friend class GameServer;
    friend class GameClient;

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

class MultiplayerECSComponentReplicationBase {
public:
    static inline MultiplayerECSComponentReplicationBase* first = nullptr;
    MultiplayerECSComponentReplicationBase* next;
    uint16_t component_index = 0;

    MultiplayerECSComponentReplicationBase() {
        next = first;
        first = this;
        if (next)
            component_index = next->component_index + 1;
    }

    //Called on the server so reused component slots get properly send if an entity is destroyed and reused within the same tick.
    //  And that destroyed entities don't need to send "destroyed component" packets for each component.
    virtual void onEntityDestroyed(uint32_t index) = 0;

    virtual void sendAll(sp::io::DataBuffer& packet) = 0;
    virtual void update(sp::io::DataBuffer& packet) = 0;
    virtual void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) = 0;   //Called from client
    virtual void remove(sp::ecs::Entity entity) = 0;                                //Called from client
};

//Simple replication setup that just sends over the whole component each time it is changed.
template<typename T> class MultiplayerECSComponentReplication : public MultiplayerECSComponentReplicationBase
{
public:
    sp::SparseSet<T> component_copy;

    void onEntityDestroyed(uint32_t index) override
    {
        component_copy.remove(index);
    }

    void sendAll(sp::io::DataBuffer& packet) override
    {
        for(auto [index, data] : sp::ecs::ComponentStorage<T>::storage.sparseset)
        {
            packet << CMD_ECS_SET_COMPONENT << component_index << index << data;
        }
    }

    void update(sp::io::DataBuffer& packet) override
    {
        for(auto [index, data] : sp::ecs::ComponentStorage<T>::storage.sparseset)
        {
            if (!component_copy.has(index) || component_copy.get(index) != data) {
                component_copy.set(index, data);
                packet << CMD_ECS_SET_COMPONENT << component_index << index << data;
            }
        }
        for(auto [index, data] : component_copy)
        {
            if (!sp::ecs::ComponentStorage<T>::storage.sparseset.has(index)) {
                component_copy.remove(index);
                packet << CMD_ECS_DEL_COMPONENT << component_index << index;
            }
        }
    }

    void receive(sp::ecs::Entity entity, sp::io::DataBuffer& packet) override
    {
        T data;
        packet >> data;
        entity.addComponent<T>(data);
    }

    void remove(sp::ecs::Entity entity) override
    {
        entity.removeComponent<T>();
    }
};

#endif//MULTIPLAYER_H
