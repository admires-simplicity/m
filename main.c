/*** INCLUDES ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#include "termhandling.h"

/*** DEFINITIONS ***/
#define CTRL_KEY(k)	((k) & 0x1f) //binary and with lowest 5 bits (decimal 31), therefor, ON bits in lowest 5 bits, which works because 'a' = 1100001 (97), and ctrl+a = 1

#define TRUE	1
#define FALSE	0

enum key {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN
};

enum states {
	OFF = 1000,
	ACTIVATED,
	EMPTY,
	FLAGGED,
	MARKED
};

/*** DATA ***/
struct termhandler { //struct for handling terminal variables
	int cx, cy;	//coordinates on the terminal
	int screenrows;
	int screencols;
	//struct termios orig_termios;
};

struct termhandler T;

//OLD COMMENT
/*I think if I put the minefield variables inside this gamehandler, I would be able to initialize them by variables?
 * Maybe, But I still don't know why that'd be better than just initializing them by variables outside everything...*/
/*struct for handling position by game cell, rather than position by terminal cell*/
struct gamehandler { 
	int cx, cy;	//coordinates in the game
	//int h;
	//int w;
	//int b;
} G;


//minefield variables
int h;
int w;
int b;

//counter variables
int flagged_spaces;
int unknown_cells;

//modifying variables
int new_h;
int new_w;

int alive;

//array to store bombs
//int minefield[16][30];
int** minefield;
//array to store player's 
//int gamefield[16][30];
int** gamefield;
//array for space checking DESCRIBE THIS BETTER
//int zcheck[16][30];
int** zcheck;


int generation = 1;	//0 = random, 1 = friendly

/*** FUNCTION PROTOTYPES ***/
int read_key();
void process_keypress();
void refresh_screen();

void init_game(int gen, int initial_h, int initial_w);
int get_win_size(int *rows, int *cols);

void move_cursor(int key);

void free_fields();

void cleanup();

/*** INIT ***/
int main(int argc, char* argv[]) {
	int init_h = 0, init_w = 0;
	char* end;
	
	enable_raw_mode();
	atexit(cleanup);

	while (--argc > 0 && (*++argv)[0] == '-') {
		errno = 0;
		switch((*argv)[1]) {
			case ('h'):
				if (argc > 1) {
					if (!(init_h = strtol(*(++argv), &end, 10)))
						die("Invalid height input");
					if (*end != '\0')
						die("Inconvertible height input");
					--argc;
				} else {
					die("You didn't enter a height!");
				}
				break;
			case ('w'):
				if (argc > 1) {
					if (!(init_w = strtol(*(++argv), &end, 10)))
						die("Invalid width input");
					if (*end != '\0')
						die("Inconvertible width input");
					--argc;
				} else {
					die("You didn't enter a width!");
				}
				break;
		}
	}


	init_game(generation, init_h, init_w);

	while (alive) {			//forever
		refresh_screen();	//refresh the screen
		process_keypress();	//handle keypress (get keypress (wait for keypress))
	}
	refresh_screen();

	return 0;
}

void cleanup() {
	write(STDOUT_FILENO, "\x1b[999;1H", 8);	//position cursor [FIX HARDCODING]
	write(STDOUT_FILENO, "\x1b[0m", 4);	//reset colors
	free_fields();
}

//Return the number of adjacent bombs, at a given x,y coordinate, in the minefield
int adjacent_bombs(int y, int x)
{
	int adj = 0;

	if (           y > 0   && minefield[y-1][x  ]) ++adj;	// ABOVE
	if (x < w-1 && y > 0   && minefield[y-1][x+1]) ++adj;	// ABOVE + RIGHT
	if (x < w-1            && minefield[y  ][x+1]) ++adj;	// RIGHT
	if (x < w-1 && y < h-1 && minefield[y+1][x+1]) ++adj;	// DOWN + RIGHT 
	if (           y < h-1 && minefield[y+1][x  ]) ++adj;	// DOWN
	if (x > 0   && y < h-1 && minefield[y+1][x-1]) ++adj;	// DOWN + LEFT
	if (x > 0              && minefield[y  ][x-1]) ++adj;	// LEFT
	if (x > 0   && y > 0   && minefield[y-1][x-1]) ++adj;	// LEFT + UP

	return adj;
}

void random_generate_minefield();
void friendly_generate_minefield();

void generate_minefield(int generation) {
	if (generation == 0)
		random_generate_minefield();
	else if (generation == 1)
		friendly_generate_minefield();
	else if (generation == -1)
		;	// THIS MEANS DO NOTHING INTENTIONALLY
	else
		die("GENERATION COMMAND NOT UNDERSTOOD");
}

//Set the minefield to initial conditions
void initialize_fields(int gen)
{
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j) {
			minefield[i][j] = 0;
			gamefield[i][j] = OFF;	//init gamefield
			zcheck[i][j] = 0;
		}
	
	generate_minefield(gen);
}

void random_generate_minefield()
{
	int pos;		//for current position in loop
	int bombs_placed = 0;
	int repeat = FALSE;
	int position[b];

	//initialize empty position array
	for (int i = 0; i < b; ++i)
		position[i] = 0;

	//place req'd num bombs
	while (bombs_placed < b) {
		//random in acceptable range
		pos = rand() % (w * h);

		//check against array (could replace with binary tree for efficiency???)
		for (int i = 0; i < bombs_placed; ++i)
			if (position[i] == pos) {
				repeat = TRUE;
				break;
			}

		//continue if repeat found
		if (repeat) {
			repeat = FALSE;
			continue;
		}

		//set pos in pos array
		position[bombs_placed] = pos;	//this maybe should be in loop?

		//set pos in minefield 2d array (could be way more efficient)
		int i = 0;	//index for number of cells
		for (int j = 0; j < h; ++j)
			for (int k = 0; k < w; ++k) {
				if (i == pos) {
					minefield[j][k] = 1; //bomb here
					++bombs_placed;
					k = w;
					j = h;
				} else 
					++i;
			}
	}

	//after req'd num bombs finished, not enough bombs -> error
	if (bombs_placed != b)
		die("NOT ENOUGH BOMBS!");
}

int coord_to_n(int y, int x)	//SHOULD THIS BE A MACRO? I GUESS NOT
{
	//I think this should be 0 indexed, so I'll build it that way, and if it needs to be 1 indexed i can change it later
	return (w * y) + x;
}

void friendly_generate_minefield()
{
	int pos;		//for current position in loop
	int bombs_placed = 0;
	int repeat = FALSE;
	int position[b+9];

	//initialize empty position array
	for (int i = 0; i < b; ++i)
		position[i] = 0;

	//ADJACENT MASK
	int x = 0;
	minefield[G.cy][G.cx] = 1;
	position[x] = coord_to_n(G.cy, G.cx);
	++x;
	if (              G.cy > 0  ) {
		minefield[G.cy-1][G.cx  ] = 1;	// ABOVE
		position[x] = coord_to_n(G.cy-1, G.cx);
		++x;
	}
	if (G.cx < w-1 && G.cy > 0  ) {
		minefield[G.cy-1][G.cx+1] = 1;	// ABOVE + RIGHT
		position[x] = coord_to_n(G.cy-1, G.cx+1);
		++x;
	}
	if (G.cx < w-1              ) {
		minefield[G.cy  ][G.cx+1] = 1;	// RIGHT
		position[x] = coord_to_n(G.cy, G.cx+1);
		++x;
	}
	if (G.cx < w-1 && G.cy < h-1) {
		minefield[G.cy+1][G.cx+1] = 1;	// DOWN + RIGHT 
		position[x] = coord_to_n(G.cy+1, G.cx+1);
		++x;
	}
	if (              G.cy < h-1) {
		minefield[G.cy+1][G.cx  ] = 1;	// DOWN
		position[x] = coord_to_n(G.cy+1, G.cx);
		++x;
	}
	if (G.cx > 0   && G.cy < h-1) {
		minefield[G.cy+1][G.cx-1] = 1;	// DOWN + LEFT
		position[x] = coord_to_n(G.cy+1, G.cx-1);
		++x;
	}
	if (G.cx > 0                ) {
		minefield[G.cy  ][G.cx-1] = 1;	// LEFT
		position[x] = coord_to_n(G.cy, G.cx-1);
		++x;
	}
	if (G.cx > 0   && G.cy > 0  ) {
		minefield[G.cy-1][G.cx-1] = 1;	// LEFT + UP
		position[x] = coord_to_n(G.cy-1, G.cx-1);
		++x;
	}


	//place req'd num bombs
	while (bombs_placed < b) {
		//random in acceptable range
		pos = rand() % (w * h);

		//check against array (could replace with binary tree for efficiency)
		for (int i = 0; i < bombs_placed+x; ++i)
			if (position[i] == pos) {
				repeat = TRUE;
				//++repeats;
				break;
			}

		//continue if repeat found
		if (repeat) {
			repeat = FALSE;
			continue;
		}

		//set pos in pos array
		position[bombs_placed+x] = pos;	//this maybe should be in loop?

		//set pos in minefield 2d array (could be way more efficient)
		int i = 0;	//index for number of cells
		for (int j = 0; j < h; ++j)
			for (int k = 0; k < w; ++k) {
				if (i == pos) {
					minefield[j][k] = 1; //bomb here
					++bombs_placed;
					k = w;
					j = h;
				} else 
					++i;
			}
	}

	//ADJACENT UNMASK
	minefield[G.cy][G.cx] = 0;
	if (              G.cy > 0  ) minefield[G.cy-1][G.cx  ] = 0;	// ABOVE
	if (G.cx < w-1 && G.cy > 0  ) minefield[G.cy-1][G.cx+1] = 0;	// ABOVE + RIGHT
	if (G.cx < w-1              ) minefield[G.cy  ][G.cx+1] = 0;	// RIGHT
	if (G.cx < w-1 && G.cy < h-1) minefield[G.cy+1][G.cx+1] = 0;	// DOWN + RIGHT 
	if (              G.cy < h-1) minefield[G.cy+1][G.cx  ] = 0;	// DOWN
	if (G.cx > 0   && G.cy < h-1) minefield[G.cy+1][G.cx-1] = 0;	// DOWN + LEFT
	if (G.cx > 0                ) minefield[G.cy  ][G.cx-1] = 0;	// LEFT
	if (G.cx > 0   && G.cy > 0  ) minefield[G.cy-1][G.cx-1] = 0;	// LEFT + UP

	//after req'd num bombs finished, not enough bombs -> error
	if (bombs_placed != b)
		die("NOT ENOUGH BOMBS!");

}


int** alloc_int_2d(int n, int m) {
	int** p = malloc((n * sizeof *p) + (n * m * sizeof **p)); //allocate p to point to a size of memory capable of storing n pointers to pointers and m pointers for each n
	int* first_location_of_second_dimension = (int*) (p + n);

	for (int i = 0; i < n; ++i) {
		p[i] = first_location_of_second_dimension + (i * m); //point pointer pointer i to a pointer loacated i * m pointers after the pointer pointers
	}

	return p;
}

void alloc_fields() {
	int i, j;

	minefield = alloc_int_2d(h, w);
	gamefield = alloc_int_2d(h, w);
	zcheck = alloc_int_2d(h, w);
}

int **realloc_int_2d(int** p, int n, int m) {
	p = realloc(p, ((n * sizeof *p) + (n * m * sizeof **p)));

	int* first_location_of_second_dimension = (int*) (p + n);

	for (int i = 0; i < n; ++i) {
		p[i] = first_location_of_second_dimension + (i * m);
	}

	return p;
}

void realloc_fields() {
	minefield = realloc_int_2d(minefield, h, w);
	gamefield = realloc_int_2d(gamefield, h, w);
	zcheck = realloc_int_2d(zcheck, h, w);

	if (minefield == NULL || gamefield == NULL || zcheck == NULL) {
		die("Some field was unable to reallocate (update this error message l8r)");		
	}
}

void free_fields() {
	free(minefield);
	free(gamefield);
	free(zcheck);
}

void reposition_cursor() {
	
}

int equivalent_coord(int old_coord, int old_max, int new_max) {
	float old_percent_of_max = old_coord / old_max;

	return old_percent_of_max * new_max;
}

void reset() {

	int i;
	

	float current_percent_y = (float) G.cy / (float) h;	
	float current_percent_x = (float) G.cx / (float) w;

	int new_y = (int) (current_percent_y * (float) new_h);
	int new_x = (int) (current_percent_x * (float) new_w);

	//int y_to_move = equivalent_coord(G.cy, h, new_h) - G.cy; 
	int y_to_move = new_y - G.cy;
	int x_to_move = new_x - G.cx;

	h = new_h;
	w = new_w;
	b = (int) ((float)h * (float)w) * 0.2125;

	realloc_fields();

	initialize_fields(-1);

//	if (y_to_move > 0) {
//		for (int i = 0; i < y_to_move; ++i) {
//			move_cursor(ARROW_DOWN);
//		}
//	}
//	else if (y_to_move < 0) {
//		for (int i = 0; i < -y_to_move; ++i) {
//			move_cursor(ARROW_UP);
//		}
//
//	}

	while (x_to_move || y_to_move) {
		if (y_to_move > 0) {
			move_cursor(ARROW_DOWN);
			--y_to_move;
		} else if (y_to_move < 0) {
			move_cursor(ARROW_UP);
			++y_to_move;
		}

		if (x_to_move > 0) {
			move_cursor(ARROW_RIGHT);
			--x_to_move;
		} else if (x_to_move < 0) {
			move_cursor(ARROW_LEFT);
			++x_to_move;
		}
	}

	generate_minefield(generation);
	//disable_raw_mode();
	//
	unknown_cells = h * w;
}

void center_cursor() {
	//center cursor in minefield (might have to move this later)
	T.cx = (w-1);
	if (w % 2) {
		--T.cx;
	}
	T.cy = (h-1)/2;

	G.cx = (w-1)/2;
	G.cy = (h-1)/2;
}

//initialize minefields in T struct
void init_game(int gen, int initial_h, int initial_w) {
	alive = TRUE;

	T.cx = G.cx = 0;
	T.cy = G.cy = 0;

	//SET MINEFIELD VARIABLES
	if (!initial_h)
		initial_h = 16;
	if (!initial_w)
		initial_w = 30;
	h = new_h = initial_h;
	w = new_w = initial_w;
	b = (int) ((float)h * (float)w) * 0.2125;

	unknown_cells = h * w;

	//minefield_alloc();
	//or should it be
	//alloc_2d(minefield);
	
	free_fields();

	alloc_fields();


	center_cursor();

	srand(time(NULL)); //seed random generator

	initialize_fields(gen);

	if (get_win_size(&T.screenrows, &T.screencols) == -1) die("get_win_size");
}

/*** TERMINAL ***/



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

void draw_minefield(struct abuf *ab)
{
//	char *c;
//	char buf[2]; //size for one character, and the null byte? lel
//	int z;
//
//	for (int i = 0; i < h; ++i) {
//		//printf("%2d: ", i);
//		for (int j = 0; j < w; ++j) {
//			if (minefield[i][j] == 1)
//				abAppend(ab, "@", 1);
//			else
//				if (z = adjacent_bombs(i, j)) {	//not 0 adj boms
//					snprintf(buf, sizeof(buf), "%d", z);
//					abAppend(ab, buf, strlen(buf));
//				} else {
//					abAppend(ab, ".", 1);
//				}
//		}
//		abAppend(ab, "\r\n", 2);
//	}

	char *c;
	char buf[2]; //size for one char and null byte			//AT SOME POINT THIS SHOULD BE REPLACED WITH A FUNCTION TO WRITE CHARS TO SCREEN
	int z;

	//abAppend(ab, "\x1b[45;5;231m", 11);
	
	
	//SET BACKGROUND COLOR
	abAppend(ab, "\x1b[48;5;235m", 11);
	//SET FOREGROUND WHITE
	abAppend(ab, "\x1b[38;5;15m", 10);


	for (int i = 0; i < h; ++i) {
		for (int j = 0; j < w; ++j) {
			abAppend(ab, " ", 1);
			switch (gamefield[i][j]) {
				case OFF:
					if (i == G.cy && j == G.cx) {
						if (minefield[i][j]) {
							abAppend(ab, "\x1b[38;5;231m", 11);
							abAppend(ab, "\x1b[48;5;9m", 9);
							abAppend(ab, "@", 1);
							abAppend(ab, "\x1b[38;5;15m", 10);
							abAppend(ab, "\x1b[48;5;235m", 11);
						}
						else {
							die("That's probably a fatal error, brother");
						}
					} else if (minefield[i][j]) {
						abAppend(ab, "\x1b[38;5;231m", 11);
						abAppend(ab, "@", 1);
						abAppend(ab, "\x1b[38;5;15m", 10);
					} else {
						abAppend(ab, ".", 1);
					}
					break;
				case ACTIVATED:
					abAppend(ab, "@", 1);
					break;
				case EMPTY:
					abAppend(ab, " ", 1);
					break;
				case FLAGGED:
					if (minefield[i][j]) {
						abAppend(ab, "\x1b[38;5;14m", 10);
						abAppend(ab, "!", 1);
						abAppend(ab, "\x1b[38;5;15m", 10);
					} else {
						abAppend(ab, "\x1b[38;5;14m", 10);
						abAppend(ab, "\x1b[48;5;9m", 9);
						abAppend(ab, "!", 1);
						abAppend(ab, "\x1b[38;5;15m", 10);
						abAppend(ab, "\x1b[48;5;235m", 11);

					}
					break;
				case MARKED:
					abAppend(ab, "\x1b[38;5;107m", 11);
					abAppend(ab, "?", 1);
					abAppend(ab, "\x1b[38;5;15m", 10);
					break;
				case 0:
					die("Sorry pal, the 0 is deprecated, LOL. Alternatively, i might switch everything to 0 later... LOL");
					abAppend(ab, ".", 1);
					break;
				default:	//PRINTING NUMBERS
					//roygbiv
					//vibgyor
					//v b y r i g o
					if (gamefield[i][j] == 1)
						abAppend(ab, "\x1b[38;5;200m", 11);
					else if (gamefield[i][j] == 2)
						abAppend(ab, "\x1b[38;5;33m", 10);
					else if (gamefield[i][j] == 3)
						abAppend(ab, "\x1b[38;5;11m", 10);
					else if (gamefield[i][j] == 4)
						abAppend(ab, "\x1b[38;5;40m", 10);
					else if (gamefield[i][j] == 5)
						abAppend(ab, "\x1b[38;5;63m", 10);
					else if (gamefield[i][j] == 6)
						abAppend(ab, "\x1b[38;5;19m", 10);
					else if (gamefield[i][j] == 7)
						abAppend(ab, "\x1b[38;5;133m", 11);
					else if (gamefield[i][j] == 8)
						abAppend(ab, "\x1b[38;5;196m", 11);

					buf[0] = gamefield[i][j] + '0';
					buf[1] = '\0';
					abAppend(ab, buf, 1);
					//RESET FOREGROUND
					abAppend(ab, "\x1b[38;5;15m", 10);
					break;
			}
		}
		abAppend(ab, "\r\n", 2);
	}

	abAppend(ab, "\x1b[48;5;0m", 9);

	//abAppend(ab, "\x1b[45;5;0m", 9);


}

void draw_gamefield(struct abuf *ab)
{
	char *c;
	char buf[2]; //size for one char and null byte			//AT SOME POINT THIS SHOULD BE REPLACED WITH A FUNCTION TO WRITE CHARS TO SCREEN
	int z;

	//abAppend(ab, "\x1b[45;5;231m", 11);
	
	
	//SET BACKGROUND COLOR
	abAppend(ab, "\x1b[48;5;235m", 11);
	//SET FOREGROUND WHITE
	abAppend(ab, "\x1b[38;5;15m", 10);


	for (int i = 0; i < h; ++i) {
		for (int j = 0; j < w; ++j) {
			abAppend(ab, " ", 1);
			switch (gamefield[i][j]) {
				case OFF:
					abAppend(ab, ".", 1);
					break;
				case ACTIVATED:
					abAppend(ab, "@", 1);
					break;
				case EMPTY:
					abAppend(ab, " ", 1);
					break;
				case FLAGGED:
					abAppend(ab, "\x1b[38;5;14m", 10);
					abAppend(ab, "!", 1);
					abAppend(ab, "\x1b[38;5;15m", 10);
					break;
				case MARKED:
					abAppend(ab, "\x1b[38;5;107m", 11);
					abAppend(ab, "?", 1);
					abAppend(ab, "\x1b[38;5;15m", 10);
					break;
				case 0:
					die("Sorry pal, the 0 is deprecated, LOL. Alternatively, i might switch everything to 0 later... LOL");
					abAppend(ab, ".", 1);
					break;
				default:	//PRINTING NUMBERS
					//roygbiv
					//vibgyor
					//v b y r i g o
					if (gamefield[i][j] == 1)
						abAppend(ab, "\x1b[38;5;200m", 11);
					else if (gamefield[i][j] == 2)
						abAppend(ab, "\x1b[38;5;33m", 10);
					else if (gamefield[i][j] == 3)
						abAppend(ab, "\x1b[38;5;11m", 10);
					else if (gamefield[i][j] == 4)
						abAppend(ab, "\x1b[38;5;40m", 10);
					else if (gamefield[i][j] == 5)
						abAppend(ab, "\x1b[38;5;63m", 10);
					else if (gamefield[i][j] == 6)
						abAppend(ab, "\x1b[38;5;19m", 10);
					else if (gamefield[i][j] == 7)
						abAppend(ab, "\x1b[38;5;133m", 11);
					else if (gamefield[i][j] == 8)
						abAppend(ab, "\x1b[38;5;196m", 11);

					buf[0] = gamefield[i][j] + '0';
					buf[1] = '\0';
					abAppend(ab, buf, 1);
					//RESET FOREGROUND
					abAppend(ab, "\x1b[38;5;15m", 10);
					break;
			}
		}
		abAppend(ab, "\r\n", 2);
	}

	abAppend(ab, "\x1b[48;5;0m", 9);

	//abAppend(ab, "\x1b[45;5;0m", 9);

}

//THIS IS DEPRECATED AND WAS ONLY EVER USED FOR DEBUGGING, DELETE IN FINAL
int count_bombs()
{
	int count = 0;
	for (int i = 0; i < h; ++i)
	       for (int j = 0; j < w; ++j)
		       if (minefield[i][j] == 1)
			       ++count;
	return count;
}


//what I was doing here turned out to be kinda hard, and probably not very important... I might need it for a timer though? maybe not...
//char* message_buffer;
//char buf_allocated = TRUE;	//I'm just using this as a boolean... maybe I could do this with a 
//int buflen = 0;
//
//void add_message_string(char *fmt, ...);
//
//void add_message_string(char *fmt, ...) {
//	int old_buflen = buflen;
//
//	va_list ap;
//	va_start(ap, fmt);	//make ap point to 1st unnamed arg
//	
//	//to do:
//	//	allocate message_buffer
//	//	then, make add_message_string realloc enough space for the passed string and arguments... (how?)
//	//			maybe I can just use a loop which determines whether there is enough space yet, and adds one extra char??? or maybe 2 chars???
//	
//	//maybe I could do an init message string in init_game and a free_message_string in ctrl-q? and in ctrl-r
//	if (!buf_allocated) {
//		message_buffer = malloc(buflen = strlen(fmt) * sizeof *fmt);
//		buf_allocated = TRUE;
//	}
//	
//
//	int x;
//
//	while (!realloc(message_buffer, buflen * sizeof(*message_buffer))) { //buffer is not long enough
//		buflen += 2;
//		vsnprintf(message_buffer, sizeof(message_buffer), fmt, ap);
//	}
//	
//	strcpy(message_buffer[old_buflen], 
//
//	va_end(ap);
//}
//
//void free_message_string() {
//	free(message_buffer);
//	buf_allocated = FALSE;
//}

void refresh_screen() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6); //Hide cursor
	abAppend(&ab, "\x1b[2J", 4); //erase (all) in display //escape sequence starts with ESC (decimal 27), [ ...
	abAppend(&ab, "\x1b[H", 3);

	//draw_rows(&ab);
	//draw_minefield(&ab);
	if (alive){
		draw_gamefield(&ab);
	} else {
		//draw_gamefield(&ab);	//TEMPORARY
		draw_minefield(&ab);
	}


	char buf[30];
	snprintf(buf, sizeof(buf), "Unaccounted bombs: %d\r\n", b - flagged_spaces);
	abAppend(&ab, buf, strlen(buf));

	char bug[30];
	snprintf(bug, sizeof(bug), "Flagged spaces:    %d\r\n", flagged_spaces);
	abAppend(&ab, bug, strlen(bug));

	char buh[30];
	snprintf(buh, sizeof(buh), "Unknown spaces:    %d\r\n", unknown_cells);
	abAppend(&ab, buh, strlen(buh));
	
	if (!alive) {
		char bui[52];
		snprintf(bui, sizeof(bui), "GAME OVER! Press [key < --- fix this] to restart"); //write move cursor command to buf, with values converted from 0indexed to 1indexed
		abAppend(&ab, bui, strlen(bui));
	}

	char buj[32];
	snprintf(buj, sizeof(buj), "\x1b[%d;%dH", T.cy + 1, T.cx + 1); //write move cursor command to buf, with values converted from 0indexed to 1indexed
	abAppend(&ab, buj, strlen(buj));

	abAppend(&ab, "\x1b[?25h", 6); //show cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void step(int y, int x);
void fstep(int y, int x);
void reveal_adj_to_zero(int y, int x);

//Return the number of adjacent flags, at a given x,y coordinate, in the gamefield
int adjacent_flags(int y, int x)
{
	int adj = 0;

	if (           y > 0   && gamefield[y-1][x  ] == FLAGGED) ++adj;	// ABOVE
	if (x < w-1 && y > 0   && gamefield[y-1][x+1] == FLAGGED) ++adj;	// ABOVE + RIGHT
	if (x < w-1            && gamefield[y  ][x+1] == FLAGGED) ++adj;	// RIGHT
	if (x < w-1 && y < h-1 && gamefield[y+1][x+1] == FLAGGED) ++adj;	// DOWN + RIGHT 
	if (           y < h-1 && gamefield[y+1][x  ] == FLAGGED) ++adj;	// DOWN
	if (x > 0   && y < h-1 && gamefield[y+1][x-1] == FLAGGED) ++adj;	// DOWN + LEFT
	if (x > 0              && gamefield[y  ][x-1] == FLAGGED) ++adj;	// LEFT
	if (x > 0   && y > 0   && gamefield[y-1][x-1] == FLAGGED) ++adj;	// LEFT + UP

	return adj;
}

void reveal_cell(int y, int x);	//BAD PLACE FOR FUNCTION DECLARATION

void flag_reveal(int y, int x)
{
	//I THINK ITS RECURSIVE, AND I GOTTA ADD ANOTHER ZCHECKER
	
	if (           y > 0  ) { // ABOVE
		if (gamefield[y-1][x] != FLAGGED)
			fstep(y-1, x);
	}
	if (x < w-1 && y > 0  ) { // ABOVE + RIGHT
		if (gamefield[y-1][x+1] != FLAGGED)
			fstep(y-1, x+1);
	}
	if (x < w-1          ) { // RIGHT
		if (gamefield[y][x+1] != FLAGGED)
			fstep(y  , x+1);
	}
	if (x < w-1 && y < h-1) { // DOWN + RIGHT 
		if (gamefield[y+1][x+1] != FLAGGED)
			fstep(y+1, x+1);
	}
	if (           y < h-1) { // DOWN
		if (gamefield[y+1][x] != FLAGGED)
			fstep(y+1, x);
	}
	if (x > 0   && y < h-1) { // DOWN + LEFT
		if (gamefield[y+1][x-1] != FLAGGED)
			fstep(y+1, x-1);
	}
	if (x > 0                ) { // LEFT
		if (gamefield[y][x-1] != FLAGGED)
			fstep(y  , x-1);
	}
	if (x > 0   && y > 0  ) { // LEFT + UP
		if (gamefield[y-1][x-1] != FLAGGED)
			fstep(y-1, x-1);
	}
}

int zchecker(int y, int x)
{
	if (!zcheck[y][x]) {
		zcheck[y][x] = 1;
		step(y, x);
	}
}

void reveal_adj_to_zero(int y, int x)
{
	if (gamefield[y][x] >= 1 && gamefield[y][x] <= 9) //adj_to_zero was called on a nonzero space...
		die("reveal_adj_to_zero: function called on a nonzero space");	//I think I should do the name with function pointers???

	if (gamefield[y][x] == EMPTY)
		return;

	gamefield[y][x] = EMPTY;	//set this zero spot as zero

	
	if (           y > 0  ) { // ABOVE
		zchecker(y-1, x);
		//printf("ABOVE");
	}
	if (x < w-1 && y > 0  ) { // ABOVE + RIGHT
		zchecker(y-1, x+1);
		//printf("ABOVE+RIGHT");
	}
	if (x < w-1          ) { // RIGHT
		zchecker(y  , x+1);
		//printf("RIGHT");
	}
	if (x < w-1 && y < h-1) { // DOWN + RIGHT 
		zchecker(y+1, x+1);
		//printf("DOWN+RIGHT");
	}
	if (           y < h-1) { // DOWN
		zchecker(y+1, x);
		//printf("DOWN");
	}
	if (x > 0   && y < h-1) { // DOWN + LEFT
		zchecker(y+1, x-1);
		//printf("DOWN+LEFT");
	}
	if (x > 0                ) { // LEFT
		zchecker(y  , x-1);
		//printf("LEFT");
	}
	if (x > 0   && y > 0  ) { // LEFT + UP
		zchecker(y-1, x-1);
		//printf("LEFT+UP");
	}
}

void reveal_cell(int y, int x)
{
	int i = adjacent_bombs(y, x);

	if (gamefield[y][x] != i) {
		--unknown_cells;
		gamefield[y][x] = i;	//REVEAL CELL
	}

	if (i < 1 || i > 8)
		die("reveal_cell: called on nonsensical cell");
}

////THIS FUNCTION SHOULD NOT GO HERE BUT I DON'T KNOW WHERE TO PUT IT 
void step(int y, int x)
{
	int i = adjacent_bombs(y, x);

	if (minefield[y][x]) {
		//die("gameover!");
		alive = FALSE;
	} else {
		if (i) {
			if (gamefield[y][x] == i && adjacent_flags(y, x) == i) { //cell is unveiled and adj flags == adj bombs
				flag_reveal(y, x);
			} else {
				reveal_cell(y, x);
			}
		} else {
			reveal_adj_to_zero(y, x);
		}
	}
}

void fstep(int y, int x)
{
	int i = adjacent_bombs(y, x);
	
	if (minefield[y][x]) {
		die("gameover!");
	} else {
		if (i) {
			reveal_cell(y, x);
		} else {
			reveal_adj_to_zero(y, x);
		}
	}
}

//step on adjacent cells which should be stepped on
void cascade(int y, int x)
{

}


//SHOULD THIS RESET THE FCHECK POSITION OF THE FLAG?
void toggle_flag()	//set this zero spot as zero
{
	if (gamefield[G.cy][G.cx] == FLAGGED) {
		gamefield[G.cy][G.cx] = OFF;
		--flagged_spaces;
		++unknown_cells;
	} else if (gamefield[G.cy][G.cx] == OFF || gamefield[G.cy][G.cx] == MARKED) {
		gamefield[G.cy][G.cx] = FLAGGED;
		++flagged_spaces;
		--unknown_cells;
	}

//	int i;
//	
//	
//
//	if (gamefield[G.cy][G.cx] == FLAGGED)
//	{
//		if ((i = adjacent_bombs(G.cy, G.cx)) && !minefield[G.cy][G.cx])
//		{
//			gamefield[G.cy][G.cx] = i;
//		}
//		else if (zcheck[G.cy][G.cx])
//			gamefield[G.cy][G.cx] = EMPTY;
//		else
//			gamefield[G.cy][G.cx] = 0;
//	}
//	else
//		gamefield[G.cy][G.cx] = FLAGGED;
}

void toggle_marked()
{
	if (gamefield[G.cy][G.cx] == MARKED) {
		gamefield[G.cy][G.cx] = OFF;
		--flagged_spaces;
		++unknown_cells;
	} else if (gamefield[G.cy][G.cx] == OFF || gamefield[G.cy][G.cx] == FLAGGED) {
		gamefield[G.cy][G.cx] = MARKED;
		++flagged_spaces;
		--unknown_cells;
	}
}

/*** INPUT ***/
void move_cursor(int key) {
	switch (key) {
		case ARROW_LEFT:
		case 'h':
			if (T.cx != 1) {
				--G.cx;
				--T.cx;
				--T.cx;
			}
			break;
		case ARROW_RIGHT:
		case 'l':
			if (T.cx != w*2 - 1) {
				++G.cx;
			 	++T.cx;
			 	++T.cx;
			}
			break;
		case ARROW_UP:
		case 'k':
			if (T.cy != 0) {
				--G.cy;
				--T.cy;
			}
			break;
		case ARROW_DOWN:
		case 'j':
			if (T.cy != h - 1) {
				++G.cy;
				++T.cy;
			}
			break;
	}
}

void gen_rep()
{
	printf("gen = %d\r\n", generation);
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


		case CTRL_KEY('r'):
			reset();
			break;

		case ' ':
			step(G.cy, G.cx);
			break;

		case 'f':
		case 'F':
			toggle_flag();
			break;

		case 'd':
			toggle_marked();
			break;

		case '+':
			++new_h;
			break;

		case '-':
			if (new_h > 1)
				--new_h;
			break;

		case '*':
			++new_w;
			break;

		case '/':
			if (new_w > 1)
				--new_w;
			break;

		case '0':
			center_cursor();
			break;

		default:
			//printf("KEY UNKNOWN!\r\n");
			break;
	}
}
