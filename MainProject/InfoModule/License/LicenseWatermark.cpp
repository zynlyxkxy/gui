/***********************************************************************
* Copyright (C) 2024, iStar.
*
* File Name:        LicenseWatermark.cpp
* Description:      授权宽限期提醒：在窗口上叠加斜向半透明水印
***********************************************************************/

#include "LicenseWatermark.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>

const char *LicenseWatermark::ObjectName = "licenseWatermark";

LicenseWatermark::LicenseWatermark(QWidget *target, const QString &text)
    : QWidget(target)
    , m_text(text)
    , m_color(200, 40, 40, 50)      // 半透明红：alpha 50/255
    , m_angle(-30.0)                // 斜向
    , m_minPointSize(18)
    , m_maxPointSize(56)
{
    setObjectName(QString::fromLatin1(ObjectName));

    // 只负责“画”，不参与任何交互：鼠标事件全部穿透到下层控件
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setFocusPolicy(Qt::NoFocus);

    if (target)
    {
        target->installEventFilter(this);   // 跟随目标窗口缩放/显示
    }

    FollowTarget();
    show();
}

LicenseWatermark::~LicenseWatermark()
{
    // 事件过滤器在对象销毁时 Qt 会自动摘除，这里显式移除只为语义清晰
    if (QObject *target = parent())
    {
        target->removeEventFilter(this);
    }
}

LicenseWatermark *LicenseWatermark::AttachTo(QWidget *target, const QString &text)
{
    if (!target)
    {
        return nullptr;
    }

    if (LicenseWatermark *watermark = FindOn(target))
    {
        // 已经贴过：只更新文字并重新置顶，避免重复叠加成多层
        watermark->SetText(text);
        watermark->FollowTarget();
        return watermark;
    }

    return new LicenseWatermark(target, text);
}

void LicenseWatermark::DetachFrom(QWidget *target)
{
    if (LicenseWatermark *watermark = FindOn(target))
    {
        delete watermark;
    }
}

LicenseWatermark *LicenseWatermark::FindOn(QWidget *target)
{
    if (!target)
    {
        return nullptr;
    }
    return target->findChild<LicenseWatermark *>(QString::fromLatin1(ObjectName));
}

QString LicenseWatermark::Text() const
{
    return m_text;
}

void LicenseWatermark::SetText(const QString &text)
{
    if (m_text == text)
    {
        return;
    }
    m_text = text;
    update();
}

QColor LicenseWatermark::Color() const
{
    return m_color;
}

void LicenseWatermark::SetColor(const QColor &color)
{
    m_color = color;
    update();
}

qreal LicenseWatermark::Angle() const
{
    return m_angle;
}

void LicenseWatermark::SetAngle(qreal degrees)
{
    m_angle = degrees;
    update();
}

void LicenseWatermark::SetFontPointSizeRange(int minPointSize, int maxPointSize)
{
    m_minPointSize = qMax(1, minPointSize);
    m_maxPointSize = qMax(m_minPointSize, maxPointSize);
    update();
}

bool LicenseWatermark::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget())
    {
        switch (event->type())
        {
        case QEvent::Resize:
        case QEvent::Show:
            FollowTarget();
            break;

        case QEvent::ChildAdded:
            raise();        // 之后新增/置顶的子控件不要盖住水印
            break;

        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void LicenseWatermark::FollowTarget()
{
    if (QWidget *target = parentWidget())
    {
        setGeometry(target->rect());    // 铺满目标窗口（不走布局，纯手工定位）
    }
    raise();
}

void LicenseWatermark::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (m_text.isEmpty() || width() <= 0 || height() <= 0)
    {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // 字号随窗口宽度自适应，并限制在设定范围内
    QFont watermarkFont = font();
    watermarkFont.setBold(true);
    watermarkFont.setPointSize(qBound(m_minPointSize, width() / 32, m_maxPointSize));
    painter.setFont(watermarkFont);
    painter.setPen(m_color);

    const QFontMetrics metrics(watermarkFont);
    const int stepX = metrics.horizontalAdvance(m_text) + 160;   // 横向平铺间距
    const int stepY = metrics.height() * 5;                      // 纵向平铺间距
    if (stepX <= 0 || stepY <= 0)
    {
        return;                     // 防御：避免下面的循环无法推进
    }

    // 旋转后仍能铺满整个窗口
    const int radius = (width() + height()) / 2 + stepX;

    painter.translate(rect().center());
    painter.rotate(m_angle);

    for (int y = -radius; y <= radius; y += stepY)
    {
        for (int x = -radius; x <= radius; x += stepX)
        {
            painter.drawText(QPointF(x, y), m_text);
        }
    }
}
