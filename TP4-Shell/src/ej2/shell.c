#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 100

void parse_args(char *input, char *args[], int *argc) {
    *argc = 0;
    char *p = input;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '\0') break;

        char *start;
        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            start = p;
            while (*p && *p != quote) p++;
            if (*p == quote) {
                *p = '\0';
                args[(*argc)++] = start;
                p++;
            } else {
                fprintf(stderr, "Error: comillas sin cerrar\n");
                exit(EXIT_FAILURE);
            }
        } else {
            start = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
            args[(*argc)++] = start;
        }
    }
    args[*argc] = NULL;
}

// Separa la línea en comandos por pipe respetando comillas
int split_commands(char *linea, char *commands[]) {
    int count = 0;
    bool in_quotes = false;
    char quote_char = '\0';
    char *start = linea;

    for (char *p = linea; ; p++) {
        if (*p == '\0') {
            commands[count++] = start;
            break;
        }
        if (*p == '"' || *p == '\'') {
            if (in_quotes && *p == quote_char) {
                in_quotes = false;
                quote_char = '\0';
            } else if (!in_quotes) {
                in_quotes = true;
                quote_char = *p;
            }
        }
        else if (*p == '|' && !in_quotes) {
            *p = '\0';
            commands[count++] = start;
            start = p + 1;
        }
    }

    return count;
}

int validar_sintaxis_pipes(const char *linea) {
    int len = strlen(linea);
    for (int i = 0; i < len; i++) {
        if (!isspace(linea[i])) {
            if (linea[i] == '|') {
                fprintf(stderr, "Error: pipe al inicio no permitido\n");
                return 0;
            }
            break;
        }
    }
    for (int i = len - 1; i >= 0; i--) {
        if (!isspace(linea[i])) {
            if (linea[i] == '|') {
                fprintf(stderr, "Error: pipe al final no permitido\n");
                return 0;
            }
            break;
        }
    }
    for (int i = 0; i < len - 1; i++) {
        if (linea[i] == '|' && linea[i + 1] == '|') {
            fprintf(stderr, "Error: pipes consecutivos no permitidos\n");
            return 0;
        }
    }
    return 1;
}

int main() {
    char command[4096];
    char *commands[MAX_COMMANDS];
    int command_count;

    while (1) {
        fprintf(stderr, "Shell> ");
        fflush(stderr);

        if (!fgets(command, sizeof(command), stdin)) break;

        command[strcspn(command, "\n")] = '\0';  // Eliminar '\n'

        // Salida si solo se escribió "exit" (ignorando espacios)
        char *trim = command;
        while (isspace(*trim)) trim++;
        if (strcmp(trim, "exit") == 0) break;

        if (!validar_sintaxis_pipes(command)) continue;

        // Aquí usamos la nueva función para separar comandos por pipe respetando comillas
        command_count = split_commands(command, commands);

        for (int i = 0; i < command_count; i++) {
            while (isspace(*commands[i])) commands[i]++;
            if (*commands[i] == '\0') {
                fprintf(stderr, "Error: comando vacío entre pipes\n");
                command_count = 0;
                break;
            }
        }

        if (command_count == 0) continue;

        int prev_fd = -1;
        pid_t pids[MAX_COMMANDS];

        for (int i = 0; i < command_count; i++) {
            int pipefd[2];

            if (i < command_count - 1) {
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                if (prev_fd != -1) {
                    dup2(prev_fd, STDIN_FILENO);
                    close(prev_fd);
                }

                if (i < command_count - 1) {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);
                }

                char *args[MAX_ARGS];
                int argc;
                parse_args(commands[i], args, &argc);

                if (argc == 0) {
                    fprintf(stderr, "Error: comando vacío\n");
                    exit(EXIT_FAILURE);
                }

                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            }

            if (prev_fd != -1) close(prev_fd);
            if (i < command_count - 1) {
                close(pipefd[1]);
                prev_fd = pipefd[0];
            }

            pids[i] = pid;
        }

        for (int i = 0; i < command_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }

    return 0;
}
