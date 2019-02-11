/*

Example 3 - Simple sound playback

This example reads standard from input and writes
to the default PCM device for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#define CHANNELS 1
#define BYTES 2
#define FRAMESIZE CHANNELS * BYTES
#define PERIODSIZE 1024
#define SAMPLERATE 44100

#include <alsa/asoundlib.h>

int main(int argc, char **argv) {
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  /* Open PCM device for playback. */
  char *dev = argv[1];
  rc = snd_pcm_open(&handle, dev, SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", dev, snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&hwparams);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, hwparams);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

  /* format */
  if (BYTES == 1) snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S8);
  else if (BYTES == 2) snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE);

  /* channels */
  snd_pcm_hw_params_set_channels(handle, hwparams, CHANNELS);

  /* sampling rate 4000 - 192000 */
  val = SAMPLERATE;
  snd_pcm_hw_params_set_rate_near(handle, hwparams, &val, &dir);
  fprintf(stderr, "vocplay sampling rate %d\n", val);

  /* Set period size */
  frames = PERIODSIZE;
  snd_pcm_hw_params_set_period_size_near(handle, hwparams, &frames, &dir);
  fprintf(stderr, "vocplay period size %lu\n", frames);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, hwparams);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  size = frames * BYTES * CHANNELS;
  buffer = (char *) malloc(size);

  /* start the transfer when the buffer is almost full: */
  /* (buffer_size / avail_min) * avail_min */
  snd_pcm_sw_params_alloca(&swparams);
  snd_pcm_sw_params_current(handle, swparams);
  rc = snd_pcm_sw_params_set_start_threshold(handle, swparams, (snd_pcm_uframes_t) size);
  rc = snd_pcm_sw_params_set_avail_min(handle, swparams, frames);
  if (rc < 0) {
    fprintf(stderr, "unable to set sw start threshold: %s\n", snd_strerror(rc));
    exit(1);
  }
  rc = snd_pcm_sw_params(handle, swparams);
  if (rc < 0) {
    fprintf(stderr, "unable to set sw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(hwparams, &val, &dir);
  /* 5 seconds in microseconds divided by
   * period time */
  loops = 5000000 / val;

  while (loops > 0) {
    //loops--;
    rc = read(0, buffer, size);
    if (rc == 0) {
      fprintf(stderr, "end of file on input\n");
      break;
    } else if (rc != size) {
      fprintf(stderr, "short read: read %d bytes\n", rc);
    }
    rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
      // EPIPE means underrun
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from writei: %s\n", snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr, "short write, write %d frames\n", rc);
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
