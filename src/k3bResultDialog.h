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

#ifndef BURNRESULTDIALOG_H
#define BURNRESULTDIALOG_H

#include <QDialog>

class BurnResult : public QDialog
{
    Q_OBJECT

public:
    explicit BurnResult( int , QString ,QWidget *parent = nullptr);
    ~BurnResult();

    Q_SLOT void exit();
protected:
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
private:
    QPushButton *c;
};

#endif // BURNRESULTDIALOG_H

