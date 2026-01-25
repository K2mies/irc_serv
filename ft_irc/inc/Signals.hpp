#pragma once

#include <signal.h>
#include <iostream>
#include <stdexcept>

extern volatile sig_atomic_t g_signal;

void  init_signals( struct sigaction *sa );
bool  check_signals();
