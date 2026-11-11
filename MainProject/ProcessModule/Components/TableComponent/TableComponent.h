#ifndef TABLECOMPONENT_H
#define TABLECOMPONENT_H

#include <QBrush>
#include <QColor>
#include <string>
#include <vector>

class QTableWidgetItem;

namespace ISTAR { namespace GUI { class TableWidget; } }
using ISTAR::GUI::TableWidget;

/**
 * 表格组件：ISTAR::GUI::TableWidget 的包装（不拥有控件）。
 * 写格子越界自动扩行扩列；坐标对显示 "(x, y)"。
 */
class TableComponent
{
public:
    TableComponent() = default;                  // 空包装：先建后绑（setupUi 之后才有 ui 控件）
    explicit TableComponent(TableWidget *table);
    void bind(TableWidget *table);               // 空包装后续绑控件

    // ---- 尺寸 ----
    int rows() const;
    int columns() const;
    int currentRow() const;   // 当前选中行（无选中返回 -1）
    // 按比例设置列宽：ratios 逐列给权重（不必归一化，如 {1, 2, 1} 即 1:2:1）；
    // 窗口/表格拉伸时比例列自动跟随；权重 <= 0 或列表不足的列保持手动宽度。
    // 重复调用 = 整体重设比例
    void setColumnStretchRatios(const std::vector<double> &ratios);

    // ---- 读格子（读不到返回 false：无格 / 无数据）----
    bool cellEmpty(int row, int col) const;
    bool cellText(int row, int col, std::string &text) const;
    bool cellValue(int row, int col, double &value) const;         // 读单值（写入时的原始精度）
    bool cellPoint(int row, int col, double &x, double &y) const;  // 读坐标对（写入时的原始精度）

    // ---- 写格子（越界自动扩行扩列；一律双存：显示文本 + 原始数据）----
    void setCell(int row, int col, double value, int decimals = 3);        // 单值
    void setCell(int row, int col, double x, double y, int decimals = 3);  // 坐标对
    void setCellText(int row, int col, const std::string &text);           // 纯文本
    // 标记格子翻译 key（TrKeyRole，英文原稿）：语言切换时 UiTranslator 按 key 重刷显示文字。
    // 只对已存在的格子生效（配合 setCellText 使用）；key 传空 = 清除标记（该格不再翻译）
    void setCellTrKey(int row, int col, const std::string &key);

    // ---- 清除（内容清空，行/列结构保留）----
    void clearCell(int row, int col);
    void clearRow(int row);
    void clearRows(const std::vector<int> &rows);
    void clearRowExcept(int row, int exceptCol);      // 清整行，跳过第 exceptCol 列
    void clearColumn(int col);
    void clearColumns(const std::vector<int> &cols);

    // ---- 删除（行/列真删，后面的自动前移补位）----
    void removeRow(int row);
    void removeRows(const std::vector<int> &rows);
    void removeAllRows();
    void removeColumn(int col);
    void removeColumns(const std::vector<int> &cols);

    // ---- 高亮（格/行/列着色，可并存；不传颜色 = 表格原生高亮色）----
    void highlightCell(int row, int col, const QColor &color = QColor());
    void highlightRow(int row, const QColor &color = QColor());
    void highlightColumn(int col, const QColor &color = QColor());
    void clearHighlightCell(int row, int col);
    void clearHighlightRow(int row);
    void clearHighlightColumn(int col);
    void clearHighlight();

    // ---- 其他 ----
    void setReadOnly();                      // 不可编辑、可选中复制
    void setSingleRowSelection();            // 整行单选 + 失焦保持选中高亮

private:
    void ensureSize(int row, int col);
    QTableWidgetItem *setItemText(int row, int col, const std::string &text);   // 写入统一入口（居中），返回格子
    void paintHighlight(int row, int col, const QColor &color);
    void eraseCellIndex(int i);
    void eraseCellRecord(int row, int col);
    void scrollToIndex(int row, int col);

    class ColumnRatioHelper;                 // 列宽比例维护（监听表格尺寸变化重新分配）
    ColumnRatioHelper *m_ratioHelper = nullptr;

    TableWidget *t = nullptr;

    struct HlCell
    {
        int r = -1;
        int c = -1;
        QBrush back;
        QBrush fore;
    };
    std::vector<HlCell> m_hlCells;
};

#endif // TABLECOMPONENT_H
