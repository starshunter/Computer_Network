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
#include "threadpool.h"

extern int errno;
using namespace std;
 
//the thread function
void connection_handler(void *);
char *get_IP(void *);
int register_num = 0, online_num = 0;

struct User
{
    char *name = NULL;
    char *ip_addr = NULL;
    char *port_num = NULL;
    int account = -9999;
    void Register(char *);
    void LogIn(char *, char *);
    char *Info();
    void Offline();
};

void User::Register(char *input_name)
{
	name = new char(strlen(input_name));
	strcpy(name, input_name);
}

void User::LogIn(char *input_ip, char *input_port)
{
	ip_addr = new char(strlen(input_ip));
	strcpy(ip_addr, input_ip);
	port_num = new char(strlen(input_port));
	strcpy(port_num, input_port);
	if(account == -9999)
		account = 100;
}
void User::Offline()
{
	delete [] ip_addr;
	delete [] port_num;
	ip_addr = NULL;
	port_num = NULL;
}

char *User::Info()
{
	char *return_message = new char[2000];

	strcat(return_message, name);

	strcat(return_message, "#");

	strcat(return_message, ip_addr);

	strcat(return_message, "#");

	strcat(return_message, port_num);

	strcat(return_message, "\n");

	return return_message;
}

User *user[100];
 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    threadpool tp;
    tp = create_threadpool(10); 
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     

	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        /*if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }*/
        dispatch(tp, connection_handler, (void*) &client_sock);
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        //puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void connection_handler(void *socket_desc)
{
    //Get the socket descriptor

    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];
    char *ip = get_IP(socket_desc);
    int curr_usr = -1;
    
    //Send some messages to the client
    char *return_message = "You have connected to the server!\n";
    write(sock , return_message , strlen(return_message));

    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
    	//client_message[strlen(client_message) - 1] = 0;
        puts(client_message);
        char temp[2000], *ptr = 0;
        strcpy(temp, client_message);

        ptr = strtok(temp, "#");

        if(!strcmp(ptr, "REGISTER"))
        {
            bool used = 0;
            ptr = strtok(NULL, "\n");

            for(int i = 0;i < register_num;i++)
            {
                if(!strcmp(ptr, user[i]->name))
                {
                    char *return_message = "210 FAIL\n";
                    write(sock , return_message, strlen(return_message));

                    used = 1;
                    break;
                }
            }
            if(!used)
            {
                user[register_num] = new User;


                strtok(ptr, "\n");
                user[register_num]->Register(ptr);

                register_num++;

                char *return_message = "100 OK\n";
                write(sock, return_message, strlen(return_message));
            }
        }
        else if(!strcmp(client_message, "List\n"))
        {
            if(curr_usr == -1)
            {
                char *return_message = "220 AUTH_FAIL\n";
                write(sock, return_message, strlen(return_message));
            }
            else
            {
                char ab[100] = {0};
                sprintf(ab, "%d", user[curr_usr]->account);
                ab[strlen(ab)] = '\n';
                write(sock, ab, strlen(ab));

                char ob[100] = {0};
                sprintf(ob, "%d", online_num);
                ob[strlen(ob)] = '\n';
                write(sock, ob, strlen(ob));

                for(int i = 0;i < 100;i++)
                {
                    if(i < register_num && user[i]->port_num != NULL)
                    {
                        char *return_message = user[i]->Info();
                        write(sock , return_message, strlen(return_message));
                    }
                }
            }
        }
        else if(!strcmp(ptr, "Exit\n"))
        {
        	if(curr_usr != -1)
        	{
        		user[curr_usr]->Offline();
        		online_num--;
        		curr_usr = -1;
        	}
            char *return_message = "Bye\n";
            write(sock, return_message, strlen(return_message));
        }
        else
        {
            char un[100];
            strcpy(un, ptr);

            bool registered = 0;
            int index = 0;

            for(int i = 0;i < register_num;i++)
            {
                if(!strcmp(un, user[i]->name))
                {
                    registered = 1;
                    index = i;
                    if(user[i]->port_num != NULL)
                    	registered = 0;
                    break;
                }
            }

            if(curr_usr != -1)
            	registered = 0;

            if(registered)
            {
                ptr = strtok(NULL, "\n");
                user[index]->LogIn(ip, ptr);
                curr_usr = index;
                online_num ++;

                char ab[100] = {0};
                sprintf(ab, "%d", user[curr_usr]->account);
                ab[strlen(ab)] = '\n';
                write(sock, ab, strlen(ab));

                char ob[100] = {0};
                sprintf(ob, "%d", online_num);
                ob[strlen(ob)] = '\n';
                write(sock, ob, strlen(ob));

                for(int i = 0;i < 100;i++)
                {
                    if(i < register_num && user[i]->port_num != NULL)
                    {
                        char *return_message = user[i]->Info();
                        write(sock , return_message, strlen(return_message));
                    }
                }
            }
            else
            {
				char *sender = strtok(client_message, "#");
            	ptr = strtok(NULL, "#");
            	char amount[10];
            	strcpy(amount, ptr);
            	ptr = strtok(NULL, "#");
            	if(ptr != NULL)
            	{
            		user[curr_usr]->account += atoi(amount);
					for(int i = 0;i < register_num;i++)
					{
						if(!strcmp(sender, user[i]->name))
						{
							user[i]->account -= atoi(amount);
							break;
						}
					}
            	}
            	else
            	{
            		char *return_message = "220 AUTH_FAIL\n";
                	write(sock, return_message, strlen(return_message));
            	}
            }
        }
        //end of string marker
		client_message[read_size] = '\0';
		
		//Send the message back to client
        //write(sock , client_message , strlen(client_message));
		
		//clear the message buffer
		memset(client_message, 0, 2000);
    }
     
    if(read_size == 0)
    {
    	if(curr_usr != -1)
        {
        	user[curr_usr]->Offline();
        	online_num--;
        	curr_usr = -1;
        }
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    return;
} 

char *get_IP(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);
    getpeername(sock, (struct sockaddr*) &c, &cLen);
    char *ip = inet_ntoa(c.sin_addr);
    return ip;
}
