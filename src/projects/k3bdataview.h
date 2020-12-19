/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include "k3bview.h"
#include <QThread>
#include <QMutex>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QDialog>

#include "kylinburnerfilefilter.h"
#include "kylinburnerlogger.h"

class QModelIndex;
class QTreeView;
class QComboBox;
class QPushButton;

namespace K3b {
    class DataDoc;
    class DataProjectModel;
    class DataViewImpl;
    class DirProxyModel;

    namespace Device{
        class Device;
        class DeviceManager;
    }
    
    //加载文件到线程类
    class LoadWorker: public QObject
    {
        Q_OBJECT

    signals:
        void loadFinished();

    public slots:
        void load(QString, DataDoc*);
    };

    class DataView : public View
    {
        Q_OBJECT

    public:
        explicit DataView( DataDoc* doc, QWidget* parent = 0 );
        ~DataView() override;

        Device::Device * testGetDev()
        {
            if (device_index.size()) return device_index[0];
            return NULL;
        }

    public Q_SLOTS:
        void slotBurn() override;
        void slotStartBurn();
        void add_device_urls(QString filenpath);
        void addUrls( const QList<QUrl>& urls ) override;
        void slotMediaChange( K3b::Device::Device* );
        void slotDeviceChange( K3b::Device::DeviceManager* );
        void slotComboCD(int);
        void slotComboBurner(int);
        void slotOpenClicked();
        void slotRemoveClicked();
        void slotClearClicked();
        void slotNewdirClicked();
        void slotFileFilterClicked();
        void slotAddFile(QList<QUrl>);
        void slotFinish(K3b::DataDoc *);

        void isHidden(bool);
        void isBroken(bool);
        void isReplace(bool);

        void addDragFiles(QList<QUrl> urls, K3b::DirItem* targetDir);


    private Q_SLOTS:
        void slotParentDir();
        void slotCurrentDirChanged();
        void slotSetCurrentRoot( const QModelIndex& index );
        void slotOption(int, bool);

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private:
        DataDoc* m_doc;
        QList<DataDoc *> docs;
        int              lastIndex;
        int              comboIndex;
        DataViewImpl* m_dataViewImpl;
        QTreeView* m_dirView;
        DirProxyModel* m_dirProxy;
        QWidget        *mainWindow;
        KylinBurnerFileFilter *dlgFileFilter;
        ProjectBurnDialog* pdlg;

        QComboBox* combo_burner;
        QComboBox* combo_CD;
        QList<Device::Device *> device_index;
        QStringList mount_index;

        int iso_index;
        QString image_path;

        QPushButton* burn_setting; // 14px
        QPushButton* burn_button;// 18px
        QPushButton* button_add; // 14px
        QPushButton* button_remove;
        QPushButton* button_clear;
        QPushButton* button_newdir;
        QPushButton* btnFileFilter;
        QLabel      *tips;
        KylinBurnerLogger *logger;
        QList<QUrl>  lastDrop;

    private:
        void enableBurnSetting();
        void hoverBurnSetting(bool in = true);
        void pressBurnSetting();
        void disableBurnSetting();
        void enableButtonBurn();
        void hoverButtonBurn(bool in = true);
        void pressButtonBurn();
        void disableButtonBurn();
        void enableButtonAdd();
        void hoverButtonAdd(bool in = true);
        void pressButtonAdd();
        void disableButtonAdd();
        void enableButtonRemove();
        void hoverButtonRemove(bool in = true);
        void pressButtonRemove();
        void disableButtonRemove();
        void enableButtonClear();
        void hoverButtonClear(bool in = true);
        void pressButtonClear();
        void disableButtonClear();
        void enableButtonNewDir();
        void hoverButtonNewDir(bool in = true);
        void pressButtonNewDir();
        void disableButtonNewDir();

        void copyData(K3b::DataDoc *target, K3b::DataDoc *source);
        bool checkIsDeleteable(K3b::DataDoc *);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;  //事件过滤
        void paintEvent(QPaintEvent *);

    signals:
        void load(QString, DataDoc*);
        void disableCD(bool);
        //void doFileFilter(K3b::DataDoc *doc);
        void setIsHidden(bool);
        void setIsBroken(bool);
        void setIsReplace(bool);

    public slots:
        void onLoadFinished();
        void onDataChange(QModelIndex parent, QSortFilterProxyModel *model);
        void onDataDelete(bool flag);
        void slotDisableCD(bool);

    protected:
        QThread *workerThread;
        LoadWorker *worker;
    };
}

#endif
