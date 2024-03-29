#include <stdio.h>
#include <stdlib.h>

#include "atoms.h"

static inline move_t* final_extra(move_t* mv) {
	while (mv->extra)
		mv = mv->extra;
	return mv;
}

void clear_data(gamestate_t* data) {
	if (data) {
		if (data->board)
			for (int i = 0; i < data->height; i++) {
				free(data->board[i]);
			}
		free(data->board);
		free(data->player);

		free(data->raw_move_data);
		move_t* tmp;
		if (data->moves)
			for (move_t* cur = data->moves->last; cur; cur = tmp) {
				cur = final_extra(cur);
				tmp = cur->parent;
				if (tmp && tmp->extra == cur)
					tmp->extra = NULL;
				free(cur);
			}
		free(data->moves);
	}
	free(data);
}

void init_game(gamestate_t* data) {
	data->turn = data->whose_turn = 0;

	data->player = malloc(sizeof(player_t) * data->no_players);
	for (int i = 0; i < data->no_players; i++) {
		data->player[i].colour = colour[i];
		data->player[i].grids_owned = 0;
	}

	data->board = malloc(sizeof(void *) * data->height);
	for (int i = 0; i < data->height; i++) {
		data->board[i] = calloc(sizeof(grid_t), data->width);
	}
	data->moves = malloc(sizeof(game_t));
	data->moves->last = NULL;
}

gamestate_t* start(char* args) {
	int k, w, h, n;
	gamestate_t* data = calloc(1, sizeof(gamestate_t));

	if (sscanf(args, "%*s %*s %*s %n", &n) < 0) {
		data->msg = "Missing Argument\n";
		return data;
	} if (args[n]) {
		data->msg = "Too Many Arguments\n";
		return data;
	} if (sscanf(args, "%d %d %d", &k, &w, &h) != 3 || k > MAX_PLAYERS || k < MIN_PLAYERS || w < MIN_WIDTH || w > MAX_WIDTH || h < MIN_HEIGHT || h > MAX_HEIGHT) {
		data->msg = "Invalid Command Arguments\n";
		return data;
	} if (k > w*h) {
		data->msg = "Cannot Start Game\n";
		return data;
	}

	data->width = w;
	data->height = h;
	data->no_players = k;

	data->max_turns = w*h;
	data->raw_move_data = malloc(data->max_turns*sizeof(uint32_t));
	init_game(data);
	return data;
}

int check_winner(gamestate_t* data) {
	if (data->turn < data->no_players)
		return 0;
	for (int i = 0; i < data->no_players; i++)
		if (i != data->whose_turn && data->player[i].grids_owned)
			return 0;
	return 1;
}

// function-like macros are for scrubs
static inline int limit(int x, int y, int w, int h) { return 2 + (x && x + 1 < w) + (y && y + 1 < h); }

move_t* place_q(uint8_t x, uint8_t y, move_t* parent, gamestate_t* data) {
	if (data->game_over) return NULL;
	int lim = limit(x, y, data->width, data->height);
	move_t* cur = calloc(1, sizeof(move_t));
	cur->pos.component.x = x;
	cur->pos.component.y = y;
	cur->old_owner = data->board[y][x].owner;
	cur->parent = parent;

	if (cur->old_owner)
		cur->old_owner->grids_owned--;
	data->board[y][x].owner = &data->player[data->whose_turn];

	if (check_winner(data)) {
		printf("%s Wins!\n", data->player[data->whose_turn].colour);
		data->game_over = 1;
		return cur;
	}

	if (++data->board[y][x].atom_count == lim) {
		data->board[y][x].atom_count = 0;
		data->board[y][x].owner = NULL;

		move_t* tmp = cur;

		if (y) tmp->extra = place_q(x, y - 1, tmp, data);
		tmp = final_extra(tmp);

		if (x != data->width - 1) tmp->extra = place_q(x + 1, y, tmp, data);
		tmp = final_extra(tmp);

		if (y != data->height - 1) tmp->extra = place_q(x, y + 1, tmp, data);
		tmp = final_extra(tmp);

		if (x) tmp->extra = place_q(x - 1, y, tmp, data);
	} else {
		data->player[data->whose_turn].grids_owned++;
	}

	return cur;
}
void next_turn(gamestate_t* data) {
	data->turn++;

	if (data->turn == data->max_turns) {
		data->max_turns *= 2;
		data->raw_move_data = realloc(data->raw_move_data, data->max_turns*sizeof(uint32_t));
	}
	do {
		data->whose_turn = (data->whose_turn + 1) % data->no_players;
	} while (data->turn >= data->no_players && !data->player[data->whose_turn].grids_owned);
}

void place_v(char* args, gamestate_t* data) {
	int x, y, more;
	if (sscanf(args, "%d %d %n", &x, &y, &more) < 2 || args[more] || x < 0 || y < 0 || x >= data->width || y >= data->height) {
		data->msg = "Invalid Coordinates\n";
		return;
	}

	if (data->board[y][x].owner && data->board[y][x].owner != &data->player[data->whose_turn]) {
		data->msg = "Cannot Place Atom Here\n";
		return;
	}

	data->moves->last = place_q(x, y, data->moves->last, data);
	if (data->game_over) return;
	move_data_t move = {.raw_move = 0 };
	move.component.x = x;
	move.component.y = y;
	data->raw_move_data[data->turn] = move.raw_move;
	next_turn(data);
	print_turn(data->player, data->whose_turn);
}

void undo(gamestate_t* data) {
	if (!data->turn) {
		puts("Cannot Undo\n");
		return;
	}

	data->turn--;//ing back time
	do {//not worry about loss, since players cant kill self
		data->whose_turn += data->no_players - 1; // this is how you mod subtract
		data->whose_turn %= data->no_players;
	} while (!data->player[data->whose_turn].grids_owned);


	move_t* last_move = final_extra(data->moves->last);
	while (last_move != data->moves->last->parent) {

		free(last_move->extra);
		uint8_t x = last_move->pos.component.x;
		uint8_t y = last_move->pos.component.y;

		if (data->board[y][x].atom_count)
			data->player[data->whose_turn].grids_owned--;
		data->board[y][x].owner = last_move->old_owner;

		if (last_move->old_owner) {
			int lim = limit(x, y, data->width, data->height);
			data->board[y][x].atom_count += lim - 1;
			data->board[y][x].atom_count %= lim;

			last_move->old_owner->grids_owned++;
		} else {
			data->board[y][x].atom_count = 0;
		}
		last_move = last_move->parent;
	}
	free(data->moves->last);
	data->moves->last = last_move;

	print_turn(data->player, data->whose_turn);
}
