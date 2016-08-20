#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include <PCA9685.h>

//#define DEBUG


int fd;
int addr = 0x40;

void intHandler(int dummy) {
  PCA9685_setAllPWM(fd, addr, 0x00, 0x00);
  exit(0);
} // intHandler 

// main driver 
int main(void) {
  int adpt = 1;
  //float freq = 200;
  float freq = 200;
  int ret;

  signal(SIGINT, intHandler);

  // setup the I2C bus device 
  fd = PCA9685_openI2C(adpt, addr);
  if (fd < 0) {
    printf("main(): PCA9685_openI2C() returned %d for adpt %d at addr %x\n",
            fd, adpt, addr);
    exit(fd);
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = dumpAllRegs(fd, addr);
  if (ret != 0) {
    printf("main(): dumpAllRegs() returned %d\n", ret);
    exit(ret);
  } // if 
  #endif

  // initialize the pca device 
  ret = PCA9685_initPWM(fd, addr, freq);
  if (ret != 0) {
    printf("main(): PCA9685_initPWM() returned %d\n", ret);
    exit(ret);
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = dumpAllRegs(fd, addr);
  if (ret != 0) {
    printf("main(): dumpAllRegs() returned %d\n", ret);
    exit(ret);
  } // if 
  #endif

  { // perf context 
    int j = 0;
    int chan;
    int steps[_PCA9685_CHANS];
    int vals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int size = 1024;
    struct timeval then, now, diff;

    gettimeofday(&then, NULL);
    
    for (chan=0; chan<_PCA9685_CHANS; chan++) {
      steps[chan] = rand() % 100 + 1;
    } // for 

    // blink endlessly 
    while (1) {
      int i;
  
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

      ret = PCA9685_setPWMVals(fd, addr, vals);
      if (ret != 0) {
        printf("main(): setEachPwm() returned %d on addr %02x\n", ret, addr);
        exit(ret);
      } // if 

      j++;
      if (j > size) {
        long int micros;
        int millis;
        j = 0;
        gettimeofday(&now, NULL);
        timersub(&now, &then, &diff);
        micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
        millis = micros / 1000;
        printf("%d refreshes in %d ms = %d Hz\n",
                size, millis, 1000*size/millis);
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
