/*
	slow 1.0.1
	Copyright (c) 2004-2013 John Bafford
	https://github.com/jbafford/slow
	
	slow is a simple command line tool that forcibly slows the execution of another process.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is furnished
	to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define SLOWVERSION "1.0.1"
#define SLOWAUTHOR "John Bafford"
#define SLOWAUTHORWEBSITE "http://bafford.com/"
#define SLOWCOPYDATE "2004 - 2013"
#define SLOWWEBSITE "https://github.com/jbafford/slow"


enum slowStopCodes {
	kSlowStopped = 0,
	kSlowWatchMissing = 1,
	kSlowChildExited = 2,
};

enum {
	kSlowRunning = 0,
	kSlowStop = 1,
};

pid_t _slowPID;
int _slowFlag = kSlowRunning;

static void _passthruSignal(int sig)
{
	kill(_slowPID, sig);
}

static void _passthruSignalTSTP(int sig)
{
	kill(_slowPID, sig);
	
	kill(getpid(), SIGSTOP);
}

static void _signalTerminate(int sig)
{
	_slowFlag = kSlowStop;
}

static void ChildSignalHandlers(pid_t pid)
{
	signal(SIGHUP, _passthruSignal);
	signal(SIGINT, _passthruSignal);
	signal(SIGTERM, _passthruSignal);
	signal(SIGTSTP, _passthruSignalTSTP);
	signal(SIGUSR1, _passthruSignal);
	signal(SIGUSR2, _passthruSignal);
	
#ifdef SIGINFO
	signal(SIGINFO, _passthruSignal);
#endif
}

static void ExternalSignalHandlers(pid_t pid)
{
	signal(SIGHUP, _signalTerminate);
	signal(SIGINT, _signalTerminate);
}

int slow(pid_t pid, float dutyCycle, float timeSlice, int childWait)
{
	int stopTime = 1000000 * timeSlice * (1 - dutyCycle);
	int contTime = 1000000 * timeSlice * dutyCycle;
	
	_slowFlag = kSlowRunning;
	_slowPID = pid;
	
	if(childWait)
		ChildSignalHandlers(pid);
	else
		ExternalSignalHandlers(pid);
	
	//printf("%d %f %fs :: %d, %d\n", pid, dutyCycle, timeSlice, stopTime, contTime);
	
	for(;;)
	{
		if(_slowFlag == kSlowStop)
			return kSlowStopped;
		
		if(-1 == kill(pid, SIGSTOP))
			return kSlowWatchMissing;
		
		usleep(stopTime);
		
		if(-1 == kill(pid, SIGCONT))
			return kSlowWatchMissing;
		
		usleep(contTime);
		
		if(childWait)
		{
			int status;
			
			if(wait4(pid, &status, WNOHANG, NULL))
				if(WIFEXITED(status))
					return kSlowChildExited;
		}
	}
	
	return kSlowStopped;
}

void ShowVersion()
{
	printf("slow v%s\n", SLOWVERSION);
	printf("Copyright %s %s <%s>\n", SLOWCOPYDATE, SLOWAUTHOR, SLOWAUTHORWEBSITE);
	printf("%s\n", SLOWWEBSITE);
}

void ShowHelp()
{
	printf("slow [-p pid] [-d dutyCycle] [-s timeSlice] [program [args]]\n");
	printf("\t-p pid: Specify the pid of the process to slow; if not present, you must specify a process to run at the end of the command line\n");
	printf("\t-d dutyCycle: 0 < dutyCycle < 1 - portion of time to allow process to run (default: 0.5)\n");
	printf("\t-t timeSlice: The length of one stop/run cycle (default: 1 second)\n");
	printf("\t-v: version info\n");
	printf("\t-h, -?7: help\n");
}

int main(int argc, char* argv[])
{
	pid_t pid = 0;
	int childWait = 0;
	float dutyCycle = 0.5;
	float timeSlice = 1;
	int ch;
	int ok;
	
	while(-1 != (ch = getopt(argc, argv, "h?vp:d:t:")))
	{
		switch(ch)
		{
			case 'p':
				ok = sscanf(optarg, "%d", &pid);
				if(!ok || pid <= 1)
				{
					printf("slow: invalid pid\n");
					exit(2);
				}
				break;
			
			case 'd':
				ok = sscanf(optarg, "%f", &dutyCycle);
				if(!ok || dutyCycle <= 0 || dutyCycle > 1)
				{
					printf("slow: dutyCycle must be between 0 and 1\n");
					exit(2);
				}
				break;
			
			case 't':
				ok = sscanf(optarg, "%f", &timeSlice);
				if(!ok || timeSlice <= 0)
				{
					printf("slow: timeSlice must be greater than 0 seconds\n");
					exit(2);
				}
				break;
			
			case 'v':
				ShowVersion();
				exit(1);
			
			case 'h':
			case '?':
			default:
				ShowHelp();
				exit(1);
		}
	}
	
	if(!pid)
	{
		if(argc - optind > 0)
		{
			childWait = 1;
			if(!(pid = fork()))
			{
				//run the child process
				execvp(argv[optind], &argv[optind]);
				exit(1);
			}
		}
	}
	
	if(pid > 0)
	{
		int err = slow(pid, dutyCycle, timeSlice, childWait);
		
		switch(err)
		{
			case kSlowStopped:
				break;
			
			case kSlowWatchMissing:
				//printf("slow: target process missing\n");
				break;
			
			case kSlowChildExited:
				//printf("slow: child process exited\n");
				break;
			
			default:
				printf("slow: unknown error #%d\n", err);
		}
	}
	else
		printf("slow: no process specified\n");
}
