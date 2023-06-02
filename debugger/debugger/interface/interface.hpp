#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <wrl.h>
#include <wrl/client.h>

#include <d3d11.h>

#include <dcomp.h>

#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>

#include <windows.h>
#include <iostream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#pragma comment( lib, "dxgi" )
#pragma comment( lib, "dcomp" )

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class interface_t
{
private:
	bool create_device(HWND hwnd);
	void reset_device();

	void create_render_target();
	void reset_render_target();

	static LRESULT __stdcall wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void set_up(HWND window);
	void clean_up(HWND overlay_window, WNDCLASS& window_class);

	void render_imgui();
public:
	void start();
} inline g_interface;