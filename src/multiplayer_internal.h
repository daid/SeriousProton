#ifndef MULTIPLAYER_INTERNAL_H
#define MULTIPLAYER_INTERNAL_H

//Definitions shared between different SeriousProton multiplayer objects, but do not need to be exported outside the engine.

static const int16_t CMD_CREATE = 0x0001;
static const int16_t CMD_UPDATE_VALUE = 0x0002;
static const int16_t CMD_DELETE = 0x0003;
static const int16_t CMD_SET_CLIENT_ID = 0x0004;
static const int16_t CMD_SET_GAME_SPEED = 0x0005;
static const int16_t CMD_CLIENT_COMMAND = 0x0006;

#endif//MULTIPLAYER_INTERNAL_H
