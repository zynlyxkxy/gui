#include "DIWidget.h"
#include "ui_DIWidget.h"

static const char *kDiButtonStyle =
    "QPushButton{background-color:#FFFFFF; border:1px solid #4D70FA; border-radius:5px;color:#4C70FA;}"
    "QPushButton:checked{border:none; background-color:#4C70FA;color:#FFFFFF;}";

DIWidget::DIWidget(int varId, const QString &name, QWidget *parent)
    : QWidget(parent), ui(new Ui::DIWidget), m_varId(varId)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->btn_Status->setStyleSheet(kDiButtonStyle);
    ui->btn_Status->setAttribute(Qt::WA_TransparentForMouseEvents);
}

DIWidget::~DIWidget() { delete ui; }

void DIWidget::display(bool value)
{
    ui->btn_Status->setChecked(value);
}
