#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <font8x8.hpp>
#include <string>
#include <dirent.h>
#include "unistd.h"
#include "sys/mman.h"

constexpr uint32_t width = 1280;
constexpr uint32_t height = 720;
constexpr uint32_t pitch = 5120;
constexpr uint32_t bpp = 32;
constexpr uint32_t screen_size = (width * height * bpp) / 8;

uint8_t* fbp;
uint8_t screen_buffer[screen_size];

static void put_pixel(const uint32_t x, const uint32_t y, const uint32_t color) {
    const uint64_t offset = static_cast<uint64_t>(pitch) * y + static_cast<uint64_t>(x) * (bpp / 8);
    if (offset + sizeof(uint32_t) > screen_size) return;
    auto* location = reinterpret_cast<uint32_t*>(screen_buffer + offset);
    *location = color;
}

static void draw_char(const unsigned char c, const uint32_t x, const uint32_t y, const float size) {
    const unsigned char* glyph = font8x8_basic[c];
    for (uint32_t cy = 0; cy < 8; cy++) {
        for (uint32_t cx = 0; cx < 8; cx++) {
            const uint8_t mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};
            if (!(glyph[cy] & mask[cx])) continue;
            const uint32_t x0 = x + static_cast<uint32_t>(cx * size);
            const uint32_t y0 = y + static_cast<uint32_t>(cy * size);
            const uint32_t x1 = x + static_cast<uint32_t>((cx + 1) * size);
            const uint32_t y1 = y + static_cast<uint32_t>((cy + 1) * size);
            for (uint32_t py = y0; py < y1; ++py) {
                for (uint32_t px = x0; px < x1; ++px) {
                    put_pixel(px, py, 0xFFFFFF);
                }
            }
        }
    }
}

constexpr size_t CONTENT_MAX = 8192;
char content[CONTENT_MAX] = {};
uint32_t content_len = 0;
constexpr uint8_t SCALE = 2;
constexpr uint8_t CHAR_SIZE = 8 * SCALE;

static void redraw() {
    memset(screen_buffer, 0, screen_size);

    uint32_t x = 0, y = 0;

    for (uint32_t i = 0; i < content_len; i++) {
        const char c = content[i];
        if (c == '\r') continue;
        if (c == '\n') {
            x = 0;
            y += CHAR_SIZE;
            continue;
        }
        draw_char(static_cast<unsigned char>(c), x, y, SCALE);
        x += CHAR_SIZE;
        if (x >= width) {
            x = 0;
            y += CHAR_SIZE;
        }
    }

    memcpy(fbp, screen_buffer, screen_size);
}

static void append_output(const char* text, const size_t len) {
    for (size_t i = 0; i < len && content_len < CONTENT_MAX; i++) {
        content[content_len++] = text[i];
    }

    redraw();
}

static void append_output(const char* text) {
    append_output(text, strlen(text));
}

static void append_char(const char c) {
    append_output(&c, 1);
}

static void backspace() {
    if (content_len > 0) {
        content_len--;
        redraw();
    }
}

constexpr uint16_t TOKEN_BUF_SIZE = 64;
constexpr auto TOKEN_DELIM = " \t\r\n\a";

void parse_line(char* line) {
    if (strcmp(line, "") == 0) return;
    uint16_t buffer_size = TOKEN_BUF_SIZE, pos = 0;
    auto tokens = static_cast<char**>(malloc(buffer_size * sizeof(char*)));

    if (!tokens) {
        // TODO: log error
        exit(EXIT_FAILURE);
    }

    char* token = strtok(line, TOKEN_DELIM);

    while (token != nullptr) {
        tokens[pos] = token;
        pos++;

        if (pos >= buffer_size) {
            buffer_size += TOKEN_BUF_SIZE;

            auto new_tokens = static_cast<char**>(realloc(tokens, buffer_size * sizeof(char*)));

            if (!new_tokens) {
                exit(EXIT_FAILURE);
            }

            tokens = new_tokens;

        }

        token = strtok(nullptr, TOKEN_DELIM);
    }

    tokens[pos] = nullptr;

    const uint32_t arg_count = pos - 1;

    {
        const char* command = tokens[0];
        if (strcmp(command, "echo") == 0) {
            uint32_t arg_pos = 0;
            uint32_t echo_buffer_size = 64;
            auto echo_buffer = static_cast<char*>(malloc(echo_buffer_size * sizeof(char)));
            memset(echo_buffer, 0, echo_buffer_size);

            while (tokens[arg_pos + 1] != nullptr) {
                const char* str = tokens[arg_pos + 1];

                if (strlen(str) > echo_buffer_size) {
                    echo_buffer_size += 64;
                    echo_buffer = static_cast<char*>(realloc(echo_buffer, echo_buffer_size * sizeof(char)));

                    if (!echo_buffer) {
                        exit(EXIT_FAILURE);
                    }
                }

                if (arg_pos > 0) {
                    strcat(echo_buffer, " ");
                }

                strcat(echo_buffer, str);
                arg_pos++;
            }

            strcat(echo_buffer, "\n");
            append_output(echo_buffer, strlen(echo_buffer));
        } else if (strcmp(command, "ls") == 0) {
            std::string path;

            if (tokens[1] == nullptr) {
                path = "/";
            } else {
                path = tokens[1];
            }

            DIR *dir = opendir(path.c_str());

            const dirent* ent = readdir(dir);

            while (ent != nullptr) {
                append_output(ent->d_name);
            }

            closedir(dir);
        } else if (strcmp(command, "cd") == 0) {
            char* path = tokens[0];

            if (arg_count > 2) {
                append_output("Too many arguments for cd");
            }

            // TODO: Check if path exists, update PWD
        } else if (strcmp(command, "pwd") == 0) {

        } else if (strcmp(command, "exit") == 0) {
            exit(EXIT_SUCCESS);
        } else {
            const char* path = getenv("PATH");

            append_output("command not found\n");
        }
    }
}

constexpr char PROMPT[] = "> ";

static char* readline() {
    static char linebuf[512];
    uint32_t len = 0;

    while (true) {
        char c;
        const ssize_t n = read(0, &c, 1);
        if (n <= 0) {
            exit(EXIT_SUCCESS);
        }

        if (c == '\r') continue;

        if (c == '\n') {
            linebuf[len] = '\0';
            append_char('\n');
            return strdup(linebuf);
        }

        if (c == '\b' || c == 0x7F) {
            if (len > 0) {
                len--;
                backspace();
            }
            continue;
        }

        if (len + 1 < sizeof(linebuf)) {
            linebuf[len++] = c;
            append_char(c);
        }
    }
}

int main() {
    // TODO: implement a proper way of exposing the framebuffer to userspace
    // Likely some fbdev driver that a user program requests access to by providing parameters
    fbp = static_cast<uint8_t*>(
        mmap(nullptr, screen_size, PROT_READ | PROT_WRITE, MAP_ANON, 3, 0)
    );
    memset(fbp, 0, screen_size);

    append_output(PROMPT);

    while (true) {
        char* line = readline();
        parse_line(line);
        free(line);
        append_output(PROMPT);
    }

    return 0;
}
