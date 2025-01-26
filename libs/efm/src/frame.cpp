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

// Constructor for F1Frame, initializes data to the frame size
F1Frame::F1Frame() {
    frame_data.resize(get_frame_size());
}

// Get the frame size for F1Frame
int F1Frame::get_frame_size() const {
    return 24;
}

void F1Frame::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "F1Frame data:" << dataString.trimmed();
}

// Constructor for F2Frame, initializes data to the frame size
F2Frame::F2Frame() {
    frame_data.resize(get_frame_size());
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
    qInfo().noquote() << "F2Frame data:" << dataString.trimmed();
}

// Constructor for F3Frame, initializes data to the frame size
F3Frame::F3Frame() {
    frame_data.resize(get_frame_size());
    subcode = 0;
    frame_type = SUBCODE;
}

// Get the frame size for F3Frame
int F3Frame::get_frame_size() const {
    return 32;
}

// Set the frame type as subcode and set the subcode value
void F3Frame::set_frame_type_as_subcode(uint8_t subcode_value) {
    frame_type = SUBCODE;
    subcode = subcode_value;
}

// Set the frame type as sync0 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync0() {
    frame_type = SYNC0;
    subcode = 0;
}

// Set the frame type as sync1 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync1() {
    frame_type = SYNC1;
    subcode = 0;
}

// Get the frame type
F3Frame::FrameType F3Frame::get_frame_type() const {
    return frame_type;
}

// Get the subcode value
uint8_t F3Frame::get_subcode() const {
    return subcode;
}

void F3Frame::show_data() {
    QString dataString;
    for (int i = 0; i < frame_data.size(); ++i) {
        dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "F3Frame data:" << dataString.trimmed();
}
