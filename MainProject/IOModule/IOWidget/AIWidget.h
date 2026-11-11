#ifndef AIWIDGET_H
#define AIWIDGET_H

#include <QWidget>

namespace Ui { class AIWidget; }

class AIWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AIWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~AIWidget();

    int varId() const { return m_varId; }
    void display(double value);

private:
    Ui::AIWidget *ui;
    int m_varId;
};

#endif // AIWIDGET_H
