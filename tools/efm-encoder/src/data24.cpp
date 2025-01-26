/************************************************************************

    data24.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QVector>
#include <QDebug>

#include "data24.h"

Data24::Data24(const QString _filename, bool _is_wav) : filename(_filename), file(_filename), is_wav(_is_wav) {
    ready = false;

    // Are we reading a WAV file or raw data?
    if (is_wav) {
        open_wav();
    } else {
        open_raw();
    }
}

Data24::~Data24() {
    file.close();
    ready = false;
}

bool Data24::open_raw() {
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Data24::open_raw() - Failed to open file:" << filename;
        return false;
    }

    ready = true;
    return true;
}

bool Data24::open_wav() {
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Data24::open_wav() - Failed to open file:" << filename;
        return false;
    }

    // Read the WAV header
    QByteArray header = file.read(44);
    if (header.size() != 44) {
        qWarning() << "Data24::open_wav() - Failed to read WAV header from file:" << filename;
        return false;
    }

    // Check the WAV header for the correct format
    if (header.mid(0, 4) != "RIFF" || header.mid(8, 4) != "WAVE") {
        qWarning() << "Data24::open_wav() - Invalid WAV file format:" << filename;
        return false;
    }

    // Check the sampling rate (44.1KHz)
    quint32 sampleRate = *reinterpret_cast<const quint32*>(header.mid(24, 4).constData());
    if (sampleRate != 44100) {
        qWarning() << "Data24::open_wav() - Unsupported sample rate:" << sampleRate << "in file:" << filename;
        return false;
    }

    // Check the bit depth (16 bits)
    quint16 bitDepth = *reinterpret_cast<const quint16*>(header.mid(34, 2).constData());
    if (bitDepth != 16) {
        qWarning() << "Data24::open_wav() - Unsupported bit depth:" << bitDepth << "in file:" << filename;
        return false;
    }

    // Check the number of channels (stereo)
    quint16 numChannels = *reinterpret_cast<const quint16*>(header.mid(22, 2).constData());
    if (numChannels != 2) {
        qWarning() << "Data24::open_wav() - Unsupported number of channels:" << numChannels << "in file:" << filename;
        return false;
    }

    ready = true;
    return true;
}

QVector<uint8_t> Data24::read() {
    // Read the 24 bytes of data from the file
    QVector<uint8_t> data(24);
    qint64 bytesRead = file.read(reinterpret_cast<char*>(data.data()), 24);

    // If no bytes were read, return an empty vector
    if (bytesRead == 0) {
        ready = false;
        last_frame.clear();
        return QVector<uint8_t>();
    }

    // If there are less than 24 bytes read, pad data with zeros to 24 bytes
    if (bytesRead < 24) {
        data.resize(24);
        for (int i = bytesRead; i < 24; ++i) {
            data[i] = 0;
        }
    }

    last_frame = data;
    return data;
}

bool Data24::is_ready() const {
    return ready;
}

void Data24::show_data() {
    if (last_frame.isEmpty()) {
        qInfo() << "Input data: No data available";
        return;
    }

    QString dataString;
    for (int i = 0; i < last_frame.size(); ++i) {
        dataString.append(QString("%1 ").arg(last_frame[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "Input data:" << dataString.trimmed();
}