/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef K3BMD5DIALOG_H
#define K3BMD5DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

using namespace::std;
namespace K3b{

namespace Device {
    class DeviceManager;
    class Device;
}

//class AppDeviceManager;

class Md5Check : public QDialog
{
    Q_OBJECT

public:
    explicit Md5Check(QWidget *parent = nullptr);
    ~Md5Check();
    bool checkMd5(const char* cmd);

public Q_SLOTS:
    void exit();
    void slotMediaChange( K3b::Device::Device* );
    void slotDeviceChange( K3b::Device::DeviceManager* );
    void md5_start();
    void openfile();
    void checkChange(int state);

protected:
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *);

private:

    QComboBox* combo;
    QCheckBox* check;
    QLineEdit* lineedit;
    QStringList mount_index;
    QPushButton* button_open;
    QPushButton* button_ok;
    QPushButton *c;
};
}
#endif // MD5DIALOG_H

