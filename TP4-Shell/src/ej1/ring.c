#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);     // Número de procesos hijos
    int value = atoi(argv[2]); // Valor inicial
    int start = atoi(argv[3]); // Índice del proceso que inicia

    if (n < 3 || start < 0 || start >= n) {
        fprintf(stderr, "Error: n debe ser al menos 3 y 0 <= s < n\n");
        exit(EXIT_FAILURE);
    }

    int pipes[n][2];           // Anillo de pipes
    int padre_a_hijo[2];       // Pipe padre → hijo[start]
    int hijo_a_padre[2];       // Pipe hijo[last] → padre

    int last = (start + n - 1) % n; // Último en el anillo

    // Crear pipes
    for (int i = 0; i < n; i++)
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

    if (pipe(padre_a_hijo) == -1 || pipe(hijo_a_padre) == -1) {
        perror("pipe extra");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Proceso hijo i
            int next = (i + 1) % n;

            // Cerrar pipes innecesarios
            for (int j = 0; j < n; j++) {
                if (j != i) close(pipes[j][0]);     // leo solo de pipes[i][0]
                if (j != next) close(pipes[j][1]);  // escribo solo a pipes[next][1]
            }

            close(padre_a_hijo[1]); // cerrar escritura
            close(hijo_a_padre[0]); // cerrar lectura

            int num;

            if (i == start)
                read(padre_a_hijo[0], &num, sizeof(int));
            else
                read(pipes[i][0], &num, sizeof(int));

            close(pipes[i][0]);
            close(padre_a_hijo[0]);

            num++; // incrementar

            if (i == last)
                write(hijo_a_padre[1], &num, sizeof(int));
            else
                write(pipes[next][1], &num, sizeof(int));

            close(pipes[next][1]);
            close(hijo_a_padre[1]);
            exit(0);
        }
    }

    // Proceso padre
    close(padre_a_hijo[0]); // solo escribe
    close(hijo_a_padre[1]); // solo lee

    write(padre_a_hijo[1], &value, sizeof(int));
    close(padre_a_hijo[1]);

    int final_value;
    read(hijo_a_padre[0], &final_value, sizeof(int));
    close(hijo_a_padre[0]);

    printf("%d\n", final_value);

    // Cerrar pipes y esperar hijos
    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < n; i++)
        wait(NULL);

    return 0;
}
