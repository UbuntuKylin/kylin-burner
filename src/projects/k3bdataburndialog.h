/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BDATABURNDIALOG_H
#define K3BDATABURNDIALOG_H

#include "k3bprojectburndialog.h"

class QCheckBox;
class QGroupBox;
class QLabel;

namespace K3b {
    class DataDoc;
    class DataImageSettingsWidget;
    class DataModeWidget;
    class DataMultiSessionCombobox;
    
    namespace Device {
        class DeviceManager;
        class Device;
    }


    /**
     *@author Sebastian Trueg
     */
    class DataBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit DataBurnDialog(DataDoc*, QWidget *parent=0 );
        ~DataBurnDialog() override;
        
    void setComboMedium( K3b::Device::Device* dev );
    void saveConfig();

    protected:
        void setupSettingsTab();

        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void toggleAll() override;
        void paintEvent(QPaintEvent *);


        // --- settings tab ---------------------------
        DataImageSettingsWidget* m_imageSettingsWidget;
        // ----------------------------------------------

        QGroupBox* m_groupDataMode;
        DataModeWidget* m_dataModeWidget;
        DataMultiSessionCombobox* m_comboMultisession;

        QCheckBox* m_checkVerify;

    //protected Q_SLOTS:
    public Q_SLOTS:
        //void slotClose();
        void slotCancelClicked() override;
        void slotStartClicked() override;
        void slotHide() {hide();}
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

        void slotMultiSessionModeChanged();
    public:
        DataDoc*  m_doc;
    };
}

#endif
