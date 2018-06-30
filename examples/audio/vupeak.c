#include "vupeak.h"

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <PCA9685.h>
#include <signal.h>
#include <fftw3.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "config.h"

// globals, defined here only for intHandler() to cleanup
audiopwm args;
int fd;
char *buffer;
snd_pcm_t *handle;
// verbosity flag
bool verbose = false;


unsigned Microseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec*1000000 + ts.tv_nsec/1000;
}


void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(fd, args.pwm_addr, _PCA9685_MINVAL, _PCA9685_MINVAL);

  // cleanup alsa
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  //free(buffer);

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
  int M = N % 2 == 0 ? N / 2 : (N + 1) / 2;
  if (verbose) fprintf(stdout, "hanning %d %d\n", N, M);
  for (i = 0; i < M; i++) {
    window[i] = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
    window[N - i - 1] = window[i];
    if (verbose) printf("%d: %f  %d: %f\n", i, window[i], N - i - 1, window[N - i - 1]);
  }
  if (verbose) {
    for (i = 0; i < N; i++) {
      fprintf(stdout, "%f ", window[i]);
    }
    fprintf(stdout, "\n");
  }
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
  args.pwm_smoothing = 10;

  opterr = 0;
  int c;
  while ((c = getopt(argc, argv, "m:d:p:r:c:b:a:f:vDs:")) != -1) {
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
      case 's':
        args.pwm_smoothing = atoi(optarg);
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
  if (args.mode == 1) {
    fprintf(stdout, "'level' mode suggested args: -p 256 -r 192000 -s 1\n");
    fprintf(stdout, "'level' mode suggested line level input volume at 100%%\n");
  } // if mode && period
  else if (args.mode == 2) {
    fprintf(stdout, "'spectrum' mode suggested args: -p 1024 -r 44100 -s 1\n");
    fprintf(stdout, "'spectrum' mode suggested cut line level input volume to 50%%\n");
  } // if mode && period
} // process_args




int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  fprintf(stdout, "vupeak %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  process_args(argc, argv);

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
  // disabled hanning, uncomment here and where used to use
  //double *han = hanning(N);

  int average = 0;
  int minValue = 32000;
  int maxValue = -32000;
  int rc = 64;
  rc = 1024;

  // bass 44100 1024:
  unsigned int bins[16] = {0,0,1, 0,2,2, 0,3,0, 4,4,0, 20,0,0};
  unsigned int binwidths[16] = {0,0,1, 0,1,1, 0,1,0, 16,16,0, 20,0,0};

  double mins[16] = { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100 };
  double maxs[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  { int i;
    for (i = 0; i < 16; i++) {
      if (verbose) printf("%d %f - %f\n", i, mins[i], maxs[i]);
    }
  }
  //unsigned int bins[16] = {0,0,2, 0,3,3, 0,4,0, 5,5,0, 6,0,0};
  int loop = 0;
  unsigned int b = Microseconds();
  printf("  ms     hz\n");
  while (1) {
    unsigned int a = Microseconds();
    if (a - b > 3000000) {
      unsigned int diff = a - b;
      double ms = (double) diff / 1000.0 / loop;
      printf("%d %0.2f %0.2f  ", loop, ms, 1000.0/ms);
      printf("%0.2f-%0.2f ", mins[2], maxs[2]);
      printf("%0.2f-%0.2f ", mins[4], maxs[4]);
      printf("%0.2f-%0.2f ", mins[7], maxs[7]);
      printf("%0.2f-%0.2f ", mins[9], maxs[9]);
      printf("%0.2f-%0.2f ", mins[12], maxs[12]);
      printf("\n");
      b = a;
      loop = 0;
    }
    loop++;
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
        int maxSample=-32000;
        int minSample=32000;

        // process all frames recorded
        unsigned int frame;
        for (frame = 0; frame < args.audio_period; frame += 2) {
          tmp = (long) ((char*) buffer)[2 * args.audio_channels * frame + 1] << 8 | ((char*) buffer)[2 * args.audio_channels * frame];
          if (tmp < 32768) sample = tmp;
          else sample = tmp - 65536;
          if (sample > maxSample) {
            maxSample = sample;
          } // if sample >
          if (sample < minSample) {
            minSample = sample;
          } // if sample <
        } // for frame

        // intensity-based value
        int intensity_value = maxSample - minSample;

        // minValue is the smallest value seen, use for offset
        if (intensity_value < minValue) {
          minValue = intensity_value;
        }
        if (intensity_value > maxValue) {
          maxValue = intensity_value;
        }
        intensity_value -= minValue;

        // modified moving average for smoothing
        int alpha = args.pwm_smoothing;
        average = (intensity_value + (alpha-1) * average) / alpha;

        int ratio = 100.0 * average / (maxValue - minValue);
        ratio = (ratio / 10.0) * (ratio / 10.0);
        ratio = (ratio / 10.0) * (ratio / 10.0);
        if (ratio < 0) ratio = 0;
        if (ratio > 100) ratio = 100;

        int display = ratio / 100.0 * _PCA9685_MAXVAL;
        if (verbose) fprintf(stdout, "%d %d\n", intensity_value, ratio);

        // update the pwms
        PCA9685_setAllPWM(fd, args.pwm_addr, 0, display);
      }

      else if (args.mode == 2) {
        // window for fftw
        unsigned int frame;
        long tmp;
        int sample;
        for (frame = 0; frame < args.audio_period; frame++) {
          tmp = (long) ((char*) buffer)[2 * args.audio_channels * frame + 1] << 8 | ((char*) buffer)[2 * args.audio_channels * frame];
          if (tmp < 32768) sample = tmp;
          else sample = tmp - 65536;
          // apply hanning window
          in[frame] = (double) sample;
          //in[frame] = in[frame] * han[frame];
        } // for frame

        // fftw
        fftw_execute(p);
        unsigned int pwmoff[16];
        int pwmindex;
        for (pwmindex = 0; pwmindex < 16; pwmindex++) {
            // normalize by the number of frames in a period and the hanning factor
            unsigned int binindex = bins[pwmindex];
            unsigned int width = binwidths[pwmindex];
            //if (binindex == 0) {
              //pwmoff[pwmindex] = 0;
              //continue;
            //} // if index
            unsigned int j;
            double amp = 0;
            int ampbinindex = binindex;
            for (j = 0; j < width; j++) {
              double mag = 2.0 * sqrtf(out[binindex + j][0] * out[binindex + j][0] + out[binindex + j][1] * out[binindex + j][1]) / args.audio_period;
              double thisamp = 20 * log10f(mag);
              //amp += thisamp;
              if (thisamp > amp) {
                amp = thisamp;
                ampbinindex = binindex + j;
              } // if thisamp
            } // for j
            //amp /= width;
            int a = 3;
            //if (verbose) printf("check: %d amp %f mins[%d] %f maxs[%d] %f\n", ampbinindex, amp, pwmindex, mins[pwmindex], pwmindex, maxs[pwmindex]);
            if (verbose) printf("check: amp %f mins[%d] %f maxs[%d] %f\n", amp, pwmindex, mins[pwmindex], pwmindex, maxs[pwmindex]);
            if (amp > maxs[pwmindex]) {
              if (verbose) printf("maxs[%d] %f amp %f newmax %f\n", pwmindex, maxs[pwmindex], amp, ((a - 1) * maxs[pwmindex] + amp) / a);
              maxs[pwmindex] = ((a - 1) * maxs[pwmindex] + amp) / a;
            }
            if (amp < mins[pwmindex]) {
              if (verbose) printf("mins[%d] %f amp %f newmin %f\n", pwmindex, mins[pwmindex], amp, ((a - 1) * mins[pwmindex] + amp) / a);
              mins[pwmindex] = ((a - 1) * mins[pwmindex] + amp) / a;
            }
            if (mins[pwmindex] < 0) {
              mins[pwmindex] = 0;
            }
            int minsize = 30 - binindex / 2 > 0 ? 30 - binindex / 2 : 1;
            if (maxs[pwmindex] - mins[pwmindex] < minsize) {
              maxs[pwmindex] = mins[pwmindex] + minsize;
            } // if maxs-mins
            if (verbose) fprintf(stdout, "%2d:%3d %f-%f\n", ampbinindex, (int) amp, mins[pwmindex], maxs[pwmindex]);
            if (amp > mins[pwmindex]) {
              double ratio = (amp - mins[pwmindex]) / (maxs[pwmindex] - mins[pwmindex]);
              if (ratio > 1.0) ratio = 1.0;
              ratio *= ratio;
              ratio *= ratio;
              ratio *= ratio;
              ratio *= ratio;
              ratio *= ratio;
              double val = _PCA9685_MAXVAL * ratio;
              //fprintf(stdout, "%f %f\n", ratio, val);
              if (val > 4096) val = 4096;
              int alpha = args.pwm_smoothing;
              double scaled = val < pwmoff[pwmindex] ? ((alpha - 1) * pwmoff[pwmindex] + val) / alpha : val;
              pwmoff[pwmindex] = scaled;
            } else {
              pwmoff[pwmindex] = 0;
            } // if amp
        } // for i
        if (verbose) fprintf(stdout, "\n");
        // update the pwms
        unsigned int pwmon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        PCA9685_setPWMVals(fd, args.pwm_addr, pwmon, pwmoff);
        { int pwmindex;
          for (pwmindex = 0; pwmindex < 16; pwmindex++) {
            mins[pwmindex] += 0.01;
            maxs[pwmindex] -= 0.1;
          } // for pwmindex
        } // context
      } // if mode 2
    } // else good audio read
  } // while 1
} // main
