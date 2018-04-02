# Dasher Java Installation and Code Explanation
This document contains 5 part about Dasher's java version.

* Installing and testing(1)
* Licensing(2)
* Package outline(3)
* Class outline of dasher package(4)
* Contacting the author and contributing(5)


## Getting Started(1)
Dasher makes use of the GNU autotools system (automake, autoconfetc.), which are described in the 'INSTALL' file.  The following is specific to building and installing Dasher on JAVA.  The Dasher
maintainer documentation is at http://live.gnome.org/Dasher.

### Prerequisites

What things you need to install the software and how to install them

```
Java 8 or Higher
```

```
Gradle 4.6
```

```
SDK
```

### Installing Gradle 4.6 with SDK

A step by step series of examples that tell you have to get a development env running

(For SDK)Simply open a new terminal and enter

```
$ curl -s "https://get.sdkman.io" | bash
```

Follow the instructions on-screen to complete installation.
Next, open a new terminal or enter


```
$ source "$HOME/.sdkman/bin/sdkman-init.sh"
```

Lastly, run the following code snippet to ensure that installation succeeded:

```
$ sdk version
```

If all went well, the version should be displayed. Something like:

```
sdkman 5.0.0+51
```
(For Gradle) Open terminal and write

```
$ sdk install gradle 4.6
```

To test the Gradle installation, run Gradle from the command-line:

```
$ gradle
```

If all goes well, you see a welcome message like

```
:help

Welcome to Gradle 2.3.

To run a build, run gradle <task> ...

To see a list of available tasks, run gradle tasks

To see a list of command-line options, run gradle --help

BUILD SUCCESSFUL

Total time: 2.675 secs
```

### Writing tests and Building

Our test cases are under the dasher/java/src/test. After writing new test cases you can simply build and control with following commands

```
./gradlew build
```

To see diffirencess, cd into applet area

```
$ cd dasher/java/src/main/java/dasher/applet/
``` 

"test.htm" shows how you run that from within an HTML page.

### Important notes about dasher

This project builds an applet, not a standalone application.

* [ENTRY POINT](https://github.com/dasher-project/dasher/blob/master/java/src/main/java/dasher/applet/JDasherApplet.java) - is the entry point.
* [STARTING POINT](https://github.com/dasher-project/dasher/blob/master/java/src/main/java/dasher/applet/test.htm) - shows how you run that from within an HTML page.

This is all obviously quite old, and some modern browsers (notably Chrome and Edge) don't support Java applets anymore. However, as of the time of writing 
(and checking https://superuser.com/questions/1213341/how-to-get-a-browser-that-supports-java), Internet Explorer and Firefox ESR should still be able to show JDasher.

## Licensing(2)

This program is free software; you can redistribute it and/or modifyit under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

The complete license can be found in COPYING in the parent directory.

## Package Outline(3)


### Dasher

This contains the core Dasher source code. It corresponds to the DasherCore package in the original C version of Dasher. By themselves 
the classes in this package cannot form a completely working Dasher. To produce a working system, one must:

* Implement the CDasherScreen interface (see section III for details)
* Subclass CDasherInput at least once to provide co-ordinates (eg. from a mouse) on demand (again, see details below)
* Subclass CDasherInterfaceBase, specifying the abstract methods of that class. Most of these can be stubbed and still produce a working 
system, though it may lack the ability to learn from training text, or load XML files.
* Optionally, subclass CSettingsStore to provide the ability to store persistent settings. If this isn't done and the base CSettingsStore is 
used, the defaults will be loaded every time Dasher is run.

### Dasher.applet.*


This package contains the classes to complete an implementation of Dasher as a Swing applet, by more or less the means described above. It 
also provides some extra, optional functionality, such as listening for text events and showing the currently entered text in a text box.

Key components:

* JDasher, which is a subclass of CDasherInterfaceBase and provides methods to retrieve data files from a JAR
* JDasherApplet, which sets up a Swing GUI and generates Dasher events to provide control.
* JMouseInput, a subclass of CDasherInput which provides mouse co-ordinates on demand.
* JDasherScreen, which implements CDasherScreen and converts graphics primitive instructions to Java AWT drawing commands.
* JSettings, which subclasses CSettingsStore to use the Java Preferences class to store persistent settings. The Preferences class in turn defers 
to the JVM, meaning it will try to use the Registry on Windows, a .javasettings file in Linux, and so forth.

### Dasher.net.*

This is an implementation of Dasher which provides a network interface based upon XML transactions. The 'screen' (NetDasherScreen) implements 
CDasherScreen but doesn't really act as a screen; rather it gathers relevant data and sends this to a client, such as the JavaScript client 
found at www.smowton.f2s.com/jsdasher.html. Similarly, NetInput provides input by interpreting XML documents fed by the client.

The net package provides a multithreaded server to host several such Dashers at once. Loosely, the structure is that NetDasherListener is 
started as the main thread, and this spawns a NetDasherCommandInterpreterThread for every incoming connection it 
receives. These threads interpret the XML document sent by the client and respond in a variety of ways, usually creating or invoking methods 
of a NetDasherConnectionHandler (which creates an instance of Dasher, NetScreen and NetInput and acts to pass messages back and forth between 
Dasher and the network server).

These ConnectionHandler objects as well as their associated Dashers are persistent between connections, and so act to provide a continuous session.

Typically JavaScript clients will not speak directly to the Dasher server, but will pass requests to a webserver first, since security 
restrictions typically require that JavaScript does not invoke URLs which are not on the same server as the JavaScript itself.



## Class Outline Of Dasher Package(4)

* CAlphabet: Represents the alphabet we're currently working with. It it responsible for holding information about how each symbol in the 
alphabet should be typed, how it should be displayed on screen, and some aspects of how the symbols should be coloured.

* CAlphabetManager: Carries out certain functions which manipulate the general Dasher state, but depend upon Alphabet data. Example include 
outputting the appropriate text when the user enters a node, and populating child nodes when required to do so by the Model.

* CAlphabetManagerFactory: Factory class for the above.

* CAlphabetMap: Wrapper around a HashMap from a UTF-8 representation of a symbol to its unique integer identifer. This used to be much more 
complicated, but could probably now be reduced to just a HashMap which would probably be a member of CAlphabet.

* CAlphIO: Given a list of XML files, parses them for descriptions of alphabets, and uses this to report a list of available alphabets.

* CAutoSpeedControl: Uses a variety of diagnostic signs to establish whether the user would like to go slower or faster, and controls the 
Dasher maximum speed appropriately.

* CCannaConversionHelper: In the C version, provides for conversion from Hiragana to Kanji in Japanese. At the time of writing this was an 
unstable development feature, and this class is a stub at present in JDasher.

* CCircleStartHandler: Provides the possibility of a circular target which can be used to start Dasher.

* CClickFilter (extends CInputFilter): An input filter which causes Dasher to zoom when the user clicks. It does this by calculating a set of 
points forming a line between the current position and that which was clicked and adds these to m_deGotoQueue, a list of "pending points" 
which the Model consults as a potential source of movement instructions.

* CColourIO: As CAlphIO, but reads colour scheme XML files.

* CContextBase: Base class of all language models' classes representing some context for predicting input. This is typically used outside the 
language model so that the rest of the program does not depend upon a particular language model. It exposes no methods, and so contexts cannot 
be modified outside the language model.

* CCustomColours: Wraps around a colour scheme object read by CColourIO; used by the View classes to determine how the nodes should look.

* CDasherComponent: This is a base class for a large number of Dasher components, and provides several basic functionalities, including the 
handling and creation of events, and the ability to consult and modify the settings store. It provides a default EventHandler which ignores all 
events, which is designed to be overridden by subclasses. Only subclasses of CDasherComponent can register themselves to listen for events.

* CDasherInput: This is a base class for some form of co-ordinate input which provides either 1 or 2 co-ordinates. This class is abstract and 
must be extended if you wish to provide any input to Dasher.

* CDasherInterfaceBase: This is the central class of the Dasher core. It provides centralised functions which have widespread consequences 
including Start(), ChangeAlphabet() and so forth. It also provides NewFrame(long) which is the standard way to cause Dasher to update the 
model and draw a frame. It does not perform much of the functionality itself however; it defers the maintenance of the model to CDasherModel 
and the drawing of it to CDasherView and its children.

* It includes a number of abstract methods which are implementation-dependent. These must be implemented to produce a working 
Dasher, but most can be stubbed safely. The only issue which is genuinely important is to ensure that a CSettingsStore (or subclass) is 
produced before any CDasherComponents are created (which would depend upon it).

* CDasherModel: This maintains a tree of CDasherNodes, each of which represents a box with a letter in it. It is responsible for deciding 
when to create new nodes (although it defers the actual act to the AlphabetManager) as well as when to delete old nodes, in collaboration 
with the View (which informs the Model of situations such as a node being too small to draw, and so a candidate for deletion). It also plays 
a major role in making Dasher "move" by altering the crosshair position in response to newFrame calls made to CDasherInterfaceBase.

* CDasherNode: This represents a box with a symbol in it, and is the unit component of the Dasher tree maintained by the CDasherModel. It also 
posesses a Context object (child of CContextBase) which describes the prediction context for this node (ie. the data the language model should 
use in deciding its likely successors). Nodes are also aware of their size in terms of Dasher co-ordinates, though not their on-screen size.

* CDasherScreen: This defines an interface for drawing (or otherwise outputting) graphics primitives. Some class must implement this to get 
any visual output from Dasher, and an instance of this class must be registered with the Interface by means of its ChangeScreen method.

* CDasherView/Square: The base and one implementation for a 'view' which is to say a class for visualising the Dasher tree. Whereas the model 
knows only the nodes' relative probabilities, the View converts these into some visual representation; in the case of ViewSquare, into a set 
of square boxes. It responds to a Draw() request by translating the Model's data into primitive drawing commands executed by some 
implementation of CDasherScreen.

* CDefaultFilter: Default input filter, which simply causes Dasher to zoom towards wherever the user is currently hovering the mouse.

* CDelayedDraw: Helper class for drawing boxes with a symbol in them without the risk of overwriting symbols by mistake. All text to be drawn 
is placed on a stack, and at the end of the drawing cycle, and after all other primitives have been drawn, the stack is emptied and its contents 
drawn.

* CEventHandler: The event handler permits CDasherComponents to register themselves as event listeners. All components register themselves as 
part of the Component's constructor, though of course this can be overridden. When an event is raised, the HandleEvent method of each is 
called in turn with the Event as a parameter.

The EventHandler does not involve any threading whatsoever, unlike Java's built-in event subsystem. When a Dasher event is raised, the 
raising method will not continue until all listeners have had their HandleEvent routines called.

For the eventuality that a reponse to an event causes another event to be raised, the event handler keeps track of how many events are 
currently unfinished. If a listener tries to add itself whilst this number is not zero, it goes in a queue. As soon as the last event 
finishes processing, the Components in the queue are made listeners proper.

* CInputFilter: Base class of all input filters. These play a pivotal role in the update-model-and-draw frame cycle. They respond to both key 
presses and mouse clicks by their keyDown methods being invoked, and the drawing of a frame without cue through their Timer method, whose 
argument specifies the time since the last frame.

* Typically, the start of a new frame consists of the Timer method being called, which is then responsible for calling the Model's functions with 
parameters indicating what changes need making.

The inputfilter will also be invoked at the very end of the new frame cycle in order to apply any view decorations it wishes to on top of the 
standard appearance rendered by DasherView. A typical example is drawing a line to the current mouse position.

* CLanguageModel: This is responsible for Dasher's prediction of what the user is most likely to enter next. It is fed data by having its 
EnterSymbol and LearnSymbol methods called on Context objects (children of CContextBase), and will have getProbs called to determine the 
relative sizes of CDasherNodes whenver the tree is expanded by the AlphabetManager.

Details of how the modelling is done is dependent on the language model used. The only subclass included here is CPPMLanguageModel, but a few 
other possibilities were explored in the original C version and can be found in its source.

## Contacting The Author And Aontributing(5)

Author can be reached at chris@smowton.net, cs448@cam.ac.uk

Please read [CONTRIBUTING](https://docs.google.com/document/d/1qosKPV0GXbHs69PyvK9BR3S3oNgdV8il7AVfO1M4Fh4) for details on our code of conduct, and the process for submitting pull requests to us.



