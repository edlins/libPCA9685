#include <stdbool.h>

typedef struct audiopwms {
  unsigned int mode;
  char* audio_device;
  char* audio_playback_device;
  unsigned int audio_period;
  unsigned int audio_rate;
  unsigned int audio_channels;
  unsigned int audio_buffer_size;
  unsigned int audio_bytes;
  bool pwm_debug;
  unsigned int pwm_fd;
  unsigned int pwm_bus;
  unsigned int pwm_addr;
  unsigned int pwm_freq;
  unsigned int pwm_smoothing;
  unsigned int fft_period;
  unsigned int fft_hop_period;
  bool fft_hanning;
  bool test_period;
  bool save_fourier;
  bool save_timeseries;
  bool ascii_waterfall;
  bool vocoder;
  bool robotize;
  unsigned int verbosity;
} audiopwm;
