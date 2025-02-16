/************************************************************************

    audio.h

    EFM-library - Audio frame type class
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

#include "audio.h"

// Set the data for the audio frame, ensuring it matches the frame size
void Audio::set_data(const QVector<int16_t> &data)
{
    if (data.size() != 12) {
        qFatal("Audio::set_data(): Data size of %d does not match frame size of %d", data.size(),
               12);
    }
    audio_data = data;
}

// Get the data for the audio frame, returning a zero-filled vector if empty
QVector<int16_t> Audio::get_data() const
{
    if (audio_data.isEmpty()) {
        qDebug() << "Audio::get_data(): Frame is empty, returning zero-filled vector";
        return QVector<int16_t>(12, 0);
    }
    return audio_data;
}

// Set the error data for the audio frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Audio::set_error_data(const QVector<int16_t> &error_data)
{
    if (error_data.size() != 12) {
        qFatal("Audio::set_error_data(): Error sata size of %d does not match frame size of %d",
               error_data.size(), 12);
    }
    audio_error_data = error_data;
}

// Get the error_data for the audio frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<int16_t> Audio::get_error_data() const
{
    if (audio_data.isEmpty()) {
        qDebug() << "Audio::get_error_data(): Error frame is empty, returning zero-filled vector";
        return QVector<int16_t>(12, 0);
    }
    return audio_error_data;
}

// Count the number of errors in the audio frame
uint32_t Audio::count_errors() const
{
    uint32_t error_count = 0;
    for (int i = 0; i < 12; ++i) {
        if (audio_error_data[i] == 1) {
            error_count++;
        }
    }
    return error_count;
}

// Check if the audio frame is full (i.e., has data)
bool Audio::is_full() const
{
    return !audio_data.isEmpty();
}

// Check if the audio frame is empty (i.e., has no data)
bool Audio::is_empty() const
{
    return audio_data.isEmpty();
}

// Show the frame data and errors in debug
void Audio::show_data()
{
    QString dataString;
    bool has_error = false;
    for (int i = 0; i < audio_data.size(); ++i) {
        if (audio_error_data[i] == 0) {
            dataString.append(QString("%1 ").arg(audio_data[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            has_error = true;
        }
    }
    if (has_error) {
        qInfo().noquote() << "Audio:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "Audio:" << dataString.trimmed();
    }
}

int Audio::get_frame_size() const
{
    return 12;
}