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

// Section time class ---------------------------------------------------------------------------------------------------
SectionTime::SectionTime() : frames(0) {
    // There are 75 frames per second, 60 seconds per minute, and 60 minutes per hour
    // so the maximum number of frames is 75 * 60 * 60 = 270000
    if (frames >= 270000) {
        qFatal("SectionTime::SectionTime(): Invalid frame count of %d", frames);
    }
} 

SectionTime::SectionTime(int32_t _frames) : frames(_frames) {
    if (frames >= 270000) {
        qFatal("SectionTime::SectionTime(): Invalid frame count of %d", frames);
    }
}

SectionTime::SectionTime(uint8_t _minutes, uint8_t _seconds, uint8_t _frames) {
    set_time(_minutes, _seconds, _frames);
}

void SectionTime::set_frames(int32_t _frames) {
    if (frames < 0 || frames >= 270000) {
        qFatal("SectionTime::SectionTime(): Invalid frame count of %d", frames);
    }

    frames = _frames;
}

void SectionTime::set_time(uint8_t _minutes, uint8_t _seconds, uint8_t _frames) {
    // Set the time in minutes, seconds, and frames

    // Ensure the time is sane
    if (_minutes >= 60) {
        _minutes = 59;
        qDebug().nospace() << "SectionTime::set_time(): Invalid minutes value " << _minutes << ", setting to 59";
    }
    if (_seconds >= 60) {
        _seconds = 59;
        qDebug().nospace() << "SectionTime::set_time(): Invalid seconds value " << _seconds << ", setting to 59";
    }
    if (_frames >= 75) {
        _frames = 74;
        qDebug().nospace() << "SectionTime::set_time(): Invalid frames value " << _frames << ", setting to 74";
    }

    frames = (_minutes * 60 + _seconds) * 75 + _frames;
}

QString SectionTime::to_string() const {
    // Return the time in the format MM:SS:FF
    return QString("%1:%2:%3").arg(frames / (75 * 60), 2, 10, QChar('0')).arg((frames / 75) % 60, 2, 10, QChar('0')).arg(frames % 75, 2, 10, QChar('0'));
}

QByteArray SectionTime::to_bcd() {
    // Return 3 bytes of BCD data representing the time as MM:SS:FF
    QByteArray bcd;

    uint32_t mins = frames / (75 * 60);
    uint32_t secs = (frames / 75) % 60;
    uint32_t frms = frames % 75;

    bcd.append(int_to_bcd(mins));
    bcd.append(int_to_bcd(secs));
    bcd.append(int_to_bcd(frms));
    
    return bcd;
}

uint8_t SectionTime::int_to_bcd(uint32_t value) {
    if (value > 99) {
        qFatal("SectionTime::int_to_bcd(): Value must be in the range 0 to 99.");
    }

    uint16_t bcd = 0;
    uint16_t factor = 1;

    while (value > 0) {
        bcd += (value % 10) * factor;
        value /= 10;
        factor *= 16;
    }

    // Ensure the result is always 1 byte (00-99)
    return bcd & 0xFF;
}

// Section metadata class -----------------------------------------------------------------------------------------------
void SectionMetadata::set_section_type(SectionType _section_type) {
    section_type = _section_type;

    // Ensure track number is sane
    if (section_type == SectionType::LEAD_IN) track_number = 0;
    if (section_type == SectionType::LEAD_OUT) track_number = 0;
    if ((section_type == SectionType::USER_DATA) && (track_number < 1 || track_number > 98)) {
        track_number = 1;
    }
}

void SectionMetadata::set_track_number(uint8_t _track_number) {
    track_number = _track_number;

    // Ensure track number is sane
    if (section_type == SectionType::LEAD_IN) track_number = 0;
    if (section_type == SectionType::LEAD_OUT) track_number = 0;
    if ((section_type == SectionType::USER_DATA) && (track_number < 1 || track_number > 98)) {
        track_number = 1;
    }
}