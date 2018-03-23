# Windows Development Environment for Dasher

This is a guide for Dasher developers on Windows.

## Build Status

Windows:
[![Build status](https://ci.appveyor.com/api/projects/status/hpvrm5230qlay15m?svg=true)](https://ci.appveyor.com/project/erdemkiraz/dasher)
+
Java:
[![CircleCI Build Status](https://circleci.com/gh/dasher-project/dasher.svg?style=shield)](https://circleci.com/gh/dasher-project/dasher)

## Dasher Version 5.0

Dasher is a zooming predictive text entry system, designed for situations where keyboard input is impractical (for instance, accessibility or PDAs). 
It is usable with highly limited amounts of physical input while still allowing high rates of text entry.

Dasher is Free software released under the GPL. Further documentation may be found in the Doc/ subdirectory

## Requirements

Expat XML Parser 
Downloads can be found at http://expat.sourceforge.net

The project files use environment variables to locate the root directories of 
the prerequisites.  You can set these in the Windows control panel: 
Control Panels->System->Advanced->Environment Variables.

Here are the variable names and their default install locations (you may
need to change these to reflect reality on your system):

EXPAT          C:\Program Files\Expat 2.2.5

## Building
Visual Studio 2013 solution/project files are included in the 
Src/Win32 directory.

**NOTE** 
'''
Default build tools of Visiual Studio don't work. So you need to modify the Target Framework and Platform Toolset. Instructions can be found at [ How to Retarget ](https://docs.microsoft.com/en-us/cpp/build/how-to-modify-the-target-framework-and-platform-toolset)
'''

## Support and Feedback
Please file any bug reports in the [ GNOME Bugzilla system ](http://bugzilla.gnome.org/)
using the 'dasher' package name. 
You can find the Dasher website at: 
http://www.inference.phy.cam.ac.uk/dasher/

Thank you for trying Dasher,

The Dasher Team


