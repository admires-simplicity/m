/*** INCLUDES ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** DEFINES ***/
#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k)	((k) & 0x1f) //binary and with lowest 5 bits (decimal 31), therefor, ON bits in lowest 5 bits, which works because 'a' = 1100001 (97), and ctrl+a = 1

enum key {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN
};

enum states {
	OFF,
	FLAGGED,
	ACTIVATED
};

/*** DATA ***/
struct termhandler { //struct for handling terminal variables
	int cx, cy;
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct termhandler T;

typedef struct cell {
	int x;
	int y;
	int prox;	//number of bombs in proximity
	int bomb;	//is/isn't bomb
	int state;	
} cell;

typedef struct map {
	//cell c[x_rows][y_rows];
} map;

/*** PROTOTYPES ***/
void enable_raw_mode();
void disable_raw_mode();
void die (const char*);

int read_key();
void process_keypress();
void refresh_screen();

void init_editor();
int get_win_size(int *rows, int *cols);

//int minefield[][30] =
//{ //probably like 50 bombs
//	{0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},	//30
//	{0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0},
//	{0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0},
//	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
///*7*/	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
//	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
//};
//
//int field_w = sizeof *minefield / sizeof (int);
//int field_h = sizeof minefield / (field_w * sizeof(int)); // / (sizeof field_w * sizeof (int));
//
//sizeof minefield = sizeof field_w * sizeof field_h * sizeof (int);

int h = 16;
int w = 30;
int b = 0;

int field[16][30];


/*** INIT ***/
int main(void) {
	enable_raw_mode();
	init_editor();
//
//	make_field(29, 16);
//
	while (1) {			//forever
		refresh_screen();	//refresh the screen
		//printf("screen is %d x %d\r\n", T.screencols, T.screenrows);
		process_keypress();	//handle keypress (get keypress (wait for keypress))
	}
//	printf("field_w = %d\n", field_w);
//	printf("field_h = %d\n", field_h);




	return 0;
}


void initialize_field()
{
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			field[i][j] = 0;
}





//initialize fields in T struct
void init_editor() {
	T.cx = 0;
	T.cy = 0;

	initialize_field();

	if (get_win_size(&T.screenrows, &T.screencols) == -1) die("get_win_size");
}

/*** TERMINAL ***/
//write error message to screen and exit unsucessfully
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);	//clear screen
	write(STDOUT_FILENO, "\x1b[H", 3);	//position cursor 1,1

	perror(s);
	exit(1);
}

//edit termios struct, so terminal behaves how I want it to
void enable_raw_mode() {
	if (tcgetattr(STDIN_FILENO, &T.orig_termios) == -1) die("tcgetattr");	//get termios struct
	atexit(disable_raw_mode);

	struct termios raw = T.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //bin AND on c_lflag bitfield, where echo, icanon bits are 0 , but all others 1, i.e. turn specified options off, and keep others as they are
	raw.c_cc[VMIN] = 0;	//control characters field
	raw.c_cc[VTIME] = 1;


	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //set termios struct
}

//set the termios struct to be same as it was when program executed
void disable_raw_mode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &T.orig_termios) == -1) die ("tcsetattr");
}

//wait for one keypress, and return it
int read_key() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {		//while have not read any bits
		if (nread == -1 && errno != EAGAIN) die ("read");	//if read function returns error, kill program
	}

	if (c == '\x1b') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[') {
			switch(seq[1]) {
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
			}
		}

		return '\x1b';
	} else {
		return c;
	}
}

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

/*** APPEND BUFFER ***/
struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

/*** OUTPUT ***/
////void draw_rows(struct abuf *ab) {
////	int y;
////	for (y = 0; y < T.screenrows; ++y) {
////
////		//if (y == E.screenrows / 3) {
////		//	char welcome[80];
////		//	int welcomelen = snprintf(welcome, sizeof(welcome),
////		//			"Kilo editor -- version %s", KILO_VERSION);
////		//	if (welcomelen > E.screencols) welcomelen = E.screencols;
////		//	int padding = (E.screencols - welcomelen) / 2;
////		//	if (padding) {
////		//		abAppend(ab, "~", 1);
////		//		--padding;
////		//	}
////		//	while (padding--) abAppend(ab, " ", 1);
////		//	abAppend(ab, welcome, welcomelen);
////		//} else {
////		//	abAppend(ab, "~", 1);
////		//}
////
////
////		//abAppend(ab, "\x1b[K", 3);
////		//if (y < E.screenrows - 1) {
////		//	abAppend(ab, "\r\n", 2);
////		//}
////	}
////}

void draw_field(struct abuf *ab)
{
//	char buf[32];
//	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", T.cy + 1, T.cx + 1); //write move command to H buf, with values converted from 0indexed to 1indexed
//	abAppend(ab, buf, strlen(buf));

	char *c;
	for (int i = 0; i < h; ++i) {
		//printf("%2d: ", i);
		for (int j = 0; j < w; ++j) {
			////snprintf(c, 1, "%d", field[i][j]);
			//printf("%d", field[i][j]);
			abAppend(ab, "0", 1);
		}
		abAppend(ab, "\r\n", 2);
	}
}

void refresh_screen() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6); //Hide cursor
	abAppend(&ab, "\x1b[2J", 4); //erase (all) in display //escape sequence starts with ESC (decimal 27), [ ...
	abAppend(&ab, "\x1b[H", 3);

	//draw_rows(&ab);
	draw_field(&ab);
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", T.cy + 1, T.cx + 1); //write move command to H buf, with values converted from 0indexed to 1indexed
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6); //show cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/*** INPUT ***/
void move_cursor(int key) {
	switch (key) {
		case ARROW_LEFT:
		case 'h':
			if (T.cx != 0) {
				--T.cx;
			}
			break;
		case ARROW_RIGHT:
		case 'l':
			if (T.cx != T.screencols - 1) {
			 ++T.cx;
			}
			break;
		case ARROW_UP:
		case 'k':
			if (T.cy != 0) {
				--T.cy;
			}
			break;
		case ARROW_DOWN:
		case 'j':
			if (T.cy != T.screenrows - 1) {
				++T.cy;
			}
			break;
	}
}

//get a key and handle it
void process_keypress() {
	int c = read_key();

	switch(c) {
		case CTRL_KEY('q'):
			//write(STDOUT_FILENO, "\x1b[2J", 4);	//clear screen
			//write(STDOUT_FILENO, "\x1b[H", 3);	//position cursor 1,1

			exit (0);
			break;
		//case 'a':
			//write(STDOUT_FILENO, "lol", 3);

		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
		case 'h':
		case 'j':
		case 'k':
		case 'l':
			move_cursor(c);
			break;



		default:
			//printf("KEY UNKNOWN!\r\n");
			break;
	}
}
