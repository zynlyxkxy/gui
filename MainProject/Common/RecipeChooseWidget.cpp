#include "RecipeChooseWidget.h"

#include "istar_ui_stylesheet.h"
#include "istar_language.h"

#define TR_STR(msg)  QString::fromStdString(Tr(msg))
#define TR_STRQ(msg) QString::fromStdString(Tr((msg).toStdString()))

#include <algorithm>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QCollator>
#include <QDebug>
#include <QString>

using namespace ISTAR::UIStyle;

RecipeChooseWidget::RecipeChooseWidget(QWidget *parent)
    : QDialog(parent)
    , m_pWgt(nullptr)
    , m_pScroll(new QScrollArea)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QFrame *pFrame = new QFrame;
    pFrame->setStyleSheet("QFrame{background-color:#FFFFFF;border-radius:10px;}");

    QVBoxLayout *pFrameLayout = new QVBoxLayout(this);
    pFrameLayout->addWidget(pFrame);

    m_pMainLayout = new QVBoxLayout(pFrame);
    m_pMainLayout->setContentsMargins(40, 20, 40, 20);
    m_pMainLayout->setSpacing(20);

    QLabel *pTitle = new QLabel(TR_STR("Select Recipe"));
    pTitle->setProperty("trKey", QStringLiteral("Select Recipe"));   // 供 ProcessPage::refreshTexts 原地切换
    pTitle->setStyleSheet(iStarUIStyleSheet->fontStyle_bold_14);
    m_pMainLayout->addWidget(pTitle, 0, Qt::AlignLeft);

    m_pMainLayout->addWidget(m_pScroll);
    m_pScroll->setFixedSize(300, 500);

    QHBoxLayout *pHLayout = new QHBoxLayout;
    m_pMainLayout->addLayout(pHLayout);

    QPushButton *pOkBtn = new QPushButton(TR_STR("OK"));
    pOkBtn->setProperty("trKey", QStringLiteral("OK"));   // 供 ProcessPage::refreshTexts 原地切换
    pOkBtn->setFixedSize(120, 40);
    pOkBtn->setStyleSheet(iStarUIStyleSheet->Button_Border + iStarUIStyleSheet->fontStyle_bold_12);
    pHLayout->addWidget(pOkBtn);
    connect(pOkBtn, &QPushButton::clicked, this, [=]()
    {
        if (!m_strChoose.isEmpty())
        {
            emit sig_recipeChosen(m_strChoose);
        }
        accept();
    });

    QPushButton *pCancelBtn = new QPushButton(TR_STR("Cancel"));
    pCancelBtn->setProperty("trKey", QStringLiteral("Cancel"));   // 供 ProcessPage::refreshTexts 原地切换
    pCancelBtn->setFixedSize(120, 40);
    pCancelBtn->setStyleSheet(iStarUIStyleSheet->Button_Border + iStarUIStyleSheet->fontStyle_bold_12);
    pHLayout->addWidget(pCancelBtn);
    connect(pCancelBtn, &QPushButton::clicked, this, [=]()
    {
        reject();
    });
}

void RecipeChooseWidget::SetLst(const QStringList &lst)
{
    m_strChoose.clear();
    if (m_pWgt)
    {
        delete m_pWgt;
        m_pWgt = nullptr;
    }
    if (m_pScroll)
    {
        delete m_pScroll;
        m_pScroll = nullptr;
    }

    m_pWgt = new QWidget;
    m_pWgt->setStyleSheet("QWidget{background:transparent}");
    QVBoxLayout *pLayout = new QVBoxLayout(m_pWgt);

    QCollator collator;
    collator.setNumericMode(true);
    QList<QString> lsSort = lst;
    std::sort(lsSort.begin(), lsSort.end(),
              [&collator](const QString &a, const QString &b)
    {
        return collator.compare(a, b) < 0;
    });

    for (int i = 0; i < lsSort.size(); ++i)
    {
        QRadioButton *pBtn = new QRadioButton(lsSort[i]);
        pBtn->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
        pLayout->addWidget(pBtn);
        connect(pBtn, &QRadioButton::toggled, this, [=](bool b)
        {
            if (b)
            {
                m_strChoose = pBtn->text();
            }
        });
    }

    m_pScroll = new QScrollArea;
    m_pScroll->setWidget(m_pWgt);
    m_pScroll->setFixedSize(300, 500);
    m_pMainLayout->insertWidget(1, m_pScroll);


}
