TODO is in TODO.txt

-----

Installing on OS X:
  Please install boost using macports:
    sudo port install boost
  Build and install
    make
    make test
    make install-local

Installing on Ubuntu:
  Please install boost using apt-get
    sudo apt-get install libboost-dev libboost-doc (from memory, may be incorrect)
    sudo apt-get install libboost-thread (from memory, may be incorrect)
  Build and install
    make
    make test
    make install-local

-----------

If only one tile, pick:

infinity tile until needing to split?
or default level tile?
problem with default level tile is that we might want to propagate it
upwards

should infinity tile be hacked to cover all time?


go up in levels until the tile is too large
separate positive and negative roots






DONE add tests for lock, unlock.  use boost threads





BodyTrack datastore design: https://docs.google.com/document/d/1Hhpw2jGVkDOo5fZjMAAkJ0cFW_eWMrTHIjcq1UJvibg/edit?hl=en_US







Dealing with variable sampling rates

Approaches:

put everything that fits into a bin
Problems:
- need to filter before display
- need to filter before calculating
- need to filter differently depending on display or calculating
So, the DB interface: get me this range of values, with at least this
resolution (but not "at most this resolution")

Make a bin be ~1MB (32K samples x 32 bytes = 1MB)
Lowest level tile has no summarizations time,value.  64K samples x 16 bytes = 1MB
Lowest level 8bit samples tile has time, values.  1M samples x 1 bytes = 1MB

1g samples/

Building jsoncpp:
cd jsoncpp-src-0.5.0/
python scons.py platform=linux-gcc check

2TB = $100 + $1/yr  1%/yr=power,for drive
1GB = $0.05, plus 

24*30 = 

1KWH = 0.10

1GB = 

-------------------------

New approach:
full-res all original samples
subsampling filter is a function applied

--------------------

If the amount of storage required to store N samples varies by a factor of 32, how do we cope with this?
1) does subsampling necessarily inflate 1x to 32/2x = 16x?  yuck
    or does subsampling filter and create something with even sampling?  maybe float val + float stddev = 8x instead of 32x

1Gsample/day x 1B = 1GB/day = 0.05 / day
1Gsample/day x 32B = 32GB/day = 1.50 / day


-------------------------
To install with optional FFT support via the NFFTv3 library, which
is under the GPLv2 license:

Download and install NFFT, version 3 (as of the time of this writing,
NFFT 3.2.2 was the most recent version, and is also the version which
was used for testing during development.  You might try a newer version
if it's available):

$ wget http://www-user.tu-chemnitz.de/~potts/nfft/download/nfft-3.2.2.tar.gz
$ tar xvzf nfft-3.2.2.tar.gz
$ cd nfft-3.2.2
$ ./configure
$ make
$ sudo make install
$ cd ..
$ rm -r nfft-3.2.2 nfft-3.2.2.tar.gz

This should install the NFFT library on your system.  Then, to opt in
to NFFT support, add FFT_SUPPORT=1 to the make command line
(the Makefile compiles with FFT support whenever FFT_SUPPORT is set to
a non-empty string).  In other words, replace "make target" with
"make target FFT_SUPPORT=1" for whichever choice of target you would
have used normally.  For example, instead of "make test", execute

$ make test FFT_SUPPORT=1

You will know that your freshly built binaries support the Fourier
transform by running

$ ./gettile

If the help message contains a sentence mentioning the DFT or Fourier
transform, such as "If the string '.DFT' is appended to the channel name,
the discrete Fourier transform of the data is returned instead,"
then of course your binaries have been compiled with FFT support.

