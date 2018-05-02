#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdint.h>

#include "PCA9685.h"
#include "libPCA9685Config.h"

// debug flag
int _PCA9685_DEBUG = 0;
// test flag
int _PCA9685_TEST = 0;
// mode1 value hardware defaults (all call and sleep)
unsigned char _PCA9685_MODE1 = 0x00 | _PCA9685_ALLCALLBIT | _PCA9685_SLEEPBIT;
// mode2 value hardware defaults (totem pole mode)
unsigned char _PCA9685_MODE2 = 0x00 | _PCA9685_OUTDRVBIT;

#define INT2VOIDP(i) (void*)(uintptr_t)(i)

/////////////////////////////////////////////////////////////////////
// open the I2C bus device and assign the default slave address 
int PCA9685_openI2C(unsigned char adapterNum, unsigned char addr) {
  if (_PCA9685_DEBUG) {
    printf("**************DEBUG**************\n");
    printf("libPCA9685 %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  }
  int fd;
  int ret;

  // build the I2C bus device filename 
  char filename[20];
  sprintf(filename, "/dev/i2c-%d", adapterNum);

  // open the I2C bus device 
  fd = _PCA9685_open(filename, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "PCA9685_openI2C(): _PCA9685_open() returned %d for %s\n", fd, filename);
    return -1;
  } // if 

  if (_PCA9685_DEBUG) {
    printf("PCA9685_openI2C(): opened %s as fd %d\n", filename, fd);
  }

  // set the default slave address for read() and write() 
  void *p = INT2VOIDP(addr);
  ret = _PCA9685_ioctl(fd, I2C_SLAVE, (char *) p);
  if (ret < 0) {
    fprintf(stderr, "PCA9685_openI2C(): _PCA9685_ioctl() returned %d for addr %d\n", ret, addr);
    return -1;
  } // if 

  return fd;
} // PCA9685_openI2C 



/////////////////////////////////////////////////////////////////////
// initialize a PCA9685 device to defaults, turn off PWM's, and set the freq 
int PCA9685_initPWM(int fd, unsigned char addr, unsigned int freq) {
  int ret;

  // send a software reset to get defaults 
  unsigned char resetval = _PCA9685_RESETVAL;
  ret = _PCA9685_writeI2CRaw(fd, _PCA9685_MODE1REG, 1, &resetval);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_initPWM(): _PCA9685_writeI2CRaw() returned %d\n", ret);
    return -1;
  } // if 

  // after the reset, all of the control registers default vals are ok 

  // turn all PWM's off 
  ret = PCA9685_setAllPWM(fd, addr, 0x00, 0x00);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_initPWM(): PCA9685_setAllPWM() returned %d\n", ret);
    return -1;
  } // if 

  // set the oscillator frequency 
  ret = _PCA9685_setPWMFreq(fd, addr, freq);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_initPWM(): _PCA9685_setPWMFreq() returned %d\n", ret);
    return -1;
  } // if 

  // set MODE1 register using default value with AUTOINC
  // and without any of SLEEP, EXTCLK, and RESTART
  unsigned char mode1val = _PCA9685_MODE1 | _PCA9685_AUTOINCBIT;
  mode1val = mode1val & ~_PCA9685_SLEEPBIT & ~_PCA9685_EXTCLKBIT & ~_PCA9685_RESTARTBIT;
  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_MODE1REG, 1, &mode1val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_initPWM(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x\n", ret, addr);
    return -1;
  } // if 

  // set MODE2 register
  unsigned char mode2val = _PCA9685_MODE2;
  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_MODE2REG, 1, &mode2val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_initPWM(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x\n", ret, addr);
    return -1;
  } // if 

  return 0;
} // PCA9685_initPWM



/////////////////////////////////////////////////////////////////////
// set all PWM OFF vals in one transaction 
int PCA9685_setPWMVals(int fd, unsigned char addr,
                       unsigned int* onVals, unsigned int* offVals) {

  unsigned char regVals[_PCA9685_CHANS*4];
  int ret;

  { int i;
    for (i=0; i<_PCA9685_CHANS; i++) {
      regVals[i*4+0] = onVals[i] & 0xFF;
      regVals[i*4+1] = onVals[i] >> 8;
      regVals[i*4+2] = offVals[i] & 0xFF;
      regVals[i*4+3] = offVals[i] >> 8;
    } // for 

    if (_PCA9685_DEBUG) {
      { int i;
        // report the write 
        printf("PCA9685_setPWMVals(): vals[%d]: ", _PCA9685_CHANS);
        for (i=0; i<_PCA9685_CHANS; i++) {
          unsigned int offValue = regVals[i*4+3] << 8;
          offValue += regVals[i*4+2];
          printf(" %03x", offValue);
        } // for 
        printf("\n");
      }
    }
  
    ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_BASEPWMREG,
                               _PCA9685_CHANS*4, regVals);
    if (ret != 0) {
      fprintf(stderr, "PCA9685_setPWMVals(): _PCA9685_writeI2CReg() returned ");
      fprintf(stderr, "%d, addr %02x, reg %02x, len %d\n",
              ret, addr, _PCA9685_BASEPWMREG, _PCA9685_CHANS*4);
      return -1;
    } // if 
  } // int context 
  return 0;
} // PCA9685_setPWMVals



/////////////////////////////////////////////////////////////////////
// set a PWM channel (4 bytes) with the low 12 bits from a 16-bit value 
int PCA9685_setPWMVal(int fd, unsigned char addr, unsigned char reg,
                      unsigned int on, unsigned int off) {
  unsigned char val;
  int ret;

  if (_PCA9685_DEBUG) {
    printf("PCA9685_setPWMVal(): reg %02x, on %02x, off %02x\n", reg, on, off);
  }

  // ON_L 
  val = on & 0xFF; // mask all bits above 8 
  ret = _PCA9685_writeI2CReg(fd, addr, reg, 1, &val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_setPWMVal(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x reg %02x val %02x\n", ret, addr, reg, val);
    return -1;
  } // if 

  // ON_H 
  val = on >> 8; // fetch all bits above 8 
  ret = _PCA9685_writeI2CReg(fd, addr, reg+1, 1, &val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_setPWMVal(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x reg %02x val %02x\n", ret, addr, reg, val);
    return -1;
  } // if 

  // OFF_L 
  val = off & 0xFF; // mask all bits above 8 
  ret = _PCA9685_writeI2CReg(fd, addr, reg+2, 1, &val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_setPWMVal(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x reg %02x val %02x\n", ret, addr, reg, val);
    return -1;
  } // if 

  // OFF_H 
  val = off >> 8; // fetch all bits above 8 
  ret = _PCA9685_writeI2CReg(fd, addr, reg+3, 1, &val);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_setPWMVal(): _PCA9685_writeI2CReg() returned ");
    fprintf(stderr, "%d on addr %02x reg %02x val %02x\n", ret, addr, reg, val);
    return -1;
  } // if 
  
  return 0;
} // PCA9685_setPWMVal 



/////////////////////////////////////////////////////////////////////
// set all PWM channels using the ALL_LED registers 
int PCA9685_setAllPWM(int fd, unsigned char addr,
                      unsigned int on, unsigned int off) {
  int ret;

  // send the values to the ALL_LED registers 
  ret = PCA9685_setPWMVal(fd, addr, _PCA9685_ALLLEDREG, on, off);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_setAllPWM(): PCA9685_setPWMVal() returned %d\n", ret);
    return -1;
  } // if 

  return 0;
} // PCA9685_setAllPWM 



/////////////////////////////////////////////////////////////////////
// get both register values in one transaction
int PCA9685_getRegVals(int fd, unsigned char addr,
                       unsigned char* mode1val, unsigned char* mode2val) {
  int ret;
  unsigned char readBuf[2];

  ret = _PCA9685_readI2CReg(fd, addr, _PCA9685_MODE1REG, 2, readBuf);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_getRegVals(): _PCA9685_readI2CReg() returned ");
    fprintf(stderr, "%d on reg %02x\n", ret, _PCA9685_MODE1REG);
    return -1;
  } // if err

  *mode1val = readBuf[0];
  *mode2val = readBuf[1];

  return 0;
} // PCA9685_getPWMVals


/////////////////////////////////////////////////////////////////////
// get all PWM channels in an array of OFF vals in one transaction
int PCA9685_getPWMVals(int fd, unsigned char addr,
                       unsigned int* onVals, unsigned int* offVals) {
  int ret;
  unsigned char readBuf[_PCA9685_CHANS*4];

  ret = _PCA9685_readI2CReg(fd, addr, _PCA9685_BASEPWMREG,
                            _PCA9685_CHANS*4, readBuf);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_getPWMVals(): _PCA9685_readI2CReg() returned ");
    fprintf(stderr, "%d on reg %02x\n", ret, _PCA9685_BASEPWMREG);
    return -1;
  } // if err

  int i;
  for (i=0; i<_PCA9685_CHANS; i++) {
    onVals[i] = readBuf[i*4+1] << 8;
    onVals[i] += readBuf[i*4+0];
    offVals[i] = readBuf[i*4+3] << 8;
    offVals[i] += readBuf[i*4+2];
  } // for channels

  if (_PCA9685_DEBUG) {
    int i;
    // report the read
    printf("PCA9685_getPWMVals(): vals[%d]: ", _PCA9685_CHANS);
    for (i=0; i<_PCA9685_CHANS; i++) {
      printf(" %03x", offVals[i]);
    } // for
    printf("\n");
  } // if debug

  return 0;
} // PCA9685_getPWMVals



/////////////////////////////////////////////////////////////////////
// get a single PWM channel 16-bit ON val and 16-bit OFF val
int PCA9685_getPWMVal(int fd, unsigned char addr, unsigned char reg,
                      unsigned int* on, unsigned int* off) {
  int ret;
  unsigned char readBuf[4];

  ret = _PCA9685_readI2CReg(fd, addr, reg, 4, readBuf);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_getPWMVal(): _PCA9685_readI2CReg() returned ");
    fprintf(stderr, "%d on reg %02x\n", ret, reg);
    return -1;
  } // if err

  *on = readBuf[1] << 8;
  *on += readBuf[0];
  *off = readBuf[3] << 8;
  *off += readBuf[2];

  return 0;
} // PCA9685_getPWMVal


/////////////////////////////////////////////////////////////////////
// print out the values of all registers used in a PCA9685
int PCA9685_dumpAllRegs(int fd, unsigned char addr) {
  unsigned char loBuf[_PCA9685_LOREGS];
  unsigned char hiBuf[_PCA9685_HIREGS];
  int ret;

  // read all the low PCA9685 registers 
  ret = _PCA9685_readI2CReg(fd, addr, _PCA9685_FIRSTLOREG,
                            _PCA9685_LOREGS, loBuf);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_dumpAllRegs(): _PCA9685_readI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // display all of the low PCA9685 register values 
  _PCA9685_dumpLoRegs(loBuf);

  // read all the high PCA9685 registers 
  ret = _PCA9685_readI2CReg(fd, addr, _PCA9685_FIRSTHIREG,
                            _PCA9685_HIREGS, hiBuf);
  if (ret != 0) {
    fprintf(stderr, "PCA9685_dumpAllRegs(): _PCA9685_readI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // display all of the high PCA9685 register values 
  _PCA9685_dumpHiRegs(hiBuf);

  return 0;
} // PCA9685_dumpAllRegs 
/////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////
// internal functions, may be used but usually not required:

/////////////////////////////////////////////////////////////////////
// set the PWM frequency 
int _PCA9685_setPWMFreq(int fd, unsigned char addr, unsigned int freq) {
  int ret;
  unsigned char mode1Val;
  unsigned char prescale;

  // get initial mode1Val 
  ret = _PCA9685_readI2CReg(fd, addr, _PCA9685_MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_setPWMFreq(): _PCA9685_readI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // clear restart 
  mode1Val = mode1Val & ~_PCA9685_RESTARTBIT;
  // set sleep 
  mode1Val = mode1Val | _PCA9685_SLEEPBIT;

  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_setPWMFreq(): _PCA9685_writeI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // freq must be in range
  freq = (freq > _PCA9685_MAXFREQ
               ? _PCA9685_MAXFREQ
               : (freq < _PCA9685_MINFREQ
                       ? _PCA9685_MINFREQ
                       : freq));
  // calculate and set prescale 
  prescale = (unsigned char)(25000000.0f / (4096.0f * freq) - 0.5f);

  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_PRESCALEREG, 1, &prescale);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_setPWMFreq(): _PCA9685_writeI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // wake 
  mode1Val = mode1Val & ~_PCA9685_SLEEPBIT;
  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_setPWMFreq(): _PCA9685_writeI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  // allow the oscillator to stabilize at least 500us 
  { struct timeval sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_usec = 1000;
    ret = select(0, NULL, NULL, NULL, &sleeptime);
    if (ret < 0) {
      fprintf(stderr, "_PCA9685_setPWMFreq(): select() returned %d\n", ret);
      return -1;
    } // if 
  } // context 

  // restart 
  mode1Val = mode1Val | _PCA9685_RESTARTBIT;
  ret = _PCA9685_writeI2CReg(fd, addr, _PCA9685_MODE1REG, 1, &mode1Val);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_setPWMFreq(): _PCA9685_writeI2CReg() returned %d\n", ret);
    return -1;
  } // if 

  return 0;
} // _PCA9685_setPWMFreq 



/////////////////////////////////////////////////////////////////////
// dump the contents of the first 70 registers (modes and PWMs) 
int _PCA9685_dumpLoRegs(unsigned char* buf) {
  int i;
  for (i=0; i<_PCA9685_LOREGS; i++) {
    if ((i-6)%16 == 0) { printf("\n"); }
    printf("%02x ", buf[i]);
  } // for 
  printf("\n");

  return 0;
} // _PCA9685_dumpLoRegs 



/////////////////////////////////////////////////////////////////////
// dump the contents of the last six registers 
int _PCA9685_dumpHiRegs(unsigned char* buf) {
  int i;
  for (i=0; i<_PCA9685_HIREGS; i++) {
    printf("%02x ", buf[i]);
  } // for 
  printf("\n");

  return 0;
} // _PCA9685_dumpHiRegs 



/////////////////////////////////////////////////////////////////////
// read characters from a register at an address 
int _PCA9685_readI2CReg(int fd, unsigned char addr, unsigned char startReg,
            int len, unsigned char* readBuf) {
  int ret;
  // will be using a two message transaction 
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msgs[2];

  // first message writes the start register address 
  msgs[0].addr = addr;
  msgs[0].flags = 0x00;
  msgs[0].len = 1;
  msgs[0].buf = &startReg;

  // second message reads the register value(s) 
  msgs[1].addr = addr;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = len;
  msgs[1].buf = readBuf;

  data.msgs = msgs;
  data.nmsgs = 2;

  // send the combined transaction 
  ret = _PCA9685_ioctl(fd, I2C_RDWR, (char *) &data);
  if (ret < 0) {
    fprintf(stderr, "_PCA9685_readI2CReg(): _PCA9685_ioctl() returned ");
    fprintf(stderr, "%d on addr %02x start %02x\n", ret, addr, startReg);
    return -1;
  } // if 

  if (_PCA9685_DEBUG) {
    { int i;
      // report the read 
      printf("_PCA9685_readI2CReg(): %02x:%02x:%02x", addr, startReg, len);
      // report the result 
      for (i=0; i<len; i++) {
        printf(" %02x", readBuf[i]);
      } // for 
      printf("\n");
    }
  }
  
  return 0;
} // _PCA9685_readI2CReg 



/////////////////////////////////////////////////////////////////////
// write characters to a register at an address 
int _PCA9685_writeI2CReg(int fd, unsigned char addr, unsigned char startReg,
             int len, unsigned char* writeBuf) {
  int ret;

  if (_PCA9685_DEBUG) {
    { int i;
      printf("_PCA9685_writeI2CReg(): %02x:%02x:%02x", addr, startReg, len);
      for (i=0; i<len; i++) {
        printf(" %02x", writeBuf[i]);
      } // for
      printf("\n");
    } // context
  }

  // prepend the register address to the buffer 
  unsigned char* rawBuf;
  rawBuf = (unsigned char*)malloc(len+1);
  rawBuf[0] = startReg;
  memcpy(&rawBuf[1], writeBuf, len);

  // pass the new buffer to the raw writer 
  ret = _PCA9685_writeI2CRaw(fd, addr, len+1, rawBuf);
  if (ret != 0) {
    fprintf(stderr, "_PCA9685_writeI2CReg(): _PCA9685_writeI2CRaw() returned ");
    fprintf(stderr, "%d on addr %02x reg %02x\n", ret, addr, startReg);
    return -1;
  } // if 

  free(rawBuf);

  return 0;
} // _PCA9685_writeI2CReg 



/////////////////////////////////////////////////////////////////////
// write characters to an i2c address 
int _PCA9685_writeI2CRaw(int fd, unsigned char addr, int len,
                         unsigned char* writeBuf) {
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msgs[1];
  int ret;

  // one msg in the transaction 
  msgs[0].addr = addr;
  msgs[0].flags = 0x00;
  msgs[0].len = len;
  msgs[0].buf = writeBuf;

  data.msgs = msgs;
  data.nmsgs = 1;

  // send a combined transaction 
  ret = _PCA9685_ioctl(fd, I2C_RDWR, (char *) &data);
  if (ret < 0) {
    int i;
    fprintf(stderr, "_PCA9685_writeI2CRaw(): _PCA9685_ioctl() returned ");
    fprintf(stderr, "%d on addr %02x\n", ret, addr);
    fprintf(stderr, "_PCA9685_writeI2CRaw(): len = %d, buf = ", len);
    for (i=0; i<len; i++) {
      fprintf(stderr, "%02x ", writeBuf[i]);
    } // for 
    fprintf(stderr, "\n");
    return -1;
  } // if 

  return 0;
} // _PCA9685_writeI2CRaw 



/////////////////////////////////////////////////////////////////////
// wrapper for ioctl()
int _PCA9685_ioctl(int fd, unsigned long int request, char *argp) {
  if (_PCA9685_DEBUG || _PCA9685_TEST) {
    if (request == I2C_RDWR) {
      struct i2c_rdwr_ioctl_data *datap = (struct i2c_rdwr_ioctl_data *) argp;
      struct i2c_rdwr_ioctl_data data = *datap;
      struct i2c_msg *msgp = (struct i2c_msg *) data.msgs;
      struct i2c_msg msg = *msgp;
      printf("_PCA9685_ioctl: fd = %d request = RDWR data.nmesgs = %d msg.addr = 0x%02x msg.flags = 0x%02x msg.len = %d *msg.buf = ",
              fd, data.nmsgs, msg.addr, msg.flags, msg.len);
      int i;
      for (i = 0; i < msg.len; i++) {
        unsigned char c = *(msg.buf + i);
        printf("0x%02x ", c);
      } // for
      printf("\n");
    } // if RDWR
    else if (request == I2C_SLAVE) {
      printf("_PCA9685_ioctl: fd = %d request = SLAVE argp = %p\n", fd, argp);
    } // if SLAVE
  } // if debug or test

  if (_PCA9685_TEST) {
    return 0;
  } // if test

  int ret;
  ret = ioctl(fd, request, argp);
  if (ret < 0) {
    fprintf(stderr, "_PCA9685_ioctl(): ioctl() returned %d\n", ret);
  } // if ret
  return ret;
} // _PCA9685_ioctl



/////////////////////////////////////////////////////////////////////
// wrapper for open()
int _PCA9685_open(const char *pathname, int flags) {
  if (_PCA9685_DEBUG || _PCA9685_TEST) {
    printf("_PCA9685_open: pathname = %s flags = 0x%02x\n", pathname, flags);
  } // if debug or test

  if (_PCA9685_TEST) {
    return 0;
  } // if test

  int ret = open(pathname, flags);
  if (ret < 0) {
    fprintf(stderr, "_PCA9685_open: open() returned %d\n", ret);
  } // if ret
  return ret;
} // _PCA9685_open
