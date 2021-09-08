#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <asm-generic/ioctls.h>
#include <time.h>
#include "server.h"
#include "log.h"

static u32 			HashAddr(struct sockaddr_in *addr);
static void 		InitClients(void);
static Client 	 	*CreateClient(void);
static void 		CloseClient(Client *client);

static Server 	server;
static Client 	clients[MAX_CLIENTS];
static Client 	*clientTable[CLIENT_TABLE_SIZE];
static Client 	*firstFreeClient;
static Client 	*firstConnectedClient;

int Server_SendClient(Client *cli, const void *buf, u16 size){

	return sendto(server.socket, buf, size, 0, (struct sockaddr *)&cli->addr, sizeof(struct sockaddr_in));
}

int Server_Start(void){

	server.socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	if(server.socket <= 0) {
		LOG(LOG_RED, "Error creating server socket\n");
		goto error;
	}
	
	int option = 1;
	setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
			
	if(fcntl(server.socket, F_SETFL, O_NONBLOCK, 1) == -1){
		LOG(LOG_RED, "Error setting server socket to non-blocking.\n");
		goto error;
	}

	struct sockaddr_in addr = {
		.sin_port         = htons(SERVER_PORT),
		.sin_family       = AF_INET,
		.sin_addr.s_addr  = INADDR_ANY
	};

	int res = bind(server.socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	
	if(res < 0){
		LOG(LOG_RED, "Error binding server socket\n");
		goto error;
	}

	InitClients();

	return 1;

	error:

	if(server.socket > 0)
		close(server.socket);

	return -1;
}

void Server_Close(void){

	close(server.socket);
}

int Server_Recv(void *data, u16 len, struct sockaddr_in *from){

	socklen_t size = sizeof(struct sockaddr_in);

	int ret = recvfrom(server.socket, (char *)data, len, 0, (struct sockaddr *)from, &size);

	if(ret < 0){
		if(errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;

		return -1;
	}

	if((ret <= 0) || (ret >= 4 && *((u32 *)data) == PROTO_ID))
		return ret;

	return 0;
}

static void InitClients(void){

	firstConnectedClient = 0;

	Client **curr = &firstFreeClient;

	int k;
	for(k = 1; k < MAX_CLIENTS; k++){
		*curr = &clients[k];
		(*curr)->prevFree = &clients[k-1];
		(*curr)->prevFree->nextFree = &clients[k];
		curr = &(*curr)->nextFree;
	}
}

static Client *CreateClient(void){

	if(!firstFreeClient){
#ifdef DEBUG
		LOG(LOG_RED, "OUT OF FREE CLIENTS.\n");
#endif
		return 0;
	}

	Client *client = firstFreeClient;

	firstFreeClient = firstFreeClient->nextFree;

	if(firstFreeClient)
		firstFreeClient->prevFree = 0;

	if(firstConnectedClient)
		firstConnectedClient->prev = client;

	client->next = firstConnectedClient;

	firstConnectedClient = client;

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

	if(firstFreeClient)
		firstFreeClient->prevFree = client;

	if(firstConnectedClient == client)
		firstConnectedClient = 0;

	client->nextFree = firstFreeClient;

	firstFreeClient = client;
}

static u32 HashAddr(struct sockaddr_in *addr){

    return (addr->sin_addr.s_addr + addr->sin_port) % CLIENT_TABLE_SIZE;
}

Client *Server_ClientFromAddr(struct sockaddr_in *addr){

    Client *client = clientTable[HashAddr(addr)];

    if(!client) return 0;

    while(client->addr.sin_addr.s_addr != addr->sin_addr.s_addr){

    	if(!client->nextCollision) return 0;

    	client = client->nextCollision;
    }

    return client;
}

void Server_RemoveClient(Client *client){

	Client *prev = client->prevCollision;
	Client *next = client->nextCollision;

	if(!prev)
		clientTable[HashAddr(&client->addr)] = next;

	if(prev) prev->next = next;
	if(next) next->prev = prev;

	CloseClient(client);
}

Client *Server_AddClient(struct sockaddr_in *addr){

	u32 hash = HashAddr(addr);

    Client *client = clientTable[hash];

    if(client){
	
	    while(client->nextCollision){

	    	client = client->nextCollision;

	    	if(!client->nextCollision) break;
	    }
    
    } else {
	
	    client = CreateClient();

	    clientTable[hash] = client;
    }

    client->addr = *addr;

    return client;
}

void Server_DisconnectTimedOut(void){

	Client *currClient = firstConnectedClient;

	time_t currTime;
	time(&currTime);	

	while(currClient){

		Client *next = currClient->next;

		if((u32)currTime - currClient->lastRecvTime > TIMEOUT_TIME){

			Server_RemoveClient(currClient);

#ifdef DEBUG
			LOGF(LOG_YELLOW, "Client timed out. ip=%s\n", inet_ntoa(currClient->addr.sin_addr));
#endif

		}

		currClient = next;
	}

}