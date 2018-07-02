#include <stdbool.h>

typedef struct audiopwms {
  unsigned int mode;
  char *audio_device;
  unsigned int audio_period;
  unsigned int audio_rate;
  unsigned int audio_channels;
  unsigned int audio_buffer_size;
  unsigned int audio_buffer_period;
  unsigned int audio_overlap;
  unsigned int pwm_bus;
  unsigned int pwm_addr;
  unsigned int pwm_freq;
  bool pwm_debug;
  unsigned int pwm_smoothing;
  bool fft_hanning;
} audiopwm;
