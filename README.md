## DVB direct: DVB easy handling tools for Linux

Easily handle DVB captures from the command line, without thousands of parameters; in fact easier
than using many desktop apps.

### 'mpegts' python script

#### Capabilities

* Easily determine the channels in a captured file
* Show the channels information
* Play, save or compress a separate program (including all audio channels and dvb subtitles)

#### Requirements

* python 3.4+
* ffprobe, ffplay, ffmpeg
* no technical skills

### Capturing

A dedicated tool to properly capture the DVB signal is planned. Meanwhile, to get the MPEGTS files
you may use e.g. dvbv5-zap, despite it needs configuration and it is a bit weird for this purpose
(requires to select one specific channel even to record all of them):

$ dvbv5-zap -P -r "a specific channel name" -o allchannels.mts

The generated file will record all the channels multiplexed in the same transponder. So you can
record and/or watch several programs at a time.
