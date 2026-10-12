#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include "unistd.h"

static char* cwd = nullptr;
static char* prompt;

static void append_output(const char* text, const size_t len) {
    printf("%.*s", static_cast<int>(len), text);
    fflush(stdout);
}

static void append_output(const char* text) {
    append_output(text, strlen(text));
}

static void append_char(const char c) {
    append_output(&c, 1);
}

static void backspace() {
    printf("\b \b");
    fflush(stdout);
}

constexpr uint16_t TOKEN_BUF_SIZE = 64;
constexpr auto TOKEN_DELIM = " \t\r\n\a";

static void parse_line(char* line) {
    if (strcmp(line, "") == 0) return;
    uint16_t buffer_size = TOKEN_BUF_SIZE, pos = 0;
    auto argv = static_cast<char**>(malloc(buffer_size * sizeof(char*)));

    if (!argv) {
        exit(EXIT_FAILURE);
    }

    char* token = strtok(line, TOKEN_DELIM);

    while (token != nullptr) {
        argv[pos] = token;
        pos++;

        if (pos >= buffer_size) {
            buffer_size += TOKEN_BUF_SIZE;

            printf("reallocing\n");
            const auto new_tokens = static_cast<char**>(realloc(argv, buffer_size * sizeof(char*)));

            if (!new_tokens) {
                exit(EXIT_FAILURE);
            }

            argv = new_tokens;
        }

        token = strtok(nullptr, TOKEN_DELIM);
    }

    argv[pos] = nullptr;

    const uint32_t argc = pos;

    {
        const char* command = argv[0];
        if (strcmp(command, "echo") == 0) {
            uint32_t arg_pos = 0;
            uint32_t echo_buffer_size = 64;
            auto echo_buffer = static_cast<char*>(malloc(echo_buffer_size * sizeof(char)));
            memset(echo_buffer, 0, echo_buffer_size);

            while (argv[arg_pos + 1] != nullptr) {
                const char* str = argv[arg_pos + 1];

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
            const char* path = argv[1] ? argv[1] : cwd;

            DIR* dir = opendir(path);

            if (!dir) {
                append_output("ls: failed to open directory\n");
                return;
            }

            while (const dirent* ent = readdir(dir)) {
                append_output(ent->d_name);
                append_char(' ');
            }

            append_char('\n');

            closedir(dir);
        } else if (strcmp(command, "cd") == 0) {
            if (argc > 2) {
                append_output("Too many arguments for cd\n");
                return;
            }

            const char* path;

            if (argc == 1) {
                path = getenv("HOME");

                if (!path) {
                    append_output("cd: HOME not set\n");
                    return;
                }
            } else {
                path = argv[1];
            }

            if (chdir(path) == -1) {
                append_output("cd: No such file or directory\n");
                return;
            }

            if (!getcwd(cwd, 4096)) {
                append_output("cd: failed to get current directory\n");
                return;
            }

            prompt = static_cast<char*>(
                realloc(prompt, strlen(cwd) + 3)
            );

            if (!prompt) {
                append_output("cd: failed to allocate prompt\n");
                return;
            }

            snprintf(prompt, strlen(cwd) + 3, "%s> ", cwd);
        } else if (strcmp(command, "pwd") == 0) {
            append_output(cwd);
        } else if (strcmp(command, "exit") == 0) {
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "clear") == 0) {
            // Clear the screen
            append_output("\033[H\033[J");
        } else {
            char path[] = "/bin:/usr/bin";
            constexpr auto delim = ":";
            char* token = strtok(path, delim);
            char* executable_path = nullptr;

            while (token != nullptr) {
                const size_t path_len = strlen(token) + strlen(command) + 2;

                auto* full_path = static_cast<char*>(malloc(path_len));

                if (!full_path) {
                    exit(EXIT_FAILURE);
                }

                snprintf(full_path, path_len, "%s/%s", token, command);

                struct stat buf;

                if (stat(full_path, &buf) == 0) {
                    executable_path = full_path;
                    break;
                }

                free(full_path);
                token = strtok(nullptr, delim);
            }

            if (executable_path) {
                const pid_t pid = fork();

                if (pid < 0) {
                    free(executable_path);
                    append_output("fork failed\n");
                    return;
                }

                if (pid == 0) {
                    execve(executable_path, argv, nullptr);

                    append_output("execve failed\n");
                    _exit(127);
                }

                int status;
                waitpid(pid, &status, 0);

                printf("Status: %d\n", WEXITSTATUS(status));

                free(executable_path);
            }
        }
    }

    free(argv);
}

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
    {
        char cwd_buf[256];
        cwd = getcwd(cwd_buf, sizeof(cwd_buf));
        if (strcmp(cwd_buf, "") == 0) {
            cwd = strdup("/");
        }

        prompt = static_cast<char*>(malloc(strlen(cwd) + 4));
        snprintf(prompt, strlen(cwd) + 4, "%s> ", cwd);
    }

    append_output(prompt);

    while (true) {
        char* line = readline();
        parse_line(line);
        free(line);
        append_output(prompt);
    }

    return 0;
}
