#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMANDS 128
#define MAX_ARGS 256
#define MAX_COMMAND_LENGTH 256
#define MAX_INPUT_LENGTH 5000

char *path[] = {"bin/", "./", NULL};
char prompt[] = "% ";

void print_prompt() {
    printf("%s", prompt);
    fflush(stdout);
}

void execute_command(char *command) {
    char *args[MAX_ARGS];
    int arg_count = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    int i = 0;
    while (path[i] != NULL) {
        char *path_with_command = malloc(strlen(path[i]) + strlen(args[0]) + 1);
        strcpy(path_with_command, path[i]);
        strcat(path_with_command, args[0]);
        execv(path_with_command, args);
        free(path_with_command);
        i++;
    }

    if(isdigit(args[0])){
        int j = atoi(args[0]);
        while(j--){
            //
        }
    }else{
        printf("Unknown command: %s\n", args[0]);
        exit(1);
    }

    
}

void execute_pipe(char **commands, int num_commands) {
    int fds[num_commands - 1][2];
    int i, j, k;
    for (i = 0; i < num_commands - 1; i++) {
        if (pipe(fds[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    for (i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            if (i == 0) {
                dup2(fds[i][1], STDOUT_FILENO);
            } else if (i == num_commands - 1) {
                if (commands[i - 1][strlen(commands[i - 1]) - 1] != '|') {
                    dup2(fds[i - 1][0], STDIN_FILENO);
                }
            } else {
                dup2(fds[i - 1][0], STDIN_FILENO);
                dup2(fds[i][1], STDOUT_FILENO);
            }

            for (j = 0; j < num_commands - 1; j++) {
                close(fds[j][0]);
                close(fds[j][1]);
            }

            // Remove pipe number from command
            char *command = malloc(strlen(commands[i]) + 1);
            strcpy(command, commands[i]);
            char *pipe_num_loc = strstr(command, " |");
            if (pipe_num_loc != NULL) {
                *pipe_num_loc = '\0';
            }

            execute_command(command);
        }
    }
    for (j = 0; j < num_commands - 1; j++) {
        close(fds[j][0]);
        close(fds[j][1]);
    }
    for (k = 0; k < num_commands; k++) {
        wait(NULL);
    }
}


void set_env(char *name, char *value) {
    if (setenv(name, value, 1) != 0) {
        perror("setenv");
        exit(1);
    }
}

void print_env(char *name) {
    char *value = getenv(name);
    if (value != NULL) {
        printf("%s\n", value);
    }
}

void parse_input(char *input, char **commands, int *num_commands) {
    *num_commands = 0;

    char *token = strtok(input, "|");
    while (token != NULL) {
        // Trim leading and trailing whitespace from command
        while (*token == ' ') {
            token++;
        }
        int len = strlen(token);
        while (len > 0 && token[len - 1] == ' ') {
            token[--len] = '\0';
        }

        commands[*num_commands] = token;
        (*num_commands)++;
        token = strtok(NULL, "|");
    }
}


int main() {
    char input[MAX_INPUT_LENGTH];
    char *commands[MAX_COMMANDS];
    int num_commands = 0;

    while (1) {
        print_prompt();

        if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                exit(0);
            } else {
                perror("fgets");
                exit(1);
            }
        }

        input[strcspn(input, "\n")] = '\0'; // Remove newline character

        // Parse input into commands
        parse_input(input, commands, &num_commands);

        if (num_commands == 1) {
            char *command = commands[0];
            if (strcmp(command, "exit") == 0) {
                exit(0);
            } else if (strncmp(command, "cd ", 3) == 0) {
                char *dir = command + 3;
                if (chdir(dir) == -1) {
                    perror("chdir");
                }
            } else if (strncmp(command, "setenv ", 7) == 0) {
                char *var = strtok(command + 7, " ");
                char *val = strtok(NULL, " ");
                if (var != NULL && val != NULL) {
                    set_env(var, val);
                }
            } else if (strncmp(command, "printenv ", 9) == 0) {
                char *var = strtok(command + 9, " ");
                if (var != NULL) {
                    print_env(var);
                }
            } else if (strcmp(command, "ls") == 0)
            {
                execute_command(command);
            } else {
                execute_command(command);
            }
        } else {
            execute_pipe(commands, num_commands);
        }

        // Reset the commands array and num_commands counter
        memset(commands, 0, sizeof(commands));
        num_commands = 0;
    }

    return 0;
}