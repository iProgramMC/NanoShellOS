# NanoShell - The second year

NanoShell is turning 2 years old today!
This year, I don't have much to show, since I didn't take too many screenshots during this time. So I will talk about
the few screenshots that I've indeed taken! A lot of changes done to the kernel this year were actually behind the curtain,
so that explains the relative scarcity of shots.

See [Happy 1st](happy_1st.md) for the first year.

**2023-02-04**: Stylistic changes!

This image features stylistic changes to the system.  The action buttons at the top right were changed from being actual
controls, the way they were before, to being part of the window frame.  At this point I didn't do the border change, so
all window borders are thin.

This also demonstrates the table widget! It's quite heavy at this point.

<img src="/images/2023_02_04_22_42_59.png">

**2023-02-12**: Custom window borders

Custom window border sizes were also implemented.  This was done by keeping the application from accessing the entire
window -- it only gets the contents of the window to draw in now. This is achieved using some VBEData trickery, where
the application exposed context is actually only part of the full window's frame buffer.

<img src="/images/2023_02_12_21_53_28.png">

**2023-02-23**: Properties menu

This screenshot is very simple.  It demonstrates the properties menu.  Here, I've gone into the properties of Chess.nse,
a program also introduced in 2023.  This information is fetched from the new resource table.

Unfortunately Chess has no AI, so you'll have to take turns playing.

<img src="/images/2023_02_23_12_31_02.png">

**2023-06-11**: An attempt to run Halfix!

[Halfix](https://github.com/nepx/halfix) is a portable x86 emulator written in C99. This seemed really attractive to me,
running _real_ OSes inside of NanoShell? Sign me up!

Unfortunately this project exposed the shortcomings of rolling everything on your own - tons of bugs caused this project
to fail. The OS never booted. It instead threw an interesting blue screen, whose text was actually the colors that used
to be at those pixels before the crash - so you could partly see the Windows logo through it.

This screenshot also shows the recently added CPU and memory views in the task bar.

<img src="/images/2023_06_11_18_11_46.png">

**2023-09-10**: Calm.

This screenshot shows nothing special. I do find it interesting how I haven't yet updated it to say 2019-2023. Strange..

At this point, interest is waning in favor of another OS project, [Boron](https://github.com/iProgramMC/Boron).  This
project is more involved.  I've started it in late August 2023.

Note: I mentioned NanoShell64 in year 1's note. However, that was since given up on. I had a strange bug where sometimes,
the OS would just lock up executing an idle thread.  This bug was actually easily fixable, but it led me to create Boron,
so I'm going to let it slide.

<img src="/images/2023_09_10_20_35_33.png">

**2023-11-09**: It's A Me, Mario!

It's-a him!  I've actually always wanted to see Mario 64 run on NanoShell, and it finally happened!  Took a while.  At
first I tried using TinyGL, but that didn't work sadly (created corrupted graphics), so I resorted to using 
[the DOS port's software renderer](https://github.com/fgsfdsfgs/sm64-port/blob/dos/src/pc/gfx/gfx_soft.c).

<img src="/images/2023_11_09_22_35_47.png">


And with that, this little screenshot tour of the second year of NanoShell is over! Let's see what 2024 has to bring.

See you in 2025!

*iProgramInCpp, 2024-01-19*

