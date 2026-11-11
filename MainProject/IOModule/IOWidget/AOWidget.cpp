#include "AOWidget.h"
#include "ui_AOWidget.h"

#include <cmath>

static const char *kAoCurrentStyle =
    "QDoubleSpinBox{border-radius:2px;background-color:#C8C8C8;}"
    "QDoubleSpinBox:disabled{background-color:#C8C8C8;}";

static const char *kAoTargetStyle =
    "QDoubleSpinBox{border:1px solid #C0C0C0; border-radius:2px; background-color:#F0F0F5;}"
    "QDoubleSpinBox:focus{border:1px solid #4C70FA;}"
    "QDoubleSpinBox:hover{border:1px solid #A4B7FF;}";

static const char *kAoSetButtonStyle =
    "QPushButton{background-color:rgba(115,143,253,180);color:#FFFFFF;border-radius:5px;font-size:16px;}"
    "QPushButton:hover{border:none; background-color:rgba(115,143,253,120);color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4D70FA;color:#FFFFFF;}"
    "QPushButton:disabled{border:none; background-color:#D4D4D4;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

AOWidget::AOWidget(int varId, const QString &name, QWidget *parent)
    : QWidget(parent), ui(new Ui::AOWidget), m_varId(varId)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->spb_CurrentValue->setStyleSheet(kAoCurrentStyle);
    ui->spb_TargetValue->setStyleSheet(kAoTargetStyle);
    ui->btn_SetValue->setStyleSheet(kAoSetButtonStyle);

    // 名称列拉伸，把填写框推到右端
    ui->gridLayout->setColumnStretch(1, 1);

    connect(ui->btn_SetValue, &QPushButton::clicked, this, [this]() {
        emit setAo(m_varId, ui->spb_TargetValue->value());
    });
}

AOWidget::~AOWidget() { delete ui; }

void AOWidget::display(double currentValue)
{
    double absVal = qAbs(currentValue);
    int intDigits;
    if (absVal < 1.0)
        intDigits = 0;
    else
        intDigits = static_cast<int>(std::floor(std::log10(absVal))) + 1;
    int decimals = qMax(0, 12 - intDigits);

    double safeMax = 9.0e18 / std::pow(10.0, decimals);

    ui->spb_CurrentValue->setDecimals(decimals);
    ui->spb_CurrentValue->setRange(-safeMax, safeMax);
    ui->spb_CurrentValue->setValue(currentValue);
}
