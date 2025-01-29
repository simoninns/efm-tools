/************************************************************************

    frame.cpp

    EFM-library - EFM Frame type classes
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

#include "frame.h"
#include <QVector>
#include <QDebug>

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

// Set the data for the frame, ensuring it matches the frame size
void Frame::set_data(const QVector<uint8_t>& data) {
    if (data.size() != get_frame_size()) {
        qFatal("Frame::set_data(): Data size of %d does not match frame size of %d", data.size(), get_frame_size());
    }
    frame_data = data;
}

// Get the data for the frame, returning a zero-filled vector if empty
QVector<uint8_t> Frame::get_data() const {
    if (frame_data.isEmpty()) {
        qDebug() << "Frame::get_data(): Frame is empty, returning zero-filled vector";
        return QVector<uint8_t>(get_frame_size(), 0);
    }
    return frame_data;
}

// Check if the frame is full (i.e., has data)
bool Frame::is_full() const {
    return !frame_data.isEmpty();
}

// Check if the frame is empty (i.e., has no data)
bool Frame::is_empty() const {
    return frame_data.isEmpty();
}

// Constructor for Data24, initializes data to the frame size
Data24::Data24() {
    frame_data.resize(get_frame_size());

    // Set defaults
    frame_type = FrameType::USER_DATA;
    frame_time.set_min(0);
    frame_time.set_sec(0);
    frame_time.set_frame(0);
}

// We override the set_data function to ensure the data is 24 bytes
// since it's possible to have less than 24 bytes of data
void Data24::set_data(const QVector<uint8_t>& data) {
    frame_data = data;

    // If there are less than 24 bytes, pad data with zeros to 24 bytes
    if (frame_data.size() < 24) {
        frame_data.resize(24);
        for (int i = frame_data.size(); i < 24; ++i) {
            frame_data[i] = 0;
        }
    }
}

// Get the frame size for Data24
int Data24::get_frame_size() const {
    return 24;
}

void Data24::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "Data24 data:" << dataString.trimmed() << "-" << frame_time.to_string() << "Track:" << track_number;
}

void Data24::set_frame_type(FrameType _frame_type) {
    frame_type = _frame_type;
}

FrameType Data24::get_frame_type() const {
    return frame_type;
}

void Data24::set_frame_time(const FrameTime& _frame_time) {
    frame_time = _frame_time;
}

FrameTime Data24::get_frame_time() const {
    return frame_time;
}

void Data24::set_track_number(uint8_t _track_number) {
    if (_track_number > 99) {
        qFatal("Data24::set_track_number(): Invalid track number of %d", _track_number);
    }
    track_number = _track_number;
}

uint8_t Data24::get_track_number() const {
    return track_number;
}

// Constructor for F1Frame, initializes data to the frame size
F1Frame::F1Frame() {
    frame_data.resize(get_frame_size());

    // Set defaults
    frame_type = FrameType::USER_DATA;
    frame_time.set_min(0);
    frame_time.set_sec(0);
    frame_time.set_frame(0);
}

// Get the frame size for F1Frame
int F1Frame::get_frame_size() const {
    return 24;
}

void F1Frame::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        if (frame_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
        }
    }
    qInfo().noquote() << "F1Frame data:" << dataString.trimmed() << "-" << frame_time.to_string() << "Track:" << track_number;
}

void F1Frame::set_frame_type(FrameType _frame_type) {
    frame_type = _frame_type;
}

FrameType F1Frame::get_frame_type() const {
    return frame_type;
}

void F1Frame::set_frame_time(const FrameTime& _frame_time) {
    frame_time = _frame_time;
}

FrameTime F1Frame::get_frame_time() const {
    return frame_time;
}

void F1Frame::set_track_number(uint8_t _track_number) {
    if (_track_number > 99) {
        qFatal("F1Frame::set_track_number(): Invalid track number of %d", _track_number);
    }
    track_number = _track_number;
}

uint8_t F1Frame::get_track_number() const {
    return track_number;
}

// Set the error data for the frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void F1Frame::set_error_data(const QVector<uint8_t>& error_data) {
    if (error_data.size() != get_frame_size()) {
        qFatal("Frame::set_error_data(): Error sata size of %d does not match frame size of %d", error_data.size(), get_frame_size());
    }
    frame_error_data = error_data;
}

// Get the error_data for the frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<uint8_t> F1Frame::get_error_data() const {
    if (frame_error_data.isEmpty()) {
        qDebug() << "Frame::get_error_data(): Error frame is empty, returning zero-filled vector";
        return QVector<uint8_t>(get_frame_size(), 0);
    }
    return frame_data;
}

// Constructor for F2Frame, initializes data to the frame size
F2Frame::F2Frame() {
    frame_data.resize(get_frame_size());

    // Set defaults
    frame_type = FrameType::USER_DATA;
    frame_time.set_min(0);
    frame_time.set_sec(0);
    frame_time.set_frame(0);
}

// Get the frame size for F2Frame
int F2Frame::get_frame_size() const {
    return 32;
}

void F2Frame::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "F2Frame data:" << dataString.trimmed() << "-" << frame_time.to_string() << "Track:" << track_number;
}

void F2Frame::set_frame_type(FrameType _frame_type) {
    frame_type = _frame_type;
}

FrameType F2Frame::get_frame_type() const {
    return frame_type;
}

void F2Frame::set_frame_time(const FrameTime& _frame_time) {
    frame_time = _frame_time;
}

FrameTime F2Frame::get_frame_time() const {
    return frame_time;
}

void F2Frame::set_track_number(uint8_t _track_number) {
    if (_track_number > 99) {
        qFatal("F2Frame::set_track_number(): Invalid track number of %d", _track_number);
    }
    track_number = _track_number;
}

uint8_t F2Frame::get_track_number() const {
    return track_number;
}

// Constructor for F3Frame, initializes data to the frame size
F3Frame::F3Frame() {
    frame_data.resize(get_frame_size());
    subcode_byte = 0;
    f3_frame_type = SUBCODE;
}

// Get the frame size for F3Frame
int F3Frame::get_frame_size() const {
    return 32;
}

// Set the frame type as subcode and set the subcode value
void F3Frame::set_frame_type_as_subcode(uint8_t subcode_value) {
    f3_frame_type = SUBCODE;
    subcode_byte = subcode_value;
}

// Set the frame type as sync0 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync0() {
    f3_frame_type = SYNC0;
    subcode_byte = 0;
}

// Set the frame type as sync1 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync1() {
    f3_frame_type = SYNC1;
    subcode_byte = 0;
}

// Get the F3 frame type
F3Frame::F3FrameType F3Frame::get_f3_frame_type() const {
    return f3_frame_type;
}

// Get the F3 frame type as a QString
QString F3Frame::get_f3_frame_type_as_string() const {
    switch (f3_frame_type) {
        case SUBCODE:
            return "SUBCODE";
        case SYNC0:
            return "SYNC0";
        case SYNC1:
            return "SYNC1";
        default:
            return "UNKNOWN";
    }
}

// Get the subcode value
uint8_t F3Frame::get_subcode_byte() const {
    return subcode_byte;
}

void F3Frame::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "F3Frame data:" << dataString.trimmed();
}
