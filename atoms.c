#include <stdio.h>
#include "atoms.h"
#include <stdlib.h>
#include <string.h>

static const char* colour[] = {
	"Red", "Green", "Purple", "Blue", "Yellow", "White"
};

static save_file_t* data = NULL;
static grid_t** board = NULL;
static player_t* player = NULL;
static game_t* moves = NULL;
static int turn;
static size_t max_turns;
static uint8_t whose_turn;
static int game_over = 0;


char* get_input() {
	char* buffer = malloc(MAX_LINE);

	if (fgets(buffer, MAX_LINE, stdin))
		return buffer;

	free(buffer);
	return NULL;
}


static const char* help = // won't you please
	"\n"
	"HELP displays this help message\n"
	"QUIT quits the current game\n"
	"\n"
	"DISPLAY draws the game board in terminal\n"
	"START <number of players> <width> <height> starts the game\n"
	"PLACE <x> <y> places an atom in a grid space\n"
	"UNDO undoes the last move made\n"
	"STAT displays game statistics\n"
	"\n"
	"SAVE <filename> saves the state of the game\n"
	"LOAD <filename> loads a save file\n"
	"PLAYFROM <turn> plays from n steps into the game\n";

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

void print_grid() {

	uint8_t w = data->width, h = data->height;

	char buffer[h + 1][w*3 + 2];
	memset(buffer, ' ', h*(w*3 + 2));

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			buffer[y][3*x] = '|';

			grid_t* p = board[y] + x;
			if (p->owner)
				sprintf(buffer[y] + 3*x + 1, "%c%d", *p->owner->colour, p->atom_count);

		}
		buffer[y][w*3] = '|';
		buffer[y][w*3 + 1] = '\n';
	}
	buffer[h][0] = buffer[h][3*w] = '+';
	memset(buffer[h] + 1, '-', 3*w - 1);
	buffer[h][3*w + 1] = '\0';

	printf("\n%s\n%s\n\n", buffer[h], buffer[0]);
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


void print_turn() {
	printf("%s's Turn\n\n", player[whose_turn].colour);
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
		board[y][x].owner = &player[whose_turn];
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

void print_stats() {

	if (!data) {
		puts("Game Not In Progress\n");
		return;
	}

	for (int i = 0; i < data->no_players; i++) {
		printf("Player %s:\n", player[i].colour);
		if (player[i].grids_owned || turn < data->no_players)
			printf("Grid Count: %d\n\n", player[i].grids_owned);
		else
			puts("Lost\n");
	}
}


void save(char * fn) {

	if (fopen(fn, "rb")) {
		puts("File Already Exists\n");
		return;
	}

	FILE* f = fopen(fn, "wb");
	fwrite(&data->width, sizeof(uint8_t), 1, f);
	fwrite(&data->height, sizeof(uint8_t), 1, f);
	fwrite(&data->no_players, sizeof(uint8_t), 1, f);
	fwrite(data->raw_move_data, sizeof(uint32_t), turn, f);

	fclose(f);
	puts("Game Saved\n");
}


void load(char * fn) {

	if (data) {
		puts("Restart Application To Load Save\n");
		return;
	}

	FILE* f = fopen(fn, "rb");
	if (!f) {
		puts("Cannot Load Save\n");
		return;
	}
	data = malloc(sizeof(save_file_t));

	data->raw_move_data = NULL;

	fread(&data->width, sizeof(uint8_t), 1, f);
	fread(&data->height, sizeof(uint8_t), 1, f);
	fread(&data->no_players, sizeof(uint8_t), 1, f);

	puts("Game Loaded\n");

	char* input;
	int more = 0;
	turn = -1;
	while (turn == -1) {
		input = get_input();
		if (!input) continue;
		if (!strcmp(input, "QUIT\n")) {
			fclose(f);
			free(input);
			puts("Bye!");
			game_over = 1;
			return;
		} if (!strcmp(input, "PLAYFROM END\n")) {
			fseek(f, 0, SEEK_END);
			turn = (ftell(f) - HEADER_SIZE + 1)/sizeof(uint32_t);
			fseek(f, HEADER_SIZE, SEEK_SET);
		} else if (!sscanf(input, "PLAYFROM %d %n", &turn, &more) || input[more]) {
			turn = -1;
			puts("Invalid Command\n");
		} else if (turn < 0) {
			puts("Invalid Turn Number\n");
		}
		free(input);
	}

	max_turns = 2*turn + 1;
	data->raw_move_data = malloc(max_turns*sizeof(uint32_t));
	turn = fread(data->raw_move_data, sizeof(uint32_t), turn, f);
	fclose(f);
	max_turns = 2*turn + 1;
	
	
	init_game();
	for (int i = 0; i < (max_turns - 1)/2; i++) {
		move_data_t pos = {.raw_move = data->raw_move_data[i]};
		moves->moves = place_q(pos.component.x, pos.component.y, moves->moves);
		next_turn();
		if (game_over) return;
	}
	puts("Game Ready");
	print_turn();
}


void parseCommand(char* args) {

	int rest;
	char first[MAX_LINE];
	if (!sscanf(args, "%7s %n", first, &rest)) {
		return;
	} if (!strcmp(first, "HELP") && !args[rest]) {
		puts(help);
	} else if (!strcmp(first, "QUIT") && !args[rest]) {
		puts("Bye!");
		game_over = 1;
	} else if (data && !strcmp(first, "DISPLAY")) {
		print_grid();
	} else if (!data && !strcmp(first, "START")) {
		start(args + rest);
	} else if (data && !strcmp(first, "PLACE")) {
		place_v(args + rest);
	} else if (!strcmp(first, "UNDO") && !args[rest]) {
		undo();
	} else if (!strcmp(first, "STAT") && !args[rest]) {
		print_stats();
	} else if (data && sscanf(args, "SAVE %s %n", first, &rest) > 0 && !args[rest]) {
		save(first);
	} else if (!data && sscanf(args, "LOAD %s %n", first, &rest) > 0 && !args[rest]) {
		load(first);
	} else {
		puts("Invalid command\n");
	}
}
int main(void) {

	char* input;
	while (!game_over && ((input = get_input()) || !feof(stdin))) {
		parseCommand(input);
		free(input);
	}
	clear_data();
}

