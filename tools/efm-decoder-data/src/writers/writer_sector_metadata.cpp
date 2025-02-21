/************************************************************************

    writer_sector_metadata.h

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

#include "writer_sector_metadata.h"

// This writer class writes metadata about sector data to a file

WriterSectorMetadata::WriterSectorMetadata()
{}

WriterSectorMetadata::~WriterSectorMetadata()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool WriterSectorMetadata::open(const QString &filename)
{
    m_file.setFileName(filename);
    if (!m_file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterSectorMetadata::open() - Could not open file" << filename << "for writing";
        return false;
    }
    qDebug() << "WriterSectorMetadata::open() - Opened file" << filename << "for data writing";

    // Write the metadata header
    m_file.write("efm-decode - Sector Metadata\n");
    m_file.write("Format: Address, mode and data valid flag\n");
    m_file.write("Each address represents a 2048 byte sector\n");

    return true;
}

void WriterSectorMetadata::write(const Sector &sector)
{
    if (!m_file.isOpen()) {
        qCritical() << "WriterSectorMetadata::write() - File is not open for writing";
        return;
    }

    // Write a metadata entry for the sector
    QString metadata = QString::number(sector.address().address()) + ","
            + QString::number(sector.mode());

    if (sector.isDataValid()) {
        metadata += ",true";
    } else {
        metadata += ",false";
    }

    // Write the metadata to the metadata file
    metadata += "\n";
    m_file.write(metadata.toUtf8());
}

void WriterSectorMetadata::close()
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.close();
    qDebug() << "WriterSectorMetadata::close(): Closed the sector metadata file" << m_file.fileName();
}

qint64 WriterSectorMetadata::size() const
{
    if (m_file.isOpen()) {
        return m_file.size();
    }

    return 0;
}