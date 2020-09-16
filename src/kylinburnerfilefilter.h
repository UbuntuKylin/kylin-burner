#ifndef KYLINBURNERFILEFILTER_H
#define KYLINBURNERFILEFILTER_H

#include "k3bdataprojectmodel.h"
#include "k3bdatadoc.h"
#include "kylinburnerfilefilterselection.h"

#include <QWidget>
#include <QEvent>

namespace Ui {
class KylinBurnerFileFilter;
}

class KylinBurnerFileFilter : public QWidget
{
    Q_OBJECT

public:
    explicit KylinBurnerFileFilter(QWidget *parent);
    ~KylinBurnerFileFilter();

public:
    bool eventFilter(QObject *obj, QEvent *event);
signals:
    void finished(K3b::DataDoc *);
public slots:
    void slotDoubleClicked(QModelIndex idx);
    void slotDoFileFilter(K3b::DataDoc *doc);
    void slotDoChangeSetting(int option, bool enable);
private slots:
    void on_btnSetting_clicked();

    void on_btnRecovery_clicked();

private:
    void labelCloseStyle(bool in);
    void onChanged(K3b::DirItem *parent);

private:
    K3b::DataDoc          *currentData, *oldData;
    K3b::DataProjectModel *model;
    bool                   isChange;
    bool                   isHidden;
    bool                   isBroken;
    bool                   isReplace;
    KylinBurnerFileFilterSelection *selection;
private:
    Ui::KylinBurnerFileFilter *ui;
};

#endif // KYLINBURNERFILEFILTER_H
