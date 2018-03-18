Dasher Version 4.11
-------------------
Dasher is a zooming predictive text entry system, designed for situations
where keyboard input is impractical (for instance, accessibility or PDAs). It
is usable with highly limited amounts of physical input while still allowing
high rates of text entry.

Dasher is Free software released under the GPL. Further documentation
may be found in the Doc/ subdirectory

Requirements
------------
Dasher makes use of the following third party software (note that this
list is potentially inaccurate or incomplete - please send any
corrections to 'dasherteam@yahoogroups.com):

General:
 * The Expat XML parser (required)

Linux (see INSTALL.Linux for build time requirements):
 * GTK+ (2.6 or greater) (required)
 * GNOME (optional, but highly recommended)
 * Libcanna (optional, for Hiragana to Kanji conversion)
 
MS Windows:
 * MS Speech API (optional)
 * MS Table API (optional - required for tablet start/stop mode)

MS Windows CE:
 * (not sure - if anyone knows then please fill this in)

Platform Specific Notes
-----------------------
* Solaris: Dasher has been tested on Solaris 10. You will need to
upgrade to gnome-doc-utils 0.3.2 to install the documentation, which
in turn will require libxml 2.6.12 and libxslt 1.1.8. These packages
are not required to run dasher once it is built.

There have been some issues with the gthread-2.0 pkgconfig file
including compiler flags which are not understood by the compiler. You
may need to shadow this file in another directory with these flags
removed.

GNU make (gmake) must be used if building gnome documentation.

* Darwin / OS-X: The native UI in the macos directory is badly out of
date and will almost definitely require considerable effort to
build. We do not currently have the resources to maintain this port,
but if you are interested in contributing then let us know
(dasher@inf.phy.cam.ac.uk). The GTK2 front end should build on Darwin
with minimal changes, and with a recent enough version of GTK will not
require the X11 server.

Installation Instructions
-------------------------
Please see the file INSTALL.<platform> for details on how to build and
install Dasher from source on your host system.

Font Support
------------
Dasher includes some alphabets which require fonts that you probably don't 
have installed; for example, for the International Phonetic Alphabet, Korean
and Hiragana (Japanese).  We aren't legally allowed to distribute a font for
the IPA, but you can download one yourself from:  

   http://www.sil.org/computing/fonts/encore-ipa.html

Kanji Entry
-----------

As of Dasher 3.99.0, experimental support for Japanese Kanji entry
by Hiragana conversion. To build Dasher with this enabled on Linux,
you need the libcanna conversion library. You also need to be able to
access a canna conversion server, which can be on the local machine or
over a network. Please note that Dasher will not start a canna server
if none is running - you must do so manually before running Dasher. 
Please see the Canna documentation for more details.

Kanji conversion in Microsoft Windows is via the IME API, which is
part of Windows XP and Asian builds of earlier Windows
versions. Support for Kanji conversion in Dasher under Windows is
still very much under development, so please take care when trying
it out. In particular, Dasher will crash if you try to enable Kanji
conversion on a machine which does not have Japanese localisation
installed.

To use Kanji conversion in Dasher, you must select 'Japanese Canna'
from the 'Alphabet' section of the preferences dialogue, and the
Japanese language model from the 'Prediction' section.

Support and Feedback
--------------------
Please file any bug reports in the GNOME Bugzilla system:

http://bugzilla.gnome.org/

using the 'dasher' package name. We also appreciate general feedback -
please send any comments to 'dasher@inf.phy.cam.ac.uk'.

You can find the Dasher website at:

  http://www.inference.phy.cam.ac.uk/dasher/

Thank you for trying Dasher,

The Dasher Team

