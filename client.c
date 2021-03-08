#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <signal.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <pthread.h>

#define SIZE 4096

//global
char user_name[32]; 
char password[32];
volatile sig_atomic_t flag = 0;
int socket_fd=0;
int wants_registration = 1; 

//utility function for printing an arrow at the start of each line
void print_arrow(){
	printf("->");
	fflush(stdout);
}


void sigint_handler(int signal)
{
	flag = 1;
}

void send_message_handler(void * arg)
{
	char message[SIZE] = "";
	char user_name_and_message[SIZE+34] = "";

	for(;;)
	{
		print_arrow();
		fgets(message, SIZE, stdin);
		message[strlen(message) - 1] = '\0';

		if(!strcmp(message, "/exit"))
		{
			break;
		}
		else if(!strcmp(message, "/all"))
		{
			send(socket_fd, message, strlen(message), 0);
		}
		else
		{
			sprintf(user_name_and_message, "%s: %s\n", user_name, message);
			send(socket_fd, user_name_and_message, strlen(user_name_and_message), 0);
		}

		memset(message, '\0', strlen(message));
		memset(user_name_and_message, '\0', strlen(user_name_and_message));

	}

	sigint_handler(2);
}

void receive_message_handler(void * arg)
{
	char server_message[SIZE] = "";
	for(;;)
	{
		int r;
		if((r=recv(socket_fd, server_message, SIZE, 0)) > 0)
		{
			printf("%s", server_message);
			print_arrow();
		}
		else if(r == 0)
		{
			break;
		}

		memset(server_message, '\0', strlen(server_message));
	}

	sigint_handler(2);
}

void registration(char * ip_address, int port_number)
{
	printf("REGISTRATION:\n");

	printf("Username: ");
	
	getchar();

	fgets(user_name, 32, stdin);
	user_name[strlen(user_name)-1]='\0'; //remove endl character

	printf("Password: ");

	fgets(password, 32, stdin);
	password[strlen(password)-1]='\0'; //remove endl character

	struct sockaddr_in server_address; 

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip_address);
	server_address.sin_port = htons(port_number);

	int connection;

	if((connection = connect(socket_fd, (struct sockaddr*) 
		&server_address, sizeof(server_address))) == -1)
	{
		perror("Connection to server failed.\n");
		exit(EXIT_FAILURE);
	}

	send(socket_fd, &wants_registration, sizeof(int), 0);

	send(socket_fd, user_name, sizeof(user_name), 0);

	send(socket_fd, password, sizeof(password), 0);

	pthread_t send_message_thread;

	if((pthread_create(&send_message_thread, NULL, (void *)send_message_handler,NULL)) != 0)
	{
		perror("Message sender thread failed.\n");
		exit(EXIT_FAILURE);
	}

	pthread_t receive_message_thread;

	if((pthread_create(&receive_message_thread, NULL, (void *)receive_message_handler, NULL)) != 0)
	{
		perror("Message receiver thread failed.\n");
		exit(EXIT_FAILURE);
	}	

	for(;;)
	{
		if(flag){
			printf("\nFarewell.\n");
			break;
		}
	}

	close(socket_fd);

	exit(EXIT_SUCCESS);
}

void login(char * ip_address, int port_number)
{
	printf("LOGIN.\n");
	printf("Username: ");
	getchar();

	fgets(user_name, 32, stdin);
	user_name[strlen(user_name)-1]='\0';
	
	printf("Password: ");

	fgets(password, 32, stdin);
	password[strlen(password)-1]='\0';
	
	struct sockaddr_in server_address; 

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip_address);
	server_address.sin_port = htons(port_number);

	int connection;

	if((connection = connect(socket_fd, (struct sockaddr*) 
		&server_address, sizeof(server_address))) == -1)
	{
		perror("Connection to server failed.\n");
		exit(EXIT_FAILURE);
	}

	wants_registration = 0;

	send(socket_fd, &wants_registration, sizeof(int), 0);

	send(socket_fd, user_name, sizeof(user_name), 0);

	send(socket_fd, password, sizeof(password), 0);

	pthread_t send_message_thread;

	if(pthread_create(&send_message_thread, NULL, (void *)send_message_handler,NULL) != 0)
	{
		perror("Message sender thread failed.\n");
		exit(EXIT_FAILURE);
	}

	pthread_t receive_message_thread;

	if(pthread_create(&receive_message_thread, NULL, (void *)receive_message_handler, NULL) != 0)
	{
		perror("Message receiver thread failed.\n");
		exit(EXIT_FAILURE);
	}	

	for(;;)
	{
		if(flag){
			printf("\nFarewell.\n");
			break;
		}
	}

	close(socket_fd);

	exit(EXIT_SUCCESS);

}

void menu(char *ip_address, int port_number)
{

  int option;

  printf("== WELCOME TO THE FORUM ==\n");
  printf("1.Registration\n");
  printf("2.Login\n");
  printf("3.Exit\n");
  while (1) 
  {

  	printf("\nYour option:");
  	scanf("%d", & option);

    	switch (option) 
    	{

    	case 1:
      	registration(ip_address, port_number);
      	break;
    	case 2:
      	login(ip_address, port_number);
      	break;
    	case 3:
      	exit(EXIT_FAILURE);
      	break;
    	default:
      	printf("Wrong option!\n");

    	}
	}
}

int main(int argc, char **argv){

	if(argc != 2)
	{
		printf("Run program as such: ./%s <port_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *ip_address = "0.0.0.0";

	int port_number = atoi(argv[1]);

	signal(SIGINT, sigint_handler);

	menu(ip_address, port_number);


return 0;

}
