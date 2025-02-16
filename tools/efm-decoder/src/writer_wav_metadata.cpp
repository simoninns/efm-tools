/************************************************************************

    writer_wav_metadata.cpp

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

#include "writer_wav_metadata.h"

// This writer class writes metadata about audio data to a file in WAV format
// This is used when the output is stereo audio data

WriterWavMetadata::WriterWavMetadata() { }

WriterWavMetadata::~WriterWavMetadata()
{
    if (file.isOpen()) {
        file.close();
    }
}

bool WriterWavMetadata::open(const QString &filename)
{
    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterWavMetadata::open() - Could not open file" << filename
                    << "for writing";
        return false;
    }
    qDebug() << "WriterWavMetadata::open() - Opened file" << filename << "for data writing";

    // Write the metadata header
    file.write("efm-decode - WAV Metadata\n");
    file.write("Format: Absolute time, track number, track time, error list (if present)\n");
    file.write("Each time-stamp represents one section with 12*98 samples (positions 0 to 1175)\n");
    file.write("Each section is 1/75th of a second - so timestamps are MM:DD:section 0-74\n");

    return true;
}

void WriterWavMetadata::write(const AudioSection &audio_section)
{
    if (!file.isOpen()) {
        qCritical() << "WriterWavMetadata::write() - File is not open for writing";
        return;
    }

    // Write a metadata entry for the section
    QString metadata = audio_section.metadata.get_absolute_section_time().to_string() + ","
            + QString::number(audio_section.metadata.get_track_number()) + ","
            + audio_section.metadata.get_section_time().to_string();

    // Each Audio section contains 98 frames that we need to write metadata for in the output file
    QString section_error_list;
    for (int index = 0; index < 98; index++) {
        Audio audio = audio_section.get_frame(index);
        QVector<int16_t> errors = audio.get_error_data();

        // Are there any errors in this section?
        if (errors.contains(1)) {
            // If the frame contains errors we need to add it to the metadata
            // the position of the error is the frame sample number * current frame
            // i.e. 0 to 1175
            for (int i = 0; i < errors.size(); i++) {
                // Each frame is 12 samples, each section is 98 frames
                int32_t error_location_in_section = i + (index * 12);
                if (errors[i] != 0)
                    section_error_list += "," + QString::number(error_location_in_section);
            }
        }
    }

    // Write the metadata to the metadata file
    metadata += section_error_list;
    metadata += "\n";
    file.write(metadata.toUtf8());
}

void WriterWavMetadata::close()
{
    if (!file.isOpen()) {
        return;
    }

    file.close();
    qDebug() << "WriterWavMetadata::close(): Closed the WAV metadata file" << file.fileName();
}

int64_t WriterWavMetadata::size()
{
    if (file.isOpen()) {
        return file.size();
    }

    return 0;
}