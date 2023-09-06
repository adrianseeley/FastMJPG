Taken Sep 6 2023 from 6d08c14c50af34d45450edacd7ee5b7a69d93d26

+ The delta is a measure from the time the image data is fully received from the video capture device, until after the image has been rendered to the screen.
+ This does not involve the refresh rate of the monitor, only when the graphics call has completed to render the image to the screen.
+ GStreamer doesn't seem to allow accessing the v4l2 capture timestamp directly which would be more accurate, so this is the best option available.

```sh
gstreamer:

delta: 22045 useconds, frames: 150
delta: 22013 useconds, frames: 300
delta: 21798 useconds, frames: 450
delta: 21882 useconds, frames: 600
delta: 21899 useconds, frames: 750
delta: 21811 useconds, frames: 900
delta: 21853 useconds, frames: 1050
delta: 21870 useconds, frames: 1200
delta: 21808 useconds, frames: 1350
delta: 21841 useconds, frames: 1500
delta: 21854 useconds, frames: 1650
delta: 21815 useconds, frames: 1800
```

```sh
fastmjpg:

delta: 4545 useconds, frames: 150
delta: 4539 useconds, frames: 300
delta: 4570 useconds, frames: 450
delta: 4601 useconds, frames: 600
delta: 4608 useconds, frames: 750
delta: 4606 useconds, frames: 900
delta: 4612 useconds, frames: 1050
delta: 4634 useconds, frames: 1200
delta: 4644 useconds, frames: 1350
delta: 4649 useconds, frames: 1500
delta: 4671 useconds, frames: 1650
delta: 4681 useconds, frames: 1800
```