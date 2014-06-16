#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H


#include <SFML/Network.hpp>
#include "Updatable.h"
#include "stringImproved.h"

static const int defaultServerPort = 35666;
static const int multiplayerMagicNumber = 0x2fab3f0f;

class MultiplayerObject;
class GameServer;
class GameClient;

extern P<GameServer> gameServer;
extern P<GameClient> gameClient;

class GameServer : public Updatable
{
    sf::Clock updateTimeClock;
    sf::UdpSocket broadcastListenSocket;
    sf::TcpListener listenSocket;
    sf::SocketSelector selector;
    string serverName;
    int versionNumber;
    int sendDataCounter;
    float sendDataRate;
    float lastGameSpeed;
    
    struct ClientInfo
    {
        sf::TcpSocket* socket;
        int32_t clientId;
    };
    int32_t nextClientId;
    std::vector<ClientInfo> clientList;

    int32_t nextObjectId;
    std::map<int32_t, P<MultiplayerObject> > objectMap;
public:
    GameServer(string serverName, int versionNumber, int listenPort = defaultServerPort);
    
    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);
    float getSendDataRate() { return sendDataRate; }
private:
    void registerObject(P<MultiplayerObject> obj);
    void sendAll(sf::Packet& packet);
    
    void genenerateCreatePacketFor(P<MultiplayerObject> obj, sf::Packet& packet);
    void genenerateDeletePacketFor(int32_t id, sf::Packet& packet);
    
    friend class MultiplayerObject;
public:
    virtual void onNewClient(int32_t clientId) {}
    virtual void onDisconnectClient(int32_t clientId) {}
};

class GameClient : public Updatable
{
    sf::TcpSocket socket;
    std::map<int32_t, P<MultiplayerObject> > objectMap;
    int32_t clientId;
public:
    GameClient(sf::IpAddress server, int portNr = defaultServerPort);
    
    P<MultiplayerObject> getObjectById(int32_t id);
    virtual void update(float delta);
    
    int32_t getClientId() { return clientId; }

    friend class MultiplayerObject;
};

class ServerScanner : public Updatable
{
    sf::Clock updateTimeClock;
    int serverPort;
    sf::UdpSocket socket;
    float broadcastDelay;

public:
    struct ServerInfo
    {
        sf::IpAddress address;
        float timeout;
        string name;
    };
private:

    std::vector<struct ServerInfo> serverList;
    int versionNumber;
    const static float serverTimeout = 30.0;
public:

    ServerScanner(int versionNumber, int serverPort = defaultServerPort);
    
    virtual void update(float delta);
    
    std::vector<ServerInfo> getServerList();
};

template <typename T> struct multiplayerReplicationFunctions
{
    static bool isChanged(void* data, void* prev_data_ptr)
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
};
template <> bool multiplayerReplicationFunctions<string>::isChanged(void* data, void* prev_data_ptr);

class MultiplayerObject : public virtual PObject
{
    const static int32_t noId = 0xFFFFFFFF;
    int32_t multiplayerObjectId;
    bool replicated;
    bool on_server;
    string multiplayerClassIdentifier;
    
    struct MemberReplicationInfo
    {
        void* ptr;
        int64_t prev_data;
        float update_delay;
        float update_timeout;
        
        bool(*isChangedFunction)(void* data, void* prev_data_ptr);
        void(*sendFunction)(void* data, sf::Packet& packet);
        void(*receiveFunction)(void* data, sf::Packet& packet);
    };
    std::vector<MemberReplicationInfo> memberReplicationInfo;
public:
    MultiplayerObject(string multiplayerClassIdentifier);
    
    bool isServer() { return on_server; }
    bool isClient() { return !on_server; }
    
    template <typename T> void registerMemberReplication(T* member, float update_delay = 0.0)
    {
        assert(!replicated);
        assert(memberReplicationInfo.size() < 0xFFFF);
        assert(sizeof(T) <= sizeof(MemberReplicationInfo::prev_data));
        MemberReplicationInfo info;
        info.ptr = member;
        info.prev_data = -1;
        info.update_delay = update_delay;
        info.update_timeout = 0.0;
        info.isChangedFunction = &multiplayerReplicationFunctions<T>::isChanged;
        info.sendFunction = &multiplayerReplicationFunctions<T>::sendData;
        info.receiveFunction = &multiplayerReplicationFunctions<T>::receiveData;
        memberReplicationInfo.push_back(info);
    }
    
    void registerCollisionableReplication();
    
    int32_t getMultiplayerId() { return multiplayerObjectId; }
    void sendCommand(sf::Packet& packet);//Send a command from the client to the server.
    
    virtual void onReceiveCommand(int32_t clientId, sf::Packet& packet) {} //Got data from a client, handle it.
private:
    friend class GameServer;
    friend class GameClient;
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
    P<GameServer> tmp = gameServer;
    gameServer = NULL;
    MultiplayerObject* ret = new T();
    gameServer = tmp;
    return ret;
}
#define REGISTER_MULTIPLAYER_CLASS(className, name) MultiplayerClassListItem MultiplayerClassListItem ## className(name, createMultiplayerObject<className>);

#endif//MULTIPLAYER_H
