#include "kylinburnerfilefilter.h"
#include "ui_kylinburnerfilefilter.h"

KylinBurnerFileFilter::KylinBurnerFileFilter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerFileFilter)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->move(parent->width() / 2 - width() / 2, parent->height() / 2 - height() / 2);
    this->hide();
    ui->labelTitle->setText(i18n("Kylin-Burner"));
}

KylinBurnerFileFilter::~KylinBurnerFileFilter()
{
    delete ui;
}
