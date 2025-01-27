DubrDOS is a simple, educational operating system designed to run in low-level environments. It provides basic command-line functionality, games, and system utilities to demonstrate OS concepts like hardware interaction, keyboard handling, and text-based UI.

Features
Command-line Interface:

Execute commands directly from the terminal.
Handle keyboard input, backspace, and enter commands.

Basic Commands

cls: Clear the screen.
help: Display a list of available commands.
shutdown: Simulate shutting down the system.
setcolor <foreground> <background>: Change the text and background colors.
calc <number1> <operator> <number2>: Perform basic arithmetic operations (+, -, *, /).
tictactoe: Play a simple game of Tic-Tac-Toe.

Tic-Tac-Toe Game

Interactive two-player game.
Move commands allow players to make their moves (e.g., move 1 1).
Customizable Display:

Change text and background colors using the setcolor command.
Keyboard Input Handling:

Dynamically process input characters and handle special keys (like backspace).
How to Build and Run
Prerequisites
x86 emulator: Use QEMU or Bochs.
Compiler: A cross-compiler for x86 (e.g., GCC targeting i386).
Assembler: NASM (Netwide Assembler) for assembly language files.
Steps

Build the Kernel:

nasm -f elf32 kernel.asm -o kasm.o

gcc -m32 -c kernel.c -o kc.o

ld -T link.ld -o k kasm.o kc.o -build-id=none

objcopy -O elf32-i386 k kernel-5

Commands Overview
Command	Description

cls	Clears the screen.

help	Displays a list of available commands.

shutdown	Simulates a system shutdown.

setcolor <fg> <bg>	Changes the text (fg) and background (bg) colors. Example: setcolor red black.

calc <num1> <op> <num2>	Performs a calculation. Example: calc 5 + 3. Supports +, -, *, /.

tictactoe	Starts the Tic-Tac-Toe game.

move <row> <col>	Makes a move in Tic-Tac-Toe. Example: move 1 1.

Color Options for setcolor

Color Name	Code	Color Name	Code

black	0	light_gray	7

blue	1	dark_gray	8

green	2	light_blue	9

cyan	3	light_green	10

red	4	light_cyan	11

magenta	5	light_red	12

brown	6	light_magenta	13

yellow	14	white	15

Project Structure

kernel.asm: The bootloader that initializes the system and loads the kernel.
kernel.c: The main kernel file containing command handling, UI, and system logic.
link.ld: Linker script to place the kernel at the correct memory address.
Usage Tips
Start the system by running the OS image in an emulator.
Use the help command to discover available functionality.
Customize the display using setcolor to make your terminal unique.

License

This project is intended for educational purposes. You are free to modify, redistribute, and experiment with the code as needed.

Enjoy learning about low-level systems with DubrDOS! 🎉
