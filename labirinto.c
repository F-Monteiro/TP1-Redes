#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int type;
    int moves[100];
    int board[10][10];
} action;

int **server_map = NULL, **player_map = NULL;
int rows = 0, cols = 0;

/* TYPE
start   0 Enviada pelo cliente ao servidor no início do jogo
move    1 Enviada pelo cliente para se mover no labirinto
map     2 Enviada pelo cliente requisitando o mapa
hint    3 Enviada pelo cliente solicitando a dica
update  4 Enviada pelo servidor sempre que houver mudança no estado do jogo
win     5 Enviada pelo servidor ao cliente quando o jogo for ganho
reset   6 Enviada pelo cliente ao servidor para reiniciar o jogo
exit    7 Enviada pelo cliente ao servidor quando deseja se desconectar
*/

action empty_action(){  //Done (client)
    action empty;
    empty.type = 9;
    int i, j, k;
    for(k = 0; k < 100; k++){
        empty.moves[k] = 0;
    }
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            empty.board[i][j] = 0;
        }
    }
    return empty;
}

void start_map(char* filename) {  //Done (server)
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

action check_player_moves() {  //Done (server)
    int i, j, k, quebra_loop = 0;
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

void update_player_map() {  //Done (server)
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

action win() {  //Done (server)
    action winner = empty_action();
    winner.type = 5;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            winner.board[i][j] = server_map[i][j];
        }
    }
    return winner;
}

action move_player(int direction){  //Done (server)
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
    switch (direction)
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

action send_map(){  //Done (server)
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

void print_map(){  //Done (client)
    char symbols[6] = {'#', '_', '>', 'X', '?', '+'};
    int number, i, j;
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            number = player_map[i][j];
            printf ("%c ", symbols[number]);
        }
        printf("\n");
    }
}

void restart_game(){  //Done (server)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            player_map[i][j] = 4;
            if (server_map[i][j] == 2){
                player_map[i][j] = 5;
            }
        }
    }
}

void print_moves(action possible_moves){ //Done (client)
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

int main(){
    char filename[] = "input/in.txt";
    int num, mov;
    action Control;
    while(1){
        printf("Choose action: ");
        scanf("%d", &num);
        if(num == 0 || num == 6){
            start_map(filename);
            update_player_map();
            Control = check_player_moves();
            print_moves(Control);
        }
        if(num == 2){
            print_map();
        }
        if(num == 1){
            printf("1-up; 2-right; 3-down; 4-left. Choose:");
            scanf("%d", &mov);
            Control = move_player(mov);
            if(Control.type == 5){
                printf("Winner winner chicken dinner");
            } else{
                print_moves(Control);

            }
        }
        if(num == 7){
            break;
        }
    }
    return 0;
}