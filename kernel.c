#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h> // For strcmp, strtok, atoi

// Constants
#define VIDEO_MEMORY 0xb8000
#define WHITE_ON_BLUE 0x1F
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BACKSPACE_SCANCODE 0x0E
#define DELETE_SCANCODE 0x53
#define MAX_VARS 10
#define VAR_NAME_LEN 32
#define VAR_VALUE_LEN 32

// Globals
typedef struct {
    char name[VAR_NAME_LEN];
    char value[VAR_VALUE_LEN];
} Variable;

Variable variables[MAX_VARS];
size_t var_count = 0;
static char input_buffer[256];
static size_t input_index = 0;
static uint16_t cursor_pos = 0;
static uint8_t text_color = 0xF;  // Default: white
static uint8_t text_color1 = 0xF;  // Default: white
static uint8_t bg_color = 0x1;   // Default: blue
static char splash_screen[80] = "Welcome to DubrDos!"; // Default splash screen
// Function Prototypes
void init_system(void);
void clear_screen(void);
void display_text(const char *text, uint16_t row, uint16_t col);
void handle_keyboard(void);
void update_cursor(uint16_t position);
void process_input(void);
void print_char(char c);
void set_splash(const char *new_splash);
void create_variable(const char *name, const char *value);
char *get_variable_value(const char *name);
void print_color_text(const char *text, const char *color_name);
void set_color_splash(const char *color_name);
uint16_t get_cursor_row(void);
uint16_t get_cursor_col(void);
void execute_command(const char *command);
int color_code_from_name(const char *name);

// I/O Port Access Functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    // Handle negative numbers
    if (*str == '-') {
        sign = -1;
        str++;
    }

    // Convert characters to integers
    while (*str) {
        if (*str < '0' || *str > '9') {
            break; // Stop on non-digit character
        }
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}
// Custom implementation of strncpy
char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

// Custom implementation of strlen
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}
int strncmp(const char *str1, const char *str2, size_t n) {
    while (n > 0 && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}
char *strtok(char *str, const char *delim) {
    static char *next_token = NULL;
    if (str == NULL) {
        str = next_token;
    }
    if (str == NULL) {
        return NULL;
    }

    // Skip leading delimiters
    while (*str && strchr(delim, *str)) {
        str++;
    }
    if (*str == '\0') {
        return NULL;
    }

    // Find the end of the token
    char *token_start = str;
    while (*str && !strchr(delim, *str)) {
        str++;
    }

    if (*str) {
        *str = '\0';
        next_token = str + 1;
    } else {
        next_token = NULL;
    }

    return token_start;
}
char *strchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

void itoa(int value, char *str, int base) {
    char *digits = "0123456789ABCDEF";
    char buffer[32];
    int i = 0, is_negative = 0;

    // Handle negative numbers for base 10
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }

    // Convert the number to the given base
    do {
        buffer[i++] = digits[value % base];
        value /= base;
    } while (value > 0);

    // Add the negative sign if necessary
    if (is_negative) {
        buffer[i++] = '-';
    }

    // Reverse the string into the output buffer
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
}

void snprintf(char *str, size_t size, const char *format, const char *str_value, int int_value) {
    const char *p = format;
    char *out = str;

    while (*p && (out - str) < (size - 1)) {
        if (*p == '%' && *(p + 1) == 's') {
            // Handle string
            const char *s = str_value;
            while (*s && (out - str) < (size - 1)) {
                *out++ = *s++;
            }
            p += 2; // Skip %s
        } else if (*p == '%' && *(p + 1) == 'd') {
            // Handle integer
            char num_buffer[32];
            itoa(int_value, num_buffer, 10);
            for (char *q = num_buffer; *q && (out - str) < (size - 1); q++) {
                *out++ = *q;
            }
            p += 2; // Skip %d
        } else {
            *out++ = *p++;
        }
    }
    *out = '\0'; // Null-terminate the string
}


void clear_screen(void) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t blank = ' ' | ((text_color | (bg_color << 4)) << 8);
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video_memory[i] = blank;
    }
    cursor_pos = 3 * SCREEN_WIDTH; // Start input on line 3
    update_cursor(cursor_pos);
}

void display_text(const char *text, uint16_t row, uint16_t col) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t offset = row * SCREEN_WIDTH + col;
    while (*text) {
        video_memory[offset++] = *text | ((text_color | (bg_color << 4)) << 8);
        text++;
    }
}
// Function to set a new splash screen
void set_splash(const char *new_splash) {
    strncpy(splash_screen, new_splash, sizeof(splash_screen) - 1);
    splash_screen[sizeof(splash_screen) - 1] = '\0'; // Ensure null termination
    display_text("Splash updated!", get_cursor_row(), 0);
}
// Print a single character to the screen at the current cursor position
void print_char(char c) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;

    if (c == '\n') {
        cursor_pos = (cursor_pos / SCREEN_WIDTH + 1) * SCREEN_WIDTH; // Move to the next row
    } else {
        video_memory[cursor_pos++] = c | ((text_color | (bg_color << 4)) << 8);
    }

    if (cursor_pos >= SCREEN_WIDTH * SCREEN_HEIGHT) {
        cursor_pos = 3 * SCREEN_WIDTH; // Reset to line 3
    }
    update_cursor(cursor_pos);
}

// Update the hardware cursor position
void update_cursor(uint16_t position) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

// Get the current cursor row
uint16_t get_cursor_row(void) {
    return cursor_pos / SCREEN_WIDTH;
}

// Get the current cursor column
uint16_t get_cursor_col(void) {
    return cursor_pos % SCREEN_WIDTH;
}

// Handle keyboard input
void handle_keyboard(void) {
    if ((inb(KEYBOARD_STATUS_PORT) & 0x01) == 0) {
        return;
    }

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    if (scancode & 0x80) {
        return; // Ignore key release
    }

    char key = '\0';
    switch (scancode) {
        case BACKSPACE_SCANCODE: // Backspace
            if (input_index > 0) {
                input_index--;
                cursor_pos--;
                uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
                video_memory[cursor_pos] = ' ' | (WHITE_ON_BLUE << 8); // Clear character
                update_cursor(cursor_pos);
            }
            return;

        case DELETE_SCANCODE: // Delete
            if (cursor_pos < SCREEN_WIDTH * SCREEN_HEIGHT) {
                uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
                video_memory[cursor_pos] = ' ' | (WHITE_ON_BLUE << 8); // Clear character
            }
            return;

        case 0x1C: key = '\n'; break; // Enter
        case 0x39: key = ' '; break;  // Space
        case 0x02: key = '1'; break;
        case 0x03: key = '2'; break;
        case 0x04: key = '3'; break;
        case 0x05: key = '4'; break;
        case 0x06: key = '5'; break;
        case 0x07: key = '6'; break;
        case 0x08: key = '7'; break;
        case 0x09: key = '8'; break;
        case 0x0A: key = '9'; break;
        case 0x0B: key = '0'; break;
        case 0x10: key = 'q'; break;
        case 0x11: key = 'w'; break;
        case 0x12: key = 'e'; break;
        case 0x13: key = 'r'; break;
        case 0x14: key = 't'; break;
        case 0x15: key = 'y'; break;
        case 0x16: key = 'u'; break;
        case 0x17: key = 'i'; break;
        case 0x18: key = 'o'; break;
        case 0x19: key = 'p'; break;
        case 0x1E: key = 'a'; break;
        case 0x1F: key = 's'; break;
        case 0x20: key = 'd'; break;
        case 0x21: key = 'f'; break;
        case 0x22: key = 'g'; break;
        case 0x23: key = 'h'; break;
        case 0x24: key = 'j'; break;
        case 0x25: key = 'k'; break;
        case 0x26: key = 'l'; break;
        case 0x2C: key = 'z'; break;
        case 0x2D: key = 'x'; break;
        case 0x2E: key = 'c'; break;
        case 0x2F: key = 'v'; break;
        case 0x30: key = 'b'; break;
        case 0x31: key = 'n'; break;
        case 0x32: key = 'm'; break;

        // New operators for calculator
        case 0x4E: key = '-'; break; // '-'
        case 0x4A: key = '/'; break; // '/'
        case 0x37: key = '*'; break; // '*'
        case 0x4C: key = '+'; break; // '+'

        default: break;
    }

    if (key != '\0') {
        if (key == '\n') {
            process_input();
            input_index = 0;
        } else {
            if (input_index < sizeof(input_buffer) - 1) {
                input_buffer[input_index++] = key;
                print_char(key);
            }
        }
    }
}


// Globals for Tic-Tac-Toe
char board[3][3];
char current_player;

// Function Prototypes for Tic-Tac-Toe
void start_tictactoe(void);
void display_board(void);
void make_move(int row, int col);
int check_winner(void);
void reset_board(void);

// Start the Tic-Tac-Toe game
void start_tictactoe(void) {
    reset_board();
    current_player = 'X';
    display_text("Tic-Tac-Toe started! Use row and col (e.g., 1 1).", get_cursor_row(), 0);
    display_board();
}

// Display the Tic-Tac-Toe board
// Display the Tic-Tac-Toe board
void display_board(void) {
    char row_text[16];
    for (int i = 0; i < 3; i++) {
        int index = 0;
        row_text[index++] = ' ';
        row_text[index++] = board[i][0];
        row_text[index++] = ' ';
        row_text[index++] = '|';
        row_text[index++] = ' ';
        row_text[index++] = board[i][1];
        row_text[index++] = ' ';
        row_text[index++] = '|';
        row_text[index++] = ' ';
        row_text[index++] = board[i][2];
        row_text[index] = '\0'; // Null-terminate the string
        display_text(row_text, get_cursor_row() + (i * 2), 0);

        if (i < 2) {
            display_text("---|---|---", get_cursor_row() + (i * 2) + 1, 0);
        }
    }
    cursor_pos = (get_cursor_row() + 6) * SCREEN_WIDTH; // Position cursor for input
    update_cursor(cursor_pos);
}


// Make a move in the game
void make_move(int row, int col) {
    if (row < 1 || row > 3 || col < 1 || col > 3) {
        display_text("Invalid position! Use row and col (1-3).", get_cursor_row(), 0);
        return;
    }

    row--; col--; // Convert to zero-based index
    if (board[row][col] != ' ') {
        display_text("Position already taken!", get_cursor_row(), 0);
        return;
    }

    board[row][col] = current_player;

    if (check_winner()) {
        display_board();
        char result_text[32];
        char current_player_str[2] = { current_player, '\0' }; // Convert char to string
        snprintf(result_text, sizeof(result_text), "Player %s wins!", current_player_str, 0);
        display_text(result_text, get_cursor_row(), 0);
        reset_board();
        return;
    }

    // Check for a draw
    int draw = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') {
                draw = 0;
                break;
            }
        }
        if (!draw) break;
    }

    if (draw) {
        display_board();
        display_text("It's a draw!", get_cursor_row(), 0);
        reset_board();
        return;
    }

    // Switch players
    current_player = (current_player == 'X') ? 'O' : 'X';
    display_board();
}

// Check if there's a winner
int check_winner(void) {
    // Check rows, columns, and diagonals
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == current_player && board[i][1] == current_player && board[i][2] == current_player) return 1;
        if (board[0][i] == current_player && board[1][i] == current_player && board[2][i] == current_player) return 1;
    }
    if (board[0][0] == current_player && board[1][1] == current_player && board[2][2] == current_player) return 1;
    if (board[0][2] == current_player && board[1][1] == current_player && board[2][0] == current_player) return 1;

    return 0;
}

// Reset the Tic-Tac-Toe board
void reset_board(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}
// Function to create a variable
void create_variable(const char *name, const char *value) {
    if (var_count < MAX_VARS) {
        strncpy(variables[var_count].name, name, VAR_NAME_LEN - 1);
        variables[var_count].name[VAR_NAME_LEN - 1] = '\0'; // Ensure null termination
        strncpy(variables[var_count].value, value, VAR_VALUE_LEN - 1);
        variables[var_count].value[VAR_VALUE_LEN - 1] = '\0'; // Ensure null termination
        var_count++;
        display_text("Variable created!", get_cursor_row(), 0);
    } else {
        display_text("Variable limit reached!", get_cursor_row(), 0);
    }
}

// Function to get a variable's value
char *get_variable_value(const char *name) {
    for (size_t i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL; // Variable not found
}
// Function to display all variables
void display_variables(void) {
    for (size_t i = 0; i < var_count; i++) {
        char output[VAR_NAME_LEN + VAR_VALUE_LEN + 10]; // Buffer for output
        snprintf(output, sizeof(output), "%s: %s", variables[i].name, variables[i].value);
        display_text(output, get_cursor_row(), 0);
    }
    cursor_pos = (get_cursor_row() + var_count + 1) * SCREEN_WIDTH; // Move cursor below displayed variables
    update_cursor(cursor_pos);
}

// Function to fill the screen area with a specified character and color
void fill_area(char fill_char, uint8_t fg_color, uint8_t bg_color, uint16_t start_row, uint16_t start_col, uint16_t height, uint16_t width) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t offset;
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            offset = (start_row + row) * SCREEN_WIDTH + (start_col + col);
            if (offset < SCREEN_WIDTH * SCREEN_HEIGHT) { // Ensure we don't go out of bounds
                video_memory[offset] = fill_char | ((fg_color | (bg_color << 4)) << 8);
            }
        }
    }
    cursor_pos = (start_row + height) * SCREEN_WIDTH; // Move cursor below filled area
    update_cursor(cursor_pos);
}
// Function to print text in a specified color
void print_color_text(const char *text, const char *color_name) {
    int color_code = color_code_from_name(color_name);
    if (color_code == -1) {
        display_text("Invalid color name!", get_cursor_row() + 1, 0);
        return;
    }
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t offset = cursor_pos; // Use current cursor position
    while (*text) {
        video_memory[offset++] = *text | ((color_code | (bg_color << 4)) << 8);
        text++;
    }
    cursor_pos = offset; // Update cursor position
    update_cursor(cursor_pos);
}

// Function to set the splash screen color
void set_color_splash(const char *color_name) {
    int color_code = color_code_from_name(color_name);
    if (color_code == -1) {
        display_text("Invalid color name!", get_cursor_row(), 0);
        return;
    }
    text_color1 = color_code;
    display_text("Splash text color updated!", get_cursor_row(), 0);
}
// Pause Execution
void pause_com(void) {
    display_text("Press any key to continue...", get_cursor_row(), 0);
    while (!(inb(KEYBOARD_STATUS_PORT) & 0x01)); // Wait for key press
}

// Shutdown Command
void shutdown_system(void) {
    display_text("System shutting down...", get_cursor_row(), 0);
    // Issue halt instruction
    __asm__ __volatile__("cli; hlt");
}
// Execute commands (extended with tictactoe)
void execute_command(const char *command) {
    if (strcmp(command, "cls") == 0) {
        clear_screen();
    } else if (strncmp(command, "setcolor ", 9) == 0) {
        char *args = (char *)command + 9;
        char *fg_str = strtok(args, " ");
        char *bg_str = strtok(NULL, " ");

        if (fg_str && bg_str) {
            int fg = color_code_from_name(fg_str);
            int bg = color_code_from_name(bg_str);

            if (fg != -1 && bg != -1) {
                text_color = fg;
                bg_color = bg;
                clear_screen();
                display_text("Colors updated successfully!", get_cursor_row(), 0);
            } else {
                display_text("Invalid color names!", get_cursor_row(), 0);
            }
        } else {
            display_text("Usage: setcolor <fg> <bg>", get_cursor_row(), 0);
        }
    } else if (strcmp(command, "shutdown") == 0) {
        shutdown_system(); // Call shutdown
    } else if (strcmp(command, "help") == 0) {
        display_text("Available commands:",get_cursor_row(), 0);
        display_text("cls - clear screen",get_cursor_row() + 1, 0);
        display_text("shutdown - shut down the system",get_cursor_row() + 2, 0);
        display_text("tictactoe - play Tic-Tac-Toe",get_cursor_row() + 3, 0);
        display_text("move - make a move in Tic-Tac-Toe",get_cursor_row() + 4, 0);
        display_text("calc 1 + 1 - calculator",get_cursor_row() + 5, 0);
        display_text("setcolor - set text and bg color",get_cursor_row() + 6, 0);
        display_text("pause - pause",get_cursor_row() + 7, 0);
        display_text("setsplash <text> - set splash screen",get_cursor_row() + 8, 0);
        display_text("printcolortext <color> <text> - print text in color", get_cursor_row() + 9, 0);
        display_text("setcolorsplash <color> - set splash text color", get_cursor_row() + 10, 0);
        display_text("createvar <name> <value> - create a variable", get_cursor_row() + 11, 0);
        display_text("var <name> <value> - assign a value to a variable", get_cursor_row() + 12, 0);
        display_text("showvars - Displays all defined variables and their values", get_cursor_row() + 13, 0);
        display_text("fill <char> <fg_color> <bg_color> <height> <width> - Fills a specified area with a character and colors", get_cursor_row() + 14, 0);

        // Move cursor below the displayed text
        cursor_pos = (get_cursor_row() + 16) * SCREEN_WIDTH; // 5 lines of help text
        update_cursor(cursor_pos);
    } else if (strcmp(command, "showvars") == 0) {
        display_variables(); // Show all variables
    } else if (strncmp(command, "fill ", 5) == 0) {
        clear_screen();
        char *args = (char *)command + 5;
        char *fill_char_str = strtok(args, " ");
        char *fg_color_str = strtok(NULL, " ");
        char *bg_color_str = strtok(NULL, " ");
        char *dimensions_str = strtok(NULL, " ");

        if (fill_char_str && fg_color_str && bg_color_str && dimensions_str) {
            char fill_char = fill_char_str[0]; // Take the first character as the fill character
            int fg_color = color_code_from_name(fg_color_str);
            int bg_color = color_code_from_name(bg_color_str);
            int height = atoi(dimensions_str);
            int width = atoi(strtok(NULL, " ")); // Get width from the next token

            if (fg_color != -1 && bg_color != -1) {
                fill_area(fill_char, fg_color, bg_color, 1, 1, height, width);
            } else {
                display_text("Invalid color names!", get_cursor_row(), 0);
            }
        } else {
            display_text("Usage: fill <char> <fg_color> <bg_color> <height> <width>", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "calc ", 5) == 0) {
        char *args = (char *)command + 5;
        char *op1_str = strtok(args, " ");
        char *op_str = strtok(NULL, " ");
        char *op2_str = strtok(NULL, " ");

        if (op1_str && op_str && op2_str) {
            int op1 = atoi(op1_str);
            int op2 = atoi(op2_str);
            int result = 0;

            if (strcmp(op_str, "+") == 0) {
                result = op1 + op2;
            } else if (strcmp(op_str, "-") == 0) {
                result = op1 - op2;
            } else if (strcmp(op_str, "*") == 0) {
                result = op1 * op2;
            } else if (strcmp(op_str, "/") == 0 && op2 != 0) {
                result = op1 / op2;
            } else {
                display_text("Invalid operator or division by zero!", get_cursor_row(), 0);
                return;
            }

            char result_text[64];
            snprintf(result_text, sizeof(result_text), "Result: %d", 0, result);
            display_text(result_text, get_cursor_row(), 0);
        } else {
            display_text("Invalid calculator syntax!", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "setsplash ", 10) == 0) {
        const char *new_splash = command + 10; // Get the new splash text
        set_splash(new_splash);
        clear_screen();
    } else if (strcmp(command, "pause") == 0) {
        pause_com();
    } else if (strcmp(command, "tictactoe") == 0) {
        start_tictactoe();
    } else if (strncmp(command, "move ", 5) == 0) {
        char *args = (char *)command + 5;
        char *row_str = strtok(args, " ");
        char *col_str = strtok(NULL, " ");
        if (row_str && col_str) {
            int row = atoi(row_str);
            int col = atoi(col_str);
            make_move(row, col);
        } else {
            display_text("Invalid move syntax! Use move row col.", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "printcolortext ", 15) == 0) {
        char *args = (char *)command + 15;
        char *color_name = strtok(args, " ");
        char *text = strtok(NULL, "\0"); // Get the rest of the string as text
        if (color_name && text) {
            print_color_text(text, color_name);
        } else {
            display_text("Usage: printcolortext <color> <text>", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "setcolorsplash ", 15) == 0) {
        char *args = (char *)command + 15;
        char *color_name = strtok(args, " ");
        if (color_name) {
            set_color_splash(color_name);
        } else {
            display_text("Usage: setcolorsplash <color>", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "createvar ", 10) == 0) {
        char *args = (char *)command + 10;
        char *var_name = strtok(args, " ");
        char *var_value = strtok(NULL, "\0"); // Get the rest of the string as value
        if (var_name && var_value) {
            create_variable(var_name, var_value);
        } else {
            display_text("Usage: createvar <name> <value>", get_cursor_row(), 0);
        }
    } else if (strncmp(command, "var", 5) == 0) {
        char *args = (char *)command + 5;
        char *var_name = strtok(args, " ");
        char *var_value = strtok(NULL, "\0"); // Get the rest of the string as value
        if (var_name && var_value) {
            create_variable(var_name, var_value);
        } else {
            display_text("Usage: var = <name> <value>", get_cursor_row(), 0);
        }
    } else {
        display_text("Unknown command! Enter help!", get_cursor_row(), 0);
    }
}
int color_code_from_name(const char *name) {
    if (strcmp(name, "black") == 0) return 0;
    if (strcmp(name, "blue") == 0) return 1;
    if (strcmp(name, "green") == 0) return 2;
    if (strcmp(name, "cyan") == 0) return 3;
    if (strcmp(name, "red") == 0) return 4;
    if (strcmp(name, "magenta") == 0) return 5;
    if (strcmp(name, "brown") == 0) return 6;
    if (strcmp(name, "light_gray") == 0) return 7;
    if (strcmp(name, "dark_gray") == 0) return 8;
    if (strcmp(name, "light_blue") == 0) return 9;
    if (strcmp(name, "light_green") == 0) return 10;
    if (strcmp(name, "light_cyan") == 0) return 11;
    if (strcmp(name, "light_red") == 0) return 12;
    if (strcmp(name, "light_magenta") == 0) return 13;
    if (strcmp(name, "yellow") == 0) return 14;
    if (strcmp(name, "white") == 0) return 15;
    return -1;
}


// Process input when the Enter key is pressed
void process_input(void) {
    input_buffer[input_index] = '\0'; // Null-terminate the string
    execute_command(input_buffer);
    cursor_pos = (get_cursor_row() + 1) * SCREEN_WIDTH; // Move to next line
    update_cursor(cursor_pos);
}

// Initialize the system
void init_system(void) {
    clear_screen();
    cursor_pos = 3 * SCREEN_WIDTH; // Start at line 3
    update_cursor(cursor_pos);
}

// Main kernel entry point
void kmain(void) {
    init_system();
    while (1) {
        handle_keyboard(); // Poll for keyboard input
        display_text(splash_screen, 0, (SCREEN_WIDTH - strlen(splash_screen)) / 2);
    }
}
