#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

#define BUFSZ 1024

void print_moves(action possible_moves){
    printf("Possible moves: ");
    int more_than_one = 0;
    for(int i = 0; i < 100 && possible_moves.moves[i] != 0; i++){
        if(more_than_one == 1){
            printf(", ");
        }
        switch (possible_moves.moves[i])
        {
        case 1:
            printf("up");
            break;
        case 2:
            printf("right");
            break;
        case 3:
            printf("down");
            break;
        case 4:
            printf("left");
            break;
        default:
            printf("%d",possible_moves.moves[i]);
            break;
        }
        more_than_one = 1;
    }
    printf(".\n");
}

action send_move(action possible_moves, int move){
	int i, possible = 0;
	action movement = empty_action();
	
	for (i=0;possible_moves.moves[i] != 0; i++){
		if(possible_moves.moves[i] == move){
			possible = 1;
		}
	}
	if(possible == 0){
		printf("error: you cannot go this way \n");
		movement.type = 9;
	} else{
		movement.moves[0] = move;
		movement.type = 1;
	}
	return movement;
}

action read_command(action received, int game_won, int game_started){
	char command[50];
	action message = empty_action();
	scanf("%s", command);
	if(game_started == 0 && strcmp(command, "start") == 0){
		message.type = 0;
	} else if (game_started == 0){ //If did not start a game, only accept start
		message.type = 9;
		printf("error: start the game first\n");
		return message;
	} else if (strcmp(command, "reset") == 0) {
        message.type = 6;
    } else if (strcmp(command, "exit") == 0) {
        message.type = 7;
    } else if (game_won == 1){ //If you win a game, only accept reset or exit
		message.type = 9;
		return message;
	} else if (strcmp(command, "up") == 0) {
        message = send_move(received, 1);
    } else if (strcmp(command, "right") == 0) {
        message = send_move(received, 2);
    } else if (strcmp(command, "down") == 0) {
        message = send_move(received, 3);
	} else if (strcmp(command, "left") == 0) {
        message = send_move(received, 4);
    } else if (strcmp(command, "map") == 0) {
        message.type = 2;
    } else if (strcmp(command, "hint") == 0) {
        message.type = 3;
    } else {
		printf("error: command not found \n");
		message.type = 9;
	}
	return message;
}

void print_map(action received){
    char symbols[6] = {'#', '_', '>', 'X', '?', '+'};
    int number, i, j;
    int rows = sizeof(received.board) / sizeof(received.board[0]);
    int cols = sizeof(received.board[0]) / sizeof(received.board[0][0]);
	for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            number = received.board[i][j];
            printf ("%c ", symbols[number]);
        }
        printf("\n");
    }
}

void print_hint(action received){
	printf("Hint: ");
    int more_than_one = 0;
    for(int i = 0; i < 100 && received.moves[i] != 0; i++){
        if(more_than_one == 1){
            printf(", ");
        }
        switch (received.moves[i])
        {
        case 1:
            printf("up");
            break;
        case 2:
            printf("right");
            break;
        case 3:
            printf("down");
            break;
        case 4:
            printf("left");
            break;
        default:
            printf("%d",received.moves[i]);
            break;
        }
        more_than_one = 1;
    }
    printf(".\n");
}

void print_win(action received){
	printf("You escaped!");
	print_map(received);
}

int receive_message(action received){
	int win = 0;
	if (received.type == 2){ //map
		print_map(received);
	}
	if (received.type == 3){ //hint
		print_hint(received);
	}
	if (received.type == 4){ //Print possible moves (start/move)
		print_moves(received);
	}
	if (received.type == 5){ //win
		print_win(received);
		win = 1;
	}
	return win;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);

	action message, response;
	memset(&message, 0, sizeof(action));

	int game_started = 0;
	int game_won = 0;

	ssize_t count;

	while(1){

		//Prepare the message
		message = read_command(response, game_won, game_started);
		if(message.type != 0){
			game_started = 1;
		}
		if(message.type != 9){
			count = send(s, &message, sizeof(message), 0);
        	if (count != sizeof(message)) {
            	logexit("send");
        	}
		}

		if (message.type == 7) {
            break;
        }

		if(message.type != 9 && game_started == 1){
			memset(&response, 0, sizeof(response));
			ssize_t total = 0;
			while (total < sizeof(response)) {
				count = recv(s, ((char *)&response) + total, sizeof(response) - total, 0);
				if (count == 0) {
					// Connection terminated
					break;
				}
				total += count;
			}
			printf("Message received");
			
			game_won = receive_message(response);
		}
	}
	close(s);
	exit(EXIT_SUCCESS);
}