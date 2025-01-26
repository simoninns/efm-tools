/************************************************************************

    data24.h

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#ifndef DATA24_H
#define DATA24_H

#include <QVector>
#include <QString>
#include <QByteArray>
#include <QFile>

class Data24 {
public:
    Data24(const QString _filename, bool _is_wav);
    ~Data24();
    QVector<uint8_t> read();
    bool is_ready() const;
    void show_data();

private:
    QFile file;
    QString filename;
    bool is_wav;
    bool ready;
    QVector<uint8_t> last_frame;

    bool open_wav();
    bool open_raw();
};

#endif // DATA24_H
