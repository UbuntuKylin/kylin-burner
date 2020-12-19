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

#include "k3b.h"
#include "k3bdataburndialog.h"
#include "k3bdataimagesettingswidget.h"
#include "k3bdatamultisessioncombobox.h"
#include "k3bdataview.h"
#include "../k3bapplication.h"

#include "k3bisooptions.h"
#include "k3bdatadoc.h"
#include "k3bdevice.h"
#include "k3bwriterselectionwidget.h"
#include "k3btempdirselectionwidget.h"
#include "k3bjob.h"
#include "k3bcore.h"
#include "k3bstdguiitems.h"
#include "k3bdatamodewidget.h"
#include "k3bglobals.h"
#include "k3bwritingmodewidget.h"
#include "k3bmediacache.h"
#include "k3bfilecompilationsizehandler.h"
#include "ThemeManager.h"

#include <QFile>
#include <QFileInfo>
#include <QPoint>
#include <QVariant>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QLayout>
#include <QToolTip>
#include <QRadioButton>
#include <QTabWidget>
#include <QSpinBox>
#include <QGridLayout>
#include <QMessageBox>

#include <KConfig>
#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>
#include <KIO/Global>
#include <KMessageBox>


K3b::DataBurnDialog::DataBurnDialog(K3b::DataDoc* _doc, QWidget *parent )
    : K3b::ProjectBurnDialog( _doc, parent )
{
    m_doc = _doc;
    prepareGui();

    setTitle( i18n("Data Project"), i18n("Size: %1", KIO::convertSize(_doc->size()) ) );

    disconnect( button_ok, SIGNAL( clicked() ), this, SLOT( slotStartClicked() ) );
    connect( button_ok, SIGNAL( clicked() ), this, SLOT( slotSaveToClicked() ) );

    // for now we just put the verify checkbox on the main page...
    m_checkVerify = K3b::StdGuiItems::verifyCheckBox( m_optionGroup );
    m_optionGroupLayout->addWidget( m_checkVerify );
    //**************
    m_checkVerify->hide();

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_optionGroupLayout->addItem( spacer );

    // create image settings tab
    m_imageSettingsWidget = new K3b::DataImageSettingsWidget( this );
    addPage( m_imageSettingsWidget, i18n("Filesystem") );

    setupSettingsTab();

    connect( m_comboMultisession, SIGNAL(activated(int)),
             this, SLOT(slotMultiSessionModeChanged()) );

    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE );

    m_tempDirSelectionWidget->setSelectionMode( K3b::TempDirSelectionWidget::FILE );
    QString path = _doc->tempDir();
    if( !path.isEmpty() ) {
        m_tempDirSelectionWidget->setTempPath( path );
    }
    if( !_doc->isoOptions().volumeID().isEmpty() ) {
        m_tempDirSelectionWidget->setDefaultImageFileName( _doc->isoOptions().volumeID() + ".iso" );
    }

    connect( m_imageSettingsWidget->m_editVolumeName, SIGNAL(textChanged(QString)),
             m_tempDirSelectionWidget, SLOT(setDefaultImageFileName(QString)) );
}

K3b::DataBurnDialog::~DataBurnDialog(){
}

void K3b::DataBurnDialog::slotCancelClicked()
{
    saveConfig( );
    saveSettingsToProject();
    close();
}

void K3b::DataBurnDialog::saveConfig( )
{
    KConfigGroup grp( KSharedConfig::openConfig(), "default " + m_doc->typeString() + " settings" );   
    saveSettings( grp );
}

void K3b::DataBurnDialog::setComboMedium( K3b::Device::Device* dev )
{
    m_writerSelectionWidget->setWantedMedium( dev );
}


void K3b::DataBurnDialog::saveSettingsToProject()
{
    K3b::ProjectBurnDialog::saveSettingsToProject();

    // save iso image settings
    K3b::IsoOptions o = ((K3b::DataDoc*)doc())->isoOptions();
    m_imageSettingsWidget->save( o );
    ((K3b::DataDoc*)doc())->setIsoOptions( o );

    // save image file path
    ((K3b::DataDoc*)doc())->setTempDir( m_tempDirSelectionWidget->tempPath() );

    // save multisession settings
    ((K3b::DataDoc*)doc())->setMultiSessionMode( m_comboMultisession->multiSessionMode() );

    ((K3b::DataDoc*)doc())->setDataMode( m_dataModeWidget->dataMode() );

    ((K3b::DataDoc*)doc())->setVerifyData( m_checkVerify->isChecked() );
}


void K3b::DataBurnDialog::readSettingsFromProject()
{
    K3b::ProjectBurnDialog::readSettingsFromProject();

    // read multisession
    m_comboMultisession->setMultiSessionMode( ((K3b::DataDoc*)doc())->multiSessionMode() );

    if( !doc()->tempDir().isEmpty() )
        m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );
    else
        m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() + doc()->name() + ".iso" );

    m_checkVerify->setChecked( ((K3b::DataDoc*)doc())->verifyData() );

    m_imageSettingsWidget->load( ((K3b::DataDoc*)doc())->isoOptions() );

    m_dataModeWidget->setDataMode( ((K3b::DataDoc*)doc())->dataMode() );

    toggleAll();
    slotMultiSessionModeChanged();
}


void K3b::DataBurnDialog::setupSettingsTab()
{
    //QWidget* frame = new QWidget( this );
    QWidget* frame = new QWidget(  );
    QGridLayout* frameLayout = new QGridLayout( frame );

    m_groupDataMode = new QGroupBox( i18n("Datatrack Mode"), frame );
    m_dataModeWidget = new K3b::DataModeWidget( m_groupDataMode );
    QVBoxLayout* groupDataModeLayout = new QVBoxLayout( m_groupDataMode );
    groupDataModeLayout->addWidget( m_dataModeWidget );

    QGroupBox* groupMultiSession = new QGroupBox( i18n("Multisession Mode"), frame );
    m_comboMultisession = new K3b::DataMultiSessionCombobox( groupMultiSession );
    QVBoxLayout* groupMultiSessionLayout = new QVBoxLayout( groupMultiSession );
    groupMultiSessionLayout->addWidget( m_comboMultisession );

    frameLayout->addWidget( m_groupDataMode, 0, 0 );
    frameLayout->addWidget( groupMultiSession, 1, 0 );
    frameLayout->setRowStretch( 2, 1 );

    //addPage( frame, i18n("Misc") );
}


void K3b::DataBurnDialog::slotStartClicked()
{
    if( m_checkOnlyCreateImage->isChecked() ||
        m_checkCacheImage->isChecked() ) {
        QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
        if( fi.isDir() )
            m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );

        if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
#if 0
            if( KMessageBox::warningContinueCancel( this,
                                                    i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath()),
                                                    i18n("File Exists"), KStandardGuiItem::overwrite() )
                == KMessageBox::Continue ) {
                // delete the file here to avoid problems with free space in K3b::ProjectBurnDialog::slotStartClicked
                QFile::remove( m_tempDirSelectionWidget->tempPath() );
            }
#endif
            QMessageBox box(QMessageBox::Warning,
                            i18n("File Exists"),
                            i18n("Do you want to overwrite %1?",m_tempDirSelectionWidget->tempPath()),
                            QMessageBox::Ok | QMessageBox::Cancel,
                            this);
            QPoint p(k3bappcore->k3bMainWindow()->pos().x() + (k3bappcore->k3bMainWindow()->width() - box.width()) / 2,
                     k3bappcore->k3bMainWindow()->pos().y() + (k3bappcore->k3bMainWindow()->height() - box.height()) / 2);
            box.move(p);
            if (box.exec() == QMessageBox::Ok)
                QFile::remove( m_tempDirSelectionWidget->tempPath() );
            else
                return;
        }
    }

    if( m_writingModeWidget->writingMode() == K3b::WritingModeSao &&
        m_comboMultisession->multiSessionMode() != K3b::DataDoc::NONE &&
        m_writerSelectionWidget->writingApp() == K3b::WritingAppCdrecord )
        if( KMessageBox::warningContinueCancel( this,
                                                i18n("Most writers do not support writing "
                                                     "multisession CDs in DAO mode.") )
            == KMessageBox::Cancel )
            return;
    //readSettingsFromProject();
    K3b::ProjectBurnDialog::slotStartClicked();
}


void K3b::DataBurnDialog::loadSettings( const KConfigGroup& c )
{
    K3b::ProjectBurnDialog::loadSettings(c);

    m_dataModeWidget->loadConfig(c);
    m_comboMultisession->loadConfig( c );

    K3b::IsoOptions o = K3b::IsoOptions::load( c );
    m_imageSettingsWidget->load( o );

    m_checkVerify->setChecked( c.readEntry( "verify data", false ) );

    toggleAll();
}


void K3b::DataBurnDialog::saveSettings( KConfigGroup c )
{
    K3b::ProjectBurnDialog::saveSettings(c);

    m_dataModeWidget->saveConfig(c);
    m_comboMultisession->saveConfig( c );

    K3b::IsoOptions o;
    m_imageSettingsWidget->save( o );
    o.save( c );

    c.writeEntry( "verify data", m_checkVerify->isChecked() );
}


void K3b::DataBurnDialog::toggleAll()
{
    K3b::ProjectBurnDialog::toggleAll();

    if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
        m_checkVerify->setChecked(false);
        m_checkVerify->setEnabled(false);
    }
    else
        m_checkVerify->setEnabled(true);

    m_comboMultisession->setDisabled( m_checkOnlyCreateImage->isChecked() );
    // we can only select the data mode for CD media
    // IDEA: why not give GUI elements like this a slot that reacts on media changes?
    m_dataModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() ||
                                   !K3b::Device::isCdMedia( k3bappcore->mediaCache()->diskInfo( m_writerSelectionWidget->writerDevice() ).mediaType() ) );

    // Multisession in DAO is not possible
    if( m_writingModeWidget->writingMode() == K3b::WritingModeSao ) {
        if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::START ||
            m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
            m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
            KMessageBox::information( this, i18n("It is not possible to write multisession media in DAO mode. "
                                                 "Multisession has been disabled."),
                                      i18n("Multisession Problem"),
                                      "multisession_no_dao" );

        m_comboMultisession->setEnabled(false);
    }
    else {
        m_comboMultisession->setEnabled(true);
    }
}

void K3b::DataBurnDialog::paintEvent(QPaintEvent *e)
{
    QPalette pal = QApplication::style()->standardPalette();
    QColor c;

    c.setRed(231); c.setBlue(231); c.setGreen(231);
    if (c == pal.background().color())
    {
        pal.setColor(QPalette::Background, QColor("#FFFFFF"));
        setPalette(pal);
    }
    else
    {
        setPalette(pal);
    }

    QWidget::paintEvent(e);
}

void K3b::DataBurnDialog::slotMultiSessionModeChanged()
{
    if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
        m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
        m_spinCopies->setEnabled(false);

    // wait for the proper medium
    // we have to do this in another slot than toggleAll to avoid an endless loop
    // FIXME: K3b::InteractionDialog::slotToggleAll is endless loop protected
    if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::NONE )
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );
    else if( m_comboMultisession->multiSessionMode() == K3b::DataDoc::CONTINUE ||
             m_comboMultisession->multiSessionMode() == K3b::DataDoc::FINISH )
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_INCOMPLETE );
    else
        m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE );
}



