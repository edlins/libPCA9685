#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PCA9685_H
#define _PCA9685_H

// number of channels
#define _PCA9685_CHANS		16

// position and length of usable registers (for dumping to stdout)
#define _PCA9685_FIRSTLOREG	0x00
#define _PCA9685_LOREGS		70
#define _PCA9685_FIRSTHIREG	0xFA
#define _PCA9685_HIREGS		5

// register addresses
#define _PCA9685_MODE1REG	0x00
#define _PCA9685_BASEPWMREG	0x06
#define _PCA9685_ALLLEDREG	0xFA
#define _PCA9685_PRESCALEREG	0xFE

// bit positions within MODE1 register
#define _PCA9685_SLEEPBIT	0x10
#define _PCA9685_AUTOINCBIT	0x20
#define _PCA9685_RESTARTBIT	0x80

// control register to initiate device reset
#define _PCA9685_RESETVAL	0x06

// PWM frequency limits
#define _PCA9685_MAXFREQ	1526
#define _PCA9685_MINFREQ	24

// PWM value limits
#define _PCA9685_MINVAL		0x000
#define _PCA9685_MAXVAL		0xFFF


// open the I2C bus device and assign the default slave address
int PCA9685_openI2C(unsigned char adpt, unsigned char addr);

// initialize a pca device to defaults, turn off PWM's, and set the freq
int PCA9685_initPWM(int fd, unsigned char addr, unsigned int freq);

// set all PWM channels from two arrays of ON and OFF vals in one transaction
int PCA9685_setPWMVals(int fd, unsigned char addr,
                       unsigned int* onVals, unsigned int* offVals);

// set a single PWM channel with a 16-bit ON val and a 16-bit OFF val
int PCA9685_setPWMVal(int fd, unsigned char addr, unsigned char reg,
                      unsigned int on, unsigned int off);

// set all PWM channels with one 16-bit ON val and one 16-bit OFF val
int PCA9685_setAllPWM(int fd, unsigned char addr,
                      unsigned int on, unsigned int off);

// get all PWM channels in two arrays of ON and OFF vals in one transaction
int PCA9685_getPWMVals(int fd, unsigned char addr,
                       unsigned int* onVals, unsigned int* offVals);

// get a single PWM channel 16-bit ON val and 16-bit OFF val
int PCA9685_getPWMVal(int fd, unsigned char addr, unsigned char reg,
                      unsigned int* on, unsigned int* off);

// print out the values of all registers used in a pca
int PCA9685_dumpAllRegs(int fd, unsigned char addr);



// set the PWM frequency
int _PCA9685_setPWMFreq(int fd, unsigned char addr, unsigned int freq);

// dump the contents of the LO registers (modes and PWM)
int _PCA9685_dumpLoRegs(unsigned char* buf);

// dump the contents of the last six registers
int _PCA9685_dumpHiRegs(unsigned char* buf);

// read I2C bytes from a register at an address
int _PCA9685_readI2CReg(int fd, unsigned char addr, unsigned char startReg,
            int len, unsigned char* readBuf);

// write I2C bytes to a register at an address
int _PCA9685_writeI2CReg(int fd, unsigned char addr, unsigned char startReg,
             int len, unsigned char* writeBuf);

// write I2C bytes to an address  
int _PCA9685_writeI2CRaw(int fd, unsigned char addr, int len,
                         unsigned char* writeBuf);

#endif

#ifdef __cplusplus
}
#endif
