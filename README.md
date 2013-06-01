slow
====

A simple cli tool that forcibly slows the execution of another process  
Copyright (c) 2004 - 2013 [John Bafford](http://bafford.com)


WHAT IT IS
----------
Slow is a unix command line tool that makes it easy to cause another process to run slowly. Slow does this by taking advantage of two signals, SIGSTOP and SIGCONT, which respectively stop and continue a unix process’ execution.

Slow has been tested on Mac OS X and Linux.

Slow may be useful if you have a long-running cpu-bound task and need it to not consume all available cpu resources. I have also used slow to slow the execution of part of a data import and processing tool chain where one part of the chain consumed disk space with temporary data far faster than that data could be processed and freed.


COMPILING SLOW
--------------
Slow comes with a Makefile; make slow will compile it. I suggest then copying it to /usr/local/bin or your favorite binary installation directory.


HOW TO USE SLOW
---------------
`slow [-p pid] [-d dutyCycle] [-s timeSlice] [program [args]]`

	-p pid: Specify the pid of the process to slow; if not present, you must specify a process to run at the end of the command line
	-d dutyCycle: 0 < dutyCycle < 1 - portion of time to allow process to run (default: 0.5)
	-t timeSlice: The length of one stop/run cycle (default: 1 second)
	-v: version info
	-h, -?: help

Slow can run in two modes: the first, by specifying a process id with the `-p` option, causes slow to attempt to cause an existing process to slow down. Alternatively, you can have it run a new command by specifying the target program and any arguments at the end of the slow command.

The `-d dutyCycle option allows you to specify how much slow attempts to slow the target process. This is a number between 0 and 1. The default (0.5) will cause a process to run 50% of the time.

The `-t timeSlice` option allows you to specify how big a timeslice slow uses. Over the duration of the timeslice, slow will cause the process to run for a percentage of time specified by `-d dutyCycle`, and then cause it to sleep for the remainder.

For example, with the default values, slow will run the target process for 0.5 seconds, and then cause it to sleep for 0.5 seconds, repeating until slow is stopped or the target process terminates. Specifying `-d 0.25 -s 0.5` will cause slow to run the process for 0.125 seconds and sleep for 0.375 seconds.

If slow is run by specifying a command to run, slow will pass the following signals it receives through to the child process: `SIGHUP`, `SIGINT`, `SIGTERM`, `SIGTSTP`, `SIGINFO`, `SIGUSR1`, `SIGUSR2`. This will allow interaction with the target process as usual, so a Control-C will kill the target process, and a Control-Z will suspend the target process (and `slow`).

