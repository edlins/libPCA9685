#define CHANNELS 1
#define FRAMES 26
#define AUDIOFREQ 22050

/* low freq resp = audiofreq / 2*frames */
/* high freq resp = audiofreq / 2 */

#define ADPT 1
#define ADDR 0x40
#define PWMFREQ 200

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <PCA9685.h>
#include <signal.h>

int fd;
unsigned char addr;

void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);
  exit(dummy);
}

int initPCA9685(int adpt, int addr, int freq) {
  int afd = PCA9685_openI2C(adpt, addr);
  PCA9685_initPWM(afd, addr, freq);
  return afd;
}


snd_pcm_t* initALSA(char *dev, char **bufferPtr, snd_pcm_uframes_t *framesPtr) {
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  //snd_pcm_uframes_t frames;
  //char *buffer;

  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, dev, SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", dev, snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  rc = snd_pcm_hw_params_any(handle, params);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_any() failed %d\n", rc);
  }

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_access() failed %d\n", rc);
  }

  /* Signed 16-bit little-endian format */
  rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_format() failed %d\n", rc);
  }

  /* channels */
  rc = snd_pcm_hw_params_set_channels(handle, params, 2);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_channels() failed %d\n", rc);
  }

  /* 44100 bits/second sampling rate (CD quality) */
  val = AUDIOFREQ;
  rc = snd_pcm_hw_params_set_rate_near(handle, params, &val, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_rate_near() failed %d\n", rc);
  }
  fprintf(stdout, "set_rate_near val %u\n", val);

  /* Set period size to 32 frames. */
  *framesPtr = FRAMES;
  fprintf(stdout, "frames %lu\n", *framesPtr);
  rc = snd_pcm_hw_params_set_period_size_near(handle, params, framesPtr, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_period_size_near() failed %d\n", rc);
  }
  fprintf(stdout, "frames %lu\n", *framesPtr);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: (%d), %s\n", rc, snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, framesPtr, NULL);
  size = *framesPtr * 2 * CHANNELS; /* 2 bytes/sample, 2 channels */
  *bufferPtr = (char *) malloc(size);

  /* We want to loop for 5 seconds */
  rc = snd_pcm_hw_params_get_period_time(params, &val, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_get_period_time() failed %d\n", rc);
  }
  return handle;
}




int main(int argc, char **argv) {
  //fprintf(stdout, "quickstart %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  signal(SIGINT, intHandler);

  // ALSA init
  char *buffer;
  snd_pcm_t *handle;
  snd_pcm_uframes_t frames;
  char *dev = "default";
  if (argc > 1) dev = argv[1];
  handle = initALSA(dev, &buffer, &frames);

  // libPCA9685 init
  int adpt = ADPT;
  addr = ADDR;
  int freq = PWMFREQ;
  fd = initPCA9685(adpt, addr, freq);

  int maxVal = 32000;
  int minVal = -32000;
  int average = 0;
  int minValue = 32000;
  while (1) {
    int rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } else {
    /* rc = number of frames read */
    int i;
    int sample;
    long tmp;
    int minSample=32000;
    int maxSample=-32000;
    for (i = 0; i < rc; i += 1) {
      tmp = (long)buffer[4*i+1] << 8 | buffer[4*i];
      if (tmp < 32768) sample = tmp;
      else sample = tmp - 65536;
      //sample = sample - minVal;
      //if (sample < 0) sample *= -1;
      if (sample > maxSample) {
        maxSample = sample;
      }
      if (sample < minSample) {
        minSample = sample;
      }
    }
    int value = maxSample - minSample;
    if (value < minValue) {
      minValue = value;
    }
    value -= minValue;
    //if (value < 100) {
      //value = 0;
    //}
    average = (value + 2 * average) / 3;
    //int ratio = 100.0 * (average - minVal) / (maxVal - minVal);
    int ratio = 100 * (average) / (1000);
    //int ratio = 100 * (value) / (1000);
    if (ratio < 0) ratio = 0;
    if (ratio > 100) ratio = 100;
    int display = ratio / 100.0 * _PCA9685_MAXVAL;
    //fprintf(stdout, "%d %d %d %d\n", value, average, ratio, display);
    int j;
    //for (j = 0; j < ratio / 10; j++) {
      //fprintf(stdout, "*");
    //}
    //fprintf(stdout, "\n");
    PCA9685_setAllPWM(fd, addr, 0, display);
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
