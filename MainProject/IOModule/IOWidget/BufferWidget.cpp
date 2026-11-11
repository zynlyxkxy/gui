#include "BufferWidget.h"
#include "ui_BufferWidget.h"

static const char *kBufButtonStyle =
    "QPushButton{background-color:rgba(115,143,253,180);color:#FFFFFF;border-radius:5px;font-size:14px;}"
    "QPushButton:hover{border:none; background-color:rgba(115,143,253,120);color:#FFFFFF;}"
    "QPushButton:pressed{border:none; background-color:#4D70FA;color:#FFFFFF;}"
    "QPushButton:disabled{border:none; background-color:#D4D4D4;color:#FFFFFF;}"
    "QLabel{background-color:transparent;}";

static const char *kBufStatusStyle =
    "QPushButton{background-color:#FFFFFF; border:1px solid #4D70FA; border-radius:5px;color:#4C70FA;}"
    "QPushButton:checked{border:none; background-color:#4C70FA;color:#FFFFFF;}";

BufferWidget::BufferWidget(int varId, const QString &name, QWidget *parent)
    : QWidget(parent), ui(new Ui::BufferWidget), m_varId(varId)
{
    ui->setupUi(this);
    ui->lbl_Index->setText(QString("%1").arg(varId, 4, 10, QChar('0')));
    ui->lbl_Name->setText(name);
    ui->btn_Start->setStyleSheet(kBufButtonStyle);
    ui->btn_Stop->setStyleSheet(kBufButtonStyle);
    ui->btn_Status->setStyleSheet(kBufStatusStyle);
    ui->btn_Status->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->gridLayout->setColumnStretch(1, 1);

    connect(ui->btn_Start, &QPushButton::clicked, this, [this]() {
        emit startBuffer(m_varId);
    });
    connect(ui->btn_Stop, &QPushButton::clicked, this, [this]() {
        emit stopBuffer(m_varId);
    });
}

BufferWidget::~BufferWidget() { delete ui; }

void BufferWidget::display(bool running)
{
    ui->btn_Status->setChecked(running);
}
