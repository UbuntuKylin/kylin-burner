/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BVCDVIEW_H
#define K3BVCDVIEW_H

#include "k3bview.h"

#include <QModelIndex>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>

class QAction;
class QTreeView;
class QWidget;

namespace K3b {

    class ProjectBurnDialog;
    class VcdDoc;
    class VcdProjectModel;
    namespace Device {
        class DeviceManager;
        class Device;
    }

    class VcdView : public View
    {
        Q_OBJECT

        public:
            VcdView( VcdDoc* doc, QWidget* parent );
            ~VcdView() override;
   
            void MediaCopy( K3b::Device::Device* dev );
      
        public Q_SLOTS:
            void slotOpenfile();
            void slotSetting();
            void slotStartBurn();
           
            //*******************************************
            void slotComboCD( int );
            void slotComboISO( int );
            void slotMediaChange( K3b::Device::Device* );
            void slotDeviceChange( K3b::Device::DeviceManager* );

        private Q_SLOTS:
            void slotSelectionChanged();
            void slotProperties() override;
            void slotRemove();
            void slotItemActivated( const QModelIndex& index );

        protected:
            ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private:
            void enableBurnerStart()
            {
                button_start->setEnabled(true);
                button_start->setStyleSheet("QPushButton{"
                                            "background-color:rgba(61, 107, 229, 1);"
                                            "font: 18px;border-radius: 4px;color:#ffffff}"
                                            "QPushButton:hover{background-color:rgba(61, 107, 229, 1);}");
            }
            void disableBurnerStart()
            {
                button_start->setEnabled(false);
                button_start->setStyleSheet("background-color:rgba(233, 233, 233, 1);"
                                            "font: 18px;border-radius: 4px;color:#C1C1C1");
            }
            void init();
        public:
            QLabel* label_CD;
            QComboBox* combo_CD;
            QComboBox* combo_iso;
            QPushButton* button_openfile;
            QPushButton* button_start;
            QString filepath;
            int flag;
            int CD_index;
            int device_count;

            QList<Device::Device*> device_index;

        private:
            VcdDoc* m_doc;
            VcdProjectModel* m_model;
            QTreeView* m_view;
            QString image_path;
            int     comboIndex;
            int     lastIndex;
            int     lastSourceIndex;
            bool    isBurner;
            QList<K3b::Device::Device*> cdDevices;
            QList<K3b::Device::Device*> sourceDevices;

            QAction* m_actionProperties;
            QAction* m_actionRemove;
    };
}

#endif
