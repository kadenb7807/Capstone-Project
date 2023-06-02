#include "debugger.hpp"

#include <thread>

void debugger::c_debugger::add_breakpoint(break_point_t::type_t type, std::uintptr_t address)
{
    std::unique_lock locker{ m_breakpoint_mutex };

    if (!m_instruction == DEBUGGER_NOTATTACHED || is_active_breakpoint(address))
        return;

    if (type == break_point_t::type_t::SOFTWARE_BREAKPOINT)
    {
        std::unique_ptr<std::remove_pointer_t< HANDLE >, decltype(&CloseHandle) > handle(OpenProcess(PROCESS_ALL_ACCESS, false, pid), CloseHandle);

        break_point_t break_point{ .address = address };
        ReadProcessMemory(handle.get(), reinterpret_cast<void*>(address), &break_point.old_byte, sizeof(std::uint8_t), nullptr);

        std::uint8_t int3 = 0xCC;
        WriteProcessMemory(handle.get(), reinterpret_cast<void*>(address), &int3, sizeof(int3), nullptr);

        FlushInstructionCache(handle.get(), reinterpret_cast<void*>(address), 1);

        m_breakpoints.push_back(break_point);
    }
}

bool debugger::c_debugger::is_active_breakpoint(std::uintptr_t address)
{
    const auto address_is_in_breakpoints = [address](const break_point_t& break_point)
    {
        return break_point.address == address;
    };

    return std::find_if(m_breakpoints.begin(), m_breakpoints.end(), address_is_in_breakpoints) != m_breakpoints.end();
}

std::vector<debugger::break_point_t> debugger::c_debugger::get_breakpoints()
{
    std::unique_lock locker{ m_breakpoint_mutex };

    const auto copy = m_breakpoints;

    return copy;
}

std::optional<debugger::break_point_t> debugger::c_debugger::get_active_breakpoint(std::uintptr_t address)
{
    for (const auto& break_point : m_breakpoints)
    {
        if (break_point.address == address)
            return break_point;
    }

    return std::nullopt;
}

void debugger::c_debugger::handle_bp(std::uintptr_t exception_address)
{
    if (const auto& break_point = get_active_breakpoint(exception_address))
    {
        using smart_handle_t = std::unique_ptr<std::remove_pointer_t< HANDLE >, decltype(&CloseHandle) >;

        smart_handle_t thread(OpenThread(THREAD_ALL_ACCESS, false, m_event.dwThreadId), CloseHandle);
        smart_handle_t handle(OpenProcess(PROCESS_ALL_ACCESS, false, m_event.dwProcessId), CloseHandle);

        CONTEXT context;
        context.ContextFlags = CONTEXT_ALL;
        GetThreadContext(thread.get(), &context);

        set_context(context);

        m_instruction = DEBUGGER_BROKEN;

        while (m_instruction != DEBUGGER_CONTINUE && m_instruction != DEBUGGER_STEP)
        {
            if (m_instruction == DEBUGGER_SHOULD_DETACH)
                return;
        }

        context.EFlags |= 0x100; /* TRAP (single step) flag */

        context.Rip--;
        SetThreadContext(thread.get(), &context);

        WriteProcessMemory(handle.get(), reinterpret_cast<void*>(exception_address), &break_point->old_byte, sizeof(std::uint8_t), nullptr);
        FlushInstructionCache(handle.get(), reinterpret_cast<void*>(exception_address), 1);

        m_needs_restore = exception_address;
    }
}

void debugger::c_debugger::wait_for_status(int instruction, std::function<void()> func)
{
    while (instruction != m_instruction) {}

    func();
}

void debugger::c_debugger::core_debugger_thread()
{
    while (true)
    {
        const auto func = [&]() {

            std::printf("Waiting for attach\n");
            wait_for_status(DEBUGGER_SHOULD_ATTACH, [this]() {});

            if (!DebugActiveProcess(pid))
            {
                std::printf("Failed to attach to process\n");

                m_instruction = DEBUGGER_NOTATTACHED;

                return;
            }

            m_instruction = DEBUGGER_WAITING;

            std::printf("Attached\n");

            while (true)
            {
                if (!WaitForDebugEvent(&g_debugger.m_event, 0))
                {
                    if (m_instruction == DEBUGGER_SHOULD_DETACH)
                    {
                        if (!DebugActiveProcessStop(pid))
                        {
                            std::printf("Failed detach\n");
                        }

                        m_instruction = DEBUGGER_NOTATTACHED;

                        return;
                    }

                    continue;
                }

                switch (g_debugger.m_event.dwDebugEventCode)
                {
                case EXCEPTION_DEBUG_EVENT:
                {
                    switch (g_debugger.m_event.u.Exception.ExceptionRecord.ExceptionCode)
                    {
                    case EXCEPTION_SINGLE_STEP:
                    {
                        std::printf("Got single step\n");

                        using smart_handle_t = std::unique_ptr<std::remove_pointer_t< HANDLE >, decltype(&CloseHandle) >;

                        smart_handle_t thread(OpenThread(THREAD_ALL_ACCESS, false, m_event.dwThreadId), CloseHandle);

                        CONTEXT context;
                        context.ContextFlags = CONTEXT_ALL;
                        GetThreadContext(thread.get(), &context);

                        set_context(context);

                        if (m_needs_restore)
                        {
                            smart_handle_t handle(OpenProcess(PROCESS_ALL_ACCESS, false, m_event.dwProcessId), CloseHandle);

                            std::uint8_t int3 = 0xCC;
                            WriteProcessMemory(handle.get(), reinterpret_cast<void*>(m_needs_restore), &int3, sizeof(int3), nullptr);

                            FlushInstructionCache(handle.get(), reinterpret_cast<void*>(m_needs_restore), 1);

                            m_needs_restore = 0;
                        }

                        if (m_instruction == DEBUGGER_STEP)
                        {
                            m_instruction = DEBUGGER_BROKEN;

                            while (m_instruction != DEBUGGER_CONTINUE && m_instruction != DEBUGGER_STEP)
                            {
                                if (m_instruction == DEBUGGER_SHOULD_DETACH)
                                    return;
                            }

                            if (m_instruction == DEBUGGER_STEP)
                            {
                                context.EFlags |= 0x100; /* TRAP (single step) flag */

                                SetThreadContext(thread.get(), &context);
                            }
                        }

                        break;
                    }
                    case EXCEPTION_BREAKPOINT:
                    {
                        std::printf("Got debug event\n");

                        const auto address = g_debugger.m_event.u.Exception.ExceptionRecord.ExceptionAddress;

                        if (g_debugger.is_active_breakpoint(reinterpret_cast<std::uintptr_t>(address)))
                            g_debugger.handle_bp(reinterpret_cast<std::uintptr_t>(address));

                        break;
                    }
                    default:
                        break;
                    }

                    break;
                }
                case EXIT_PROCESS_DEBUG_EVENT:
                {
                    std::printf("stopping debug\n");
                    ContinueDebugEvent(g_debugger.m_event.dwProcessId, g_debugger.m_event.dwThreadId, DBG_CONTINUE); /* If the continued thread previously reported an EXIT_THREAD_DEBUG_EVENT debugging event,
                                                                                                                        ContinueDebugEvent closes the handle the debugger has to the thread. If the continued thread previously reported an EXIT_PROCESS_DEBUG_EVENT debugging event,
                                                                                                                        ContinueDebugEvent closes the handles the debugger has to the process and to the thread. */

                    return; /* Break out of the while loop so thread isnt leaked */
                }
                }
                ContinueDebugEvent(g_debugger.m_event.dwProcessId, g_debugger.m_event.dwThreadId, DBG_CONTINUE);
            }
        };

        func();
    }
}

CONTEXT debugger::c_debugger::get_context()
{
    std::unique_lock locker{ m_context_mutex };

    const auto context = m_context;

    return context;
}

void debugger::c_debugger::set_context(CONTEXT context)
{
    std::unique_lock locker{ m_context_mutex };

    m_context = context;
}

void debugger::c_debugger::attach(const utilities::window_result_t& window)
{
    if (m_instruction != DEBUGGER_NOTATTACHED)
        detach();

    std::printf("Attaching\n");

    pid = window.pid;

    m_instruction = DEBUGGER_SHOULD_ATTACH;

    wait_for_status(DEBUGGER_WAITING, [this]() {});
}

void debugger::c_debugger::detach()
{
    std::printf("Detaching\n");

    m_instruction = DEBUGGER_SHOULD_DETACH;

    wait_for_status(DEBUGGER_NOTATTACHED, []() { });

    std::printf("Detached\n");
}


