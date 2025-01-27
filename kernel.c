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
#define COLOR_ATTRIBUTE (text_color | (bg_color << 4))
// Color codes
#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define RED 0x4
#define MAGENTA 0x5
#define BROWN 0x6
#define LIGHT_GRAY 0x7
#define DARK_GRAY 0x8
#define LIGHT_BLUE 0x9
#define LIGHT_GREEN 0xA
#define LIGHT_CYAN 0xB
#define LIGHT_RED 0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW 0xE
#define WHITE 0xF
#define BIOS_MEMORY_LOCATION 0x413 // Memory size (KB)

// Globals
static char input_buffer[256];
static size_t input_index = 0;
static uint16_t cursor_pos = 0;
static uint8_t text_color = WHITE;
static uint8_t bg_color = BLUE;


// Function Prototypes
void init_system(void);
void clear_screen(void);
void display_text(const char *text, uint16_t row, uint16_t col);
void handle_keyboard(void);
void update_cursor(uint16_t position);
void process_input(void);
void print_char(char c);
uint16_t get_cursor_row(void);
uint16_t get_cursor_col(void);
void execute_command(const char *command);

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
// Function to convert color name to numeric value
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
    return -1; // Invalid name
}

void snprintf(char *str, size_t size, const char *format, int value) {
    const char *p = format;
    char *out = str;

    while (*p && (out - str) < (size - 1)) {
        if (*p == '%' && *(p + 1) == 'd') {
            char num_buffer[32];
            itoa(value, num_buffer, 10);
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

// Clear the screen and repaint the entire background
void clear_screen(void) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t blank = ' ' | (COLOR_ATTRIBUTE << 8); // Use current COLOR_ATTRIBUTE for blank spaces
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video_memory[i] = blank; // Fill with blank spaces and current background color
    }
    cursor_pos = 3 * SCREEN_WIDTH; // Start input on line 3
    update_cursor(cursor_pos);
}


// Display a string at a specific row and column
void display_text(const char *text, uint16_t row, uint16_t col) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
    uint16_t offset = row * SCREEN_WIDTH + col;
    while (*text) {
        video_memory[offset++] = *text | (COLOR_ATTRIBUTE << 8);
        text++;
    }
}

// Print a single character to the screen at the current cursor position
void print_char(char c) {
    uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;

    if (c == '\n') {
        cursor_pos = (cursor_pos / SCREEN_WIDTH + 1) * SCREEN_WIDTH; // Move to the next row
    } else {
        video_memory[cursor_pos++] = c | (COLOR_ATTRIBUTE << 8);
    }

    // Update cursor and wrap around if needed
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
        snprintf(result_text, sizeof(result_text), "Player %c wins!", current_player);
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


// Shutdown Command
void shutdown_system(void) {
    display_text("System shutting down...", get_cursor_row(), 0);
    // Issue halt instruction
    __asm__ __volatile__("cli; hlt");
}
// Function to get CPU vendor
void get_cpu_vendor(char *vendor) {
    uint32_t eax, ebx, ecx, edx;
    eax = 0; // CPUID with EAX=0 gets vendor string
    __asm__ __volatile__(
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax)
    );
    *(uint32_t *)(vendor + 0) = ebx;
    *(uint32_t *)(vendor + 4) = edx;
    *(uint32_t *)(vendor + 8) = ecx;
    vendor[12] = '\0'; // Null-terminate the string
}

// Function to get installed memory size
int get_memory_size_kb(void) {
    return *((uint16_t *)BIOS_MEMORY_LOCATION); // BIOS memory location 0x413
}

// Display PC characteristics
void display_sysinfo(void) {
    char vendor[13];
    get_cpu_vendor(vendor);

    int memory_size_kb = get_memory_size_kb();
    char mem_buffer[32];

    // Display system info
    display_text("System Information:", get_cursor_row(), 0);
    display_text("CPU Vendor: ", get_cursor_row() + 1, 0);
    display_text(vendor, get_cursor_row() + 1, 12);

    snprintf(mem_buffer, sizeof(mem_buffer), "Memory: %d KB", memory_size_kb);
    display_text(mem_buffer, get_cursor_row() + 2, 0);

    cursor_pos = (get_cursor_row() + 4) * SCREEN_WIDTH; // Move cursor below info
    update_cursor(cursor_pos);
}

// Execute commands (extended with sysinfo)
void execute_command(const char *command) {
    if (strcmp(command, "cls") == 0) {
        clear_screen();
    } else if (strcmp(command, "shutdown") == 0) {
        shutdown_system();
    } else if (strcmp(command, "help") == 0) {
        display_text("Available commands:", get_cursor_row(), 0);
        display_text("cls - clear screen", get_cursor_row() + 1, 0);
        display_text("shutdown - shut down the system", get_cursor_row() + 2, 0);
        display_text("tictactoe - play Tic-Tac-Toe", get_cursor_row() + 3, 0);
        display_text("move - make a move in Tic-Tac-Toe", get_cursor_row() + 4, 0);
        display_text("setcolor - change text and background colors", get_cursor_row() + 5, 0);
        display_text("sysinfo - display system information", get_cursor_row() + 6, 0);

        // Move cursor below the displayed text
        cursor_pos = (get_cursor_row() + 7) * SCREEN_WIDTH;
        update_cursor(cursor_pos);
    } else if (strcmp(command, "sysinfo") == 0) {
        display_sysinfo();
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
                display_text("Color updated!", get_cursor_row(), 0);
            } else {
                display_text("Invalid color names! Use valid names (e.g., red white).", get_cursor_row(), 0);
            }
        } else {
            display_text("Invalid syntax! Use setcolor <foreground> <background>.", get_cursor_row(), 0);
        }
    } else {
        display_text("Unknown command!", get_cursor_row(), 0);
    }
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
        display_text("Welcome to DubrDos!", 0, 23);
        handle_keyboard(); // Poll for keyboard input
    }
}
