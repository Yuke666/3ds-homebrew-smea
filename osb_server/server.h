#ifndef SERVER_DEF
#define SERVER_DEF

#include <netinet/in.h>
#include <stdint.h>

#define TIMEOUT_TIME 			10
#define SERVER_PORT 			23415
#define PROTO_ID 				591549263
#define PROTO_ID_REV 			1330856483
#define MAX_CLIENTS 			1024
#define CLIENT_TABLE_SIZE 		(0x01 << 20) // the bigger the less collisions, 
#define MAX_PACKET_SIZE 		1024

typedef uint32_t u32;
typedef uint8_t u8;
typedef uint16_t u16;

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
	NUM_MESSAGES,
};

typedef struct {
	int socket;
	int snapshot;
} Server;

typedef struct {

	u32 protoID;
	u8 type;
	u16 sequence;

} PacketHeader;

typedef struct _Client {
	struct _Client 			*nextCollision;
	struct _Client 			*prevCollision;
	struct _Client 			*nextFree;
	struct _Client 			*prevFree;
	struct _Client 			*next;
	struct _Client 			*prev;
	struct sockaddr_in 		addr;
	u32						lastRecvTime;
	void 					*data;
} Client;

int 		Server_Start(void);
void 		Server_Close(void);
int 		Server_SendClient(Client *client, const void *buffer, u16 size);

/*
	if len is less than the data available, the remaining data is dropped.
	len must be at least 4, for the protocol id
*/

int 		Server_Recv(void *data, u16 len, struct sockaddr_in *from);
void 		Server_RemoveClient(Client *client);
Client 		*Server_AddClient(struct sockaddr_in *addr);
Client 		*Server_ClientFromAddr(struct sockaddr_in *addr);
void 		Server_DisconnectTimedOut(void);

#endif