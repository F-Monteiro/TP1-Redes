#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

int **server_map = NULL, **player_map = NULL;
int rows = 0, cols = 0;

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void start_map(char* filename) {
    FILE *file;
    int value;
    
    file = fopen(filename, "r");

    char ch;
    while (fscanf(file, "%d", &value) == 1) { //Contar número de colunas
        cols++;
        if (fgetc(file) == '\n') {
            break;
        }
    }
    rows++;
    while ((ch = fgetc(file)) != EOF) { //Contar número de linhas
        if (ch == '\n') {
            rows++;
        }
    }
    fseek(file, 0, SEEK_SET);

    server_map = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        server_map[i] = (int *)malloc(cols * sizeof(int));
    }

    player_map = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        player_map[i] = (int *)malloc(cols * sizeof(int));
    }

    fseek(file, 0, SEEK_SET);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            player_map[i][j] = 4;
        }
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(file, "%d", &server_map[i][j]);
            if (server_map[i][j] == 2){
                player_map[i][j] = 5;
            }
        }
    }

    fclose(file);
}

void update_player_map() {
    int i1, i2, j1, j2, quebra_loop = 0;
    for (i1 = 0; i1 < rows; i1++) { //Achar local do jogador
        for (j1 = 0; j1 < cols; j1++) {
            if (player_map[i1][j1] == 5){
                quebra_loop = 1;
                break;
            }
        }
        if (quebra_loop == 1){
            break;
        }
    }

    for (i2 = i1 - 1; i2 <= i1 + 1; i2++) { //Atualizar visão ao redor do jogador
        for (j2 = j1 - 1; j2 <= j1 + 1; j2++) {
            if (i2 >= 0 && i2 < rows && j2 >= 0 && j2 < cols){
                if(player_map[i2][j2] == 4){
                    player_map[i2][j2] = server_map[i2][j2];
                }
            }
        }
    }
}

action check_player_moves() {
    int i, j, quebra_loop = 0;
    for (i = 0; i < rows; i++) { //Achar local do jogador
        for (j = 0; j < cols; j++) {
            if (player_map[i][j] == 5){
                quebra_loop = 1;
                break;
            }
        }
        if (quebra_loop == 1){
            break;
        }
    }
    
    action Possible = empty_action();
    
    int move = 0;
    if(i - 1 >= 0){ // Cima = 1
        if(server_map[i-1][j] == 1){
            Possible.moves[move] = 1;
            move++;
        }
    }
    if(j + 1 < rows){ // Direita = 2
        if(server_map[i][j+1] == 1){
            Possible.moves[move] = 2;
            move++;
        }
    }
    if(i + 1 < cols){ // Baixo = 3
        if(server_map[i+1][j] == 1){
            Possible.moves[move] = 3;
            move++;
        }
    }
    if(j - 1 >= 0){ // Esquerda = 4
        if(server_map[i][j-1] == 1){
            Possible.moves[move] = 4;
            move++;
        }
    }
    return Possible;
}

action win() {
    action winner = empty_action();
    winner.type = 5;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            winner.board[i][j] = server_map[i][j];
        }
    }
    return winner;
}

action move_player(action received){
    int i, j, quebra_loop;
    action update = empty_action();
    for (i = 0; i < rows; i++) { //Achar local do jogador
        for (j = 0; j < cols; j++) {
            if (player_map[i][j] == 5){
                quebra_loop = 1;
                break;
            }
        }
        if (quebra_loop == 1){
            break;
        }
    }
    player_map[i][j] = server_map[i][j];
    switch (received.moves[0])
    {
    case 1:
        i--;
        break;
    case 2:
        j++;
        break;
    case 3:
        i++;
        break;
    case 4:
        j--;
        break;
    }
    player_map[i][j] = 5;
    if(server_map[i][j] == 3){
        update = win();
    } else {
        update_player_map();
        update = check_player_moves();
        update.type = 4;
    }
    
    return update;    
}

action send_map(){
    action Map = empty_action();
    Map.type = 4;
    int i, j;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            Map.board[i][j] = player_map[i][j];
        }
    }
    return Map;
}

void restart_game(){
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            player_map[i][j] = 4;
            if (server_map[i][j] == 2){
                player_map[i][j] = 5;
            }
        }
    }
}

action read_command(action received){
	action message = empty_action();
	if (received.type == 0) { //Start
        update_player_map();
        message = check_player_moves();
    } else if (received.type == 1) { //Move
        message = move_player(received);
    } else if (received.type == 2){ //Map
		message = send_map();
	} else if(received.type == 3){ //Hint

		/*Put hint function here after I implement it*/
	
    } else if (received.type == 6){ //Reset
        restart_game();
        update_player_map();
        message = check_player_moves();
    }
	return message;
}

int main(int argc, char **argv) {
    if (argc < 5 || strcmp(argv[3], "-i") != 0) {
        usage(argc, argv);
    }

    start_map(argv[4]);  //It's fine up to here, map printed fine on server side

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        printf("client connected\n");

        while (1) {
            action received, message;
            ssize_t count = recv(csock, &received, sizeof(received), 0);
            message = read_command(received);  //read_command simply doesn't work

            memset(message.moves, 0, sizeof(message.moves));
            memset(message.board, 0, sizeof(message.board));

            count = send(csock, &message, sizeof(message), 0);
            if (count != sizeof(message)) {
                logexit("send");
            }
        }

        close(csock);
    }

    exit(EXIT_SUCCESS);
}
