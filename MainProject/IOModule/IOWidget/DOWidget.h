#ifndef DOWIDGET_H
#define DOWIDGET_H

#include <QWidget>

namespace Ui { class DOWidget; }

class DOWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DOWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~DOWidget();

    int varId() const { return m_varId; }
    void display(bool value);

signals:
    void setDo(int varId, bool value);

private:
    Ui::DOWidget *ui;
    int m_varId;
};

#endif // DOWIDGET_H
