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

typedef QList<K3b::DataDoc *> FDatas;
typedef struct _fstatus
{
    bool     isHidden;
    bool     isBroken;
    bool     isReplace;
} fstatus;

typedef QList<fstatus> FStatus;

class KylinBurnerFileFilter : public QWidget
{
    Q_OBJECT

public:
    explicit KylinBurnerFileFilter(QWidget *parent);
    ~KylinBurnerFileFilter();

public:
    bool eventFilter(QObject *obj, QEvent *event);

public:
    void setHidden(int , bool);
    void setBroken(int , bool);
    void setReplace(int , bool);
    void addData();
    void removeData(int index);
    void setDoFileFilter(int idx);
    K3b::DataDoc *getData(int idx)
    {
        if (idx > -1 && idx < datas.size()) return datas[idx];
        return NULL;
    }
    fstatus getStatus(int idx)
    {
        if (idx > -1 && idx < datas.size()) return stats[idx];
        return {false, false, false};
    }
    void reload(int);

signals:
    void finished(K3b::DataDoc *);
    void setOption(int, bool);

public slots:
    void slotDoubleClicked(QModelIndex idx);
    void slotDoFileFilter(K3b::DataDoc *doc);
    void slotDoChangeSetting(int option, bool enable);
private slots:
    void on_btnSetting_clicked();

    void on_btnRecovery_clicked();

private:
    void labelCloseStyle(bool in);

private:
    K3b::DataDoc          *currentData, *oldData;
    K3b::DataProjectModel *model;
    FDatas                 datas;
    FStatus                stats;
    bool                   isChange;
    bool                   isHidden;
    bool                   isBroken;
    bool                   isReplace;
    KylinBurnerFileFilterSelection *selection;
private:
    Ui::KylinBurnerFileFilter *ui;
};

#endif // KYLINBURNERFILEFILTER_H
