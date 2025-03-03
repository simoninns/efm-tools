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
    m_inErrorRange(false)
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

    SectionMetadata metadata = audioSection.metadata;
    SectionTime absoluteSectionTime = metadata.absoluteSectionTime();

    // Do we have the start time already?
    if (!m_haveStartTime) {
        m_startTime = absoluteSectionTime;
        m_haveStartTime = true;
    }

    // Get the relative time from the start time
    SectionTime relativeSectionTime = absoluteSectionTime - m_startTime;

    for (int subSection = 0; subSection < 98; ++subSection) {
        Audio audio = audioSection.frame(subSection);
        QVector<qint16> audioData = audio.data();
        QVector<bool> errors = audio.errorData();
        
        for (int sampleOffset = 0; sampleOffset < 12; sampleOffset += 2) {
            bool hasError = errors.at(sampleOffset) || errors.at(sampleOffset+1);
            
            if (hasError && !m_inErrorRange) {
                // Start of new error range
                m_rangeStart = convertToAudacityTimestamp(relativeSectionTime.minutes(), relativeSectionTime.seconds(),
                relativeSectionTime.frameNumber(), subSection, sampleOffset);
                m_inErrorRange = true;
            } else if (!hasError && m_inErrorRange) {
                // End of error range
                QString rangeEnd;
                if (sampleOffset == 0) {
                    // Handle wrap to previous subsection
                    if (subSection > 0) {
                        rangeEnd = convertToAudacityTimestamp(relativeSectionTime.minutes(), relativeSectionTime.seconds(),
                            relativeSectionTime.frameNumber(), subSection - 1, 11);
                    } else {
                        // If we're at the first subsection, just use the current position
                        rangeEnd = convertToAudacityTimestamp(relativeSectionTime.minutes(), relativeSectionTime.seconds(),
                            relativeSectionTime.frameNumber(), subSection, sampleOffset);
                    }
                } else {
                    rangeEnd = convertToAudacityTimestamp(relativeSectionTime.minutes(), relativeSectionTime.seconds(),
                        relativeSectionTime.frameNumber(), subSection, sampleOffset - 1);
                }

                QString sampleTimeStamp = QString("%1").arg(absoluteSectionTime.toString());
                QString outputString = m_rangeStart + "\t" + rangeEnd + "\tError: " + sampleTimeStamp + "\n";
                m_file.write(outputString.toUtf8());
                m_inErrorRange = false;
            }
        }
    }
}

void WriterWavMetadata::close()
{
    if (!m_file.isOpen()) {
        return;
    }

    // If we're still in an error range when closing, write the final range
    if (m_inErrorRange) {
        QString outputString = m_rangeStart + "\t" + m_rangeStart + "\tError: Incomplete range\n";
        m_file.write(outputString.toUtf8());
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
    total_seconds += (frames) / FRAME_RATE;
    
    // Convert subsection to fractional time
    total_seconds += subsection / (FRAME_RATE * SUBSECTIONS_PER_FRAME);
    
    // Convert sample to fractional time
    total_seconds += (sample/2) / (FRAME_RATE * SUBSECTIONS_PER_FRAME * SAMPLES_PER_SUBSECTION);

    // Format the output string with 6 decimal places
    return QString::asprintf("%.6f", total_seconds);
}