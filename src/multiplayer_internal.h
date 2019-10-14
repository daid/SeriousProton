#ifndef MULTIPLAYER_INTERNAL_H
#define MULTIPLAYER_INTERNAL_H

//Definitions shared between different SeriousProton multiplayer objects, but do not need to be exported outside the engine.
typedef int16_t command_t;
static const command_t CMD_CREATE = 0x0001;
static const command_t CMD_UPDATE_VALUE = 0x0002;
static const command_t CMD_DELETE = 0x0003;
static const command_t CMD_SET_CLIENT_ID = 0x0004;
static const command_t CMD_SET_GAME_SPEED = 0x0005;
static const command_t CMD_CLIENT_COMMAND = 0x0006;
static const command_t CMD_ALIVE = 0x0007;
static const command_t CMD_CLIENT_AUDIO_COMM = 0x0008;
static const command_t CMD_REQUEST_AUTH = 0x0009;
static const command_t CMD_CLIENT_SEND_AUTH = 0x0010;
static const command_t CMD_SERVER_COMMAND = 0x0011;
static const command_t CMD_ALIVE_RESP = 0x0012;

#endif//MULTIPLAYER_INTERNAL_H
