#include "vupeak.h"

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <PCA9685.h>
#include <signal.h>
#include <fftw3.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

// globals, defined here only for intHandler() to cleanup
audiopwm args;
int fd;
char *buffer;
snd_pcm_t *handle;
// verbosity flag
bool verbose = false;


void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, args.pwm_addr, _PCA9685_MINVAL, _PCA9685_MINVAL);

  // cleanup alsa
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  // cleanup fftw
  //fftw_destroy_plan(p);
  //fftw_free(in);
  //fftw_free(out);

  exit(dummy);
}

int initPCA9685(audiopwm args) {
  _PCA9685_DEBUG = args.pwm_debug;
  int afd = PCA9685_openI2C(args.pwm_bus, args.pwm_addr);
  PCA9685_initPWM(afd, args.pwm_addr, args.pwm_freq);
  return afd;
}


snd_pcm_t* initALSA(audiopwm args, char **bufferPtr) {
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  rc = snd_pcm_open(&handle, args.audio_device, SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", args.audio_device, snd_strerror(rc));
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
  //unsigned int rate = args.audio_rate;
  rc = snd_pcm_hw_params_set_rate_near(handle, params, &args.audio_rate, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_rate_near() failed %d\n", rc);
  fprintf(stdout, "sample rate %u\n", args.audio_rate);
  snd_pcm_uframes_t frames;
  snd_pcm_uframes_t *framesPtr = &frames;
  *framesPtr = args.audio_period;
  rc = snd_pcm_hw_params_set_period_size_near(handle, params, framesPtr, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_period_size_near() failed %d\n", rc);
  fprintf(stdout, "period size %lu\n", *framesPtr);
  //fprintf(stdout, "frequency response %lu - %d\n", rate / (2 * *framesPtr), rate / 2);
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: (%d), %s\n", rc, snd_strerror(rc));
    exit(1);
  }
  snd_pcm_hw_params_get_period_size(params, framesPtr, NULL);
  size = *framesPtr * 2 * args.audio_channels; /* 2 bytes/sample, 2 channels */
  fprintf(stdout, "buffer size %d\n", size);
  *bufferPtr = (char *) malloc(size);
  rc = snd_pcm_hw_params_get_period_time(params, &val, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_get_period_time() failed %d\n", rc);
  return handle;
}



double *hanning(int N) {
  int i;
  double *window = (double *) malloc(sizeof(double) * N);
  if (verbose) fprintf(stdout, "hanning: ");
  for (i = 0; i < N; i++) {
    window[i] = 0.5 * (1 - cos(2 * M_PI * i / N));
    if (verbose) fprintf(stdout, "%f ", window[i]);
  }
  if (verbose) fprintf(stdout, "\n");
  return window;
}


// period p 1024, rate r 44100, bus b 1, address a 0x40, pwm freq f 200, audio device d default, mode m level (spectrum)
void process_args(int argc, char **argv) {
  // default values
  args.mode = 1;
  args.audio_device = "default";
  args.audio_period = 1024;
  args.audio_rate = 44100;
  args.audio_channels = 2;
  args.pwm_bus = 1;
  args.pwm_addr = 0x40;
  args.pwm_freq = 200;
  args.pwm_debug = false;

  opterr = 0;
  int c;
  while ((c = getopt(argc, argv, "m:d:p:r:c:b:a:f:vD")) != -1) {
    switch (c) {
      case 'm':
        args.mode = 0;
        if (strcmp(optarg, "level") == 0) args.mode = 1;
        else if (strcmp(optarg, "spectrum") == 0) args.mode = 2;
        if (args.mode == 0) {
          fprintf(stderr, "Illegal mode %s\n", optarg);
          exit(-1);
        } // if
        break;
      case 'd':
        args.audio_device = optarg;
        break;
      case 'p':
        args.audio_period = atoi(optarg);
        break;
      case 'r':
        args.audio_rate = atoi(optarg);
        break;
      case 'c':
        args.audio_channels = atoi(optarg);
        break;
      case 'b':
        args.pwm_bus = atoi(optarg);
        break;
      case 'a':
        args.pwm_addr = atoi(optarg);
        break;
      case 'f':
        args.pwm_freq = atoi(optarg);
        break;
      case 'v':
        verbose = true;
        break;
      case 'D':
        args.pwm_debug = true;
        break;
      case '?':
        fprintf(stderr, "problem\n");
        exit(-1);
      default:
        fprintf(stderr, "optopt '%c'\n", optopt);
        abort();
    } //switch
  } // while
  // sanity checks
  if (args.mode == 1 && args.audio_period > 45) {
    fprintf(stdout, "'levels' mode works best with a short period like 45\n");
  } // if mode && period
} // process_args




int main(int argc, char **argv) {
  process_args(argc, argv);

  //fprintf(stdout, "quickstart %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  signal(SIGINT, intHandler);

  // ALSA init
  handle = initALSA(args, &buffer);

  // libPCA9685 init
  fd = initPCA9685(args);

  // fftw init
  int N = args.audio_period;
  fftw_plan p;
  fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  double in[N];
  p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
  double *han = hanning(N);

  int average = 0;
  int minValue = 32000;
  int rc = 64;
  rc = 1024;
  while (1) {
    rc = snd_pcm_readi(handle, buffer, args.audio_period);
    if (rc == -EPIPE) {
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != (int) args.audio_period) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } else {

      if (args.mode == 1) {
        // find the min and max value
        int sample;
        long tmp;
        int minSample=32000;
        int maxSample=-32000;
        // process all frames recorded
        unsigned int i;
        for (i = 0; i < args.audio_period; i++) {
          tmp = (long) ((char*) buffer)[2 * args.audio_channels * i + 1] << 8 | ((char*) buffer)[2 * args.audio_channels * i];
          if (tmp < 32768) sample = tmp;
          else sample = tmp - 65536;
          if (sample > maxSample) {
            maxSample = sample;
          }
          if (sample < minSample) {
            minSample = sample;
          } // if sample
        } // for i
        // intensity-based value
        int intensity_value = maxSample - minSample;
        if (verbose) fprintf(stdout, "intensity %d\n", intensity_value);

        // minValue is the smallest value seen, use for offset
        if (intensity_value < minValue) {
          minValue = intensity_value;
        }
        intensity_value -= minValue;
        if (verbose) fprintf(stdout, "adjusted intensity %d\n", intensity_value);

        // modified moving average for smoothing
        average = (intensity_value + 2 * average) / 3;
        if (verbose) fprintf(stdout, "average %d\n", average);
        int ratio = 100 * (average) / (1000);
        if (ratio < 0) ratio = 0;
        if (ratio > 100) ratio = 100;
        if (verbose) fprintf(stdout, "ratio %d\n", ratio);
        int display = ratio / 100.0 * _PCA9685_MAXVAL;
        if (verbose) fprintf(stdout, "display %d\n", display);
        if (verbose) fprintf(stdout, "%d %d %d\n", args.audio_period, intensity_value, display);
        // update the pwms
        PCA9685_setAllPWM(fd, args.pwm_addr, 0, display);
      }

      else if (args.mode == 2) {
        // window for fftw
        unsigned int i;
        long tmp;
        int sample;
        for (i = 0; i < args.audio_period; i++) {
          tmp = (long) ((char*) buffer)[2 * args.audio_channels * i + 1] << 8 | ((char*) buffer)[2 * args.audio_channels * i];
          if (tmp < 32768) sample = tmp;
          else sample = tmp - 65536;
          in[i] = (double) sample * han[i];
        } // for i

        // fftw
        fftw_execute(p);
        unsigned int pwmoff[16];
        unsigned int minbin = 11;
        for (i = 0; i < (args.audio_period / 2) + 1; i++) {
          if (i >= minbin && i <= minbin+15) {
            // normalize by the number of frames in a period and the hanning factor
            double mag = 2.0 * sqrtf(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / args.audio_period;
            double amp = 20 * log10f(mag);
            if (verbose) fprintf(stdout, "%3d ", (int) amp);
            float cutoff = 2;
            if (amp > cutoff) {
              double val = pow(amp - cutoff, 3);
              if (val > 4096) val = 4096;
              int alpha = 10;
              pwmoff[i-minbin] = (unsigned int) ((alpha-1) * pwmoff[i-minbin] + val) / alpha;
            } else {
              pwmoff[i-minbin] = 0;
            } // if amp
          } // if i
        } // for i
        if (verbose) fprintf(stdout, "\n");
        // update the pwms
        unsigned int pwmon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        PCA9685_setPWMVals(fd, args.pwm_addr, pwmon, pwmoff);
      } // if mode 2
    } // else good audio read
  } // while 1
} // main
