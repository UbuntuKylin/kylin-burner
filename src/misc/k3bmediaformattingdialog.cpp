/*
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
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

#include "k3bmediaformattingdialog.h"
#include "xatom-helper.h"

#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include "k3bResultDialog.h"

#include "k3bdvdformattingjob.h"
#include "k3bblankingjob.h"

#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3bwriterselectionwidget.h"
#include "k3bwritingmodewidget.h"
#include "k3bjobprogressdialog.h"
#include "ThemeManager.h"

#include <KConfig>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KMessageBox>

#include <QGroupBox>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QToolTip>
#include <QPainter>
#include <QBitmap>


K3b::MediaFormattingDialog::MediaFormattingDialog( QWidget* parent )
    : K3b::InteractionDialog( parent,
                            i18n("Format and Erase"),
                            i18n( "CD-RW" ) + '/' + i18n( "DVD±RW" ) + '/' + i18n( "BD-RE" ),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "Formatting and Erasing" ) // config group
{

    QWidget* frame = mainWidget();
    

    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);

    this->setObjectName("CleanDialog");
    ThManager()->regTheme(this, "ukui-white", "#CleanDialog{background-color: #FFFFFF;"
                                              "}");
    ThManager()->regTheme(this, "ukui-black", "#CleanDialog{background-color: #000000;"
                                              "}");
    resize(430, 280);
    button_ok->setObjectName("BtnCleanOK");
    ThManager()->regTheme(button_ok, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
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
    ThManager()->regTheme(button_ok, "ukui-black",
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


    button_cancel->setObjectName("BtnCleanCancel");
    ThManager()->regTheme(button_cancel, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
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
    ThManager()->regTheme(button_cancel, "ukui-black",
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
    icon = new QLabel();
    icon->setFixedSize(30,30);
    icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/logo.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    //title->setFixedSize(48,11);
    title->setFixedSize(80,30);
    //title->setFixedWidth(48);
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setObjectName("CleanTitles");
    ThManager()->regTheme(title, "ukui-white", "background-color:transparent;"
                                               "font: 14px; color: #444444;");
    ThManager()->regTheme(title, "ukui-black", "background-color:transparent;"
                                               "font: 14px; color: #FFFFFF;");

    c = new QPushButton();
    c->setFlat(true);
    c->setFixedSize(30,30);
    c->setIcon(QIcon(":/icon/icon/icon-关闭-默认.png"));
    c->setIconSize(QSize(26, 26));
    c->setProperty("isWindowButton", 0x2);
    c->setProperty("useIconHighlightEffect", 0x8);
    c->installEventFilter(this);
#if 0
    close->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                         "border:none;background-color:rgb(233, 233, 233);"
                         "border-radius: 4px;background-color:transparent;}"
                          "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                         "border:none;background-color:rgb(248, 100, 87);"
                         "border-radius: 4px;}");
#endif
    connect(c, SIGNAL( clicked() ), this, SLOT( slotCancelClicked() ));

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    label_top->setFixedWidth(430);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(11, 4, 4, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(0);
    titlebar->addWidget(title);
    titlebar->addStretch();
    titlebar->addWidget(c);
    //titlebar->addSpacing(5);

    QLabel* label_title = new QLabel( this );
    label_title->setText( i18n("clean") );
//    label_title->setFixedHeight(24);
    label_title->setStyleSheet("QLabel{width:48px;\
                                      height:24px;\
                                      font-size:24px;\
                                      font-family:Microsoft YaHei;\
                                      font-weight:400;\
                                      color:rgba(68,68,68,1);}");
    label_title->setObjectName("CleanLabelTitles");
    ThManager()->regTheme(label_title, "ukui-white", "background-color:transparent;"
                                               "font: 24px; font-weight:400; color: #444444;");
    ThManager()->regTheme(label_title, "ukui-black", "background-color:transparent;"
                                               "font: 24px; font-weight:400; color: #FFFFFF;");

    QLabel* label_CD = new QLabel( this );
    label_CD->setText( i18n("select CD") );
 //   label_CD->setFixedHeight(12);
    label_CD->setStyleSheet("QLabel{width:140px;\
                                       height:12px;\
                                       font-size:14px;\
                                       font-family:Microsoft YaHei;\
                                       font-weight:400;\
                                       color:rgba(68,68,68,1);}");
   label_CD->setObjectName("CleanLabelCDs");
   ThManager()->regTheme(label_CD, "ukui-white", "background-color:transparent;"
                                              "font: 14px; color: #444444;");
   ThManager()->regTheme(label_CD, "ukui-black", "background-color:transparent;"
                                              "font: 14px; color: #FFFFFF;");

    m_writerSelectionWidget = new K3b::WriterSelectionWidget( frame );
    m_writerSelectionWidget->setFixedWidth(369);
    m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_REWRITABLE );
    m_writerSelectionWidget->hideSpeed();
    // we need state empty here for preformatting DVD+RW.
    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_COMPLETE|
                                                   K3b::Device::STATE_INCOMPLETE|
                                                   K3b::Device::STATE_EMPTY );
    m_writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppDvdRwFormat );
    m_writerSelectionWidget->setForceAutoSpeed(true);
#if 1
    //QGroupBox* groupWritingMode = new QGroupBox( i18n("Writing Mode"), frame );
    QGroupBox* groupWritingMode = new QGroupBox( i18n("Writing Mode"));
    m_writingModeWidget = new K3b::WritingModeWidget( K3b::WritingModeIncrementalSequential|K3b::WritingModeRestrictedOverwrite,
                                                    groupWritingMode );
    QVBoxLayout* groupWritingModeLayout = new QVBoxLayout( groupWritingMode );
    groupWritingModeLayout->addWidget( m_writingModeWidget );
    groupWritingModeLayout->addStretch( 1 );

    //QGroupBox* groupOptions = new QGroupBox( i18n("Settings"), frame );
    QGroupBox* groupOptions = new QGroupBox( i18n("Settings"));
    m_checkForce = new QCheckBox( i18n("Force"), groupOptions );
    m_checkQuickFormat = new QCheckBox( i18n("Quick format"), groupOptions );
    QVBoxLayout* groupOptionsLayout = new QVBoxLayout( groupOptions );
    groupOptionsLayout->addWidget( m_checkForce );
    groupOptionsLayout->addWidget( m_checkQuickFormat );
    groupOptionsLayout->addStretch( 1 );
    
    //QGridLayout* grid = new QGridLayout( frame );
    //grid->addWidget( groupWritingMode, 1, 0 );
    //grid->addWidget( groupOptions, 1, 1 );
    //grid->setRowStretch( 1, 1 );
#endif
#if 1
    QLabel* label_bottom = new QLabel( this );
    QVBoxLayout* vlayout = new QVBoxLayout( label_bottom );
    vlayout->setContentsMargins( 31, 0, 0, 0 );
    vlayout->addWidget( label_title );
    vlayout->addSpacing(29);
    vlayout->addWidget( label_CD );
    vlayout->addSpacing(10);
    vlayout->addWidget( m_writerSelectionWidget );
    vlayout->addStretch(0);

    QVBoxLayout* vlayout_frame  =  new QVBoxLayout( frame );
    vlayout_frame->setContentsMargins( 0, 0, 0, 0 );

    vlayout_frame->addWidget( label_top );
    vlayout_frame->addSpacing(30);
    vlayout_frame->addWidget( label_bottom );
#else
    label_top->move( 11, 0);
    label_title->move(31, 57);
    label_CD->move( 31, 110);
    m_writerSelectionWidget->move(31,132);
#endif
    m_checkForce->setToolTip( i18n("Force formatting of empty DVDs") );
    m_checkForce->setWhatsThis( i18n("<p>If this option is checked K3b will format a "
                                     "DVD-RW even if it is empty. It may also be used to "
                                     "force K3b to format a DVD+RW, BD-RE or a DVD-RW in restricted "
                                     "overwrite mode."
                                     "<p><b>Caution:</b> It is not recommended to format a DVD often "
                                     "as it may become unusable after only 10-20 reformat procedures."
                                     "<p>DVD+RW and BD-RE media only needs to be formatted once. After that it "
                                     "just needs to be overwritten. The same applies to DVD-RW in "
                                     "restricted overwrite mode.") );

    m_checkQuickFormat->setToolTip( i18n("Try to perform quick formatting") );
    m_checkQuickFormat->setWhatsThis( i18n("<p>If this option is checked K3b will tell the writer "
                                           "to perform a quick format."
                                           "<p>Erasing a rewritable medium completely can take a very long "
                                           "time and some writers perform a full format even if "
                                           "quick format is enabled." ) );

    connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotToggleAll()) );

    slotToggleAll();

}

bool K3b::MediaFormattingDialog::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == c)
    {
        if(e->type() == QEvent::HoverEnter)
            c->setIcon(QIcon(":/icon/icon/icon-关闭-悬停点击.png"));
        else if (e->type() == QEvent::HoverLeave)
            c->setIcon(QIcon(":/icon/icon/icon-关闭-默认.png"));
        else return QWidget::eventFilter(obj, e);
    }
    return QWidget::eventFilter(obj, e);
}

K3b::MediaFormattingDialog::~MediaFormattingDialog()
{
    delete icon;
}


void K3b::MediaFormattingDialog::setDevice( K3b::Device::Device* dev )
{
    icon->setFocus();
    m_writerSelectionWidget->setWriterDevice( dev );
}

void K3b::MediaFormattingDialog::slotCancelClicked()
{
    icon->setFocus();
    this->close();
}

void K3b::MediaFormattingDialog::slotFinished(bool f)
{
    flag = f;
}

void K3b::MediaFormattingDialog::slotStartClicked()
{
    Device::Device* dev = m_writerSelectionWidget->writerDevice();
    if (NULL == dev) {
        KMessageBox::information( this, i18n("the media in device is not cleanable"),
                                     i18n("no media to clean") );
        /*
        KMessageBox::information( this, i18n("no media to clean"),
                                     i18n("no media to clean") );
        */
        button_ok->setEnabled(false); return;
    }
    if (!((m_writerSelectionWidget->wantedMediumType() & dev->readCapabilities()) ||
          (m_writerSelectionWidget->wantedMediumType() & dev->writeCapabilities())))
    {
        KMessageBox::information( this, i18n("the media in device is not cleanable"),
                                     i18n("no media to clean") );
        button_ok->setEnabled(false);
        return;
    }

    K3b::Medium medium = k3bappcore->mediaCache()->medium( m_writerSelectionWidget->writerDevice() );

    K3b::JobProgressDialog dlg( parentWidget(), false );

    K3b::Job* theJob = 0;

    if( medium.diskInfo().mediaType() & K3b::Device::MEDIA_CD_ALL ) {
        K3b::BlankingJob* job = new K3b::BlankingJob( &dlg, this );

        job->setDevice( m_writerSelectionWidget->writerDevice() );
        job->setSpeed( m_writerSelectionWidget->writerSpeed() );
        job->setForce( m_checkForce->isChecked() );
        job->setWritingApp( m_writerSelectionWidget->writingApp() );
        // no support for all the strange erasing modes anymore, they did not work anyway
        job->setFormattingMode( m_checkQuickFormat->isChecked() ? FormattingQuick : FormattingComplete );

        theJob = job;
    }
    else { // DVDFormattingJob handles DVD and BD discs
        K3b::DvdFormattingJob* job = new K3b::DvdFormattingJob( &dlg, this );

        job->setDevice( m_writerSelectionWidget->writerDevice() );
        job->setMode( m_writingModeWidget->writingMode() );
        job->setForce( m_checkForce->isChecked() );
        job->setFormattingMode( m_checkQuickFormat->isChecked() ? FormattingQuick : FormattingComplete );

        theJob = job;
    }

    //hide();
    connect( theJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );

    dlg.startJob( theJob );

    delete theJob;

    if( KConfigGroup( KSharedConfig::openConfig(), "General Options" ).readEntry( "keep action dialogs open", false ) )
        show();
    else
        close();
    BurnResult* dialog = new BurnResult( flag, "clean");
    dialog->show();
}


void K3b::MediaFormattingDialog::toggleAll()
{
    K3b::Medium medium = k3bappcore->mediaCache()->medium( m_writerSelectionWidget->writerDevice() );
    K3b::WritingModes modes = 0;


    if ( medium.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_RW|K3b::Device::MEDIA_DVD_RW_SEQ|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
        modes |=  K3b::WritingModeIncrementalSequential|K3b::WritingModeRestrictedOverwrite;
    }
    m_writingModeWidget->setSupportedModes( modes );
    setButtonEnabled( START_BUTTON, m_writerSelectionWidget->writerDevice() != 0 );
}


void K3b::MediaFormattingDialog::loadSettings( const KConfigGroup& c )
{
    m_checkForce->setChecked( c.readEntry( "force", false ) );
    m_checkQuickFormat->setChecked( c.readEntry( "quick format", true ) );
    m_writerSelectionWidget->loadConfig( c );
    m_writingModeWidget->loadConfig( c );
}


void K3b::MediaFormattingDialog::saveSettings( KConfigGroup c )
{
    c.writeEntry( "force", m_checkForce->isChecked() );
    c.writeEntry( "quick format", m_checkQuickFormat->isChecked() );
    m_writerSelectionWidget->saveConfig( c );
    m_writingModeWidget->saveConfig( c );
}



