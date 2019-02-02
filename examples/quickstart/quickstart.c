// quickstart example for libPCA9685
// copyright 2018 Scott Edlin

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include <PCA9685.h>
#include "config.h"

int fd;
int addr = 0x40;
// Example for a second device. Set addr2 to device address (if set to 0x00 no second device will be used)
int fd2;
int addr2 = 0x00;


void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);
  exit(dummy);
}


int initHardware(int adpt, int addr, int freq) {
  int afd = PCA9685_openI2C(adpt, addr);
  PCA9685_initPWM(afd, addr, freq);
  return afd;
}


int main(void) {
  //_PCA9685_DEBUG = 1;
  fprintf(stdout, "quickstart %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  int adpt = 1;
  int freq = 200;
  signal(SIGINT, intHandler);
  fd = initHardware(adpt, addr, freq);
  unsigned int setOnVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned int setOffVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  // DEVICE #2
  if (addr2) {
    fd2 = initHardware(adpt, addr2, freq);
    unsigned int setOnVals2[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int setOffVals2[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  } // if addr2

  // blink endlessly 
  while (1) {
    // setup random values array (seizure mode)
    int i;
    for (i=0; i<_PCA9685_CHANS; i++) {
      int random = rand();
      int value = (float) random / (float) RAND_MAX * _PCA9685_MAXVAL;
      setOffVals[i] = value;
    }
    // set the on and off vals on the PCA9685
    PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);

    // DEVICE #2
    if (addr2) {
      for (i=0; i<_PCA9685_CHANS; i++) {
        int random = rand();
        int value = (float) random / (float) RAND_MAX * _PCA9685_MAXVAL;
        setOffVals[i] = value;
      }
    
      // set the on and off vals on the second PCA9685
      PCA9685_setPWMVals(fd2, addr2, setOnVals, setOffVals);
    } // if addr2
  }
  return 0;
}
