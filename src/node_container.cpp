#include "node_container.h"
#include "node_window.h"
#include "stx.h"
#include "imrad.h"
#include "cppgen.h"
#include "binding_input.h"
#include "binding_eval.h"
#include "ui_table_cols.h"
#include "ui_message_box.h"
#include <algorithm>
#include <array>

Table::ColumnData::ColumnData()
{
    sizingPolicy.add$(ImGuiTableColumnFlags_None);
    sizingPolicy.add$(ImGuiTableColumnFlags_WidthFixed);
    sizingPolicy.add$(ImGuiTableColumnFlags_WidthStretch);

    flags.add$(ImGuiTableColumnFlags_AngledHeader);
    flags.add$(ImGuiTableColumnFlags_DefaultHide);
    flags.add$(ImGuiTableColumnFlags_DefaultSort);
    //flags.add$(ImGuiTableColumnFlags_Disabled);
    flags.add$(ImGuiTableColumnFlags_NoClip);
    flags.add$(ImGuiTableColumnFlags_NoHeaderLabel);
    flags.add$(ImGuiTableColumnFlags_NoHeaderWidth);
    flags.add$(ImGuiTableColumnFlags_NoHide);
    flags.add$(ImGuiTableColumnFlags_NoResize);
    flags.add$(ImGuiTableColumnFlags_NoSort);
    flags.add$(ImGuiTableColumnFlags_NoSortAscending);
    flags.add$(ImGuiTableColumnFlags_NoSortDescending);
}

Table::ColumnData::ColumnData(const std::string& l, ImGuiTableColumnFlags_ policy, float w)
    : ColumnData()
{
    label = l;
    sizingPolicy = policy;
    width = w;
}

std::string
Table::ColumnData::SizingPolicyString()
{
    if (sizingPolicy & ImGuiTableColumnFlags_WidthFixed)
        return "(Fixed)";
    else if (sizingPolicy & ImGuiTableColumnFlags_WidthStretch)
        return "(Stretch)";
    else
        return "";
}

std::vector<UINode::Prop>
Table::ColumnData::Properties()
{
    return {
        { "behavior.flags", &flags },
        { "behavior.label", &label },
        { "behavior.sizingPolicy", &sizingPolicy },
        { "behavior.width", &width },
        { "behavior.visible", &visible },
    };
}

bool Table::ColumnData::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 1:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    case 2:
        ImGui::Text("sizingPolicy");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = sizingPolicy != Defaults().sizingPolicy ? InputDirectVal_Modified : 0;
        changed = InputDirectValEnum(&sizingPolicy, fl, ctx);
        break;
    case 3:
        ImGui::Text("width");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = width - Defaults().width ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&width, fl, ctx);
        break;
    case 4:
        ImGui::Text("visible");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = visible != Defaults().visible ? InputBindable_Modified : 0;
        changed = InputBindable(&visible, InputBindable_Modified, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("visible", &visible, ctx);
        break;
    }
    return changed;
}

Table::Table(UIContext& ctx)
{
    size_x = -1; //here 0 has the same effect as -1 but -1 works with our sizing visualization
    size_y = 0;

    flags.add$(ImGuiTableFlags_Resizable);
    flags.add$(ImGuiTableFlags_Reorderable);
    flags.add$(ImGuiTableFlags_Hideable);
    flags.add$(ImGuiTableFlags_Sortable);
    flags.add$(ImGuiTableFlags_ContextMenuInBody);
    flags.separator();
    flags.add$(ImGuiTableFlags_RowBg);
    flags.add$(ImGuiTableFlags_BordersInnerH);
    flags.add$(ImGuiTableFlags_BordersInnerV);
    flags.add$(ImGuiTableFlags_BordersOuterH);
    flags.add$(ImGuiTableFlags_BordersOuterV);
    //flags.add$(ImGuiTableFlags_NoBordersInBody);
    //flags.add$(ImGuiTableFlags_NoBordersInBodyUntilResize);
    flags.separator();
    flags.add$(ImGuiTableFlags_SizingFixedFit);
    flags.add$(ImGuiTableFlags_SizingFixedSame);
    //flags.add$(ImGuiTableFlags_SizingStretchProp); combined flag, looks confusing
    flags.add$(ImGuiTableFlags_SizingStretchSame);
    flags.separator();
    flags.add$(ImGuiTableFlags_PadOuterX);
    flags.add$(ImGuiTableFlags_NoPadOuterX);
    flags.add$(ImGuiTableFlags_NoPadInnerX);
    flags.add$(ImGuiTableFlags_ScrollX);
    flags.add$(ImGuiTableFlags_ScrollY);
    flags.add$(ImGuiTableFlags_HighlightHoveredColumn);

    columnData.resize(3);
    for (size_t i = 0; i < columnData.size(); ++i)
        columnData[i].label = std::string(1, (char)('A' + i));
}

std::unique_ptr<Widget> Table::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Table>(*this);
    //rowCount can be shared
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* Table::DoDraw(UIContext& ctx)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (style_cellPadding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, style_cellPadding.eval_px(ctx));
    if (!style_headerBg.empty())
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, style_headerBg.eval(ImGuiCol_TableHeaderBg, ctx));
    if (!style_rowBg.empty())
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, style_rowBg.eval(ImGuiCol_TableRowBg, ctx));
    if (!style_rowBgAlt.empty())
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, style_rowBgAlt.eval(ImGuiCol_TableRowBgAlt, ctx));
    if (!style_childBg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_childBg.eval(ImGuiCol_ChildBg, ctx));

    int n = std::max(1, (int)columnData.size());
    ImVec2 size{ size_x.eval_px(ImGuiAxis_X, ctx), size_y.eval_px(ImGuiAxis_Y, ctx) };
    float rh = rowHeight.eval_px(ImGuiAxis_Y, ctx);
    int fl = flags;
    if (stx::count(ctx.selected, this)) //force columns at design time
        fl |= ImGuiTableFlags_BordersInner;
    std::string name = "table" + std::to_string((uint64_t)this);
    if (ImGui::BeginTable(name.c_str(), n, fl, size))
    {
        //need to override drawList because when table is in a Child mode its drawList will be drawn on top
        drawList = ImGui::GetWindowDrawList();

        for (const auto& cd : columnData) {
            ImGui::TableSetupColumn(cd.label.c_str(), cd.sizingPolicy | cd.flags, cd.width);
            /*if (!cd.visible.empty())
                ImGui::TableSetColumnEnabled(i, cd.visible.eval(ctx));*/
        }
        if (header)
        {
            if (!style_headerFontName.empty() || !style_headerFontSize.empty()) 
                ImGui::PushFont(style_headerFontName.eval(ctx), style_headerFontSize.eval(ctx));
            
            ImGui::TableHeadersRow();
        
            if (!style_headerFontName.empty() || !style_headerFontSize.empty())
                ImGui::PopFont();
        }

        ImGui::TableNextRow(0, rh);
        ImGui::TableSetColumnIndex(0);

        for (const auto& child : child_iterator(children, false))
        {
            child->Draw(ctx);
        }

        int n = itemCount.limit.value();
        for (int r = ImGui::TableGetRowIndex() + 1; r < header + n; ++r)
            ImGui::TableNextRow(0, rh);

        if (child_iterator(children, true))
        {
            //ImGui::GetCurrentWindow()->SkipItems = false;
            auto cpos = ImRad::GetCursorData();
            ImGui::PushClipRect(ImGui::GetCurrentTable()->InnerRect.Min, ImGui::GetCurrentTable()->InnerClipRect.Max, false);
            for (const auto& child : child_iterator(children, true))
            {
                child->Draw(ctx);
            }
            ImGui::PopClipRect();
            ImRad::SetCursorData(cpos);
        }

        ImGui::EndTable();
    }

    if (!style_childBg.empty())
        ImGui::PopStyleColor();
    if (!style_headerBg.empty())
        ImGui::PopStyleColor();
    if (!style_rowBg.empty())
        ImGui::PopStyleColor();
    if (!style_rowBgAlt.empty())
        ImGui::PopStyleColor();
    if (style_cellPadding.has_value())
        ImGui::PopStyleVar();

    return drawList;
}

std::vector<UINode::Prop>
Table::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.headerBg", &style_headerBg },
        { "appearance.rowBg", &style_rowBg },
        { "appearance.rowBgAlt", &style_rowBgAlt },
        { "appearance.childBg", &style_childBg },
        { "appearance.cellPadding", &style_cellPadding },
        { "appearance.rowHeight##table", &rowHeight },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "appearance.hfont.summary", nullptr },
        { "appearance.hfont.name", &style_headerFontName },
        { "appearance.hfont.size", &style_headerFontSize },
        { "appearance.header##table", &header },
        { "behavior.flags##table", &flags },
        { "behavior.columns##table", nullptr },
        { "behavior.rowCount##table", &itemCount.limit },
        { "behavior.rowFilter##table", &rowFilter },
        { "behavior.scrollFreeze.frz##table", nullptr },
        { "behavior.scrollFreeze.x##table", &scrollFreeze_x },
        { "behavior.scrollFreeze.y##table", &scrollFreeze_y },
        { "behavior.scrollWhenDragging", &scrollWhenDragging },
        { "bindings.rowIndex##1", &itemCount.index }
        });
    return props;
}

bool Table::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("headerBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_headerBg, ImGuiCol_TableHeaderBg, ctx);
        break;
    case 1:
        ImGui::Text("rowBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_rowBg, ImGuiCol_TableRowBg, ctx);
        break;
    case 2:
        ImGui::Text("rowBgAlt");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_rowBgAlt, ImGuiCol_TableRowBgAlt, ctx);
        break;
    case 3:
        ImGui::Text("childBg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_childBg, ImGuiCol_ChildBg, ctx);
        break;
    case 4:
        ImGui::Text("cellPadding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_cellPadding, ctx);
        break;
    case 5:
        ImGui::Text("rowHeight");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = rowHeight != Defaults().rowHeight ? InputBindable_Modified : 0;
        changed = InputBindable(&rowHeight, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowHeight", &rowHeight, ctx);
        break;
    case 6:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        TextFontInfo(ctx);
        break;
    case 7:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 8:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 9:
    {
        ImGui::Text("headerFont");
        ImGui::TableNextColumn();
        bool changed = style_headerFontName != Defaults().style_headerFontName ||
            style_headerFontSize != Defaults().style_headerFontSize;
        ::TextFontInfo(style_headerFontName, style_headerFontSize, changed, ctx);
        break;
    }
    case 10:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_headerFontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header_font_name", &style_headerFontName, ctx);
        break;
    case 11:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_headerFontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header_font_size", &style_headerFontSize, ctx);
        break;
    case 12:
        ImGui::Text("showHeader");
        ImGui::TableNextColumn();
        fl = header != Defaults().header ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&header, fl, ctx);
        break;
    case 13:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 14:
    {
        ImGui::Text("columns");
        ImGui::TableNextColumn();
        ImGui::PushFont(ctx.pgbFont);
        std::string label = "[" + std::to_string(columnData.size()) + "]";
        /*ImGui::Text(label.c_str());
        ImGui::PopFont();
        ImGui::SameLine(0, 0);
        ImGui::Dummy(ImGui::CalcItemSize({ -2 * ImGui::GetFrameHeight(), ImGui::GetFrameHeight() }, 0, 0));
        ImGui::SameLine(0, 0);*/
        if (ImRad::Selectable((label + "##columns").c_str(), false, 0, { -ImGui::GetFrameHeight(), 0 }))
        {
            changed = true;
            tableCols.ctx = &ctx;
            tableCols.columns = columnData;
            tableCols.OpenPopup([this](ImRad::ModalResult mr) {
                columnData = tableCols.columns;
                });
        }
        ImGui::PopFont();
        break;
    }
    case 15:
        ImGui::Text("rowCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowCount", &itemCount.limit, ctx);
        break;
    case 16:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("rowFilter");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = rowFilter != Defaults().rowFilter ? InputBindable_Modified : 0;
        changed = InputBindable(&rowFilter, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("rowFilter", &rowFilter, ctx);
        ImGui::EndDisabled();
        break;
    case 17:
    {
        ImGui::Text("scrollFreeze");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        std::string tmp = std::to_string(scrollFreeze_x) + ", " +
            std::to_string(scrollFreeze_y);
        fl = scrollFreeze_x != Defaults().scrollFreeze_x || scrollFreeze_y != Defaults().scrollFreeze_y;
        ImGui::PushFont(fl ? ctx.pgbFont : ctx.pgFont);
        ImGui::TextUnformatted(tmp.c_str());
        ImGui::PopFont();
        break;
    }
    case 18:
        ImGui::Text("columns");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = scrollFreeze_x ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&scrollFreeze_x, fl, ctx);
        break;
    case 19:
        ImGui::Text("rows");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = scrollFreeze_y ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&scrollFreeze_y, fl, ctx);
        break;
    case 20:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&scrollWhenDragging, 0, ctx);
        break;
    case 21:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("rowIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 22, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
Table::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "table.setup", &onSetup },
        { "table.beginRow", &onBeginRow },
        { "table.endRow", &onEndRow },
        });
    return props;
}

bool Table::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("Setup");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Setup", &onSetup, 0, ctx);
        break;
    case 1:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("BeginRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_BeginRow", &onBeginRow, 0, ctx);
        ImGui::EndDisabled();
        break;
    case 2:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("EndRow");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_EndRow", &onEndRow, 0, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::EventUI(i - 3, ctx);
    }
    return changed;
}

void Table::DoExport(std::ostream& os, UIContext& ctx)
{
    if (style_cellPadding.has_value())
    {
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, "
            << style_cellPadding.to_arg(ctx.unit) << ");\n";
    }
    if (!style_headerBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, "
            << style_headerBg.to_arg() << ");\n";
    }
    if (!style_rowBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBg, "
            << style_rowBg.to_arg() << ");\n";
    }
    if (!style_rowBgAlt.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, "
            << style_rowBgAlt.to_arg() << ");\n";
    }
    if (!style_childBg.empty())
    {
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, "
            << style_childBg.to_arg() << ");\n";
    }
    if (scrollWhenDragging)
    {
        os << ctx.ind << "ImRad::PushInvisibleScrollbar();\n";
    }

    os << ctx.ind << "if (ImGui::BeginTable("
        << "\"table" << ctx.varCounter << "\", "
        << columnData.size() << ", "
        << flags.to_arg() << ", "
        << "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", " << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }"
        << "))\n"
        << ctx.ind << "{\n";
    ctx.ind_up();

    if (scrollWhenDragging)
    {
        os << ctx.ind << "ImRad::ScrollWhenDragging(true);\n";
    }

    int hasNoPolicy = 0;
    for (const auto& cd : columnData)
    {
        std::string fl;
        if (!cd.sizingPolicy)
            ++hasNoPolicy;
        else
            fl += cd.sizingPolicy.to_arg() + " | ";
        if (cd.flags)
            fl += cd.flags.to_arg() + " | ";
        if (!cd.visible.empty())
            fl += "(" + cd.visible.to_arg() + " ? 0 : ImGuiTableColumnFlags_Disabled) | ";
        if (fl != "")
            fl.resize(fl.size() - 3);
        else
            fl = "0";

        os << ctx.ind << "ImGui::TableSetupColumn(" << cd.label.to_arg() << ", "
            << fl << ", ";
        if (cd.sizingPolicy == ImGuiTableColumnFlags_WidthFixed) {
            direct_val<dimension_t> dim(cd.width);
            os << dim.to_arg(ctx.unit);
        }
        else
            os << cd.width;
        os << ");\n";
    }
    if (hasNoPolicy && hasNoPolicy != columnData.size())
        PushError(ctx, "either specify sizingPolicy for all columns or none");

    os << ctx.ind << "ImGui::TableSetupScrollFreeze(" << scrollFreeze_x.to_arg() << ", "
        << scrollFreeze_y.to_arg() << ");\n";

    if (!onSetup.empty())
        os << ctx.ind << onSetup.to_arg() << "();\n";

    if (header)
    {
        if (!style_headerFontName.empty() || !style_headerFontSize.empty())
            os << ctx.ind << "ImGui::PushFont(" << style_headerFontName.to_arg() 
            << ", " << style_headerFontSize.to_arg() << ");\n";
        
        os << ctx.ind << "ImGui::TableHeadersRow();\n";
    
        if (!style_headerFontName.empty() || !style_headerFontSize.empty())
            os << ctx.ind << "ImGui::PopFont();\n";
    }

    if (!itemCount.empty())
    {
        std::string contVar = itemCount.container_expr();
        std::string idxVar = itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);

        os << "\n" << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n" << ctx.ind << "{\n";
        ctx.ind_up();

        if (contVar != "")
        {
            os << ctx.ind << "auto& " << ctx.codeGen->CUR_ITEM_VAR_NAME << " = "
                << contVar << "[" << idxVar << "];\n";
        }
        if (!rowFilter.empty())
        {
            os << ctx.ind << "if (!(" << rowFilter.to_arg() << "))\n";
            ctx.ind_up();
            os << ctx.ind << "continue;\n";
            ctx.ind_down();
        }

        os << ctx.ind << "ImGui::PushID(" << idxVar << ");\n";
    }

    os << ctx.ind << "ImGui::TableNextRow(0, ";
    if (!rowHeight.empty())
        os << rowHeight.to_arg(ctx.unit);
    else
        os << "0";
    os << ");\n";
    os << ctx.ind << "ImGui::TableSetColumnIndex(0);\n";

    if (!onBeginRow.empty() && !itemCount.empty())
        os << ctx.ind << onBeginRow.to_arg() << "();\n";

    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : child_iterator(children, false))
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    if (!onEndRow.empty() && !itemCount.empty())
        os << ctx.ind << onEndRow.to_arg() << "();\n";

    if (!itemCount.empty()) {
        os << ctx.ind << "ImGui::PopID();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    if (child_iterator(children, true))
    {
        //draw overlay children at the end so they are visible,
        //inside table's child because ItemOverlap works only between items in same window
        os << ctx.ind << "auto cpos" << ctx.varCounter << " = ImRad::GetCursorData();\n";
        os << ctx.ind << "ImGui::PushClipRect(ImRad::GetParentInnerRect().Min, ImRad::GetParentInnerRect().Max, false);\n";
        //SkipItems is normally reset by TableSetColumnIndex but in case there are no rows...
        os << ctx.ind << "ImGui::GetCurrentWindow()->SkipItems = false;\n";
        os << ctx.ind << "/// @separator\n\n";

        for (auto& child : child_iterator(children, true))
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::PopClipRect();\n";
        os << ctx.ind << "ImRad::SetCursorData(cpos" << ctx.varCounter << ");\n";
    }

    os << ctx.ind << "ImGui::EndTable();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_rowBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_rowBgAlt.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_childBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_headerBg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (style_cellPadding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (scrollWhenDragging)
        os << ctx.ind << "ImRad::PopInvisibleScrollbar();\n";

    ++ctx.varCounter;
}

void Table::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() && sit->params[0] == "ImGuiStyleVar_CellPadding")
            style_cellPadding.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableHeaderBg")
            style_headerBg.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBg")
            style_rowBg.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_TableRowBgAlt")
            style_rowBgAlt.set_from_arg(sit->params[1]);
        if (sit->params.size() && sit->params[0] == "ImGuiCol_ChildBg")
            style_childBg.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTable")
    {
        header = false;
        columnData.clear();
        ctx.importLevel = sit->level;

        /*if (sit->params.size() >= 2) {
            std::istringstream is(sit->params[1]);
            size_t n = 3;
            is >> n;
        }*/

        if (sit->params.size() >= 3) {
            if (!flags.set_from_arg(sit->params[2]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
        }

        if (sit->params.size() >= 4) {
            auto size = cpp::parse_size(sit->params[3]);
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
    {
        scrollWhenDragging = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableSetupColumn")
    {
        ColumnData cd;
        cd.label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2) {
            cd.sizingPolicy.set_from_arg(sit->params[1]);
            cd.flags.set_from_arg(sit->params[1]);

            size_t i = sit->params[1].find("(");
            if (i != std::string::npos) {
                size_t j = sit->params[1].find("? 0 :", i);
                std::string expr = sit->params[1].substr(i + 1, j - i - 1);
                cd.visible.set_from_arg(Trim(expr));
            }
        }

        if (sit->params.size() >= 3) {
            if (cd.sizingPolicy == ImGuiTableColumnFlags_WidthFixed) {
                direct_val<dimension_t> dim = 0;
                dim.set_from_arg(sit->params[2]);
                cd.width = (float)dim;
            }
            else {
                cd.width.set_from_arg(sit->params[2]);
            }
        }

        columnData.push_back(std::move(cd));
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableSetupScrollFreeze")
    {
        if (sit->params.size() >= 1)
            scrollFreeze_x.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2)
            scrollFreeze_y.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushFont")
    {
        if (sit->params.size())
            style_headerFontName.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2)
            style_headerFontSize.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableHeadersRow")
    {
        header = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::TableNextRow")
    {
        if (sit->params.size() >= 2)
            rowHeight.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::ForBlock)
    {
        itemCount.set_from_arg(sit->line);
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 1 &&
        columnData.size() &&
        sit->callee.compare(0, 7, "ImGui::") &&
        sit->callee.compare(0, 7, "ImRad::"))
    {
        onSetup.set_from_arg(sit->callee);
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel + 2 &&
        //columnData.size() &&
        sit->callee.compare(0, 7, "ImGui::") &&
        sit->callee.compare(0, 7, "ImRad::"))
    {
        if (!onBeginRow.empty() || children.size()) //todo
            onEndRow.set_from_arg(sit->callee);
        else
            onBeginRow.set_from_arg(sit->callee);
    }
    else if (sit->kind == cpp::IfStmt && sit->level == ctx.importLevel + 2 &&
        //columnData.size() &&
        !sit->cond.compare(0, 2, "!(") && sit->cond.back() == ')')
    {
        rowFilter.set_from_arg(sit->cond.substr(2, sit->cond.size() - 3));
    }
}

//---------------------------------------------------------

Child::Child(UIContext& ctx)
{
    //zero size will be rendered wrongly?
    //it seems 0 is equivalent to -1 but only if children exist which is confusing
    size_x = size_y = 20;

    flags.add$(ImGuiChildFlags_Borders);
    flags.add$(ImGuiChildFlags_AlwaysUseWindowPadding);
    flags.add$(ImGuiChildFlags_AlwaysAutoResize);
    flags.add$(ImGuiChildFlags_AutoResizeX);
    flags.add$(ImGuiChildFlags_AutoResizeY);
    flags.add$(ImGuiChildFlags_FrameStyle);
    flags.add$(ImGuiChildFlags_NavFlattened);
    flags.add$(ImGuiChildFlags_ResizeX);
    flags.add$(ImGuiChildFlags_ResizeY);

    wflags.add$(ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    wflags.add$(ImGuiWindowFlags_AlwaysVerticalScrollbar);
    wflags.add$(ImGuiWindowFlags_NoBackground);
    wflags.add$(ImGuiWindowFlags_NoNavFocus);
    wflags.add$(ImGuiWindowFlags_NoNavInputs);
    wflags.add$(ImGuiWindowFlags_NoSavedSettings);
    wflags.add$(ImGuiWindowFlags_NoScrollbar);
}

std::unique_ptr<Widget> Child::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Child>(*this);
    //itemCount is shared
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

int Child::Behavior()
{
    int fl = Widget::Behavior() | SnapInterior;
    if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeX))
        fl |= HasSizeX;
    if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) || !(flags & ImGuiChildFlags_AutoResizeY))
        fl |= HasSizeY;
    return fl;
}

ImDrawList* Child::DoDraw(UIContext& ctx)
{
    if (flags & ImGuiWindowFlags_AlwaysAutoResize)
    {
        ctx.isAutoSize = true;
        HashCombineData(ctx.layoutHash, true);
    }

    if (style_padding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
    if (style_rounding.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style_rounding.eval_px(ctx));
    if (style_borderSize.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, style_borderSize.eval_px(ctx));
    if (!style_bg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ImGuiCol_ChildBg, ctx));

    ImVec2 sz;
    sz.x = size_x.eval_px(ImGuiAxis_X, ctx);
    sz.y = size_y.eval_px(ImGuiAxis_Y, ctx);
    if (!sz.x && children.empty())
        sz.x = 30;
    if (!sz.y && children.empty())
        sz.y = 30;

    ImRad::IgnoreWindowPaddingData paddingData;
    if (!style_outerPadding)
        ImRad::PushIgnoreWindowPadding(&sz, &paddingData);

    //after calling BeginChild, win->ContentSize and scrollbars are updated and drawn
    //only way how to disable scrollbars when using !style_outer_padding is to use
    //ImGuiWindowFlags_NoScrollbar

    int wfl = wflags | ImGuiWindowFlags_NoSavedSettings;
    ImGui::SetNextWindowScroll({ 0, 0 }); //click on a child of child causes scrolling which doesn't go back
    ImGui::BeginChild("", sz, flags, wfl);

    if (style_spacing.has_value())
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing);

    if (columnCount.has_value() && columnCount.value() >= 2)
        ImGui::Columns(columnCount.value(), "columns", columnBorder);

    for (const auto& child : child_iterator(children, false))
    {
        child->Draw(ctx);
    }

    auto cpos = ImRad::GetCursorData();
    ImGui::PushClipRect(ImGui::GetCurrentWindow()->InnerRect.Min, ImGui::GetCurrentWindow()->InnerRect.Max, false); //cancels column clip
    for (const auto& child : child_iterator(children, true))
    {
        child->Draw(ctx);
    }
    ImGui::PopClipRect();
    ImRad::SetCursorData(cpos);

    if (style_spacing.has_value())
        ImGui::PopStyleVar();

    ImGui::EndChild();

    if (!style_outerPadding)
        ImRad::PopIgnoreWindowPadding(paddingData);

    if (!style_bg.empty())
        ImGui::PopStyleColor();
    if (style_rounding.has_value())
        ImGui::PopStyleVar();
    if (style_padding.has_value())
        ImGui::PopStyleVar();
    if (style_borderSize.has_value())
        ImGui::PopStyleVar();

    return ImGui::GetWindowDrawList();
}

void Child::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = ImGui::GetItemRectMin();
    cached_size = ImGui::GetItemRectSize();
}

void Child::DoExport(std::ostream& os, UIContext& ctx)
{
    std::string datavar, szvar;
    if (!style_outerPadding)
    {
        datavar = "_data" + std::to_string(ctx.varCounter);
        szvar = "_sz" + std::to_string(ctx.varCounter);
        os << ctx.ind << "ImVec2 " << szvar << "{ "
            << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " };\n";
        os << ctx.ind << "ImRad::IgnoreWindowPaddingData " << datavar << ";\n";
        os << ctx.ind << "ImRad::PushIgnoreWindowPadding(&" << szvar << ", &" << datavar << ");\n";
    }

    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
    if (style_borderSize.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, " << style_borderSize.to_arg(ctx.unit) << ");\n";
    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";

    os << ctx.ind << "ImGui::BeginChild(\"child" << ctx.varCounter << "\", ";
    if (szvar != "")
        os << szvar << ", ";
    else {
        os << "{ " << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
            << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1]) << " }, ";
    }
    os << flags.to_arg() << ", " << wflags.to_arg() << ");\n";

    os << ctx.ind << "{\n";
    ctx.ind_up();

    if (scrollWhenDragging)
        os << ctx.ind << "ImRad::ScrollWhenDragging(false);\n";
    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";

    bool hasColumns = !columnCount.has_value() || columnCount.value() >= 2;
    if (hasColumns)
    {
        os << ctx.ind << "ImGui::Columns(" << columnCount.to_arg() << ", \"\", "
            << columnBorder.to_arg() << ");\n";
        //for (size_t i = 0; i < columnsWidths.size(); ++i)
            //os << ctx.ind << "ImGui::SetColumnWidth(" << i << ", " << columnsWidths[i].c_str() << ");\n";
    }

    if (!itemCount.empty())
    {
        os << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n" << ctx.ind << "{\n";
        ctx.ind_up();

        std::string contVar = itemCount.container_expr();
        std::string idxVar = itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);
        if (contVar != "")
        {
            os << ctx.ind << "auto& " << ctx.codeGen->CUR_ITEM_VAR_NAME << " = "
                << contVar << "[" << idxVar << "];\n";
        }

        os << ctx.ind << "ImGui::PushID(" << idxVar << ");\n";
    }

    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : child_iterator(children, false))
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    if (!itemCount.empty())
    {
        os << ctx.ind << "ImGui::PopID();\n";
        if (hasColumns)
            os << ctx.ind << "ImGui::NextColumn();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";

    if (child_iterator(children, true))
    {
        os << ctx.ind << "auto cpos" << ctx.varCounter << " = ImRad::GetCursorData();\n";
        os << ctx.ind << "ImGui::PushClipRect(ImRad::GetParentInnerRect().Min, ImRad::GetParentInnerRect().Max, false);\n";
        os << ctx.ind << "/// @separator\n";

        for (auto& child : child_iterator(children, true))
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::PopClipRect();\n";
        os << ctx.ind << "ImRad::SetCursorData(cpos" << ctx.varCounter << ");\n";
    }

    os << ctx.ind << "ImGui::EndChild();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_borderSize.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";

    if (!style_outerPadding)
        os << ctx.ind << "ImRad::PopIgnoreWindowPadding(" << datavar << ");\n";

    ++ctx.varCounter;
}

void Child::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiCol_ChildBg")
            style_bg.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
            style_padding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
            style_spacing.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildRounding")
            style_rounding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ChildBorderSize")
            style_borderSize.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::Other &&
        (!sit->line.compare(0, 10, "ImVec2 _sz") || !sit->line.compare(0, 9, "ImVec2 sz"))) //sz for compatibility
    {
        style_outerPadding = false;
        size_t i = sit->line.find('{');
        if (i != std::string::npos) {
            auto size = cpp::parse_size(sit->line.substr(i));
            size_x.set_from_arg(size.first);
            size_y.set_from_arg(size.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
    {
        if (sit->params.size() >= 2) {
            auto size = cpp::parse_size(sit->params[1]);
            if (size.first != "" && size.second != "") {
                size_x.set_from_arg(size.first);
                size_y.set_from_arg(size.second);
            }
        }

        if (sit->params.size() >= 3) {
            if (!flags.set_from_arg(sit->params[2]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[2] + "\"");
        }
        if (sit->params.size() >= 4) {
            if (!wflags.set_from_arg(sit->params[3]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[3] + "\"");
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Columns")
    {
        if (sit->params.size())
            *columnCount.access() = sit->params[0];

        if (sit->params.size() >= 3)
            columnBorder = sit->params[2] == "true";
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::ScrollWhenDragging")
    {
        scrollWhenDragging = true;
    }
    else if (sit->kind == cpp::ForBlock)
    {
        itemCount.set_from_arg(sit->line);
    }
}


std::vector<UINode::Prop>
Child::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.color", &style_bg },
        { "appearance.border", &style_border },
        { "appearance.padding", &style_padding },
        { "appearance.spacing", &style_spacing },
        { "appearance.rounding", &style_rounding },
        { "appearance.borderSize", &style_borderSize },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "appearance.outer_padding", &style_outerPadding },
        { "appearance.column_border##child", &columnBorder },
        { "behavior.flags##child", &flags },
        { "behavior.wflags##child", &wflags },
        { "behavior.column_count##child", &columnCount },
        { "behavior.item_count##child", &itemCount.limit },
        { "behavior.scrollWhenDragging", &scrollWhenDragging },
        { "bindings.itemIndex##1", &itemCount.index },
        });
    return props;
}

bool Child::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("color");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("border");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_border, ImGuiCol_Border, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("border", &style_border, ctx);
        break;
    case 2:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_padding, ctx);
        break;
    case 3:
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_spacing, ctx);
        break;
    case 4:
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_rounding, ctx);
        break;
    case 5:
        ImGui::Text("borderSize");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_borderSize, ctx);
        break;
    case 6:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        TextFontInfo(ctx);
        break;
    case 7:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 8:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 9:
        ImGui::Text("outerPadding");
        ImGui::TableNextColumn();
        fl = style_outerPadding != Defaults().style_outerPadding ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&style_outerPadding, fl, ctx);
        break;
    case 10:
        ImGui::Text("columnBorder");
        ImGui::TableNextColumn();
        fl = columnBorder != Defaults().columnBorder ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&columnBorder, fl, ctx);
        break;
    case 11:
    {
        int ch = InputDirectValFlags("childFlags", &flags, Defaults().flags, ctx);
        if (ch) {
            changed = true;
            //these flags are difficult to get right and there are asserts so fix it here
            if (ch == ImGuiChildFlags_AutoResizeX || ch == ImGuiChildFlags_AutoResizeY) {
                if (!(flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)))
                    flags &= ~ImGuiChildFlags_AlwaysAutoResize;
            }
            if (ch == ImGuiChildFlags_AlwaysAutoResize) {
                if (flags & ImGuiChildFlags_AlwaysAutoResize) {
                    if (!(flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)))
                        flags |= ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
                }
            }
            //
            if (flags & ImGuiChildFlags_AutoResizeX)
                size_x = 0;
            if (flags & ImGuiChildFlags_AutoResizeY)
                size_y = 0;
        }
        break;
    }
    case 12:
        changed = InputDirectValFlags("windowFlags", &wflags, Defaults().wflags, ctx);
        break;
    case 13:
        ImGui::Text("columnCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = columnCount != Defaults().columnCount  ? InputBindable_Modified : 0;
        changed = InputBindable(&columnCount, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("columnCount", &columnCount, ctx);
        break;
    case 14:
        ImGui::Text("itemCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("itemCount", &itemCount.limit, ctx);
        break;
    case 15:
        ImGui::Text("scrollWhenDragging");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = scrollWhenDragging != Defaults().scrollWhenDragging ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&scrollWhenDragging, fl, ctx);
        break;
    case 16:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("itemIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 17, ctx);
    }
    return changed;
}

//---------------------------------------------------------

Splitter::Splitter(UIContext& ctx)
{
    size_x = size_y = -1;

    if (ctx.createVars)
        position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Impl));
}

std::unique_ptr<Widget> Splitter::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<Splitter>(*this);
    if (!position.empty() && ctx.createVars) {
        sel->position.set_from_arg(ctx.codeGen->CreateVar("float", "100", CppGen::Var::Impl));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* Splitter::DoDraw(UIContext& ctx)
{
    ImVec2 size;
    size.x = size_x.eval_px(ImGuiAxis_X, ctx);
    size.y = size_y.eval_px(ImGuiAxis_Y, ctx);

    if (!style_bg.empty())
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style_bg.eval(ImGuiCol_ChildBg, ctx));
    ImGui::BeginChild("splitter", size);

    ImGuiAxis axis = ImGuiAxis_X;
    float th = 0, pos = 0;
    if (children.size() == 2) {
        axis = children[1]->sameLine ? ImGuiAxis_X : ImGuiAxis_Y;
        pos = children[0]->cached_pos[axis] + children[0]->cached_size[axis];
        th = children[1]->cached_pos[axis] - children[0]->cached_pos[axis] - children[0]->cached_size[axis];
    }

    ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);
    ImRad::Splitter(axis == ImGuiAxis_X, th, &pos, min_size1, min_size2);
    ImGui::PopStyleColor();

    for (size_t i = 0; i < children.size(); ++i)
        children[i]->Draw(ctx);

    ImGui::EndChild();
    if (!style_bg.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void Splitter::DoExport(std::ostream& os, UIContext& ctx)
{
    if (children.size() != 2)
        PushError(ctx, "need exactly 2 children");
    if (position.empty())
        PushError(ctx, "position is unassigned");
    if (children.empty() || position.empty() ||
        (!stx::count(children[0]->size_x.used_variables(), position.value()) &&
        !stx::count(children[0]->size_y.used_variables(), position.value())))
        PushError(ctx, "first child doesn't reference \"" + position.value() + "\" in its size");

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_ChildBg, " << style_bg.to_arg() << ");\n";

    os << ctx.ind << "ImGui::BeginChild(\"splitter" << ctx.varCounter << "\", { "
        << size_x.to_arg(ctx.unit, ctx.stretchSizeExpr[0]) << ", "
        << size_y.to_arg(ctx.unit, ctx.stretchSizeExpr[1])
        << " });\n" << ctx.ind << "{\n";
    ctx.ind_up();
    ++ctx.varCounter;

    os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Separator, 0x00000000);\n";
    os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, 0x00000000);\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_SeparatorActive, " << style_active.to_arg() << ");\n";

    bool axisX = true;
    direct_val<dimension_t> th = ImGui::GetStyle().ItemSpacing.x;
    if (children.size() == 2) {
        axisX = children[1]->sameLine;
        th = children[1]->cached_pos[!axisX] - children[0]->cached_pos[!axisX] - children[0]->cached_size[!axisX];
    }

    os << ctx.ind << "ImRad::Splitter("
        << std::boolalpha << axisX
        << ", " << th.to_arg(ctx.unit)
        << ", &" << position.to_arg()
        << ", " << min_size1.to_arg(ctx.unit)
        << ", " << min_size2.to_arg(ctx.unit)
        << ");\n";

    os << ctx.ind << "ImGui::PopStyleColor();\n";
    os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";

    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndChild();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_bg.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void Splitter::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::BeginChild")
    {
        if (sit->params.size() >= 2) {
            auto sz = cpp::parse_size(sit->params[1]);
            size_x.set_from_arg(sz.first);
            size_y.set_from_arg(sz.second);
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImRad::Splitter")
    {
        if (sit->params.size() >= 3 && !sit->params[2].compare(0, 1, "&"))
            position.set_from_arg(sit->params[2].substr(1));
        if (sit->params.size() >= 4)
            min_size1.set_from_arg(sit->params[3]);
        if (sit->params.size() >= 5)
            min_size2.set_from_arg(sit->params[4]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_ChildBg")
            style_bg.set_from_arg(sit->params[1]);
        if (sit->params.size() >= 2 && sit->params[0] == "ImGuiCol_SeparatorActive")
            style_active.set_from_arg(sit->params[1]);
    }
}


std::vector<UINode::Prop>
Splitter::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.bg", &style_bg },
        { "appearance.active", &style_active },
        { "behavior.min1##splitter", &min_size1 },
        { "behavior.min2##splitter", &min_size2 },
        { "bindings.sashPos##1", &position },
        });
    return props;
}

bool Splitter::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("bg");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_bg, ImGuiCol_ChildBg, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_bg, ctx);
        break;
    case 1:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_active, ImGuiCol_SeparatorActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 2:
        ImGui::Text("minSize1");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = min_size1 != Defaults().min_size1 ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&min_size1, fl, ctx);
        break;
    case 3:
        ImGui::Text("minSize2");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = min_size2 != Defaults().min_size2 ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&min_size2, fl, ctx);
        break;
    case 4:
        ImGui::Text("sashPos");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&position, false, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 5, ctx);
    }
    return changed;
}

//---------------------------------------------------------

CollapsingHeader::CollapsingHeader(UIContext& ctx)
{
    flags.add$(ImGuiTreeNodeFlags_Bullet);
    flags.add$(ImGuiTreeNodeFlags_DefaultOpen);
    //flags.add$(ImGuiTreeNodeFlags_Framed);
    flags.add$(ImGuiTreeNodeFlags_FramePadding);
    //flags.add$(ImGuiTreeNodeFlags_Leaf);
    //flags.add$(ImGuiTreeNodeFlags_NoTreePushOnOpen);
    flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
    flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
    flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
    flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
}

std::unique_ptr<Widget> CollapsingHeader::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<CollapsingHeader>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* CollapsingHeader::DoDraw(UIContext& ctx)
{
    if (!style_header.empty())
        ImGui::PushStyleColor(ImGuiCol_Header, style_header.eval(ImGuiCol_Header, ctx));
    if (!style_hovered.empty())
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style_hovered.eval(ImGuiCol_HeaderHovered, ctx));
    if (!style_active.empty())
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, style_active.eval(ImGuiCol_HeaderActive, ctx));

    //automatically expand to show selected child and collapse to save space
    //in the window and avoid potential scrolling
    if (ctx.selected.size() == 1)
    {
        ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
    }
    if (ImGui::CollapsingHeader(DRAW_STR(label), flags))
    {
        for (size_t i = 0; i < children.size(); ++i)
        {
            children[i]->Draw(ctx);
        }
    }

    if (!style_header.empty())
        ImGui::PopStyleColor();
    if (!style_hovered.empty())
        ImGui::PopStyleColor();
    if (!style_active.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void CollapsingHeader::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorScreenPos().y - p1.y - pad.y;
}

void CollapsingHeader::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_header.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Header, " << style_header.to_arg() << ");\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderHovered, " << style_hovered.to_arg() << ");\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_HeaderActive, " << style_active.to_arg() << ");\n";

    if (!open.empty())
        os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ");\n";

    os << ctx.ind << "if (ImGui::CollapsingHeader(" << label.to_arg() << ", "
        << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();

    os << ctx.ind << "/// @separator\n\n";

    for (auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";

    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_header.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_active.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void CollapsingHeader::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_Header")
            style_header.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_HeaderHovered")
            style_hovered.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_HeaderActive")
            style_active.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
    {
        if (sit->params.size() >= 1)
            open.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::CollapsingHeader")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2) {
            if (!flags.set_from_arg(sit->params[1]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[1] + "\"");
        }
    }
}

std::vector<UINode::Prop>
CollapsingHeader::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.header", &style_header },
        { "appearance.hovered", &style_hovered },
        { "appearance.active", &style_active },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "behavior.flags##coh", &flags },
        { "behavior.label", &label, true },
        { "behavior.open", &open }
        });
    return props;
}

bool CollapsingHeader::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("header");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_header, ImGuiCol_Header, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("header", &style_header, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_hovered, ImGuiCol_HeaderHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("active");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_active, ImGuiCol_HeaderActive, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("active", &style_active, ctx);
        break;
    case 4:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        TextFontInfo(ctx);
        break;
    case 5:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 6:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 7:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 8:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 9:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = open != Defaults().open ? InputBindable_Modified : 0;
        changed = InputBindable(&open, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("open", &open, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 10, ctx);
    }
    return changed;
}

//---------------------------------------------------------

TreeNode::TreeNode(UIContext& ctx)
{
    flags.add$(ImGuiTreeNodeFlags_Bullet);
    flags.add$(ImGuiTreeNodeFlags_DefaultOpen);
    //flags.add$(ImGuiTreeNodeFlags_Framed);
    flags.add$(ImGuiTreeNodeFlags_FramePadding);
    flags.add$(ImGuiTreeNodeFlags_Leaf);
    flags.add$(ImGuiTreeNodeFlags_NoTreePushOnOpen);
    flags.add$(ImGuiTreeNodeFlags_OpenOnArrow);
    flags.add$(ImGuiTreeNodeFlags_OpenOnDoubleClick);
    flags.add$(ImGuiTreeNodeFlags_SpanAllColumns);
    flags.add$(ImGuiTreeNodeFlags_SpanAvailWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanFullWidth);
    flags.add$(ImGuiTreeNodeFlags_SpanTextWidth);
}

std::unique_ptr<Widget> TreeNode::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TreeNode>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TreeNode::DoDraw(UIContext& ctx)
{
    if (ctx.selected.size() == 1)
    {
        //automatically expand to show selection and collapse to save space
        //in the window and avoid potential scrolling
        ImGui::SetNextItemOpen((bool)FindChild(ctx.selected[0]));
    }
    lastOpen = false;
    auto ps = PrepareString(label.value());
    if (ImGui::TreeNodeEx(ps.label.c_str(), flags))
    {
        lastOpen = true;
        for (const auto& child : children)
            child->Draw(ctx);

        ImGui::TreePop();
    }
    ImVec2 offset{
        ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.x * 2,
        flags & ImGuiTreeNodeFlags_FramePadding ? ImGui::GetStyle().FramePadding.y : 0
    };
    DrawTextArgs(ps, ctx, offset);

    return ImGui::GetWindowDrawList();
}

void TreeNode::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    cached_size = ImGui::GetItemRectSize();
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;

    if (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth))
    {
        //todo cached_size.x = ImGui::GetContentRegionAvail().x - p1.x + sp.x;
    }
    else
    {
        cached_size.x = ImGui::CalcTextSize(label.c_str(), 0, true).x + 4 * sp.x;
    }

    if (lastOpen)
    {
        for (const auto& child : children)
        {
            auto p2 = child->cached_pos + child->cached_size;
            cached_size.x = std::max(cached_size.x, p2.x - cached_pos.x);
            cached_size.y = std::max(cached_size.y, p2.y - cached_pos.y);
        }
    }
}

void TreeNode::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!open.empty())
        os << ctx.ind << "ImGui::SetNextItemOpen(" << open.to_arg() << ");\n";

    if (PrepareString(label.value()).error)
        PushError(ctx, "label is formatted wrongly");

    os << ctx.ind << "if (ImGui::TreeNodeEx(" << label.to_arg() << ", " << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";

    ctx.ind_up();
    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
    {
        child->Export(os, ctx);
    }

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::TreePop();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";
}

void TreeNode::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::TreeNodeEx")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2) {
            if (!flags.set_from_arg(sit->params[1]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[1] + "\"");
        }
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemOpen")
    {
        if (sit->params.size())
            open.set_from_arg(sit->params[0]);
    }
}

std::vector<UINode::Prop>
TreeNode::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "behavior.flags", &flags },
        { "behavior.label", &label, true },
        { "behavior.open", &open },
        });
    return props;
}

bool TreeNode::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        TextFontInfo(ctx);
        break;
    case 2:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 3:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 4:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 5:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 6:
        ImGui::Text("open");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = open != Defaults().open ? InputBindable_Modified : 0;
        changed = InputBindable(&open, fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("open", &open, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 7, ctx);
    }
    return changed;
}

//--------------------------------------------------------------

TabBar::TabBar(UIContext& ctx)
{
    flags.add$(ImGuiTabBarFlags_DrawSelectedOverline);
    flags.add$(ImGuiTabBarFlags_FittingPolicyResizeDown);
    flags.add$(ImGuiTabBarFlags_FittingPolicyScroll);
    flags.add$(ImGuiTabBarFlags_NoTabListScrollingButtons);
    flags.add$(ImGuiTabBarFlags_Reorderable);
    flags.add$(ImGuiTabBarFlags_TabListPopupButton);

    if (ctx.createVars)
        children.push_back(std::make_unique<TabItem>(ctx));
}

std::unique_ptr<Widget> TabBar::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TabBar>(*this);
    //itemCount can be shared
    if (!activeTab.empty() && ctx.createVars) {
        sel->activeTab.set_from_arg(ctx.codeGen->CreateVar("int", "", CppGen::Var::Interface));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TabBar::DoDraw(UIContext& ctx)
{
    if (!style_tab.empty())
        ImGui::PushStyleColor(ImGuiCol_Tab, style_tab.eval(ImGuiCol_Tab, ctx));
    if (!style_hovered.empty())
        ImGui::PushStyleColor(ImGuiCol_TabHovered, style_hovered.eval(ImGuiCol_TabHovered, ctx));
    if (!style_selected.empty())
        ImGui::PushStyleColor(ImGuiCol_TabSelected, style_selected.eval(ImGuiCol_TabSelected, ctx));
    if (!style_tabDimmed.empty())
        ImGui::PushStyleColor(ImGuiCol_TabDimmed, style_tab.eval(ImGuiCol_TabDimmed, ctx));
    if (!style_dimmedSelected.empty())
        ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, style_dimmedSelected.eval(ImGuiCol_TabDimmedSelected, ctx));
    if (!style_overline.empty())
        ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, style_overline.eval(ImGuiCol_TabSelectedOverline, ctx));
    if (!style_framePadding.empty())
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style_framePadding.eval_px(ctx));

    std::string id = "##" + std::to_string((uintptr_t)this);
    //add tab names in order so that when tab order changes in designer we get a
    //different id which will force ImGui to recalculate tab positions and
    //render tabs correctly
    for (const auto& child : children)
        id += std::to_string(uintptr_t(child.get()) & 0xffff);

    if (ImGui::BeginTabBar(id.c_str(), flags))
    {
        for (const auto& child : children)
            child->Draw(ctx);

        ImGui::EndTabBar();
    }

    if (!style_framePadding.empty())
        ImGui::PopStyleVar();
    if (!style_tab.empty())
        ImGui::PopStyleColor();
    if (!style_hovered.empty())
        ImGui::PopStyleColor();
    if (!style_selected.empty())
        ImGui::PopStyleColor();
    if (!style_tabDimmed.empty())
        ImGui::PopStyleColor();
    if (!style_dimmedSelected.empty())
        ImGui::PopStyleColor();
    if (!style_overline.empty())
        ImGui::PopStyleColor();

    return ImGui::GetWindowDrawList();
}

void TabBar::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = p1;
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    cached_pos.x -= pad.x;
    cached_size.x = ImGui::GetContentRegionAvail().x + 2 * pad.x;
    cached_size.y = ImGui::GetCursorScreenPos().y - p1.y;
}

void TabBar::DoExport(std::ostream& os, UIContext& ctx)
{
    if (!style_tab.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_Tab, " << style_tab.to_arg() << ");\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabHovered, " << style_hovered.to_arg() << ");\n";
    if (!style_selected.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabSelected, " << style_selected.to_arg() << ");\n";
    if (!style_overline.empty())
        os << ctx.ind << "ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, " << style_overline.to_arg() << ");\n";

    os << ctx.ind << "if (ImGui::BeginTabBar(\"tabBar" << ctx.varCounter << "\", "
        << flags.to_arg() << "))\n";
    os << ctx.ind << "{\n";
    ++ctx.varCounter;
    ctx.ind_up();

    if (style_regularWidth)
    {
        os << ctx.ind << "int _nTabs = std::max(1, ImGui::GetCurrentTabBar()->ActiveTabs);\n"; //respects hidden tabs
        os << ctx.ind << "float _tabWidth = (ImGui::GetContentRegionAvail().x - (_nTabs - 1) * ImGui::GetStyle().ItemInnerSpacing.x) / _nTabs - 1;\n";
    }

    if (!itemCount.empty())
    {
        std::string idxVar = itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);
        std::string contVar = itemCount.container_expr();
        
        os << ctx.ind << itemCount.to_arg(ctx.codeGen->FOR_VAR_NAME) << "\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();

        if (contVar != "")
        {
            os << ctx.ind << "auto& " << ctx.codeGen->CUR_ITEM_VAR_NAME << " = "
                << contVar << "[" << idxVar << "];\n";
        }
        //BeginTabBar does this
        //os << ctx.ind << "ImGui::PushID(" << itemCount.index_name_or(ctx.codeGen->DefaultForVarName()) << ");\n";
    }
    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    if (!itemCount.empty())
    {
        //os << ctx.ind << "ImGui::PopID();\n"; EndTabBar does this
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }

    os << ctx.ind << "ImGui::EndTabBar();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (!style_tab.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_hovered.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_selected.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
    if (!style_overline.empty())
        os << ctx.ind << "ImGui::PopStyleColor();\n";
}

void TabBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleColor")
    {
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_Tab")
            style_tab.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabHovered")
            style_hovered.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabSelected")
            style_selected.set_from_arg(sit->params[1]);
        if (sit->params.size() == 2 && sit->params[1] == "ImGuiCol_TabSelectedOverline")
            style_overline.set_from_arg(sit->params[1]);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabBar")
    {
        ctx.importLevel = sit->level;

        if (sit->params.size() >= 2) {
            if (!flags.set_from_arg(sit->params[1]))
                PushError(ctx, "unrecognized flag in \"" + sit->params[1] + "\"");
        }
    }
    else if (sit->kind == cpp::ForBlock && sit->level == ctx.importLevel + 1)
    {
        itemCount.set_from_arg(sit->line);
    }
}

std::vector<UINode::Prop>
TabBar::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.text", &style_text },
        { "appearance.tab", &style_tab },
        { "appearance.hovered", &style_hovered },
        { "appearance.selected", &style_selected },
        { "appearance.overline", &style_overline },
        { "appearance.regularWidth", &style_regularWidth },
        { "appearance.padding", &style_framePadding },
        { "appearance.font.summary", nullptr },
        { "appearance.font.name", &style_fontName },
        { "appearance.font.size", &style_fontSize },
        { "behavior.flags", &flags },
        { "behavior.tabCount", &itemCount.limit },
        { "bindings.tabIndex##1", &itemCount.index },
        { "bindings.activeTab##1", &activeTab },
        });
    return props;
}

bool TabBar::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("text");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_text, ImGuiCol_Text, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("color", &style_text, ctx);
        break;
    case 1:
        ImGui::Text("tab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_tab, ImGuiCol_Tab, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("bg", &style_tab, ctx);
        break;
    case 2:
        ImGui::Text("hovered");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_hovered, ImGuiCol_TabHovered, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("hovered", &style_hovered, ctx);
        break;
    case 3:
        ImGui::Text("selected");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_selected, ImGuiCol_TabSelected, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("selected", &style_selected, ctx);
        break;
    case 4:
        ImGui::Text("overline");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_overline, ImGuiCol_TabSelectedOverline, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("overline", &style_overline, ctx);
        break;
    case 5:
        ImGui::Text("regularWidth");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_regularWidth, 0, ctx);
        break;
    case 6:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_framePadding, ctx);
        break;
    case 7:
        ImGui::Text("font");
        ImGui::TableNextColumn();
        TextFontInfo(ctx);
        break;
    case 8:
        ImGui::Text("name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontName, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font", &style_fontName, ctx);
        break;
    case 9:
        ImGui::Text("size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&style_fontSize, InputBindable_ParentStr, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("font_size", &style_fontSize, ctx);
        break;
    case 10:
        changed = InputDirectValFlags("flags", &flags, Defaults().flags, ctx);
        break;
    case 11:
        ImGui::Text("tabCount");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDataSize(&itemCount.limit, true, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("tabCount", &itemCount.limit, ctx);
        break;
    case 12:
        ImGui::BeginDisabled(itemCount.empty());
        ImGui::Text("tabIndex");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&itemCount.index, true, ctx);
        ImGui::EndDisabled();
        break;
    case 13:
        ImGui::Text("activeTab");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputFieldRef(&activeTab, true, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 14, ctx);
    }
    return changed;
}

//---------------------------------------------------------

TabItem::TabItem(UIContext& ctx)
{
}

std::unique_ptr<Widget> TabItem::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<TabItem>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* TabItem::DoDraw(UIContext& ctx)
{
    auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);
    float tabw = 0;
    if (tb && tb->style_regularWidth) {
        tabw = (ImGui::GetContentRegionAvail().x - (tb->children.size() - 1) * ImGui::GetStyle().ItemInnerSpacing.x) / tb->children.size() - 1;
        ImGui::SetNextItemWidth(tabw);
    }
    /*float padx = ImGui::GetCurrentTabBar()->FramePadding.x;
    if (tabw) {
        float w = ImGui::CalcTextSize(DRAW_STR(label)).x;
        //hack: ImGui::GetCurrentTabBar()->FramePadding.x = std::max(0.f, (tabw - w) / 2);
    }*/

    bool sel = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
    bool tmp = true;
    if (ImGui::BeginTabItem(DRAW_STR(label), closeButton ? &tmp : nullptr, sel ? ImGuiTabItemFlags_SetSelected : 0))
    {
        //ImGui::GetCurrentTabBar()->FramePadding.x = padx;

        for (const auto& child : children)
            child->Draw(ctx);

        ImGui::EndTabItem();
    }
    /*else
        ImGui::GetCurrentTabBar()->FramePadding.x = padx;*/

    return ImGui::GetWindowDrawList();
}

void TabItem::DoDrawTools(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;
    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
        - parent->children.begin();

    ImGui::SetNextWindowPos(cached_pos, 0, { 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);

    ImGui::BeginDisabled(!idx);
    if (ImGui::Button(ICON_FA_ANGLE_LEFT)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_PLUS)) {
        parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<TabItem>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = (parent->children.begin() + idx + 1)->get();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(idx + 1 == parent->children.size());
    if (ImGui::Button(ICON_FA_ANGLE_RIGHT)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void TabItem::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    const ImGuiTabBar* tabBar = ImGui::GetCurrentTabBar();
    cached_pos = tabBar->BarRect.GetTL();
    cached_size.x = tabBar->BarRect.GetWidth();
    cached_size.y = tabBar->BarRect.GetHeight();
    int idx = tabBar->LastTabItemIdx;
    if (idx >= 0) {
        const ImGuiTabItem& tab = tabBar->Tabs[idx];
        cached_pos.x += tab.Offset;
        cached_size.x = tab.Width;
    }
}

void TabItem::DoExport(std::ostream& os, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);

    if (tb->style_regularWidth) {
        os << ctx.ind << "ImGui::SetNextItemWidth(_tabWidth);\n";
    }

    std::string var = "_open" + std::to_string(ctx.varCounter);
    if (closeButton) {
        os << ctx.ind << "bool " << var << " = true;\n";
        ++ctx.varCounter;
    }

    os << ctx.ind << "if (ImGui::BeginTabItem(" << label.to_arg() << ", ";
    if (closeButton)
        os << "&" << var << ", ";
    else
        os << "nullptr, ";
    std::string idx;
    if (tb && !tb->activeTab.empty())
    {
        if (!tb->itemCount.empty())
        {
            idx = tb->itemCount.index_name_or(ctx.codeGen->FOR_VAR_NAME);
        }
        else
        {
            size_t n = stx::find_if(tb->children, [this](const auto& ch) {
                return ch.get() == this; }) - tb->children.begin();
            idx = std::to_string(n);
        }
        os << tb->activeTab.to_arg() << " == " << idx << " ? ImGuiTabItemFlags_SetSelected : 0";
    }
    else
    {
        os << "ImGuiTabItemFlags_None";
    }
    os << "))\n";
    os << ctx.ind << "{\n";

    ctx.ind_up();
    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
    {
        child->Export(os, ctx);
    }

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndTabItem();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (closeButton && !onClose.empty())
    {
        os << ctx.ind << "if (!" << var << ")\n";
        ctx.ind_up();
        /*if (idx != "") no need to activate, user can check itemCount.index
            os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";*/
        os << ctx.ind << onClose.to_arg() << "();\n";
        ctx.ind_down();
    }
    /*if (idx != "") user can add IsItemActivated event on his own and there is itemCount.index
    {
        os << ctx.ind << "if (ImGui::IsItemActivated())\n";
        ctx.ind_up();
        os << ctx.ind << tb->activeTab.to_arg() << " = " << idx << ";\n";
        ctx.ind_down();
    }*/
}

void TabItem::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    auto* tb = dynamic_cast<TabBar*>(ctx.parents[ctx.parents.size() - 2]);

    if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::SetNextItemWidth")
    {
        tb->style_regularWidth = true;
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginTabItem")
    {
        if (sit->params.size() >= 1)
            label.set_from_arg(sit->params[0]);

        if (sit->params.size() >= 2 && !sit->params[1].compare(0, 1, "&"))
            closeButton = true;

        if (tb && sit->params.size() >= 3 && sit->params[2].size() > 32)
        {
            const auto& p = sit->params[2];
            size_t i = p.find("==");
            if (i != std::string::npos && p.substr(p.size() - 32, 32) == "?ImGuiTabItemFlags_SetSelected:0")
            {
                std::string var = p.substr(0, i); // i + 2, p.size() - 32 - i - 2);
                tb->activeTab.set_from_arg(var);
            }
        }
    }
    else if (sit->kind == cpp::IfBlock && !sit->cond.compare(0, 5, "!tmpOpen"))
    {
        ctx.importLevel = sit->level;
    }
    else if (sit->kind == cpp::CallExpr && sit->level == ctx.importLevel)
    {
        onClose.set_from_arg(sit->callee);
    }
}

std::vector<UINode::Prop>
TabItem::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "behavior.label", &label, true },
        { "behavior.closeButton", &closeButton }
        });
    return props;
}

bool TabItem::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputBindable(&label, 0, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("label", &label, ctx);
        break;
    case 1:
        ImGui::Text("closeButton");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = closeButton != Defaults().closeButton ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&closeButton, fl, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 2, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
TabItem::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "tabItem.close", &onClose },
        });
    return props;
}

bool TabItem::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::BeginDisabled(!closeButton);
        ImGui::Text("Closed");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_Closed", &onClose, 0, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::EventUI(i - 1, ctx);
    }
    return changed;
}

//---------------------------------------------------------

MenuBar::MenuBar(UIContext& ctx)
{
    if (ctx.createVars)
        children.push_back(std::make_unique<MenuIt>(ctx));
}

std::unique_ptr<Widget> MenuBar::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<MenuBar>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* MenuBar::DoDraw(UIContext& ctx)
{
    if (ImGui::BeginMenuBar())
    {
        //for (const auto& child : children) defend against insertion within the loop
        for (size_t i = 0; i < children.size(); ++i)
            children[i]->Draw(ctx);

        ImGui::EndMenuBar();
    }

    //menuBar is drawn from Begin anyways
    return ImGui::GetWindowDrawList();
}

void MenuBar::CalcSizeEx(ImVec2 x1, UIContext& ctx)
{
    cached_pos = ImGui::GetCurrentWindow()->MenuBarRect().GetTL();
    cached_size = ImGui::GetCurrentWindow()->MenuBarRect().GetSize();
}

void MenuBar::DoExport(std::ostream& os, UIContext& ctx)
{
    os << ctx.ind << "if (ImGui::BeginMenuBar())\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();

    for (const auto& child : children) {
        MenuIt* it = dynamic_cast<MenuIt*>(child.get());
        if (it) it->ExportAllShortcuts(os, ctx);
    }

    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndMenuBar();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";
}

void MenuBar::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    auto* win = dynamic_cast<TopWindow*>(ctx.root);
    if (win)
        win->flags |= ImGuiWindowFlags_MenuBar;
}

std::vector<UINode::Prop>
MenuBar::Properties()
{
    return {};
}

bool MenuBar::PropertyUI(int i, UIContext& ctx)
{
    return false;
}

std::vector<UINode::Prop>
MenuBar::Events()
{
    return {};
}

bool MenuBar::EventUI(int i, UIContext& ctx)
{
    return false;
}

//---------------------------------------------------------

ContextMenu::ContextMenu(UIContext& ctx)
{
}

std::unique_ptr<Widget> ContextMenu::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<ContextMenu>(*this);
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* ContextMenu::DoDraw(UIContext& ctx)
{
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);

    ctx.contextMenus.push_back(label);
    bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
    if (open)
    {
        ImGui::SetNextWindowPos(cached_pos);
        if (style_padding.has_value())
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style_padding.eval_px(ctx));
        if (style_spacing.has_value())
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style_spacing.eval_px(ctx));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
            style_rounding.empty() ? ImGui::GetStyle().PopupRounding : style_rounding.eval_px(ctx));

        std::string id = label + "##" + std::to_string((uintptr_t)this);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        bool sel = stx::count(ctx.selected, this);
        if (sel)
            ImGui::PushStyleColor(ImGuiCol_Border, ctx.colors[UIContext::Selected]);
        ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
        {
            if (sel)
                ImGui::PopStyleColor();
            ctx.activePopups.push_back(ImGui::GetCurrentWindow());
            //for (const auto& child : children) defend against insertions within the loop
            for (size_t i = 0; i < children.size(); ++i)
                children[i]->Draw(ctx);

            ImGui::End();
        }
        ImGui::PopStyleVar();

        ImGui::PopStyleVar();
        if (style_spacing.has_value())
            ImGui::PopStyleVar();
        if (style_padding.has_value())
            ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();
    return ImGui::GetWindowDrawList();
}

void ContextMenu::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    cached_pos = ctx.rootWin->InnerRect.Min;
    cached_size = { 0, 0 }; //won't rect-select
}

void ContextMenu::DoExport(std::ostream& os, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];

    ExportAllShortcuts(os, ctx);

    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, " << style_padding.to_arg(ctx.unit) << ");\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, " << style_rounding.to_arg(ctx.unit) << ");\n";
    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, " << style_spacing.to_arg(ctx.unit) << ");\n";

    os << ctx.ind << "if (ImGui::BeginPopup(" << label.to_arg() << ", "
        << "ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings"
        << "))\n";
    os << ctx.ind << "{\n";
    ctx.ind_up();

    os << ctx.ind << "/// @separator\n\n";

    for (const auto& child : children)
        child->Export(os, ctx);

    os << ctx.ind << "/// @separator\n";
    os << ctx.ind << "ImGui::EndPopup();\n";
    ctx.ind_down();
    os << ctx.ind << "}\n";

    if (style_spacing.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_rounding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
    if (style_padding.has_value())
        os << ctx.ind << "ImGui::PopStyleVar();\n";
}

void ContextMenu::ExportAllShortcuts(std::ostream& os, UIContext& ctx)
{
    for (const auto& child : children)
    {
        auto* it = dynamic_cast<MenuIt*>(child.get());
        if (it)
            it->ExportAllShortcuts(os, ctx);
    }
}

void ContextMenu::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginPopup")
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::PushStyleVar")
    {
        if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_WindowPadding")
            style_padding.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_ItemSpacing")
            style_spacing.set_from_arg(sit->params[1]);
        else if (sit->params.size() == 2 && sit->params[0] == "ImGuiStyleVar_PopupRounding")
            style_rounding.set_from_arg(sit->params[1]);
    }
}

std::vector<UINode::Prop>
ContextMenu::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.padding", &style_padding },
        { "appearance.spacing", &style_spacing },
        { "appearance.rounding", &style_rounding },
        { "behavior.label", &label, true },
        });
    return props;
}

bool ContextMenu::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
        ImGui::Text("padding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_padding, ctx);
        break;
    case 1:
        ImGui::Text("spacing");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_spacing, ctx);
        break;
    case 2:
        ImGui::Text("rounding");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&style_rounding, ctx);
        break;
    case 3:
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        break;
    default:
        return Widget::PropertyUI(i - 4, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
ContextMenu::Events()
{
    return {};
}

bool ContextMenu::EventUI(int i, UIContext& ctx)
{
    return false;
}

//---------------------------------------------------------

MenuIt::MenuIt(UIContext& ctx)
{
    //shortcut.set_global(true);
}

std::unique_ptr<Widget> MenuIt::Clone(UIContext& ctx)
{
    auto sel = std::make_unique<MenuIt>(*this);
    if (!checked.has_single_variable() && ctx.createVars) {
        sel->checked.set_from_arg(ctx.codeGen->CreateVar("bool", "false", CppGen::Var::Interface));
    }
    sel->CloneChildrenFrom(*this, ctx);
    return sel;
}

ImDrawList* MenuIt::DoDraw(UIContext& ctx)
{
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);

    if (separator)
        ImGui::Separator();

    if (children.size()) //submenu
    {
        assert(ctx.parents.back() == this);
        const UINode* par = ctx.parents[ctx.parents.size() - 2];
        bool mbm = dynamic_cast<const MenuBar*>(par); //menu bar item

        ImGui::MenuItem(label.c_str());

        if (!mbm)
        {
            ImGui::PushFont(ImGui::GetDefaultFont(), 0);
            float w = ImGui::CalcTextSize(ICON_FA_ANGLE_RIGHT).x;
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - w);
            ImGui::Text(ICON_FA_ANGLE_RIGHT);
            ImGui::PopFont();
        }
        bool open = ctx.selected.size() == 1 && FindChild(ctx.selected[0]);
        if (open)
        {
            ImVec2 pos = cached_pos;
            if (mbm)
                pos.y += ctx.rootWin->MenuBarHeight;
            else {
                ImVec2 sp = ImGui::GetStyle().ItemSpacing;
                ImVec2 pad = ImGui::GetStyle().WindowPadding;
                pos.x += ImGui::GetWindowSize().x - sp.x;
                pos.y -= pad.y;
            }
            ImGui::SetNextWindowPos(pos);
            std::string id = label + "##" + std::to_string((uintptr_t)this);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
            ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
            {
                ctx.activePopups.push_back(ImGui::GetCurrentWindow());
                //for (const auto& child : children) defend against insertions within the loop
                for (size_t i = 0; i < children.size(); ++i)
                    children[i]->Draw(ctx);
                ImGui::End();
            }
            ImGui::PopStyleVar();
        }
    }
    else if (ownerDraw)
    {
        std::string s = onChange.to_arg();
        if (s.empty())
            s = "Draw event missing";
        ImGui::MenuItem(s.c_str(), nullptr, nullptr, false);
    }
    else //menuItem
    {
        bool check = !checked.empty();
        auto ps = PrepareString(label.value());
        ImGui::MenuItem(ps.label.c_str(), shortcut.c_str(), check);
        DrawTextArgs(ps, ctx);
    }
    ImGui::PopStyleVar();
    return ImGui::GetWindowDrawList();
}

void MenuIt::DoDrawTools(UIContext& ctx)
{
    if (ctx.parents.empty())
        return;
    assert(ctx.parents.back() == this);
    auto* parent = ctx.parents[ctx.parents.size() - 2];
    bool vertical = !dynamic_cast<MenuBar*>(parent);
    size_t idx = stx::find_if(parent->children, [this](const auto& ch) { return ch.get() == this; })
        - parent->children.begin();
    //currently we always sit it on top of the menu so that it doesn't overlap with topmenus/submenus
    ImVec2 pos = ctx.rootWin->InnerRect.Min; //PopupMenu pos
    ImVec2 mbmpos = cached_pos; //MenuBar item pos
    for (int i = (int)ctx.parents.size() - 2; i >= 0; --i) {
        if (dynamic_cast<MenuBar*>(ctx.parents[i])) {
            pos = mbmpos;
            break;
        }
        mbmpos = ctx.parents[i]->cached_pos;
    }

    //draw toolbox
    //no WindowFlags_StayOnTop
    const ImVec2 bsize{ 30, 0 };
    ImGui::SetNextWindowPos(pos, 0, vertical ? ImVec2{ 0, 1.f } : ImVec2{ 0, 1.f });
    ImGui::Begin("extra", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoSavedSettings);

    ImGui::BeginDisabled(!idx);
    if (ImGui::Button(vertical ? ICON_FA_ANGLE_UP : ICON_FA_ANGLE_LEFT, bsize)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx - 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_DOWN : ICON_FA_PLUS ICON_FA_ANGLE_RIGHT, bsize)) {
        parent->children.insert(parent->children.begin() + idx + 1, std::make_unique<MenuIt>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = parent->children[idx + 1].get();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(children.size());
    if (ImGui::Button(vertical ? ICON_FA_PLUS ICON_FA_ANGLE_RIGHT : ICON_FA_PLUS ICON_FA_ANGLE_DOWN, bsize)) {
        children.push_back(std::make_unique<MenuIt>(ctx));
        if (ctx.selected.size() == 1 && ctx.selected[0] == this)
            ctx.selected[0] = children[0].get();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(idx + 1 == parent->children.size());
    if (ImGui::Button(vertical ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT, bsize)) {
        auto ptr = std::move(parent->children[idx]);
        parent->children.erase(parent->children.begin() + idx);
        parent->children.insert(parent->children.begin() + idx + 1, std::move(ptr));
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void MenuIt::CalcSizeEx(ImVec2 p1, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];
    bool mbm = dynamic_cast<const MenuBar*>(par);
    ImVec2 sp = ImGui::GetStyle().ItemSpacing;
    cached_pos = p1;
    const ImGuiMenuColumns* mc = &ImGui::GetCurrentWindow()->DC.MenuColumns;
    cached_pos.x += mc->OffsetLabel;
    cached_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
    cached_size.x += sp.x;
    if (mbm)
    {
        ++cached_pos.y;
        cached_size.y = ctx.rootWin->MenuBarHeight - 2;
    }
    else
    {
        if (separator)
            cached_pos.y += sp.y;
    }
}

void MenuIt::DoExport(std::ostream& os, UIContext& ctx)
{
    assert(ctx.parents.back() == this);
    const UINode* par = ctx.parents[ctx.parents.size() - 2];

    if (separator)
        os << ctx.ind << "ImGui::Separator();\n";

    if (children.size())
    {
        os << ctx.ind << "if (ImGui::BeginMenu(" << label.to_arg() << "))\n";
        os << ctx.ind << "{\n";
        ctx.ind_up();
        os << ctx.ind << "/// @separator\n\n";

        for (const auto& child : children)
            child->Export(os, ctx);

        os << ctx.ind << "/// @separator\n";
        os << ctx.ind << "ImGui::EndMenu();\n";
        ctx.ind_down();
        os << ctx.ind << "}\n";
    }
    else if (ownerDraw)
    {
        if (onChange.empty())
            PushError(ctx, "ownerDraw is set but Draw event is not assigned!");
        os << ctx.ind << onChange.to_arg() << "();\n";
    }
    else
    {
        bool ifstmt = !onChange.empty();
        os << ctx.ind;
        if (ifstmt)
            os << "if (";

        os << "ImGui::MenuItem(" << label.to_arg() << ", \""
            << shortcut.c_str() << "\", ";
        if (checked.is_reference())
            os << "&" << checked.to_arg();
        else if (!checked.empty())
            os << checked.to_arg();
        else
            os << "false";
        os << ")";
        //ImGui::Shortcut is exported in ExportShortcut pass

        if (ifstmt) {
            os << ")\n";
            ctx.ind_up();
            os << ctx.ind << onChange.to_arg() << "();\n";
            ctx.ind_down();
        }
        else
            os << ";\n";
    }
}

//As of now ImGui still doesn't process shortcuts in unopened menus so
//separate Shortcut pass is needed
void MenuIt::ExportShortcut(std::ostream& os, UIContext& ctx)
{
    if (shortcut.empty() || ownerDraw)
        return;

    os << ctx.ind << "if (";
    if (!disabled.has_value() || disabled.value())
        os << "!(" << disabled.to_arg() << ") && ";
    //force RouteGlobal otherwise it won't get fired when menu popup is open
    os << "ImGui::Shortcut(" << shortcut.to_arg() << "))\n";
    ctx.ind_up();
    if (!onChange.empty())
        os << ctx.ind << onChange.to_arg() << "();\n";
    else
        os << ctx.ind << ";\n";
    ctx.ind_down();
}

void MenuIt::ExportAllShortcuts(std::ostream& os, UIContext& ctx)
{
    ExportShortcut(os, ctx);

    for (const auto& child : children)
    {
        auto* it = dynamic_cast<MenuIt*>(child.get());
        if (it)
            it->ExportAllShortcuts(os, ctx);
    }
}

void MenuIt::DoImport(const cpp::stmt_iterator& sit, UIContext& ctx)
{
    if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginPopup")
    {
        PushError(ctx, "ContextMenu protocol changed. Please replace top level \"@ MenuIt\" tags with \"@ ContextMenu\" tags manually");
    }
    else if ((sit->kind == cpp::CallExpr && sit->callee == "ImGui::MenuItem") ||
        (sit->kind == cpp::IfCallThenCall && sit->callee == "ImGui::MenuItem"))
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
        if (sit->params.size() >= 2 && cpp::is_cstr(sit->params[1]))
            *shortcut.access() = sit->params[1].substr(1, sit->params[1].size() - 2);
        if (sit->params.size() >= 3) {
            bool amp = !sit->params[2].compare(0, 1, "&");
            checked.set_from_arg(sit->params[2].substr(amp));
            if (checked.has_value() && !checked.value())
                checked.set_from_arg("");
        }

        if (sit->kind == cpp::IfCallThenCall)
            onChange.set_from_arg(sit->callee2);
    }
    else if (sit->kind == cpp::IfCallBlock && sit->callee == "ImGui::BeginMenu")
    {
        if (sit->params.size())
            label.set_from_arg(sit->params[0]);
    }
    else if (sit->kind == cpp::CallExpr && sit->callee == "ImGui::Separator")
    {
        separator = true;
    }
    else if (sit->kind == cpp::CallExpr && sit->callee.compare(0, 7, "ImGui::"))
    {
        ownerDraw = true;
        onChange.set_from_arg(sit->callee);
    }
}

std::vector<UINode::Prop>
MenuIt::Properties()
{
    auto props = Widget::Properties();
    props.insert(props.begin(), {
        { "appearance.ownerDraw", &ownerDraw },
        { "behavior.label", &label, true },
        { "behavior.shortcut", &shortcut },
        { "behavior.separator", &separator },
        { "bindings.checked##1", &checked },
        });
    return props;
}

bool MenuIt::PropertyUI(int i, UIContext& ctx)
{
    bool changed = false;
    int fl;
    switch (i)
    {
    case 0:
        ImGui::Text("ownerDraw");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = ownerDraw != Defaults().ownerDraw ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&ownerDraw, fl, ctx);
        break;
    case 1:
        ImGui::BeginDisabled(ownerDraw);
        ImGui::Text("label");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        changed = InputDirectVal(&label, InputDirectVal_Modified, ctx);
        ImGui::EndDisabled();
        break;
    case 2:
        ImGui::BeginDisabled(ownerDraw || children.size());
        ImGui::Text("shortcut");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = shortcut.empty() ? 0 : InputDirectVal_Modified;
        changed = InputDirectVal(&shortcut, fl, ctx);
        ImGui::EndDisabled();
        break;
    case 3:
        ImGui::Text("separator");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = separator != Defaults().separator ? InputDirectVal_Modified : 0;
        changed = InputDirectVal(&separator, fl, ctx);
        break;
    case 4:
        ImGui::BeginDisabled(ownerDraw || children.size());
        ImGui::Text("checked");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight());
        fl = checked != Defaults().checked ? InputBindable_Modified : 0;
        changed = InputBindable(&checked, InputBindable_ShowVariables | InputBindable_ShowNone | fl, ctx);
        ImGui::SameLine(0, 0);
        changed |= BindingButton("checked", &checked, ctx);
        ImGui::EndDisabled();
        break;
    default:
        return Widget::PropertyUI(i - 5, ctx);
    }
    return changed;
}

std::vector<UINode::Prop>
MenuIt::Events()
{
    auto props = Widget::Events();
    props.insert(props.begin(), {
        { "menuIt.change", &onChange },
        });
    return props;
}

bool MenuIt::EventUI(int i, UIContext& ctx)
{
    bool changed = false;
    switch (i)
    {
    case 0:
    {
        ImGui::BeginDisabled(children.size());
        std::string name = ownerDraw ? "Draw" : "Change";
        ImGui::Text("%s", name.c_str());
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        changed = InputEvent(GetTypeName() + "_" + name, &onChange, 0, ctx);
        ImGui::EndDisabled();
        break;
    }
    default:
        changed = Widget::EventUI(i - 1, ctx);
        break;
    }
    return changed;
}
