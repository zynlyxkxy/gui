#ifndef AOWIDGET_H
#define AOWIDGET_H

#include <QWidget>

namespace Ui { class AOWidget; }

class AOWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AOWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~AOWidget();

    int varId() const { return m_varId; }
    void display(double currentValue);

signals:
    void setAo(int varId, double value);

private:
    Ui::AOWidget *ui;
    int m_varId;
};

#endif // AOWIDGET_H
