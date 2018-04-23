// quickstart example for libPCA9685
// copyright 2018 Scott Edlin

#include <stdlib.h>
#include <signal.h>
#include <PCA9685.h>

int fd;
int addr = 0x40;


void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);
  exit(0);
}


int initHardware(int adpt, int addr, int freq) {
  int afd = PCA9685_openI2C(adpt, addr);
  PCA9685_initPWM(fd, addr, freq);
  return afd;
}


int main(void) {
  int adpt = 1;
  int freq = 200;
  signal(SIGINT, intHandler);
  fd = initHardware(adpt, addr, freq);
  unsigned int setOnVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned int setOffVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  // blink endlessly 
  while (1) {
    // setup random values array (seizure mode)
    for (int i=0; i<_PCA9685_CHANS; i++) {
      int random = rand();
      int value = (float) random / (float) RAND_MAX * _PCA9685_MAXVAL;
      setOffVals[i] = value;
    }
    // set the on and off vals on the PCA9685
    PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);
  }
  return 0;
}
