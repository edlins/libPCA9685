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

  // header row for stats
  mvprintw(0,  0, "frames");
  mvprintw(0, 10, "kbits");
  mvprintw(0, 20, "ms");
  mvprintw(0, 30, "avg");
  mvprintw(0, 40, "Hz");
  mvprintw(0, 50, "avg");
  mvprintw(0, 60, "kbps");
  mvprintw(0, 70, "avg");

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
  char automatic = 0x01;
  char manual = 0x00;

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
    int chan;
    int steps[_PCA9685_CHANS];
    int vals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int size = 4096;
    int kbits = (size * _PCA9685_CHANS * 4 * 8) / 1024;
    int mema = 0;
    int fema = 0;
    int kema = 0;
    float alpha = 0.1;
    struct timeval then, now, diff;

    gettimeofday(&then, NULL);
    
    // initialize steps for automatic mode
    for (chan=0; chan<_PCA9685_CHANS; chan++) {
      steps[chan] = rand() % 100 + 1;
    } // for 

    // blink endlessly 
    while (1) {

      if (automatic) {
        int i;
        // read a char (non-blocking)
        // if m, manual = 0x01 and automatic = 0x00
        for (i=0; i<_PCA9685_CHANS; i++) {
          vals[i] += steps[i];
          if (vals[i] > 4095) {
            vals[i] = 4095;
            steps[i] *= -1;
          } // if 
          if (vals[i] < 0) {
            vals[i] = 0;
            steps[i] = rand() % 100 + 1;
          } // if 
        } // for 
      } // if auto

      else if (manual) {
        // read a char (blocking)
        // if left, chan--
        // else if right, chan++
        // else if up, vals[chan]+=128
        // else if down, vals[chan]-=128
        // else if a, manual = 0x00 and automatic = 0x01
        // else if 0, all vals = 0
        // else if 1, all vals = 1
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
      if (j >= size) {
        long int micros;
        int millis;
        int updfreq;
        int kbps;
        j = 0;
        gettimeofday(&now, NULL);
        timersub(&now, &then, &diff);
        micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
        millis = micros / 1000;
        updfreq = (int)((float)1000*(float)size/(float)millis);
        kbps = kbits * 1000 / millis;
        if (fema == 0) {
          fema = updfreq;
        } // if
        else {
          fema = (alpha * updfreq) + ((1 - alpha) * fema);
        } // else
        if (mema == 0) {
          mema = millis;
        } // if
        else {
          mema = (alpha * millis) + ((1 - alpha) * mema);
        } // else
        if (kema == 0) {
          kema = kbps;
        } // if
        else {
          kema = (alpha * kbps) + ((1 - alpha) * kema);
        } // else
        #ifdef NCMODE
        mvprintw(1, 0, "%d", size);
        mvprintw(1, 10, "%d", kbits);
        mvprintw(1, 20, "%d", millis);
        mvprintw(1, 30, "%d", mema);
        mvprintw(1, 40, "%d", updfreq);
        mvprintw(1, 50, "%d", fema);
        mvprintw(1, 60, "%d", kbps);
        mvprintw(1, 70, "%d", kema);
        refresh();
        #else
        printf("%04d      %04d      %04d      %04d      %04d      %04d      %04d      %04d\n",
                size, kbits, millis, mema, updfreq, fema, kbps, kema);
        #endif
        then = now;
      } // if 

      // at 3.2MHz i2c, it's getting around 2900Hz at ~16% cpu
      //   but combined reads don't work beyond one byte.
      //   with a 19ms sleep it slows to about 48Hz at ~0.7% cpu
      // at 1.0MHz (Fm+) it's geting 1500Hz at ~10%cpu.
      //   with a 19ms sleep it slows to about 50Hz at ~0.7% cpu 
      // int handler breaks nanosleep(); 
    } // while 
  } // perf context 

  exit(0);
} // main 
