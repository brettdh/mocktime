Mocktime
========

Dead-simple, non-fancy `gettimeofday`-based time mocking library.

In my own projects, I've found that sometimes I want to test code that
depends on the passage of time - for example, calculating network
round-trip time, or [estimating energy usage][powertutor] of an Android
phone based on system parameters.  Using `gettimeofday` and `usleep`
for these tests can make my test suite take a long time to run.

The `timeofday` is just a number, though, so I should be able to mock it.
I couldn't find any existing library that would let me do this (at least,
not without changing all my code to use something other than `gettimeofday`),
so I wrote this.

## How to use

First, build and install the library:

    $ git clone https://github.com/brettdh/mocktime.git
    $ cd mocktime
    $ make
    $ sudo make install
    
And then in your code:

    mocktime_enable_mocking();
    
    struct timeval fake_time = {1445470140, 0}; /* October 21, 2015  04:29pm PDT */
    mocktime_settimeofday(&fake_time, NULL);
    
    struct timeval the_present;
    mocktime_gettimeofday(&the_present, NULL);
    /* 'the_present' now contains the time that you set. */
    
After you call `mocktime_enable_mocking`, all future calls to the other 
`mocktime_*` functions (excluding `enable_mocking` and `disable_mocking`)
will operate on the fake `timeofday` inside the `mocktime` library.
If you later call `mocktime_disable_mocking`, or if you never call
`mocktime_enable_mocking`, the calls will pass through to their
non-mocked counterparts.

For convenience, there's also a `mocktime_usleep` function that advances
the mocked time by the specified number of microseconds, just as you'd expect.

## Android

I also use this library on Android, so I've included an NDK build setup.
just run `ndk-build` in the root directory, and then point your own
`Android.mk` at the generated library.  You'll need to use the
`PREBUILT_SHARED_LIBRARY` to "install" `libmocktime.so` into your project's 
build path, and then you'll add it to your `LOCAL_SHARED_LIBRARIES`.
See the Android NDK documentation for details.

## Caveats

Mocktime only supports a single, global mocked `timeofday` and is therefore
NOT thread-safe.

[powertutor]: http://github.com/msg555/powertutor
