#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <pthread.h>

#include <sys/types.h>

#include <signal.h>

#define MAX 100

#define SIZE 4096

#define FILE_NAME "dbase"

//global
static _Atomic unsigned int client_counter = 0;
static int user_id = 0;
FILE *dbase;
int wants_registration;

typedef struct{
	struct sockaddr_in client_address;
	int socket_fd;
	int user_id;
	char user_name[32];
	char password[32];
}client;

client * client_array[MAX];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

char * encrypt(char * string, int offset)
{
	for (int i = 0; i < strlen(string); i++) 
	{
    string[i] = string[i] + offset++;
    }

  	return string;
}

char * decrypt(char * string, int offset) 
{

  for (int i = 0; i < strlen(string); i++) 
  {
    string[i] = string[i] - offset++;
  }

  return string;
}

void print_client_address(struct sockaddr_in client_address)
{
	printf("%d.%d.%d.%d\n",
		(client_address.sin_addr.s_addr & 0xFF),
	    (client_address.sin_addr.s_addr & 0xFF00) >> 8,
	    (client_address.sin_addr.s_addr & 0xFF0000) >> 16,
	    (client_address.sin_addr.s_addr & 0xFF000000) >> 24);
}

void add_client_to_queue(client *instc_client)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX; i++)
	{
		if(!client_array[i])
		{
			client_array[i] = instc_client;
			break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

void remove_client_from_queue(int user_id)
{
  
  pthread_mutex_lock( & clients_mutex);

  for (int i = 0; i < MAX; i++) 
  {
    if(client_array[i]) 
    {
      if(client_array[i] -> user_id == user_id) 
      {
        client_array[i] = NULL;
        break;
      }
    }
  }

  pthread_mutex_unlock( & clients_mutex);

}

void send_message_to_other_clients(char * server_message, int user_id)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX; i++)
	{
		if(client_array[i])
		{
			if(client_array[i] -> user_id != user_id)
			{
				if((write(client_array[i]->socket_fd, server_message, strlen(server_message))) < 0)
				{
					perror("Message sent incorrectly\n");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_sender(char * server_message, int user_id)
{
  pthread_mutex_lock(&clients_mutex);

  for (int i = 0; i < MAX; ++i) 
  {
    if (client_array[i]) 
    {
      if (client_array[i] -> user_id == user_id) 
      {
        if (write(client_array[i] -> socket_fd, server_message, strlen(server_message)) < 0) 
        {
          perror("Message sent to sender failed.\n");
          break;
        }
      }
    }
  }

  pthread_mutex_unlock(&clients_mutex);
}

void write_credentials(char *user_name, char *password, client * instc_client)
{
  
  dbase = fopen(FILE_NAME, "a");
  char password_copy[32];
  strcpy(password_copy, password);
  fprintf(dbase, "%s %s\n", user_name, encrypt(password_copy, 3));
  
  fclose(dbase);

}

int find_credentials(char * user_name, char * password)
{
  char local_user_name[32];
  char local_password[32];

  dbase = fopen(FILE_NAME, "r");

  while (!feof(dbase)) 
  {
    fscanf(dbase, "%s %s\n", local_user_name, local_password);
    char local_password_copy[32];
    strcpy(local_password_copy, local_password);
    if (!strcmp(local_user_name, user_name) && !strcmp(decrypt(local_password_copy, 3), password)) 
    {

      fclose(dbase);
      return 1;
    }
  }

  fclose(dbase);
  return 0;
}

void client_handler(void * arg)
{

	char server_message[SIZE];
	char user_name[32];
	char password[32];
	int  exit_flag = 0;

	client_counter++;

	client * instc_client = (client *) arg;

	recv(instc_client -> socket_fd, &wants_registration, sizeof(int), 0);

	if(recv(instc_client -> socket_fd, user_name, 32, 0) <= 0 || strlen(user_name) > 32)
	{
		perror("Invalid username\n");
		exit_flag = 1;
	}

	if(recv(instc_client -> socket_fd, password, 32, 0) <= 0 || strlen(password) < 6 || strlen(password) > 32)
	{
		perror("Invalid password\n");
		exit_flag = 1;
	}

	if(!exit_flag)
	{
		if(wants_registration)
		{
			write_credentials(user_name, password, instc_client);
		}
		strcpy(instc_client->user_name, user_name);
		strcpy(instc_client->password, password);
		if(!find_credentials(instc_client->user_name, instc_client->password))
		{
			exit_flag = 1;
		}
		else
		{
			sprintf(server_message, "%s has joined.\n", instc_client->user_name);
			printf("%s", server_message);
			send_message_to_other_clients(server_message, instc_client->user_id);
		}
	}

	memset(server_message, '\0', SIZE);

	for(;;)
	{
		if(exit_flag)
			break;
	

	int r;
	if((r = recv(instc_client->socket_fd, server_message, SIZE, 0)) > 0)
	{

		if(!strcmp(server_message, "/all"))
		{

		char clients_list[10000];

        strcpy(clients_list, "");

        strcat(clients_list, "\n");

        for (int i = 0; i < MAX; ++i) 
        {

          if(client_array[i]) 
          {
            strcat(clients_list, client_array[i] -> user_name);
            strcat(clients_list, "\n");
          }
        }

        send_message_to_sender(clients_list, instc_client -> user_id);
		}
		else if(strlen(server_message) > 0)
		{
			send_message_to_other_clients(server_message, instc_client->user_id);
			server_message[strlen(server_message) - 1] = '\0';
			printf("%s\n", server_message);
		}
	}
	else if(r == 0)
	{
		sprintf(server_message, "%s has left.\n", instc_client->user_name);
		printf("%s", server_message);
		send_message_to_other_clients(server_message, instc_client->user_id);

		exit_flag = 1;
	}
	else
	{
		perror("Receiving message failed.\n");
		exit_flag = 1;
	}

	memset(server_message, '\0', SIZE);

	}


	close(instc_client->socket_fd);
	remove_client_from_queue(instc_client->user_id);
	free(instc_client);
	client_counter--;
	pthread_detach(pthread_self());

}

int main(int argc, char **argv)
{

	if(argc != 2)
	{
		printf("Run program as such: ./%s <port_number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char * ip_address = "0.0.0.0";

	int port_number = atoi(argv[1]);

	int option = 1;

	int listen_fd = 0;

	int connection_fd = 0;

	struct sockaddr_in server_address;

	struct sockaddr_in client_address;

	pthread_t client_thread;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ip_address);
	server_address.sin_port = htons(port_number);

	signal(SIGPIPE,SIG_IGN);

	if((setsockopt(listen_fd,SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*) &option, sizeof(option))) < 0)
	{
		perror("Error at setting socket option\n");
		exit(EXIT_FAILURE);
	}

	if((bind(listen_fd, (struct sockaddr*)&server_address, sizeof(server_address))) < 0)
	{
		perror("Error at binding socket\n");
		exit(EXIT_FAILURE);
	}

	if((listen(listen_fd, 10)) < 0)
	{
		perror("Error at socket listening\n");
		exit(EXIT_FAILURE);
	}

	printf("=== WELCOME TO THE FORUM ===\n");

	for(;;)
	{
		socklen_t client_length = sizeof(client_address);
		connection_fd = accept(listen_fd, (struct sockaddr*) & client_address, &client_length);

		if((client_counter+1) == MAX)
		{
			printf("Number of clients exceeded.\n");
			printf("Client address - ");
			print_client_address(client_address);
			printf(":%d\n", client_address.sin_port);
			close(connection_fd);
			continue;
		}

		client * instc_client = (client *) malloc(sizeof(client));
		instc_client -> client_address = client_address;
		instc_client -> socket_fd = connection_fd;
		instc_client -> user_id = user_id++;

		add_client_to_queue(instc_client);
		pthread_create(&client_thread, NULL, (void*)client_handler, instc_client);
	
		sleep(1);
	}

return 0;
}