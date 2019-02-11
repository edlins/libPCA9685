/*

Example 4 - Simple sound recording

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#define SAMPLERATE 44100
#define BYTES 2
#define CHANNELS 1
#define FRAMESIZE BYTES * CHANNELS
#define PERIODSIZE 1024

#include <alsa/asoundlib.h>
#include <fftw3.h>

void manipulate(int N, fftw_complex* out) {
  for (int i = 0; i < N/2; i++) {
    if (i == 10) fprintf(stderr, "%.0f ", out[i][0]);
    out[i][1] = 0;
  } // for i
} // manipulate

int main(int argc, char **argv) {
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  /* Open PCM device for recording (capture). */
  char *dev = argv[1];
  rc = snd_pcm_open(&handle, dev, SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr, "unable to open pcm device '%s': %s\n", dev, snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

  /* 8 or 16 bit */
  if (BYTES == 1) snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S8);
  else if (BYTES == 2) snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

  snd_pcm_hw_params_set_channels(handle, params, CHANNELS);

  /* sampling rate 4000 - 192000 */
  val = SAMPLERATE;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
  fprintf(stderr, "vocrec sample rate %d\n", val);

  /* Set period size in frames. */
  frames = PERIODSIZE;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
  fprintf(stderr, "vocrec period size %lu\n", frames);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  size = frames * BYTES * CHANNELS;
  buffer = (char *) malloc(size);

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  loops = 5000000 / val;

  int N = frames;
  fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  fftw_plan p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_plan pi = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

  while (loops > 0) {
    //loops--;
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from read: %s\n",
              snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    }
    for (unsigned int i = 0; i < frames; i++) {
      // extract a signed int from the unsigned 2-byte first channel of the frame
      long tmp = (long) ((char*) buffer)[BYTES * CHANNELS * i];
      if (BYTES == 2) tmp |= (long) ((char*) buffer)[BYTES * CHANNELS * i + 1] << 8;
      if (BYTES == 1 && tmp >= 128) tmp = tmp - 256;
      else if (BYTES == 2 && tmp >= 32768) tmp = tmp - 65536;
      // insert the sample into the complex input array, zeroing the imaginary part
      in[i][0] = (double) tmp;
      in[i][1] = 0.0;
      //if (i == 2) {
        //fprintf(stderr, "%.0f  ", in[i][0]);
      //}
    }
    fftw_execute(p);
    manipulate(N, out);
    fftw_execute(pi);
    for (unsigned int i = 0; i < frames; i++) {
      long tmp = (long) in[i][0];
      if (BYTES == 1 && tmp < 0) tmp += 256;
      if (BYTES == 2 && tmp < 0) tmp += 65536;
      tmp /= frames;
      buffer[BYTES * CHANNELS * i] = tmp % 256;
      if (BYTES == 2) buffer[BYTES * CHANNELS * i + 1] = tmp / 256;
      // zero out other channel
      if (CHANNELS == 2) {
        buffer[BYTES * CHANNELS * i + BYTES] = 0;
        if (BYTES == 2) buffer[BYTES * CHANNELS * i + BYTES + 1] = 0;
      }
      //if (i == 2) {
        //fprintf(stderr, "%.0f\n", in[i][0] / frames);
      //}
    }
    rc = write(1, buffer, size);
    if (rc != size)
      fprintf(stderr,
              "short write: wrote %d bytes\n", rc);
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
