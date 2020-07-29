#include "k3bfileselect.h"

myFileSelect::myFileSelect(QWidget *parent) :
    QFileDialog(parent)
{

}
void  myFileSelect::go()
{
    QDialog::accept();
}
