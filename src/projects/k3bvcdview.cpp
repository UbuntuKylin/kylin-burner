/*
 *
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
    
    QLabel *widget_label = new QLabel(this);

    QVBoxLayout *vlayout = new QVBoxLayout(widget_label);
    vlayout->setContentsMargins(10,0,0,0);
 
    QLabel* label_title = new QLabel(this);
    label_title->setText(i18n("copy image"));
    QFont title_font;
    title_font.setPixelSize(24);
    label_title->setFont( title_font );
    label_title->setStyleSheet("color: #444444");

    QLabel *label_iso = new QLabel(this);
    label_iso->setText(i18n("CD to copy"));
    QFont label_font;
    label_font.setPixelSize(14);
    label_iso->setFont( label_font );
    label_iso->setStyleSheet("color: #444444");

    combo_iso = new QComboBox(this);
    combo_iso->setFixedSize(360, 30);
    
    label_CD = new QLabel(this);
    label_CD->setText(i18n("Copy CD"));
    label_CD->setFont( label_font );
    label_CD->setStyleSheet("color: #444444");

    combo_CD = new QComboBox(this);
    combo_CD->setFixedSize(360, 30);
#if 0
    button_setting = new QPushButton();
    button_setting->setText(i18n("setting"));
    button_setting->setFixedSize(80, 30);
    button_setting->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                  "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;}"
                                  "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;}");
#endif
    button_openfile = new QPushButton(this);
    button_openfile->setText(i18n("choice"));
    button_openfile->setFixedSize(80, 30);
    button_openfile->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                   "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#ffffff}"
                                   "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color:#ffffff}");

    QPushButton *button_start = new QPushButton(this);
    button_start->setText(i18n("Create image"));
    button_start->setFixedSize(140, 45);
    button_start->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                "QPushButton:hover{background-color:rgb(107, 142, 235);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 18px;border-radius: 4px;color: rgb(255,255,255);}");
        
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
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        combo_iso->setEnabled( false );
        combo_iso->setCurrentText( "please insert a device" );
        combo_CD->setEnabled( false );
        //lineedit_CD->setEnabled( false );
    }else
        slotMediaChange( 0 );

}

void K3b::VcdView::slotMediaChange( K3b::Device::Device* dev)
{
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    combo_iso->clear();
    combo_CD->clear();
    device_index.clear();
    //qDebug()<< "device count" << device_list.count() <<endl;
    
    foreach(K3b::Device::Device* device, device_list){
        combo_iso->setEnabled( true );
        combo_CD->setEnabled( true );
        //lineedit_CD->setEnabled( true );

        //device_index.append( device );

        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );

        if ( device->diskInfo().diskState() == K3b::Device::STATE_EMPTY ){
            combo_CD->addItem( QIcon(":/icon/icon/icon-光盘.png"), i18n("empty medium " ));
            continue;
        }
        //qDebug()<< "device disk state" << device->diskInfo().diskState() <<endl;
        if ( !( device->diskInfo().diskState() & (K3b::Device::STATE_COMPLETE | K3b::Device::STATE_INCOMPLETE ) ) ){
            qDebug()<< "empty medium" << device <<endl;

            combo_iso->addItem( i18n("please insert a available medium") );
            combo_CD->addItem( i18n("please insert a available medium") );
            continue;
        }
        if( !(device->diskInfo().mediaType() & K3b::Device::MEDIA_ALL) ){
            qDebug()<< "media type cannot use" << device->diskInfo().mediaType() <<endl;

            combo_iso->addItem( i18n("please insert a available medium") );
            combo_CD->addItem( i18n("please insert a available medium") );
            continue;
        }
        //qDebug()<< "mount point" << device <<endl;
        device_index.append( device );
        combo_iso->addItem( QIcon(":/icon/icon/icon-光盘.png"), medium.shortString() + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );
        combo_CD->addItem( QIcon(":/icon/icon/icon-光盘.png"), medium.shortString() + KIO::convertSize( device->diskInfo().remainingSize().mode1Bytes() ) );

    }

}

void K3b::VcdView::slotOpenfile()
{
    filepath = QFileDialog::getExistingDirectory(this, "open file dialog", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks/* | QFileDialog::DontUseNativeDialog*/);

    if( filepath.isEmpty() )
        return;
   flag = 1;
   combo_CD->setEditable( true );
   combo_CD->setCurrentText( filepath );

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


