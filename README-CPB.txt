Finally got it to build on my Mac (macOS 11.6.x) on 2022-01-19 after much pain.  Here's what I did:

1) upgraded MacPorts (not sure if this was actually necessary) and all the stuff I had installed using MacPorts (boost was my primary concern, because the datastore requires it)

2) upgraded gcc/g++ view homebrew

3) discovered my Xcode command line tools was outdated, thanks to this tip: https://stackoverflow.com/a/70742911/703200

4) reinstalled my Xcode command line tools, following the instructions given from running "brew doctor"

On my Mac, I have to specify a different compiler since the default one is ancient.  Do that simply by doing:

   make COMPILER=/usr/local/bin/g++-11
