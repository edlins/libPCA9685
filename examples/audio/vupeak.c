// audio params
// this fucks up alsa if set to 1
#define CHANNELS 2
// periods with more frames take more time to process
// periods with fewer frames increase the bin size
#define FRAMES 1024
// lower sampling rates take more time to fill the buffer
// higher sampling rates increase the bin size
#define AUDIOFREQ 44100

// pwm params
#define ADPT 1
#define ADDR 0x40
#define PWMFREQ 200     // just needs to be high enough to hide the flicker

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <PCA9685.h>
#include <signal.h>
#include <fftw3.h>
#include <math.h>
#include <sys/time.h>

int fd;
unsigned char addr;

void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);

  // cleanup alsa
  //snd_pcm_drain(handle);
  //snd_pcm_close(handle);
  //free(buffer);

  // cleanup fftw
  //fftw_destroy_plan(p);
  //fftw_free(in);
  //fftw_free(out);

  exit(dummy);
}

int initPCA9685(int adpt, int addr, int freq) {
  //_PCA9685_DEBUG = true;
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
  rc = snd_pcm_open(&handle, dev, SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", dev, snd_strerror(rc));
    exit(1);
  }
  snd_pcm_hw_params_alloca(&params);
  rc = snd_pcm_hw_params_any(handle, params);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_any() failed %d\n", rc);
  rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_access() failed %d\n", rc);
  rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_format() failed %d\n", rc);
  rc = snd_pcm_hw_params_set_channels(handle, params, 2);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_channels() failed %d\n", rc);
  unsigned int rate = AUDIOFREQ;
  rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_rate_near() failed %d\n", rc);
  fprintf(stdout, "rate %u\n", rate);
  *framesPtr = FRAMES;
  rc = snd_pcm_hw_params_set_period_size_near(handle, params, framesPtr, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_period_size_near() failed %d\n", rc);
  fprintf(stdout, "frames %lu\n", *framesPtr);
  fprintf(stdout, "frequency response %lu - %d\n", rate / (2 * *framesPtr), rate / 2);
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: (%d), %s\n", rc, snd_strerror(rc));
    exit(1);
  }
  snd_pcm_hw_params_get_period_size(params, framesPtr, NULL);
  size = *framesPtr * 2 * CHANNELS; /* 2 bytes/sample, 2 channels */
  fprintf(stdout, "buffer size %d\n", size);
  *bufferPtr = (char *) malloc(size);
  rc = snd_pcm_hw_params_get_period_time(params, &val, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_get_period_time() failed %d\n", rc);
  return handle;
}



double *hanning(int N) {
  int i;
  double *window = (double *) malloc(sizeof(double) * N);
  //fprintf(stdout, "hanning:\n");
  for (i = 0; i < N; i++) {
    window[i] = 0.5 * (1 - cos(2 * M_PI * i / N));
    //fprintf(stdout, "%f ", window[i]);
  }
  //fprintf(stdout, "\n");
  return window;
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

  // fftw init
  int N = frames;
  fftw_plan p;
  fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  double in[N];
  p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
  double *han = hanning(N);

  int average = 0;
  int minValue = 32000;
  int rc = 64;
  struct timeval then, now, diff;
  rc = 1024;
  gettimeofday(&then, NULL);
  while (1) {
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != (int) frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } else {
      // find the min and max time and value
      int sample;
      long tmp;
      int minSample=32000;
      int maxSample=-32000;

      // process all frames recorded
      int i;
      for (i = 0; i < FRAMES; i++) {
        //fprintf(stdout, "examing buffer at %d and %d\n", 2*CHANNELS*i+1, 2*CHANNELS*i);
        tmp = (long)((char*)buffer)[2*CHANNELS*i+1] << 8 | ((char*)buffer)[2*CHANNELS*i];
        if (tmp < 32768) sample = tmp;
        else sample = tmp - 65536;
        if (sample > maxSample) {
          maxSample = sample;
        }
        if (sample < minSample) {
          minSample = sample;
        }
        in[i] = (double) sample * han[i];
      }
      // fftw
      fftw_execute(p);
      unsigned int pwmoff[16];
      int minbin = 11;
      for (i = 0; i < (FRAMES / 2) + 1; i++) {
        if (i >= minbin && i <= minbin+15) {
          // normalize by the number of frames in a period and the hanning factor
          double mag = 2.0 * sqrtf(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / FRAMES;
          double amp = 20 * log10f(mag);
          //fprintf(stdout, "%3d ", (int) amp);
          float cutoff = 2;
          if (amp > cutoff) {
            double val = pow(amp - cutoff, 3);
            if (val > 4096) val = 4096;
            int alpha = 10;
            pwmoff[i-minbin] = (unsigned int) ((alpha-1) * pwmoff[i-minbin] + val) / alpha;
          } else {
            pwmoff[i-minbin] = 0;
          }
        }
      }
      //fprintf(stdout, "\n");

      // intensity-based value
      int intensity_value = maxSample - minSample;
      //fprintf(stdout, "intensity %d\n", intensity_value);

      // minValue is the smallest value seen, use for offset
      if (intensity_value < minValue) {
        minValue = intensity_value;
      }
      intensity_value -= minValue;
      //fprintf(stdout, "adjusted intensity %d\n", intensity_value);

      // modified moving average for smoothing
      average = (intensity_value + 2 * average) / 3;
      //fprintf(stdout, "average %d\n", average);
      int ratio = 100 * (average) / (1000);
      if (ratio < 0) ratio = 0;
      if (ratio > 100) ratio = 100;
      //fprintf(stdout, "ratio %d\n", ratio);
      int display = ratio / 100.0 * _PCA9685_MAXVAL;
      //fprintf(stdout, "display %d\n", display);

      // update the pwms
      //fprintf(stdout, "%d %d %d\n", frames, intensity_value, display);
      //PCA9685_setAllPWM(fd, addr, 0, display);
      unsigned int pwmon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      PCA9685_setPWMVals(fd, addr, pwmon, pwmoff);

      gettimeofday(&now, NULL);
      timersub(&now, &then, &diff);
      long int micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
      int millis = micros / 1000;
      //fprintf(stdout, "%d %d\n", rc, millis);
      then = now;
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
