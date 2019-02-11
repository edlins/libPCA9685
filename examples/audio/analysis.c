#include <stdio.h>
#include <fftw3.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define A 128
#define H 128
#define F 512
#define C 1
#define B 2

void zero_imag(fftw_complex* c, int n) {
  for (int i = 0; i < n; i++) {
    c[i][1] = 0;
  } // for i
} // zero_real

void readi(char* buf, int a, int b, int c) {
  static long t = 0;
  fprintf(stderr, "read %d frames from pcm\n", a);
  for (int i = 0; i < a; i++) {
    int value = ((1 << 8 * b) / 2) * sin(t / 10.0);
    int sample = value;
    if (sample < 0) sample += 1 << 8 * b;
    //fprintf(stderr, "index %d value %d sample %d ", i, value, sample);
    for (int channel = 0; channel < c; channel++) {
      for (int bytenum = 0; bytenum < b; bytenum++) {
        int index = i * b + bytenum + channel * b;
        uint8_t byteval = (uint8_t) (sample & 0xFF);
        if (channel == 1) byteval = 0;
        buf[index] = byteval;
        //fprintf(stderr, "buf[%d] = %02x ", index, byteval);
        sample >>= 8;
      } // for bytenum
      //fprintf(stderr, "\n");
    } // for channel
    t++;
  } // for i
} // readi

double extract_sample(char* buf, int i, int b, int c) {
  long sample = 0;
  for (int bytenum = b - 1; bytenum >= 0; bytenum--) {
    int index = i * b * c + bytenum;
    uint8_t byteval = (uint8_t) buf[index];
    sample <<= 8;
    sample |= (byteval & 0xFF);
    //fprintf(stderr, "buf[%d] = %02x ", index, byteval);
  } // for bytenum
  long value = sample;
  if (value > (1 << 8 * b) / 2) value -= 1 << 8 * b;
  //fprintf(stderr, "frame %d sample %ld value %ld\n", i, sample, value);
  return (double) value;
} // extract_sample

void shift(int16_t* dest, int16_t* src, int size) {
  //fprintf(stderr, "shift %d from %p to %p\n", size, src, dest);
  memmove(dest, src, size * sizeof(int16_t));
} // shift_real

void dump(fftw_complex* b, int n) {
  for (int i = 0; i < n; i++) {
    fprintf(stderr, "%.0f ", b[i][0]);
  } // for i
  fprintf(stderr, "\n");
} // dump

void transform(fftw_complex* b, int n) {
  dump(b, n);
} // transform

void set_reals(fftw_complex* c, int16_t* r, int n) {
  for (int i = 0; i < n; i++) {
    c[i][0] = r[i];
  } // for i
} // set_reals

int main(void) {
  fftw_complex* fb = (fftw_complex*) malloc(F * sizeof(fftw_complex));
  int16_t* sb = (int16_t*) malloc(F * sizeof(int16_t));
  zero_imag(fb, F);
  char* ab = (char*) malloc(A * B * C * sizeof(uint8_t));
  int transforms = 10;
  int audio_offset = A;
  int fourier_offset = 0;
  while (1) {
    fprintf(stderr, "start audio %d fourier %d\n", audio_offset, fourier_offset);
    if (fourier_offset == F) {
      set_reals(fb, sb, F);
      //window(fb);
      transform(fb, F);
      if (--transforms == 0) exit(0);
      shift(sb, sb + H, F - H);
      fourier_offset = F - H;
    } // if fourier
    if (audio_offset == A) {
      readi(ab, A, B, C);
      audio_offset = 0;
    } // if audio
    fprintf(stderr, "audio %d -> fourier %d\n", audio_offset, fourier_offset);
    sb[fourier_offset] = extract_sample(ab, audio_offset, B, C);
    fourier_offset++;
    audio_offset++;
  } // while
  return 0;
} // main
