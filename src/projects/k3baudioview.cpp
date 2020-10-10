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

#include "config-k3b.h"
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
#include "misc/k3bimagewritingdialog.h"
#include "k3bapplication.h"
#include "k3bappdevicemanager.h"
#include "k3bmediacache.h"


K3b::AudioView::AudioView( K3b::AudioDoc* doc, QWidget* parent )
    : K3b::View( doc, parent )
{
    logger = LogRecorder::instance().registration(i18n("write image").toStdString().c_str());
    logger->debug("Draw write image begin...");
    m_doc = doc;
    
    QLabel *widget_label = new QLabel(this);
    QVBoxLayout *layout = new QVBoxLayout(widget_label);
    layout->setContentsMargins(10,0,0,0);    
    
    QLabel *label_title = new QLabel(this);
    label_title->setText(i18n("write image"));
    QFont title_font;
    title_font.setPixelSize(24);
    label_title->setFont( title_font );
    label_title->setStyleSheet("color:#444444;");

    label_title->setObjectName("WriteImage");
    ThManager()->regTheme(label_title, "ukui-white", "#WriteImage{font: 24px; color: #444444;}");
    ThManager()->regTheme(label_title, "ukui-black", "#WriteImage{font: 24px; color: #FFFFFF;}");

    QLabel *label_iso = new QLabel(this);
    label_iso->setText(i18n("select iso"));
    QFont label_font;
    label_font.setPixelSize(14);
    label_iso->setFont( label_font );
    label_iso->setStyleSheet("color:#444444;");

    label_iso->setObjectName("LabelISO");
    ThManager()->regTheme(label_iso, "ukui-white", "#LabelISO{font: 14px; color: #444444;}");
    ThManager()->regTheme(label_iso, "ukui-black", "#LabelISO{font: 14px; color: #FFFFFF;}");

    lineedit_iso = new QLineEdit(this);
    lineedit_iso->setFixedSize(360, 30);
    //lineedit_iso->setDragEnabled( false );
    lineedit_iso->setEnabled( false );
    //lineedit_iso->setStyleSheet("QLineEdit{background-color: transparent;border:1px solid rgba(220,221,222,1);border-radius:4px;}");

    lineedit_iso->setObjectName("LineEditISO");
    ThManager()->regTheme(lineedit_iso, "ukui-white", "#LineEditISO{background-color: transparent;"
                                                      "border:1px solid rgba(220,221,222,1);"
                                                      "border-radius:4px;}");
    ThManager()->regTheme(lineedit_iso, "ukui-black", "#LineEditISO{background-color: transparent;"
                                                      "border:1px solid rgba(255, 255, 255);"
                                                      "border-radius:4px;}");

    lineEdit_icon = new QLabel( lineedit_iso );
    lineEdit_icon->setFixedSize( 15,15);
    lineEdit_text = new QLabel( lineedit_iso );
    lineEdit_text->setObjectName("LineEditText");
    ThManager()->regTheme(lineEdit_text, "ukui-white", "#LineEditText{background-color: transparent;"
                                                      "font:14px; color: #444444;}",
                          QString(), QString(),"#LineEditText{background-color: transparent;"
                                               "font:14px; color: #444444;}");
    ThManager()->regTheme(lineEdit_text, "ukui-black", "#LineEditText{background-color: transparent;"
                                                      "font:14px; color: #FFFFFF;}",
                          QString(), QString(),"#LineEditText{background-color: transparent;"
                                               "font:14px; color: #FFFFFF;}");
    
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
    button_openfile->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                   "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#ffffff}"
                                   "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color:#ffffff}");
    button_openfile->setObjectName("WriteImageBrowse");
    ThManager()->regTheme(button_openfile, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_openfile, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");

    QLabel *label_space = new QLabel(this);
    
    QLabel *label_CD = new QLabel(this);
    label_CD->setText(i18n("select disk"));
    label_CD->setFont( label_font );
    label_CD->setStyleSheet("color:#444444;");

    label_CD->setObjectName("SelectDisk");
    ThManager()->regTheme(label_CD, "ukui-white", "#SelectDisk{font: 14px; color: #444444;}");
    ThManager()->regTheme(label_CD, "ukui-black", "#SelectDisk{font: 14px; color: #FFFFFF;}");
    
    combo_CD = new QComboBox(this);
    combo_CD->setFixedSize(360, 30);
    combo_CD->addItem( i18n("please insert CD") );
    combo_CD->setEnabled( false );
    combo_CD->setStyleSheet("QComboBox{background:rgba(255,255,255,1);  border:1px solid rgba(220,221,222,1);border-radius:4px;}"
                            "QComboBox::drop-down{subcontrol-origin: padding; subcontrol-position: top right; \
                             border-top-right-radius: 3px; \
                             border-bottom-right-radius: 3px;}"
                             "QComboBox::down-arrow{width: 8px; height: 16;"
                             "image: url(:/icon/icon/draw-down.jpg);"
                             "padding: 0px 0px 0px 0px;}");
    combo_CD->setObjectName("comboEquipment1");
    ThManager()->regTheme(combo_CD, "ukui-white","#comboEquipment1{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#comboEquipment1::hover{border:1px solid #6B8EEB;}"
                                                 "#comboEquipment1::selected{border:1px solid #6B8EEB;}"
                                                 "#comboEquipment1 QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#comboEquipment1 QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#comboEquipment1 QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#comboEquipment1 QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboEquipment1::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#comboEquipment1::down-arrow{image: url(:/icon/icon/draw-down.jpg); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#comboEquipment1 QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#comboEquipment1 QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#comboEquipment1 QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#comboEquipment1 QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#comboEquipment1{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#comboEquipment1::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    ThManager()->regTheme(combo_CD, "ukui-black","#comboISO{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;"
                                                 "background-color: #242424;}"
                                                 "#comboISO::hover{border:1px solid #6B8EEB;"
                                                 "background-color: rgba(0, 0, 0, 0.15);}"
                                                 "#comboISO::selected{border:1px solid #6B8EEB;"
                                                 "background-color: #242424}"
                                                 "#comboISO QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #242424;border:1px solid #DCDDDE;}"
                                                 "#comboISO QAbstractItemView::hover{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#comboISO QAbstractItemView::item{"
                                                 "background-color: rgba(0, 0, 0, 0.15);border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO QAbstractItemView::item::hover{"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#comboISO::down-arrow{image: url(:/lb/icon_xl.png); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#comboISO QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#comboISO QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#comboISO QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#comboISO QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#comboISO{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    combo_CD->setEnabled(false);

    button_setting = new QPushButton(this);
    button_setting->setText(i18n("setting"));
    button_setting->setFixedSize(80, 30);
    button_setting->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                  "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#ffffff}"
                                  "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color:#ffffff}");
    button_setting->setObjectName("WriteImageSetting");
    ThManager()->regTheme(button_setting, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_setting, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
    button_setting->setEnabled(false);

    button_start = new QPushButton(this);
    button_start->setText(i18n("start"));
    button_start->setFixedSize(140, 45);
    button_start->setObjectName("WriteImageStart");
    ThManager()->regTheme(button_start, "ukui-white",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #6B8DEB;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #3D6BE5;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #415FC3;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #E9E9E9;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_start, "ukui-black",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #6B8DEB;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #3D6BE5;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #415FC3;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #E9E9E9;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: rgba(193, 193, 193, 1);");
    button_start->setEnabled( false );
    //button_start->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 18px;border-radius: 4px;}");

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
//m_audioViewImpl = new AudioViewImpl( this, m_doc, actionCollection() );

    //setMainWidget( m_audioViewImpl->view() );
/*
    fillStatusDisplay()->showTime();

    toolBox()->addAction( actionCollection()->action( "project_audio_convert" ) );
    toolBox()->addAction( actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();

    toolBox()->addActions( createPluginsActions( m_doc->type() ) );
    toolBox()->addSeparator();

#ifdef ENABLE_AUDIO_PLAYER
    toolBox()->addAction( actionCollection()->action( "player_previous" ) );
    toolBox()->addAction( actionCollection()->action( "player_play" ) );
    toolBox()->addAction( actionCollection()->action( "player_pause" ) );
    toolBox()->addAction( actionCollection()->action( "player_stop" ) );
    toolBox()->addAction( actionCollection()->action( "player_next" ) );
    toolBox()->addAction( actionCollection()->action( "player_seek" ) );
    toolBox()->addSeparator();

    connect( m_audioViewImpl->player(), SIGNAL(stateChanged()),
             this, SLOT(slotPlayerStateChanged()) );
#endif // ENABLE_AUDIO_PLAYER
*/
    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since View already added the default actions
    setXML( "<!DOCTYPE gui SYSTEM \"kpartgui.dtd\">"
            "<gui name=\"k3bproject\" version=\"1\">"
            "<MenuBar>"
            " <Menu name=\"project\"><text>&amp;Project</text>"
            "  <Action name=\"project_audio_convert\"/>"
#ifdef ENABLE_MUSICBRAINZ
            "  <Action name=\"project_audio_musicbrainz\"/>"
#endif
            " </Menu>"
            "</MenuBar>"
            "</gui>", true );
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
            
            combo_CD->addItem(QIcon(":/icon/icon/icon-光盘.png"), i18n("medium is not empty") );
            button_setting->setEnabled(false);
            continue;
        }
        if( !(device->diskInfo().mediaType() & K3b::Device::MEDIA_WRITABLE) ){
            qDebug()<< "media cannot write" << device->diskInfo().mediaType() <<endl;

            combo_CD->addItem(QIcon(":/icon/icon/icon-光盘.png"), i18n("medium is unavailable") );
            button_setting->setEnabled(false);
            continue;
        }
        qDebug()<< "mount point" << device <<endl;
        device_index.append( device );
        button_setting->setEnabled(true);
        combo_CD->addItem( QIcon(":/icon/icon/icon-光盘.png"), i18n("empty medium available space ") + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );
        if( !lineEdit_text->text().isEmpty() ){
            button_start->setEnabled( true );
            button_start->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                        "QPushButton:hover{background-color:rgb(107, 142, 235);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                        "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 18px;border-radius: 4px;color: rgb(255,255,255);}");
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
     

    lineEdit_icon->setStyleSheet("background-image:url(:/icon/icon/icon-镜像.png);\
                                 background-color:transparent;\
                                 background-repeat: no-repeat;});");
    if( !device_index.isEmpty() ){
        button_start->setEnabled( true );
        button_start->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                    "QPushButton:hover{background-color:rgb(107, 142, 235);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                    "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 18px;border-radius: 4px;color: rgb(255,255,255);}");
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


