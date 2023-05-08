#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include<dirent.h>

#define MAX_COMMANDS 128
#define MAX_ARGS 256
#define MAX_COMMAND_LENGTH 256
#define MAX_INPUT_LENGTH 5000

#define PORT 1122
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define SERVER_ADDR "127.0.0.1"

typedef struct {
    int socket_fd;
    char name[20];
    int id;
} client_info;

client_info client_list[MAX_CLIENTS];
int num_clients = 0;
char *path[] = {"bin/", "./", NULL};


void *handle_client(void *arg) {
    int socket_fd = (int)arg;
    char buffer[1024];
    int read_size;
    int client_index;
    int my_id;

    // Get client address

    // Add client to list of active clients
    if (num_clients < MAX_CLIENTS) {
        client_list[num_clients].socket_fd = socket_fd;
        sprintf(client_list[num_clients].name, "no_name");
        client_list[num_clients].id = num_clients;
        my_id = num_clients;
        num_clients++;
    } else {
        write(socket_fd, "Server full\n", strlen("Server full\n"));
        close(socket_fd);
        pthread_exit(NULL);
    }

    // Find client index
    for (int i = 0; i < num_clients; i++) {
        if (client_list[i].socket_fd == socket_fd) {
            client_index = i;
            break;
        }
    }
    write(socket_fd, "% ", 2);
    // Handle client connection
    while (1) {
        
        read_size = recv(socket_fd, &buffer, 1024, 0);
        printf("%d",read_size);
        if (read_size <= 0) {
            // Client disconnected
            break;
        }
        //memmove(buffer,buffer+2,sizeof buffer - sizeof *buffer);
        buffer[read_size-2] = '\0';  // Make sure buffer is null-terminated
        
        // Parse message
        char *command = strtok(buffer, " \n");
        char *param = strtok(NULL,"\n");
        //printf("%s",command);

        // Handle command
        if (strcmp(command, "name") == 0) {
            // Update client name
            char *name = strtok(param, "\n");
            printf("%s\n",name);
            if (name == NULL) {
                write(socket_fd, "Invalid command\n", strlen("Invalid command\n"));
                write(socket_fd, "% ", 2);
            } else {
                int name_taken = 0;
                for (int i = 0; i < num_clients; i++) {
                    if (strcmp(client_list[i].name, name) == 0 && i != client_index) {
                        name_taken = 1;
                        break;
                    }
                }
                if (name_taken) {
                    write(socket_fd, "Name already taken\n", strlen("Name already taken\n"));
                } else {
                    strcpy(client_list[client_index].name, name);
                    write(socket_fd, "Name updated\n", strlen("Name updated\n"));
                }
                write(socket_fd, "% ", 2);
            }
        } else if (strcmp(command, "who") == 0) {
            // Send list of connected clients
            char list[1024] = "";
            for (int i = 0; i < num_clients; i++) {
                char me_indicator[5] = "";
                if (client_list[i].id == my_id) {
                    strcpy(me_indicator, " <-me");
                }
                char client_info[50] = "";
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                getpeername(client_list[i].socket_fd, (struct sockaddr *)&client_addr, &addr_len);
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
                int port = ntohs(client_addr.sin_port);
                sprintf(client_info, "%d\t%s\t%s:%d%s\n", client_list[i].id, client_list[i].name, ip_str, port, me_indicator);
                strcat(list, client_info);
            }
            write(socket_fd, list, strlen(list));
            write(socket_fd, "% ", 2);
        }else if (strcmp(command, "tell") == 0) {
    // Send private message
    char *to_id_str = strtok(param, " ");
    char *msg = strtok(NULL, "");
    if (to_id_str == NULL || msg == NULL) {
        continue;
    }
    int to_id = atoi(to_id_str);
    if (to_id >= num_clients || to_id < 0 || to_id == my_id) {
        write(socket_fd, "Can't write message to yourself.\n", strlen("Can't write message to yourself.\n"));
    } else {
        char message[1024];
        sprintf(message, "<user(%d) told you>: %s\n", my_id, msg);
        write(client_list[to_id].socket_fd, message, strlen(message));
        write(client_list[to_id].socket_fd, "% ", 2);
        write(socket_fd, "send accept\n", strlen("send accept\n"));
        //write(socket_fd, "% ", 2);
    }
    write(socket_fd, "% ", 2);
}else if (strcmp(command, "yell") == 0) {
            // Send message to all connected clients
            if (param == NULL) {
                write(socket_fd, "Invalid command\n", strlen("Invalid command\n"));
            } else {
                char message[1024];
                sprintf(message, "<User(%d)> yells %s\n", client_list[client_index].id, param);
                for (int i = 0; i < num_clients; i++) {
                    write(client_list[i].socket_fd, message, strlen(message));
                    write(client_list[i].socket_fd,"% ",2);
                }
                // write(socket_fd, "% ", 2);
            }
        }  else if (strcmp(command, "msg") == 0) {
            // Send message to specific client
            char *id_str = strtok(param, " \n");
            char *msg = strtok(NULL, "\n");
            if (id_str == NULL || msg == NULL) {
                write(socket_fd, "Invalid command\n", strlen("Invalid command\n"));
            } else {
                int id = atoi(id_str);
                int client_fd = -1;
                for (int i = 0; i < num_clients; i++) {
                    if (client_list[i].id == id) {
                        client_fd = client_list[i].socket_fd;
                        break;
                    }
                }
                if (client_fd == -1) {
                    write(socket_fd, "Invalid user ID\n", strlen("Invalid user ID\n"));
                } else {
                    char msg_buf[1024];
                    sprintf(msg_buf, "%s: %s\n", client_list[client_index].name, msg);
                    write(client_fd, msg_buf, strlen(msg_buf));
                }
                write(socket_fd, "% ", 2);
            }
        }else if(strcmp(command,"quit") == 0){
            break;
        } else if(strcmp(command, "ls") == 0){
            DIR *dir;
            struct dirent *ent;
            char dirlist[1024];
            if ((dir = opendir(".")) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    char dir_info[100] = "";
                    sprintf(dir_info,"%s\n",ent->d_name);
                    strcat(dirlist, dir_info);
                }
                write(socket_fd,dirlist,strlen(dirlist));              
                closedir(dir);
            }
            write(socket_fd, "% ", 2);
        }else if (strncmp(command, "cd ", 3) == 0) {
                char *dir = command + 3;
                if (chdir(dir) == -1) {
                    perror("chdir");
                }
                write(socket_fd, "% ", 2);
            } else if (strcmp(command, "setenv") == 0) {
                char *var = param;
                char *val = strtok(param, " ");
                if (var != NULL && val != NULL) {
                    if (setenv(var, val, 1) != 0) {
                        perror("setenv");
                        exit(1);
                    }
                }
                write(socket_fd, "% ", 2);
            } else if (strcmp(command, "printenv") == 0) {
                char *var = param;
                if (var != NULL) {
                    char *value = getenv(var);
                    if (value != NULL) {
                        write(socket_fd,value,strlen(value));
                    }
                }
                write(socket_fd, "% ", 2);
            }else{
                write(socket_fd, "Invalid command\n", strlen("Invalid command\n"));
                write(socket_fd, "% ", 2);
            }
    }

    // Remove client from list of active clients
    for (int i = 0; i < num_clients; i++) {
        if (client_list[i].id == my_id) {
            for (int j = i; j < num_clients - 1; j++) {
                client_list[j] = client_list[j+1];
            }
            num_clients--;
            break;
        }
    }

    close(socket_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    int addrlen = sizeof(server_addr);
    pthread_t thread_id;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    // Bind socket to address and port
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(PORT);

if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
}

// Listen for incoming connections
if (listen(server_fd, MAX_CLIENTS) < 0) {
    perror("listen failed");
    exit(EXIT_FAILURE);
}

// Handle incoming connections
while (1) {
    printf("Waiting for incoming connections...\n");

    // Accept incoming connection
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Add client to list of active clients
    client_list[num_clients].socket_fd = client_fd;
    snprintf(client_list[num_clients].name, sizeof(client_list[num_clients].name), "client%d", client_fd);
    client_list[num_clients].id = num_clients;
    num_clients++;
    
    

    // Create thread to handle client communication
    if (pthread_create(&thread_id, NULL, handle_client, (void *)client_fd) < 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
}

return 0;
}