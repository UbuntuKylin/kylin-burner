/*
 *
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

    private Q_SLOTS:
        void slotParentDir();
        void slotCurrentDirChanged();
        void slotSetCurrentRoot( const QModelIndex& index );

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private:
        DataDoc* m_doc;
        DataViewImpl* m_dataViewImpl;
        QTreeView* m_dirView;
        DirProxyModel* m_dirProxy;

        QComboBox* combo_burner;
        QComboBox* combo_CD;
        QList<Device::Device *> device_index;
        QStringList mount_index;

        int iso_index;
        QString image_path;

        QPushButton* burn_setting;
        QPushButton* burn_button;
        QPushButton* button_add;
        QPushButton* button_remove;
        QPushButton* button_clear;
        QPushButton* button_newdir;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;  //事件过滤

    signals:
        void load(QString, DataDoc*);

    public slots:
        void onLoadFinished();

    protected:
        QThread *workerThread;
        LoadWorker *worker;
    };
}

#endif
