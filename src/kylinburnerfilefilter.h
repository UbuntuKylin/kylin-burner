/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

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
