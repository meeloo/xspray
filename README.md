Xspray
======
A front end for lldb on OS X for Mac and iOS targets, with a twist

![Xspray](https://raw.githubusercontent.com/meeloo/xspray/master/XsprayScreen.png?raw=true "Xspray LLDB front end for OSX and iOS")

The full story:
---------------
About one year ago I took some time experimenting with creating a new GUI for lldb. My idea was to build a debugger that permits to deeply inspect data at runtime. As a developer it is often very frustrating to the IDE stop in the code and not be able to really inspect variables that are big containers (beyond a dozen of floats…). When I deal with real time audio, image processing, video, maths, etc., not having access to a more visual representation of my data often makes my life miserable. I also would like to have a complete set of tools to analyse my data: compute the average, the sum, the first derivative of a big vector of float can be life saving.
So I started to toy with lldb on the Mac and I developed a prototype to see if my idea was feasible. LLDB is an absolutely amazing project (along with LLVM and Clang…) and developing a simple plotter that can inspect std::vector and std::list at runtime was relatively easy on OSX (and the answers I got to my newbie questions on the lldb-dev mailing list have been very helpful too!). Making it work with an iOS target proved much harder as there is very little documentation about how Xcode and iTunes installs application and start the debugger there. After much trial an error I succeeded to have that working too.
The next idea was to build a complete GUI for LLDB that would work on Android too: native debugging on that plateform is a pure nightmare of horrible scripts and arcane command line instructions. Unfortunately lldb was not at all able to gdb-remote a linux host (even x86 gdb-remote didn’t work at all at the time) and an android debugserver implementation seemed out of question. Which is really a shame as I’m pretty sure game developers (for exemple) would love to have a full blown debugger on that platform. (and one with the features I envisioned would be really very cool…). So I stopped working on it (also I was running out of free time).

Unfortunately, I haven’t had much time to really touch my prototype since september 2013 as after giving the idea some thoughts I haven’t found compelling proof that I could make a living by selling this kind of software (nobody wants to launch a separate debugger from Xcode right?). But I thought it was too bad to have all that code lying there doing nothing and helping nobody. So I have decided to release this small prototype as an open source project on github: http://github.com/meeloo/Xspray.

The features:
--------------
- load an iOS or MacOSX application
- get infos from the iOS devices plugged on the computer
- see the list of compiles units
- pause/ step over / step in / step out / continue
- load C/ObjC/C++ source and display it with syntax colouring (thanks to libclang…).
- add/remove breakpoints from the source code by clicking in the gutter
- list the module dependencies (dylibs, frameworks, etc) and object files.
- launch the program on the Mac or iOS device (will as for permission, etc).
- stop on breakpoints
- inspect variables
- display a crude plot when you select a variable that contains a std::list, std::vector or array of simple types (signed and unsigned integers of 8, 16, and 32 bits, floats, doubles)
- probably more things that I have forgotten
- written in C++, mostly portable to linux and windows thanks to NUI (could be done in a couple of weeks I guess).
- the code is still very messy, particularly on the iOS side.

Some features are not plugged in the UI yet (iOS devices that were already plugged at startup don’t show up in the UI for exemple). And the UI in itself is far from looking like a finished product, it not even alpha grade in my opinion, but it works.

Future:
-------
Here is a list of some of the features that I wanted to add one day (and I might do it one day):
- TimeMachine for debugger: store the value of very variable of every point of the stack trace on each breakpoint stop. The idea is to have a timeline slider that permits to go back in time and still see what happened a couple of loop iteration before the current one, for exemple. (don’t you hate it when you have an error occurring after about 50 iterations of a loop and in your StepOver frenzy you miss the important point and you have to start over again?).
- With the data collected in the TimeMachine I want to be able to plot the evolution of any variable / class member at a particular breakpoint. I would call that visual watched variables.
- statistics and vector operations on the data. because in 2014 we can do better than an hexadecimal dump of memory.
- a memory visualiser that can interpret a chunk of memory as some common data type and visualise it: 8, 16, 24, 32 bit PCM audio, rgb/rgba/grey level pictures, plain text (and variants) with configurable encoding, etc.
- a plugin system for custom data type visualisation



Dependencies:
-------------
- lldb / llvm / clang of course
- NUI: my multi-platform UI framework on top of OpenGL. https://github.com/libnui/nui3


Building Xspray
---------------
To build Xspray on OSX you need llvm, clang and lldb.

Setting up the folders for correct dependencies:

     WORK  
       `-- lldb
       |
       `-- nui3
       |
       `-- Xspray

Change to the directory where you want to do development work and checkout LLDB:

    > cd WORK
    > svn co http://llvm.org/svn/llvm-project/lldb/trunk lldb
    > git clone git@github.com:libnui/nui3.git
    > git clone git@github.com:meeloo/Xspray.git

You will also need swig installed. I got it view brew:

    > brew install swig

Note1: Beware! If you have installed Xcode 5 and you are trying to build with Xcode 4, chances are the system svn will be v1.7 while Xcode 4 is still 1.6 and it will prevent the build script from fetching llvm and clang sources. If thats the case, use the svn binary that is inside the Xcode bundle like that:

    > /Applications/Xcode.app/Contents/Developer/usr/bin/svn co http://llvm.org/svn/llvm-project/lldb/trunk lldb

Note2: I have found that we don't need to use a special cryptographic certificate as the lldb mentions, using the default Mac Developper cert will work fine on any machine. YAY! For that change, select the LLDB project in Xcode, then select the *lldb\_gdbserver* target and look for the code signing section. Change all *lldb\_codesign* to your own Mac OS Developper certificate.

In general, building the LLDB trunk revision requires trunk revisions of both LLVM and Clang but the Xcode project takes care of this directly.

So just change the Active Scheme to LLDB and command+B to build LLDB and all its dependencies (llvm, clang...). It will probably complain about already existing folder but ignore these errors. Sometime I have compilation errors because my LLDB and LLVM/Clang are desynchronized. When this happens I haven't found another solution: I just checkout a fresh LLDB to replace the current broken one.

------
Xspray:

You can now launch the Xspray workspace in Xcode, it is located in WORK/Xspray/Xspray.xcworkspace

Select the LLDB scheme and edit it to change the default configuration to release. Build LLDB (go have a cup of coffe, it will svn checkout llvm + clang and build them, then build itself, it takes a while). Or watch the all the Lord of the rings movies a couple of times.
Once it is built without error (hopefully) you can select the Xspray target and build it. It should produce a valid Mac application bundle that contains a valid lldb framework (and libclang.dylib).

Warning: for now, the executable I do my tests on is hardcoded in the MainWindow.cpp. Edit it out to point to an existing debugable executable on your machine.

Feel free to ask any question :D

Sébastien
