/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudioview.h"
#include "k3baudioviewimpl.h"

#include "k3bapplication.h"
#include "k3baudioburndialog.h"
#include "k3bdataburndialog.h"

#include "k3baudiodoc.h"
#include "k3baudioprojectmodel.h"
#include "k3bfillstatusdisplay.h"
#include "k3bpluginmanager.h"

#include "ThemeManager.h"

#include "config-kylinburner.h"
#ifdef ENABLE_AUDIO_PLAYER
#include "k3baudiotrackplayer.h"
#endif // ENABLE_AUDIO_PLAYER

#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>
#include <KActionCollection>

#include <QString>
#include <QDebug>
#include <QAction>
#include <QLayout>
#include <QScrollBar>
#include <QTreeView>
#include <fcntl.h>

#include "misc/k3bimagewritingdialog.h"
#include <QMdiArea>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include "misc/k3bimagewritingdialog.h"
#include "k3bapplication.h"
#include "k3bappdevicemanager.h"
#include "k3bmediacache.h"

void K3b::AudioView::on_textChanged(const QString text)
{
    qDebug() << "need to hide the show";
    lineEdit_icon->hide();
    lineEdit_text->hide();
}

void K3b::AudioView::on_editingFinish()
{
    int i = 0;
    QString str;
    qDebug() << "Input text: " << lineedit_iso->text();
    QFileInfo fileinfo(lineedit_iso->text());
    if (!fileinfo.exists() || !fileinfo.isFile() || (fileinfo.suffix() != "iso"))
    {
        lineedit_iso->clear();
        qDebug() << "Invalid file.";
        lineEdit_icon->show();
        lineEdit_text->show();
        return;
    }
    lineedit_iso->clear();
    long int file_size = fileinfo.size();
    double size;
    do{
        size = (float)file_size / (float)1024;
        file_size = file_size / 1024;
        i++;
    }while(file_size > 1024);
    switch(i){
    case 0:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "b";
        break;
    case 1:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "k";
        break;
    case 2:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "M";
        break;
    case 3:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "G";
        break;
    default:
        str = fileinfo.fileName() + " " + QString::number(file_size);
        break;
    }
    lineEdit_icon->show();
    lineEdit_text->show();

    lineEdit_icon->setStyleSheet("background-image:url(:/icon/icon/icon-image.png);\
                                 background-color:transparent;\
                                 background-repeat: no-repeat;});");
    if( !device_index.isEmpty() ){
        button_start->setEnabled( true );
    }

    lineEdit_text->setText( str );
}

K3b::AudioView::AudioView( K3b::AudioDoc* doc, QWidget* parent )
    : K3b::View( doc, parent )
{
    logger = LogRecorder::instance().registration(i18n("write image").toStdString().c_str());
    logger->debug("Draw write image begin...");
    m_doc = doc;
    setAttribute(Qt::WA_TranslucentBackground, true);
    
    QLabel *widget_label = new QLabel(this);
    QVBoxLayout *layout = new QVBoxLayout(widget_label);
    layout->setContentsMargins(10,0,0,0);    
    
    QLabel *label_title = new QLabel(this);
    label_title->setText(i18n("write image"));
    QFont title_font;
    title_font.setPixelSize(24);
    label_title->setFont( title_font );

    label_title->setObjectName("WriteImage");

    QLabel *label_iso = new QLabel(this);
    label_iso->setText(i18n("select iso"));
    QFont label_font;
    label_font.setPixelSize(14);
    label_iso->setFont( label_font );

    label_iso->setObjectName("LabelISO");


    lineedit_iso = new QLineEdit(this);
    lineedit_iso->setFixedSize(360, 30);
    connect(lineedit_iso, SIGNAL(editingFinished()), this, SLOT(on_editingFinish()));//on_textChanged
    connect(lineedit_iso, SIGNAL(textChanged(QString)), this, SLOT(on_textChanged(QString)));//on_textChanged

    lineedit_iso->setObjectName("LineEditISO");


    lineEdit_icon = new QLabel( lineedit_iso );
    lineEdit_icon->setFixedSize( 15,15);
    lineEdit_text = new QLabel( lineedit_iso );
    lineEdit_text->setObjectName("LineEditText");

    QHBoxLayout* hlayout = new QHBoxLayout( lineedit_iso );
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->addSpacing(10);
    hlayout->addWidget( lineEdit_icon );
    hlayout->addSpacing(5);
    hlayout->addWidget( lineEdit_text );
    hlayout->addStretch(0);
 
    QPushButton *button_openfile = new QPushButton(this);
    button_openfile->setText(i18n("browse"));
    button_openfile->setFixedSize(80, 30);
    button_openfile->setFont(label_font);

    
    QLabel *label_CD = new QLabel(this);
    label_CD->setText(i18n("select disk"));
    label_CD->setFont( label_font );

    label_CD->setObjectName("SelectDisk");
    
    combo_CD = new QComboBox(this);
    combo_CD->setFixedSize(360, 30);
    combo_CD->addItem( i18n("please insert CD") );
    combo_CD->setEnabled( false );
    combo_CD->setFont(label_font);
    combo_CD->setObjectName("comboEquipment1");

    combo_CD->setEnabled(false);

    button_setting = new QPushButton(this);
    button_setting->setText(i18n("setting"));
    button_setting->setFixedSize(80, 30);
    button_setting->setFont(label_font);

    button_setting->setEnabled(false);

    button_start = new QPushButton(this);
    button_start->setText(i18n("start"));
    button_start->setFixedSize(140, 45);
    button_start->setObjectName("WriteImageStart");
    label_font.setPixelSize(18);
    button_start->setFont(label_font);

    button_start->setEnabled( false );

#if 1
    QLabel* CD_label = new QLabel( widget_label); 
    CD_label->setFixedHeight(30);
    QHBoxLayout *CD_layout = new QHBoxLayout( CD_label );
    CD_layout->setContentsMargins(0,0,0,0);
    CD_layout->addSpacing( 0 );
    CD_layout->addWidget(combo_CD);
    CD_layout->addSpacing( 10 );
    CD_layout->addWidget( button_setting );
    CD_layout->addStretch( 0 );
     
    QLabel* iso_label = new QLabel( widget_label); 
    iso_label->setFixedHeight(30);
    QHBoxLayout *iso_layout = new QHBoxLayout( iso_label );
    iso_layout->setContentsMargins(0,0,0,0);
    iso_layout->addSpacing( 0 );
    iso_layout->addWidget( lineedit_iso );
    iso_layout->addSpacing( 10 );
    iso_layout->addWidget(button_openfile);
    iso_layout->addStretch( 0 );

    QLabel* start_label = new QLabel( widget_label); 
    start_label->setFixedHeight(45);
    QHBoxLayout *start_layout = new QHBoxLayout( start_label );
    start_layout->setContentsMargins(0,0,25,0);
    start_layout->addStretch( 0 );
    start_layout->addWidget( button_start);
    start_layout->addSpacing( 0 );
    
    layout->addWidget( label_title );
    layout->addSpacing( 35 );
    layout->addWidget( label_iso );
    layout->addSpacing( 10 );
    layout->addWidget( iso_label );
    layout->addSpacing( 70 );
    layout->addWidget( label_CD );
    layout->addSpacing( 10 );
    layout->addWidget( CD_label );
    layout->addStretch( 0 );
    layout->addWidget( start_label );
    layout->addSpacing( 25 );
#endif
    setMainWidget( widget_label );
    
    connect( button_openfile, SIGNAL(clicked()), this, SLOT(slotOpenfile()) );
    connect( button_setting, SIGNAL(clicked()), this, SLOT(slotSetting()) );
    connect( button_start, SIGNAL(clicked()), this, SLOT(slotStartBurn()) );
#if 1
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
              this, SLOT(slotMediaChange(K3b::Device::Device*)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
              this, SLOT(slotDeviceChange(K3b::Device::DeviceManager*)) );
#endif

    logger->debug("Draw write image end");
}

K3b::AudioView::~AudioView()
{
}

void K3b::AudioView::slotDeviceChange( K3b::Device::DeviceManager* manager)
{
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        combo_CD->setCurrentText(i18n("please insert CD"));
    }else{
        slotMediaChange( 0 );
    }
}

void K3b::AudioView::slotMediaChange( K3b::Device::Device* dev)
{
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    combo_CD->clear();
    device_index.clear();
    qDebug()<< "device count" << device_list.count() <<endl;
    
    foreach(K3b::Device::Device* device, device_list){
    
        combo_CD->setEnabled( true );

        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        //KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );

        qDebug()<< "device disk state" << device->diskInfo().diskState() <<endl;

        if ( device->diskInfo().diskState() != K3b::Device::STATE_EMPTY ){
            qDebug()<< "empty medium" << device <<endl;
            
            combo_CD->addItem(QIcon(":/icon/icon/icon-disk.png"), i18n("medium is not empty") );
            button_setting->setEnabled(false);
            continue;
        }
        if( !(device->diskInfo().mediaType() & K3b::Device::MEDIA_WRITABLE) ){
            qDebug()<< "media cannot write" << device->diskInfo().mediaType() <<endl;

            combo_CD->addItem(QIcon(":/icon/icon/icon-disk.png"), i18n("medium is unavailable") );
            button_setting->setEnabled(false);
            continue;
        }
        qDebug()<< "mount point" << device <<endl;
        device_index.append( device );
        button_setting->setEnabled(true);
        combo_CD->addItem( QIcon(":/icon/icon/icon-disk.png"), i18n("empty medium available space ") + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );
        if( !lineEdit_text->text().isEmpty() ){
            button_start->setEnabled( true );
        }

    }
}

void K3b::AudioView::slotOpenfile()
{
    int i = 0;
    QString str;
    filepath = QFileDialog::getOpenFileName(this, i18n("Browse"), "/home","iso file(*.iso *.udf)", 0/*, QFileDialog::DontUseNativeDialog*/);

    if(filepath == NULL)
        return;

    lineedit_iso->clear();

    QFileInfo fileinfo( filepath );
    long int file_size = fileinfo.size();
    double size;
    do{
        size = (float)file_size / (float)1024;
        file_size = file_size / 1024;
        i++;
    }while(file_size > 1024);
    switch(i){
    case 0:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "b";
        break;
    case 1:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "k";
        break;
    case 2:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "M";
        break;
    case 3:
        str = fileinfo.fileName() + " " + QString::number(size, 'f', 2) + "G";
        break;
    default:
        str = fileinfo.fileName() + " " + QString::number(file_size);
        break;
    }
     

    lineEdit_icon->setStyleSheet("background-image:url(:/icon/icon/icon-image.png);\
                                 background-color:transparent;\
                                 background-repeat: no-repeat;});");
    if( !device_index.isEmpty() ){
        button_start->setEnabled( true );
    }
    
    lineEdit_text->setText( str );
}

void K3b::AudioView::slotSetting()
{
    if( lineEdit_text->text().isEmpty() ) { 
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn") );
    }else{   
        dlg = new K3b::ImageWritingDialog( this );
        dlg->exec();
    }
}

void K3b::AudioView::slotStartBurn()
{
    dlg = new K3b::ImageWritingDialog( this );
    if( lineEdit_text->text().isEmpty() ) { 
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn") );
    }else if( device_index.isEmpty() ){
        KMessageBox::information( this, i18n("Please insert medium first."),
                                  i18n("No CD to Burn") );
    }else{   
        int index = combo_CD->currentIndex();
        dlg->loadConfig();
        dlg->setComboMedium( device_index.at( index ) );
        qDebug()<< "block name:" << device_index.at( index )->blockDeviceName() <<endl;

        dlg->setImage( QUrl::fromLocalFile( filepath ) );
        dlg->saveConfig();
        dlg->slotStartClicked();
    }
}

void K3b::AudioView::slotBurn()
{
    if( lineedit_iso->text().isEmpty() ) { 
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn") );
    }   
    else {
        QFileInfo f(filepath);
        if (f.size() >
                device_index.at(combo_CD->currentIndex())->diskInfo().remainingSize().mode1Bytes())
        {
            KMessageBox::information( this, i18n("Please change your medium, it's space too small."),
                                      i18n("No Enough Space") );
            return;
        }

        ProjectBurnDialog* dlg = newBurnDialog( this );
        dlg->execBurnDialog(true);
        delete dlg;
    }   
}

void K3b::AudioView::addUrls( const QList<QUrl>& urls )
{
    m_audioViewImpl->addUrls( urls );
}


K3b::ProjectBurnDialog* K3b::AudioView::newBurnDialog( QWidget* parent )
{
    return new AudioBurnDialog( m_doc, parent );
}


void K3b::AudioView::slotPlayerStateChanged()
{
#ifdef ENABLE_AUDIO_PLAYER
    if( m_audioViewImpl->player()->state() == AudioTrackPlayer::Playing ) {
        actionCollection()->action( "player_play" )->setVisible( false );
        actionCollection()->action( "player_pause" )->setVisible( true );
    }
    else {
        actionCollection()->action( "player_play" )->setVisible( true );
        actionCollection()->action( "player_pause" )->setVisible( false );
    }
#endif // ENABLE_AUDIO_PLAYER
}


