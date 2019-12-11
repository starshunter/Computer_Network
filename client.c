#include <stdio.h>
#include <stdlib.h>
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
		
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));

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

			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			if (buffer[0] == '1')
				printf("User %s register successfully!\n", temp);
			else
				printf("User %s has already been registered, please try another user name!\n", temp);
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

			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			if (buffer[0] == '2' && buffer[1] == '2' && buffer[2] == '0')
				printf("This user name has not been registered, please register first or try another user name!\n");
			else
				printf("%s", buffer);

		}
		else if (!strcmp(input, "3"))
		{
			strcpy(message, "List\n");

			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			char *cur = strtok(buffer, "\n");
			cur = strtok(NULL, "\n");
			printf("%s\n", cur);
			cur = strtok(NULL, "\n");
			while(cur != NULL)
			{
				char *usr, *ip, *port;
				usr = strsep(&cur, "#");
				ip = strsep(&cur, "#");
				port = strsep(&cur, "#");
				printf("User : %s  IP : %s  Port : %s\n", usr, ip, port);
				cur = strtok(NULL, "\n");
			}

		}
		else if (!strcmp(input, "q"))
		{
			strcpy(message, "Exit\n");
			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			printf("%s\n", buffer);
			break;
		}
	}
	return 0;
}