#pragma once

#include <vector>
#include <string>

#include <Windows.h>

namespace utilities
{
	struct window_result_t
	{
		std::string name;
		std::uint32_t pid, tid;
		HWND window;
	};

	std::vector<window_result_t> get_windows();

	bool is_supported(HANDLE handle);
}