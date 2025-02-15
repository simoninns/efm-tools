/************************************************************************

    reader_data.cpp

    ld-efm-decoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-decoder is free software: you can redistribute it and/or
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

#include "reader_data.h"

ReaderData::ReaderData() {
}

ReaderData::~ReaderData() {
    if (file.isOpen()) {
        file.close();
    }
}

bool ReaderData::open(const QString &filename) {
    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "ReaderData::open() - Could not open file" << filename << "for reading";
        return false;
    }
    qDebug() << "ReaderData::open() - Opened file" << filename << "for data reading";
    return true;
}

QByteArray ReaderData::read(uint32_t chunk_size) {
    if (!file.isOpen()) {
        qCritical() << "ReaderData::read() - File is not open for reading";
        return QByteArray();
    }
    QByteArray data = file.read(chunk_size);
    return data;
}

void ReaderData::close() {
    if (!file.isOpen()) {
        return;
    }

    file.close();
    qDebug() << "ReaderData::close(): Closed the data file" << file.fileName(); 
}

int32_t ReaderData::size() {
    return file.size();
}