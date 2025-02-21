/************************************************************************

    writer_wav_metadata.cpp

    efm-decoder-audio - EFM Data24 to Audio decoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    This application is free software: you can redistribute it and/or
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

// This writer class writes metadata about audio data to a file
// This is used when the output is stereo audio data

WriterWavMetadata::WriterWavMetadata()
{}

WriterWavMetadata::~WriterWavMetadata()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool WriterWavMetadata::open(const QString &filename)
{
    m_file.setFileName(filename);
    if (!m_file.open(QIODevice::WriteOnly)) {
        qCritical() << "WriterWavMetadata::open() - Could not open file" << filename << "for writing";
        return false;
    }
    qDebug() << "WriterWavMetadata::open() - Opened file" << filename << "for data writing";

    // Write the metadata header
    m_file.write("efm-decode - WAV Metadata\n");
    m_file.write("Format: Absolute time, track number, track time, error list (if present)\n");
    m_file.write("Each time-stamp represents one section with 12*98 samples (positions 0 to 1175)\n");
    m_file.write("Each section is 1/75th of a second - so timestamps are MM:DD:section 0-74\n");

    return true;
}

void WriterWavMetadata::write(const AudioSection &audioSection)
{
    if (!m_file.isOpen()) {
        qCritical() << "WriterWavMetadata::write() - File is not open for writing";
        return;
    }

    // Write a metadata entry for the section
    QString metadata = audioSection.metadata.absoluteSectionTime().toString() + ","
            + QString::number(audioSection.metadata.trackNumber()) + ","
            + audioSection.metadata.sectionTime().toString();

    // Each Audio section contains 98 frames that we need to write metadata for in the output file
    QString sectionErrorList;
    for (int index = 0; index < 98; index++) {
        Audio audio = audioSection.frame(index);
        QVector<int16_t> errors = audio.errorData();

        // Are there any errors in this section?
        if (errors.contains(1)) {
            // If the frame contains errors we need to add it to the metadata
            // the position of the error is the frame sample number * current frame
            // i.e. 0 to 1175
            for (int i = 0; i < errors.size(); i++) {
                // Each frame is 12 samples, each section is 98 frames
                int32_t errorLocationInSection = i + (index * 12);
                if (errors[i] != 0)
                    sectionErrorList += "," + QString::number(errorLocationInSection);
            }
        }
    }

    // Write the metadata to the metadata file
    metadata += sectionErrorList;
    metadata += "\n";
    m_file.write(metadata.toUtf8());
}

void WriterWavMetadata::close()
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.close();
    qDebug() << "WriterWavMetadata::close(): Closed the WAV metadata file" << m_file.fileName();
}

qint64 WriterWavMetadata::size() const
{
    if (m_file.isOpen()) {
        return m_file.size();
    }

    return 0;
}