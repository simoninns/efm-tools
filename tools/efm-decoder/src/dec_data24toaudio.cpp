/************************************************************************

    dec_data24toaudio.cpp

    ld-efm-decoder - EFM data decoder
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

#include "dec_data24toaudio.h"

Data24ToAudio::Data24ToAudio()
    : m_startTime(SectionTime(59, 59, 74)),
      m_endTime(SectionTime(0, 0, 0)),
      m_invalidData24FramesCount(0),
      m_validData24FramesCount(0),
      m_invalidSamplesCount(0),
      m_validSamplesCount(0)
{}

void Data24ToAudio::pushSection(const Data24Section &data24Section)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(data24Section);

    // Process the queue
    processQueue();
}

AudioSection Data24ToAudio::popSection()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool Data24ToAudio::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void Data24ToAudio::processQueue()
{
    // Process the input buffer
    while (!m_inputBuffer.isEmpty()) {
        Data24Section data24Section = m_inputBuffer.dequeue();
        AudioSection audioSection;

        // Sanity check the Data24 section
        if (!data24Section.isComplete()) {
            qFatal("Data24ToAudio::processQueue - Data24 Section is not complete");
        }

        for (int index = 0; index < 98; ++index) {
            QVector<quint8> data24Data = data24Section.frame(index).getData();
            QVector<quint8> data24ErrorData = data24Section.frame(index).getErrorData();

            if (data24Section.frame(index).countErrors() != 0) {
                ++m_invalidData24FramesCount;
            } else {
                ++m_validData24FramesCount;
            }

            // Convert the 24 bytes of data into 12 16-bit audio samples
            QVector<qint16> audioData;
            QVector<qint16> audioErrorData;
            for (int i = 0; i < 24; i += 2) {
                qint16 sample = (data24Data[i + 1] << 8) | data24Data[i];
                audioData.append(sample);

                // Set an error flag if either byte of the sample is an error
                if (data24ErrorData[i + 1] == 1 || data24ErrorData[i] == 1) {
                    audioErrorData.append(1);
                    ++m_invalidSamplesCount;
                } else {
                    audioErrorData.append(0);
                    ++m_validSamplesCount;
                }
            }

            // Put the resulting data into an Audio frame and push it to the output buffer
            Audio audio;
            audio.setData(audioData);
            audio.setErrorData(audioErrorData);

            audioSection.pushFrame(audio);
        }

        audioSection.metadata = data24Section.metadata;

        if (audioSection.metadata.absoluteSectionTime() < m_startTime) {
            m_startTime = audioSection.metadata.absoluteSectionTime();
        }

        if (audioSection.metadata.absoluteSectionTime() >= m_endTime) {
            m_endTime = audioSection.metadata.absoluteSectionTime();
        }

        // Add the section to the output buffer
        m_outputBuffer.enqueue(audioSection);
    }
}

void Data24ToAudio::showStatistics()
{
    qInfo() << "Data24 to Audio statistics:";
    qInfo().nospace() << "  Data24 Frames:";
    qInfo().nospace() << "    Total Frames: "
                      << m_validData24FramesCount + m_invalidData24FramesCount;
    qInfo().nospace() << "    Valid Frames: " << m_validData24FramesCount;
    qInfo().nospace() << "    Invalid Frames: " << m_invalidData24FramesCount;

    qInfo() << "  Audio Samples:";
    qInfo().nospace() << "    Total stereo samples: "
                      << (m_validSamplesCount + m_invalidSamplesCount) / 2;
    qInfo().nospace() << "    Valid stereo samples: " << m_validSamplesCount / 2;
    qInfo().nospace() << "    Corrupt stereo samples: " << m_invalidSamplesCount / 2;

    qInfo() << "  Section time information:";
    qInfo().noquote() << "    Start time:" << m_startTime.toString();
    qInfo().noquote() << "    End time:" << m_endTime.toString();
    qInfo().noquote() << "    Total time:" << (m_endTime - m_startTime).toString();
}