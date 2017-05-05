#ifndef ATOMS_H
#define ATOMS_H

#include <stdint.h>

#define MAX_LINE 255
#define MIN_WIDTH 2
#define MIN_HEIGHT 2
#define MAX_WIDTH 255
#define MAX_HEIGHT 255
#define MIN_PLAYERS 2
#define MAX_PLAYERS 6
#define HEADER_SIZE 3

typedef struct move_t move_t;
typedef struct grid_t grid_t;
typedef struct game_t game_t;
typedef struct save_t save_t;
typedef struct player_t player_t;
typedef struct gamestate_t gamestate_t;
typedef union move_data_t move_data_t;

union move_data_t {
    struct {
        uint8_t x;
        uint8_t y;
        uint16_t padding;
    } component; 
    
    uint32_t raw_move;
};

struct move_t {
  move_data_t pos;
  move_t* parent;
  move_t* extra;
  player_t* old_owner; // NULL if unoccupied
};

struct game_t {
  move_t* last;
};

struct grid_t {
  player_t* owner;
  int atom_count;
};

struct player_t {
  const char* colour;
  int grids_owned;
};

struct gamestate_t {
    uint8_t width;
    uint8_t height;
    
    player_t* player;
    uint8_t no_players;
    
    grid_t** board;
    
    uint32_t* raw_move_data;
    size_t capacity;
    int turn;
    uint8_t whose_turn;
    game_t* moves;
    
    char* msg;
    
    int game_over;
};

static const char * const colour[] = {
    "Red", "Green", "Purple", "Blue", "Yellow", "White"
};

#endif
