#ifndef UITRANSLATOR_H
#define UITRANSLATOR_H

#include <QtCore/qnamespace.h>   // Qt::UserRole（自定义 role 基准编号）
#include "chart/qcustomplot.h"

class QWidget;

// ---- 自定义 item role 分配表（全工程唯一登记处，新增 role 先来这登记）----
// Qt::UserRole + 0 : 业务标记/列标签（BondProc "header"/rawCmd、ProcessPage 列标签）
// Qt::UserRole + 1 : "check" 参数类型标记（BondProc）、COLUMN_SPAN_ROLE（平台 RbTableHeaderView）
// Qt::UserRole + 20: CellValuesRole（TableComponent 格子原始数值，显示文本按精度截断而数据不丢）
// Qt::UserRole + 30: 翻译 key（表格格子的英文原稿）
enum
{
    TrKeyRole = Qt::UserRole + 30,   // 表格格子的英文原稿：setData(TrKeyRole, key)
};

/**
 * 界面文本翻译器（文本即 key，每个页面一个实例）：
 *
 * refresh(root)：按当前平台语言重刷 root 下所有"认识的"控件文本，语言切换/启动时调用。
 *   机制（懒惰捕获）：每个控件第一次被刷新时，若其当前文本是纯 ASCII 英文（.ui 原稿/
 *   构造函数声明的英文），把它捕获为 key 存入控件动态属性；此后每次刷新都用存的 key
 *   查词典，永不重读当前显示文本 —— 中文切回英文恒正确。
 *
 *   覆盖范围：
 *   - QTabWidget 逐个 tab 标题（key 缓存于 "trTabKeys"）
 *   - QAbstractButton 按钮文本（含 QPushButton/QCheckBox/EffectButton，缓存于 "trKey"）
 *   - QLabel 文本（含各 GroupBox 内部标题 lbl_Title，缓存于 "trKey"）
 *   - QTableWidget 表头列（ASCII 懒惰捕获，缓存于 "trHeaderKeys"）
 *   - QTableWidget 行头/竖表头（同上，缓存于 "trRowHeaderKeys"）
 *   - QTableWidget 单元格：仅 TrKeyRole(UserRole+30) 存有 key 的才翻译 ——
 *     动态内容填格时 setData(TrKeyRole, 英文原稿)；数值/数据格没有 key，永不触碰
 *   - QLineEdit 占位文本（缓存于 "trPlaceholderKey"）
 *   - QGraphicsView 图例：仅当视图带动态属性 "trLegend"（见 DiagramBase::setLegendVisible）
 *     时触发一次重绘；图例文字本身在 paintEvent 里按当前语言现算，不经本类捕获
 *
 * 词典查不到词条 → Tr 返回原文 = 英文兜底，无需特殊处理。
 * 本类无任何状态、无注册：每次 refresh 现场扫描，新建控件自动收编，无时序要求。
 */
class UiTranslator
{
public:
    UiTranslator() = default;
    ~UiTranslator() = default;

    // 重刷 root 下全部可翻译控件（语言切换时调用；每次现扫，新建控件自动收编）
    void refresh(QWidget *root);
};

#endif // UITRANSLATOR_H
