#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <linux/input.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <poll.h>

#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

// The game state can be used to detect what happens on the playfield
#define GAMEOVER   0
#define ACTIVE     (1 << 0)
#define ROW_CLEAR  (1 << 1)
#define TILE_ADDED (1 << 2)

//new definition
#define NUM_WORDS 64
#define FILESIZE (NUM_WORDS * sizeof(uint16_t))
#define RGB_RED 0xF800
#define RGB_YELLOW 0xA400
#define RGB_GREEN 0x4E00
#define RGB_WHITE 0xDE10
#define RGB_PURPLE 0xE110

// If you extend this structure, either avoid pointers or adjust
// the game logic allocate/deallocate and reset the memory
typedef struct {
  bool occupied;
  int color;
} tile;

typedef struct {
  unsigned int x;
  unsigned int y;
} coord;

typedef struct {
  coord const grid;                     // playfield bounds
  unsigned long const uSecTickTime;     // tick rate
  unsigned long const rowsPerLevel;     // speed up after clearing rows
  unsigned long const initNextGameTick; // initial value of nextGameTick

  unsigned int tiles; // number of tiles played
  unsigned int rows;  // number of rows cleared
  unsigned int score; // game score
  unsigned int level; // game level

  tile *rawPlayfield; // pointer to raw memory of the playfield
  tile **playfield;   // This is the play field array
  unsigned int state;
  coord activeTile;                       // current tile

  unsigned long tick;         // incremeted at tickrate, wraps at nextGameTick
                              // when reached 0, next game state calculated
  unsigned long nextGameTick; // sets when tick is wrapping back to zero
                              // lowers with increasing level, never reaches 0
} gameConfig;

//new global variables //fix
typedef struct {
  int fbufferMatrixLed;
  int fbufferJoy;
} framebuffers;

framebuffers fb = {
  .fbufferMatrixLed = -1,
  .fbufferJoy = -1,
};
struct input_event e;
uint16_t *ledp;
uint16_t *resetp;

int color_array[] = {RGB_RED, RGB_YELLOW, RGB_GREEN, RGB_WHITE, RGB_PURPLE}; //color array

gameConfig game = {
                   .grid = {8, 8},
                   .uSecTickTime = 10000,
                   .rowsPerLevel = 2,
                   .initNextGameTick = 50,
};



bool initializeSenseHat();
void freeSenseHat();
int readSenseHatJoystick();

int pickcolor();
void renderSenseHatMatrix(bool const playfieldChanged);
bool addNewTile();
bool moveRight();
bool moveLeft();
bool moveDown();
bool clearRow();


void advanceLevel();
void newGame();
void gameOver();
bool sTetris(int const key);

int readKeyboard();
void renderConsole(bool const playfieldChanged);






