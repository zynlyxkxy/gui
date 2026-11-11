#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QWidget>

namespace Ui { class SettingWidget; }

class SettingWidget : public QWidget
{
    Q_OBJECT
public:
    // isDouble=true → SettingDouble, isDouble=false → SettingBool
    explicit SettingWidget(int varId, const QString &name, bool isDouble,
                           QWidget *parent = nullptr);
    ~SettingWidget();

    int varId() const { return m_varId; }

signals:
    void setSettingDouble(int varId, double value);
    void setSettingBool(int varId, bool value);

private:
    Ui::SettingWidget *ui;
    int m_varId;
    bool m_isDouble;
};

#endif // SETTINGWIDGET_H
