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

// Frame class
// --------------------------------------------------------------------------------------------------------

// Set the data for the frame, ensuring it matches the frame size
void Frame::set_data(const QVector<uint8_t> &data)
{
    if (data.size() != get_frame_size()) {
        qFatal("Frame::set_data(): Data size of %d does not match frame size of %d", data.size(),
               get_frame_size());
    }
    frame_data = data;
}

// Get the data for the frame, returning a zero-filled vector if empty
QVector<uint8_t> Frame::get_data() const
{
    if (frame_data.isEmpty()) {
        qDebug() << "Frame::get_data(): Frame is empty, returning zero-filled vector";
        return QVector<uint8_t>(get_frame_size(), 0);
    }
    return frame_data;
}

// Set the error data for the frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Frame::set_error_data(const QVector<uint8_t> &error_data)
{
    if (error_data.size() != get_frame_size()) {
        qFatal("Frame::set_error_data(): Error sata size of %d does not match frame size of %d",
               error_data.size(), get_frame_size());
    }
    frame_error_data = error_data;
}

// Get the error_data for the frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<uint8_t> Frame::get_error_data() const
{
    if (frame_data.isEmpty()) {
        qDebug() << "Frame::get_error_data(): Error frame is empty, returning zero-filled vector";
        return QVector<uint8_t>(get_frame_size(), 0);
    }
    return frame_error_data;
}

// Count the number of errors in the frame
uint32_t Frame::count_errors() const
{
    uint32_t error_count = 0;
    for (int i = 0; i < frame_error_data.size(); ++i) {
        if (frame_error_data[i] == 1) {
            error_count++;
        }
    }
    return error_count;
}

// Check if the frame is full (i.e., has data)
bool Frame::is_full() const
{
    return !frame_data.isEmpty();
}

// Check if the frame is empty (i.e., has no data)
bool Frame::is_empty() const
{
    return frame_data.isEmpty();
}

// Constructor for Data24, initializes data to the frame size
Data24::Data24()
{
    frame_data.resize(get_frame_size());
    frame_error_data.resize(get_frame_size());
    frame_error_data.fill(0);
}

// We override the set_data function to ensure the data is 24 bytes
// since it's possible to have less than 24 bytes of data
void Data24::set_data(const QVector<uint8_t> &data)
{
    frame_data = data;

    // If there are less than 24 bytes, pad data with zeros to 24 bytes
    if (frame_data.size() < 24) {
        frame_data.resize(24);
        for (int i = frame_data.size(); i < 24; ++i) {
            frame_data[i] = 0;
        }
    }
}

void Data24::set_error_data(const QVector<uint8_t> &error_data)
{
    frame_error_data = error_data;

    // If there are less than 24 bytes, pad data with zeros to 24 bytes
    if (frame_error_data.size() < 24) {
        frame_error_data.resize(24);
        for (int i = frame_error_data.size(); i < 24; ++i) {
            frame_error_data[i] = 0;
        }
    }
}

// Get the frame size for Data24
int Data24::get_frame_size() const
{
    return 24;
}

void Data24::show_data()
{
    QString dataString;
    bool has_error = false;
    for (int i = 0; i < frame_data.size(); ++i) {
        if (frame_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            has_error = true;
        }
    }
    if (has_error) {
        qInfo().noquote() << "Data24:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "Data24:" << dataString.trimmed();
    }
}

// Constructor for F1Frame, initializes data to the frame size
F1Frame::F1Frame()
{
    frame_data.resize(get_frame_size());
    frame_error_data.resize(get_frame_size());
    frame_error_data.fill(0);
}

// Get the frame size for F1Frame
int F1Frame::get_frame_size() const
{
    return 24;
}

void F1Frame::show_data()
{
    QString dataString;
    bool has_error = false;
    for (int i = 0; i < frame_data.size(); ++i) {
        if (frame_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            has_error = true;
        }
    }
    if (has_error) {
        qInfo().noquote() << "F1Frame:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "F1Frame:" << dataString.trimmed();
    }
}

// Constructor for F2Frame, initializes data to the frame size
F2Frame::F2Frame()
{
    frame_data.resize(get_frame_size());
    frame_error_data.resize(get_frame_size());
    frame_error_data.fill(0);
}

// Get the frame size for F2Frame
int F2Frame::get_frame_size() const
{
    return 32;
}

void F2Frame::show_data()
{
    QString dataString;
    bool has_error = false;
    for (int i = 0; i < frame_data.size(); ++i) {
        if (frame_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            has_error = true;
        }
    }
    if (has_error) {
        qInfo().noquote() << "F2Frame:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "F2Frame:" << dataString.trimmed();
    }
}

// Constructor for F3Frame, initializes data to the frame size
F3Frame::F3Frame()
{
    frame_data.resize(get_frame_size());
    subcode_byte = 0;
    f3_frame_type = SUBCODE;
}

// Get the frame size for F3Frame
int F3Frame::get_frame_size() const
{
    return 32;
}

// Set the frame type as subcode and set the subcode value
void F3Frame::set_frame_type_as_subcode(uint8_t subcode_value)
{
    f3_frame_type = SUBCODE;
    subcode_byte = subcode_value;
}

// Set the frame type as sync0 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync0()
{
    f3_frame_type = SYNC0;
    subcode_byte = 0;
}

// Set the frame type as sync1 and set the subcode value to 0
void F3Frame::set_frame_type_as_sync1()
{
    f3_frame_type = SYNC1;
    subcode_byte = 0;
}

// Get the F3 frame type
F3Frame::F3FrameType F3Frame::get_f3_frame_type() const
{
    return f3_frame_type;
}

// Get the F3 frame type as a QString
QString F3Frame::get_f3_frame_type_as_string() const
{
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
uint8_t F3Frame::get_subcode_byte() const
{
    return subcode_byte;
}

void F3Frame::show_data()
{
    QString dataString;
    bool has_error = false;
    for (int i = 0; i < frame_data.size(); ++i) {
        if (frame_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(frame_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            has_error = true;
        }
    }

    QString errorString;
    if (has_error)
        errorString = "ERROR";
    else
        errorString = "";

    if (f3_frame_type == SUBCODE) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed()
                          << " subcode:" << QString("0x%1").arg(subcode_byte, 2, 16, QChar('0'))
                          << errorString;
    } else if (f3_frame_type == SYNC0) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " SYNC0" << errorString;
    } else if (f3_frame_type == SYNC1) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " SYNC1" << errorString;
    } else {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " UNKNOWN" << errorString;
    }
}
