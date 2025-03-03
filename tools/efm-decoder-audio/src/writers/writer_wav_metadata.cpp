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

WriterWavMetadata::WriterWavMetadata() :
    m_currentTrack(-1)
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

    return true;
}

void WriterWavMetadata::write(const AudioSection &audioSection)
{
    if (!m_file.isOpen()) {
        qCritical() << "WriterWavMetadata::write() - File is not open for writing";
        return;
    }

    // Write an Audacity text label entry for:
    //   1. The start of a new track
    //   2. Any errors in the audio data, errors can be:
    //      - Point errors (a single sample)
    //      - Region errors (a continuous range of samples)

    // Sample positions are denoted by the timeline position in the audio data
    // Start time <TAB> End time <TAB> Label text <NEWLINE>
    // The start and end times are in seconds and milliseconds, so sample 106472627 (L/R) is 2414.345283 seconds

    SectionMetadata metadata = audioSection.metadata;
    SectionTime sectionTime = metadata.sectionTime();

    for (int subSection = 0; subSection < 98; ++subSection) {
        Audio audio = audioSection.frame(subSection);
        QVector<qint16> audioData = audio.data();
        QVector<bool> errors = audio.errorData();
        for (int sampleOffset = 0; sampleOffset < 12; sampleOffset += 2) {
            if (errors.at(sampleOffset) || errors.at(sampleOffset+1)) {
                QString timeStamp = convertToAudacityTimestamp(sectionTime.minutes(), sectionTime.seconds(),
                    sectionTime.frameNumber(), subSection, sampleOffset);

                QString outputString = timeStamp + "\t" + timeStamp + "\tError\n";

                m_file.write(outputString.toUtf8());
            }
        }
    }


    // // Each Audio section contains 98 frames that we need to write metadata for in the output file
    // QString sectionErrorList;
    // for (int index = 0; index < 98; index++) {
    //     Audio audio = audioSection.frame(index);
    //     QVector<bool> errors = audio.errorData();

    //     // Are there any errors in this section?
    //     if (errors.contains(true)) {
    //         // If the frame contains errors we need to add it to the metadata
    //         // the position of the error is the frame sample number * current frame
    //         // i.e. 0 to 1175
    //         for (int i = 0; i < errors.size(); i++) {
    //             // Each frame is 12 samples, each section is 98 frames
    //             int32_t errorLocationInSection = i + (index * 12);
    //             if (errors[i] == true)
    //                 sectionErrorList += "," + QString::number(errorLocationInSection);
    //         }
    //     }
    // }

    // // Only write metadata is the section contains errors or the track number has changed
    // if (!sectionErrorList.isEmpty() || m_currentTrack != audioSection.metadata.trackNumber()) {
    //     // Write a metadata entry for the section
    //     QString frameEntry = audioSection.metadata.absoluteSectionTime().toString() + ","
    //             + QString::number(audioSection.metadata.trackNumber()) + ","
    //             + audioSection.metadata.sectionTime().toString();

    //     // Write the metadata to the metadata file
    //     frameEntry += sectionErrorList;
    //     frameEntry += "\n";
    //     m_file.write(frameEntry.toUtf8());
    // }

    // // Save the current track
    // m_currentTrack = audioSection.metadata.trackNumber();
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

QString WriterWavMetadata::convertToAudacityTimestamp(qint32 minutes, qint32 seconds, qint32 frames,
    qint32 subsection, qint32 sample)
{
    // Constants for calculations
    constexpr double FRAME_RATE = 75.0;      // 75 frames per second
    constexpr double SUBSECTIONS_PER_FRAME = 98.0; // 98 subsections per frame
    constexpr double SAMPLES_PER_SUBSECTION = 6.0; // 6 stereo samples per subsection

    // Convert minutes and seconds to total seconds
    double total_seconds = (minutes * 60.0) + seconds;
    
    // Convert frames to seconds
    total_seconds += (frames-1) / FRAME_RATE;
    
    // Convert subsection to fractional time
    total_seconds += subsection / (FRAME_RATE * SUBSECTIONS_PER_FRAME);
    
    // Convert sample to fractional time
    total_seconds += (sample/2) / (FRAME_RATE * SUBSECTIONS_PER_FRAME * SAMPLES_PER_SUBSECTION);

    // Format the output string with 6 decimal places
    return QString::asprintf("%.6f", total_seconds);
}