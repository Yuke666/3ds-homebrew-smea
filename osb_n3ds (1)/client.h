#ifndef CLIENT_DEF
#define CLIENT_DEF

#include <netinet/in.h>
#include "window.h"
#include <stdint.h>

#define KEEP_ALIVE_TIME			1000
#define CLIENT_ERR_MSG_LEN 		32
#define SERVER_IP 				"192.168.3.109"
#define SERVER_PORT 			23415
#define SOCKET_CONTEXT_SIZE 	0x10000
#define TIMEOUT_TIME 			10
#define PROTO_ID 				591549263
#define PROTO_ID_REV 			1330856483
#define HEADER_BYTES 			4
#define MAX_PACKET_SIZE 		1024

enum {
	MSG_TYPE_DISCONNECT = 0,
	MSG_TYPE_CONNECT,
	MSG_TYPE_ACK,
	MSG_TYPE_KEEP_ALIVE,
	MSG_TYPE_GAME_UPDATE,
	MSG_TYPE_SEARCH_GAME,
	MSG_TYPE_SELECT_CHARACTER,
	MSG_TYPE_QUIT_GAME,
	MSG_TYPE_START_GAME,
	MSG_TYPE_OPPONENT_DISCONNECT,
	MSG_TYPE_ALLY_DISCONNECT,
};

typedef struct {
	u32 protoID;
	u8 type;
} PacketHeader;

typedef struct {

	int socket;

	int snapshot; // inc every snapshot

	u64 lastSendTime;

} Client;

typedef struct {

	struct sockaddr_in addr;



	int snapshot; // set every snapshot recieved more recent than the current.

	u64 lastRecvTime;

} Server;

int 		Client_Start(void);
void 		Client_Close(void);

/*
	size needs to be at least HEADER_SIZE,
	and the buffer must begin with a valid header.
*/

int 		Client_SendServer(const void *buffer, u16 size);

/*
	if len is less than the data available, the remaining data is dropped.
	len must be at least 4, for the protocol id
*/

int 		Client_Recv(void *data, u16 len);
int 		Client_HasTimedOut(void);
int 		Client_IsConnected(void);
void 		Client_GetErrorMessage(char *into);
void 		Client_KeepAlive(void);
void 		Client_SendMessage(u8 msg);

#endif