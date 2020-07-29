#include "k3bFileFilterDialog.h"

#include <KLocalizedString>
#include <KConfig>
#include <KSharedConfig>
#include <KIO/Global>
#include <KConfigGroup>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QBitmap>

FileFilter::FileFilter(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->setFixedSize(430, 250);

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
    title->setFixedSize(48,13);
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
    connect(close, &QPushButton::clicked, this, &FileFilter::filter_exit);

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
    
    QLabel *filter_label = new QLabel( i18n("filterSet") );
    filter_label->setFixedHeight(25);
    filter_label->setStyleSheet("QLabel{background-color:transparent;\
                                        background-repeat: no-repeat;\
                                        width:96px;\
                                        height:24px;\
                                        font-size:24px;\
                                        font-family:Microsoft YaHei;\
                                        font-weight:400;\
                                        color:rgba(68,68,68,1);\
                                        line-height:32px;}");

    discard_hidden_file = new QCheckBox( i18n("discard hidden file") );
    discard_hidden_file->setChecked(true);
    discard_hidden_file->setFixedHeight(16);
    QFont label_font;
    label_font.setPixelSize(14);
    discard_hidden_file->setFont( label_font );
    discard_hidden_file->setStyleSheet("color:#444444;");

    
    discard_broken_link = new QCheckBox( i18n("discard broken link") );
    discard_broken_link->setChecked(true);
    discard_broken_link->setFixedHeight(18);
    discard_broken_link->setFont( label_font );
    discard_broken_link->setStyleSheet("color:#444444;");
    
    follow_link = new QCheckBox( i18n("follow link") );
    follow_link->setChecked(false);
    follow_link->setFixedHeight(18);
    follow_link->setFont( label_font );
    follow_link->setStyleSheet("color:#444444;");

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(25, 0, 0, 0);
    vlayout->addWidget(filter_label);
    vlayout->addSpacing(28);
    vlayout->addWidget( discard_hidden_file);
    vlayout->addSpacing(13);
    vlayout->addWidget( discard_broken_link);
    vlayout->addSpacing(13);
    vlayout->addWidget(follow_link);
    vlayout->addStretch(0);


    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(this);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget( label_top );
    mainWidgetLayout->addSpacing(24);
    mainWidgetLayout->addLayout(vlayout);

    this->setModal( true );

}

FileFilter::~FileFilter()
{
}

void FileFilter::filter_exit()
{
    KConfigGroup grp( KSharedConfig::openConfig(), "default data settings" );   
    grp.writeEntry( "discard hidden file", discard_hidden_file->isChecked() );
    grp.writeEntry( "discard symlinks", false );
    grp.writeEntry( "discard broken symlinks", discard_broken_link->isChecked() );
    grp.writeEntry( "follow symbolic links", follow_link->isChecked() );
    
    this->close();
}
