This is the supporting benchmark for the post at TODO link.

This benchmark relies heavily on the [`perf_events`](http://man7.org/linux/man-pages/man2/perf_event_open.2.html) subsystem and so only runs on Linux.

## Build

    make

## Checking perf_event_paranoid

To run these tests you need a `/proc/sys/kernel/perf_event_paranoid` setting of 1 or less, or to be running as root. Many modern system are setting this to 3, a very conservative setting. If the value on your system is greater than 1, you can either set it (until reboot) to 1 like so:

    echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid

Or you can run the tests as root (e.g., `sudo ./bench)

## Running

The benchmark includes several tests. You can run them all like this (this will crash pretty quickly on platforms without AVX-512):

    ./bench

You can run a specific test by providing its name:

    ./bench vporymm

You can list the available tests:

    ./bench list tests


## Generating Results

You can run all the test required for generating the data used in the post using the ./data.sh scripts. First you should set your TSC frequency as the `MHZ` variable in the environment:

    export MHZ=3200

Then you can collect most of the data like this:

    scripts/data.sh

This generates the results in the `./results` dir by default. This generates all the results _except_ for those related to voltage. To generate those, you need to jump through a few hoops, because voltage readings require MSR access. First, you must have `msr-tools` installed, however that works on your platform (e.g., `apt install msr-tools` on Ubuntu). Then you can either run data.sh as root, like so:

    sudo MHZ=$MHZ DO_VOLTS=1 scripts/data.sh

The `DO_VOLTS=1` indicates to the script that it should run only the voltage tests. This causes the files written to `./results` to be owned by root (unless the files already existed with different ownership), so you should `chown` them to your current user after. You can also run this as a non-root user but you have to jump though some hoops (this is probably not entirely safe, certainly not on any host with untrusted users or code):

~~~
# set the special msr files world readable
sudo chmod a+r /dev/cpu/*/msr
# give the bench binary the caps needed to read the msr files (not needed on all systems)
sudo setcap cap_sys_rawio=ep ./bench
# run the test as non-root
DO_VOLTS=1 scripts/data.sh
~~~

## Generating Plots

Once the results are generated, you can generate the plots with:

    scripts/plots.sh

That will show the plots, one by one, in matplotlib's interactive viewer. To write them out to SVG files, just provide the `OUTDIR` environment variable:

~~~
mkdir plots
OUTDIR=./plots scripts/plots.sh
~~~

The script expects all the result files to exist, feel free to comment out some lines if you've just generated partial results. You can also pass the script a regex and it will only generate plots whose title matches the regex. For example, the following generates only the 3 voltage plots:

    ./scripts/plots.sh Voltage






    
