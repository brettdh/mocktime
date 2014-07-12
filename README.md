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

    $ make
    $ sudo make install

And then in your code:

    mocktime_enable_mocking();

    struct timeval fake_time = {1445470140, 0}; /* October 21, 2015  04:29pm PDT */
    mocktime_settimeofday(&fake_time, NULL);

    struct timeval the_present;
    mocktime_gettimeofday(&the_present, NULL);
    /* 'the_present' now contains the time that you set. */

After you call `mocktime_enable_mocking`, all future calls to the
other `mocktime_*` functions will operate on the fake `timeofday`
inside the `mocktime` library.  If you later call
`mocktime_disable_mocking`, or if you never call
`mocktime_enable_mocking`, the calls will pass through to their
non-mocked counterparts.

For convenience, there's also a `mocktime_usleep` function that advances
the mocked time by the specified number of microseconds, just as you'd expect.

## TODO

* Automatically replace `gettimeofday` and friends with the mocked
  version using `dlsym` tricks. I may get to it eventually; feel free
  to submit a pull request for this.

* Add mocks for `std::chrono` facilities in C++11, as they're much
  nicer to use.  Should maintain C-only compatibility, though.

## Android

I also use this library on Android, so I've included an NDK build setup.
First symlink the `jni` directory into your NDK modules path:

    $ ln -s /path/to/mocktime/jni $NDK_MODULE_PATH/edu.umich.mobility/mocktime

then add this line to the end of your `Android.mk`:

    $(call import-module, edu.umich.mobility/mocktime)

You'll also need to add `mocktime` to your `LOCAL_SHARED_LIBRARIES`.

## Caveats

Mocktime only supports a single, global mocked `timeofday` and is therefore
NOT thread-safe. I haven't thought at all about what it would mean to let one
thread mock-sleep while others continue running - i.e. whether it's feasible
or sensible at all.

[powertutor]: http://github.com/msg555/powertutor
