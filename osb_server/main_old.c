#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "log.h"
#include "stage.h"

#define ACK_BYTE 0x01
#define DISCONNECT_BYTE 0x02
#define SNAPSHOT_BYTE 0x03
#define FOUND_GAME_BYTE 0x04
#define SEARCH_FOR_GAME_BYTE 0x05
#define SELECT_CHARACTER_BYTE 0x06

#define NUM_PLAYERS_PER_GAME MAX_PLAYERS

#define MAX_USERNAME_LENGTH 16
#define MAX_GAMES 100
#define MAX_CLIENTS MAX_GAMES * MAX_PLAYERS
#define MAX_RECV_SIZE 1024

typedef struct Game Game;
typedef struct Client Client;

struct Client {
	u8			character;
	Client 		*nextFree;
	Client 		*prevFree;
	Client 		*next;
	Client 		*prev;
	Game 		*game;
	u16 		lastPacketTime;
	u32 		userID;
	int 		socket;
	char 		username[MAX_USERNAME_LENGTH];
};

struct Game {
	u8			started;
	Game 		*nextFree;
	Game 		*prevFree;
	Game 		*next;
	Game 		*prev;
	Stage 		stage;
	Client 		*clients[MAX_PLAYERS];
};

static Client 				clients[MAX_CLIENTS];
static Client 				*freeClients;
static Client 				*connectedClients;
static int 					serverSocket;
static struct sockaddr_in 	server;
static pthread_t 			gamesThread;
static Game 				games[MAX_GAMES];
static Game 				*freeGames;
static Game 				*activeGames;
static Client 				*queue[MAX_PLAYERS];
static u8 					playersInQueue;
static u8 					quit;

static void *GamesThread(void *);
static void InitGames(void);
static void InitClients(void);
static Game *InitGame(Client **clients, int num);
static Client *InitClient(void);
static void CloseGame(Game *game);
static void CloseClient(Client *client);
static void BroadcastToPlayers(Game *game, u8 *buffer, u16 len);
static void Exit(void);

static void SignalHandler(int signum){

	(void)signum;

	quit = 1;
}

static int Init(void){

	quit = 0;

	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGKILL, SignalHandler);
	signal(SIGQUIT, SignalHandler);

	server.sin_port         = htons(23415);
	server.sin_family       = AF_INET;
	server.sin_addr.s_addr  = INADDR_ANY;
	serverSocket 			= socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	int optval = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	
	if(fcntl(serverSocket, FIONBIO, &optval) == -1){

		NO_INFO_LOG(LOG_RED, "Failed to set as non blocking.\n");
		goto error;
	}


	if(serverSocket == -1) {
		NO_INFO_LOG(LOG_RED, "Error creating server socket\n");
		goto error;
	}
	
	int res = bind(serverSocket,(struct sockaddr *)&server, sizeof(server));
	
	if(res == -1){
		NO_INFO_LOG(LOG_RED, "Error binding server socket\n");
		goto error;
	}
	
	res = listen(serverSocket,6);
	
	if(res == -1){
		NO_INFO_LOG(LOG_RED, "Error listening on server socket\n");
		goto error;
	}

	NO_INFO_LOG(LOG_GREEN, "Server initialized.\n");

	NO_INFO_LOG(LOG_YELLOW, "Initializing games and clients..\n");

	memset(&games, 0, sizeof(games));
	memset(&clients, 0, sizeof(clients));

	connectedClients 	= NULL;
	activeGames 		= NULL;
	playersInQueue		= 0;

	InitGames();
	InitClients();

	NO_INFO_LOG(LOG_YELLOW, "Creating games thread..\n");

	pthread_create(&gamesThread, NULL, GamesThread, NULL);

	NO_INFO_LOG(LOG_GREEN, "Successfully initialized.\n");

	return 1;

	error:

	shutdown(serverSocket, SHUT_RDWR);

	return -1;    
}

static void Exit(void){

	shutdown(serverSocket, SHUT_RDWR);

	printf("\nExited\n");
}

// Games ------------------------------------------------------

static void InitGames(void){

	Game **currGame = &freeGames;

	int k;
	for(k = 1; k < MAX_GAMES; k++){
		*currGame = &games[k];
		(*currGame)->prevFree = &games[k-1];
		(*currGame)->prevFree->nextFree = &games[k];
		currGame = &(*currGame)->nextFree;
	}
}

static Game *InitGame(Client **clients, int num){

#ifdef DEBUG
	if(!freeGames){
		NO_INFO_LOG(LOG_RED, "OUT OF FREE GAMES.");
		return NULL;
	}
#endif

	Game *game = freeGames;

	freeGames = freeGames->nextFree;

	if(freeGames)
		freeGames->prevFree = NULL;

	if(activeGames)
		activeGames->prev = game;

	game->next = activeGames;

	activeGames = game;


	int k;
	for(k = 0; k < num; k++)
		clients[k]->game = game;

	u8 buffer = FOUND_GAME_BYTE;
	BroadcastToPlayers(game, &buffer, 1);

	game->started = 0;

	return game;
}

static void CloseGame(Game *game){

	if(game->prev){

		if(game->prev->next)
			game->prev->next->prev = game->prev;

		game->prev->next = game->next;
	
	} else if(game->next){

		game->next->prev = game->prev;
	}

	if(freeGames)
		freeGames->prevFree = game;

	if(game == activeGames)
		activeGames = NULL;

	game->nextFree = freeGames;

	freeGames = game;
}

// Clients ------------------------------------------------------

static void InitClients(void){

	Client **currClient = &freeClients;

	int k;
	for(k = 1; k < MAX_CLIENTS; k++){
		*currClient = &clients[k];
		(*currClient)->prevFree = &clients[k-1];
		(*currClient)->prevFree->nextFree = &clients[k];
		currClient = &(*currClient)->nextFree;
	}
}

static Client *InitClient(void){

#ifdef DEBUG
	if(!freeClients){
		NO_INFO_LOG(LOG_RED, "OUT OF FREE CLIENTS.");
		return NULL;
	}
#endif

	Client *client = freeClients;

	freeClients = freeClients->nextFree;

	if(freeClients)
		freeClients->prevFree = NULL;

	if(connectedClients)
		connectedClients->prev = client;

	client->next = connectedClients;

	connectedClients = client;

	return client;
 }

static void CloseClient(Client *client){

	if(client->prev){

		if(client->prev->next)
			client->prev->next->prev = client->prev;

		client->prev->next = client->next;
	
	} else if(client->next){

		client->next->prev = client->prev;
	}

	if(freeClients)
		freeClients->prevFree = client;

	if(connectedClients == client)
		connectedClients = NULL;

	client->nextFree = freeClients;
	client->socket = -1;

	freeClients = client;
}

// ---------------------------------------------------------------

static void BroadcastToPlayers(Game *game, u8 *buffer, u16 len){

	int k;
	for(k = 0; k < NUM_PLAYERS_PER_GAME; k++){

	    sendto(serverSocket, buffer, len, 0, (sockaddr *)&game->clients[k]->socket, 0);
	}

}

static void SelectCharacter(Client *client, u8 character){

	u8 buffer[] = { SELECT_CHARACTER_BYTE, character };

	BroadcastToPlayers(client->game, buffer, 2);

	client->character = character;
}

// ---------------------------------------------------------------

static void *GamesThread(void *data){

	(void)data;

	NO_INFO_LOG(LOG_GREEN, "Games thread created.\n");

	while(1){

		Game *currGame = activeGames;

		while(currGame){


			currGame = currGame->next;
		}
	}

	pthread_exit(NULL);
}

static void RunServer(void){

	char buffer[MAX_RECV_SIZE];

	fd_set readfds;
	
	while(!quit){

		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);

		int max = serverSocket;
		
		Client *currClient = connectedClients;

		while(currClient){

			FD_SET(currClient->socket, &readfds);

			if(currClient->socket > max) 
				max = currClient->socket;

			currClient = currClient->next;
		}
		
	    int activity = select(max+1, &readfds, NULL, NULL, NULL);

	    if(activity < 0)
	        break;

	    // handle messages ----------------------------

	    currClient = connectedClients;

		while(currClient){

	        if(!FD_ISSET(currClient->socket, &readfds)){
	        	currClient = currClient->next;
	        	continue;
	        }

	        // ddos protection needed.

	        Client *next = currClient->next;

            int len = 0;
            ioctl(currClient->socket, FIONREAD, &len);
            
            // disconnections -------------------------

            if(len <= 0 || buffer[0] == DISCONNECT_BYTE){

#ifdef DEBUG
                NO_INFO_LOG(LOG_GREEN, "Disconnect client from message.\n");
#endif

                if(currClient->game){

                	// remove from game.
                }

                CloseClient(currClient);

                currClient = next;
            
                continue;
            }

            len = len > MAX_RECV_SIZE ? MAX_RECV_SIZE : len;

            len = recv(currClient->socket, buffer, len, 0);

            buffer[len-1] = 0;

            NO_INFO_LOG(LOG_GREEN, buffer);
            NO_INFO_LOG(LOG_GREEN, "\n");


            char *message = "---------------- SERVER ----------------\n\n"
				            "this is a message from the server,\0";

		    // send(currClient->socket, buffer, len, 0);
		    send(currClient->socket, message, strlen(message) + 1, 0);

    //         if(buffer[0] == SEARCH_FOR_GAME_BYTE){

    //         	queue[playersInQueue++] = currClient;

    //         	if(playersInQueue == NUM_PLAYERS_PER_GAME){

				// 	InitGame(queue, NUM_PLAYERS_PER_GAME);
    //         	}

    //         } else if(buffer[0] == SELECT_CHARACTER_BYTE){
				
				// SelectCharacter(currClient, buffer[1]);


    //         // } else if(buffer[0] == SNAPSHOT_BYTE){

    //         }

			currClient = next;
		}

	    // handle new connections (if any) ------------

	    if(FD_ISSET(serverSocket, &readfds)){

	        struct sockaddr_in clientSockAddr;

	        u32 addrlen = sizeof(clientSockAddr);

	        int socket = accept(serverSocket, (struct sockaddr*)&clientSockAddr, &addrlen);

#ifdef DEBUG
            NO_INFO_LOG(LOG_GREEN, "New connection\n");
#endif

            Client *client = InitClient();

            if(client){
	            client->socket = socket;
	            client->game = NULL;
            } else {
		        NO_INFO_LOG(LOG_RED, "Too many connections.\n");
            	break;
            }
	    }
	}

	pthread_kill(gamesThread, SIGTERM);
}

int main(int argc, char **argv){

	int res = Init();

	if(res < 0)
		return 1;		

	RunServer();

	Exit();

	return 0;
}