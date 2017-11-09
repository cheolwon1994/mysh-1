#include <signal.h>
#include <unistd.h>

#include "signal_handlers.h"

void catch_sigint(int sig)
{
	if(sig==SIGINT)
		puts("CTRL+C pressed");
}

void catch_sigtstp(int sig);
{
	if(sig==SIGTSTP)
		puts("CTRL+Z pressed");
}
