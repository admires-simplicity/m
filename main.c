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

/*** DEFINES ***/
#define KILO_VERSION "0.0.1"

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

//int mineminefield[][30] =
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
//int minefield_w = sizeof *mineminefield / sizeof (int);
//int minefield_h = sizeof mineminefield / (minefield_w * sizeof(int)); // / (sizeof minefield_w * sizeof (int));
//
//sizeof mineminefield = sizeof minefield_w * sizeof minefield_h * sizeof (int);

int h = 16;
int w = 30;
//int b = (int) ((float)h * (float)w) / 0.2125;
int b = 102;

int minefield[16][30];
int gamefield[16][30];

int generation = 1;	//0 = random, 1 = friendly

/*** INIT ***/
int main(void) {
	enable_raw_mode();
	init_editor();
//
//	make_minefield(29, 16);
//
	while (1) {			//forever
		refresh_screen();	//refresh the screen
		//printf("screen is %d x %d\r\n", T.screencols, T.screenrows);
		process_keypress();	//handle keypress (get keypress (wait for keypress))
	}
//	printf("minefield_w = %d\n", minefield_w);
//	printf("minefield_h = %d\n", minefield_h);




	return 0;
}

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

void generate_minefield();

void initialize_minefield()
{
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j) {
			minefield[i][j] = 0;
			gamefield[i][j] = minefield[i][j];
		}

	generate_minefield();
}

void generate_minefield()
{
	void random_generate_minefield();
	void friendly_generate_minefield();

	if (generation == 0)
		random_generate_minefield();
	else if (generation == 1)
		friendly_generate_minefield();
	else
		die("GENERATION COMMAND NOT UNDERSTOOD");

}

//void random_generate_minefield()
//{
//	int zzz;
//	int pos;
//	int bombs_placed;
//	zzz = 0;
//
//	for (bombs_placed = 0; bombs_placed < b; ++bombs_placed) {
//		pos = rand() % (w * h); //generate number in range of #cells
//
//		int i = 0;	//index for number of cells
//		for (int j = 0; j < h; ++j)
//			for (int k = 0; k < w; ++k) {
//				++zzz;
//				if (zzz > 29000) die("big zzz");	///TEST, SEEMS TO NEVER GO OVER 29000
//				if (i++ == pos) {
//					minefield[j][k] = 1; //bomb here
//					k = w;
//					j = h;
//					}
//				}
//	}
//
//	if (bombs_placed != b)
//		die("NOT ENOUGH BOMBS!");
//	//printf("generation == %d\n", generation);
//	printf("%d\r\n", zzz++);
//}


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

		//check against array (could replace with binary tree for efficiency)
		for (int i = 0; i < bombs_placed; ++i)
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

int coord_to_n(int y, int x)	//SHOULD THIS BE A MACRO?
{
	//I think this should be 0 indexed, so I'll build it that way, and if it needs to be 1 indexed i can change it later
	return (w * y) + x;
}

void friendly_generate_minefield()
{
	//int zzz = 0;

	int pos;		//for current position in loop
	int bombs_placed = 0;
	int repeat = FALSE;
	int position[b+9];

	//initialize empty position array
	for (int i = 0; i < b; ++i)
		position[i] = 0;

	//ADJACENT MASK
	int x = 0;
	minefield[T.cy][T.cx] = 1;
	position[x] = coord_to_n(T.cy, T.cx);
	++x;
	if (              T.cy > 0  ) {
		minefield[T.cy-1][T.cx  ] = 1;	// ABOVE
		position[x] = coord_to_n(T.cy-1, T.cx);
		++x;
	}
	if (T.cx < w-1 && T.cy > 0  ) {
		minefield[T.cy-1][T.cx+1] = 1;	// ABOVE + RIGHT
		position[x] = coord_to_n(T.cy-1, T.cx+1);
		++x;
	}
	if (T.cx < w-1              ) {
		minefield[T.cy  ][T.cx+1] = 1;	// RIGHT
		position[x] = coord_to_n(T.cy, T.cx+1);
		++x;
	}
	if (T.cx < w-1 && T.cy < h-1) {
		minefield[T.cy+1][T.cx+1] = 1;	// DOWN + RIGHT 
		position[x] = coord_to_n(T.cy+1, T.cx+1);
		++x;
	}
	if (              T.cy < h-1) {
		minefield[T.cy+1][T.cx  ] = 1;	// DOWN
		position[x] = coord_to_n(T.cy+1, T.cx);
		++x;
	}
	if (T.cx > 0   && T.cy < h-1) {
		minefield[T.cy+1][T.cx-1] = 1;	// DOWN + LEFT
		position[x] = coord_to_n(T.cy+1, T.cx-1);
		++x;
	}
	if (T.cx > 0                ) {
		minefield[T.cy  ][T.cx-1] = 1;	// LEFT
		position[x] = coord_to_n(T.cy, T.cx-1);
		++x;
	}
	if (T.cx > 0   && T.cy > 0  ) {
		minefield[T.cy-1][T.cx-1] = 1;	// LEFT + UP
		position[x] = coord_to_n(T.cy-1, T.cx-1);
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
				//printf("zzz = %d\r\n", zzz++);
				if (i == pos) {
					minefield[j][k] = 1; //bomb here
					++bombs_placed;
					k = w;
					j = h;
				/////	if (minefield[T.cy][T.cx] || adjacent_bombs(T.cy, T.cx)) { //if bomb is within radius of cursor
				/////		minefield[j][k] = 0;
				/////		--bombs_placed;
				/////		position[bombs_placed] = 0;	//this maybe should be in loop?
				/////	}
				} else 
					++i;
			}
	}

	//ADJACENT UNMASK
	minefield[T.cy][T.cx] = 0;
	if (              T.cy > 0  ) minefield[T.cy-1][T.cx  ] = 0;	// ABOVE
	if (T.cx < w-1 && T.cy > 0  ) minefield[T.cy-1][T.cx+1] = 0;	// ABOVE + RIGHT
	if (T.cx < w-1              ) minefield[T.cy  ][T.cx+1] = 0;	// RIGHT
	if (T.cx < w-1 && T.cy < h-1) minefield[T.cy+1][T.cx+1] = 0;	// DOWN + RIGHT 
	if (              T.cy < h-1) minefield[T.cy+1][T.cx  ] = 0;	// DOWN
	if (T.cx > 0   && T.cy < h-1) minefield[T.cy+1][T.cx-1] = 0;	// DOWN + LEFT
	if (T.cx > 0                ) minefield[T.cy  ][T.cx-1] = 0;	// LEFT
	if (T.cx > 0   && T.cy > 0  ) minefield[T.cy-1][T.cx-1] = 0;	// LEFT + UP

	//after req'd num bombs finished, not enough bombs -> error
	if (bombs_placed != b)
		die("NOT ENOUGH BOMBS!");



//	int zzz;
//	int pos;
//	zzz = 0;
//
//	int repeat = 0;
//	int repeats = 0;
//
//	int bombs_placed = 0;
//	int position[b];
//	for (int i = 0; i < b; ++i)
//		position[i] = 0;
//	while (bombs_placed < b) {
//		pos = rand() % (w * h);
//
//		//if (pos == 0)
//		//	continue;
//		for (int i = 0; i < bombs_placed; ++i)
//			if (position[i] == pos) {
//				repeat = 1;
//				++repeats;
//				break;
//			}
//
//		if (repeat) {
//			repeat = 0;
//			continue;
//		}
//		
//		int i = 0;
//		for (int j = 0; j < h; ++j)
//			for (int k = 0; k < w; ++k) {
//				++zzz;
//				if (zzz > 1000000) {
//					for (int w = 0; w < bombs_placed+1; ++w)
//						printf("position[%d] == %d", w, position[w]);
//					printf("pos == %d", pos);
//					printf("i==%d", i);
//					printf(",bp==%d", bombs_placed);
//					printf(",repeats==%d", repeats);
//					die("probably error");
//				}
//				printf("%d\r\n", zzz);
//				if (i == pos) {
//					if (!(minefield[T.cy][T.cx] || adjacent_bombs(T.cy, T.cx))) { //if bomb is within radius of cursor
//						minefield[j][k] = 1; //bomb here
//						position[bombs_placed] = pos;
//						++bombs_placed;
//					}
//					k = w; //continue
//					j = h;
//				} else
//					++i;
//			}
//
//
//		//pos[44] == 0pos==187
//		//pos[16] == 0pos[17] == 0pos==33
//		//
//	}
//
//
//	printf("%d\r\n", zzz);
//
//	//the largest value of zzz should be 48960, should it not?
//	//and i'm calculating repeat values, so, zzz should still not be over 48960
}

//attempt at friendly_generate that doesn't work because random_gen() doesn't count the positions beforehand because it shouldn't...
//void friendly_generate_minefield()
//{
//	//mask adjacent bombs
//	minefield[T.cy][T.cx] = 1;
//	if (              T.cy > 0  ) minefield[T.cy-1][T.cx  ] = 1;	// ABOVE
//	if (T.cx < w-1 && T.cy > 0  ) minefield[T.cy-1][T.cx+1] = 1;	// ABOVE + RIGHT
//	if (T.cx < w-1              ) minefield[T.cy  ][T.cx+1] = 1;	// RIGHT
//	if (T.cx < w-1 && T.cy < h-1) minefield[T.cy+1][T.cx+1] = 1;	// DOWN + RIGHT 
//	if (              T.cy < h-1) minefield[T.cy+1][T.cx  ] = 1;	// DOWN
//	if (T.cx > 0   && T.cy < h-1) minefield[T.cy+1][T.cx-1] = 1;	// DOWN + LEFT
//	if (T.cx > 0                ) minefield[T.cy  ][T.cx-1] = 1;	// LEFT
//	if (T.cx > 0   && T.cy > 0  ) minefield[T.cy-1][T.cx-1] = 1;	// LEFT + UP
//	//randomgen
//	random_generate_minefield();
//	//unmask adjacent bombs
//	if (              T.cy > 0  ) minefield[T.cy-1][T.cx  ] = 0;	// ABOVE
//	if (T.cx < w-1 && T.cy > 0  ) minefield[T.cy-1][T.cx+1] = 0;	// ABOVE + RIGHT
//	if (T.cx < w-1              ) minefield[T.cy  ][T.cx+1] = 0;	// RIGHT
//	if (T.cx < w-1 && T.cy < h-1) minefield[T.cy+1][T.cx+1] = 0;	// DOWN + RIGHT 
//	if (              T.cy < h-1) minefield[T.cy+1][T.cx  ] = 0;	// DOWN
//	if (T.cx > 0   && T.cy < h-1) minefield[T.cy+1][T.cx-1] = 0;	// DOWN + LEFT
//	if (T.cx > 0                ) minefield[T.cy  ][T.cx-1] = 0;	// LEFT
//	if (T.cx > 0   && T.cy > 0  ) minefield[T.cy-1][T.cx-1] = 0;	// LEFT + UP
//	minefield[T.cy][T.cx] = 0;
//}


//////THIS DOESN'T WORK YET?
////void friendly_generate_minefield()
////{
////
////	int zzz;
////	int pos;
////	zzz = 0;
////
////	////for (int bombs_placed = 0; bombs_placed < b; ++bombs_placed) {
////	////	pos = rand() % (w * h);
////
////	////	int i = 0;
////	////	for (int j = 0; j < h; ++j)
////	////		for (int k = 0; k < w; ++k) {
////	////			++zzz;
////	////			if (i == pos) {
////	////				minefield[j][k] = 1; //bomb here
////	////				if (minefield[T.cy][T.cx] || adjacent_bombs(T.cy, T.cx)) { //if bomb is within radius of cursor
////	////					printf("BOMB IN GENSPACE\r\n");
////	////					minefield[j][k] == 0; 	//delete the bomb here
////	////					//--bombs_placed;		//counteract bomb increment
////	////				}
////	////				k = w; //continue
////	////				j = h;
////	////			} else
////	////				++i;
////	////		}
////
////
////	////}
////	
////	int repeat = 0;
////	int repeats = 0;
////
////	int bombs_placed = 0;
////	int position[b];
////	for (int i = 0; i < b; ++i)
////		position[i] = 0;
////	while (bombs_placed < b) {
////		pos = rand() % (w * h);
////
////		//if (pos == 0)
////		//	continue;
////		for (int i = 0; i < bombs_placed; ++i)
////			if (position[i] == pos) {
////				repeat = 1;
////				++repeats;
////				break;
////			}
////
////		if (repeat) {
////			repeat = 0;
////			continue;
////		}
////		
////		int i = 0;
////		for (int j = 0; j < h; ++j)
////			for (int k = 0; k < w; ++k) {
////				++zzz;
////				if (zzz > 1000000) {
////					for (int w = 0; w < bombs_placed+1; ++w)
////						printf("position[%d] == %d", w, position[w]);
////					printf("pos == %d", pos);
////					printf("i==%d", i);
////					printf(",bp==%d", bombs_placed);
////					printf(",repeats==%d", repeats);
////					die("probably error");
////				}
////				printf("%d\r\n", zzz);
////				if (i == pos) {
////					if (!(minefield[T.cy][T.cx] || adjacent_bombs(T.cy, T.cx))) { //if bomb is within radius of cursor
////						minefield[j][k] = 1; //bomb here
////						position[bombs_placed] = pos;
////						++bombs_placed;
////					}
////					k = w; //continue
////					j = h;
////				} else
////					++i;
////			}
////
////
////		//pos[44] == 0pos==187
////		//pos[16] == 0pos[17] == 0pos==33
////		//
////	}
////
////
////	printf("%d\r\n", zzz);
////
////	//the largest value of zzz should be 48960, should it not?
////	//and i'm calculating repeat values, so, zzz should still not be over 48960
////}

void initialize_gamefield()
{
	int i, j;

	for (i = 0; i < h; ++i)
		for (j = 0; j < w; ++j)
			gamefield[i][j] = OFF;
}


//initialize minefields in T struct
void init_editor() {
	T.cx = 0;
	T.cy = 0;

	srand(time(NULL));

	initialize_minefield();
	initialize_gamefield();

	if (get_win_size(&T.screenrows, &T.screencols) == -1) die("get_win_size");
}

/*** TERMINAL ***/
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
	if (tcgetattr(STDIN_FILENO, &T.orig_termios) == -1) die("tcgetattr");	//get termios struct
	atexit(disable_raw_mode);

	struct termios raw = T.orig_termios;
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

//char 

void draw_minefield(struct abuf *ab)
{
//	char buf[32];
//	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", T.cy + 1, T.cx + 1); //write move command to H buf, with values converted from 0indexed to 1indexed
//	abAppend(ab, buf, strlen(buf));

	char *c;
	char buf[2]; //size for one character, and the null byte? lel
	int z;

	for (int i = 0; i < h; ++i) {
		//printf("%2d: ", i);
		for (int j = 0; j < w; ++j) {
			////snprintf(c, 1, "%d", minefield[i][j]);
			//printf("%d", minefield[i][j]);
			if (minefield[i][j] == 1)
				abAppend(ab, "*", 1);
			else
				if (z = adjacent_bombs(i, j)) {	//not 0 adj boms
					snprintf(buf, sizeof(buf), "%d", z);
					abAppend(ab, buf, strlen(buf));
				} else {
					abAppend(ab, ".", 1);
				}
		}
		abAppend(ab, "\r\n", 2);
	}
}

void draw_gamefield(struct abuf *ab)
{
	char *c;
	char buf[2]; //size for one char and null byte			//AT SOME POINT THIS SHOULD BE REPLACED WITH A FUNCTION TO WRITE CHARS TO SCREEN
	int z;

	for (int i = 0; i < h; ++i) {
		for (int j = 0; j < w; ++j) {
			switch (gamefield[i][j]) {
				case OFF:
					abAppend(ab, "#", 1);
					break;
				case ACTIVATED:
					abAppend(ab, "*", 1);
					break;
				case EMPTY:
					abAppend(ab, ".", 1);
					break;
				case FLAGGED:
					abAppend(ab, "!", 1);
					break;
				case MARKED:
					abAppend(ab, "?", 1);
					break;
				case 0:
					abAppend(ab, "#", 1);
					break;
				default:
					buf[0] = gamefield[i][j] + '0';
					buf[1] = '\0';
					abAppend(ab, buf, 1);
					break;
			}
		}
		abAppend(ab, "\r\n", 2);
	}


}

int count_bombs()
{
	int count = 0;
	for (int i = 0; i < h; ++i)
	       for (int j = 0; j < w; ++j)
		       if (minefield[i][j] == 1)
			       ++count;
	return count;
}

void refresh_screen() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6); //Hide cursor
	abAppend(&ab, "\x1b[2J", 4); //erase (all) in display //escape sequence starts with ESC (decimal 27), [ ...
	abAppend(&ab, "\x1b[H", 3);

	//draw_rows(&ab);
	//draw_minefield(&ab);
	draw_gamefield(&ab);
	char bug[20];
	snprintf(bug, sizeof(bug), "nbombs == %d\r\n", count_bombs());
	abAppend(&ab, bug, strlen(bug));
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", T.cy + 1, T.cx + 1); //write move command to H buf, with values converted from 0indexed to 1indexed
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6); //show cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

////THIS FUNCTION SHOULD NOT GO HERE BUT I DON'T KNOW WHERE TO PUT IT 
void step()
{
	int i = 0;

	if (minefield[T.cy][T.cx])
		//gameover();
		die("gameover!");
	else if (i = adjacent_bombs(T.cy, T.cx))
		gamefield[T.cy][T.cx] = i;
	else
		gamefield[T.cy][T.cx] = EMPTY;
}

void flag_coord()
{
	gamefield[T.cy][T.cx] = FLAGGED;
}

void mark_coord()
{
	gamefield[T.cy][T.cx] = MARKED;
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
			if (T.cx != w - 1) {
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
			if (T.cy != h - 1) {
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

			
			write(STDOUT_FILENO, "\x1b[17;1H", 7);	//position cursor 1,1
			atexit(gen_rep);
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
			initialize_minefield();
			break;

		case ' ':
			step();
			break;

		case 'f':
			flag_coord();
			break;

		case 'd':
			mark_coord();
			break;

		default:
			//printf("KEY UNKNOWN!\r\n");
			break;
	}
}
