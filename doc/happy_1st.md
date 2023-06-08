# NanoShell - The first year

The NanoShell operating system is turning 1 year old today. (counting from the first commit in [this repository](https://github.com/iProgramMC/NanoShellOS) ).
A part of the code had been around for a while before that, the project having started on December 3rd, 2021 ([source](https://github.com/iProgramMC/OSProject)), however,
I think its birthday is when the code was published. You don't consider a person's birthday the date that they were conceived, after all. :)

Here's a little history of what the OS' development looked like. It was an arduous journey, but I think one that paid off. There's still more coming, though!


The images are taken from the [images](https://github.com/iProgramMC/NanoShellOS/blob/master/images) directory from the root of this repository.


**2021-12-15**: The first ever screenshot of what became NanoShell.

First ELF test, simple memory allocator and paging.

Conveniently blocked out by the cyan background "hello world!" text is my name, iProgramInCpp. So really, it said "iProgramInCpp's Operating System V0.10".

<img src="/images/2021_12_15 191947.png"/>

Another screenshot on the same day also demonstrates the keyboard input.
<img src="/images/2021_12_15 211157.png"/>

**2021-12-25**: Added a simple shell, getting into the nitty gritty of OS development.

I replaced the old handmade font (which was meant to resemble Paper Mario 64's font) with a font called [Tamsyn](http://www.fial.com/~scott/tamsyn-font).
It looks very nice indeed.

<img src="/images/2021_12_25 190026.png"/>

The version number is meant to resemble a [Windows NT beta build](https://www.betaarchive.com/imageupload/1296964043.or.66965.png)
([context](https://www.betaarchive.com/forum/viewtopic.php?t=16721&start=175)).

**2021-12-26**: The start of the graphics framework.

I had decided to get started with the graphics framework early on. It was quite slow, but improvements did come over time.
<img src="/images/2021_12_26 220703.png"/>

**2021-12-27**: Multitasking!

Nothing to say here. 1024 threads all racing for their own little column on the screen. Quite satisfying to watch, although the fact I was doing HLT instead of
implementing proper task yielding and pausing did lead to slowdowns. It's a puny single core OS after all.
<img src="/images/2021_12_27 194803.png"/>

**2021-12-30**: NanoShell booting into the GUI for the first time.

Here we have a couple draggable windows. The text is drawn character by character as `VidTextOut` didn't yet exist.

<img src="/images/2021_12_30_22_13_01.png"/>

It was a data race-y mess, having crappy locks for everything from the graphics subsystem. Window dragging was done within the mouse interrupt, so it pretty much
ground everything to a halt while dragging large windows... it was a mess. It would be sorted eventually.

**2022-01-02**: It's 2022!

This screenshot big leaps in the window system. I added widgets, icons, and Scribble, a simple program where you can paint by dragging with the mouse. Also, a new homemade font.

All windows have a shadow to them, making them look 3D-ish.  This would eventually be removed, instead opting for a more Windows-y appearance.
The inspiration for this appearance came from [a concept on the OSDev forums](https://forum.osdev.org/viewtopic.php?f=11&t=30806). So is the font, however, that actually stuck around :)
I am no expert in typography, so the font has had some changes throughout its life span, but it stayed mostly the same.

<img src="/images/2022_01_02_13_16_15.png"/>

**2022-01-16**: File system, more polish, and our first windowed program!

I polished up the UI a little bit. The buttons now look like Windows 3.1, there's no longer that huge shadow, and we can run executables from the VFS
using the new Cabinet. The NanoShell Version Applet is loaded from the VFS.

<img src="/images/2022_01_16_18_05_20.png"/>

**2022-01-19**: This repository's publication! Also, a less than perfect FAT32 driver, and a working windowed shell!

<img src="/images/2022_01_19_13_33_49.png"/>
<img src="/images/2022_01_19_15_45_49.png"/>

Err, yeah. Not looking great.

<img src="/images/2022_01_19_22_08_29.png"/>
Also, a working windowed shell! It's less than perfect, but for now it works well enough.

**2022-01-31**: Minimizable windows, and DOOM!

<img src="/images/2022_01_29_17_21_34.png"/>

Those free-floating icons above the launcher are minimized windows. You could right click them to restore them.

<img src="/images/2022_01_31_18_48_49.png"/>

You can now commit gross acts of murder in NanoShell.

**2022-02-06**: It's-a-me, Mario!

I ported a 'portable' NES emulator to NanoShell shortly after DOOM, and it works like a charm! The reason it looks like that is because I modded SMB1 to look like Super Mario world, a little bit.

The UI has been refined a little bit, titles are now centered, the clock shows the actual time, and more.

<img src="/images/2022_02_06_16_32_42.png"/>

**2022-03-08**: UI polish.

A running theme right now, I've polished the UI once again. Now it looks like a blend of Windows 3.1 (centered titles, buttons), 95 (the 3D window border) and 2000 (gradient title bars).

About resizing windows, to this day it is done by holding ALT while dragging the title bar.

<img src="/images/2022_03_08_17_48_06.png"/>

**2022-04-03**: Task bar moved to the bottom of the screen.

This change would not last very long.

<img src="/images/2022_04_03_21_28_10.png"/>

**2022-05-15**: More features!

This screenshot demonstrates a couple of new programs: the visual builder (Codename V-Builder), the magnifier and the sticky notes.

<img src="/images/2022_05_15_16_56_35.png"/>

###### At this point, due to me getting carried away with other projects, I kind of lost motivation. However, I did one change during that time to the memory manager...
**2022-08-31**: Memory manager overhaul!

In short, the memory manager that I did have in NanoShell had very bad shortcomings which needed to be treated fast. So I decided to just rewrite
the whole thing.

I did it in [an experimental split-off repository](https://github.com/iProgramMC/NanoShellExperimental), and then
[merged](https://github.com/iProgramMC/NanoShellOS/commit/0af23f9cd1d8d579d2ff47f7dc914e9eec3f711c) all the changes into the main line repository.

I've got nothing to show, however the experimental repository is left pretty much as it was (I tried a revamp of the file system in there but decided to do it in the main repository).

**2022-11-07**: The Great Comeback

For the great comeback I started by adding basic ANSI code support, and then ported [NyanCat](https://github.com/klange/nyancat) to test it out. It works like a charm here :)

<img src="/images/2022_11_07_19_35_35.png"/>

After that, the OS underwent one of the biggest overhauls it's ever seen -- I replaced the old cranky FS with new file system code. I added Ext2 file system support, and (temporarily?)
got rid of FAT32 support. I think this was a good change however, as the file system code is some of the cleanest in NanoShell to date.

**2022-12-24**: Table view, and a wave file player!

<img src="/images/2022_12_24 23_25_01.png"/>

I added a simple wave file player which works by piping audio through to an audio device (right now, it's hardcoded to be the Sound Blaster 16 card.)

The table view widget wasn't that hard to implement, however it is quite big in terms of its memory usage. I am planning to remedy that soon.

**2022-12-29**: TCC has been ported.

NanoShell can now compile its own executables and run them, all through TCC. It was quite hard to port seeing as I didn't have all the basic C functions ready, however,
it's all worth it in the end.

<img src="/images/2022_12_29 18_10_54.png"/>

**2022-12-31**: Fireworks!

In the last hours of 2022 I implemented some fireworks that take over the screen. It is satisfying to watch after the real fireworks have died down.

<img src="/images/2022_12_31_23_47_05.png"/>


And with that, this little screenshot tour of the first year of NanoShell is over. Let's see what 2023 has to bring!

I've also been working on a [64-bit NanoShell](https://github.com/iProgramMC/NanoShell64), however it's in the experimental phase right now and this NanoShell still has interest from me.


See you in 2024!

*iProgramInCpp, 2023-01-19*
