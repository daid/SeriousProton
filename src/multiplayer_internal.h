#ifndef MULTIPLAYER_INTERNAL_H
#define MULTIPLAYER_INTERNAL_H

//Definitions shared between different SeriousProton multiplayer objects, but do not need to be exported outside the engine.
typedef uint16_t command_t;
static const command_t CMD_CREATE = 0x0001;
static const command_t CMD_UPDATE_VALUE = 0x0002;
static const command_t CMD_DELETE = 0x0003;
static const command_t CMD_SET_CLIENT_ID = 0x0004;
static const command_t CMD_SET_GAME_SPEED = 0x0005;
static const command_t CMD_CLIENT_COMMAND = 0x0006;
static const command_t CMD_ALIVE = 0x0007;
static const command_t CMD_REQUEST_AUTH = 0x0009;
static const command_t CMD_NEW_PROXY_CLIENT = 0x000a;
static const command_t CMD_SET_PROXY_CLIENT_ID = 0x000b;
static const command_t CMD_DEL_PROXY_CLIENT = 0x000c;
static const command_t CMD_PROXY_CLIENT_COMMAND = 0x000d;
static const command_t CMD_PROXY_TO_CLIENTS = 0x000e;
static const command_t CMD_SERVER_CONNECT_TO_PROXY = 0x000f;
static const command_t CMD_CLIENT_SEND_AUTH = 0x0010;
static const command_t CMD_SERVER_COMMAND = 0x0011;
static const command_t CMD_ALIVE_RESP = 0x0012;

static const command_t CMD_AUDIO_COMM_START = 0x0020;
static const command_t CMD_AUDIO_COMM_DATA = 0x0021;
static const command_t CMD_AUDIO_COMM_STOP = 0x0022;

#endif//MULTIPLAYER_INTERNAL_H
