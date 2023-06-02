#pragma once

#include "utilities/utilities.hpp"

#include <optional>
#include <vector>
#include <mutex>
#include <functional>

#define DEBUGGER_NOTATTACHED -1
#define DEBUGGER_WAITING 0
#define DEBUGGER_SHOULD_ATTACH 1
#define DEBUGGER_SHOULD_DETACH 2
#define DEBUGGER_STEP 3
#define DEBUGGER_CONTINUE 4
#define DEBUGGER_BROKEN 5

namespace debugger
{
	struct break_point_t
	{
		enum class type_t
		{
			SOFTWARE_BREAKPOINT,
			HARDWARE_BREAKPOINT
		} type;

		std::uintptr_t address;

		std::uint8_t old_byte; /* For software breakpoints as we must replace the byte with a 0xCC (int3) */
	};

	class c_debugger
	{
		int pid = 0;

		std::mutex m_breakpoint_mutex;
		std::vector<break_point_t> m_breakpoints;

		std::mutex m_context_mutex;
		CONTEXT m_context;

		std::uintptr_t m_needs_restore = 0;
	public:
		std::atomic_int m_instruction = DEBUGGER_NOTATTACHED;

		std::vector<break_point_t> get_breakpoints();
			
		void add_breakpoint(break_point_t::type_t type, std::uintptr_t address);
		bool is_active_breakpoint(std::uintptr_t address);
		std::optional<break_point_t> get_active_breakpoint(std::uintptr_t address);

		void attach(const utilities::window_result_t& window);
		void detach();

		void handle_bp(std::uintptr_t exception_address);

		void wait_for_status(int instruction, std::function<void()> callback);

		void core_debugger_thread();

		CONTEXT get_context();
		void set_context(CONTEXT context);

		DEBUG_EVENT m_event;
	} inline g_debugger;
}