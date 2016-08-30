// demo driver example for libPCA9685
// copyright 2016 Scott Edlin

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <ncurses.h>

#include <PCA9685.h>

//#define DEBUG
//#define VALIDATE
#define NCMODE


int fd;
int addr = 0x40;


void cleanup() {
  // attmempt to turn off all PWM
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);
  #ifdef NCMODE
  // attmempt to end the ncurses window session
  endwin();
  #endif
} // cleanup



void intHandler(int dummy) {
  cleanup();
  fprintf(stdout, "Caught signal, exiting\n");
  exit(0);
} // intHandler 


int initHardware(int adpt, int addr, int freq) {
  int fd;
  int ret;

  // setup the I2C bus device 
  fd = PCA9685_openI2C(adpt, addr);
  if (fd < 0) {
    fprintf(stderr, "initHardware(): PCA9685_openI2C() returned ");
    fprintf(stderr, "%d for adpt %d at addr %x\n", fd, adpt, addr);
    return -1;
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = PCA9685_dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_dumpAllRegs() returned %d\n", ret);
    return -1;
  } // if 
  #endif

  // initialize the pca device 
  ret = PCA9685_initPWM(fd, addr, freq);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_initPWM() returned %d\n", ret);
    return -1;
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = PCA9685_dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_dumpAllRegs() returned %d\n", ret);
    return -1;
  } // if 
  #endif

  return fd;
} // initHardware


int dumpStats(int stats[]) {
  int i;
  for(i=0; i<8; i++) {
    #ifdef NCMODE
    mvprintw(1, i*10, "%-10d", stats[i]);
    #else
    printf("%-10d", stats[i]);
    if (i==7) { printf("\n"); }
    #endif
  } // for

  return 0;
} // dumpStats


int dumpVals(unsigned int row, unsigned int* vals, unsigned char chan) {
  unsigned char i;
  for(i=0; i<_PCA9685_CHANS; i++) {

    #ifdef NCMODE
    unsigned int colorVal;
    colorVal = (unsigned int)(vals[i] / 682.667f) + 1;

    if (i == chan) {
      attron(A_BOLD | COLOR_PAIR(colorVal));
    } // if
    else {
      attron(A_NORMAL | COLOR_PAIR(colorVal));
    } // else

    mvprintw(row, 5*i, "%03x", vals[i]);
    attroff(COLOR_PAIR(colorVal));
    attroff(A_BOLD);

    #else
    printf("%03x  ", vals[i]);
    if (i==_PCA9685_CHANS) { printf("\n"); };

    #endif
  } // for

  #ifdef NCMODE
  refresh();
  #endif

  return 0;
} // dumpVals



int initScreen() {

  #ifdef NCMODE
  int ret;
  WINDOW* wndw;

  // start ncurses mode
  wndw = initscr();
  if (wndw != stdscr) {
    fprintf(stderr, "initScreen(): initscr() did not return stdscr\n");
    return -1;
  } // if

  // no line buffering but allow control chars like CTRL-C
  cbreak();

  // enable extended keys
  keypad(stdscr, TRUE);

  // no echo
  noecho();

  // initialize ncurses colors
  ret = start_color();
  if (ret == ERR) {
    fprintf(stderr, "initScreen(): start_color() returned ERR\n");
    return -1;
  } // if err

  // define color pairs in opposite ROYGBIV order
  init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(2, COLOR_BLUE, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_GREEN, COLOR_BLACK);
  init_pair(5, COLOR_YELLOW, COLOR_BLACK);
  init_pair(6, COLOR_RED, COLOR_BLACK);

  // header row for stats
  mvprintw(0,  0, "frames");
  mvprintw(0, 10, "bits");
  mvprintw(0, 20, "ms");
  mvprintw(0, 30, "avg");
  mvprintw(0, 40, "Hz");
  mvprintw(0, 50, "avg");
  mvprintw(0, 60, "kbps");
  mvprintw(0, 70, "avg");

  // dump off values to start
  dumpVals(3, (unsigned int[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 0);

  #ifdef VALIDATE
  dumpVals(4, (unsigned int[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, -1);
  #endif

  refresh();

  #else
  printf("frames    bits      ms        avg       Hz        avg       kbps      avg\n");

  #endif

  return 0;
} // initScreen









// main driver 
int main(void) {
  int adpt = 1;
  int freq = 200;
  int ret;
  char automatic;
  char manual;

  #ifdef NCMODE
  automatic = false;
  manual = true;
  #else
  automatic = true;
  manual = false;
  #endif

  // register the signal handler to catch interrupts
  signal(SIGINT, intHandler);

  // initialize the I2C bus adpt and a PCA9685 at addr with freq
  fd = initHardware(adpt, addr, freq);
  if (fd < 0) {
    cleanup();
    fprintf(stderr, "main(): initHardware() returned ");
    fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, adpt, addr);
    return -1;
  } // if

  // initialize the screen
  ret = initScreen();
  if (ret < 0) {
    cleanup();
    fprintf(stderr, "main(): initScreen() returned %d\n", ret);
    return -1;
  } // if

  { // perf context 
    int j = 0;
    int chan = 0;
    int steps[_PCA9685_CHANS];
    int minStep = 1;
    int maxStep = 100;
    unsigned int setOnVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int setOffVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    #ifdef VALIDATE
    unsigned int getOnVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int getOffVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    #endif

    int size = 512;
    int bits;
    int mema = 0;
    int fema = 0;
    int kema = 0;
    float alpha = 0.1;
    float invalpha = 1.0 - alpha;
    struct timeval then, now, diff;

    gettimeofday(&then, NULL);
    
    // initialize steps for automatic mode
    int i;
    for (i=0; i<_PCA9685_CHANS; i++) {
      steps[i] = rand() % maxStep + minStep;
    } // for 

    int c;
    // blink endlessly 
    while (1) {

      if (automatic) {
        // read a char (non-blocking)
        c = getch();
        // if m, switch to manual
        if (c != ERR) {
          if (c == (int)('m')) {
            automatic = false;
            manual = true;
          } // if
        } // if

        for (i=0; i<_PCA9685_CHANS; i++) {

          // don't go larger than MAX
          if ((steps[i] > 0) &&
             (setOffVals[i] >= _PCA9685_MAXVAL - steps[i])) {
            setOffVals[i] = _PCA9685_MAXVAL;
            steps[i] *= -1;
          } // if 

          // don't go smaller than MIN
          else if ((steps[i] < 0) &&
                  (setOffVals[i] <= _PCA9685_MINVAL - steps[i])) {
            setOffVals[i] = _PCA9685_MINVAL;
            steps[i] = rand() % (maxStep - minStep) + minStep;
          } // else if 

          // take the step
          else {
            setOffVals[i] += steps[i];
          } // else
        } // for 
      } // if auto

      else if (manual) {
        int c;
        int upStep = 16;

        // read a char (blocking)
        nodelay(stdscr, false);
        c = getch();

        // if left, chan--
        if (c == KEY_LEFT) {
          chan = (chan > 0) ? chan-1 : _PCA9685_CHANS;
        } // if

        // else if right, chan++
        else if (c == KEY_RIGHT) {
          chan = (chan < _PCA9685_CHANS) ? chan+1 : 0;
        } // if

        // else if up, boost vals[chan]
        else if (c == KEY_UP) {
          setOffVals[chan] = (setOffVals[chan]  > _PCA9685_MAXVAL - upStep)
                       ? _PCA9685_MAXVAL : setOffVals[chan] + upStep;
        } // if

        // else if down, drop vals[chan]
        else if (c == KEY_DOWN) {
          setOffVals[chan] = (setOffVals[chan] < _PCA9685_MINVAL + upStep)
                       ? _PCA9685_MINVAL : setOffVals[chan] - upStep;
        } // if

        // else if period, inc vals[chan]
        else if (c == (int)('.')) {
          setOffVals[chan] = (setOffVals[chan] > _PCA9685_MAXVAL - 1)
                       ? _PCA9685_MAXVAL : setOffVals[chan] + 1;
        } // if

        // else if down, drop vals[chan]
        else if (c == (int)(',')) {
          setOffVals[chan] = (setOffVals[chan] < _PCA9685_MINVAL + 1)
                       ? _PCA9685_MINVAL : setOffVals[chan] - 1;
        } // if

        // else if a, switch to automatic
        else if (c == (int)('a')) {
          manual = false;
          automatic = true;
          gettimeofday(&then, NULL);
          nodelay(stdscr, true);
        } // if

        // else if 0, set to minimum val
        else if (c == (int)('0')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            setOffVals[i] = _PCA9685_MINVAL;
          } // for
        } // if

        // else if 1, set to maximum val
        else if (c == (int)('1')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            setOffVals[i] = _PCA9685_MAXVAL;
          } // for
        } // if
        // TODO: else if s, toggle strobe
      } // if manual


      // SET THE VALS, EVERY TIME THROUGH THE LOOP
      ret = PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);
      if (ret != 0) {
        cleanup();
        fprintf(stderr, "main(): PCA9685_setPWMVals() returned ");
        fprintf(stderr, "%d on addr %02x\n", ret, addr);
        return -1;
      } // if 

      #ifdef VALIDATE
      // GET THE VALS, EVERY TIME THROUGH THE LOOP
      ret = PCA9685_getPWMVals(fd, addr, getOnVals, getOffVals);
      if (ret != 0) {
        cleanup();
        fprintf(stderr, "main(): PCA9685_getPWMVals() returned ");
        fprintf(stderr, "%d on addr %02x at reg 0x06\n", ret, addr);
        return -1;
      } // if err

      // compare the written vals to the read vals
      for(int i=0; i<_PCA9685_CHANS; i++) {
        if (getOnVals[i] != setOnVals[i]) {
          cleanup();
          fprintf(stderr, "main(): getOnVals[%d] is %03x ", i, getOnVals[i]);
          fprintf(stderr, "but setOnVals[%d] is %03x\n", i, setOnVals[i]);
          exit(-1-i);
        } // if data mismatch
        if (getOffVals[i] != setOffVals[i]) {
          cleanup();
          fprintf(stderr, "main(): getOffVals[%d] is %03x ", i, getOffVals[i]);
          fprintf(stderr, "but setOffVals[%d] is %03x\n", i, setOffVals[i]);
          exit(-1-i);
        } // if data mismatch
      } // for channels
      #endif

      // increment the loop counter
      j++;

      if (manual || j >= size) {
        int stats[8];
        long int micros;
        int millis;
        int updfreq;
        int kbps;
        j = 0;

        if (automatic) {
          gettimeofday(&now, NULL);
          timersub(&now, &then, &diff);
          then = now;

          micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
          millis = micros / 1000;
          millis = (millis == 0) ? 1 : millis;
          updfreq = (int)((float)1000000 * (float)size) / (float)micros;
          bits = size * _PCA9685_CHANS * 4 * 8;
          #ifdef VALIDATE
          bits *= 2;
          #endif
          kbps = (bits * 1000000.0f) / (micros * 1024.0f);

          fema = (fema == 0) ? updfreq
            : (alpha * updfreq) + (invalpha * fema);
          mema = (mema == 0) ? millis
            : (alpha * millis) + (invalpha * mema);
          kema = (kema == 0) ? kbps
            : (alpha * kbps) + (invalpha * kema);

          stats[0] = size;
          stats[1] = bits;
          stats[2] = millis;
          stats[3] = mema;
          stats[4] = updfreq;
          stats[5] = fema;
          stats[6] = kbps;
          stats[7] = kema;

          dumpStats(stats);
        } // if automatic

        dumpVals(3, setOffVals, chan);

        #ifdef VALIDATE
        dumpVals(4, getOffVals, -1);
        #endif

      } // if update screen
    } // while forever
  } // perf context 

  return 0;
} // main 
