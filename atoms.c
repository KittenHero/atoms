#include <stdio.h>
#include "atoms.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>


static save_file_t* data = NULL;
static grid_t** board = NULL;
static player_t* player = NULL;
static game_t* moves = NULL;
static int turn;
static size_t max_turns;
static uint8_t whose_turn;
static int game_over = 0;

move_t* final_extra(move_t* mv) { while (mv->extra) mv = mv->extra; return mv; }

void clear_data() {
	if (board)
		for (int i = 0; i < data->height; i++) {
			free(board[i]);
		}
	free(board);
	free(player);

	if (data)
		free(data->raw_move_data);
	free(data);

	move_t* tmp;
	if (moves)
		for (move_t* cur = moves->moves; cur; cur = tmp) {
			cur = final_extra(cur);
			tmp = cur->parent;
			if (tmp && tmp->extra == cur) tmp->extra = NULL;
			free(cur);
		}
	free(moves);
}


void init_game() {
	turn = whose_turn = 0;

	player = malloc(sizeof(player_t) * data->no_players);
	for (int i = 0; i < data->no_players; i++) {
		player[i].colour = colour[i];
		player[i].grids_owned = 0;
	}

	board = malloc(sizeof(void *) * data->height);
	for (int i = 0; i < data->height; i++) {
		board[i] = calloc(sizeof(grid_t), data->width);
	}
	moves = malloc(sizeof(game_t));
	moves->moves = NULL;
}

void start(char* args) {
	int k, w, h, n;
	if (sscanf(args, "%*s %*s %*s %n", &n) < 0) {
		puts("Missing Argument\n");
		return;
	} if (args[n]) {
		puts("Too Many Arguments\n");
		return;
	} if (sscanf(args, "%d %d %d", &k, &w, &h) != 3 || k > MAX_PLAYERS || k < MIN_PLAYERS || w < MIN_WIDTH || w > MAX_WIDTH || h < MIN_HEIGHT || h > MAX_HEIGHT) {
		puts("Invalid Command Arguments\n");
		return;
	} if (k > w*h) {
		puts("Cannot Start Game\n");
		return;
	}

	data = malloc(sizeof(save_file_t));
	data->width = w;
	data->height = h;
	data->no_players = k;

	max_turns = w*h;
	data->raw_move_data = malloc(max_turns*sizeof(uint32_t));
	init_game();
	puts("Game Ready");
	print_turn();
}

int check_winner() {
	if (turn < data->no_players)
		return 0;
	for (int i = 0; i < data->no_players; i++)
		if (i != whose_turn && player[i].grids_owned)
			return 0;
	return 1;
}

// function-like macros are for scrubs
int limit(int x, int y) { return 2 + (x && x + 1 < data->width) + (y && y + 1 < data->height); }

move_t* place_q(uint8_t x, uint8_t y, move_t* parent) {
	if (game_over) return NULL;
	int lim = limit(x, y);

	move_t* cur = calloc(1, sizeof(move_t));
	cur->pos.component.x = x;
	cur->pos.component.y = y;
	cur->old_owner = board[y][x].owner;
	cur->parent = parent;

	if (cur->old_owner)
		cur->old_owner->grids_owned--;
	board[y][x].owner = &player[whose_turn];
	
	if (check_winner()) {
		printf("%s Wins!\n", player[whose_turn].colour);
		game_over = 1;
		return cur;
	}

	if (++board[y][x].atom_count == lim) {
		board[y][x].atom_count = 0;
		board[y][x].owner = NULL;

		move_t* tmp = cur;

		if (y) tmp->extra = place_q(x, y - 1, tmp);
		tmp = final_extra(tmp);

		if (x != data->width - 1) tmp->extra = place_q(x + 1, y, tmp);
		tmp = final_extra(tmp);

		if (y != data->height - 1) tmp->extra = place_q(x, y + 1, tmp);
		tmp = final_extra(tmp);

		if (x) tmp->extra = place_q(x - 1, y, tmp);
	} else {
		player[whose_turn].grids_owned++;
	}

	return cur;
}
void next_turn() {
	turn++;

	if (turn == max_turns) {
		max_turns *= 2;
		data->raw_move_data = realloc(data->raw_move_data, max_turns*sizeof(uint32_t));
	}
	do {
		whose_turn = (whose_turn + 1) % data->no_players;
	} while (turn >= data->no_players && !player[whose_turn].grids_owned);
}

void place_v(char* args) {
	int x, y, more;
	if (sscanf(args, "%d %d %n", &x, &y, &more) < 2 || args[more] || x < 0 || y < 0 || x >= data->width || y >= data->height) {
		puts("Invalid Coordinates\n");
		return;
	}

	if (board[y][x].owner && board[y][x].owner != &player[whose_turn]) {
		puts("Cannot Place Atom Here\n");
		return;
	}

	moves->moves = place_q(x, y, moves->moves);
	if (game_over) return;
	move_data_t move = {.raw_move = 0 };
	move.component.x = x;
	move.component.y = y;
	data->raw_move_data[turn] = move.raw_move;
	next_turn();
	print_turn();
}

void undo() {
	if (!turn) {
		puts("Cannot Undo\n");
		return;
	}

	turn--;//ing back time
	do {//not worry about loss, since players cant kill self
		whose_turn += data->no_players - 1; // this is how you mod subtract
		whose_turn %= data->no_players;
	} while (!player[whose_turn].grids_owned);
	
	
	move_t* last_move = final_extra(moves->moves);
	while (last_move != moves->moves->parent) {
		
		free(last_move->extra);
		uint8_t x = last_move->pos.component.x;
		uint8_t y = last_move->pos.component.y;

		if (board[y][x].atom_count)
			player[whose_turn].grids_owned--;
		board[y][x].owner = last_move->old_owner;

		if (last_move->old_owner) {
			int lim = limit(x, y);
			board[y][x].atom_count += lim - 1;
			board[y][x].atom_count %= lim;

			last_move->old_owner->grids_owned++;
		} else {
			board[y][x].atom_count = 0;
		}
		last_move = last_move->parent;
	}
	free(moves->moves);
	moves->moves = last_move;

	print_turn();
}

int main(void) {
	puts("Type HELP to list commands");
	char* input;
	while (!game_over && ((input = get_input()) || !feof(stdin))) {
		parseCommand(input);
		free(input);
		if (board) print_grid();
	}
	clear_data();
}
