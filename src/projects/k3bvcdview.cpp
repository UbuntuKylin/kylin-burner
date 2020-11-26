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

#include "k3bvcdview.h"
#include "k3bvcdprojectmodel.h"
#include "k3bvcddoc.h"
#include "k3bvcdburndialog.h"
#include "k3bvcdtrackdialog.h"
#include "k3bfillstatusdisplay.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3baction.h"
#include "ThemeManager.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QItemSelectionModel>
#include <QString>
#include <QAction>
#include <QHeaderView>
#include <QLayout>
#include <QTreeView>

#include <QLabel>
#include <QFileDialog>
#include "misc/k3bmediacopydialog.h"
#include "k3bapplication.h"
#include "k3bappdevicemanager.h"
#include "k3bmediacache.h"
#include <KMountPoint>

K3b::VcdView::VcdView( K3b::VcdDoc* doc, QWidget* parent )
:
    K3b::View( doc, parent ),
    m_doc( doc ),
    m_model( new K3b::VcdProjectModel( m_doc, this ) ),
    m_view( new QTreeView( ) )
{
    flag = 0;
    comboIndex = 0;
    
    QLabel *widget_label = new QLabel(this);

    QVBoxLayout *vlayout = new QVBoxLayout(widget_label);
    vlayout->setContentsMargins(10,0,0,0);
 
    QLabel* label_title = new QLabel(this);
    label_title->setText(i18n("copy image"));
    QFont title_font;
    title_font.setPixelSize(24);
    label_title->setFont( title_font );
    label_title->setStyleSheet("color: #444444");
    label_title->setObjectName("LabelTitle");
#if 0
    ThManager()->regTheme(label_title, "ukui-white", "#LabelTitle{font: 24px; color: #444444;}");
    ThManager()->regTheme(label_title, "ukui-black", "#LabelTitle{font: 24px; color: #FFFFFF;}");
#endif
    setAttribute(Qt::WA_TranslucentBackground, true);

    QLabel *label_iso = new QLabel(this);
    label_iso->setText(i18n("CD to copy"));
    QFont label_font;
    label_font.setPixelSize(14);
    label_iso->setFont( label_font );
    label_iso->setStyleSheet("color: #444444");
    label_iso->setObjectName("LabelISO_1");
    ThManager()->regTheme(label_iso, "ukui-white", "#LabelISO_1{font: 14px; color: #444444;}");
    ThManager()->regTheme(label_iso, "ukui-black", "#LabelISO_1{font: 14px; color: #FFFFFF;}");

    combo_iso = new QComboBox(this);
    combo_iso->setFixedSize(360, 30);
    combo_iso->setObjectName("ComboISO_1");
#if 0
    ThManager()->regTheme(combo_iso, "ukui-white","#ComboISO_1{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboISO_1::hover{border:1px solid #6B8EEB;}"
                                                 "#ComboISO_1::selected{border:1px solid #6B8EEB;}"
                                                 "#ComboISO_1 QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#ComboISO_1 QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#ComboISO_1 QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboISO_1 QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#ComboISO_1::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#ComboISO_1::down-arrow{image: url(:/icon/icon/icon_xl.png); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#ComboISO_1 QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#ComboISO_1 QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#ComboISO_1 QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#ComboISO_1 QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#ComboISO_1{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#ComboISO_1::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    ThManager()->regTheme(combo_iso, "ukui-black","#comboISO{border:1px solid #DCDDDE;"
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
                                                 "background-color: rgba(0, 0, 0, 0.15);"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO QAbstractItemView::item::hover{"
                                                 "background-color: #3D6BE5;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#comboISO::down-arrow{image: url(:/icon/icon/icon_xl.png); "
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
 #endif
    label_CD = new QLabel(this);
    label_CD->setText(i18n("Copy CD"));
    label_CD->setFont( label_font );
    label_CD->setStyleSheet("color: #444444");
    label_CD->setObjectName("LabelCD_1");
    ThManager()->regTheme(label_CD, "ukui-white", "#LabelCD_1{font: 14px; color: #444444;}");
    ThManager()->regTheme(label_CD, "ukui-black", "#LabelCD_1{font: 14px; color: #FFFFFF;}");

    combo_CD = new QComboBox(this);
    combo_CD->setFixedSize(360, 30);
    combo_CD->setObjectName("ComboCD_2");
#if 0
    ThManager()->regTheme(combo_CD, "ukui-white","#ComboCD_2{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboCD_2::hover{border:1px solid #6B8EEB;}"
                                                 "#ComboCD_2::selected{border:1px solid #6B8EEB;}"
                                                 "#ComboCD_2 QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#ComboCD_2 QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#ComboCD_2 QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboCD_2 QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#ComboCD_2::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#ComboCD_2::down-arrow{image: url(:/icon/icon/icon_xl.png); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#ComboCD_2 QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#ComboCD_2 QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#ComboCD_2 QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#ComboCD_2 QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#ComboCD_2{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#ComboCD_2::drop-down{subcontrol-origin: padding;"
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
                                                 "background-color: rgba(0, 0, 0, 0.15);"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO QAbstractItemView::item::hover{"
                                                 "background-color: #3D6BE5;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#comboISO::down-arrow{image: url(:/icon/icon/icon_xl.png); "
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
#endif
    image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    lastIndex = comboIndex;
    combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);
    isBurner = false;
#if 0
    button_setting = new QPushButton();
    button_setting->setText(i18n("setting"));
    button_setting->setFixedSize(80, 30);
    button_setting->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                  "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;}"
                                  "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;}");
#endif
    button_openfile = new QPushButton(this);
    if (isBurner) button_openfile->setText(i18n("setting"));
    else button_openfile->setText(i18n("choice"));
    button_openfile->setFixedSize(80, 30);
    button_openfile->setObjectName("Choice");
#if 0
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
                          "background-color: rgba(72, 72, 76, 1);"
                          "border: none; border-radius: 4px;"
                          "font: 14px \"MicrosoftYaHei\";"
                          "color: rgba(255, 255, 255, 1);",
                          "background-color:rgba(55, 55, 55, 1);"
                          "border: none; border-radius: 4px;"
                          "font: 14px \"MicrosoftYaHei\";"
                          "color: rgba(255, 255, 255, 1);",
                          "background-color: rgba(57, 58, 62, 1);"
                          "border: none; border-radius: 4px;"
                          "font: 14px \"MicrosoftYaHei\";"
                          "color: rgba(77, 78, 81, 1);");
#endif
    button_start = new QPushButton(this);
    if (isBurner) button_start->setText(i18n("start burner"));
    else button_start->setText(i18n("Create image"));
    button_start->setFixedSize(140, 45);
    button_start->setEnabled( false );
    button_start->setObjectName("CopyStart");
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
                          "background-color: rgba(57, 58, 62, 1);"
                          "font: 18px \"MicrosoftYaHei\";"
                          "color: rgba(77, 78, 81, 1);");
    /*
    button_start->setStyleSheet("QPushButton{"
                                "background-color:rgba(61, 107, 229, 1);"
                                "font: 18px;border-radius: 4px;color:#ffffff}"
                                "QPushButton:hover{background-color:rgba(61, 107, 229, 1);}");
   */
     
    CD_index = 0;
    combo_CD->setEnabled( true );
    button_openfile->setEnabled( true );
#if 1
    QLabel* CD_label = new QLabel( widget_label); 
    CD_label->setFixedHeight(30);
    QHBoxLayout *CD_layout = new QHBoxLayout( CD_label );
    CD_layout->setContentsMargins(0,0,0,0);
    CD_layout->addSpacing( 0 );
    CD_layout->addWidget(combo_CD);
    CD_layout->addSpacing( 10 );
    CD_layout->addWidget(button_openfile);
    CD_layout->addStretch( 0 );
    
    QLabel* start_label = new QLabel( widget_label); 
    start_label->setFixedHeight(45);
    QHBoxLayout *start_layout = new QHBoxLayout( start_label );
    start_layout->setContentsMargins(0,0,25,0);
    start_layout->addStretch( 0 );
    start_layout->addWidget( button_start);
    start_layout->addSpacing( 0 );
    
    vlayout->addWidget( label_title );
    vlayout->addSpacing( 35 );
    vlayout->addWidget( label_iso );
    vlayout->addSpacing( 10 );
    vlayout->addWidget( combo_iso );
    vlayout->addSpacing( 70 );
    vlayout->addWidget( label_CD );
    vlayout->addSpacing( 10 );
    vlayout->addWidget( CD_label );
    vlayout->addStretch( 0 );
    vlayout->addWidget( start_label );
    vlayout->addSpacing( 25 );
#endif
    setMainWidget( widget_label );
   
    connect( combo_CD, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboCD(int) ) );
    connect( combo_iso, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboISO(int) ) );
    connect( button_openfile, SIGNAL(clicked()), this, SLOT(slotOpenfile()) );
    connect( button_start, SIGNAL(clicked()), this, SLOT(slotStartBurn()) );

#if 1
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
              this, SLOT(slotMediaChange(K3b::Device::Device*)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
              this, SLOT(slotDeviceChange(K3b::Device::DeviceManager*)) );
#endif

/*
    m_view->setModel( m_model );
    m_view->setAcceptDrops( true );
    m_view->setDragEnabled( true );
    m_view->setDragDropMode( QTreeView::DragDrop );
    m_view->setItemsExpandable( false );
    m_view->setRootIsDecorated( false );
    m_view->setSelectionMode( QTreeView::ExtendedSelection );
    m_view->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_view->setContextMenuPolicy( Qt::ActionsContextMenu );
    // FIXME: make QHeaderView::Interactive the default but connect to model changes and call header()->resizeSections( QHeaderView::ResizeToContents );
    m_view->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
    m_view->setEditTriggers( QAbstractItemView::NoEditTriggers );
    setMainWidget( m_view );

    // setup actions
    m_actionProperties = K3b::createAction( this, i18n("Properties"), "document-properties",
                                            0, this, SLOT(slotProperties()),
                                            actionCollection(), "vcd_show_props" );

    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete",
                                        Qt::Key_Delete, this, SLOT(slotRemove()),
                                        actionCollection(), "vcd_remove_track" );

    connect( m_view, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotItemActivated(QModelIndex)) );
    connect( m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotSelectionChanged()) );

    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_view->addAction( m_actionRemove );
    m_view->addAction( separator );
    m_view->addAction( m_actionProperties );
    m_view->addAction( separator );
    m_view->addAction( actionCollection()->action("project_burn") );*/
}


K3b::VcdView::~VcdView()
{
}

void K3b::VcdView::slotDeviceChange( K3b::Device::DeviceManager* manager)
{
    /*
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        combo_iso->setEnabled( false );
        combo_iso->setCurrentText( "please insert a device" );
        combo_CD->setEnabled( false );
        //lineedit_CD->setEnabled( false );
    }else
        slotMediaChange( 0 );
    */

}

void K3b::VcdView::slotMediaChange( K3b::Device::Device* dev)
{
    int idx = -1, cdIdx = 0;;
    idx = cdDevices.indexOf(dev);
    K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );
    KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() );

    qDebug() << "----------Image Copying...----------------";

    if (-1 == idx)
    {
        cdDevices << dev;
        if (!mountPoint)
        {
            idx = sourceDevices.indexOf(dev);
            if (-1 != idx)
            {
                combo_iso->removeItem(idx);
                sourceDevices.removeAt(idx);
            }
            if ( dev->diskInfo().diskState() == K3b::Device::STATE_EMPTY )
                combo_CD->insertItem(comboIndex++,
                                     QIcon(":/icon/icon/icon-光盘.png"),
                                     i18n("empty medium " ));
            else
                combo_CD->insertItem(comboIndex++,
                                     QIcon(":/icon/icon/icon-光盘.png"),
                                     i18n("please insert a available medium" ));
        }
        else
        {
            idx = sourceDevices.indexOf(dev);
            if (-1 == idx)
            {
                sourceDevices << dev;
                combo_iso->addItem( QIcon(":/icon/icon/icon-光盘.png"),
                                    medium.shortString() + \
                                    KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ) );
            }
            else
            {
                combo_iso->setItemText(idx, medium.shortString() + \
                                       KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ));
            }
            combo_CD->insertItem(comboIndex++,
                                 QIcon(":/icon/icon/icon-光盘.png"),
                                 medium.shortString() + \
                                 KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ));
        }
    }
    else
    {
        cdIdx = idx + 1;
        if (!mountPoint)
        {
            idx = sourceDevices.indexOf(dev);
            if (-1 != idx)
            {
                combo_iso->removeItem(idx);
                sourceDevices.removeAt(idx);
            }
            if ( dev->diskInfo().diskState() == K3b::Device::STATE_EMPTY )
                combo_CD->setItemText(cdIdx, i18n("empty medium " ));
            else
                combo_CD->setItemText(cdIdx, i18n("please insert a available medium" ));
        }
        else
        {
            qDebug() << medium.shortString();
            idx = sourceDevices.indexOf(dev);
            if (-1 == idx)
            {
                sourceDevices << dev;
                combo_iso->addItem( QIcon(":/icon/icon/icon-光盘.png"),
                                    medium.shortString() + \
                                    KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ) );
            }
            else
            {
                if ("无介质" == medium.shortString())
                {
                    combo_iso->removeItem(idx);
                    sourceDevices.removeAt(idx);
                }
                else
                {
                    combo_iso->setItemText(idx, medium.shortString() + \
                                       KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ));
                }
            }
            combo_CD->setItemText(cdIdx, medium.shortString() + \
                                  KIO::convertSize( dev->diskInfo().remainingSize().mode1Bytes() ));
        }
    }
    lastSourceIndex = combo_iso->currentIndex();
    lastIndex = combo_CD->currentIndex();

    if (i18n("empty medium " ) == combo_CD->currentText() ||
            i18n("please insert a available medium" ) == combo_CD->currentText())
    {
        disableBurnerStart();
    }
    else
    {
        enableBurnerStart();
    }
    if (0 == combo_iso->count()) disableBurnerStart();
    else if (0 == lastIndex) enableBurnerStart();
    else disableBurnerStart();
    /*
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    combo_iso->clear();
    //combo_CD->clear();
    device_index.clear();
    CD_index = 0;
    device_count = 0;

    foreach(K3b::Device::Device* device, device_list){
        combo_iso->setEnabled( true );
        combo_CD->setEnabled( true );

        //device_index.append( device );

        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );

        if ( device->diskInfo().diskState() == K3b::Device::STATE_EMPTY ){
            //combo_CD->addItem( QIcon(":/icon/icon/icon-光盘.png"), i18n("empty medium " ));
            combo_CD->insertItem(comboIndex++,  QIcon(":/icon/icon/icon-光盘.png"), i18n("empty medium " ));
            CD_index++;
            continue;
        }
        if ( !( device->diskInfo().diskState() & (K3b::Device::STATE_COMPLETE | K3b::Device::STATE_INCOMPLETE ) ) ){
            qDebug()<< "empty medium" << device <<endl;

            combo_iso->addItem( i18n("please insert a available medium") );
            combo_CD->addItem( i18n("please insert a available medium") );
            CD_index++;
            continue;
        }
        if( !(device->diskInfo().mediaType() & K3b::Device::MEDIA_ALL) ){
            qDebug()<< "media type cannot use" << device->diskInfo().mediaType() <<endl;

            combo_iso->addItem( i18n("please insert a available medium") );
            combo_CD->addItem( i18n("please insert a available medium") );
            CD_index++;
            continue;
        }
        //qDebug()<< "mount point" << device <<endl;
        CD_index++;
        device_count = CD_index;
        device_index.append( device );
        combo_iso->addItem( QIcon(":/icon/icon/icon-光盘.png"), medium.shortString() + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );
        combo_CD->addItem( QIcon(":/icon/icon/icon-光盘.png"), medium.shortString() + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );

    }
    */
}

void K3b::VcdView::slotComboISO(int idx)
{
    if (0 == combo_iso->count()) disableBurnerStart();
    if (-1 == idx) return;

    if (combo_CD->currentIndex() &&
            sourceDevices[idx] == cdDevices[combo_CD->currentIndex() - 1])
        combo_CD->setCurrentIndex(0);
}

void K3b::VcdView::slotComboCD(int ret)
{
    qDebug() << "Last index is : " << lastIndex << "| Current index is : " << ret;
    if (ret)
    {
        if (sourceDevices.size() && cdDevices[ret - 1] == sourceDevices[combo_iso->currentIndex()])
        {
            qDebug() << "Select the same CD";
            combo_CD->setCurrentIndex(lastIndex);
            return;
        }
        isBurner = true;
    }
    else
    {
        isBurner = false;
        button_start->setEnabled(true);
    }
    lastIndex = ret;

    if (isBurner)
    {
        button_openfile->setText(i18n("setting"));
        button_start->setText(i18n("start burner"));
        if (i18n("empty medium " ) == combo_CD->currentText() ||
                i18n("please insert a available medium" ) == combo_CD->currentText())
        {
            button_start->setEnabled(false);
        }
        else
        {
            button_start->setEnabled(true);
        }
    }
    else
    {
        button_openfile->setText(i18n("choice"));
        button_start->setText(i18n("Create image"));
    }
    /*
    qDebug() << "index: " << ret << endl;
    button_start->setEnabled( true );
    button_start->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                "QPushButton:hover{background-color:rgb(107, 142, 235);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 18px;border-radius: 4px;color: rgb(255,255,255);}");
    if( ret >= device_count ){
        flag = 1;
    }else{
        flag = 0;
    }
    */
}

void K3b::VcdView::slotOpenfile()
{
    /*
    if( device_index.isEmpty() )
        return;
    filepath = QFileDialog::getExistingDirectory(this, "open file dialog", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if( filepath.isEmpty() )
        return;
   flag = 1;
   //combo_CD->setEditable( true );
   CD_index++;
   combo_CD->addItem( filepath );
   combo_CD->setCurrentText( filepath );
   */
   if (isBurner)
   {
       qDebug() << "do setting";
       ProjectBurnDialog* dlg = newBurnDialog( this );
       dlg->execBurnDialog(true);
       delete dlg;
   }
   else
   {
       filepath = QFileDialog::getExistingDirectory(this, i18n("choice"), image_path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
       if (filepath.isEmpty()) return;
       image_path = filepath + "/kylin_burner.iso";
       combo_CD->setItemText(0, image_path);
   }

}

void K3b::VcdView::MediaCopy( K3b::Device::Device* dev )
{
    K3b::MediaCopyDialog *d = new K3b::MediaCopyDialog( this );
    d->setReadingDevice( device_index.at( combo_iso->currentIndex() ) );
    d->setComboMedium( device_index.at( combo_CD->currentIndex() ) );
    qDebug()<< "from" << device_index.at( combo_iso->currentIndex() )->blockDeviceName() << "to" << device_index.at( combo_CD->currentIndex() )->blockDeviceName() <<endl;
    d->saveConfig();
    d->exec();
}

void K3b::VcdView::slotSetting()
{
    MediaCopy(0);
}

void K3b::VcdView::slotStartBurn()
{
#if 0
    qDebug() << "flag: " << flag << endl;
    return;
#endif
    K3b::Device::Device* dev = sourceDevices[combo_iso->currentIndex()];
    K3b::MediaCopyDialog *dlg = new K3b::MediaCopyDialog( this );
    dlg->setReadingDevice(dev);
    if (isBurner)
    {
        qDebug() << "Burning...";
        dlg->loadConfig();
        dlg->setOnlyCreateImage(false);
        dlg->setComboMedium( cdDevices[combo_CD->currentIndex() - 1] );
        dlg->saveConfig();
    }
    else
    {
        qDebug() << "Copying...";
        dlg->setOnlyCreateImage(true);
        dlg->setTempDirPath( image_path );
        dlg->saveConfig();
    }
    button_start->setEnabled(false);
    dlg->slotStartClicked();
    button_start->setEnabled(true);
    if (combo_CD->currentIndex())
    {
        if (sourceDevices[combo_iso->currentIndex()] ==
            cdDevices[combo_CD->currentIndex() - 1]) combo_CD->setCurrentIndex(0);
    }
    dev->eject();
    /*
    int iso_index = combo_iso->currentIndex();
    int CD_index = combo_CD->currentIndex();
    K3b::MediaCopyDialog *dlg = new K3b::MediaCopyDialog( this );
    if( device_index.isEmpty() ){
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn") );
    }else {
        dlg->setReadingDevice( device_index.at( iso_index ) );
        if ( flag ){
            dlg->loadConfig();
            dlg->setOnlyCreateImage(true);
            dlg->setTempDirPath( combo_CD->currentText() );
            dlg->saveConfig();
            dlg->slotStartClicked();
        }else if ( device_index.at( iso_index ) == device_index.at( CD_index )){
            dlg->setComboMedium( device_index.at( CD_index ) );
            dlg->saveConfig();
            dlg->slotStartClicked();
        }else{
            dlg->loadConfig();
            dlg->setOnlyCreateImage(true);
            dlg->setComboMedium( device_index.at( CD_index ) );
            dlg->saveConfig();
            qDebug()<< "from" << device_index.at( iso_index )->blockDeviceName() << "to" << device_index.at( CD_index )->blockDeviceName() <<endl;
            dlg->slotStartClicked();
        }
    }
    */
}

K3b::ProjectBurnDialog* K3b::VcdView::newBurnDialog( QWidget * parent)
{
    return new K3b::VcdBurnDialog( m_doc, parent );
}


void K3b::VcdView::init()
{
    if( !k3bcore->externalBinManager()->foundBin( "vcdxbuild" ) ) {
        qDebug() << "(K3b::VcdView) could not find vcdxbuild executable";
        KMessageBox::information( this,
                        i18n( "Could not find VcdImager executable. "
                        "To create Video CDs you have to install VcdImager >= 0.7.12. "
                        "You can find this on your distribution’s software repository or download "
                        "it from http://www.vcdimager.org" ) );
    }
}


void K3b::VcdView::slotSelectionChanged()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if( !selected.isEmpty() ) {
        m_actionRemove->setEnabled( true );
    }
    else {
        m_actionRemove->setEnabled( false );
    }
}


void K3b::VcdView::slotProperties()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();

    if( selection.isEmpty() ) {
        // show project properties
	    View::slotProperties();
    }
    else {
        QList<K3b::VcdTrack*> selected;

        Q_FOREACH( const QModelIndex& index, selection ) {
            selected.append( m_model->trackForIndex(index) );
        }

        QList<K3b::VcdTrack*> tracks = *m_doc->tracks();

        K3b::VcdTrackDialog dlg( m_doc, tracks, selected, this );
        dlg.exec();
    }
}


void K3b::VcdView::slotRemove()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();

    // create a list of persistent model indexes to be able to remove all of them
    QList<QPersistentModelIndex> indexes;
    Q_FOREACH( const QModelIndex& index, selected ) {
        indexes.append( QPersistentModelIndex( index ) );
    }

    // and now ask the indexes to be removed
    Q_FOREACH( const QPersistentModelIndex& index, indexes ) {
        m_model->removeRow( index.row(), index.parent() );
    }
}


void K3b::VcdView::slotItemActivated( const QModelIndex& index )
{
    if( VcdTrack* track = m_model->trackForIndex( index ) ) {
        QList<VcdTrack*> tracks = *m_doc->tracks();
        QList<VcdTrack*> selected;
        selected.append( track );
        K3b::VcdTrackDialog dlg( m_doc, tracks, selected, this );
        dlg.exec();
    }
}


