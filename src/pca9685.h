#ifndef PCA9685_H
#define PCA9685_H

#define NUMCHANNELS 16

/* initialize a buffer with <len> <fill>s
*/
int initBuf(unsigned char* buf, int len, unsigned char fill);

/* write characters to an i2c address  
*/
int writeI2Craw(int fd, int addr, int len, unsigned char* writeBuf);

/* write characters to a register at an address
*/
int writeI2C(int fd, int addr, unsigned char startReg,
             int len, unsigned char* writeBuf);

/* read characters from a register at an address
*/
int readI2C(int fd, unsigned char addr, unsigned char startReg,
            int len, unsigned char* readBuf);

/* open the I2C bus device and assign the default slave address
*/
int setupI2C(int adapterNum, int addr);

/* set a pwm channel (4 bytes) with the low 12 bits from a 16-bit value
*/
int setPwm(int fd, int addr, char reg, int on, int off);

/* set all pwm channels using the ALL_LED registers 
*/
int setAllPwm(int fd, int addr, int on, int off);

/* set the pwm frequency
*/
int setPwmFreq(int fd, int addr, float freq);

/* initialize a pca device to defaults, turn off pwm's, and set the freq
*/
int initpca(int fd, int addr, float freq);

/* dump the contents of the first 70 registers (modes and pwms)
*/
int dumpLoRegs(unsigned char* buf);

/* dump the contents of the last six registers
*/
int dumpHiRegs(unsigned char* buf);

/* print out the values of all registers used in a pca
*/
int dumpAllRegs(int fd, int addr);

/* set each of the pwm vals separately
*/
int setEachPwm(int fd, int addr, int* vals);

/* set all pwm vals in one transaction
*/
int setEachPwmBatch(int fd, int addr, int len, int* vals);

#endif
