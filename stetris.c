#include "stetris.h"


// This function is called on the start of your application
// Here you can initialize what ever you need for your task
// return false if something fails, else true
bool initializeSenseHat() {
  uint16_t *mapmem;
  struct fb_fix_screeninfo info;
  char name[256];

  char strinput[] = "dev/input/eventX";
  char strfilepath[] = "/dev/fbX";

  unsigned int i = 0;
  unsigned int j = 0;
  bool cond = true;
  unsigned int leninput = strlen(strinput);
  unsigned int lenfilepath = strlen(strfilepath);

  while(cond){
    strinput[leninput - 1] = i + '0';
    strfilepath[lenfilepath - 1] = i + '0';

    fb.fbufferMatrixLed = open(strinput, O_RDWR);
    fb.fbufferJoy = open(strfilepath, O_RDONLY|O_NONBLOCK);
  
    //check to open the FILEPATH If returns -1 wrong filepath
    if (fb.fbufferMatrixLed == -1) {
      fprintf(stderr, "Error path of led matrix");
      j = j + 1;
    }

    if(fb.fbufferJoy == -1){
      fprintf(stderr, "Error path of joystick");
      close(fb.fbufferMatrixLed);
      j = j + 1;
    }
    //check the ioctl
    if (ioctl(fb.fbufferMatrixLed, FBIOGET_FSCREENINFO, &info) == -1) {
      fprintf(stderr, "Error (call to 'ioctl' for led MATRIX)");
      close(fb.fbufferMatrixLed);
      close(fb.fbufferJoy);
      j = j + 1;
    }

    if(ioctl(fb.fbufferJoy, EVIOCGNAME(sizeof(name)), name) == -1){
      fprintf(stderr, "Error (call to 'ioctl' for Joystick)");
      close(fb.fbufferMatrixLed);
      close(fb.fbufferJoy);
      j = j + 1;
    }
    
    if(strcmp(name, "Raspberry Pi Sense HAT Joystick") != 0){//check the name of the device
      fprintf(stderr, "Error Joystick not found");
      close(fb.fbufferMatrixLed);
      close(fb.fbufferJoy);
      j = j + 1;
    }
    
    if (strcmp(info.id, "RPi-Sense FB") != 0) { //check the name of the device
        fprintf(stderr, "Error: RPi-Sense FB not found");
        close(fb.fbufferMatrixLed);
        close(fb.fbufferJoy);
        j = j + 1;
    }

    if(j > 0){
      j = 0;
    }else{
      cond = false;
    }
    i = i + 1;
    if(i > 10){
      return false;
    }
  }

  mapmem = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fb.fbufferMatrixLed, 0); //maps the array matrix to map
  if (mapmem == MAP_FAILED) {
    close(fb.fbufferMatrixLed);
    close(fb.fbufferJoy);
    fprintf(stderr, "Error mmapping the file");
    return false;
  }
  memset(mapmem, 0, FILESIZE); //resets the led matrix to all leds off
  ledp = mapmem; //pointer to map used later
  resetp = mapmem; //pointer to the default map
  fprintf(stderr, "Success");

  return true;
}

// This function is called when the application exits
// Here you can free up everything that you might have opened/allocated
void freeSenseHat() {
  memset(ledp, 0, FILESIZE); //clear the led array
  close(fb.fbufferMatrixLed); //closes the matrix file descriptor
  close(fb.fbufferJoy); //closes the joystick file descriptor 
}

// This function should return the key that corresponds to the joystick press
// KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, with the respective direction
// and KEY_ENTER, when the the joystick is pressed
// !!! when nothing was pressed you MUST return 0 !!!
int readSenseHatJoystick() {
  read(fb.fbufferJoy, &e, sizeof(struct input_event)); //read input event
  if(e.type == EV_KEY){ //if input event is equal to EV_KEY send it the the event code or else return 0
    if(e.value == 1){ //only check if the key is pressed once
      return e.code;
    }
  }
  
  return 0;
}

int pickcolor(){
  srand(time(0)); //chooses random color based on the time now
  return color_array[rand() % 5]; //picks one of the color defined in the collor_array function
}


// This function should render the gamefield on the LED matrix. It is called
// every game tick. The parameter playfieldChanged signals whether the game logic
// has changed the playfield
void renderSenseHatMatrix(bool const playfieldChanged) {
  
  if (!playfieldChanged)
    return;
  
  ledp = resetp; //use the default tile to reset the ledp pointer
  for(int i = 0; i < game.grid.y; i++){ //goes through the game grid
    for(int j = 0; j < game.grid.x; j++){
      if(game.playfield[j][i].occupied == true){ //if occupied then it means it shoud be displayed on led matrix
        
        if(game.playfield[j][i].color == 0){
          game.playfield[j][i].color = pickcolor(); //chooses random color
          *(ledp + i + j * 8) = game.playfield[j][i].color; //assigns random color to tile
        }else{
          *(ledp + i + j * 8) = game.playfield[j][i].color; //if it allready has a color assign that color
        }


      }else{
        *(ledp + i + j * 8) = 0; //update the matrix board
      }
    }
  }
  
}


// The game logic uses only the following functions to interact with the playfield.
// if you choose to change the playfield or the tile structure, you might need to
// adjust this game logic <> playfield interface

static inline void newTile(coord const target) {
  game.playfield[target.y][target.x].occupied = true;
}

static inline void copyTile(coord const to, coord const from) {
  memcpy((void *) &game.playfield[to.y][to.x], (void *) &game.playfield[from.y][from.x], sizeof(tile));
}

static inline void copyRow(unsigned int const to, unsigned int const from) {
  memcpy((void *) &game.playfield[to][0], (void *) &game.playfield[from][0], sizeof(tile) * game.grid.x);

}

static inline void resetTile(coord const target) {
  memset((void *) &game.playfield[target.y][target.x], 0, sizeof(tile));
}

static inline void resetRow(unsigned int const target) {
  memset((void *) &game.playfield[target][0], 0, sizeof(tile) * game.grid.x);
}

static inline bool tileOccupied(coord const target) {
  return game.playfield[target.y][target.x].occupied;
}

static inline bool rowOccupied(unsigned int const target) {
  for (unsigned int x = 0; x < game.grid.x; x++) {
    coord const checkTile = {x, target};
    if (!tileOccupied(checkTile)) {
      return false;
    }
  }
  return true;
}


static inline void resetPlayfield() {
  for (unsigned int y = 0; y < game.grid.y; y++) {
    resetRow(y);
  }
}

// Below here comes the game logic. Keep in mind: You are not allowed to change how the game works!
// that means no changes are necessary below this line! And if you choose to change something
// keep it compatible with what was provided to you!

bool addNewTile() {
  game.activeTile.y = 0;
  game.activeTile.x = (game.grid.x - 1) / 2;
  if (tileOccupied(game.activeTile))
    return false;
  newTile(game.activeTile);
  return true;
}

bool moveRight() {
  coord const newTile = {game.activeTile.x + 1, game.activeTile.y};
  if (game.activeTile.x < (game.grid.x - 1) && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    return true;
  }
  return false;
}

bool moveLeft() {
  coord const newTile = {game.activeTile.x - 1, game.activeTile.y};
  if (game.activeTile.x > 0 && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    return true;
  }
  return false;
}


bool moveDown() {
  coord const newTile = {game.activeTile.x, game.activeTile.y + 1};
  if (game.activeTile.y < (game.grid.y - 1) && !tileOccupied(newTile)) {
    copyTile(newTile, game.activeTile);
    resetTile(game.activeTile);
    game.activeTile = newTile;
    return true;
  }
  return false;
}


bool clearRow() {
  if (rowOccupied(game.grid.y - 1)) {
    for (unsigned int y = game.grid.y - 1; y > 0; y--) {
      copyRow(y, y - 1);
    }
    resetRow(0);
    return true;
  }
  return false;
}

void advanceLevel() {
  game.level++;
  switch(game.nextGameTick) {
  case 1:
    break;
  case 2 ... 10:
    game.nextGameTick--;
    break;
  case 11 ... 20:
    game.nextGameTick -= 2;
    break;
  default:
    game.nextGameTick -= 10;
  }
}

void newGame() {
  game.state = ACTIVE;
  game.tiles = 0;
  game.rows = 0;
  game.score = 0;
  game.tick = 0;
  game.level = 0;
  resetPlayfield();
}

void gameOver() {
  game.state = GAMEOVER;
  game.nextGameTick = game.initNextGameTick;
}


bool sTetris(int const key) {
  bool playfieldChanged = false;

  if (game.state & ACTIVE) {
    // Move the current tile
    if (key) {
      playfieldChanged = true;
      switch(key) {
      case KEY_LEFT:
        moveLeft();
        break;
      case KEY_RIGHT:
        moveRight();
        break;
      case KEY_DOWN:
        while (moveDown()) {};
        game.tick = 0;
        break;
      default:
        playfieldChanged = false;
      }
    }

    // If we have reached a tick to update the game
    if (game.tick == 0) {
      // We communicate the row clear and tile add over the game state
      // clear these bits if they were set before
      game.state &= ~(ROW_CLEAR | TILE_ADDED);

      playfieldChanged = true;
      // Clear row if possible
      if (clearRow()) {
        game.state |= ROW_CLEAR;
        game.rows++;
        game.score += game.level + 1;
        if ((game.rows % game.rowsPerLevel) == 0) {
          advanceLevel();
        }
      }

      // if there is no current tile or we cannot move it down,
      // add a new one. If not possible, game over.
      if (!tileOccupied(game.activeTile) || !moveDown()) {
        if (addNewTile()) {
          game.state |= TILE_ADDED;
          game.tiles++;
        } else {
          gameOver();
        }
      }
    }
  }

  // Press any key to start a new game
  if ((game.state == GAMEOVER) && key) {
    playfieldChanged = true;
    newGame();
    addNewTile();
    game.state |= TILE_ADDED;
    game.tiles++;
  }

  return playfieldChanged;
}

int readKeyboard() {
  struct pollfd pollStdin = {
       .fd = STDIN_FILENO,
       .events = POLLIN
  };
  int lkey = 0;

  if (poll(&pollStdin, 1, 0)) {
    lkey = fgetc(stdin);
    if (lkey != 27)
      goto exit;
    lkey = fgetc(stdin);
    if (lkey != 91)
      goto exit;
    lkey = fgetc(stdin);
  }
 exit:
    switch (lkey) {
      case 10: return KEY_ENTER;
      case 65: return KEY_UP;
      case 66: return KEY_DOWN;
      case 67: return KEY_RIGHT;
      case 68: return KEY_LEFT;
    }
  return 0;
}

void renderConsole(bool const playfieldChanged) {
  if (!playfieldChanged)
    return;

  // Goto beginning of console
  fprintf(stdout, "\033[%d;%dH", 0, 0);
  for (unsigned int x = 0; x < game.grid.x + 2; x ++) {
    fprintf(stdout, "-");
  }
  fprintf(stdout, "\n");
  for (unsigned int y = 0; y < game.grid.y; y++) {
    fprintf(stdout, "|");
    for (unsigned int x = 0; x < game.grid.x; x++) {
      coord const checkTile = {x, y};
      fprintf(stdout, "%c", (tileOccupied(checkTile)) ? '#' : ' ');
    }
    switch (y) {
      case 0:
        fprintf(stdout, "| Tiles: %10u\n", game.tiles);
        break;
      case 1:
        fprintf(stdout, "| Rows:  %10u\n", game.rows);
        break;
      case 2:
        fprintf(stdout, "| Score: %10u\n", game.score);
        break;
      case 4:
        fprintf(stdout, "| Level: %10u\n", game.level);
        break;
      case 7:
        fprintf(stdout, "| %17s\n", (game.state == GAMEOVER) ? "Game Over" : "");
        break;
    default:
        fprintf(stdout, "|\n");
    }
  }
  for (unsigned int x = 0; x < game.grid.x + 2; x++) {
    fprintf(stdout, "-");
  }
  fflush(stdout);
}


inline unsigned long uSecFromTimespec(struct timespec const ts) {
  return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000));
}

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  // This sets the stdin in a special state where each
  // keyboard press is directly flushed to the stdin and additionally
  // not outputted to the stdout
  {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~(ICANON | ECHO);
    ttystate.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
  }

  // Allocate the playing field structure
  game.rawPlayfield = (tile *) malloc(game.grid.x * game.grid.y * sizeof(tile));
  game.playfield = (tile**) malloc(game.grid.y * sizeof(tile *));
  if (!game.playfield || !game.rawPlayfield) {
    fprintf(stderr, "ERROR: could not allocate playfield\n");
    return 1;
  }
  for (unsigned int y = 0; y < game.grid.y; y++) {
    game.playfield[y] = &(game.rawPlayfield[y * game.grid.x]);
  }

  // Reset playfield to make it empty
  resetPlayfield();
  // Start with gameOver
  gameOver();

  if (!initializeSenseHat()) {
    fprintf(stderr, "ERROR: could not initilize sense hat\n");
    return 1;
  };

  // Clear console, render first time
  fprintf(stdout, "\033[H\033[J");
  renderConsole(true);
  renderSenseHatMatrix(true);

  for(int i = 0; i < game.grid.y; i++){
    for(int j = 0; j < game.grid.x; j++){
      game.playfield[j][i].color = 0;
    }
  }

  while (true) {
    struct timeval sTv, eTv;
    gettimeofday(&sTv, NULL);
    int key = readSenseHatJoystick();
    if (!key)
      key = readKeyboard();
    if (key == KEY_ENTER)
      break;

    bool playfieldChanged = sTetris(key);
    renderConsole(playfieldChanged);
    renderSenseHatMatrix(playfieldChanged);

    // Wait for next tick
    gettimeofday(&eTv, NULL);
    unsigned long const uSecProcessTime = ((eTv.tv_sec * 1000000) + eTv.tv_usec) - ((sTv.tv_sec * 1000000 + sTv.tv_usec));
    if (uSecProcessTime < game.uSecTickTime) {
      usleep(game.uSecTickTime - uSecProcessTime);
    }
    game.tick = (game.tick + 1) % game.nextGameTick;
  }

  freeSenseHat();
  free(game.playfield);
  free(game.rawPlayfield);

  return 0;
}