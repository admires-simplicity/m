#ifndef TERMINAL_HANDLER_H
#define TERMINAL_HANDLER_H

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>

void enable_raw_mode();
void disable_raw_mode();
void die (const char*);

int get_win_size(int *rows, int *cols);
int get_cursor_position(int *rows, int *cols);

#endif
