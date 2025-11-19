#pragma once

#include <cstdint>
#include "driver/ps2/keyboard.hpp"

#define PIECE_SIZE 4

enum PieceType : uint8_t {
    PIECE_I = 0,
    PIECE_J = 1,
    PIECE_T = 2,
    PIECE_L = 3,
    PIECE_O = 4,
    PIECE_Z = 5,
    PIECE_S = 6
};

enum GameState {
    STATE_START,
    STATE_ACTIVE,
    STATE_PAUSED,
    STATE_GAME_OVER
};

struct Tile {
    uint32_t color;
};

struct PieceDef {
    uint8_t minos[PIECE_SIZE][PIECE_SIZE];
    uint32_t color;
};

struct Tetromino {
    PieceDef def;
    int8_t x;
    int8_t y;
};

class Tetris {
public:
    static void init();
    static void update();
    static void handle_key(KeyEvent ev);

private:
    static constexpr uint16_t board_height = 20;
    static constexpr uint16_t board_width = 10;

    static Tetromino held;
    static Tile board[board_height][board_width];
    static GameState state;
    static uint32_t time, score, level, full_lines;

    static uint8_t bag_pieces[7];
    static uint8_t bag_size;

    static bool collides(uint8_t piece[PIECE_SIZE][PIECE_SIZE], int8_t x, int8_t y);
    static void move(int8_t dir_x, int8_t dir_y);
    static bool rotate_piece(Tetromino &piece, bool clockwise);
    static void rotate_cw();
    static void rotate_ccw();
    static void hard_drop();

    static void new_piece();
    static void drop_piece();
    static void check_row();

    static void restart();
    static void draw();
};