#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

volatile int terminate = 0;

//end the daemon
static void termination(int signum)
{
	terminate = 1;
}

static void signal_handler()
{
	signal(SIGINT, termination);
	signal(SIGTERM, termination);
}

//main  function of daemon
static void loop()
{
	while (!terminate)
	{
		sleep(1);
	}
}

//close non standard file descriptors
static int close_nfds()
{
	unsigned int i;
	struct rlimit rlim;
	int num_fds = getrlimit(RLIMIT_NOFILE, &rlim);
	if (num_fds == -1)
	{
		return 0;
	}
	for (i = 3; i< num_fds; i++)
	{
		close(i);
	}
	return 1;
}

//resetting all signal handlers
static int reset_sig_handler()
{
	unsigned int i;
	for (i = 1; i<_NSIG; i++)
	{
		if (i != SIGKILL && i != SIGSTOP)
		{
			signal(i, SIG_DFL);
		}
	}
	return 1;
}

//clear signal mask
static int clear_sig_mask()
{
	sigset_t set;
	return ((sigemptyset(&set) == 0)
	 	&& (sigprocmask(SIG_SETMASK, &set, NULL) == 0));
}

//making all standard file descriptors null
static int make_sfds_null()
{
	int null_fd_read, null_fd_write;
	return (((null_fd_read = open("/dev/null", O_RDONLY)) != -1)
	&& (dup2(null_fd_read, STDIN_FILENO) != -1)
	&& ((null_fd_write = open("/dev/null", O_WRONLY)) != -1)
	&& (dup2(null_fd_write, STDOUT_FILENO) != -1)
	&& (dup2(null_fd_write, STDERR_FILENO) != -1));
}

//creating file
static int pid_file(const char *pid_f)
{
	pid_t pid = getpid();
	char pid_str[1000];
	int fds;
	sprintf(pid_str, "%d", pid);
	if ((fds = open(pid_f, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) == -1)
	{
		return 0;
	}
	if (write(fds, pid_str, strlen(pid_str)) == -1)
	{
		return 0;
	}
	close(fds);
	return 1;
}

//sending notification
static void notification(int wrtfds,char message)
{
	char byte = message;
	while(write(wrtfds, &byte, 1) == 0);
	close(wrtfds);
}

//waiting for notification
static char wait_notification(int rdfds)
{
	char buf[10];
	ssize_t bytes_read = read(rdfds, buf, 1);
	return buf[0];
}

int main()
{
	close_nfds();
	reset_sig_handler();
	clear_sig_mask();
	int pipefds[2];
	if (pipe(pipefds) == -1)
	{
		//creting a pipe
		printf("error pipe");
	}
	else
	{
		pid_t pid1 = fork();
		if (pid1 == -1)
		{
			//creating first fork
			printf("fork error");
			exit(0);
		}
		else if (pid1 == 0)
		{
			close(pipefds[0]);
			//changing the session
			if (setsid() == -1)
			{
				notification(pipefds[1],'0');
				exit(0);
			}
			pid_t pid2 = fork();
			if (pid2 == -1)
			{
				//creating second fork
				printf("error fork");
				notification(pipefds[1],'0');
				exit(0);
			}
			else if (pid2 == 0)
			{
				const char *pid_f = "/home/tejas/daemon.txt";
				make_sfds_null();
				umask(0);
				if (chdir("/") == -1)
				{
					printf("Could not change to root directoey");
					notification(pipefds[1],'0');
					exit(0);
				}
				pid_file(pid_f);
				notification(pipefds[1],'1');
				openlog("DAEMON", LOG_NDELAY,LOG_USER);
				syslog(LOG_INFO, "daemon started. PID: %ld", (long)getpid());
				signal_handler();
				loop();
				unlink(pid_f);
			}
			else
			{
				exit(0);
			}
		}
		else
		{
			close(pipefds[1]);
			char exit_status = wait_notification(pipefds[0]);
			close(pipefds[0]);
			printf("%c",exit_status);
		}
	}
}
