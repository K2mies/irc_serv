#include "Signals.hpp"

static void  signal_handler( int signal );

void  init_signals(  struct sigaction *sa ){
	sigemptyset(&sa->sa_mask);
	sigaddset(&sa->sa_mask, SIGINT);
	sigaddset(&sa->sa_mask, SIGQUIT);
	sigaddset(&sa->sa_mask, SIGTERM);

	sa->sa_handler = signal_handler;
	sa->sa_flags = 0;

	sigaction(SIGINT, sa, NULL);
	sigaction(SIGQUIT, sa, NULL);
	sigaction(SIGTERM, sa, NULL);

	signal(SIGPIPE, SIG_IGN);

	g_signal = 0;
}

static void  signal_handler( int signal ){
	if (g_signal == 0){
		if (signal == SIGINT)
			g_signal = SIGINT;
		if (signal == SIGQUIT)
			g_signal = SIGQUIT;
		if (signal == SIGTERM)
			g_signal = SIGTERM;
	}
}

bool  check_signals(){
	if (g_signal != 0){
		if (g_signal == SIGTERM)
			std::cerr << "SIGTERM signal caught, server shutdown now.\n";
		if (g_signal == SIGQUIT)
			std::cerr << "SIGQUIT signal caught, server shutdown now.\n";
		if (g_signal == SIGINT)
			std::cerr << "SIGINT signal caught, server shutdown now.\n";
		return (false);
	}
	return (true);
}
