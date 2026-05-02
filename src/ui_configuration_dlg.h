// Generated with ImRAD 0.10-WIP
// visit https://github.com/tpecholt/imrad

#pragma once
#include "imrad.h"

class ConfigurationDlg
{
public:
    /// @begin interface
    void OpenPopup(std::function<void(ImRad::ModalResult)> clb = [](ImRad::ModalResult){});
    void ClosePopup(ImRad::ModalResult mr = ImRad::Cancel);
    void Draw();

    struct Config {
        int id = -1;
        std::string name;
        std::string style;
        std::string unit;
    };

    std::vector<Config> configs;
    std::vector<std::string> styles;
    std::string defaultStyle;
    std::string defaultUnit;
    /// @end interface

    std::function<bool(const std::string&, const std::string&, std::string&)> copyStyleFun;

private:
    /// @begin impl
    void DrawPopups();
    void ResetLayout();
    void Init();

    void AddButton_Change();
    void RemoveButton_Change();
    void Name_Change();
    void Input_Activated();
    void StyleCombo_DrawItems();
    void AddStyleButton_Change();

    ImGuiID ID = 0;
    ImRad::ModalResult modalResult;
    std::function<void(ImRad::ModalResult)> callback;
    int selRow = 0;
    int curRow = 0;
    bool focusName = false;
    ImRad::VBox vb1;
    ImRad::HBox hb3;
    /// @end impl

    bool CheckName(int i);
    bool Check();
};

extern ConfigurationDlg configurationDlg;
