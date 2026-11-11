#include "AIWidget.h"
#include "ui_AIWidget.h"

#include <cmath>

static const char *kAiSpinBoxStyle =
    "QDoubleSpinBox{border-radius:2px;background-color:#F3F4FA;}"
    "QDoubleSpinBox:focus{border:1px solid #4C70FA;}"
    "QDoubleSpinBox:hover{border:1px solid #A4B7FF;}"
    "QDoubleSpinBox:disabled{background-color:#C8C8C8;}";

AIWidget::AIWidget(int varId, const QString &name, QWidget *parent)
    : QWidget(parent), ui(new Ui::AIWidget), m_varId(varId)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->spb_Value->setStyleSheet(kAiSpinBoxStyle);
}

AIWidget::~AIWidget() { delete ui; }

void AIWidget::display(double value)
{
    double absVal = qAbs(value);
    int intDigits;
    if (absVal < 1.0)
        intDigits = 0;
    else
        intDigits = static_cast<int>(std::floor(std::log10(absVal))) + 1;
    int decimals = qMax(0, 12 - intDigits);

    // QDoubleSpinBox 内部 int64 存值×10^decimals，范围不能超过 INT64_MAX/10^decimals
    double safeMax = 9.0e18 / std::pow(10.0, decimals);

    ui->spb_Value->setDecimals(decimals);
    ui->spb_Value->setRange(-safeMax, safeMax);
    ui->spb_Value->setValue(value);
}
