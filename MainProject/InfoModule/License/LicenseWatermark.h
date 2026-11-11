/***********************************************************************
* Copyright (C) 2024, iStar.
*
* File Name:        LicenseWatermark.h
* Description:      授权宽限期提醒：在窗口上叠加斜向半透明水印
***********************************************************************/

#ifndef LICENSEWATERMARK_H
#define LICENSEWATERMARK_H

#include <QColor>
#include <QString>
#include <QWidget>

/// 斜向半透明水印遮罩（用于授权宽限期提醒）
///
/// 用法：
///     LicenseWatermark::AttachTo(mainWindow, QStringLiteral("软件已到期，剩余3天，及时联系生产厂商"));
///     ...
///     LicenseWatermark::DetachFrom(mainWindow);
///
/// 特性：
///   - 铺满目标窗口，随窗口缩放自动跟随，并始终保持在该窗口子控件的最上层
///   - Qt::WA_TransparentForMouseEvents：不拦截任何鼠标事件，界面照常可操作
///   - 文字按 -30° 斜向平铺，半透明，字号随窗口宽度自适应
///   - 父对象即目标窗口，随窗口一起销毁，不会泄漏
class LicenseWatermark : public QWidget
{
    Q_OBJECT

public:
    /// @param target 被叠加水印的窗口/控件，同时作为父对象
    /// @param text   水印文字，为空则不绘制任何内容
    explicit LicenseWatermark(QWidget *target, const QString &text = QString());
    ~LicenseWatermark() override;

    /// @brief 在 target 上叠加水印；若已存在则只更新文字并置顶，不会重复叠加
    /// @return 水印对象；target 为空时返回 nullptr（不做任何事）
    static LicenseWatermark *AttachTo(QWidget *target, const QString &text);

    /// @brief 移除 target 上的水印（不存在则什么也不做）
    static void DetachFrom(QWidget *target);

    /// @brief 取 target 上已有的水印对象（无则返回 nullptr）
    static LicenseWatermark *FindOn(QWidget *target);

    /// @brief 水印控件的对象名（AttachTo / FindOn 靠它做查重）
    static const char *ObjectName;

    QString Text() const;
    void SetText(const QString &text);

    QColor Color() const;
    void SetColor(const QColor &color);          ///< 建议传半透明色，如 QColor(200, 40, 40, 50)

    qreal Angle() const;
    void SetAngle(qreal degrees);                ///< 斜向角度，默认 -30

    /// @brief 字号范围（随窗口宽度自适应，默认 18 ~ 56 pt）
    void SetFontPointSizeRange(int minPointSize, int maxPointSize);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    /// @brief 跟随目标窗口的尺寸/位置
    void FollowTarget();

    QString m_text;
    QColor  m_color;
    qreal   m_angle;
    int     m_minPointSize;
    int     m_maxPointSize;
};

#endif // LICENSEWATERMARK_H
