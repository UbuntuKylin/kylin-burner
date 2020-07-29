/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_COPY_DIALOG_H_
#define _K3B_MEDIA_COPY_DIALOG_H_

#include "k3binteractiondialog.h"
#include <KIO/Global>

class QCheckBox;
class QSpinBox;
class QGroupBox;
class QComboBox;

namespace K3b {
    namespace Device {
        class Device;
    }

    class WriterSelectionWidget;
    class TempDirSelectionWidget;
    class MediaSelectionComboBox;
    class WritingModeWidget;

    class MediaCopyDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit MediaCopyDialog( QWidget *parent = 0 );
        ~MediaCopyDialog() override;
        
        void setComboMedium( K3b::Device::Device* dev );
        void setTempDirPath( const QString& dir );
        void saveConfig();
        void loadConfig();
        void setOnlyCreateImage( bool ret);
        QCheckBox* m_checkOnlyCreateImage;
        QCheckBox* m_checkCacheImage;

        void setReadingDevice( Device::Device* );
        Device::Device* readingDevice() const;
        
    
    public Q_SLOTS:
        void slotStartClicked() override;
        void slotCancelClicked() override;
        //void slotSaveClicked();
        /* receive fniished signal*/
        void slotFinished(bool);

    private Q_SLOTS:
        //void slotStartClicked() override;
        void updateOverrideDevice();

    protected:
        void toggleAll() override;
        void init() override;

    private:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        KIO::filesize_t neededSize() const;

        WriterSelectionWidget* m_writerSelectionWidget;
        TempDirSelectionWidget* m_tempDirSelectionWidget;
        QCheckBox* m_checkSimulate;
        //QCheckBox* m_checkCacheImage;
        QCheckBox* m_checkDeleteImages;
        //QCheckBox* m_checkOnlyCreateImage;
        QCheckBox* m_checkReadCdText;
        QCheckBox* m_checkIgnoreDataReadErrors;
        QCheckBox* m_checkIgnoreAudioReadErrors;
        QCheckBox* m_checkNoCorrection;
        QCheckBox* m_checkVerifyData;
        MediaSelectionComboBox* m_comboSourceDevice;
        QComboBox* m_comboParanoiaMode;
        QSpinBox* m_spinCopies;
        QSpinBox* m_spinDataRetries;
        QSpinBox* m_spinAudioRetries;
        WritingModeWidget* m_writingModeWidget;
        QComboBox* m_comboCopyMode;

        QGroupBox* m_groupAdvancedDataOptions;
        QGroupBox* m_groupAdvancedAudioOptions;

        /*  flag to choice whether burn */
        int flag;
    };
}

#endif
