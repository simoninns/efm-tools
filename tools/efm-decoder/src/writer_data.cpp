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
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool WriterData::open(const QString &filename)
{
    m_file.setFileName(filename);
    if (!m_file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterData::open() - Could not open file" << filename << "for writing";
        return false;
    }
    qDebug() << "WriterData::open() - Opened file" << filename << "for data writing";
    return true;
}

void WriterData::write(const Data24Section &data24Section)
{
    if (!m_file.isOpen()) {
        qCritical() << "WriterData::write() - File is not open for writing";
        return;
    }

    // Each Data24 section contains 98 frames that we need to write to the output file
    for (int index = 0; index < 98; index++) {
        Data24 data24 = data24Section.frame(index);
        m_file.write(reinterpret_cast<const char *>(data24.getData().data()),
                     data24.getFrameSize() * sizeof(uint8_t));
    }
}

void WriterData::close()
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.close();
    qDebug() << "WriterData::close(): Closed the data file" << m_file.fileName();
}

qint64 WriterData::size() const
{
    if (m_file.isOpen()) {
        return m_file.size();
    }

    return 0;
}