#include <signal.h>
#include <sporks/bot.h>

bool reload = false;

/**
 * Set up signals to ignore some common non-fatals.
 * We'll use SIGHUP for commandline rehash.
 */
void Bot::SetSignals()
{
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, &Bot::SetSignal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGXFSZ, SIG_IGN);
}

/**
 * It's not recommended to do a lot inside a signal handler,
 * so set a flag and look out for this to do a reload.
 * Reloading via SIGHUP isn't actually implemented yet,
 * so this flag is ignored by the rest of the program.
 */
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

