= Audio analysis example for libPCA9685

== Dependencies
sudo apt-get install libasound2
sudo apt-get install libasound2-dev

== Default audio
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
