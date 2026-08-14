#include "gui.h"
#include "overlay.h"
#include "VarCtl.h"
#include "controller.h"

#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
    constexpr int kToggleKey = VK_F2;

    bool g_holdMode = false;


    // ========================================================================
    // THEME
    // ========================================================================

    void ApplyTheme()
    {
        static bool applied = false;

        if (applied)
            return;

        applied = true;

        const ImVec4 accent =
            ImVec4(0.004f, 0.667f, 0.910f, 1.00f);

        const ImVec4 accentHover =
            ImVec4(0.180f, 0.745f, 0.945f, 1.00f);

        const ImVec4 accentDim =
            ImVec4(0.004f, 0.667f, 0.910f, 0.35f);

        const ImVec4 accentSoft =
            ImVec4(0.004f, 0.667f, 0.910f, 0.55f);


        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;

        style.WindowBorderSize = 1.0f;

        style.WindowPadding =
            ImVec2(10.0f, 10.0f);

        style.FramePadding =
            ImVec2(8.0f, 4.0f);

        style.ItemSpacing =
            ImVec2(8.0f, 6.0f);


        ImVec4* c = style.Colors;

        c[ImGuiCol_Text] =
            ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        c[ImGuiCol_TextDisabled] =
            ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

        c[ImGuiCol_WindowBg] =
            ImVec4(0.03f, 0.03f, 0.03f, 1.00f);

        c[ImGuiCol_PopupBg] =
            ImVec4(0.05f, 0.05f, 0.05f, 1.00f);

        c[ImGuiCol_Border] =
            ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

        c[ImGuiCol_FrameBg] =
            ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

        c[ImGuiCol_FrameBgHovered] =
            accentDim;

        c[ImGuiCol_FrameBgActive] =
            accentSoft;

        c[ImGuiCol_TitleBg] =
            ImVec4(0.02f, 0.02f, 0.02f, 1.00f);

        c[ImGuiCol_TitleBgActive] =
            ImVec4(0.02f, 0.02f, 0.02f, 1.00f);

        c[ImGuiCol_ScrollbarGrabActive] =
            accent;

        c[ImGuiCol_CheckMark] =
            accent;

        c[ImGuiCol_SliderGrab] =
            accent;

        c[ImGuiCol_SliderGrabActive] =
            accentHover;

        c[ImGuiCol_Button] =
            ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

        c[ImGuiCol_ButtonHovered] =
            accentDim;

        c[ImGuiCol_ButtonActive] =
            accentSoft;

        c[ImGuiCol_Header] =
            accentDim;

        c[ImGuiCol_HeaderHovered] =
            accentSoft;

        c[ImGuiCol_HeaderActive] =
            accent;

        c[ImGuiCol_Separator] =
            ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

        c[ImGuiCol_TextSelectedBg] =
            accentSoft;
    }


    // ========================================================================
    // KEY EDGE DETECTION
    // ========================================================================

    bool KeyPressedEdge(int vk, bool& previous)
    {
        bool down =
            vk != 0 &&
            (GetAsyncKeyState(vk) & 0x8000) != 0;

        bool edge =
            down && !previous;

        previous = down;

        return edge;
    }


    // ========================================================================
    // CONTROLLER / KEYBOARD TOGGLES
    // ========================================================================
    // NOTE: this still drives every controlled variable from ONE shared
    // trigger (same behavior as before, when it hardcoded CamDistCtl +
    // CamRotCtl together) -- it now just loops over however many variables
    // VarCtl has instead of naming each one. If you ever want independent
    // per-variable toggles, this is the function to change.

    void HandleTriggers()
    {
        static bool keyWasDown = false;

        uint32_t bound =
            Pad::g_boundButton.load();


        // --------------------------------------------------------------------
        // HOLD MODE
        // --------------------------------------------------------------------

        if (g_holdMode)
        {
            bool padHeld =
                bound != 0 &&
                Pad::IsHeld(bound);

            bool keyHeld =
                (GetAsyncKeyState(kToggleKey) & 0x8000) != 0;


            bool active =
                padHeld || keyHeld;


            for (size_t i = 0; i < VarCtl::Count(); i++)
                VarCtl::SetActiveAt(i, active);


            Pad::ConsumePress(bound);

            keyWasDown = keyHeld;

            return;
        }


        // --------------------------------------------------------------------
        // TOGGLE MODE
        // --------------------------------------------------------------------

        bool fired =
            Pad::ConsumePress(bound);

        if (KeyPressedEdge(kToggleKey, keyWasDown))
            fired = true;


        if (fired)
        {
            bool current =
                VarCtl::Count() > 0 &&
                VarCtl::IsActiveAt(0);

            bool newState = !current;

            for (size_t i = 0; i < VarCtl::Count(); i++)
                VarCtl::SetActiveAt(i, newState);
        }
    }


    // ========================================================================
    // CONTROLLER REBIND
    // ========================================================================

    void HandleRebind()
    {
        if (uint32_t captured = Pad::TakeCaptured())
        {
            Pad::g_boundButton.store(captured);
        }
    }


    // ========================================================================
    // GUI
    // ========================================================================

    void DrawWindow()
    {
        ImGui::Begin(
            "Jumpers mod || Cliff Diving",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        );


        // ====================================================================
        // CONTROLLED VARIABLES
        // ====================================================================
        // One block per VarCtl entry, generated from the list in VarCtl.cpp
        // instead of being hand-written per variable. Adding a variable
        // there automatically gets it a section here, no gui.cpp changes.
        // ====================================================================

        for (size_t i = 0; i < VarCtl::Count(); i++)
        {
            ImGui::PushID(static_cast<int>(i));

            const char* name = VarCtl::NameAt(i);

            ImGui::TextUnformatted(name);

            ImGui::Separator();


            if (!VarCtl::ReadyAt(i))
            {
                ImGui::TextDisabled(
                    "waiting for il2cpp runtime"
                );
            }
            else if (!VarCtl::FoundAt(i))
            {
                ImGui::TextDisabled(
                    "%s field not found",
                    name
                );
            }
            else
            {
                ImGui::Text(
                    "current %.3f",
                    VarCtl::CurrentValueAt(i)
                );


                bool active =
                    VarCtl::IsActiveAt(i);


                if (ImGui::Checkbox(
                    (std::string(name) + " override").c_str(),
                    &active
                ))
                {
                    VarCtl::SetActiveAt(i, active);
                }


                ImGui::SetNextItemWidth(160.0f);


                ImGui::SliderFloat(
                    name,
                    VarCtl::OverridePtrAt(i),
                    VarCtl::LowerLimitAt(i),
                    VarCtl::UpperLimitAt(i)
                );
            }


            ImGui::PopID();

            ImGui::Spacing();
        }


        ImGui::Separator();
        ImGui::Spacing();


        // ====================================================================
        // MODE
        // ====================================================================

        const float labelColumn =
            ImGui::CalcTextSize("controller button").x +
            ImGui::GetStyle().ItemSpacing.x * 2.0f;


        int mode =
            g_holdMode ? 1 : 0;


        ImGui::TextUnformatted("mode");

        ImGui::SameLine(labelColumn);


        bool modeChanged =
            ImGui::RadioButton(
                "toggle",
                &mode,
                0
            );


        ImGui::SameLine();


        modeChanged |=
            ImGui::RadioButton(
                "hold",
                &mode,
                1
            );


        if (modeChanged)
            g_holdMode = mode == 1;


        // ====================================================================
        // CONTROLLER BUTTON
        // ====================================================================

        uint32_t bound =
            Pad::g_boundButton.load();


        ImGui::TextUnformatted(
            "controller button"
        );

        ImGui::SameLine(labelColumn);

        ImGui::SetNextItemWidth(110.0f);


        if (ImGui::BeginCombo(
            "##bind",
            Pad::NameOf(bound)
        ))
        {
            if (ImGui::Selectable(
                "none",
                bound == 0
            ))
            {
                Pad::g_boundButton.store(0);
            }


            for (int i = 0;
                i < Pad::kButtonCount;
                i++)
            {
                if (ImGui::Selectable(
                    Pad::kButtons[i].name,
                    bound == Pad::kButtons[i].mask
                ))
                {
                    Pad::g_boundButton.store(
                        Pad::kButtons[i].mask
                    );
                }
            }


            ImGui::EndCombo();
        }


        ImGui::SameLine();


        if (Pad::Listening())
        {
            if (ImGui::Button(
                "press a button"
            ))
            {
                Pad::CancelListen();
            }
        }
        else
        {
            if (ImGui::Button("input"))
            {
                Pad::Listen();
            }
        }


        ImGui::End();
    }
}


// ============================================================================
// GUI FRAME
// ============================================================================

namespace Gui
{
    void Frame()
    {
        ApplyTheme();

        HandleRebind();

        HandleTriggers();


        VarCtl::Tick();


        if (Overlay::MenuOpen())
        {
            DrawWindow();
        }
    }
}