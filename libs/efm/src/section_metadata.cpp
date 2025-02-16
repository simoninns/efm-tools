/************************************************************************

    section_metadata.cpp

    EFM-library - Frame metadata classes
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

#include "section_metadata.h"

// SectionType class
// ---------------------------------------------------------------------------------------------------
QString SectionType::toString() const
{
    switch (m_type) {
    case LeadIn:
        return QStringLiteral("LEAD_IN");
    case LeadOut:
        return QStringLiteral("LEAD_OUT");
    case UserData:
        return QStringLiteral("USER_DATA");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

// Section time class
// ---------------------------------------------------------------------------------------------------
SectionTime::SectionTime() : m_frames(0)
{
    // There are 75 frames per second, 60 seconds per minute, and 60 minutes per hour
    // so the maximum number of frames is 75 * 60 * 60 = 270000
    if (m_frames >= 270000) {
        qFatal("SectionTime::SectionTime(): Invalid frame count of %d", m_frames);
    }
}

SectionTime::SectionTime(qint32 frames) : m_frames(frames)
{
    if (m_frames >= 270000) {
        qFatal("SectionTime::SectionTime(): Invalid frame count of %d", m_frames);
    }
}

SectionTime::SectionTime(quint8 minutes, quint8 seconds, quint8 frames)
{
    setTime(minutes, seconds, frames);
}

void SectionTime::setFrames(qint32 frames)
{
    if (frames < 0 || frames >= 270000) {
        qFatal("SectionTime::setFrames(): Invalid frame count of %d", frames);
    }

    m_frames = frames;
}

void SectionTime::setTime(quint8 minutes, quint8 seconds, quint8 frames)
{
    // Set the time in minutes, seconds, and frames

    // Ensure the time is sane
    if (minutes >= 60) {
        minutes = 59;
        qDebug().nospace() << "SectionTime::setTime(): Invalid minutes value " << minutes
                           << ", setting to 59";
    }
    if (seconds >= 60) {
        seconds = 59;
        qDebug().nospace() << "SectionTime::setTime(): Invalid seconds value " << seconds
                           << ", setting to 59";
    }
    if (frames >= 75) {
        frames = 74;
        qDebug().nospace() << "SectionTime::setTime(): Invalid frames value " << frames
                           << ", setting to 74";
    }

    m_frames = (minutes * 60 + seconds) * 75 + frames;
}

QString SectionTime::toString() const
{
    // Return the time in the format MM:SS:FF
    return QString("%1:%2:%3")
            .arg(m_frames / (75 * 60), 2, 10, QChar('0'))
            .arg((m_frames / 75) % 60, 2, 10, QChar('0'))
            .arg(m_frames % 75, 2, 10, QChar('0'));
}

QByteArray SectionTime::toBcd() const
{
    // Return 3 bytes of BCD data representing the time as MM:SS:FF
    QByteArray bcd;

    quint32 mins = m_frames / (75 * 60);
    quint32 secs = (m_frames / 75) % 60;
    quint32 frms = m_frames % 75;

    bcd.append(intToBcd(mins));
    bcd.append(intToBcd(secs));
    bcd.append(intToBcd(frms));

    return bcd;
}

quint8 SectionTime::intToBcd(quint32 value)
{
    if (value > 99) {
        qFatal("SectionTime::intToBcd(): Value must be in the range 0 to 99.");
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

// Section metadata class
// -----------------------------------------------------------------------------------------------
void SectionMetadata::setSectionType(const SectionType &sectionType)
{
    m_sectionType = sectionType;

    // Ensure track number is sane
    if (m_sectionType.type() == SectionType::LeadIn)
        m_trackNumber = 0;
    if (m_sectionType.type() == SectionType::LeadOut)
        m_trackNumber = 0;
    if ((m_sectionType.type() == SectionType::UserData) && (m_trackNumber < 1 || m_trackNumber > 98)) {
        m_trackNumber = 1;
    }
}

void SectionMetadata::setTrackNumber(quint8 trackNumber)
{
    m_trackNumber = trackNumber;

    // Ensure track number is sane
    if (m_sectionType.type() == SectionType::LeadIn)
        m_trackNumber = 0;
    if (m_sectionType.type() == SectionType::LeadOut)
        m_trackNumber = 0;
    if ((m_sectionType.type() == SectionType::UserData) && (m_trackNumber < 1 || m_trackNumber > 98)) {
        m_trackNumber = 1;
    }
}