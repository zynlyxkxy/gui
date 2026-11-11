#include "TableComponent.h"

#include "tablewidget/tablewidget.h"    // ISTAR::GUI::TableWidget
#include "Common/UiTranslator.h"        // TrKeyRole（表格格子翻译 key）

#include <QAbstractItemView>
#include <QEvent>
#include <QPalette>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>

namespace {

// 数值抽屉：存写入时的原始 double（显示文本按 decimals 截断，数据不丢）
enum { CellValuesRole = Qt::UserRole + 20 };

// 清格：显示文本与原始值一并清空
void clearItemData(QTableWidgetItem *item)
{
    item->setText(QString());
    item->setData(CellValuesRole, QVariant());
}

}  // namespace

TableComponent::TableComponent(TableWidget *table)
    : t(table)
{
}

void TableComponent::bind(TableWidget *table)
{
    t = table;
}

int TableComponent::rows() const
{
    return (t != nullptr) ? t->rowCount() : 0;
}

int TableComponent::columns() const
{
    return (t != nullptr) ? t->columnCount() : 0;
}

int TableComponent::currentRow() const
{
    return (t != nullptr) ? t->currentRow() : -1;
}

// ---------------------------------------------------------------------------
// 列宽比例维护：Qt 5.12 的 QHeaderView 没有 setStretchFactor，Stretch 模式又是
// 等分而非按权，这里自己分——监听视口 resize，把比例列按权重均分剩余可用宽度
// ---------------------------------------------------------------------------
class TableComponent::ColumnRatioHelper : public QObject
{
public:
    explicit ColumnRatioHelper(QTableWidget *table)
        : QObject(table), m_table(table)
    {
    }

    void setRatios(const std::vector<double> &ratios)
    {
        m_ratios = ratios;
        apply();
        if (!m_installed)
        {
            m_table->viewport()->installEventFilter(this);   // 之后窗口拉伸自动保持比例
            m_installed = true;
        }
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        // setColumnWidth 可能连带视口再 resize（滚动条出现/消失），m_applying 防递归
        if (!m_applying && watched == m_table->viewport() && event->type() == QEvent::Resize)
        {
            apply();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void apply()
    {
        const int colCount = m_table->columnCount();
        if (colCount <= 0)
        {
            return;
        }
        double total = 0.0;   // 比例列权重和
        int fixed = 0;        // 非比例列（保持手动宽度）合计
        int lastRatioCol = -1;
        for (int c = 0; c < colCount; ++c)
        {
            const double ratio = (c < static_cast<int>(m_ratios.size())) ? m_ratios[c] : 0.0;
            if (ratio > 0.0)
            {
                total += ratio;
                lastRatioCol = c;
            }
            else
            {
                fixed += m_table->columnWidth(c);
            }
        }
        const int avail = m_table->viewport()->width() - fixed;
        if (total <= 0.0 || avail <= 0)
        {
            return;
        }
        m_applying = true;
        int used = 0;
        for (int c = 0; c < colCount && c < static_cast<int>(m_ratios.size()); ++c)
        {
            if (m_ratios[c] <= 0.0)
            {
                continue;
            }
            if (c == lastRatioCol)
            {
                // 最后一个比例列吃掉取整余数：总和恰好 = 可用宽度，绝不弹横向滚动条
                m_table->setColumnWidth(c, qMax(1, avail - used));
            }
            else
            {
                const int w = qMax(1, qRound(avail * m_ratios[c] / total));
                m_table->setColumnWidth(c, w);
                used += w;
            }
        }
        m_applying = false;
    }

    QTableWidget *m_table = nullptr;
    std::vector<double> m_ratios;
    bool m_installed = false;
    bool m_applying = false;
};

void TableComponent::setColumnStretchRatios(const std::vector<double> &ratios)
{
    if (t == nullptr)
    {
        return;
    }
    if (m_ratioHelper == nullptr)
    {
        m_ratioHelper = new ColumnRatioHelper(t);   // 挂在表格上，随表格销毁
    }
    m_ratioHelper->setRatios(ratios);
}

void TableComponent::setCell(int row, int col, double value, int decimals)
{
    QTableWidgetItem *item =
            setItemText(row, col, QString::number(value, 'f', decimals).toStdString());
    if (item != nullptr)
    {
        item->setData(CellValuesRole, QVariantList{ value });
    }
}

void TableComponent::setCell(int row, int col, double x, double y, int decimals)
{
    QTableWidgetItem *item =
            setItemText(row, col, QString("(%1, %2)")
                                  .arg(QString::number(x, 'f', decimals),
                                       QString::number(y, 'f', decimals)).toStdString());
    if (item != nullptr)
    {
        item->setData(CellValuesRole, QVariantList{ x, y });
    }
}

void TableComponent::setCellText(int row, int col, const std::string &text)
{
    if (QTableWidgetItem *item = setItemText(row, col, text))
    {
        item->setData(CellValuesRole, QString::fromStdString(text));   // 原始数据 = 文本本身
    }
}

void TableComponent::setCellTrKey(int row, int col, const std::string &key)
{
    if (t == nullptr || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
    {
        return;
    }
    if (QTableWidgetItem *item = t->item(row, col))
    {
        // 只挂 TrKeyRole（翻译 key = 英文原稿），显示文字由 UiTranslator::refresh 按 key 重刷；
        // key 传空 = 清除标记（该格不再翻译）
        item->setData(TrKeyRole, key.empty() ? QVariant() : QVariant(QString::fromStdString(key)));
    }
}

bool TableComponent::cellText(int row, int col, std::string &text) const
{
    if (t == nullptr || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
    {
        return false;
    }
    const QTableWidgetItem *item = t->item(row, col);
    if (item == nullptr)
    {
        return false;
    }
    const QString q = item->text();
    if (q.isEmpty())
    {
        return false;
    }
    text = q.toStdString();
    return true;
}

bool TableComponent::cellEmpty(int row, int col) const
{
    std::string text;
    return !cellText(row, col, text);
}

bool TableComponent::cellValue(int row, int col, double &value) const
{
    if (t == nullptr || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
    {
        return false;
    }
    const QTableWidgetItem *item = t->item(row, col);
    if (item == nullptr)
    {
        return false;
    }
    const QVariantList vals = item->data(CellValuesRole).toList();
    if (vals.size() < 1)
    {
        return false;
    }
    value = vals.at(0).toDouble();
    return true;
}

bool TableComponent::cellPoint(int row, int col, double &x, double &y) const
{
    if (t == nullptr || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
    {
        return false;
    }
    const QTableWidgetItem *item = t->item(row, col);
    if (item == nullptr)
    {
        return false;
    }
    const QVariantList vals = item->data(CellValuesRole).toList();
    if (vals.size() < 2)
    {
        return false;
    }
    x = vals.at(0).toDouble();
    y = vals.at(1).toDouble();
    return true;
}

void TableComponent::clearCell(int row, int col)
{
    if (t == nullptr)
    {
        return;
    }
    if (QTableWidgetItem *item = t->item(row, col))
    {
        clearItemData(item);
    }
}

void TableComponent::clearRow(int row)
{
    clearRowExcept(row, -1);
}

void TableComponent::clearRowExcept(int row, int exceptCol)
{
    if (t == nullptr)
    {
        return;
    }
    for (int c = 0; c < t->columnCount(); ++c)
    {
        if (c == exceptCol)
        {
            continue;
        }
        if (QTableWidgetItem *item = t->item(row, c))
        {
            clearItemData(item);
        }
    }
}

void TableComponent::clearColumns(const std::vector<int> &cols)
{
    if (t == nullptr)
    {
        return;
    }
    for (int c : cols)
    {
        if (c < 0)
        {
            continue;
        }
        for (int r = 0; r < t->rowCount(); ++r)
        {
            if (QTableWidgetItem *item = t->item(r, c))
            {
                clearItemData(item);
            }
        }
    }
}

void TableComponent::clearRows(const std::vector<int> &rows)
{
    for (int r : rows)
    {
        clearRow(r);
    }
}

void TableComponent::clearColumn(int col)
{
    if (t == nullptr)
    {
        return;
    }
    for (int r = 0; r < t->rowCount(); ++r)
    {
        if (QTableWidgetItem *item = t->item(r, col))
        {
            clearItemData(item);
        }
    }
}

void TableComponent::removeRow(int row)
{
    if (t != nullptr && row >= 0 && row < t->rowCount())
    {
        clearHighlight();   // 行号位移，高亮索引失效
        t->removeRow(row);
    }
}

void TableComponent::removeRows(const std::vector<int> &rows)
{
    if (t == nullptr)
    {
        return;
    }
    std::vector<int> sorted = rows;   // 从大到小删，避免行号位移错位
    std::sort(sorted.begin(), sorted.end(), [](int a, int b) { return a > b; });
    for (int r : sorted)
    {
        if (r >= 0 && r < t->rowCount())
        {
            removeRow(r);
        }
    }
}

void TableComponent::removeColumn(int col)
{
    if (t != nullptr && col >= 0 && col < t->columnCount())
    {
        clearHighlight();   // 列号位移，高亮索引失效
        t->removeColumn(col);
    }
}

void TableComponent::removeColumns(const std::vector<int> &cols)
{
    if (t == nullptr)
    {
        return;
    }
    std::vector<int> sorted = cols;   // 从大到小删
    std::sort(sorted.begin(), sorted.end(), [](int a, int b) { return a > b; });
    for (int c : sorted)
    {
        if (c >= 0 && c < t->columnCount())
        {
            removeColumn(c);
        }
    }
}

void TableComponent::highlightCell(int row, int col, const QColor &color)
{
    if (t == nullptr || row < 0 || row >= t->rowCount() || col < 0 || col >= t->columnCount())
    {
        return;
    }
    paintHighlight(row, col, color);
    scrollToIndex(row, col);
}

void TableComponent::highlightRow(int row, const QColor &color)
{
    if (t == nullptr || row < 0 || row >= t->rowCount())
    {
        return;
    }
    for (int c = 0; c < t->columnCount(); ++c)
    {
        paintHighlight(row, c, color);
    }
    scrollToIndex(row, 0);
}

void TableComponent::highlightColumn(int col, const QColor &color)
{
    if (t == nullptr || col < 0 || col >= t->columnCount())
    {
        return;
    }
    for (int r = 0; r < t->rowCount(); ++r)
    {
        paintHighlight(r, col, color);
    }
}

void TableComponent::clearHighlightCell(int row, int col)
{
    eraseCellRecord(row, col);
}

void TableComponent::clearHighlightRow(int row)
{
    if (t == nullptr)
    {
        return;
    }
    for (int i = m_hlCells.size() - 1; i >= 0; --i)
    {
        if (m_hlCells[i].r == row)
        {
            eraseCellIndex(i);
        }
    }
}

void TableComponent::clearHighlightColumn(int col)
{
    if (t == nullptr)
    {
        return;
    }
    for (int i = m_hlCells.size() - 1; i >= 0; --i)
    {
        if (m_hlCells[i].c == col)
        {
            eraseCellIndex(i);
        }
    }
}

void TableComponent::clearHighlight()
{
    for (int i = m_hlCells.size() - 1; i >= 0; --i)
    {
        eraseCellIndex(i);
    }
}

void TableComponent::removeAllRows()
{
    if (t == nullptr)
    {
        return;
    }
    clearHighlight();
    t->setRowCount(0);
}

void TableComponent::setReadOnly()
{
    if (t != nullptr)
    {
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void TableComponent::setSingleRowSelection()
{
    if (t == nullptr)
    {
        return;
    }
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);

    // 失焦后选区改按 Inactive 调色板渲染（默认浅灰，暗色主题下近乎不可见）。
    // 把 Inactive 高亮色对齐 Active：选中行在表格失去焦点（点按钮 / 弹窗）时保持高亮
    QPalette pal = t->palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight,
                 pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText,
                 pal.color(QPalette::Active, QPalette::HighlightedText));
    t->setPalette(pal);
}

void TableComponent::paintHighlight(int row, int col, const QColor &color)
{
    // 空格建 item，保证整行/整列高亮不留缝
    if (t->item(row, col) == nullptr)
    {
        setItemText(row, col, std::string());
    }
    if (QTableWidgetItem *item = t->item(row, col))
    {
        const bool native = !color.isValid();   // 无效色 = 原生高亮（Highlight + HighlightedText）
        HlCell hl;
        hl.r = row;
        hl.c = col;
        hl.back = native ? t->palette().brush(QPalette::Highlight) : QBrush(color);
        hl.fore = native ? t->palette().brush(QPalette::HighlightedText) : QBrush();
        item->setBackground(hl.back);
        item->setForeground(hl.fore);

        eraseCellRecord(row, col);
        m_hlCells.push_back(hl);
    }
}

void TableComponent::eraseCellIndex(int i)
{
    const HlCell &hl = m_hlCells[i];
    if (QTableWidgetItem *item = t->item(hl.r, hl.c))
    {
        item->setBackground(QBrush());
        item->setForeground(QBrush());
    }
    m_hlCells.erase(m_hlCells.begin() + i);
}

void TableComponent::eraseCellRecord(int row, int col)
{
    if (t == nullptr)
    {
        return;
    }
    for (int i = m_hlCells.size() - 1; i >= 0; --i)
    {
        if (m_hlCells[i].r == row && m_hlCells[i].c == col)
        {
            eraseCellIndex(i);
        }
    }
}

void TableComponent::scrollToIndex(int row, int col)
{
    if (t != nullptr)
    {
        t->scrollTo(t->model()->index(row, col));
    }
}

void TableComponent::ensureSize(int row, int col)
{
    if (row + 1 > t->rowCount())
    {
        t->setRowCount(row + 1);
    }
    if (col + 1 > t->columnCount())
    {
        t->setColumnCount(col + 1);
    }
}

QTableWidgetItem *TableComponent::setItemText(int row, int col, const std::string &text)
{
    if (t == nullptr || row < 0 || col < 0)
    {
        return nullptr;
    }
    ensureSize(row, col);

    const QString qtext = QString::fromStdString(text);
    QTableWidgetItem *item = t->item(row, col);
    if (item != nullptr)
    {
        item->setText(qtext);
    }
    else
    {
        item = new QTableWidgetItem(qtext);
        item->setTextAlignment(Qt::AlignCenter);
        t->setItem(row, col, item);
    }
    return item;
}
