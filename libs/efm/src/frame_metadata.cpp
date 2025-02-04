/************************************************************************

    frame_metadata.cpp

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

#include "frame_metadata.h"

// Frame time class ---------------------------------------------------------------------------------------------------
void FrameTime::set_min(uint8_t _min) {
    if (_min > 59) {
        qFatal("FrameTime::set_min(): Invalid minute value of %d", _min);
    }
    min = _min;
}

void FrameTime::set_sec(uint8_t _sec) {
    if (_sec > 59) {
        qFatal("FrameTime::set_sec(): Invalid second value of %d", _sec);
    }
    sec = _sec;
}

void FrameTime::set_frame(uint8_t _frame) {
    if (_frame > 74) {
        qFatal("FrameTime::set_frame(): Invalid frame value of %d", _frame);
    }
    frame = _frame;
}

QByteArray FrameTime::to_bcd() const {
    QByteArray bcd;
    bcd.append((min / 10) << 4 | (min % 10));
    bcd.append((sec / 10) << 4 | (sec % 10));
    bcd.append((frame / 10) << 4 | (frame % 10));
    return bcd;
}

// Increment the frame time by one frame, wrapping around as needed
void FrameTime::increment_frame() {
    frame++;
    if (frame > 74) {
        frame = 0;
        sec++;
        if (sec > 59) {
            sec = 0;
            min++;
            if (min > 59) {
                min = 0;
                qWarning("FrameTime::increment_frame(): Frame time has exceeded 60 minutes - wrapping around");
            }
        }
    }
}

void FrameTime::set_time_in_frames(int32_t time_in_frames) {
    if (time_in_frames < 0) {
        qFatal("FrameTime::set_time_in_frames(): Negative time in frames");
    }

    min = time_in_frames / (60 * 75);
    sec = (time_in_frames / 75) % 60;
    frame = time_in_frames % 75;
}

bool FrameTime::operator==(const FrameTime& other) const {
    return min == other.min && sec == other.sec && frame == other.frame;
}

bool FrameTime::operator!=(const FrameTime& other) const {
    return !(*this == other);
}

bool FrameTime::operator<(const FrameTime& other) const {
    if (min != other.min) return min < other.min;
    if (sec != other.sec) return sec < other.sec;
    return frame < other.frame;
}

bool FrameTime::operator>(const FrameTime& other) const {
    return other < *this;
}

FrameTime FrameTime::operator+(const FrameTime& other) const {
    FrameTime result = *this;
    result.frame += other.frame;
    result.sec += other.sec + result.frame / 75;
    result.frame %= 75;
    result.min += other.min + result.sec / 60;
    result.sec %= 60;
    result.min %= 60;
    return result;
}

FrameTime FrameTime::operator-(const FrameTime& other) const {
    FrameTime result = *this;
    int totalFrames1 = (min * 60 + sec) * 75 + frame;
    int totalFrames2 = (other.min * 60 + other.sec) * 75 + other.frame;
    int diffFrames = totalFrames1 - totalFrames2;

    if (diffFrames < 0) {
        qFatal("FrameTime::operator-(): Resulting frame time is negative");
    }

    result.min = (diffFrames / 75) / 60;
    result.sec = (diffFrames / 75) % 60;
    result.frame = diffFrames % 75;
    return result;
}

QString FrameTime::to_string() const {
    return QString("%1:%2.%3").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(frame, 2, 10, QChar('0'));
}

// Frame metadata class -----------------------------------------------------------------------------------------------
void FrameMetadata::set_frame_type(FrameType _frame_type) {
    frame_type = _frame_type;

    // Ensure track number is sane
    if (frame_type == FrameType::LEAD_IN) track_number = 0;
    if (frame_type == FrameType::LEAD_OUT) track_number = 0;
    if ((frame_type == FrameType::USER_DATA) && (track_number < 1 || track_number > 98)) {
        track_number = 1;
    }
}

void FrameMetadata::set_track_number(uint8_t _track_number) {
    track_number = _track_number;

    // Ensure track number is sane
    if (frame_type == FrameType::LEAD_IN) track_number = 0;
    if (frame_type == FrameType::LEAD_OUT) track_number = 0;
    if ((frame_type == FrameType::USER_DATA) && (track_number < 1 || track_number > 98)) {
        track_number = 1;
    }
}