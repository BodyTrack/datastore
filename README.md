BodyTrack / Fluxtream / ESDR Datastore
======================================

Efficiently handle high volumes of numeric and text time series, with precomputed level-of-detail summarizations.

Goals:

* Fast to write samples or sequences of samples
* Fast to read time-bounded sequences of samples at requested level of detail

Installing on OS X
------------------

Install boost using e.g. macports:

    sudo port install boost

Build and install

    make
    make test
    make install-local

Installing on Ubuntu
--------------------

Please install boost using apt-get (TODO(rsargent): verify this)

    sudo apt-get install libboost-dev libboost-doc
    sudo apt-get install libboost-thread

Build and install

    make
    make test
    make install-local

Importing sparse data
---------------------

JSON representation of data for import includes a dense array of timestamped rows:

    "channel_names": ["latitude", "longitude", "accuracy", "bearing", "provider", "speed"],
    "data": [
      [1312774909.76562, 32.8465, -117.27, 0, 0, "network", 1],
      [1312774910.76562, 32.9013, -117.265, 0, 0, "network", 2],
      [1312774911.76562, 32.8982, -117.252, 0, 0, "network", 3]
    ],

You can use `null` for sparse data.  This will result in no sample
being added for that particular channel at that particular time:

    [1312774910.76562, null, null, 0, 0, "network", 2]

Datastore will only hold one value for a device.channel at a
particular time.  So if you write a new value at a time that already
has a value, you'll overwrite the old value with the new.

Deleting data samples
---------------------

It's possible to delete samples when you overwrite.  If you want to
delete individual samples (or, overwrite with "no sample" so to
speak), you can use `false` in JSON import for the values you wish to
delete:

    [1312774909.76562, false, false, false, "speedometer", 4]

FFT support:
------------

Note: FFT support is unfinished, so this documentation isn't right, yet:

To install with optional FFT support via the NFFTv3 library, which
is under the GPLv2 license:

Download and install NFFT, version 3 (as of the time of this writing,
NFFT 3.2.2 was the most recent version, and is also the version which
was used for testing during development.  You might try a newer version
if it's available):

    wget http://www-user.tu-chemnitz.de/~potts/nfft/download/nfft-3.2.2.tar.gz
    tar xvzf nfft-3.2.2.tar.gz
    cd nfft-3.2.2
    ./configure
    make
    sudo make install
    cd ..
    rm -r nfft-3.2.2 nfft-3.2.2.tar.gz

This should install the NFFT library on your system.  Then, to opt in
to NFFT support, add FFT_SUPPORT=1 to the make command line
(the Makefile compiles with FFT support whenever FFT_SUPPORT is set to
a non-empty string).  In other words, replace "make target" with
"make target FFT_SUPPORT=1" for whichever choice of target you would
have used normally.  For example, instead of "make test", execute

    make test FFT_SUPPORT=1

You will know that your freshly built binaries support the Fourier
transform by running

    ./gettile

If the help message contains a sentence mentioning the DFT or Fourier
transform, such as "If the string '.DFT' is appended to the channel name,
the discrete Fourier transform of the data is returned instead,"
then of course your binaries have been compiled with FFT support.

