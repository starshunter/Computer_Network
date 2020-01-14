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
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <iomanip>
#include <fstream>
#include "threadpool.h"

using namespace std;

void *receiver_connection_handler(void *);
void *sender_connection_handler(void *);
void p2p_receiver_handler(void *);
void p2p_sender_handler(void *);
unsigned char *rsa_encrypt(char *);
unsigned char *rsa_decrypt(char *);

char p2p_message[1000] = {0};
FILE *pub, *pri;
RSA *publicRSA = nullptr, *privateRSA = nullptr;  // 因為回傳的是指標
int main(int argc , char *argv[])
{
	// =============================================================================================================
	
    if((pub = fopen("pub.pem", "r")) == NULL) {
        cout << "pub Error" << endl;
        exit(-1);
    }
    if((pri = fopen("pri.pem", "r")) == NULL) {
        cout << "pri Error" << endl;
        exit(-1);
    }
    // 初始化算法庫
    OpenSSL_add_all_algorithms();
    // 從 .pem 格式讀取公私鑰
    if((privateRSA = PEM_read_RSAPrivateKey(pri, NULL, NULL, NULL)) == NULL) { 
        cout << "Read pri error" << endl;
    }
    if((publicRSA = PEM_read_RSA_PUBKEY(pub, NULL, NULL, NULL)) == NULL) {
        cout << "Read pub error " << endl;
    }
    fclose(pri), fclose(pub);
    cout << "pub: " << publicRSA << endl;
    cout << "pri: " << privateRSA << endl;
    int rsa_len = RSA_size(publicRSA); // 幫你算可以加密 block 大小，字數超過要分開加密
    
    const unsigned char * src = (const unsigned char *)"peter#20#anan"; //  測試的明文
    // 要開空間來存放加解密結果，型態要改成 unsigned char *
    unsigned char * enc = (unsigned char *)malloc(rsa_len); 
    unsigned char * dec = (unsigned char *)malloc(rsa_len);
    // 加密時因為 RSA_PKCS1_PADDING 的關係，加密空間要減 11，回傳小於零出錯
    if(RSA_public_encrypt(rsa_len-11, src, enc, publicRSA, RSA_PKCS1_PADDING) < 0) {
        cout << "enc error" << endl;
    }
    // 加密後就會變成一堆奇怪字元
    cout << "enc: " << enc << endl;
    // 解密時不用減 11，回傳小於零出錯
    if(RSA_private_decrypt(rsa_len, enc, dec, privateRSA, RSA_PKCS1_PADDING) < 0){
        cout << "dec error" << endl;
    }
    cout << "dec: " << dec << endl;
    cout<< rsa_len <<endl;
    // 因為是它的函式 new 出來的東東，需要用他的函式釋放記憶體

    // =================================================================================================================
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
				char temp_len[10];
				char *ptr2p2p = &p2p_message[0];
				sprintf(temp_len, "%d", strlen(ptr2p2p));
				puts("Transaction confirm");
				puts("receive from other client");
				puts(temp_len);
				puts(ptr2p2p);
				char *p2p_message_origin = (char *)rsa_decrypt(ptr2p2p);
				char *p2p_message_encrypt = (char *)rsa_encrypt(p2p_message_origin);
				//char *temp = (char*)rsa_decrypt(p2p_message_encrypt);
				sprintf(temp_len, "%d", strlen(p2p_message_encrypt));
				puts("send p2p message to server");
				puts(temp_len);
				puts(p2p_message_encrypt);
				puts("origin message");
				puts(p2p_message_origin);
				//puts(temp);
				send(socket_desc, p2p_message_encrypt, strlen(p2p_message_encrypt), 0);
				memset(p2p_message, 0, sizeof(p2p_message));
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
	RSA_free(publicRSA);
    RSA_free(privateRSA);
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
	unsigned char *client_message_encrypt = rsa_encrypt(client_message);
	unsigned char *temp = rsa_decrypt((char *)client_message_encrypt);
	char temp_len[10];
	sprintf(temp_len, "%d", int(strlen((const char*)client_message_encrypt)));
	puts("encrypted message");
	puts(temp_len);
	cout << client_message_encrypt << endl;
	puts("encrypt than decrypt");
	cout << temp << endl;
	//cout << client_message_encrypt << endl;
	send(socket_desc, client_message_encrypt, strlen((char*)client_message_encrypt), 0);
} 

void p2p_receiver_handler(void *socket_desc)
{
	int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000]={0}, temp_len[10];
    recv(sock , client_message , 2000 , 0);
    sprintf(temp_len, "%d", strlen(client_message));
    puts("receive from other client");
    puts(temp_len);
    puts(client_message);
    //client_message[strlen(client_message)] = 0;
    memset(p2p_message, 0, sizeof(p2p_message));
    strcpy(p2p_message, client_message);
    /*char *payer = strtok(client_message, "#");
    char *amount = strtok(NULL, "#");
    printf("%s wants to pay you %s dollars\n", payer, amount);*/
    memset(client_message, 0, sizeof(client_message));
    return;
}

unsigned char *rsa_encrypt(char *input)
{
	/*FILE *pub, *pri;
    RSA *publicRSA = nullptr, *privateRSA = nullptr;  // 因為回傳的是指標
    if((pub = fopen("pub.pem", "r")) == NULL) {
        cout << "pub Error" << endl;
        exit(-1);
    }
    if((pri = fopen("pri.pem", "r")) == NULL) {
        cout << "pri Error" << endl;
        exit(-1);
    }
    OpenSSL_add_all_algorithms();
    // 從 .pem 格式讀取公私鑰
    if((privateRSA = PEM_read_RSAPrivateKey(pri, NULL, NULL, NULL)) == NULL) { 
        cout << "Read pri error" << endl;
    }
    if((publicRSA = PEM_read_RSA_PUBKEY(pub, NULL, NULL, NULL)) == NULL) {
        cout << "Read pub error " << endl;
    }
    fclose(pri), fclose(pub);*/
    //cout << "pub: " << publicRSA << endl;
    //cout << "pri: " << privateRSA << endl;
    const unsigned char * src = (const unsigned char *)input;
    int rsa_len = RSA_size(publicRSA); // 幫你算可以加密 block 大小，字數超過要分開加密
    unsigned char * enc = (unsigned char *)malloc(rsa_len);
    if(RSA_public_encrypt(rsa_len-11, src, enc, publicRSA, RSA_PKCS1_PADDING) < 0){
    	cout << "enc error" << endl;
    }
    return enc;
}

unsigned char *rsa_decrypt(char *input)
{
	/*FILE *pub, *pri;
    RSA *publicRSA = nullptr, *privateRSA = nullptr;  // 因為回傳的是指標
    if((pub = fopen("pub.pem", "r")) == NULL) {
        cout << "pub Error" << endl;
        exit(-1);
    }
    if((pri = fopen("pri.pem", "r")) == NULL) {
        cout << "pri Error" << endl;
        exit(-1);
    }
    OpenSSL_add_all_algorithms();
    // 從 .pem 格式讀取公私鑰
    if((privateRSA = PEM_read_RSAPrivateKey(pri, NULL, NULL, NULL)) == NULL) { 
        cout << "Read pri error" << endl;
    }
    if((publicRSA = PEM_read_RSA_PUBKEY(pub, NULL, NULL, NULL)) == NULL) {
        cout << "Read pub error " << endl;
    }
    fclose(pri), fclose(pub);*/
    //cout << "pub: " << publicRSA << endl;
    //cout << "pri: " << privateRSA << endl;
    const unsigned char * enc = (const unsigned char *)input;
    int rsa_len = RSA_size(publicRSA); // 幫你算可以加密 block 大小，字數超過要分開加密
    unsigned char * dec = (unsigned char *)malloc(rsa_len);  
    if(RSA_private_decrypt(rsa_len, enc, dec, privateRSA, RSA_PKCS1_PADDING) < 0){
    	cout << "dec error" << endl;
    }
    return dec;
}