/************************************************************************

    sector.cpp

    EFM-library - EFM Section classes
    Copyright (C) 2025 Simon Inns

    This file is part of EFM-Tools.

    This is free software: you can redistribute it and/or
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

#include "sector.h"

// Sector address class
// ---------------------------------------------------------------------------------------------------
SectorAddress::SectorAddress() : m_address(0)
{
    // There are 75 frames per second, 60 seconds per minute, and 60 minutes per hour
    // so the maximum number of frames is 75 * 60 * 60 = 270000
    if (m_address < 0 || m_address >= 270000) {
        qFatal("SectorAddress::SectionTime(): Invalid address value of %d", m_address);
    }
}

SectorAddress::SectorAddress(qint32 address) : m_address(address)
{
    if (m_address < 0 || m_address >= 270000) {
        qFatal("SectorAddress::SectionTime(): Invalid address value of %d", m_address);
    }
}

SectorAddress::SectorAddress(quint8 minutes, quint8 seconds, quint8 frames)
{
    setTime(minutes, seconds, frames);
}

void SectorAddress::setAddress(qint32 address)
{
    if (address < 0 || address >= 270000) {
        qFatal("SectorAddress::setFrames(): Invalid address value of %d", address);
    }

    m_address = address;
}

void SectorAddress::setTime(quint8 minutes, quint8 seconds, quint8 frames)
{
    // Set the address in minutes, seconds, and frames

    // Ensure the time is sane
    if (minutes >= 60) {
        qDebug().nospace() << "SectorAddress::setTime(): Invalid minutes value " << minutes
            << ", setting to 59";
        minutes = 59;
    }
    if (seconds >= 60) {
        qDebug().nospace() << "SectorAddress::setTime(): Invalid seconds value " << seconds
            << ", setting to 59";
        seconds = 59;
    }
    if (frames >= 75) {
        qDebug().nospace() << "SectorAddress::setTime(): Invalid frames value " << frames
            << ", setting to 74";
        frames = 74;
    }

    m_address = (minutes * 60 + seconds) * 75 + frames;
}

QString SectorAddress::toString() const
{
    // Return the time in the format MM:SS:FF
    return QString("%1:%2:%3")
            .arg(m_address / (75 * 60), 2, 10, QChar('0'))
            .arg((m_address / 75) % 60, 2, 10, QChar('0'))
            .arg(m_address % 75, 2, 10, QChar('0'));
}

QByteArray SectorAddress::toBcd() const
{
    // Return 3 bytes of BCD data representing the time as MM:SS:FF
    QByteArray bcd;

    quint32 mins = m_address / (75 * 60);
    quint32 secs = (m_address / 75) % 60;
    quint32 frms = m_address % 75;

    bcd.append(intToBcd(mins));
    bcd.append(intToBcd(secs));
    bcd.append(intToBcd(frms));

    return bcd;
}

quint8 SectorAddress::intToBcd(quint32 value)
{
    if (value > 99) {
        qFatal("SectorAddress::intToBcd(): Value must be in the range 0 to 99.");
    }

    quint16 bcd = 0;
    quint16 factor = 1;

    while (value > 0) {
        bcd += (value % 10) * factor;
        value /= 10;
        factor *= 16;
    }

    // Ensure the result is always 1 byte (00-99)
    return bcd & 0xFF;
}

// Raw sector class
// The raw sector is 2352 bytes (unscrambled) and contains user data and error correction data
RawSector::RawSector()
    : m_data(QByteArray(2340, 0)), // 2352 bytes - 12 byte sync pattern
      m_errorData(QByteArray(2340, 0))
{}

void RawSector::pushData(const QByteArray &inData)
{
    m_data = inData;
}

void RawSector::pushErrorData(const QByteArray &inData)
{
    m_errorData = inData;
}

QByteArray RawSector::data() const
{
    return m_data;
}

QByteArray RawSector::errorData() const
{
    return m_errorData;
}

quint32 RawSector::size() const
{
    return m_data.size();
}

void RawSector::showData()
{
    const int bytesPerLine = 48;
    bool hasError = false;

    for (int offset = 0; offset < m_data.size(); offset += bytesPerLine) {
        // Print offset
        QString line;
        
        line = "RawSector::showData() - " + QString("%1: ").arg(offset, 6, 16, QChar('0'));
        
        // Print hex values
        for (int i = 0; i < bytesPerLine && (offset + i) < m_data.size(); ++i) {
            if (static_cast<quint8>(m_errorData[offset + i]) == 0) {
                line.append(QString("%1 ").arg(static_cast<quint8>(m_data[offset + i]), 2, 16, QChar('0')));
            } else {
                line.append("XX ");
                hasError = true;
            }
        }

        qInfo().noquote() << line.trimmed();
    }

    if (hasError) {
        qInfo().noquote() << "RawSector contains errors";
    }
}

// Sector class
// The sector is 2048 bytes and contains user data only (post error correction)
Sector::Sector()
    : m_data(QByteArray(2048, 0)),
      m_errorData(QByteArray(2048, 0))
{}

void Sector::pushData(const QByteArray &inData)
{
    m_data = inData;
}

void Sector::pushErrorData(const QByteArray &inData)
{
    m_errorData = inData;
}

QByteArray Sector::data() const
{
    return m_data;
}

QByteArray Sector::errorData() const
{
    return m_errorData;
}

quint32 Sector::size() const
{
    return m_data.size();
}

void Sector::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < m_data.size(); ++i) {
        if (static_cast<quint8>(m_errorData[i]) == 0) {
            dataString.append(QString("%1 ").arg(m_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }
    if (hasError) {
        qInfo().noquote() << "Sector:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "Sector:" << dataString.trimmed();
    }
}

void Sector::setAddress(SectorAddress address)
{
    m_address = address;
}

SectorAddress Sector::address() const
{
    return m_address;
}

void Sector::setMode(qint32 mode)
{
    // -1 is invalid/unknown
    // 0 is mode 0
    // 1 is mode 1
    // 2 is mode 2

    if (mode < -1 || mode > 2) {
        qFatal("Sector::setMode(): Invalid mode value of %d", mode);
    }
    m_mode = mode;
}

qint32 Sector::mode() const
{
    return m_mode;
}