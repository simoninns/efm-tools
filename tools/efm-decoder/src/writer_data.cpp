/************************************************************************

    writer_data.cpp

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

#include "writer_data.h"

// This writer class writes raw data to a file directly from the Data24 sections
// This is (generally) used when the output is not stereo audio data

WriterData::WriterData() { }

WriterData::~WriterData()
{
    if (file.isOpen()) {
        file.close();
    }
}

bool WriterData::open(const QString &filename)
{
    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterData::open() - Could not open file" << filename << "for writing";
        return false;
    }
    qDebug() << "WriterData::open() - Opened file" << filename << "for data writing";
    return true;
}

void WriterData::write(const Data24Section &data24_section)
{
    if (!file.isOpen()) {
        qCritical() << "WriterData::write() - File is not open for writing";
        return;
    }

    // Each Data24 section contains 98 frames that we need to write to the output file
    for (int index = 0; index < 98; index++) {
        Data24 data24 = data24_section.get_frame(index);
        file.write(reinterpret_cast<const char *>(data24.get_data().data()),
                   data24.get_frame_size() * sizeof(uint8_t));
    }
}

void WriterData::close()
{
    if (!file.isOpen()) {
        return;
    }

    file.close();
    qDebug() << "WriterData::close(): Closed the data file" << file.fileName();
}

int64_t WriterData::size()
{
    if (file.isOpen()) {
        return file.size();
    }

    return 0;
}