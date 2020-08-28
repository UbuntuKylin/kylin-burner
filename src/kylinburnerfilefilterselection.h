#ifndef KYLINBURNERFILEFILTERSELECTION_H
#define KYLINBURNERFILEFILTERSELECTION_H

#include <QWidget>

namespace Ui {
class KylinBurnerFileFilterSelection;
}

class KylinBurnerFileFilterSelection : public QWidget
{
    Q_OBJECT

public:
    explicit KylinBurnerFileFilterSelection(QWidget *parent);
    ~KylinBurnerFileFilterSelection();
public:
    bool eventFilter(QObject *obj, QEvent *event);
    void labelCloseStyle(bool in);
    void setOption(bool hidden, bool broken, bool replace);
signals:
    void changeSetting(int option, bool enable);
private slots:
    void on_checkBoxHidden_stateChanged(int arg1);

    void on_checkBoxWrongSymbol_stateChanged(int arg1);

    void on_checkBoxReplaceSymbol_stateChanged(int arg1);

private:
    Ui::KylinBurnerFileFilterSelection *ui;
};

#endif // KYLINBURNERFILEFILTERSELECTION_H
