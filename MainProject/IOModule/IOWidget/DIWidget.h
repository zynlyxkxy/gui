#ifndef DIWIDGET_H
#define DIWIDGET_H

#include <QWidget>

namespace Ui { class DIWidget; }

class DIWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DIWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~DIWidget();

    int varId() const { return m_varId; }
    void display(bool value);

private:
    Ui::DIWidget *ui;
    int m_varId;
};

#endif // DIWIDGET_H
