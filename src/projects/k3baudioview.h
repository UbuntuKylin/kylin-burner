/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BAUDIOVIEW_H
#define K3BAUDIOVIEW_H

#include "k3bview.h"
#include "misc/k3bimagewritingdialog.h"

#include <QStringList>
#include <QComboBox>
#include <QLineEdit>

#include "kylinburnerlogger.h"

namespace K3b {

    class AudioDoc;
    class AudioTrack;
    class AudioViewImpl;
    class ViewColumnAdjuster;

    class AudioView : public View
    {
        Q_OBJECT

    public:
        AudioView( AudioDoc* doc, QWidget* parent );
        ~AudioView() override;

    public Q_SLOTS:
        void addUrls( const QList<QUrl>& urls ) override;
        
        //*******************************************
        void slotMediaChange( K3b::Device::Device* );
        void slotDeviceChange( K3b::Device::DeviceManager* );
        QLineEdit *ISO() {return lineedit_iso;}

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private Q_SLOTS:
        void slotPlayerStateChanged();
        void slotOpenfile();
        void slotSetting();
        void slotStartBurn();
        void slotBurn();
        void on_editingFinish();
        void on_textChanged(const QString text);

    private:
        AudioDoc* m_doc;
        AudioViewImpl* m_audioViewImpl;
        
        K3b::ImageWritingDialog *dlg;
        QLineEdit *lineedit_iso;
        QLabel* lineEdit_icon;
        QLabel* lineEdit_text;
        QComboBox *combo_CD;
        QPushButton *button_setting;
        QPushButton* button_start;
        QList<Device::Device*> device_index;
        QString filepath;
        KylinBurnerLogger *logger;
    };
}

#endif
