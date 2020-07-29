#include "k3bResultDialog.h"

#include <KLocalizedString>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>
#include <QBitmap>
#include <QPainter>

BurnResult::BurnResult( int ret ,QString str, QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setFixedSize(430, 260);

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
    connect(close, SIGNAL(clicked()), this, SLOT(exit() ) );

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(27);
    QHBoxLayout *titlebar = new QHBoxLayout(label_top);
    titlebar->setContentsMargins(11, 0, 0, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch();
    titlebar->addWidget(close);
    titlebar->addSpacing(5);

    QLabel* label_icon = new QLabel();
    label_icon->setFixedSize(50,50);
    label_icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/icon-right.png);"
                              "background-repeat: no-repeat;background-color:transparent;}");
   
    QString string = str + " success!";
    QLabel* label_info = new QLabel( i18n( string.toLatin1().data() ), this );
    label_info->setStyleSheet("QLabel{background-color:transparent;"
                              "background-repeat: no-repeat;font:30px;color:#444444;}");

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addSpacing( 113 );
    hlayout->addWidget( label_icon );
    hlayout->addSpacing( 24 );
    hlayout->addWidget( label_info );
    hlayout->addStretch( 0 );
    
    if( !ret ){
        label_icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/icon-error.png);"
                                  "background-repeat: no-repeat;background-color:transparent;}");
        string = str + " failed!";
        label_info->setText( i18n( string.toLatin1().data() ) );
    }
    
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget( label_top );
    mainLayout->addSpacing( 78 );
    mainLayout->addLayout( hlayout );
    mainLayout->addStretch( 0 );

    setModal( true );
}

BurnResult::~BurnResult()
{
   
}

void BurnResult::exit()
{
    this->close();
}

