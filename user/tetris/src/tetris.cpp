#include "tetris.hpp"

#include "driver/ps2/keyboard.hpp"
#include "driver/screen.hpp"
#include "lib/cstring.hpp"
#include "lib/rand.hpp"
#include "lib/format.hpp"
#include "kernel/log.hpp"
#include "lib/ui/color.hpp"
#include "lib/string.hpp"

namespace TetrisConfig {
    constexpr uint16_t BOARD_HEIGHT = 20;
    constexpr uint16_t BOARD_WIDTH = 10;
    constexpr uint32_t FRAMES_PER_SECOND = 100;
    constexpr uint32_t INITIAL_FRAMES_PER_DROP = 100;
    constexpr uint32_t LINES_PER_LEVEL = 10;
    constexpr uint32_t MIN_FRAMES_PER_DROP = 10;
    constexpr uint32_t BORDER_COLOR = 0x333333;
}

constexpr PieceDef PIECE_DEFS[7] = {
    // I piece
    {{{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 0x00FFFF},
    // J piece
    {{{0, 1, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}, 0x0000FF},
    // T piece
    {{{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 0x800080},
    // L piece
    {{{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}, 0xFFA500},
    // O piece
    {{{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 0xFFFF00},
    // Z piece
    {{{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 0xFF0000},
    // S piece
    {{{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 0x00FF00}
};

Tetromino Tetris::held;
Tile Tetris::board[TetrisConfig::BOARD_HEIGHT][TetrisConfig::BOARD_WIDTH] = {};
GameState Tetris::state = STATE_START;
uint32_t Tetris::time = 0;
uint32_t Tetris::score = 0;
uint32_t Tetris::full_lines = 0;
uint32_t Tetris::level = 1;
uint8_t Tetris::bag_pieces[7] = {};
uint8_t Tetris::bag_size = 0;

// Game timing
static uint32_t frame_counter = 0;
static uint32_t seconds_counter = 0;
static uint32_t frames_per_drop = TetrisConfig::INITIAL_FRAMES_PER_DROP;

static float ui_scale;
static uint16_t block_size;
static uint16_t border_width;
static uint16_t border_blocks;
static int32_t playfield_x;
static int32_t playfield_y;
static uint32_t playfield_pixel_width;
static uint32_t playfield_pixel_height;
static uint32_t info_x;
static uint16_t line_height;

void Tetris::init() {
    keyboard::register_listener(handle_key);

    ui_scale = static_cast<float>(screen::framebuffer.height) / 480.0f;
    if (ui_scale < 1.0f) ui_scale = 1.0f;

    block_size = static_cast<uint16_t>(15 * ui_scale);
    border_width = static_cast<uint16_t>(15 * ui_scale);
    line_height = static_cast<uint16_t>(16 * ui_scale);
    border_blocks = (border_width + block_size - 1) / block_size;

    playfield_pixel_width = block_size * TetrisConfig::BOARD_WIDTH;
    playfield_pixel_height = block_size * TetrisConfig::BOARD_HEIGHT;

    const uint32_t center_x = screen::framebuffer.width / 2;
    const uint32_t center_y = screen::framebuffer.height / 2;

    playfield_x = center_x - playfield_pixel_width / 2;
    playfield_y = center_y - playfield_pixel_height / 2;

    info_x = playfield_x + playfield_pixel_width + border_width + 20 * ui_scale;
}

uint8_t next_piece_index = 0;

void Tetris::new_piece() {
    if (bag_size == 0) {
        for (uint8_t i = 0; i < 7; i++) bag_pieces[i] = i;
        for (uint32_t i = 6; i > 0; --i) {
            const int j = rand() % (i + 1);
            const uint8_t tmp = bag_pieces[i];
            bag_pieces[i] = bag_pieces[j];
            bag_pieces[j] = tmp;
        }
        bag_size = 7;
    }

    const uint8_t piece_index = bag_pieces[--bag_size];

    held.def = PIECE_DEFS[piece_index];
    held.x = 3;
    held.y = 0;

    // Store next piece index
    next_piece_index = (bag_size > 0) ? bag_pieces[bag_size - 1] : 0;
}

bool Tetris::collides(uint8_t piece[PIECE_SIZE][PIECE_SIZE], const int8_t x, const int8_t y) {
    for (uint8_t rel_y = 0; rel_y < PIECE_SIZE; rel_y++) {
        for (uint8_t rel_x = 0; rel_x < PIECE_SIZE; rel_x++) {
            if (piece[rel_y][rel_x] == 0) continue;

            const int8_t abs_x = x + rel_x;
            const int8_t abs_y = y + rel_y;

            // Check bounds
            if (abs_x < 0 || abs_x >= TetrisConfig::BOARD_WIDTH ||
                abs_y >= TetrisConfig::BOARD_HEIGHT) {
                return true;
            }

            // Check collision with placed blocks (ignore if above board)
            if (abs_y >= 0 && board[abs_y][abs_x].color != 0) {
                return true;
            }
        }
    }

    return false;
}

void Tetris::move(const int8_t dir_x, const int8_t dir_y) {
    const int8_t next_x = held.x + dir_x;
    const int8_t next_y = held.y + dir_y;

    if (collides(held.def.minos, next_x, next_y)) {
        if (dir_y > 0) drop_piece();
        return;
    }

    held.x = next_x;
    held.y = next_y;
}

void Tetris::hard_drop() {
    while (!collides(held.def.minos, held.x, held.y + 1)) {
        held.y++;
    }
    drop_piece();
}

static void compute_rotated(
    uint8_t dst[PIECE_SIZE][PIECE_SIZE],
    const uint8_t src[PIECE_SIZE][PIECE_SIZE],
    const bool clockwise
) {
    for (uint8_t i = 0; i < PIECE_SIZE; i++) {
        for (uint8_t j = 0; j < PIECE_SIZE; j++) {
            if (clockwise) {
                dst[j][PIECE_SIZE - 1 - i] = src[i][j];
            } else {
                dst[PIECE_SIZE - 1 - j][i] = src[i][j];
            }
        }
    }
}

static int div_round(const int num, const int den) {
    if (den == 0) return 0;
    if (num >= 0) return (num + den / 2) / den;
    return -((-num + den / 2) / den);
}

bool Tetris::rotate_piece(Tetromino&piece, const bool clockwise) {
    uint8_t rotated[PIECE_SIZE][PIECE_SIZE] = {};
    compute_rotated(rotated, piece.def.minos, clockwise);

    int before_x = 0, before_y = 0;
    int after_x = 0, after_y = 0;
    int count = 0;

    for (uint8_t i = 0; i < PIECE_SIZE; i++) {
        for (uint8_t j = 0; j < PIECE_SIZE; j++) {
            if (piece.def.minos[i][j]) {
                before_x += j;
                before_y += i;
                count++;
            }
            if (rotated[i][j]) {
                after_x += j;
                after_y += i;
            }
        }
    }

    if (count == 0) return false;

    // Offset to keep piece centered after rotation
    const int dx = div_round(before_x - after_x, count);
    const int dy = div_round(before_y - after_y, count);
    const int8_t new_x = piece.x + dx;
    const int8_t new_y = piece.y + dy;

    if (collides(rotated, new_x, new_y)) {
        return false;
    }

    memcpy(piece.def.minos, rotated, PIECE_SIZE * PIECE_SIZE);
    piece.x = new_x;
    piece.y = new_y;
    return true;
}

void Tetris::rotate_cw() {
    rotate_piece(held, true);
}

void Tetris::rotate_ccw() {
    rotate_piece(held, false);
}

void Tetris::drop_piece() {
    // Lock piece into board
    for (uint8_t rel_y = 0; rel_y < PIECE_SIZE; rel_y++) {
        for (uint8_t rel_x = 0; rel_x < PIECE_SIZE; rel_x++) {
            if (held.def.minos[rel_y][rel_x] == 0) continue;

            const int8_t abs_x = held.x + rel_x;
            const int8_t abs_y = held.y + rel_y;

            if (abs_x < 0 || abs_x >= TetrisConfig::BOARD_WIDTH ||
                abs_y < 0 || abs_y >= TetrisConfig::BOARD_HEIGHT) {
                continue;
            }

            board[abs_y][abs_x].color = held.def.color;
        }
    }

    check_row();
    new_piece();

    // Game over if new piece immediately collides
    if (collides(held.def.minos, held.x, held.y+1)) {
        state = STATE_GAME_OVER;
    }
}

void Tetris::check_row() {
    uint8_t lines_cleared = 0;

    for (int16_t y = TetrisConfig::BOARD_HEIGHT - 1; y >= 0; y--) {
        bool is_full = true;
        bool is_empty = true;

        for (uint8_t x = 0; x < TetrisConfig::BOARD_WIDTH; x++) {
            if (board[y][x].color == 0) {
                is_full = false;
            } else {
                is_empty = false;
            }
        }

        if (is_empty) break;

        if (is_full) {
            // Shift all rows above down
            for (int16_t y2 = y; y2 > 0; y2--) {
                memcpy(board[y2], board[y2 - 1], sizeof(Tile) * TetrisConfig::BOARD_WIDTH);
            }
            memset(board[0], 0, sizeof(Tile) * TetrisConfig::BOARD_WIDTH);

            y++; // Check this row again
            full_lines++;
            lines_cleared++;
        }
    }

    // Update level and speed
    const uint32_t new_level = (full_lines / TetrisConfig::LINES_PER_LEVEL) + 1;
    if (new_level > level) {
        level = new_level;
        frames_per_drop = TetrisConfig::INITIAL_FRAMES_PER_DROP - (level * 5);
        if (frames_per_drop < TetrisConfig::MIN_FRAMES_PER_DROP) {
            frames_per_drop = TetrisConfig::MIN_FRAMES_PER_DROP;
        }
    }

    if (lines_cleared > 0 && lines_cleared <= 4) {
        constexpr uint32_t line_scores[4] = {100, 300, 500, 800};
        score += line_scores[lines_cleared - 1] * level;
    }
}

void Tetris::update() {
    draw();

    if (state != STATE_ACTIVE) return;

    // Automatic piece drop
    frame_counter++;
    if (frame_counter >= frames_per_drop) {
        move(0, 1);
        frame_counter = 0;
    }

    // Time tracking
    seconds_counter++;
    if (seconds_counter >= TetrisConfig::FRAMES_PER_SECOND) {
        time++;
        seconds_counter = 0;
    }
}

void Tetris::handle_key(const KeyEvent ev) {
    if (ev.break_key) return;

    switch (ev.scancode) {
        case KEY_X:
        case KEY_W:
        case KEY_ARROW_UP:
            rotate_cw();
            break;

        case KEY_Z:
        case KEY_CONTROL:
            rotate_ccw();
            break;

        case KEY_S:
        case KEY_ARROW_DOWN:
            move(0, 1);
            break;

        case KEY_A:
        case KEY_ARROW_LEFT:
            move(-1, 0);
            break;

        case KEY_D:
        case KEY_ARROW_RIGHT:
            move(1, 0);
            break;

        case KEY_SPACE:
            if (state == STATE_START) {
                state = STATE_ACTIVE;
                new_piece();
            } else if (state == STATE_ACTIVE) {
                hard_drop();
            }
            break;

        case KEY_R:
            restart();
            break;

        case KEY_ESCAPE:
        case KEY_P:
            if (state == STATE_ACTIVE) {
                state = STATE_PAUSED;
            } else if (state == STATE_PAUSED) {
                state = STATE_ACTIVE;
            }
            break;

        default:
            break;
    }
}

static void draw_tile(const uint32_t x, const uint32_t y, const uint16_t size, const uint32_t color) {
    if (size < 3) {
        screen::draw_rect(x, y, size, size, color);
        return;
    }

    // Base color
    screen::draw_rect(x, y, size, size, color);

    const uint16_t inset = size / 8 + 1;

    // Top highlight
    const uint32_t highlight = lighten_color(color, 25);
    const uint16_t hl_height = size / 3;
    screen::draw_rect(x + inset, y + inset, size - inset * 2, hl_height, highlight);

    // Left highlight
    const uint32_t left_hl = lighten_color(color, 12);
    const uint16_t left_width = size / 6;
    screen::draw_rect(x + inset, y + inset + hl_height, left_width, size - inset * 2 - hl_height, left_hl);

    // Bottom shadow
    const uint32_t shadow = darken_color(color, 30);
    const uint16_t shadow_height = size / 3;
    screen::draw_rect(x + inset, y + size - inset - shadow_height, size - inset * 2, shadow_height, shadow);

    // Outline
    const uint32_t outline = darken_color(color, 45);
    screen::draw_rect(x, y, size, 1, outline);
    screen::draw_rect(x, y, 1, size, outline);
    screen::draw_rect(x, y + size - 1, size, 1, outline);
    screen::draw_rect(x + size - 1, y, 1, size, outline);
}

static void draw_border() {
    const int32_t border_x = playfield_x - border_blocks * block_size;
    const int32_t border_y = playfield_y - border_blocks * block_size;
    const uint16_t total_w = TetrisConfig::BOARD_WIDTH + border_blocks * 2;
    const uint16_t total_h = TetrisConfig::BOARD_HEIGHT + border_blocks * 2;

    for (uint16_t by = 0; by < total_h; by++) {
        for (uint16_t bx = 0; bx < total_w; bx++) {
            const bool is_border = bx < border_blocks || bx >= total_w - border_blocks ||
                                   by < border_blocks || by >= total_h - border_blocks;

            if (is_border) {
                const uint32_t px = border_x + bx * block_size;
                const uint32_t py = border_y + by * block_size;
                draw_tile(px, py, block_size, TetrisConfig::BORDER_COLOR);
            }
        }
    }
}

static void draw_overlay(const char* title, const char* message) {
    constexpr float title_scale = 2.0f;
    const uint32_t box_w = playfield_pixel_width * 4 / 2;
    uint32_t box_h = 0;

    if (message) {
        box_h  = playfield_pixel_height / 4;
    } else {
        box_h = 64;
    }

    const uint32_t center_x = screen::framebuffer.width / 2;
    const uint32_t center_y = screen::framebuffer.height / 2;
    const int32_t box_x = center_x - box_w / 2;
    const int32_t box_y = center_y - box_h / 2;

    screen::draw_rect(box_x, box_y, box_w, box_h, 0x1b1b1b);
    screen::draw_rect_outline(box_x, box_y, box_w, box_h, 4, 0xFFFFFF);

    const uint32_t title_w = strlen(title) * 8 * title_scale;
    const int32_t title_x = box_x + (box_w - title_w) / 2;
    screen::draw(title, title_x, box_y + 24, title_scale, 0x40e3e0);

    if (message) {
        constexpr float msg_scale = 1.4f;
        const uint32_t msg_w = strlen(message) * 8 * msg_scale;
        const int32_t msg_x = box_x + (box_w - msg_w) / 2;
        const int32_t msg_y = box_y + box_h / 2;
        screen::draw(message, msg_x, msg_y, msg_scale, 0xCCCCCC);
    }
}

static void draw_piece(const PieceDef &piece, const uint32_t x, const uint32_t y) {
    for (uint8_t rel_y = 0; rel_y < PIECE_SIZE; rel_y++) {
        for (uint8_t rel_x = 0; rel_x < PIECE_SIZE; rel_x++) {
            if (piece.minos[rel_y][rel_x] == 0) continue;
            const int32_t grid_x = static_cast<int32_t>(x) + rel_x;
            const int32_t grid_y = static_cast<int32_t>(y) + rel_y;
            if (grid_y < 0) continue;
            const uint32_t px = playfield_x + static_cast<uint32_t>(grid_x) * block_size;
            const uint32_t py = playfield_y + static_cast<uint32_t>(grid_y) * block_size;
            draw_tile(px, py, block_size, piece.color);
        }
    }
}

void Tetris::draw() {
    // Title
    screen::draw("quark", info_x, playfield_y - border_width - 40, 4, 0x1ED760);

    // Border
    draw_border();

    if (state == STATE_START) {
        draw_overlay("Press [SPACE] to start", nullptr);
        return;
    }

    // Stats
    uint32_t info_y = playfield_y;
    screen::draw(format("FULL LINES: %d", full_lines), info_x, info_y, 1.4);
    info_y += line_height;
    screen::draw(format("LEVEL: %d", level), info_x, info_y, 1.4);
    info_y += line_height;
    screen::draw(format("SCORE: %d", score), info_x, info_y, 1.4);
    info_y += line_height;
    screen::draw(format("TIME: %02d:%02d", time / 60, time % 60), info_x, info_y, 1.4);
    info_y += line_height * 1.5;

    // Controls
    screen::draw(state == STATE_PAUSED ? "[P]: Unpause" : "[P]: Pause", info_x, info_y, 1.4);
    info_y += line_height;
    screen::draw("[R]: Restart", info_x, info_y, 1.4);

    // Draw board
    for (uint8_t y = 0; y < TetrisConfig::BOARD_HEIGHT; y++) {
        for (uint8_t x = 0; x < TetrisConfig::BOARD_WIDTH; x++) {
            if (board[y][x].color == 0) continue;
            const uint32_t px = playfield_x + x * block_size;
            const uint32_t py = playfield_y + y * block_size;
            draw_tile(px, py, block_size, board[y][x].color);
        }
    }

    // Draw active piece
    draw_piece(held.def, held.x, held.y);

    info_y += line_height * 2;
    screen::draw("Next", info_x, info_y, 1.4);

    // Draw next piece
    draw_piece(PIECE_DEFS[next_piece_index], 14, 9);

    // Overlays
    if (state == STATE_GAME_OVER) {
        draw_overlay("Game Over!", "[R] Restart");
    } else if (state == STATE_PAUSED) {
        draw_overlay("Paused", "[P] Resume");
    }
}

void Tetris::restart() {
    logger.info("Tetris: Restarting");
    state = STATE_START;
    full_lines = 0;
    score = 0;
    level = 1;
    time = 0;
    frame_counter = 0;
    seconds_counter = 0;
    frames_per_drop = TetrisConfig::INITIAL_FRAMES_PER_DROP;
    memset(board, 0, sizeof(board));
    bag_size = 0;
    new_piece();
}
