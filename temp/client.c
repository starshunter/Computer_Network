#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <stdbool.h>
#include <iostream>
#include <errno.h>
#include <vector>
#include "threadpool.h"

using namespace std;

void *receiver_connection_handler(void *);
void *sender_connection_handler(void *);
void p2p_receiver_handler(void *);
void p2p_sender_handler(void *);
char p2p_message[1000] = {0};

int main(int argc , char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	char buffer[1024], username[1024];
	bool login = false;
	int user_portnum = 0, user_balance = 0;
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

	while(!login)
	{
		printf("I want to :\n");
		printf("(1) Register");
		printf(" (2) Log In");
		printf(" (q) Exit\n");

		char input[20];

		while(1)
		{
			scanf("%s", input);
			if (!strcmp(input, "1") || !strcmp(input, "2") || !strcmp(input, "q"))
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
			strcpy(username, message);
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
			{
				char *cur = strtok(buffer, "\n");
				printf("Your account balance is %s\n", cur);
				user_balance = atoi(cur);
				cur = strtok(NULL, "\n");
				printf("Currently %s user(s) online\n", cur);
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
				login = true;
				user_portnum = atoi(temp);
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

	if(login)
	{
		pthread_t thread_id;
		int client_sock;
		int pn[1];
		pn[0] = user_portnum;
		if( pthread_create( &thread_id , NULL ,  receiver_connection_handler , (void*)pn) < 0)
        {
            perror("could not create thread");
            return 1;
        }
	}

	while(login)
	{
		printf("Hi %s, what do you want to do? :\n", username);
		printf("(1) List");
		printf(" (2) Pay");
		printf(" (y) Confirm transaction");
		printf(" (q) Exit\n");

		char input[20];

		while(1)
		{
			scanf("%s", input);
			if (!strcmp(input, "1") || !strcmp(input, "2") || !strcmp(input, "y") || !strcmp(input, "q"))
				break;
			printf("There's no operation for %s, please try again\n", input);
		}

		char message[100];
		
		if (!strcmp(input, "1"))
		{
			strcpy(message, "List\n");

			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			char *cur = strtok(buffer, "\n");
			printf("Your account balance is %s\n", cur);
			user_balance = atoi(cur);
			cur = strtok(NULL, "\n");
			printf("Currently %s user(s) online\n", cur);
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
		else if (!strcmp(input, "2"))
		{
			strcpy(message, "List\n");

			send(socket_desc, message, strlen(message), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			read(socket_desc, buffer, 1024);
			char *cur = strtok(buffer, "\n");
			printf("Your account balance is %s\n", cur);
			user_balance = atoi(cur);
			cur = strtok(NULL, "\n");
			printf("Currently %s user(s) online\n", cur);
			cur = strtok(NULL, "\n");
			int num = 1;
			char user_list[100][100], ip_list[100][100], port_list[100][100];
			while(cur != NULL)
			{
				char *usr, *ip, *port;
				usr = strsep(&cur, "#");
				strcpy(user_list[num], usr);
				ip = strsep(&cur, "#");
				strcpy(ip_list[num], ip);
				port = strsep(&cur, "#");
				strcpy(port_list[num], port);
				printf("%d. User : %s  IP : %s  Port : %s\n", num, usr, ip, port);
				cur = strtok(NULL, "\n");
				num++;
			}
			puts("Choose the user you want to pay (enter number)?");
			int payee_num = 0;
			while(1)
			{
				scanf("%d", &payee_num);
				if(payee_num >= num)
					printf("There's no user number %d, please try again\n", payee_num);
				else
					break;
			}
			puts("How many do you want to pay?");
			int pay_amount = 0;
			scanf("%d", &pay_amount);
			if(pay_amount > user_balance)
				puts("You don't have enough money");
			else if(!strcmp(username, user_list[payee_num]))
				puts("You cannot select yourself");
			else
			{
				char client_message[1000] = {0};
				strcpy(client_message, ip_list[payee_num]);
				strcat(client_message, "?");
				strcat(client_message, port_list[payee_num]);
				strcat(client_message, "?");
				strcat(client_message, username);
				strcat(client_message, "#");
				char payment[10] = {0};
				sprintf(payment, "%d", pay_amount);
				strcat(client_message, payment);
				strcat(client_message, "#");
				strcat(client_message, user_list[payee_num]);
				puts(client_message);
				pthread_t thread_id2;
				int client_sock;
				if( pthread_create( &thread_id2 , NULL ,  sender_connection_handler , (void*)client_message) < 0)
		        {
		            perror("could not create thread");
		            return 1;
		        }
			}
		}
		else if (!strcmp(input, "y"))
		{
			if(strlen(p2p_message) > 0)
			{
				puts("Transaction confirm");
				send(socket_desc, p2p_message, strlen(p2p_message), 0);
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

void *receiver_connection_handler(void *sock)
{
	int *portnum = (int*)sock;
    int socket_desc, client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(*portnum);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
    }

    listen(socket_desc , 3);
     
    //Accept and incoming connection
    c = sizeof(struct sockaddr_in);
     

	pthread_t thread_id;
	threadpool tp;
    tp = create_threadpool(10); 
    while(client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))
    {
    	puts("Another user has made a transaction with you");
    	puts("Please enter y to confirm transaction");
    	dispatch(tp, p2p_receiver_handler, (void*) &client_sock);
    }
} 

void *sender_connection_handler(void *sock)
{
	int socket_desc;
	struct sockaddr_in server;
	char buffer[1024], username[1024];
	char *message = (char *)sock;
	char *ip = strtok(message, "?");
	char *port = strtok(NULL, "?");
	char *client_message = strtok(NULL, "?");
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(port));

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
	}
	send(socket_desc, client_message, strlen(client_message), 0);
} 

void p2p_receiver_handler(void *socket_desc)
{
	int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];
    recv(sock , client_message , 2000 , 0);

    client_message[strlen(client_message)] = 0;
    puts(client_message);
    memset(p2p_message, 0, sizeof(p2p_message));
    strcpy(p2p_message, client_message);
    memset(client_message, 0, sizeof(client_message));
    return;
}