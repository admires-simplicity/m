#include "termhandling.h"

struct termios orig_termios;

//write error message to screen and exit unsucessfully
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);	//clear screen
	write(STDOUT_FILENO, "\x1b[H", 3);	//position cursor 1,1

	disable_raw_mode(); //we'd disable it when we exited, but then the perror formatting would be messed up

	perror(s);
	exit(1);
}

//edit termios struct, so terminal behaves how I want it to
void enable_raw_mode() {
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");	//get termios struct
	atexit(disable_raw_mode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //bin AND on c_lflag bitminefield, where echo, icanon bits are 0 , but all others 1, i.e. turn specified options off, and keep others as they are
	raw.c_cc[VMIN] = 0;	//control characters minefield
	raw.c_cc[VTIME] = 1;


	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //set termios struct
}

//set the termios struct to be same as it was when program executed
void disable_raw_mode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die ("tcsetattr");
}

//write the cursor position in terminal to rows, cols
int get_cursor_position(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		++i;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

//write window height and length to rows, cols
int get_win_size(int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return get_cursor_position(rows, cols);
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}
