#include <stdio.h>

#include "atoms.h"

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

void print_turn(player_t* player, uint8_t whose_turn) {
	printf("%s's Turn\n\n", player[whose_turn].colour);
}

void print_grid(grid_t** board, uint8_t w, uint8_t h) {
	
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


void print_stats(gamestate_t* data) {
	if (!data) {
		puts("Game Not In Progress\n");
		return;
	}

	for (int i = 0; i < data->no_players; i++) {
		printf("Player %s:\n", player[i].colour);
		if (data->player[i].grids_owned || turn < data->no_players)
			printf("Grid Count: %d\n\n", data->player[i].grids_owned);
		else
			puts("Lost\n");
	}
}

static char* get_input() {
	char* buffer = malloc(MAX_LINE);

	if (fgets(buffer, MAX_LINE, stdin))
		return buffer;

	free(buffer);
	return NULL;
}
gamestate_t* parseCommand(char* args, gamestate_t* data) {
	int rest;
	char first[MAX_LINE];
	if (!sscanf(args, "%7s %n", first, &rest)) {
		return;
	} if (!strcasecmp(first, "HELP") && !args[rest]) {
		puts(help);
	} else if (!strcasecmp(first, "QUIT") && !args[rest]) {
		puts("Bye!");
		if (!data) data = calloc(1, sizeof(gamestate_t));
		data->game_over = 1;
	} else if (data && !strcasecmp(first, "DISPLAY")) {
		print_grid(data->board, data->width, data->height);
	} else if (!data && !strcasecmp(first, "START")) {
		data = start(args + restgame);
        if (data->msg = NULL) {
            puts("Game Ready");
        	print_turn(data);
        } else {
            puts(data->msg);
            free(data);
            data = NULL;
        }
	} else if (data && !strcasecmp(first, "PLACE")) {
		place_v(args + rest, data);
        if (data->msg) puts(data->msg), data->msg = NULL;
	} else if (!strcasecmp(first, "UNDO") && !args[rest]) {
		undo(data);
	} else if (!strcasecmp(first, "STAT") && !args[rest]) {
		print_stats(data);
	} else if (data && sscanf(args, "SAVE %s %n", first, &rest) > 0 && !args[rest]) {
		save(first, data);
	} else if (!data && sscanf(args, "LOAD %s %n", first, &rest) > 0 && !args[rest]) {
		data = load(first);
        if (data->msg) puts(data->msg);
	} else {
		puts("Invalid command\n");
	}
	return data;
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
	fwrite(data->raw_move_data, sizeof(uint32_t), data->turn, f);

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
	data = malloc(sizeof(gamestate_t));

	data->raw_move_data = NULL;

	fread(&data->width, sizeof(uint8_t), 1, f);
	fread(&data->height, sizeof(uint8_t), 1, f);
	fread(&data->no_players, sizeof(uint8_t), 1, f);

	puts("Game Loaded\n");

	char* input;
	int more = 0;
	data->turn = -1;
	while (data->turn == -1) {
		input = get_input();
		if (!input) continue;
		if (!strcasecmp(input, "QUIT\n")) {
			fclose(f);
			free(input);
			data->msg = "Bye!";
			data->game_over = 1;
			return data;
		} if (!strcasecmp(input, "PLAYFROM END\n")) {
			fseek(f, 0, SEEK_END);
			data->turn = (ftell(f) - HEADER_SIZE + 1)/sizeof(uint32_t);
			fseek(f, HEADER_SIZE, SEEK_SET);
		} else if (!sscanf(input, "PLAYFROM %d %n", &data->turn, &more) || input[more]) {
			data->turn = -1;
			puts("Invalid Command\n");
		} else if (data->turn < 0) {
			puts("Invalid Turn Number\n");
		}
		free(input);
	}

	data->max_turns = 2*turn + 1;
	data->raw_move_data = malloc(data->max_turns*sizeof(uint32_t));
	data->turn = fread(data->raw_move_data, sizeof(uint32_t), data->turn, f);
	fclose(f);
	data->max_turns = 2*data->turn + 1;
	
	
	data = init_game(data);
	for (int i = 0; i < (data->max_turns - 1)/2; i++) {
		move_data_t pos = {.raw_move = data->raw_move_data[i]};
		moves->last = place_q(pos.component.x, pos.component.y, moves->last);
		next_turn(data);
		if (data->game_over) return;
	}
	puts("Game Ready");
	print_turn();
}

int main(void) {
	
	char* input = "Type HELP to list commands";
    puts(input);
    
    gamestate_t* data = NULL;
	while ((!data || !data->game_over) && ((input = get_input()) || !feof(stdin))) {
		data = parseCommand(input, data);
		free(input);
		if (data)
            print_grid(data->board, data->width, data->height);
	}
	clear_data(data);
}
