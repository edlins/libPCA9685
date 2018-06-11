Audio analysis example for libPCA9685

DEPENDENCIES

```
$ sudo apt-get install libasound2
$ sudo apt-get install libasound2-dev
$ sudo apt-get install libfftw3-3
```

DEFAULT AUDIO

On a debian system, to use the first USB sound card as the default,
create a ~/.asounrc with:
```
pcm.!default {
        type plug
        slave {
                pcm "hw:1,0"
        }
}

ctl.!default {
        type hw
        card 1
}
```

NOTES

https://soundcloud.com/subquakeaudio/frequency-dreams-meditate-lp
Works well for such "sparse" music with space between tones:
```
minbin = 2;
gap = 3;
cutoff = 3;
alpha = 2;
```

Then:
```
vupeak -m spectrum -p 1024 -r 44100
```

TODO

- Make gap logarithmic
- Smarter scaling instead of pow()
- Tune for shitty music like Skrillex..  :cry:

