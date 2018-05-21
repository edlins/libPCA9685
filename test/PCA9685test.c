// test suite for libPCA9685
// copyright 2018 Scott Edlin

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <stdbool.h>

#include <PCA9685.h>
#include "config.h"

int adpt;
int addr;
int fd;


int testFailOpenI2C() {
  printf("testFailOpenI2C\n");
  int fadpt = 0;
  fd = PCA9685_openI2C(fadpt, addr);
  if (fd >= 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testFailOpenI2C: PCA9685_openI2C(%d, 0x%02x) returned %d\n", fadpt, addr, fd);
    return -1;
  } // if fd
  int faddr = 240;
  fd = PCA9685_openI2C(adpt, faddr);
  if (fd >= 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testFailOpenI2C: PCA9685_openI2C(%d, 0x%02x) returned %d\n", adpt, faddr, fd);
    return -1;
  } // if fd
  printf("passed\n\n");
  return 0;
}


int testOpenI2C() {
  printf("testOpenI2C\n");
  fd = PCA9685_openI2C(adpt, addr);
  if (fd < 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testOpenI2C: PCA9685_openI2C(%d, 0x%02x) returned %d\n", adpt, addr, fd);
    return -1;
  } // if fd
  printf("passed\n\n");
  return 0;
}


int testFailInitPWM() {
  printf("testFailInitPWM\n");
  int freq = 200;
  int ffd = -1;
  int rc = PCA9685_initPWM(ffd, addr, freq);
  if (rc == 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testFailInitPWM: PCA9685_initPWM(%d, 0x%02x, %d) returned %d\n", ffd, addr, freq, rc);
    return -1;
  } // if rc
  int faddr = -240;
  rc = PCA9685_initPWM(fd, faddr, freq);
  if (rc == 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testFailInitPWM: PCA9685_initPWM(%d, 0x%02x, %d) returned %d\n", fd, faddr, freq, rc);
    return -1;
  } // if rc
  printf("passed\n\n");
  return 0;
}


int testInitPWM() {
  printf("testInitPWM\n");
  int freq = 200;
  int rc = PCA9685_initPWM(fd, addr, freq);
  if (rc != 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testInitPWM: PCA9685_initPWM(%d, 0x%02x, %d) returned %d\n", fd, adpt, freq, rc);
    return -1;
  } // if rc
  printf("passed\n\n");
  return 0;
}


int testFailWriteAllChannels() {
  printf("testFailWriteAllChannels\n");
  int ffd = -1;
  unsigned int setOnVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned int setOffVals[_PCA9685_CHANS] =
    { _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL };
  int rc = PCA9685_setPWMVals(ffd, addr, setOnVals, setOffVals);
  if (rc == 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testFailWriteAllChannels: PCA9685_setPWMVals(%d, 0x%02x, NULL, setOffVals) returned %d\n", fd, addr, rc);
    return -1;
  } // if rc
  printf("passed\n\n");
  return 0;
}


int testWriteAllChannels() {
  printf("testWriteAllChannels\n");
  unsigned int setOnVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned int setOffVals[_PCA9685_CHANS] =
    { _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL,
      _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL, _PCA9685_MAXVAL };
  int rc = PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);
  if (rc != 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testWriteAllChannels: PCA9685_setPWMVals(%d, 0x%02x, setOnVals, setOffVals) returned %d\n", fd, addr, rc);
    return -1;
  } // if rc
  printf("passed\n\n");
  return 0;
}


int testTurnOffAllChannels() {
  printf("testTurnOffAllChannels\n");
  unsigned int setOnVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  unsigned int setOffVals[_PCA9685_CHANS] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int rc = PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);
  if (rc != 0 && !_PCA9685_TEST) {
    fprintf(stderr, "ERROR: testTurnOffAllChannels: PCA9685_setPWMVals(%d, 0x%02x, setOnVals, setOffVals) returned %d\n", fd, addr, rc);
    return -1;
  } // if rc
  printf("passed\n\n");
  return 0;
}


int main(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "tdv")) != -1) {
    switch(c) {
    case 't': // test
      _PCA9685_TEST = 1;
      break;
    case 'd': // debug
      _PCA9685_DEBUG = 1;
      break;
    case 'v': // version
      fprintf(stdout, "PCA9685test %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
      exit(0);
      break;
    }
  }

  if ((argc - optind) != 2) {
    fprintf(stderr, "Usage: %s [-t] [-d] [-v] adapter address\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t test, log instead of execute hardware calls\n");
    fprintf(stderr, "  -d debug, log additional debug info\n");
    fprintf(stderr, "  -v version, print version info at startup\n");
    exit(-1);
  } // if argc

  long ladpt = strtol(argv[optind], NULL, 16);
  long laddr = strtol(argv[optind + 1], NULL, 16);
  if (ladpt > INT_MAX || ladpt < 0) {
    fprintf(stderr, "ERROR: adapter number %ld is not valid\n", ladpt);
    exit(-1);
  } // if ladpt
  adpt = ladpt;

  if (laddr > INT_MAX || laddr < 0) {
    fprintf(stderr, "ERROR: address 0x%02lx is not valid\n", laddr);
    exit(-1);
  } // if laddr
  addr = laddr;
  int rc;

  rc = testFailOpenI2C();
  if (rc) {
    fprintf(stderr, "ERROR: testFailOpenI2C() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testOpenI2C();
  if (rc) {
    fprintf(stderr, "ERROR: testOpenI2C() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testFailInitPWM();
  if (rc) {
    fprintf(stderr, "ERROR: testFailInitPWM() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testInitPWM();
  if (rc) {
    fprintf(stderr, "ERROR: testInitPWM() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testFailWriteAllChannels();
  if (rc) {
    fprintf(stderr, "ERROR: testFailWriteAllChannels() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testWriteAllChannels();
  if (rc) {
    fprintf(stderr, "ERROR: testWriteAllChannels() returned %d\n", rc);
    exit(-1);
  } // if rc

  rc = testTurnOffAllChannels();
  if (rc) {
    fprintf(stderr, "ERROR: testTurnOffAllChannels() returned %d\n", rc);
    exit(-1);
  } // if rc

  printf("All tests passed.\n");
  return 0;
}
