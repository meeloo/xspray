![Xspray](https://raw.githubusercontent.com/meeloo/xspray/master/XsprayScreen.png?raw=true "Xspray LLDB front end for OSX and iOS")

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
    > git clone git@github.com:meeloo/nui3.git
    > git clone git@github.com:meeloo/Xspray.git

You will also need swig installed. I got it view brew:
    > brew install swig

Note1: Beware! If you have installed Xcode 5 and you are trying to build with Xcode 4, chances are the system svn will be v1.7 while Xcode 4 is still 1.6 and it will prevent the build script from fetching llvm and clang sources. If thats the case, use the svn binary that is inside the Xcode bundle like that:
    > /Applications/Xcode.app/Contents/Developer/usr/bin/svn co http://llvm.org/svn/llvm-project/lldb/trunk lldb

Note2: I have found that we don't need to use a special cryptographic certificate as the lldb mentions, using the default Mac Developper cert will work fine on any machine. YAY! For that change, select the LLDB project in Xcode, then select the lldb_gdbserver target and look for the code signing section. Change all lldb_codesign to your own Mac OS Developper certificate.

In general, building the LLDB trunk revision requires trunk revisions of both LLVM and Clang but the Xcode project takes care of this directly.

So just change the Active Scheme to LLDB and command+B to build LLDB and all its dependencies (llvm, clang...). It will probably complain about already existing folder but ignore these errors. Sometime I have compilation errors because my LLDB and LLVM/Clang are desynchronized. When this happens I haven't found another solution: I just checkout a fresh LLDB to replace the current broken one.

------
Xspray:

You can now launch the Xspray workspace in Xcode:
WORK/Xspray/Xspray.xcworkspace

Select the LLDB scheme and edit it to change the default configuration to release. Build LLDB (go have a cup of coffe, it will svn checkout llvm + clang and build them, then build itself, it takes a while). Or watch the all the Lord of the rings movies a couple of times.
Once it is built without error (hopefully) you can select the Xspray project and build it. It should produce a valid Mac application bundle that contains a valid lldb framework (and libclang.dylib).

Warning: for now, the executable I do my tests on is hardcoded in the MainWindow.cpp. Edit it out to point to an existing debugable executable on your machine.

Feel free to ask any question :D

