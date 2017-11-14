#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "signal_handlers.h"

void catch_sigint(int signalNo)
{
	signal(SIGINT,SIG_IGN);
	fprintf(stderr,"\tCTRL+C를 입력하셨습니다.\n");
}

void catch_sigtstp(int signalNo)
{
	signal(SIGTSTP,SIG_IGN);
	fprintf(stderr,"\tCTRL+Z를 입력하셨습니다.\n");
}
