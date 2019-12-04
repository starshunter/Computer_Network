#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h> 
#include <string.h>

int main(int argc , char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	char buffer[1024];
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}

	memset(&buffer[0], 0, sizeof(buffer));
	read(socket_desc, buffer, 1024);
	printf("%s", buffer);

	while(1)
	{
		printf("I want to :\n");
		printf("(1) Register");
		printf(" (2) Log In");
		printf(" (3) List all the users online");
		printf(" (q) Exit\n");

		char input[20];

		while(1)
		{
			scanf("%s", input);
			if (!strcmp(input, "1") || !strcmp(input, "2") || !strcmp(input, "3") || !strcmp(input, "q"))
				break;
			printf("There's no operation for %s, please try again\n", input);
		}

		char message[100];
		if (!strcmp(input, "1"))
		{
			char temp[100];
			printf("Please enter the user name you want to register\n");
			scanf("%s", temp);
			strcpy(message, "REGISTER#");
			strcat(message, temp);
			strcat(message, "\n");
			//strcpy(message, "REGISTER#peter\n");
		}
		else if (!strcmp(input, "2"))
		{
			printf("Please enter the user name\n");
			scanf("%s", message);
			char temp[100];
			strcat(message, "#");
			printf("Please enter the port name\n");
			scanf("%s", temp);
			strcat(message, temp);
			strcat(message, "\n");
			//strcpy(message, "peter#1234\n");

		}
		else if (!strcmp(input, "3"))
		{
			strcpy(message, "List\n");

		}
		else if (!strcmp(input, "q"))
		{
			strcpy(message, "Exit\n");
			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			printf("%s", buffer);
			break;
		}
		send(socket_desc, message, strlen(message), 0);
		memset(&buffer[0], 0, sizeof(buffer));
		read(socket_desc, buffer, 1024);
		printf("%s", buffer);
	}
	return 0;
}