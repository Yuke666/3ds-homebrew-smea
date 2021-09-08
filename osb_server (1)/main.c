#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "server.h"
#include "memory.h"
#include "log.h"


static void 	DisconnectHandler(u8 *packet, u16 len, struct sockaddr_in *from);
static void 	Main_Thread(void);
static void 	*GamesThreadFunc(void *arg);
static int 		Main_Init();
static void 	Main_Exit();
static void 	ExitSignalHandler(int sig);

// MSG_TYPE_DISCONNECT = 0,
// MSG_TYPE_ACK,
// MSG_TYPE_KEEP_ALIVE,
// MSG_TYPE_GAME_UPDATE,
// MSG_TYPE_SEARCH_GAME,
// MSG_TYPE_SELECT_CHARACTER,
// MSG_TYPE_QUIT_GAME,
// MSG_TYPE_START_GAME,
// MSG_TYPE_OPPONENT_DISCONNECT,
// MSG_TYPE_ALLY_DISCONNECT,
// NUM_MESSAGES,


static void (*messageHandlers[NUM_MESSAGES])(u8 *packet, u16 len, struct sockaddr_in *from) = {
	DisconnectHandler,
	NULL,
	NULL,
	NULL,
	NULL,
};

static pthread_mutex_t 	mutex;
static pthread_t 		gamesThread;
static u8 				quit;

static void ExitSignalHandler(int sig){

	(void)sig;

	pthread_mutex_lock(&mutex);
	quit = 1;
	pthread_mutex_unlock(&mutex);
}

static void DisconnectHandler(u8 *packet, u16 len, struct sockaddr_in *from){

	(void)packet;
	(void)len;

	Client *client = Server_ClientFromAddr(from);

#ifdef DEBUG
	LOGF(LOG_CYAN, "\n%s: disconnected.\n", inet_ntoa(client->addr.sin_addr));
#endif
	
	Server_RemoveClient(client);
}

static int Main_Init(){

	if(Server_Start() < 0)
		return -1;

	LOG(LOG_GREEN, "Server initialized.\n");

	signal(SIGINT, ExitSignalHandler);
	signal(SIGTERM, ExitSignalHandler);
	signal(SIGKILL, ExitSignalHandler);
	signal(SIGQUIT, ExitSignalHandler);

	quit = 0;

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&gamesThread, NULL, GamesThreadFunc, NULL);

	return 1;
}

static void Main_Exit(){

	pthread_mutex_destroy(&mutex);

	Server_Close();

	LOG(LOG_GREEN, "Server closed.\n");
}

static void Main_Thread(void){

	u8 packet[MAX_PACKET_SIZE];

	while(1){

		pthread_mutex_lock(&mutex);

		if(quit){
			pthread_mutex_unlock(&mutex);
			break;
		}

		pthread_mutex_unlock(&mutex);

		struct sockaddr_in from;
		
		int len = Server_Recv(packet, MAX_PACKET_SIZE, &from);

		if(len < 0){

			pthread_mutex_lock(&mutex);
			quit = 1;
			pthread_mutex_unlock(&mutex);

			break;

		} else if(len > 0){

			packet[len + 1] = 0;
			
			Client *client = Server_ClientFromAddr(&from);

			PacketHeader *header = (PacketHeader *)packet;

			if(client){

				if(messageHandlers[header->type] != NULL)
					messageHandlers[header->type](packet, len, &from);

				if(header->type == MSG_TYPE_DISCONNECT){
	
					client = NULL;
				}

			} else {

				client = Server_AddClient(&from);
#ifdef DEBUG
				LOGF(LOG_CYAN, "\n%s: connected.\n", inet_ntoa(client->addr.sin_addr));
#endif
				char msg[MAX_PACKET_SIZE];

				msg[0] = PROTO_ID & 0xFF;
				msg[1] = (PROTO_ID >> 8) & 0xFF;
				msg[2] = (PROTO_ID >> 16) & 0xFF;
				msg[3] = (PROTO_ID >> 24) & 0xFF;
				// msg[4] = MSG_TYPE_GAME_UPDATE;
				msg[4] = ':';
				msg[5] = '3';

				Server_SendClient(client, msg, 6);
			}

			if(client){

				time_t currTime;
				time(&currTime);

				client->lastRecvTime = (u32)currTime;
			}
		}

		Server_DisconnectTimedOut();
	}
}

static void *GamesThreadFunc(void *arg){

	(void)arg;

	LOG(LOG_GREEN, "Entered games thread.\n");

	while(1){

		pthread_mutex_lock(&mutex);

		if(quit){
			pthread_mutex_unlock(&mutex);
			break;
		}

		pthread_mutex_unlock(&mutex);
	}

	pthread_exit(NULL);

	return NULL;
}

int main(){

	if(Main_Init() < 0)
		return 1;

	Main_Thread();

	pthread_join(gamesThread, NULL);

	Main_Exit();

	return 0;
}