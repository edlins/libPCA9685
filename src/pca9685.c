#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "pca9685.h"

#define NUMLOREGS 70
#define NUMHIREGS 5

#define FIRSTLOREG 0x00
#define MODE1REG 0x00
#define BASEPWMREG 0x06
#define FIRSTHIREG 0xFA
#define ALLLEDREG 0xFA
#define PRESCALEREG 0xFE

#define SLEEPBIT 0x10
#define AUTOINCBIT 0x20
#define RESTARTBIT 0x80

#define RESETVAL 0x06

#define MAXFREQ 1526
#define MINFREQ 24

/*
#define DEBUG
*/

/* initialize a buffer with <len> <fill>s */
int initBuf(unsigned char* buf, int len, unsigned char fill) {
  int i;
  for (i=0; i<len; i++) {
    buf[i] = fill;
  } /* for */
  return 0;
} /* initBuf */



/* write characters to an i2c address */
int writeI2Craw(int fd, int addr, int len, unsigned char* writeBuf) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msgs[1];
  int ret;

  #ifdef DEBUG
  { int i;
    /* report the write */
    printf("wr %02x:", addr);
    for (i=0; i<len; i++) {
      printf(" %02x", writeBuf[i]);
    } /* for */
    printf("\n");
  }
  #endif
  
  /* one msg in the transaction */
  msgs[0].addr = addr;
  msgs[0].flags = 0x00;
  msgs[0].len = len;
  msgs[0].buf = writeBuf;

  data.msgs = msgs;
  data.nmsgs = 1;

  /* send a combined transaction */
  ret = ioctl(fd, I2C_RDWR, &data);
  if (ret < 0) {
    int i;
    printf("writeI2Craw(): ioctl() returned %d on addr %02x\n", ret, addr);
    printf("writeI2Craw(): len = %d, buf = ", len);
    for (i=0; i<len; i++) {
      printf("%02x ", writeBuf[i]);
    } /* for */
    printf("\n");
    return ret;
  } /* if */
  return 0;
} /* writeI2Craw */



/* write characters to a register at an address */
int writeI2C(int fd, int addr, unsigned char startReg,
             int len, unsigned char* writeBuf) {
  int ret;

  /* prepend the register address to the buffer */
  unsigned char* rawBuf;
  rawBuf = (unsigned char*)malloc(len+1);
  rawBuf[0] = startReg;
  memcpy(&rawBuf[1], writeBuf, len);

  /* pass the new buffer to the raw writer */
  ret = writeI2Craw(fd, addr, len+1, rawBuf);
  if (ret != 0) {
    printf("writeI2C(): writeI2Craw() returned %d on addr %02x reg %02x\n",
            ret, addr, startReg);
    return ret;
  } /* if */

  free(rawBuf);

  return 0;
} /* writeI2C */



/* read characters from a register at an address */
int readI2C(int fd, unsigned char addr, unsigned char startReg,
            int len, unsigned char* readBuf) {
  int ret;
  /* will be using a two message transaction */
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msgs[2];

  /* first message writes the start register address */
  msgs[0].addr = addr;
  msgs[0].flags = 0x00;
  msgs[0].len = 1;
  msgs[0].buf = &startReg;

  /* second message reads the register value(s) */
  msgs[1].addr = addr;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = len;
  msgs[1].buf = readBuf;

  data.msgs = msgs;
  data.nmsgs = 2;

  /* send the combined transaction */
  ret = ioctl(fd, I2C_RDWR, &data);
  if (ret < 0) {
    printf("readI2C(): ioctl() returned %d on addr %02x start %02x\n",
            ret, addr, startReg);
    return ret;
  } /* if */

  #ifdef DEBUG
  { int i;
    /* report the read */
    printf("r %02x:%02x:%02x", addr, startReg, len);
    /* report the result */
    for (i=0; i<len; i++) {
      printf(" %02x", readBuf[i]);
    } /* for */
    printf("\n");
  }
  #endif
  
  return 0;
} /* readI2C */



/* open the I2C bus device and assign the default slave address */
int setupI2C(int adapterNum, int addr) {
  int fd;
  int ret;

  /* build the I2C bus device filename */
  char filename[20];
  sprintf(filename, "/dev/i2c-%d", adapterNum);

  /* open the I2C bus device */
  fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf("setupI2C(): open() returned %d for %s\n", fd, filename);
    return fd;
  } /* if */

  #ifdef DEBUG
  printf("opened %s as fd %d\n", filename, fd);
  #endif

  /* set the default slave address for read() and write() */
  ret = ioctl(fd, I2C_SLAVE, addr);
  if (ret < 0) {
    printf("setupI2C(): ioctl() returned %d for device %d\n", ret, addr);
    return ret;
  } /* if */

  return fd;
} /* setupI2C */



/* set a pwm channel (4 bytes) with the low 12 bits from a 16-bit value */
int setPwm(int fd, int addr, char reg, int on, int off) {
  unsigned char val;
  int ret;

  #ifdef DEBUG
  printf("setPWM(): reg %02x, on %02x, off %02x\n", reg, on, off);
  #endif

  /* ON_L */
  val = on & 0xFF; /* mask all bits above 8 */
  ret = writeI2C(fd, addr, reg, 1, &val);
  if (ret != 0) {
    printf("setPwm(): writeI2C() returned %d on addr %02x reg %02x val %02x\n",
            ret, addr, reg, val);
    return ret;
  } /* if */

  /* ON_H */
  val = on >> 8; /* fetch all bits above 8 */
  ret = writeI2C(fd, addr, reg+1, 1, &val);
  if (ret != 0) {
    printf("setPwm(): writeI2C() returned %d on addr %02x reg %02x val %02x\n",
            ret, addr, reg, val);
    return ret;
  } /* if */

  /* OFF_L */
  val = off & 0xFF; /* mask all bits above 8 */
  ret = writeI2C(fd, addr, reg+2, 1, &val);
  if (ret != 0) {
    printf("setPwm(): writeI2C() returned %d on addr %02x reg %02x val %02x\n",
            ret, addr, reg, val);
    return ret;
  } /* if */

  /* OFF_H */
  val = off >> 8; /* fetch all bits above 8 */
  ret = writeI2C(fd, addr, reg+3, 1, &val);
  if (ret != 0) {
    printf("setPwm(): writeI2C() returned %d on addr %02x reg %02x val %02x\n",
            ret, addr, reg, val);
    return ret;
  } /* if */
  
  return 0;
} /* setPwm */



/* set all pwm channels using the ALL_LED registers */
int setAllPwm(int fd, int addr, int on, int off) {
  int ret;

  /* send the values to the ALL_LED registers */
  ret = setPwm(fd, addr, ALLLEDREG, on, off);
  if (ret != 0) {
    printf("setAllPwm(): setPwm() returned %d\n", ret);
    return ret;
  } /* if */

  return 0;
} /* setAllPwm */



/* set the pwm frequency */
int setPwmFreq(int fd, int addr, float freq) {
  int ret;
  unsigned char mode1Val;
  int prescale;

  /* get initial mode1Val */
  ret = readI2C(fd, addr, MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    printf("setPwmFreq(): readI2C() returned %d\n", ret);
    return ret;
  } /* if */

  /* clear restart */
  mode1Val = mode1Val & ~RESTARTBIT;
  /* set sleep */
  mode1Val = mode1Val | SLEEPBIT;

  ret = writeI2C(fd, addr, MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    printf("setPwmFreq(): writeI2C() returned %d\n", ret);
    return ret;
  } /* if */

  /* freq must be 40Hz - 1000Hz */
  freq = (freq > MAXFREQ ? MAXFREQ : (freq < MINFREQ ? MINFREQ : freq));
  /* calculate and set prescale */
  prescale = (int)(25000000.0f / (4096 * freq) - 0.5f);

  ret = writeI2C(fd, addr, PRESCALEREG, 1, (unsigned char*)&prescale);
  if (ret != 0) {
    printf("setPwmFreq(): writeI2C() returned %d\n", ret);
    return ret;
  } /* if */

  /* wake */
  mode1Val = mode1Val & ~SLEEPBIT;
  ret = writeI2C(fd, addr, MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    printf("setPwmFreq(): writeI2C() returned %d\n", ret);
    return ret;
  } /* if */

  /* allow the oscillator to stabilize at least 500us */
  /*usleep(1000);*/
  { struct timeval sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_usec = 1000;
    ret = select(0, NULL, NULL, NULL, &sleeptime);
    if (ret < 0) {
      printf("setPwmFreq(): select() returned %d\n", ret);
      return ret;
    } /* if */
  } /* context */

  /* restart */
  mode1Val = mode1Val | RESTARTBIT;
  ret = writeI2C(fd, addr, MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    printf("setPwmFreq(): writeI2C() returned %d\n", ret);
    return ret;
  } /* if */

  return 0;
} /* setPwmFreq */



/* initialize a pca device to defaults, turn off pwm's, and set the freq */
int initpca(int fd, int addr, float freq) {
  int ret;

  /* send a software reset to get defaults */
  unsigned char resetval = RESETVAL;
  unsigned char mode1val = 0x00 | AUTOINCBIT;

  ret = writeI2Craw(fd, MODE1REG, 1, &resetval);
  if (ret != 0) {
    printf("initpca(): writeI2Craw() returned %d\n", ret);
    return ret;
  } /* if */

  /* after the reset, all of the control registers default vals are ok */

  /* turn all pwm's off */
  ret = setAllPwm(fd, addr, 0x00, 0x00);
  if (ret != 0) {
    printf("initpca(): setAllPwm() returned %d\n", ret);
    return ret;
  } /* if */

  /* set the oscillator frequency */
  ret = setPwmFreq(fd, addr, freq);
  if (ret != 0) {
    printf("initpca(): setPwmFreq() returned %d\n", ret);
    return ret;
  } /* if */

  ret = writeI2C(fd, addr, MODE1REG, 1, &mode1val);
  if (ret != 0) {
    printf("initpca(): writeI2Craw() returned %d on addr %02x\n", ret, addr);
    return ret;
  } /* if */

  return 0;
} /* initpca */



/* dump the contents of the first 70 registers (modes and pwms) */
int dumpLoRegs(unsigned char* buf) {
  int i;
  for (i=0; i<NUMLOREGS; i++) {
    if ((i-6)%16 == 0) { printf("\n"); }
    printf("%02x ", buf[i]);
  } /* for */
  printf("\n");

  return 0;
} /* dumpLoRegs */



/* dump the contents of the last six registers */
int dumpHiRegs(unsigned char* buf) {
  int i;
  for (i=0; i<NUMHIREGS; i++) {
    printf("%02x ", buf[i]);
  } /* for */
  printf("\n");

  return 0;
} /* dumpHiRegs */



/* print out the values of all registers used in a pca */
int dumpAllRegs(int fd, int addr) {
  unsigned char loBuf[NUMLOREGS];
  unsigned char hiBuf[NUMHIREGS];
  int ret;

  initBuf(loBuf, NUMLOREGS, 0xFD);
  initBuf(hiBuf, NUMHIREGS, 0xFC);

  /* read all the low pca registers */
  ret = readI2C(fd, addr, FIRSTLOREG, NUMLOREGS, loBuf);
  if (ret != 0) {
    printf("dumpAllRegs(): readI2C() returned %d\n", ret);
    exit(ret);
  } /* if */

  /* display all of the low pca register values */
  dumpLoRegs(loBuf);

  /* read all the high pca registers */
  ret = readI2C(fd, addr, FIRSTHIREG, NUMHIREGS, hiBuf);
  if (ret != 0) {
    printf("dumpAllRegs(): readI2C() returned %d\n", ret);
    exit(ret);
  } /* if */

  /* display all of the high pca register values */
  dumpHiRegs(hiBuf);

  return 0;
} /* dumpAllRegs */



/* set each of the pwm vals separately */
int setEachPwm(int fd, int addr, int* vals) {
  int ret;
  int i;
  unsigned char reg = BASEPWMREG;
  for (i=0; i<NUMCHANNELS; i++) {
    reg = BASEPWMREG + 4*i;
    #ifdef DEBUG
    printf("setEachPwm(): i %d, reg %02x\n", i, reg);
    #endif
    ret = setPwm(fd, addr, reg, 0x00, vals[i]);
    if (ret != 0) {
      printf("setEachPwm(): setPwm() returned %d ", ret);
      printf("on addr %02x reg %02x val %02x\n", addr, reg, vals[i]); 
      return ret;
    } /* if */
  } /* for */
  return 0;
} /* setEachPwm */



/* set all pwm vals in one transaction */
/* FIXME
   remove len parameter and use NUMCHANNELS constant */
int setEachPwmBatch(int fd, int addr, int len, int* vals) {
  unsigned char regVals[NUMCHANNELS*4];
  int ret;

  #ifdef DEBUG
  { int i;
    /* report the write */
    printf("vals[%d]: ", len);
    for (i=0; i<len; i++) {
      printf(" %03x", vals[i]);
    } /* for */
    printf("\n");
  }
  #endif
  
  { int i;
    for (i=0; i<NUMCHANNELS; i++) {
      regVals[i*4] = 0x00;
      regVals[i*4+1] = 0x00;
      regVals[i*4+2] = vals[i] & 0xFF;
      regVals[i*4+3] = vals[i] >> 8;
    } /* for */
    ret = writeI2C(fd, addr, BASEPWMREG, NUMCHANNELS*4, regVals);
    if (ret != 0) {
      printf("setEachPwmBatch(): writeI2C() returned %d, ", ret);
      printf("addr %02x, reg %02x, len %d\n",
              addr, BASEPWMREG, NUMCHANNELS*4);
      return ret;
    } /* if */
  } /* int context */
  return 0;
} /* setEachPwmBatch */
