#include "SettingWidget.h"
#include "ui_SettingWidget.h"

static const char *kSettingSpinBoxStyle =
    "QDoubleSpinBox{border:1px solid #C0C0C0; border-radius:2px; background-color:#F0F0F5;}"
    "QDoubleSpinBox:focus{border:1px solid #4C70FA;}"
    "QDoubleSpinBox:hover{border:1px solid #A4B7FF;}";

static const char *kSettingBoolStyle =
    "QPushButton{background-color:#FFFFFF; border:1px solid #4D70FA; border-radius:5px;color:#4C70FA;}"
    "QPushButton:checked{border:none; background-color:#4C70FA;color:#FFFFFF;}";

static const char *kSettingSetButtonStyle =
    "QPushButton{background-color:rgba(115,143,253,180);color:#FFFFFF;border-radius:5px;font-size:16px;}"
    "QPushButton:hover{border:none; background-color:rgba(115,143,253,120);color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4D70FA;color:#FFFFFF;}"
    "QPushButton:disabled{border:none; background-color:#D4D4D4;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

SettingWidget::SettingWidget(int varId, const QString &name, bool isDouble,
                               QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingWidget)
    , m_varId(varId)
    , m_isDouble(isDouble)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->spb_FloatValue->setStyleSheet(kSettingSpinBoxStyle);
    ui->btn_BoolToggle->setStyleSheet(kSettingBoolStyle);
    ui->btn_SetValue->setStyleSheet(kSettingSetButtonStyle);
    ui->gridLayout->setColumnStretch(1, 1);

    if (isDouble)
    {
        ui->btn_BoolToggle->hide();
        ui->lbl_BoolText->hide();
        connect(ui->btn_SetValue, &QPushButton::clicked, this, [this]()
        {
            emit setSettingDouble(m_varId, ui->spb_FloatValue->value());
        });
    }
    else
    {
        ui->spb_FloatValue->hide();
        connect(ui->btn_BoolToggle, &QPushButton::toggled, this, [this](bool checked)
        {
            ui->lbl_BoolText->setText(checked ? "On" : "Off");
        });
        connect(ui->btn_SetValue, &QPushButton::clicked, this, [this]()
        {
            emit setSettingBool(m_varId, ui->btn_BoolToggle->isChecked());
        });
    }
}

SettingWidget::~SettingWidget()
{
    delete ui;
}
