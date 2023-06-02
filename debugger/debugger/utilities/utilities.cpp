#include "utilities.hpp"

#include <memory>

int __stdcall enum_windows_func(HWND window, LPARAM lparam)
{
	const auto window_names = reinterpret_cast<std::vector<utilities::window_result_t>*>(lparam);

	if (const auto length = GetWindowTextLengthA(window); length && IsWindowVisible(window))
	{
		const auto buffer = std::make_unique<char[]>(static_cast<std::uint64_t>(length) + 1); // + 1 to account for the null terminator '/0'

		GetWindowTextA(window, buffer.get(), length + 1);

		DWORD pid = 0;
		const auto tid = GetWindowThreadProcessId(window, &pid);

		std::unique_ptr<std::remove_pointer_t< HANDLE >, decltype(&CloseHandle) > handle(OpenProcess(PROCESS_ALL_ACCESS, false, pid), CloseHandle);

		if (utilities::is_supported(handle.get()))
		{
			utilities::window_result_t window_result
			{
				.name = std::string{ buffer.get(), length + 1u },
				.pid = pid,
				.tid = tid,
				.window = window
			};

			window_names->push_back(window_result);
		}
	}

	return 1;
}


std::vector<utilities::window_result_t> utilities::get_windows()
{
	std::vector<utilities::window_result_t> windows;
	EnumWindows(enum_windows_func, reinterpret_cast<std::uintptr_t>(&windows));

	return windows;
}

bool utilities::is_supported(HANDLE handle) // Verify that process is 64bit
{
	int res = false;
	IsWow64Process(handle, &res);

	return res == 0;
}