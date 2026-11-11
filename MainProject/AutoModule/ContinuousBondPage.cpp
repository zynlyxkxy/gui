#include "ContinuousBondPage.h"
#include "ui_ContinuousBondPage.h"
#include "Common/UiTranslator.h"          // TrKeyRole（键合表动态格的翻译 key）
#include "istar_language.h"               // Tr()：值/名字文字按当前语言取
#include "istar_ui_stylesheet.h"
#include "dialog/messagedialog.h"
#include "groupbox/groupbox.h"
#include "istar_serializer.hpp"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

using namespace ISTAR::GUI;
using namespace ISTAR::UIStyle;

ContinuousBondPage::ContinuousBondPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContinuousBondPage)
{
    ui->setupUi(this);

    initStyle();
    initUi();
    initTable();
}

ContinuousBondPage::~ContinuousBondPage()
{
    delete ui;
}

void ContinuousBondPage::refreshTexts()
{
    m_uiTranslator.refresh(this);
}

void ContinuousBondPage::initStyle()
{
    QList<GroupBox*> lst_gb = this->findChildren<GroupBox*>();
    for (auto &i : lst_gb)
    {
        i->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
        i->SetheaderFontSize(12);
    }

    QList<QPushButton*> lst_btn = this->findChildren<QPushButton*>();
    for(auto &b : lst_btn)
    {
        b->setStyleSheet(iStarUIStyleSheet->Button_Border + iStarUIStyleSheet->fontStyle_normal_12);

    }

    QList<QDoubleSpinBox*> lst_dsb = this->findChildren<QDoubleSpinBox*>();
    for (auto &dsb : lst_dsb)
    {
        dsb->setStyleSheet(iStarUIStyleSheet->DoubleSpinBox_Dark);
        dsb->installEventFilter(this);
    }
    // QDateTimeEdit 不是 QDoubleSpinBox 的子类，DoubleSpinBox_Dark 里的 "QDoubleSpinBox{...}"
    // 选择器一条也匹配不上（setStyleSheet 直接套用无效），把选择器名替换成 QDateTimeEdit 才生效
    const QString dtEditStyle =
        QString(iStarUIStyleSheet->DoubleSpinBox_Dark).replace(QStringLiteral("QDoubleSpinBox"),
                                                               QStringLiteral("QDateTimeEdit"));
    ui->dateTimeEdit->setStyleSheet(dtEditStyle);

    QList<QLabel*> lst_label = this->findChildren<QLabel*>();
    for (auto &l : lst_label)
    {
        // .ui 里自带样式的标签（如 Yellow / Red 色块）不能直接覆盖：setStyleSheet 是整体替换，
        // 一覆盖底色就没了。合并时还有个坑：Designer 写出的是裸声明（例如
        // "background-color: rgb(255, 204, 1);"），拼在规则块后面 Qt 会把这段整段丢掉，
        // 必须包上选择器变成合法规则（已用离屏渲染取像素实测过）。
        const QString ownStyle = l->styleSheet();
        QString style = iStarUIStyleSheet->fontStyle_normal_12;
        if (!ownStyle.isEmpty())
        {
            style += ownStyle.contains(QLatin1Char(0x7B))
                         ? ownStyle
                         : QStringLiteral("QLabel{") + ownStyle + QLatin1Char(0x7D);
        }
        l->setStyleSheet(style);
    }
    ui->label_cur->setStyleSheet(iStarUIStyleSheet->fontStyle_bold_16);

    QList<QCheckBox*> lst_CheckBox = this->findChildren<QCheckBox*>();
    for(auto &CheckBox : lst_CheckBox)
    {
        CheckBox->setStyleSheet(iStarUIStyleSheet->checkbox + iStarUIStyleSheet->fontStyle_normal_12);

    }

    // 蓝色装饰条（平台 GroupBox 的 SetDecorateColor 在本 DLL 版本不渲染，手动补）
    for (auto *gb : this->findChildren<GroupBox*>())
    {
        QLabel *strip = new QLabel(gb);
        strip->setFixedSize(5, 24);
        strip->setStyleSheet("background-color:#4C70FA;border:none;");
        strip->move(0, 12);
        strip->show();
    }
}

void ContinuousBondPage::initUi()
{
    ui->gb_Proc->SetHeaderText(QStringLiteral("Parameter Settings"));
    ui->gb_Result->SetHeaderText(QStringLiteral("Result"));

    ui->btn_refresh->setFixedWidth(100);
    ui->btn_selectAll->setFixedWidth(100);
}

void ContinuousBondPage::initTable()
{
    m_table.bind(ui->tw_Data);   // setupUi 之后才能拿到控件，包装只能先建后绑

    // 列宽按 1:5:3:3:3:3:3（Index / Time / Point1~4 补偿值 / 四点均值），窗口拉伸自动保持比例
    m_table.setColumnStretchRatios({1.0, 5.0, 3.0, 3.0, 3.0, 3.0, 3.0});
}
