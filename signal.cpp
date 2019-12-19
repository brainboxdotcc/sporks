#include <signal.h>
#include <sporks/bot.h>

bool reload = false;

void Bot::SetSignals()
{
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, &Bot::SetSignal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGXFSZ, SIG_IGN);
}

void Bot::SetSignal(int signal)
{
	switch (signal) {
		case SIGHUP:
			reload = true;
		break;
		default:
		break;
	}
}

