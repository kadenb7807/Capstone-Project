#include <iostream>

#include "interface/interface.hpp"
#include "debugger/debugger.hpp"

#include <thread>

int main()
{
	/* Start the interface thread */
	std::thread{ &interface_t::start, g_interface }.detach();

	/* Start the core debugger loop */
	debugger::g_debugger.core_debugger_thread();

	return 0;
}