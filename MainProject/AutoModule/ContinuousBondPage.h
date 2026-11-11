#ifndef CONTINUOUSBONDPAGE_H
#define CONTINUOUSBONDPAGE_H

#include <QWidget>
#include "Common/UiTranslator.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

namespace Ui {
class ContinuousBondPage;
}

class ContinuousBondPage : public QWidget
{
    Q_OBJECT

public:
    explicit ContinuousBondPage(QWidget *parent = nullptr);
    ~ContinuousBondPage();

    void refreshTexts();

private:
    void initStyle();   // 全控件样式（分组框/按钮/输入框/标签/勾选框/装饰条）
    void initUi();      // 一次性界面设置（分组框标题、按钮宽度）
    void initTable();   // 键合数据表：绑定包装 + 列宽比例

    Ui::ContinuousBondPage *ui;
    UiTranslator m_uiTranslator;
    TableComponent m_table;
};

#endif // CONTINUOUSBONDPAGE_H
