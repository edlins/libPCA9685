/*

Example 4 - Simple sound recording

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.

*/

#define CHANNELS 2
#define FRAMES 512

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

int main(int argc, char **argv) {
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  snd_pcm_uframes_t frames;
  char *buffer;
  
  /* Open PCM device for recording (capture). */
  char *dev = "default";
  if (argc > 1) dev = argv[1];
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
  val = 44100;
  rc = snd_pcm_hw_params_set_rate_near(handle, params, &val, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_rate_near() failed %d\n", rc);
  }
  fprintf(stdout, "set_rate_near val %u\n", val);

  /* Set period size to 32 frames. */
  frames = FRAMES;
  fprintf(stdout, "frames %lu\n", frames);
  rc = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_set_period_size_near() failed %d\n", rc);
  }
  fprintf(stdout, "frames %lu\n", frames);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: (%d), %s\n", rc, snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, NULL);
  size = frames * 2 * CHANNELS; /* 2 bytes/sample, 2 channels */
  buffer = (char *) malloc(size);

  /* We want to loop for 5 seconds */
  rc = snd_pcm_hw_params_get_period_time(params, &val, NULL);
  if (rc < 0) {
    fprintf(stderr, "snd_pcm_hw_params_get_period_time() failed %d\n", rc);
  }
  loops = 5000000 / val;

  while (loops > 0) {
    loops--;
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    }
    /* rc = number of frames read */
    int i;
    int maxVal = 0;
    int sample;
    long tmp;
    for (i = 0; i < rc; i += 1) {
      /* 1 frame = 4 bytes = left LSB, left MSB, right LSB, right MSB */
      /* mic is mono so only use the first two bytes of each frame */
      tmp = (long)buffer[4*i+1] << 8 | buffer[4*i];
      if (tmp < 32768) sample = tmp;
      else sample = tmp - 65536;
      //fprintf(stdout, "%d ", sample);
      if (sample > maxVal) maxVal = sample;
    }
    fprintf(stdout, "%d ", maxVal);
    for (i = 0; i < maxVal / 8; i++) {
      fprintf(stdout, "X");
    }
    fprintf(stdout, "*\n");
    //rc = write(1, buffer, size);
    //if (rc != size)
      //fprintf(stderr, "short write: wrote %d bytes\n", rc);
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
