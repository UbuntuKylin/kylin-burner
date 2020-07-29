/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bprojectburndialog.h"
#include "k3bdoc.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"
#include "k3btempdirselectionwidget.h"
#include "k3bwriterselectionwidget.h"
#include "k3bstdguiitems.h"
#include "k3bwritingmodewidget.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bglobals.h"
#include "k3bcore.h"

#include <QDir>
#include <QString>
#include <QPushButton>
#include <QToolTip>

#include <KConfig>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KGuiItem>
#include <KStandardGuiItem>

#include <QDebug>
#include <QLayout>
#include <QWhatsThis>
#include <QCheckBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QBitmap>
#include <QPainter>
#include "k3bResultDialog.h"

K3b::ProjectBurnDialog::ProjectBurnDialog( K3b::Doc* doc, QWidget *parent )
    : K3b::InteractionDialog( parent,
                            i18n("Project"),
                            QString(),
                            START_BUTTON|SAVE_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "default " + doc->typeString() + " settings" ),
      m_writerSelectionWidget(0),
      m_tempDirSelectionWidget(0),
      m_imageTipText(i18n("Use the 'Image' tab to optionally adjust the path of the image."))
{
    flag = 1;

    m_doc = doc;

    KGuiItem closeItem = KStandardGuiItem::close();
    closeItem.setToolTip( i18n("Save Settings and close") );
    closeItem.setWhatsThis( i18n("Saves the settings to the project and closes the dialog.") );
    setButtonGui( SAVE_BUTTON, closeItem );
    KGuiItem cancelItem = KStandardGuiItem::cancel();
    cancelItem.setToolTip( i18n("Discard all changes and close") );
    cancelItem.setWhatsThis( i18n("Discards all changes made in the dialog and closes it.") );
    setButtonGui( CANCEL_BUTTON, cancelItem );

    m_job = 0;
}


K3b::ProjectBurnDialog::~ProjectBurnDialog(){
}


void K3b::ProjectBurnDialog::init()
{
    readSettingsFromProject();
//   if( !m_writerSelectionWidget->writerDevice() )
//     m_checkOnlyCreateImage->setChecked(true);
}


void K3b::ProjectBurnDialog::slotWriterChanged()
{
    slotToggleAll();
}


void K3b::ProjectBurnDialog::slotWritingAppChanged( K3b::WritingApp )
{
    slotToggleAll();
}


void K3b::ProjectBurnDialog::toggleAll()
{
    K3b::Device::Device* dev = m_writerSelectionWidget->writerDevice();
    if( dev ) {
        K3b::Medium burnMedium = k3bappcore->mediaCache()->medium( dev );

        if( burnMedium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_PLUS_ALL ) {
            // no simulation support for DVD+R(W)
            m_checkSimulate->setChecked(false);
            m_checkSimulate->setEnabled(false);
        }
        else {
            //m_checkSimulate->setEnabled(true);
            m_checkSimulate->setEnabled( false );
        }

        setButtonEnabled( START_BUTTON, true );
    }
    else
        setButtonEnabled( START_BUTTON, false );

    m_writingModeWidget->determineSupportedModesFromMedium( dev );

    m_writingModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkSimulate->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkCacheImage->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_checkRemoveBufferFiles->setDisabled( m_checkOnlyCreateImage->isChecked() || !m_checkCacheImage->isChecked() );
    if( m_checkOnlyCreateImage->isChecked() ) {
        m_checkRemoveBufferFiles->setChecked(false);
        setButtonEnabled( START_BUTTON, true );
    }
    m_tempDirSelectionWidget->setDisabled( !m_checkCacheImage->isChecked() && !m_checkOnlyCreateImage->isChecked() );
    m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
    m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );

    // we only support DAO with cdrdao
    if( m_writerSelectionWidget->writingApp() == K3b::WritingAppCdrdao )
        m_writingModeWidget->setSupportedModes( K3b::WritingModeSao );

    if( m_checkOnlyCreateImage->isChecked() )
        setButtonText( START_BUTTON,
                       i18n("Start"),
                       i18n("Start the image creation") );
    else
        setButtonText( START_BUTTON, i18n("Burn"),
                       i18n("Start the burning process") );
    
    m_checkSimulate->setEnabled( false );
}


int K3b::ProjectBurnDialog::execBurnDialog( bool burn )
{
    if( burn && m_job == 0 ) {
        setButtonShown( START_BUTTON, true );
        setDefaultButton( START_BUTTON );
    }
    else {
        setButtonShown( START_BUTTON, false );
        setDefaultButton( SAVE_BUTTON );
    }

    return K3b::InteractionDialog::exec();
}


void K3b::ProjectBurnDialog::slotSaveClicked()
{
    saveSettingsToProject();
    done( Saved );
}


void K3b::ProjectBurnDialog::slotCancelClicked()
{
    done( Canceled );
}


void K3b::ProjectBurnDialog::slotStartClicked()
{
    saveSettingsToProject();

    if( m_tempDirSelectionWidget ) {
        if( !doc()->onTheFly() || doc()->onlyCreateImages() ) {
            //
            // check if the temp dir exists
            //
            QString tempDir = m_tempDirSelectionWidget->tempDirectory();
            if( !QFile::exists( tempDir ) ) {
                if( KMessageBox::warningYesNo( this, i18n("Image folder '%1' does not exist. Do you want K3b to create it?", tempDir ) )
                    == KMessageBox::Yes ) {
                    if( !QDir().mkpath( tempDir ) ) {
                        KMessageBox::error( this, i18n("Failed to create folder '%1'.", tempDir ) );
                        return;
                    }
                }
                else
                    return;
            }

            //
            // check if enough space in tempdir if not on-the-fly
            //
            if( doc()->burningSize() > m_tempDirSelectionWidget->freeTempSpace() ) {
                if( KMessageBox::warningContinueCancel( this, i18n("There does not seem to be enough free space in the temporary folder. "
                                                                   "Write anyway?") ) == KMessageBox::Cancel )
                    return;
            }
        }
    }

    K3b::JobProgressDialog* dlg = 0;
   /* if( m_checkOnlyCreateImage && m_checkOnlyCreateImage->isChecked() )
        dlg = new K3b::JobProgressDialog( parentWidget() );
    else*/
        dlg = new K3b::BurnProgressDialog( parentWidget() );
    
    m_job = m_doc->newBurnJob( dlg );

    if( m_writerSelectionWidget )
        m_job->setWritingApp( m_writerSelectionWidget->writingApp() );
    prepareJob( m_job );

    hideTemporarily();

    connect( m_job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
    
    dlg->startJob(m_job);

    qDebug() << "(K3b::ProjectBurnDialog) job done. cleaning up.";
    
    delete m_job;
    
    BurnResult* dialog = new BurnResult( flag, "data");
    dialog->show();
    
    m_job = 0;
    delete dlg;

    done( Burn );
    
}

void K3b::ProjectBurnDialog::slotFinished( bool ret )
{
    flag = ret;
}


void K3b::ProjectBurnDialog::prepareGui()
{
    QVBoxLayout* mainLay = new QVBoxLayout( mainWidget() );
    mainLay->setContentsMargins( 0, 0, 0, 0 );

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setFixedSize(430, 485);

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(), 6, 6);
    setMask(bmp);

    QLabel *icon = new QLabel();
    icon->setFixedSize(16,16);
    icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/logo-小.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(50,13);
    title->setStyleSheet("QLabel{background-color:transparent;"
                         "background-repeat: no-repeat;color:#444444;"
                         "font: 12px;}");
    QPushButton *close = new QPushButton();
    close->setFixedSize(20,20);
    close->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                         "border:none;background-color:rgb(233, 233, 233);"
                         "border-radius: 4px;background-color:transparent;}"
                          "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                         "border:none;background-color:rgb(248, 100, 87);"
                         "border-radius: 4px;}");
    connect(close, SIGNAL( clicked() ), this, SLOT( close() ) );

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(27);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(11, 0, 0, 0); 
    titlebar->addWidget(icon);
    titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch();
    titlebar->addWidget(close);
    titlebar->addSpacing(5);
    
    mainLay->addWidget( label_top );
    mainLay->addSpacing( 20 );

    QLabel* label_title = new QLabel( i18n("burn setting"),this );
    //label_title->setFixedHeight(24);
    QFont label_font;
#if 0
    label_font.setPixelSize(24);
    label_title->setFont( label_font );
    label_title->setStyleSheet("color:#444444");
#endif    
    label_title->setStyleSheet("QLabel{width:96px; \
                                height:24px; \
                                font-size:24px; \
                                font-family:Microsoft YaHei; \
                                font-weight:400; \
                                color:rgba(68,68,68,1); \
                                line-height:32px;}");

    m_writerSelectionWidget = new K3b::WriterSelectionWidget();
    m_writerSelectionWidget->hideComboMedium();
    m_writerSelectionWidget->setWantedMediumType( m_doc->supportedMediaTypes() );
    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );
    m_writerSelectionWidget->setWantedMediumSize( m_doc->length() );
    //mainLay->addWidget( label_title );
    //mainLay->addSpacing( 30 );
    //mainLay->addWidget( m_writerSelectionWidget );
    //mainLay->addSpacing( 30 );

    //m_tabWidget = new QTabWidget( mainWidget() );
    m_tabWidget = new QTabWidget();
    //mainLay->addWidget( m_tabWidget );

    QWidget* w = new QWidget( m_tabWidget );
    m_tabWidget->addTab( w, i18n("Writing") );
    
    //hide tabBar
    m_tabWidget->tabBar()->hide();
    
    //QGroupBox* groupWritingMode = new QGroupBox( i18n("Writing Mode"), w );
    QGroupBox* groupWritingMode = new QGroupBox( i18n(" "), w );
    m_writingModeWidget = new K3b::WritingModeWidget( groupWritingMode );
    QVBoxLayout* groupWritingModeLayout = new QVBoxLayout( groupWritingMode );
    groupWritingModeLayout->addWidget( m_writingModeWidget );

    //m_optionGroup = new QGroupBox( i18n("Settings"), w );
    m_optionGroup = new QGroupBox();
    m_optionGroupLayout = new QVBoxLayout( m_optionGroup );

    // add the options
    m_checkCacheImage = K3b::StdGuiItems::createCacheImageCheckbox( m_optionGroup );
    m_checkCacheImage->setFixedHeight(16);
    label_font.setPixelSize(14);
    m_checkCacheImage->setFont( label_font );
    m_checkCacheImage->setStyleSheet("color:#444444;");

    m_checkSimulate = K3b::StdGuiItems::simulateCheckbox( m_optionGroup );
    m_checkSimulate->setFixedHeight(16);
    m_checkSimulate->setFont( label_font );
    m_checkSimulate->setStyleSheet("color:#444444;");
    m_checkSimulate->setDisabled( true );

    m_checkRemoveBufferFiles = K3b::StdGuiItems::removeImagesCheckbox( m_optionGroup );
    m_checkRemoveBufferFiles->setFixedHeight(16);
    m_checkRemoveBufferFiles->setFont( label_font );
    m_checkRemoveBufferFiles->setStyleSheet("color:#444444;");

    m_checkOnlyCreateImage = K3b::StdGuiItems::onlyCreateImagesCheckbox( m_optionGroup );
    m_checkOnlyCreateImage->setFixedHeight(16);
    m_checkOnlyCreateImage->setFont( label_font );
    m_checkOnlyCreateImage->setStyleSheet("color:#444444;");
   
    QLabel *lcheck = new QLabel(this);
    QVBoxLayout  *laycheck = new QVBoxLayout(lcheck);
    lcheck->setFixedHeight(112);
    laycheck->setContentsMargins(3, 0, 0, 0);
    laycheck->addWidget( m_checkSimulate );
    laycheck->addSpacing( 9 );
    laycheck->addWidget( m_checkCacheImage );
    laycheck->addSpacing( 9 );
    laycheck->addWidget( m_checkOnlyCreateImage );
    laycheck->addSpacing( 9 );
    laycheck->addWidget( m_checkRemoveBufferFiles );
    //laycheck->addSpacing( 25 );

   //tmp
    m_tempDirSelectionWidget = new K3b::TempDirSelectionWidget( );
    QLabel *m_labeltmpPath = new QLabel( m_optionGroup );
    QLineEdit *m_tmpPath = new QLineEdit( m_optionGroup );
    QString tmp_path =i18n("temp file path: ");
    KIO::filesize_t tempFreeSpace = m_tempDirSelectionWidget->freeTempSpace();
    QString tmp_size = m_tempDirSelectionWidget->tempPath() + "     "  +  KIO::convertSize(tempFreeSpace);
    m_labeltmpPath->setText( tmp_path );
    //m_labeltmpPath->setFixedSize( 56, 12);
    m_labeltmpPath->setFixedSize( 60, 18);
    m_labeltmpPath->setFont( label_font );
    m_labeltmpPath->setStyleSheet("color:#444444;");
    
    m_tmpPath->setText( tmp_size);
    m_tmpPath->setFixedSize( 368, 30);
    m_tmpPath->setFont( label_font );
    m_tmpPath->setStyleSheet("color:#444444;");

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(31, 0, 0, 0);
    vlayout->addWidget( label_title );
    vlayout->addSpacing( 20 );
    //vlayout->addStretch( 0 );
    vlayout->addWidget( m_writerSelectionWidget );
    vlayout->addSpacing( 20 );
    vlayout->addWidget( lcheck);
    vlayout->addSpacing( 12 );
    vlayout->addWidget( m_labeltmpPath );
    vlayout->addSpacing( 6 );
    vlayout->addWidget( m_tmpPath );
    vlayout->addStretch( 0 );

    mainLay->addLayout( vlayout );
/*
    m_optionGroupLayout->addWidget(m_checkSimulate);
    m_optionGroupLayout->addWidget(m_checkCacheImage);
    m_optionGroupLayout->addWidget(m_checkOnlyCreateImage);
    m_optionGroupLayout->addWidget(m_checkRemoveBufferFiles);

    //tmp
    m_optionGroupLayout->addWidget(m_labeltmpPath);
    m_optionGroupLayout->addWidget(m_tmpPath);

*/
    //QGroupBox* groupCopies = new QGroupBox( i18n("Copies"), w );
    QGroupBox* groupCopies = new QGroupBox( i18n(" "), w );
    QLabel* pixLabel = new QLabel( groupCopies );
    pixLabel->setPixmap( SmallIcon( "tools-media-optical-copy", KIconLoader::SizeMedium ) );
    pixLabel->setScaledContents( false );
    m_spinCopies = new QSpinBox( groupCopies );
    m_spinCopies->setRange( 1, 999 );
    QHBoxLayout* groupCopiesLayout = new QHBoxLayout( groupCopies );
    groupCopiesLayout->addWidget( pixLabel );
    groupCopiesLayout->addWidget( m_spinCopies );
#if 0
    // arrange it
    QGridLayout* grid = new QGridLayout( w );
    
    //grid->addWidget( groupWritingMode, 0, 0 );
    grid->addWidget( m_optionGroup, 0, 1, 3, 1 );
    //grid->addWidget( groupCopies, 2, 0 );
    //grid->addWidget( m_tempDirSelectionWidget, 1, 1, 3, 1 );
    grid->setRowStretch( 1, 1 );
    grid->setColumnStretch( 1, 1 );
#endif
    QWidget *tempW = new QWidget( m_tabWidget );
    //grid = new QGridLayout( tempW );
    m_tabWidget->addTab( tempW, i18n("Image") );
    m_tempDirSelectionWidget = new K3b::TempDirSelectionWidget( tempW );
    //grid->addWidget( m_tempDirSelectionWidget, 0, 0 );
    m_tempDirSelectionWidget->setNeededSize( doc()->size() );


//    setStyleSheet(QString::fromUtf8("border:1px solid red"));


    // tab order
    setTabOrder( m_writerSelectionWidget, m_writingModeWidget );
    setTabOrder( m_writingModeWidget, groupCopies );
    setTabOrder( groupCopies, m_optionGroup );

    // some default connections that should always be useful
    connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
    connect( m_writerSelectionWidget, SIGNAL(writerChanged(K3b::Device::Device*)),
             m_writingModeWidget, SLOT(determineSupportedModesFromMedium(K3b::Device::Device*)) );
    connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(K3b::WritingApp)), this, SLOT(slotWritingAppChanged(K3b::WritingApp)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
    connect( m_writingModeWidget, SIGNAL(writingModeChanged(WritingMode)), this, SLOT(slotToggleAll()) );

    connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotShowImageTip(bool)) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotShowImageTip(bool)) );
}

void K3b::ProjectBurnDialog::close()
{
    K3b::InteractionDialog::slotCancelClicked();
}

void K3b::ProjectBurnDialog::addPage( QWidget* page, const QString& title )
{
    m_tabWidget->addTab( page, title );
}

void K3b::ProjectBurnDialog::setOnlyCreateImage( bool ret )
{
    m_checkOnlyCreateImage->setChecked( ret );
}

void K3b::ProjectBurnDialog::setTmpPath( QString ret )
{
    m_tempDirSelectionWidget->setTempPath( ret );
}

void K3b::ProjectBurnDialog::saveSettingsToProject()
{
    m_doc->setDummy( m_checkSimulate->isChecked() );
    m_doc->setOnTheFly( !m_checkCacheImage->isChecked() );
    m_doc->setOnlyCreateImages( m_checkOnlyCreateImage->isChecked() );
    m_doc->setRemoveImages( m_checkRemoveBufferFiles->isChecked() );
    m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
    m_doc->setBurner( m_writerSelectionWidget->writerDevice() );
    m_doc->setWritingMode( m_writingModeWidget->writingMode() );
    m_doc->setWritingApp( m_writerSelectionWidget->writingApp() );
    m_doc->setCopies( m_spinCopies->value() );
}


void K3b::ProjectBurnDialog::readSettingsFromProject()
{
    m_checkSimulate->setChecked( doc()->dummy() );
    m_checkCacheImage->setChecked( !doc()->onTheFly() );
    m_checkOnlyCreateImage->setChecked( m_doc->onlyCreateImages() );
    m_checkRemoveBufferFiles->setChecked( m_doc->removeImages() );
    m_writingModeWidget->setWritingMode( doc()->writingMode() );
    m_writerSelectionWidget->setWriterDevice( doc()->burner() );
    m_writerSelectionWidget->setSpeed( doc()->speed() );
    m_writerSelectionWidget->setWritingApp( doc()->writingApp() );
    m_writerSelectionWidget->setWantedMediumType( doc()->supportedMediaTypes() );
    m_spinCopies->setValue( m_doc->copies() );
}


void K3b::ProjectBurnDialog::saveSettings( KConfigGroup c )
{
    m_writingModeWidget->saveConfig( c );
    c.writeEntry( "simulate", m_checkSimulate->isChecked() );
    c.writeEntry( "on_the_fly", !m_checkCacheImage->isChecked() );
    c.writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
    c.writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
    c.writeEntry( "copies", m_spinCopies->value() );

    m_tempDirSelectionWidget->saveConfig( c );
    m_writerSelectionWidget->saveConfig( c );
}


void K3b::ProjectBurnDialog::loadSettings( const KConfigGroup& c )
{
    m_writingModeWidget->loadConfig( c );
    m_checkSimulate->setChecked( c.readEntry( "simulate", false ) );
    m_checkCacheImage->setChecked( !c.readEntry( "on_the_fly", true ) );
    m_checkRemoveBufferFiles->setChecked( c.readEntry( "remove_image", true ) );
    m_checkOnlyCreateImage->setChecked( c.readEntry( "only_create_image", false ) );
    m_spinCopies->setValue( c.readEntry( "copies", 1 ) );

    m_tempDirSelectionWidget->readConfig( c );
    m_writerSelectionWidget->loadConfig( c );
}


void K3b::ProjectBurnDialog::slotShowImageTip( bool buttonActivated )
{
    if (buttonActivated && isVisible()) {
        // FIXME: use the tab bar's position
        QWhatsThis::showText(mapToGlobal(QPoint(rect().center().x(), rect().top())), m_imageTipText);
    }
}


