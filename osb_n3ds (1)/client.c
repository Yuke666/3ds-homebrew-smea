#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "client.h"
#include "window.h"

static void 	*socketContext;
static Server 	server;
static Client 	client;
static char 	errorMessage[CLIENT_ERR_MSG_LEN];

int Client_Start(void){

	socketContext = (u8 *)memalign(0x1000, SOCKET_CONTEXT_SIZE);

	socInit((void *)socketContext, SOCKET_CONTEXT_SIZE);

	client.socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(client.socket <= 0){
		strcpy(errorMessage, "Error creating socket\n");
		goto error;
	}

	if(fcntl(client.socket, F_SETFL, O_NONBLOCK, 1) == -1){
		strcpy(errorMessage, "Error setting socket as non-blocking\n");
		goto error;
	}

	struct sockaddr_in addr;
	addr.sin_port         = htons(SERVER_PORT);
	addr.sin_family       = AF_INET;
	addr.sin_addr.s_addr  = INADDR_ANY;

	int res = bind(client.socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	
	if(res < 0){
		strcpy(errorMessage, "Error binding server socket\n");
		goto error;
	}

	strcpy(errorMessage, "Connected to server successfully.\n");

	server.addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server.addr.sin_family = AF_INET;
	server.addr.sin_port = htons(SERVER_PORT);

	Client_SendMessage(MSG_TYPE_CONNECT);

	return 1;

	error:

	free(socketContext);

	if(client.socket > 0)
		closesocket(client.socket);


	client.socket = -1;

	return -1;
}

void Client_Close(){

	if(client.socket > 0){

		Client_SendMessage(MSG_TYPE_DISCONNECT);
	}

	free(socketContext);

	closesocket(client.socket);

	socExit();
}

int Client_SendServer(const void *buf, u16 size){

	int ret = sendto(client.socket, buf, size, 0, (struct sockaddr *)&server.addr, sizeof(struct sockaddr_in));

	client.lastSendTime = GetCurrTime();

	return ret;
}

int Client_Recv(void *data, u16 len){

	struct sockaddr_in from;

	socklen_t size = sizeof(struct sockaddr_in);

	int ret = recvfrom(client.socket, (char *)data, len, 0, (struct sockaddr *)&from, &size);

	if(ret < 0){
		if(errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;

		return -1;
	}

	if((ret <= 0) || (ret >= 4 && *((u32 *)data) == PROTO_ID))
		return ret;

	return 0;
}

int Client_HasTimedOut(void){

	return 0;
}

void Client_GetErrorMessage(char *into){

	strcpy(into, errorMessage);
}

int Client_IsConnected(void){

	return client.socket > 0;
}

void Client_KeepAlive(void){

	u64 currTime = GetCurrTime();

	if(currTime - client.lastSendTime > KEEP_ALIVE_TIME){
		Client_SendMessage(MSG_TYPE_KEEP_ALIVE);
	}
}

void Client_SendMessage(u8 msg){
		
	u8 msgPacket[5];
	msgPacket[0] = PROTO_ID & 0xFF;
	msgPacket[1] = (PROTO_ID >> 8) & 0xFF;
	msgPacket[2] = (PROTO_ID >> 16) & 0xFF;
	msgPacket[3] = (PROTO_ID >> 24) & 0xFF;
	msgPacket[4] = msg;
	
	Client_SendServer(msgPacket, 5);
}