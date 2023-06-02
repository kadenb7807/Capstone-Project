#include "menu.hpp"

#include "utilities/utilities.hpp"
#include "debugger/debugger.hpp"

#include <vector>
#include <string>
#include <format>

void menu::draw()
{
    ImGui::SetNextWindowSize({ 650, 500 }, ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(1.0f);

    static std::vector<std::string> tabs = { "Process", "Debugger" };

    static std::uint32_t open_tab = 0;

    ImGui::Begin("Debugger", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    const auto width = ImGui::GetContentRegionAvail().x / 2 - 4;

    if (ImGui::Button("Step", { 70, 0 }))
    {
        if (debugger::g_debugger.m_instruction = DEBUGGER_BROKEN)
            debugger::g_debugger.m_instruction = DEBUGGER_STEP;
    }
    ImGui::SameLine();
    if(ImGui::Button("Continue", { 70, 0 }))
    {
        if (debugger::g_debugger.m_instruction = DEBUGGER_BROKEN)
            debugger::g_debugger.m_instruction = DEBUGGER_CONTINUE;
    }
    ImGui::SameLine();

    static std::string selected_value = "none";

    static std::uint32_t idx = 0;
    if (ImGui::BeginCombo("Process", selected_value.c_str()))
    {
        const auto& windows = utilities::get_windows();

        for (auto n = 0u; n < windows.size(); n++)
        {
            const bool is_selected = (idx == n);
            if (ImGui::Selectable(windows[n].name.c_str(), is_selected))
            {
                idx = n;
                selected_value = windows[n].name;

                debugger::g_debugger.attach(windows[n]);
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    const auto height = ImGui::GetContentRegionAvail().y - 4;

    {
        ImGui::BeginChild(ImGui::GetID("DebuggerLeftSide"), { width, 0 });

        {
            ImGui::BeginChildFrame(ImGui::GetID("Registers"), { width, height });

            const auto context = debugger::g_debugger.get_context();

            const auto display_reg = [](const char* reg_name, std::uintptr_t reg_value) -> void {
                ImGui::Text(std::format("{}: 0x{:X}", reg_name, reg_value).c_str());

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.f);

                if (ImGui::Button(std::format("Copy##{}", reg_name).c_str()))
                {
                    ImGui::SetClipboardText(std::format("0x{:X}", reg_value).c_str());
                }
            };

            display_reg("Rip", context.Rip);
            display_reg("Rax", context.Rax);
            display_reg("Rcx", context.Rcx);
            display_reg("Rdx", context.Rdx);
            display_reg("Rbx", context.Rbx);
            display_reg("Rsp", context.Rsp);
            display_reg("Rbp", context.Rbp);
            display_reg("Rsi", context.Rsi);
            display_reg("Rdi", context.Rdi);
            display_reg("R8", context.R8);
            display_reg("R9", context.R9);
            display_reg("R10", context.R10);
            display_reg("R11", context.R11);
            display_reg("R12", context.R12);
            display_reg("R13", context.R13);
            display_reg("R14", context.R14);
            display_reg("R15", context.R15);

            ImGui::EndChildFrame();
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    {
        ImGui::BeginChild(ImGui::GetID("DebuggerRightSide"), { width, 0 });

        const auto break_points = debugger::g_debugger.get_breakpoints();

        {
            ImGui::BeginChildFrame(ImGui::GetID("Bps"), { width, height });

            static char buff[255];
            ImGui::InputTextWithHint("##input", "Address", buff, 255);
            ImGui::SameLine();

            const auto btnwidth = ImGui::GetContentRegionAvail().x / 2 - 4;

            if (ImGui::Button("SWBP", { btnwidth, 0 }))
            {
                const auto address = std::stoull(buff, nullptr, 0x10); // TODO: validation

                debugger::g_debugger.add_breakpoint(debugger::break_point_t::type_t::SOFTWARE_BREAKPOINT, address);
            }
            ImGui::SameLine();
            ImGui::Button("HWBP", { btnwidth, 0 });

            for (auto n = 0u; n < break_points.size(); n++)
            {
                ImGui::Text(std::format("0x{:X}: Type-{}", break_points[n].address, break_points[n].type == debugger::break_point_t::type_t::HARDWARE_BREAKPOINT ? "Hardware" : "Software").c_str());

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize("Remove").x + 5.f));

                if (ImGui::Button(std::format("Remove##", n).c_str()))
                {

                }
            }
            ImGui::EndChildFrame();
        }

        ImGui::EndChild();
    }

    ImGui::End();
}