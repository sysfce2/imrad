// Generated with ImRAD 0.10-WIP
// github.com/tpecholt/imrad

#include "ui_combo_dlg.h"
#include <cstdint>

ComboDlg comboDlg;


void ComboDlg::Draw()
{
    /// @dpi-info 141.357,1.25
    /// @style imrad
    /// @unit dp
    /// @begin TopWindow
    const float dp = ImRad::GetUserData().dpiScale;
    ID = ImGui::GetID("###ComboDlg");
    ImGui::SetNextWindowSize({ 300*dp, 350*dp }, ImGuiCond_FirstUseEver); //{ 300*dp, 350*dp }
    ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { FLT_MAX, FLT_MAX });
    bool tmpOpen = true;
    if (ImGui::BeginPopupModal(ImRad::Format("{}###ComboDlg", title).c_str(), &tmpOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (ImRad::GetUserData().activeActivity != "")
            ImRad::RenderDimmedBackground(ImRad::GetUserData().WorkRect(), ImRad::GetUserData().dimBgRatio);
        if (modalResult != ImRad::None)
        {
            ImGui::CloseCurrentPopup();
            if (modalResult != ImRad::Cancel)
                callback(modalResult);
        }
        DrawPopups();
        /// @separator

        /// @begin Input
        vb1.BeginLayout();
        ImGui::PushFont(font, (0));
        ImGui::InputTextMultiline("##value", &value, { -1, vb1.GetSize() }, 0);
        if (ImGui::IsItemActive())
            ImRad::GetUserData().imeType = ImRad::ImeText;
        vb1.AddSize(0 * ImGui::GetStyle().ItemSpacing.y, ImRad::VBox::Stretch(1));
        ImGui::PopFont();
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        /// @end Input

        /// @begin Spacer
        hb2.BeginLayout();
        ImRad::Spacing(1);
        ImRad::Dummy({ hb2.GetSize(), 15*dp });
        vb1.AddSize(2 * ImGui::GetStyle().ItemSpacing.y, 15*dp);
        hb2.AddSize(0 * ImGui::GetStyle().ItemSpacing.x, ImRad::HBox::Stretch(1));
        /// @end Spacer

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("OK", { 80*dp, 25*dp }))
        {
            ClosePopup(ImRad::Ok);
        }
        vb1.UpdateSize(0, 25*dp);
        hb2.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 80*dp);
        /// @end Button

        /// @begin Button
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel", { 80*dp, 25*dp }) ||
            ImGui::Shortcut(ImGuiKey_Escape))
        {
            ClosePopup(ImRad::Cancel);
        }
        vb1.UpdateSize(0, 25*dp);
        hb2.AddSize(1 * ImGui::GetStyle().ItemSpacing.x, 80*dp);
        /// @end Button

        /// @separator
        ImGui::EndPopup();
    }
    /// @end TopWindow
}

void ComboDlg::OpenPopup(std::function<void(ImRad::ModalResult)> clb)
{
    callback = clb;
    modalResult = ImRad::None;
    ImRad::GetUserData().dimBgRatio = 1.f;
    IM_ASSERT(ID && "Call Draw at least once to get ID assigned");
    ImGui::OpenPopup(ID);
    Init();
}

void ComboDlg::ClosePopup(ImRad::ModalResult mr)
{
    modalResult = mr;
    ImRad::GetUserData().dimBgRatio = 0.f;
}

void ComboDlg::ResetLayout()
{
    // ImGui::GetCurrentWindow()->HiddenFramesCannotSkipItems = 2;
    vb1.Reset();
    hb2.Reset();
}

void ComboDlg::DrawPopups()
{
    // TODO: Draw dependent popups here
}

void ComboDlg::Init()
{
    // TODO: Add your code here
}
