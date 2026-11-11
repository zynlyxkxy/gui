#include "DOWidget.h"
#include "ui_DOWidget.h"

static const char *kDoButtonStyle =
    "QPushButton{background-color:#FFFFFF; border:1px solid #4D70FA; border-radius:5px;color:#4C70FA;}"
    "QPushButton:hover{border:none; background-color:#A4B7FF;color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4C70FA;color:#FFFFFF;}"
    "QPushButton:checked{border:none; background-color:#4C70FA;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

DOWidget::DOWidget(int varId, const QString &name, QWidget *parent)
    : QWidget(parent), ui(new Ui::DOWidget), m_varId(varId)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->btn_Status->setStyleSheet(kDoButtonStyle);

    connect(ui->btn_Status, &QPushButton::clicked, this, [this]() {
        emit setDo(m_varId, ui->btn_Status->isChecked());
    });
}

DOWidget::~DOWidget() { delete ui; }

void DOWidget::display(bool value)
{
    ui->btn_Status->setChecked(value);
}
