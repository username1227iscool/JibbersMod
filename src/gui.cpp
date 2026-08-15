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
            ImVec4(0.910f, 0.004f, 0.004f, 1.00f);

        const ImVec4 accentHover =
            ImVec4(0.945f, 0.180f, 0.180f, 1.00f);

        const ImVec4 accentDim =
            ImVec4(0.910f, 0.004f, 0.004f, 0.35f);

        const ImVec4 accentSoft =
            ImVec4(0.910f, 0.004f, 0.004f, 0.55f);


        ImGuiStyle& style =
            ImGui::GetStyle();

        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;

        style.WindowBorderSize =
            1.0f;

        style.WindowPadding =
            ImVec2(10.0f, 10.0f);

        style.FramePadding =
            ImVec2(8.0f, 4.0f);

        style.ItemSpacing =
            ImVec2(8.0f, 6.0f);


        ImVec4* c =
            style.Colors;

        c[ImGuiCol_Text] =
            ImVec4(
                1.00f,
                1.00f,
                1.00f,
                1.00f
            );

        c[ImGuiCol_TextDisabled] =
            ImVec4(
                0.45f,
                0.45f,
                0.45f,
                1.00f
            );

        c[ImGuiCol_WindowBg] =
            ImVec4(
                0.03f,
                0.03f,
                0.03f,
                1.00f
            );

        c[ImGuiCol_PopupBg] =
            ImVec4(
                0.05f,
                0.05f,
                0.05f,
                1.00f
            );

        c[ImGuiCol_Border] =
            ImVec4(
                0.16f,
                0.16f,
                0.16f,
                1.00f
            );

        c[ImGuiCol_FrameBg] =
            ImVec4(
                0.10f,
                0.10f,
                0.10f,
                1.00f
            );

        c[ImGuiCol_FrameBgHovered] =
            accentDim;

        c[ImGuiCol_FrameBgActive] =
            accentSoft;

        c[ImGuiCol_TitleBg] =
            ImVec4(
                0.02f,
                0.02f,
                0.02f,
                1.00f
            );

        c[ImGuiCol_TitleBgActive] =
            ImVec4(
                0.02f,
                0.02f,
                0.02f,
                1.00f
            );

        c[ImGuiCol_ScrollbarGrabActive] =
            accent;

        c[ImGuiCol_CheckMark] =
            accent;

        c[ImGuiCol_SliderGrab] =
            accent;

        c[ImGuiCol_SliderGrabActive] =
            accentHover;

        c[ImGuiCol_Button] =
            ImVec4(
                0.12f,
                0.12f,
                0.12f,
                1.00f
            );

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
            ImVec4(
                0.16f,
                0.16f,
                0.16f,
                1.00f
            );

        c[ImGuiCol_TextSelectedBg] =
            accentSoft;
    }


    // ========================================================================
    // KEY EDGE DETECTION
    // ========================================================================

    bool KeyPressedEdge(
        int vk,
        bool& previous)
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

    void HandleTriggers()
    {
        static bool keyWasDown =
            false;

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
                (GetAsyncKeyState(
                    kToggleKey
                ) & 0x8000) != 0;

            bool active =
                padHeld || keyHeld;

            for (size_t i = 0;
                i < VarCtl::Count();
                ++i)
            {
                VarCtl::SetActiveAt(
                    i,
                    active
                );
            }

            Pad::ConsumePress(bound);

            keyWasDown =
                keyHeld;

            return;
        }


        // --------------------------------------------------------------------
        // TOGGLE MODE
        // --------------------------------------------------------------------

        bool fired =
            Pad::ConsumePress(bound);

        if (KeyPressedEdge(
            kToggleKey,
            keyWasDown))
        {
            fired = true;
        }

        if (fired)
        {
            bool current =
                VarCtl::Count() > 0 &&
                VarCtl::IsActiveAt(0);

            bool newState =
                !current;

            for (size_t i = 0;
                i < VarCtl::Count();
                ++i)
            {
                VarCtl::SetActiveAt(
                    i,
                    newState
                );
            }
        }
    }


    // ========================================================================
    // CONTROLLER REBIND
    // ========================================================================

    void HandleRebind()
    {
        if (uint32_t captured =
            Pad::TakeCaptured())
        {
            Pad::g_boundButton.store(
                captured
            );
        }
    }


    // ========================================================================
    // DRAW FLOAT
    // ========================================================================

    void DrawFloatControl(
        size_t index,
        const char* name)
    {
        ImGui::Text(
            "current %.3f",
            VarCtl::CurrentValueAt(index)
        );

        bool active =
            VarCtl::IsActiveAt(index);

        std::string overrideLabel =
            std::string(name) +
            " override";

        if (ImGui::Checkbox(
            overrideLabel.c_str(),
            &active
        ))
        {
            VarCtl::SetActiveAt(
                index,
                active
            );
        }

        // IMPORTANT:
        // ShowSliderAt controls visibility independently from FoundAt.
        // This prevents a missing/unresolved field from making the slider
        // disappear completely.
        if (VarCtl::ShowSliderAt(index))
        {
            ImGui::SetNextItemWidth(
                220.0f
            );

            ImGui::SliderFloat(
                "##float",
                VarCtl::OverridePtrAt(index),
                VarCtl::LowerLimitAt(index),
                VarCtl::UpperLimitAt(index),
                "%.3f"
            );
        }
    }


    // ========================================================================
    // DRAW INT
    // ========================================================================

    void DrawIntControl(
        size_t index,
        const char* name)
    {
        ImGui::Text(
            "current %d",
            VarCtl::CurrentIntAt(index)
        );

        bool active =
            VarCtl::IsActiveAt(index);

        std::string overrideLabel =
            std::string(name) +
            " override";

        if (ImGui::Checkbox(
            overrideLabel.c_str(),
            &active
        ))
        {
            VarCtl::SetActiveAt(
                index,
                active
            );
        }

        if (VarCtl::ShowSliderAt(index))
        {
            ImGui::SetNextItemWidth(
                220.0f
            );

            ImGui::SliderInt(
                "##int",
                VarCtl::OverrideIntPtrAt(index),
                static_cast<int>(
                    VarCtl::LowerLimitAt(index)
                    ),
                static_cast<int>(
                    VarCtl::UpperLimitAt(index)
                    )
            );
        }
    }


    // ========================================================================
    // DRAW BOOL
    // ========================================================================

    void DrawBoolControl(
        size_t index,
        const char* name)
    {
        ImGui::Text(
            "current %s",
            VarCtl::CurrentBoolAt(index)
            ? "true"
            : "false"
        );

        bool active =
            VarCtl::IsActiveAt(index);

        std::string overrideLabel =
            std::string(name) +
            " override";

        if (ImGui::Checkbox(
            overrideLabel.c_str(),
            &active
        ))
        {
            VarCtl::SetActiveAt(
                index,
                active
            );
        }

        bool value =
            VarCtl::OverrideBoolAt(index);

        if (ImGui::Checkbox(
            "value",
            &value
        ))
        {
            VarCtl::SetOverrideBoolAt(
                index,
                value
            );
        }
    }


    // ========================================================================
    // DRAW VECTOR3
    // ========================================================================

    void DrawVector3Control(
        size_t index,
        const char* name)
    {
        VarCtl::Vector3 current =
            VarCtl::CurrentVector3At(index);

        ImGui::Text(
            "current X %.3f",
            current.x
        );
        ImGui::Text(
            "Y %.3f",
            current.y
        );
        ImGui::Text(
            "Z %.3f",
            current.z
        );

        bool active =
            VarCtl::IsActiveAt(index);

        std::string overrideLabel =
            std::string(name) +
            " override";

        if (ImGui::Checkbox(
            overrideLabel.c_str(),
            &active
        ))
        {
            VarCtl::SetActiveAt(
                index,
                active
            );
        }

        if (VarCtl::ShowSliderAt(index))
        {
            ImGui::SetNextItemWidth(
                220.0f
            );

            VarCtl::Vector3 value =
                VarCtl::OverrideVector3At(index);

            float values[3] =
            {
                value.x,
                value.y,
                value.z
            };

            if (ImGui::SliderFloat3(
                "##vector3",
                values,
                VarCtl::LowerLimitAt(index),
                VarCtl::UpperLimitAt(index),
                "%.3f"
            ))
            {
                VarCtl::SetOverrideVector3At(
                    index,
                    {
                        values[0],
                        values[1],
                        values[2]
                    }
                );
            }
        }
    }


    // ========================================================================
    // GUI WINDOW
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

        for (size_t i = 0;
            i < VarCtl::Count();
            ++i)
        {
            ImGui::PushID(
                static_cast<int>(i)
            );

            const char* name =
                VarCtl::NameAt(i);

            ImGui::TextUnformatted(
                name
            );

            ImGui::Separator();


            // ---------------------------------------------------------------
            // Status
            // ---------------------------------------------------------------

            if (!VarCtl::ReadyAt(i))
            {
                ImGui::TextDisabled(
                    "waiting for il2cpp runtime"
                );
            }
            else if (!VarCtl::FoundAt(i))
            {
                ImGui::TextDisabled(
                    "field not found"
                );
            }


            // ---------------------------------------------------------------
            // Draw the requested control regardless of FoundAt.
            //
            // This is the important slider fix:
            // showSlider controls the GUI, while FoundAt only controls
            // whether the actual game field is currently available.
            // ---------------------------------------------------------------

            switch (VarCtl::TypeAt(i))
            {
            case VarCtl::ValueType::Float:
                DrawFloatControl(
                    i,
                    name
                );
                break;

            case VarCtl::ValueType::Int:
                DrawIntControl(
                    i,
                    name
                );
                break;

            case VarCtl::ValueType::Bool:
                DrawBoolControl(
                    i,
                    name
                );
                break;

            case VarCtl::ValueType::Vector3:
                DrawVector3Control(
                    i,
                    name
                );
                break;
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
            ImGui::CalcTextSize(
                "controller button"
            ).x +
            ImGui::GetStyle()
            .ItemSpacing.x * 2.0f;

        int mode =
            g_holdMode ? 1 : 0;

        ImGui::TextUnformatted(
            "mode"
        );

        ImGui::SameLine(
            labelColumn
        );

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
        {
            g_holdMode =
                mode == 1;
        }


        // ====================================================================
        // CONTROLLER BUTTON
        // ====================================================================

        uint32_t bound =
            Pad::g_boundButton.load();

        ImGui::TextUnformatted(
            "controller button"
        );

        ImGui::SameLine(
            labelColumn
        );

        ImGui::SetNextItemWidth(
            110.0f
        );

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
                Pad::g_boundButton.store(
                    0
                );
            }

            for (int i = 0;
                i < Pad::kButtonCount;
                ++i)
            {
                if (ImGui::Selectable(
                    Pad::kButtons[i].name,
                    bound ==
                    Pad::kButtons[i].mask
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
            if (ImGui::Button(
                "input"
            ))
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
