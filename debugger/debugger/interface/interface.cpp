#include "interface.hpp"

#include "menu/menu.hpp"
#include "utilities/utilities.hpp"

Microsoft::WRL::ComPtr< ID3D11Device > global_device;
Microsoft::WRL::ComPtr< ID3D11DeviceContext > global_device_context;
Microsoft::WRL::ComPtr< IDXGISwapChain > global_swap_chain;
Microsoft::WRL::ComPtr< ID3D11RenderTargetView > render_target_view;

void interface_t::create_render_target()
{
    ID3D11Texture2D* Texture2D;
    global_swap_chain->GetBuffer(0, IID_PPV_ARGS(&Texture2D));

    if (Texture2D)
        global_device->CreateRenderTargetView(Texture2D, nullptr, &render_target_view);

    Texture2D->Release();
}

void interface_t::reset_render_target()
{
    render_target_view.Reset();
}

bool interface_t::create_device(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC DXGI_DESC1;
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    ZeroMemory(&DXGI_DESC1, sizeof DXGI_DESC1);
    DXGI_DESC1.BufferCount = 2;
    DXGI_DESC1.BufferDesc.Width = 0;
    DXGI_DESC1.BufferDesc.Height = 0;
    DXGI_DESC1.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_DESC1.BufferDesc.RefreshRate.Numerator = 60;
    DXGI_DESC1.BufferDesc.RefreshRate.Denominator = 1;
    DXGI_DESC1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    DXGI_DESC1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    DXGI_DESC1.OutputWindow = hwnd;
    DXGI_DESC1.SampleDesc.Count = 1;
    DXGI_DESC1.SampleDesc.Quality = 0;
    DXGI_DESC1.Windowed = TRUE;
    DXGI_DESC1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    if (FAILED(
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0u, feature_level_array, 2, D3D11_SDK_VERSION,
            &DXGI_DESC1, &global_swap_chain, &global_device, &feature_level, &global_device_context)
    ))
        return false;

    create_render_target();

    return true;
}

void interface_t::reset_device()
{
    reset_render_target();

    global_swap_chain.Reset(); global_device_context.Reset(); global_device.Reset();
}

LRESULT __stdcall interface_t::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (global_swap_chain)
        {
            g_interface.reset_render_target();

            global_swap_chain->ResizeBuffers(0u, (UINT)LOWORD(lparam), (UINT)HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0u);

            g_interface.create_render_target();
        }
        return 0u;
    case WM_SYSCOMMAND:
        if ((wparam & 0xFFF0) == SC_KEYMENU)
            return 0u;

        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0u;
    default:
        break;
    }

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return S_OK;

    return ::DefWindowProc(hwnd, msg, wparam, lparam);
}

void interface_t::set_up(HWND window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    auto& style = ImGui::GetStyle();
    if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 4.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(global_device.Get(), global_device_context.Get());
}

void interface_t::clean_up(HWND overlay_window, WNDCLASS& window_class)
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    reset_device();

    ::DestroyWindow(overlay_window);
    ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);
}

void interface_t::render_imgui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    menu::draw();

    ImGui::EndFrame();
    ImGui::Render();

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    global_device_context->OMSetRenderTargets(1, &render_target_view, nullptr);
    global_device_context->ClearRenderTargetView(render_target_view.Get(), clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void interface_t::start()
{
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASS window_class = { };
    window_class.lpszClassName = L"Debugger";
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = wndproc;

    if (FAILED(RegisterClass(&window_class)))
        std::cout << "Failed to register window class." << std::endl;

    auto overlay_window = CreateWindowExW(0u, window_class.lpszClassName,
        L"Test", WS_OVERLAPPEDWINDOW, 100u, 100u, 50u, 50u, nullptr, nullptr, nullptr, nullptr);

    if (!create_device(overlay_window)) {
        return;
    }

    ::ShowWindow(overlay_window, SW_HIDE);
    ::UpdateWindow(overlay_window);

    set_up(overlay_window);

    MSG msg{ };

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0u, 0u, PM_REMOVE) != WM_NULL)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        render_imgui();

        global_swap_chain->Present(1, 0);
    }

    clean_up(overlay_window, window_class);
}