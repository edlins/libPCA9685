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
#define NCMODE


int fd;
int addr = 0x40;


void cleanup() {
  // attmempt to turn off all PWM
  PCA9685_setAllPWM(fd, addr, 0x00, 0x00);
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
    return fd;
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): dumpAllRegs() returned %d\n", ret);
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
  ret = dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): dumpAllRegs() returned %d\n", ret);
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


int dumpVals(int vals[], int chan) {
  int i;
  for(i=0; i<_PCA9685_CHANS; i++) {

    #ifdef NCMODE
    int colorVal;
    colorVal = (int)((float)vals[i] / 682.667) + 1;
    if (i == chan) {
      attron(A_BOLD | COLOR_PAIR(colorVal));
    } // if
    else {
      attron(A_NORMAL | COLOR_PAIR(colorVal));
    } // else
    mvprintw(3, 5*i, "%03x", vals[i]);
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
  ret = cbreak();
  if (ret == ERR) {
    fprintf(stderr, "initScreen(): cbreak() returned ERR\n");
    return -1;
  } // if

  // enable extended keys
  ret = keypad(stdscr, TRUE);
  if (ret == ERR) {
    fprintf(stderr, "initScreen(): keypad() returned ERR\n");
    return -1;
  } // if

  // no echo
  ret = noecho();
  if (ret == ERR) {
    fprintf(stderr, "initScreen(): noecho() returned ERR\n");
    return -1;
  } // if

  ret = start_color();
  init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(2, COLOR_BLUE, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_GREEN, COLOR_BLACK);
  init_pair(5, COLOR_YELLOW, COLOR_BLACK);
  init_pair(6, COLOR_RED, COLOR_BLACK);

  // header row for stats
  mvprintw(0,  0, "frames");
  mvprintw(0, 10, "kbits");
  mvprintw(0, 20, "ms");
  mvprintw(0, 30, "avg");
  mvprintw(0, 40, "Hz");
  mvprintw(0, 50, "avg");
  mvprintw(0, 60, "kbps");
  mvprintw(0, 70, "avg");

  dumpVals((int[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 0);

  refresh();

  #else
  printf("frames    kbits     ms        avg       Hz        avg       kbps      avg\n");

  #endif

  return 0;
} // initScreen









// main driver 
int main(void) {
  int adpt = 1;
  float freq = 200;
  int ret;
  char automatic;
  char manual;

  #ifdef NCMODE
  automatic = 0;
  manual = 1;
  #else
  automatic = 1;
  manual = 0;
  #endif

  // register the signal handler to catch interrupts
  signal(SIGINT, intHandler);

  // initialize the I2C bus adpt and a PCA9685 at addr with freq
  fd = initHardware(adpt, addr, freq);
  if (fd < 0) {
    cleanup();
    fprintf(stderr, "main(): initHardware() returned ");
    fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, adpt, addr);
    exit(fd);
  } // if

  // initialize the screen
  ret = initScreen();
  if (ret < 0) {
    cleanup();
    fprintf(stderr, "main(): initScreen() returned %d\n", ret);
    exit(ret);
  } // if

  { // perf context 
    int j = 0;
    int chan = 0;
    int steps[_PCA9685_CHANS];
    int minStep = 1;
    int maxStep = 100;
    int vals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int size = 1024;
    int kbits;
    int mema = 0;
    int fema = 0;
    int kema = 0;
    float alpha = 0.1;
    struct timeval then, now, diff;

    gettimeofday(&then, NULL);
    
    // initialize steps for automatic mode
    int i;
    for (i=0; i<_PCA9685_CHANS; i++) {
      steps[i] = rand() % maxStep + minStep;
    } // for 

    // blink endlessly 
    while (1) {

      if (automatic) {
        int i;
        int c;
        //size = 128;
        // read a char (non-blocking)
        nodelay(stdscr, true);
        c = getch();
        if (c != ERR) {
          if (c == (int)('m')) {
            automatic = false;
            manual = true;
          } // if
        } // if

        // if m, manual = 0x01 and automatic = 0x00
        for (i=0; i<_PCA9685_CHANS; i++) {
          vals[i] += steps[i];
          if (vals[i] > 4095) {
            vals[i] = 4095;
            steps[i] *= -1;
          } // if 
          if (vals[i] < 0) {
            vals[i] = 0;
            steps[i] = rand() % maxStep + minStep;
          } // if 
        } // for 
      } // if auto

      else if (manual) {
        int c;
        int upStep = 128;
        //size = 0;

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
          vals[chan] = (vals[chan] + upStep > 4095)
                       ? 4095 : vals[chan] + upStep;
        } // if

        // else if down, drop vals[chan]
        else if (c == KEY_DOWN) {
          vals[chan] = (vals[chan] - upStep < 0)
                       ? 0 : vals[chan] - upStep;
        } // if

        // else if a, manual = 0x00 and automatic = 0x01
        else if (c == (int)('a')) {
          manual = 0x00;
          automatic = 0x01;
          gettimeofday(&then, NULL);
        } // if

        // else if 0, all vals = 0
        else if (c == (int)('0')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            vals[i] = 0x00;
          } // for
        } // if

        // else if 1, all vals = 0xFFF
        else if (c == (int)('1')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            vals[i] = 4095;
          } // for
        } // if
        // else if s, toggle strobe
      } // if manual

      ret = PCA9685_setPWMVals(fd, addr, vals);
      if (ret != 0) {
        cleanup();
        fprintf(stderr, "main(): setEachPwm() returned ");
        fprintf(stderr, "%d on addr %02x\n", ret, addr);
        exit(ret);
      } // if 

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

          micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
          millis = micros / 1000;
          updfreq = (int)((float)1000*(float)size/(float)millis);
          kbits = (size * _PCA9685_CHANS * 4 * 8) / 1024;
          kbps = kbits * 1000 / millis;

          fema = (fema == 0) ? updfreq : (alpha*updfreq) + ((1-alpha)*fema);
          mema = (mema == 0) ? millis : (alpha*millis) + ((1-alpha)*mema);
          kema = (kema == 0) ? kbps : (alpha*kbps) + ((1-alpha)*kema);

          stats[0] = size;
          stats[1] = kbits;
          stats[2] = millis;
          stats[3] = mema;
          stats[4] = updfreq;
          stats[5] = fema;
          stats[6] = kbps;
          stats[7] = kema;

          ret = dumpStats(stats);
        } // if

        ret = dumpVals(vals, chan);

        then = now;
      } // if 
    } // while 
  } // perf context 

  exit(0);
} // main 
