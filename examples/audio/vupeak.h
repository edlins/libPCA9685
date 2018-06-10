typedef struct audiopwms {
  unsigned int mode;
  char *audio_device;
  unsigned int audio_period;
  unsigned int audio_rate;
  unsigned int audio_channels;
  unsigned int pwm_bus;
  unsigned int pwm_addr;
  unsigned int pwm_freq;
  unsigned int pwm_debug;
} audiopwm;
