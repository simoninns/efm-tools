/************************************************************************

    writer_wav.cpp

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

#include "writer_wav.h"

// This writer class writes audio data to a file in WAV format
// This is used when the output is stereo audio data

WriterWav::WriterWav() {
}

WriterWav::~WriterWav() {
    if (file.isOpen()) {
        file.close();
    }
}

bool WriterWav::open(const QString &filename) {
    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterWav::open() - Could not open file" << filename << "for writing";
        return false;
    }
    qDebug() << "WriterWav::open() - Opened file" << filename << "for data writing";

    // Add 44 bytes of blank header data to the file
    // (we will fill this in later once we know the size of the data)
    QByteArray header(44, 0);
    file.write(header);

    return true;
}

void WriterWav::write(const AudioSection &audio_section) {
    if (!file.isOpen()) {
        qCritical() << "WriterWav::write() - File is not open for writing";
        return;
    }

    // Each Audio section contains 98 frames that we need to write to the output file
    for (int index = 0; index < 98; index++) {
        Audio audio = audio_section.get_frame(index);
        file.write(reinterpret_cast<const char*>(audio.get_data().data()), audio.get_frame_size() * sizeof(int16_t));
    }
}

void WriterWav::close() {
    if (!file.isOpen()) {
        return;
    }

    // Fill out the WAV header
    qDebug() << "WriterWav::close(): Filling out the WAV header before closing the wav file";

    // WAV file header
    struct WAVHeader {
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunkSize;
        char wave[4] = {'W', 'A', 'V', 'E'};
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t subchunk1Size = 16; // PCM
        uint16_t audioFormat = 1; // PCM
        uint16_t numChannels = 2; // Stereo
        uint32_t sampleRate = 44100; // 44.1kHz
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 16; // 16 bits
        char data[4] = {'d', 'a', 't', 'a'};
        uint32_t subchunk2Size;
    };

    WAVHeader header;
    header.chunkSize = 36 + file.size();
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.subchunk2Size = file.size();

    // Move to the beginning of the file to write the header
    file.seek(0);
    file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

    // Now close the file
    file.close();
    qDebug() << "WriterWav::close(): Closed the WAV file" << file.fileName(); 
}

int32_t WriterWav::size() {
    if (file.isOpen()) {
        return file.size();
    }

    return 0;
}