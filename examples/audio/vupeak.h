#include <stdbool.h>

typedef struct audiopwms {
  unsigned int mode;
  char* audio_device;
  char* audio_playback_device;
  unsigned int audio_period;
  unsigned int audio_rate;
  unsigned int audio_channels;
  unsigned int audio_buffer_size;
  unsigned int audio_buffer_period;
  unsigned int audio_overlap;
  unsigned int audio_bytes;
  unsigned int pwm_bus;
  unsigned int pwm_addr;
  unsigned int pwm_freq;
  bool pwm_debug;
  unsigned int pwm_smoothing;
  bool fft_hanning;
  bool test_period;
  bool ascii_waterfall;
  bool vocoder;
} audiopwm;
