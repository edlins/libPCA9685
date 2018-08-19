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
#include <stdint.h>
#include "config.h"

// globals, defined here only for intHandler() to cleanup
audiopwm args;
snd_pcm_t *rechandle;
snd_pcm_t *playhandle;
// factor to standardize smoothing time
double speed_scaler;
unsigned int current;
int loop = 0;
bool autoexpand = true;
bool autocontract = true;
FILE* rectsfh;
FILE* pbtsfh;
FILE* spectrogramfh;
FILE* phasogramfh;
FILE* unwrapphasogramfh;


void zero(char* buf, int len) {
  if (args.verbosity & VZERO) fprintf(stderr, "zero: %d bytes at %p\n", len, buf);
  for (int i = 0; i < len; i++) {
    buf[i] = 0;
  } // for i
}


unsigned Microseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec*1000000 + ts.tv_nsec/1000;
}


void intHandler(int dummy) {
  // turn off all channels
  PCA9685_setAllPWM(args.pwm_fd, args.pwm_addr, _PCA9685_MINVAL, _PCA9685_MINVAL);

  // cleanup alsa
  snd_pcm_drain(rechandle);
  snd_pcm_close(rechandle);
  //free(inbuf);

  // cleanup fftw
  //fftw_destroy_plan(p);
  //fftw_free(in);
  //fftw_free(out);

  exit(dummy);
}

void initPCA9685(void) {
  _PCA9685_DEBUG = args.pwm_debug;
  int pwm_fd = PCA9685_openI2C(args.pwm_bus, args.pwm_addr);
  args.pwm_fd = pwm_fd;
  PCA9685_initPWM(args.pwm_fd, args.pwm_addr, args.pwm_freq);
}


snd_pcm_t* initALSA(int dir, audiopwm *args, char **bufferPtr) {
  int rc;
  snd_pcm_t* handle;
  snd_pcm_hw_params_t* params;

  if (dir == 0) {
    fprintf(stderr, "init capture: ");
    rc = snd_pcm_open(&handle, args->audio_device, SND_PCM_STREAM_CAPTURE, 0);
  } else {
    fprintf(stderr, "init playback: ");
    rc = snd_pcm_open(&handle, args->audio_device, SND_PCM_STREAM_PLAYBACK, 0);
  } // if dir

  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", args->audio_device, snd_strerror(rc));
    exit(1);
  }
  snd_pcm_hw_params_alloca(&params);
  rc = snd_pcm_hw_params_any(handle, params);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_any() failed %d\n", rc);
  rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_access() failed %d\n", rc);
  if (args->audio_bytes == 1) rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S8);
  else if (args->audio_bytes == 2) rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_format() failed %d\n", rc);
  rc = snd_pcm_hw_params_set_channels(handle, params, args->audio_channels);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_channels() failed %d\n", rc);

  rc = snd_pcm_hw_params_set_rate_near(handle, params, &args->audio_rate, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_rate_near() failed %d\n", rc);
  fprintf(stdout, "sample rate %u, ", args->audio_rate);

  rc = snd_pcm_hw_params_set_period_size_near(handle, params, (snd_pcm_uframes_t *) &args->audio_period, NULL);
  if (rc < 0) fprintf(stderr, "snd_pcm_hw_params_set_period_size_near() failed %d\n", rc);
  fprintf(stdout, "audio period %d, ", args->audio_period);

  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: (%d), %s\n", rc, snd_strerror(rc));
    exit(1);
  }
  args->audio_buffer_size = args->audio_period * args->audio_bytes * args->audio_channels;
  *bufferPtr = (char *) malloc(args->audio_buffer_size);
  fprintf(stdout, "audio buffer %p size %d bytes\n", *bufferPtr, args->audio_buffer_size);
  zero(*bufferPtr, args->audio_buffer_size);

  if (dir == 1) {
    snd_pcm_sw_params_t* swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(handle, swparams);
    rc = snd_pcm_sw_params_set_start_threshold(handle, swparams, (snd_pcm_uframes_t) args->fft_hop_period);
    //rc = snd_pcm_sw_params_set_avail_min(handle, swparams, frames);
    if (rc < 0) {
      fprintf(stderr, "unable to set sw start threshold: %s\n", snd_strerror(rc));
      exit(1);
    }
    rc = snd_pcm_sw_params(handle, swparams);
    if (rc < 0) {
      fprintf(stderr, "unable to set sw parameters: %s\n", snd_strerror(rc));
      exit(1);
    }
  } // if recording
  return handle;
}



// hanningz, w[0] = 0, w[N] != 0
double *hanning(int N) {
  int i;
  double *window = (double *) malloc(sizeof(double) * N);
  int M = N % 2 == 0 ? N / 2 : (N + 1) / 2;
  if (args.verbosity & VWIN) fprintf(stdout, "hanning %d %d\n", N, M);
  for (i = 0; i < N; i++) {
    window[i] = 0.5 * (1 - cos(2 * M_PI * i / N));
    if (args.verbosity & VWIN) printf("%d: %f\n", i, window[i]);
  }
  if (args.verbosity & VWIN) {
    for (i = 0; i < N; i++) {
      fprintf(stdout, "%f ", window[i]);
    }
    fprintf(stdout, "\n");
  }
  return window;
}


void process_args(int argc, char **argv) {
  char *usage = "\
Usage: vupeak [-m level|spectrum] [-d audio device] [-n audio fft period]\n\
              [-r audio rate] [-c audio channels] [-H audio hop period] [-B audio bytes] [-P audio playback device]\n\
              [-b pwm bus] [-a pwm address] [-f pwm frequency]\n\
              [-D] [-s pwm smoothing] [-h] [-t] [-w] [-V] [-R] [-v verbosity] [-A]\n\
where\n\
  -m sets the mode of audio processing (spectrum)\n\
  -d sets the audio device (default)\n\
  -P sets the audio playback device (default)\n\
  -n sets the fft period (1024)\n\
  -r sets the audio rate (44100)\n\
  -c sets the audio channels (2)\n\
  -H sets the audio hop period (256)\n\
  -B sets the audio bytes (2)\n\
  -b sets the pwm i2c bus (1)\n\
  -a sets the pwm i2c address (0x40)\n\
  -f sets the pwm frequency (200)\n\
  -D sets debug to true (false)\n\
  -s sets the pwm smoothing (1)\n\
  -h sets the fft hanning to true (false)\n\
  -t sets the test period to true (false)\n\
  -w sets the ascii waterfall to true (false)\n\
  -V sets the vocoder to true (false)\n\
  -R sets robotize to true (false)\n\
  -v sets the verbosity (0x00)\n\
  -A sets the original atan() to true (0)\n\
";

  // default values
  args.mode = 2;
  args.audio_device = "default";
  args.audio_playback_device = "default";
  args.fft_period = 1024;
  args.audio_rate = 44100;
  args.audio_channels = 2;
  args.audio_period = 256;
  args.fft_hop_period = 256;
  args.audio_bytes = 2;
  args.pwm_bus = 1;
  args.pwm_addr = 0x40;
  args.pwm_freq = 200;
  args.pwm_debug = false;
  args.pwm_smoothing = 1;
  args.fft_hanning = false;
  args.test_period = false;
  args.save_fourier = false;
  args.save_timeseries = false;
  args.ascii_waterfall = false;
  args.vocoder = false;
  args.robotize = false;
  args.verbosity = 0x00;
  args.orig_atan = false;

  opterr = 0;
  int c;
  while ((c = getopt(argc, argv, "m:d:P:n:r:c:p:H:B:b:a:f:Ds:htwVRFTv:A")) != -1) {
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
      case 'P':
        args.audio_playback_device = optarg;
        break;
      case 'n':
        args.fft_period = atoi(optarg);
        break;
      case 'r':
        args.audio_rate = atoi(optarg);
        break;
      case 'c':
        args.audio_channels = atoi(optarg);
        break;
      case 'p':
        args.audio_period = atoi(optarg);
        break;
      case 'H':
        args.fft_hop_period = atoi(optarg);
        break;
      case 'B':
        args.audio_bytes = atoi(optarg);
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
        args.verbosity = (int) strtol(optarg, NULL, 16);
        break;
      case 'D':
        args.pwm_debug = true;
        break;
      case 's':
        args.pwm_smoothing = atoi(optarg);
        break;
      case 'h':
        args.fft_hanning = true;
        break;
      case 't':
        args.test_period = true;
        break;
      case 'w':
        args.ascii_waterfall = true;
        break;
      case 'V':
        args.vocoder = true;
        break;
      case 'R':
        args.robotize = true;
        break;
      case 'F':
        args.save_fourier = true;
        break;
      case 'T':
        args.save_timeseries = true;
        break;
      case 'A':
        args.orig_atan = true;
        break;
      case '?':
        fprintf(stdout, "%s", usage);
        exit(0);
      default:
        fprintf(stderr, "optopt '%c'\n", optopt);
        fprintf(stderr, "%s", usage);
        abort();
    } //switch
  } // while

  // sanity checks
  if (args.mode == 1) {
    fprintf(stdout, "'level' mode suggested args: -o 0 -p 256 -r 192000\n");
  } // if mode
  //if (args.fft_hop_period > args.audio_period) {
    //fprintf(stderr, "ERROR: fft hop period cannot be larger than than audio period (yet)\n");
    //exit(-1);
  //} // if hop too large
  if (args.verbosity) {
    fprintf(stderr, "verbosity: ");
    for (int i = 0; i < 16; i++) {
      if (args.verbosity & 1 << i) {
        fprintf(stderr, "0x%02x ", 1 << i);
      } // if bitwise match
    } // for i
    fprintf(stderr, "\n");
  } // if verbosity
} // process_args


char *representation(float val) {
  if (val < 71) return " ";
  if (val < 72) return "-";
  if (val < 73) return ".";
  if (val < 74) return ",";
  if (val < 75) return ":";
  if (val < 76) return ";";
  if (val < 77) return "+";
  if (val < 78) return "=";
  if (val < 79) return "&";
  if (val < 80) return "@";
  return "# ";
}


void level(char* buffer) {
  static int minValue = 32000;
  static int maxValue = -32000;
  static int average = 0;
  // find the min and max value
  int sample;
  long tmp;
  int maxSample=-32000;
  int minSample=32000;

  // process all frames recorded
  unsigned int frame;
  for (frame = 0; frame < args.fft_period; frame++) {
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
  alpha *= speed_scaler;
  average = (intensity_value + (alpha-1) * average) / alpha;

  int ratio = 100.0 * average / (maxValue - minValue);
  ratio = (ratio / 10.0) * (ratio / 10.0);
  ratio = (ratio / 10.0) * (ratio / 10.0);
  if (ratio < 0) ratio = 0;
  if (ratio > 100) ratio = 100;

  int display = ratio / 100.0 * _PCA9685_MAXVAL;
  if (args.verbosity & VPWM) fprintf(stdout, "%d %d\n", intensity_value, ratio);

  // update the pwms
  PCA9685_setAllPWM(args.pwm_fd, args.pwm_addr, 0, display);
} // level


double extract_sample(char* buffer, int frame, int bytes, int channels) {
  // extract a signed int from the unsigned first channel of the frame
  long sample = 0;
  for (int bytenum = bytes - 1; bytenum >= 0; bytenum--) {
    int index = frame * bytes * channels + bytenum;
    uint8_t byteval = (uint8_t) buffer[index];
    sample <<= 8;
    sample |= (byteval & 0xFF);
    //fprintf(stderr, "buffer[%d] = %02x ", index, byteval);
  } // for bytenum
  long value = sample;
  if (value > (1 << 8 * bytes) / 2) value -= 1 << 8 * bytes;
  //fprintf(stderr, "frame %d sample %ld value %ld\n", frame, sample, value);
  if (value == (1 << 8 * bytes) / 2) fprintf(stderr, "clip\n");
  return (double) value;
} // extract sample


void overlap_add_sample(char* buffer, int frame, long value) {
  long prev = extract_sample(buffer, frame, args.audio_bytes, args.audio_channels);
  long orig = value;
  long modified = value;
  int index = args.audio_bytes * args.audio_channels * frame;
  if (args.audio_bytes == 1) {
    if (modified <= 0) modified += 255;
    buffer[index] += modified;
    if (args.audio_channels == 2) {
      buffer[index + 1] = 0;
    }
  } else if (args.audio_bytes == 2) {
    if (modified <= 0) modified += 65535;
    buffer[index] += modified % 256;
    buffer[index + 1] += modified / 256;
    if (args.audio_channels == 2) {
      buffer[index + 2] = 0;
      buffer[index + 3] = 0;
    } // if channels
    if (args.verbosity & VOVERLAPADD) printf("%d %d %d %ld %ld %.0f\n", frame, buffer[index], buffer[index + 1], prev, orig, (double) modified);
  } // if bytes
} // overlap_add_sample


void insert_sample(char* buffer, int frame, long value) {
  //long orig = value;
  long modified = value;
  int index = args.audio_bytes * args.audio_channels * frame;
  if (args.audio_bytes == 1) {
    if (modified <= 0) modified += 255;
    buffer[index] = modified;
    if (args.audio_channels == 2) {
      buffer[index + 1] = 0;
    }
  } else if (args.audio_bytes == 2) {
    if (modified <= 0) modified += 65535;
    buffer[index] = modified % 256;
    buffer[index + 1] = modified / 256;
    if (args.audio_channels == 2) {
      buffer[index + 2] = 0;
      buffer[index + 3] = 0;
    } // if channels
    //if (frame == 0) printf("in %d %d %d %ld %f\n", frame, buffer[index], buffer[index + 1], orig, (double) modified);
  } // if bytes
} // insert sample


void dumpbuffer(fftw_complex* buffer, int frames, int bytes, int channels) {
  for (int i = 0; i < frames; i++) {
    printf("%.0f ", buffer[i][0]);
  } // for i
  printf("\n");
} // dumpbuffer


// real value fftshift needed for zero-phase windows
void fftshift(fftw_complex* vals, int N) {
  for (int i = 0; i < N / 2; i++) {
    // only shift the real components
    // as this should be real-valued data
    double tmp = vals[i][0];
    vals[i][0] = vals[N/2+i][0];
    vals[N/2+i][0] = tmp;
  } // for i
} // fftshift


void vocoder(char* outbuf, fftw_complex* out, fftw_complex* in, fftw_plan pi, double* han) {
  if (args.robotize) {
    for (unsigned int i = 0; i < args.fft_period; i++) {
      out[i][1] = 0;
    } // if robotize
  } // for i

  fftw_execute(pi);
  fftshift(in, args.fft_period);

  FILE* invfftfh;
  FILE* pbfh;
  if (args.test_period) {
    invfftfh = fopen("invfft.dat", "w");
    pbfh = fopen("pb.dat", "w");
    //FILE* pbwinfh = fopen("pbwin.dat", "w");
  } // if testperiod

  static int hop = 0;
  int numhops = args.fft_period / args.fft_hop_period;
  int hopbytes = args.fft_hop_period * args.audio_channels * args.audio_bytes;
  if (args.verbosity & VVOCODER)
    fprintf(stderr, "vocoder: hop %d numhops %d hopbytes %d\n", hop, numhops, hopbytes);

  // if the buffer is full
  if (hop == numhops) {
    if (args.verbosity & VVOCODER) fprintf(stderr, "vocoder: buffer full at hop %d\n", hop);
    // if the buffer is larger than one hop, shift it and zero out last hop
    if (numhops > 1) {
      char* src = outbuf + hopbytes;
      // start is 1 hop in, stop is audio + fft - 1 hop
      // so frames is audio + fft - 2 hops
      int frames = args.audio_period + args.fft_period - 2 * args.fft_hop_period;
      int bytes = frames * args.audio_channels * args.audio_bytes;
      if (args.verbosity & VVOCODER) fprintf(stderr, "vocoder: shift %d from %p to %p\n", bytes, src, outbuf);
      memmove(outbuf, src, bytes);
      if (args.verbosity & VVOCODER) fprintf(stderr, "vocoder: zero %d at %p\n", hopbytes, outbuf + bytes);
      zero(outbuf + bytes, hopbytes);
    } // if numhops
    // reset the hop to the last hop in the buffer
    hop--;
  } // if buffer full

  // init the dest pointer to the hop destination
  char* dest = outbuf + hop * hopbytes;

  // build a single fft period
  // and overlap add it to the buffer
  // may roll beyond audio_period size
  if (args.verbosity & VVOCODER) fprintf(stderr, "vocoder: overlap add %d frames to %p\n", args.fft_period, dest);
  for (unsigned int i = 0; i < args.fft_period; i++) {
    int audio_frame = hop * args.fft_hop_period + i;
    // last inv transform may only partially fit in portion
    // of outbuf that gets written to pcm
    if (i >= args.audio_period) break;
    double descale = in[i][0] / args.fft_period;

    if (args.test_period) {
      fprintf(invfftfh, "%.0f\n", descale);
    } // if test period

    //if (args.fft_hanning) {
      //descale *= han[i];
    //} // if hanning

    //char* dest = outbuf + hop * args.fft_period * args.audio_bytes * args.audio_channels;
    //insert_sample(dest, i, descale);
    overlap_add_sample(dest + audio_frame * args.audio_channels * args.audio_bytes, i, descale);
  } // for i

  if (args.verbosity & VPB) dumpbuffer(outbuf, args.audio_period, args.audio_bytes, args.audio_channels);

  // if we just wrote to the last hop, play the buffer
  if (hop == numhops - 1) {
    if (args.verbosity & VVOCODER) {
      fprintf(stderr, "vocoder: hop %d numhops %d play buffer %p frames %d\n", hop, numhops, outbuf, args.audio_period);
    } // if verbosity
    if (args.test_period) {
      for (int i = 0; i < args.audio_period; i++) {
        fprintf(pbfh, "%.0f\n", extract_sample(outbuf, i, args.audio_bytes, args.audio_channels));
      } // for i
    } // if test period
    if (args.save_timeseries) {
      for (unsigned int i = 0; i < args.audio_period; i++) {
        double sample = extract_sample(outbuf, i, args.audio_bytes, args.audio_channels);
        fprintf(pbtsfh, "%.0f\n", sample);
        if (args.verbosity & VPB) fprintf(stderr, "%.0f ", sample);
      } // for i
      if (args.verbosity &VPB) fprintf(stderr, "\n");
    } // if save timeseries
    int rc = snd_pcm_writei(playhandle, outbuf, args.audio_period);
    if (rc == -EPIPE) {
      // EPIPE means underrun
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(playhandle);
    } else if (rc < 0) {
      fprintf(stderr, "error from writei: %s\n", snd_strerror(rc));
    } else if ((unsigned int) rc != args.audio_period) {
      fprintf(stderr, "short write, write %d frames\n", rc);
    }
  } // if hop
  // increment the hop
  hop++;
} // vocoder


#define MAX_LENGTH 10000

void unwrap(double p[], int N) {
    double dp[MAX_LENGTH];     
    double dps[MAX_LENGTH];    
    double dp_corr[MAX_LENGTH];
    double cumsum[MAX_LENGTH];
    double cutoff = M_PI;               /* default value in matlab */
    int j;

    assert(N <= MAX_LENGTH);
   // incremental phase variation 
   // MATLAB: dp = diff(p, 1, 1);
    for (j = 0; j < N-1; j++) {
      dp[j] = p[j+1] - p[j];
      //if (args.verbosity) fprintf(stderr, "%d dp %f\n", j, dp[j]);
   }
      
   // equivalent phase variation in [-pi, pi]
   // MATLAB: dps = mod(dp+dp,2*pi) - pi;
    for (j = 0; j < N-1; j++) {
      //dps[j] = (dp[j]+M_PI) - floor((dp[j]+M_PI) / (2*M_PI))*(2*M_PI) - M_PI;
      dps[j] = dp[j];
      //if (args.verbosity) fprintf(stderr, "%d dps %f\n", j, dps[j]);
    }

   // preserve variation sign for +pi vs. -pi
   // MATLAB: dps(dps==pi & dp>0,:) = pi;
    for (j = 0; j < N-1; j++) {
      dps[j] = (dp[j]+M_PI) - floor((dp[j]+M_PI) / (2*M_PI))*(2*M_PI) - M_PI;
      //if (args.verbosity) fprintf(stderr, "%d dps %f\n", j, dps[j]);
   }

   // incremental phase correction
   // MATLAB: dp_corr = dps - dp;
    for (j = 0; j < N-1; j++) {
      dp_corr[j] = dps[j] - dp[j];
      //if (args.verbosity) fprintf(stderr, "%d dp_corr %f\n", j, dp_corr[j]);
    }
      
   // Ignore correction when incremental variation is smaller than cutoff
   // MATLAB: dp_corr(abs(dp)<cutoff,:) = 0;
    for (j = 0; j < N-1; j++) {
      if (fabs(dp[j]) < cutoff)
        dp_corr[j] = 0;
      else if (args.verbosity) fprintf(stderr, "%d dp %f >= cutoff %f\n", j, fabs(dp[j]), cutoff);
      if (args.verbosity) fprintf(stderr, "%d dp_corr %f\n", j, dp_corr[j]);
    }

   // Find cumulative sum of deltas
   // MATLAB: cumsum = cumsum(dp_corr, 1);
    cumsum[0] = dp_corr[0];
    for (j = 1; j < N-1; j++) {
      cumsum[j] = cumsum[j-1] + dp_corr[j];
      //if (args.verbosity) fprintf(stderr, "%d cumsum %f\n", j, cumsum[j]);
    }

   // Integrate corrections and add to P to produce smoothed phase values
   // MATLAB: p(2:m,:) = p(2:m,:) + cumsum(dp_corr,1);
    for (j = 1; j < N; j++) {
      p[j] += cumsum[j-1];
      //if (args.verbosity) fprintf(stderr, "%d p %f\n", j, p[j]);
    }
}


void unwrap2(double w[], int N) {
  float cutoff = 2 * M_PI;
  float mult = 1;
  //for (int j = 0; j < 2; j++) {
  for (int i = 1; i < N - 1; i++) {
    double dp = w[i + 1] - w[i];
    if (args.verbosity && i < 100) fprintf(stderr, "%f %f %f\n", w[i], w[i + 1], dp);
    while (fabs(dp) >= cutoff) {
      if (dp < 0) {
        w[i + 1] += mult * cutoff;
      } else {
        w[i + 1] -= mult * cutoff;
      } // check sign
      if (args.verbosity && i < 100) fprintf(stderr, "-> %f ", w[i + 1]);
      dp = w[i + 1] - w[i];
    } // if discontinuous
    if (args.verbosity && i < 100) fprintf(stderr, "\n");
  } // for i
  //} // for j
  if (args.verbosity) {
    for (int i = 0; i < N; i++) {
      fprintf(stderr, "%f  ", w[i]);
    } // for i
    fprintf(stderr, "\n");
  } // if verbose
}


// looks for jumps > 90 (=~ 180 += 20ish) and corrects with +-180
void unwrap3(double w[], int N) {
  for (int i = 1; i < N; i++) {
    double delta = w[i] - w[i-1];
    if (delta > M_PI / 2) w[i] -= M_PI;
    else if (delta < -M_PI / 2) w[i] += M_PI;
    if (w[i] > M_PI) w[i] -= M_PI;
    else if (w[i] < -M_PI) w[i] += M_PI;
  } // for i
} // unwrap3


fftw_complex* padexpand(fftw_complex* in) {
  fftw_complex* out = (fftw_complex*) malloc(sizeof(fftw_complex) * args.fft_period * 2);
  for (int i = 0; i < args.fft_period * 2; i++) {
    out[i][0] = 0;
    out[i][1] = 0;
  } // for i
  for (int i = 0; i < args.fft_period / 2; i++) {
    out[i][0] = in[i][0];
  } // for i
  for (int i = args.fft_period / 2 + 1; i < args.fft_period; i++) {
    out[i + args.fft_period][0] = in[i][0];
  } // for i
  return out;
} // padexpand


typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);

hsv rgb2hsv(rgb in) {
    hsv         out;
    double      min, max, delta;
    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;
    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;
    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001) {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = 0;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan
    out.h *= 60.0;                              // degrees
    if( out.h < 0.0 )
        out.h += 360.0;
    return out;
}


rgb hsv2rgb(hsv in) {
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;
    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));
    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;
    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}


// noise at N=512 @ 44100 hz ~ 35% output line level
static double noise[16] = {54,50,47,44,40,39,35,35,34,34,31,32,30,28,28,27};
static double maxs[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double hueminval = 240;
double huemaxval = 0;
static double hues[5] = {0,0,0,0,0};
static double maxhues[5] = {0,0,0,0,0};
static double minhues[5] = {360,360,360,360,360};
static double maxvalue[5] = {0,0,0,0,0};
static double minvalue[5] = {255,255,255,255,255};
int statloops = 10;
void spectrum(fftw_complex* fftout, int n) {
  static unsigned int pwmon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int pwmoff[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double relmags[16];
  static int j = 0;
  for (int pwmindex = 0; pwmindex < 16; pwmindex++) {
    double mag = 20.0 * log10f(2.0 * sqrtf(fftout[pwmindex][0] * fftout[pwmindex][0] + fftout[pwmindex][1] * fftout[pwmindex][1]) / n);
    if (mag > maxs[pwmindex]) maxs[pwmindex] = mag;
    double relmag = 0;
    if (mag > noise[pwmindex] && maxs[pwmindex] > noise[pwmindex]) {
      relmag = (mag - noise[pwmindex]) / (maxs[pwmindex] - noise[pwmindex]);
    } // if max > noise
    relmags[pwmindex] = relmag;
    if (j > statloops) {
      //fprintf(stderr, "%.0f-%.0f %.0f %.2f ", noise[pwmindex], maxs[pwmindex], mag, relmag);
      //fprintf(stderr, "%-3.2f ", relmag);
    } // if j
  } // for pwmindex

  for (int headindex = 0; headindex < 5; headindex++) {
    bool threebin = false;
    bool twobin = true;
    if (twobin) {
      int testhead = 0;
      double lowbin = relmags[testhead * 2 + 1];
      double highbin = relmags[testhead * 2 + 2];
      double value = atan2(highbin, lowbin);
      if (value < 0.61) value = 0.61;
      if (value > 0.72) value = 0.72;
      double average = (lowbin + highbin) / 2.0;
      //int rawhue = average * 360;
      //if (rawhue < minhues[headindex]) minhues[headindex] = rawhue;
      //if (rawhue > maxhues[headindex]) maxhues[headindex] = rawhue;
      //int hue = ((double) rawhue - minhues[headindex]) / (maxhues[headindex] - minhues[headindex]) * 360;
      //minavg[headindex] += 0.1;
      //maxavg[headindex] -= 0.1;
      //if (average < 0.75) continue;
      if (value < minvalue[headindex]) minvalue[headindex] = value;
      if (value > maxvalue[headindex]) maxvalue[headindex] = value;
      double relative = (value - minvalue[headindex]) / (maxvalue[headindex] - minvalue[headindex]);
      int hue = hueminval + relative * (huemaxval - hueminval);
      hues[headindex] = (hues[headindex] * 9.0 + hue) / 10;
      hsv outhsv;
      outhsv.h = hues[headindex];
      //outhsv.h = hue;
      outhsv.s = 1.0;
      outhsv.v = average;
      rgb outrgb = hsv2rgb(outhsv);
      pwmoff[headindex * 3 + 0] = outrgb.r * _PCA9685_MAXVAL;
      pwmoff[headindex * 3 + 1] = outrgb.g * _PCA9685_MAXVAL;
      pwmoff[headindex * 3 + 2] = outrgb.b * _PCA9685_MAXVAL;
      //fprintf(stderr, "%d %.2f,%.2f %.2f %.2f %.2f-%.2f=%.2f %.2f %d\n", testhead, lowbin, highbin, average, value, minvalue[headindex], maxvalue[headindex], relative, hues[headindex], hue);
    } // if twobin

    if (threebin) {
      rgb fakergb;
      int testhead = 0;
      if (headindex == 0) testhead = 0;
      else if (headindex == 4) testhead = 1;
      else continue;
      fakergb.r = relmags[testhead * 3 + 0];
      fakergb.g = relmags[testhead * 3 + 1];
      fakergb.b = relmags[testhead * 3 + 2];
      hsv fakehsv = rgb2hsv(fakergb);
      double orighue = fakehsv.h;
      fakehsv.s = 1.0;
      // 120 - 160 for N=512
      if (args.fft_period == 1024) {
        if (testhead == 0) {
          if (fakehsv.h < 160) fakehsv.h = 160;
          if (fakehsv.h > 190) fakehsv.h = 190;
        }
      } else if (args.fft_period = 512) {
        if (testhead == 0) {
          if (fakehsv.h < 120) fakehsv.h = 120;
          if (fakehsv.h > 160) fakehsv.h = 160;
        } else if (testhead == 1) { // D.O.D MIX 006
          if (fakehsv.h < 140) fakehsv.h = 140;
          if (fakehsv.h > 180) fakehsv.h = 180;
        } else if (testhead == 2) { //
          if (fakehsv.h < 140) fakehsv.h = 140;
          if (fakehsv.h > 180) fakehsv.h = 180;
        }
      }
      if (fakehsv.h < minhues[headindex]) minhues[headindex] = fakehsv.h;
      if (fakehsv.h > maxhues[headindex]) maxhues[headindex] = fakehsv.h;
      double relhue = 0;
      if (maxhues[headindex] - minhues[headindex] != 0) {
        relhue = (fakehsv.h - minhues[headindex]) / (maxhues[headindex] - minhues[headindex]);
      } // if not zero
      double outhue = hueminval + relhue * (huemaxval - hueminval);
      hues[headindex] = (outhue + hues[headindex] * 29.0) / 30.0;
      //hues[headindex] = outhue;
      fakehsv.h = (int) hues[headindex];
      rgb outrgb = hsv2rgb(fakehsv);
      pwmoff[headindex * 3] = (int) (outrgb.r * _PCA9685_MAXVAL);
      pwmoff[headindex * 3 + 1] = (int) (outrgb.g * _PCA9685_MAXVAL);
      pwmoff[headindex * 3 + 2] = (int) (outrgb.b * _PCA9685_MAXVAL);
    } // if threebin
    if (j > statloops) {
      //fprintf(stderr, "%3.0f-%3.0f %3.0f %3.0f %3.0f ", minhues[headindex], maxhues[headindex], orighue, outhue, fakehsv.h);
    } // if j
  } // for headindex
  if (j++ > statloops) {
    //fprintf(stderr, "\n");
    j = 0;
  } // if j
  if (PCA9685_setPWMVals(args.pwm_fd, args.pwm_addr, pwmon, pwmoff) == -1) {
    fprintf(stderr, "ERROR: PCA9685_setPWMVals() returned -1, exiting\n");
    exit(-1);
  } // if setPWMVals
} // spectrum


void set_reals(fftw_complex* c, int16_t* r, int n) {
  //fprintf(stderr, "set_reals: c %p r %p n %d\n", c, r, n);
  for (int i = 0; i < n; i++) {
    c[i][0] = r[i];
    c[i][1] = 0;
  } // for i
} // set_reals


void window(fftw_complex* buffer, double* window, int n) {
  //fprintf(stderr, "window: buffer %p window %p n %d\n", buffer, window, n);
  for (int i = 0; i < n; i++) {
    buffer[i][0] *= window[i];
  } // for i
} // window


void hopadvance(int16_t* dest, int16_t* src, int size) {
  //fprintf(stderr, "hopadvance: dest %p src %p size %d\n", dest, src, size);
  memmove(dest, src, size * sizeof(int16_t));
} // shift


void save(FILE* fh, int16_t* buffer, int n) {
  for (int i = 0; i < n; i++) {
    fprintf(fh, "%d\n", buffer[i]);
  } // for i
} // save


void audiorec(snd_pcm_t* recdev, char* recbuf, int n) {
  //fprintf(stderr, "audiorec: recdev %p recbuf %p size %d\n", recdev, recbuf, n);
  bool goodaudio = false;
  while (!goodaudio) {
    int rc = snd_pcm_readi(recdev, recbuf, n);
    if (rc == -EPIPE) {
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(rechandle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != n) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } else {
      goodaudio = true;
    } // if problems
  } // while not good audio
} // audiorec


int main(int argc, char **argv) {
  //setvbuf(stdout, NULL, _IONBF, 0);
  fprintf(stdout, "vupeak %d.%d\n", libPCA9685_VERSION_MAJOR, libPCA9685_VERSION_MINOR);
  process_args(argc, argv);

  signal(SIGINT, intHandler);

  // ALSA init
  char* rb;
  rechandle = initALSA(0, &args, &rb);
  char* pb;
  if (args.vocoder) playhandle = initALSA(1, &args, &pb);

  // libPCA9685 init
  initPCA9685();

  // fftw init
  fftw_complex* fib = (fftw_complex*) malloc(args.fft_period * sizeof(fftw_complex));
  fftw_complex* fob = (fftw_complex*) malloc(args.fft_period * sizeof(fftw_complex));
  int16_t* sb = (int16_t*) malloc(args.fft_period * sizeof(int16_t));
  fftw_plan planfwd = fftw_plan_dft_1d(args.fft_period, fib, fob, FFTW_FORWARD, FFTW_ESTIMATE);
  //fftw_plan planbwd = fftw_plan_dft_1d(args.fft_period, fob, fib, FFTW_BACKWARD, FFTW_ESTIMATE);
  double *han;
  if (args.fft_hanning) han = hanning(args.fft_period);

  if (args.save_fourier) {
    spectrogramfh = fopen("spectrogram.dat", "w");
    phasogramfh = fopen("phasogram.dat", "w");
    unwrapphasogramfh = fopen("unwrapphasogram.dat", "w");
  } // if save fourier
  if (args.save_timeseries) {
    rectsfh = fopen("rects.dat", "w");
    pbtsfh = fopen("pbts.dat", "w");
  } // if save timeseries

  int audio_offset = args.audio_period;
  int fourier_offset = 0;
  while (1) {
    current = Microseconds();

    // if fourier buffer full, transform then shift
    if (fourier_offset == args.fft_period) {
      set_reals(fib, sb, args.fft_period);
      if (args.fft_hanning) window(fib, han, args.fft_period);
      fftshift(fib, args.fft_period);
      fftw_execute(planfwd);
      spectrum(fob, args.fft_period);
      hopadvance(sb, sb + args.fft_hop_period, args.fft_period - args.fft_hop_period);
      fourier_offset -= args.fft_hop_period;
    } // if fourier buffer full

    // if audio buffer empty, read pcm
    if (audio_offset == args.audio_period) {
      audiorec(rechandle, rb, args.audio_period);
      audio_offset = 0;
    } // if audio buffer empty

    // copy a frame
    //fprintf(stderr, "rb %p %d -> sb %p %d\n", rb, audio_offset, sb, fourier_offset);
    double sample = extract_sample(rb, audio_offset, args.audio_bytes, args.audio_channels);
    sb[fourier_offset] = sample;
    if (args.save_timeseries) fprintf(rectsfh, "%.0f\n", sample);

    // increment the offsets for the next frame
    audio_offset++;
    fourier_offset++;
    if (args.test_period && loop >= args.audio_period / args.fft_hop_period) exit(0);
  } // while
} // main
