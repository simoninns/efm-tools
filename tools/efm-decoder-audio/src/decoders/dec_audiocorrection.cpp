/************************************************************************

    dec_audiocorrection.cpp

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

#include "dec_audiocorrection.h"

AudioCorrection::AudioCorrection()
    : m_concealedSamplesCount(0),
      m_silencedSamplesCount(0),
      m_validSamplesCount(0),
      m_lastSectionLeftSample(0),
      m_lastSectionRightSample(0),
      m_lastSectionLeftError(0),
      m_lastSectionRightError(0)
{
}

void AudioCorrection::pushSection(const AudioSection &audioSection)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(audioSection);

    // Process the queue
    processQueue();
}

AudioSection AudioCorrection::popSection()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool AudioCorrection::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void AudioCorrection::processQueue()
{
    // Process the input buffer
    while (!m_inputBuffer.isEmpty()) {
        AudioSection audioSectionIn = m_inputBuffer.dequeue();
        AudioSection audioSectionOut;
        audioSectionOut.metadata = audioSectionIn.metadata;

        // Sanity check the Audio section
        if (!audioSectionIn.isComplete()) {
            qFatal("AudioCorrection::processQueue - Audio Section is not complete");
        }

        for (int index = 0; index < 98; ++index) {
            QVector<qint16> audioInData = audioSectionIn.frame(index).data();
            QVector<qint16> audioInErrorData = audioSectionIn.frame(index).errorData();

            // Does the frame contain any errors?
            if (audioSectionIn.frame(index).countErrors() != 0) {
                // There are errors in the frame
                if (m_showDebug) {
                    qDebug().nospace().noquote()
                            << "AudioCorrection::processQueue(): Frame " << index
                            << " in section with absolute time "
                            << audioSectionIn.metadata.absoluteSectionTime().toString()
                            << " contains errors";
                }

                for (int sidx = 0; sidx < 12; ++sidx) {
                    // Check if the sample is valid
                    if (audioInErrorData[sidx] == 1) {
                        // Determine the preceding sample value
                        qint16 precedingSample = 0;
                        qint16 precedingError = 0;
                        if (sidx > 1) {
                            precedingSample = audioInData[sidx - 2];
                            precedingError = audioInErrorData[sidx - 2];
                        } else {
                            if (sidx % 2 == 0) {
                                precedingSample = m_lastSectionLeftSample;
                                precedingError = m_lastSectionLeftError;
                            } else {
                                precedingSample = m_lastSectionRightSample;
                                precedingError = m_lastSectionRightError;
                            }
                        }

                        // Determine the following sample value
                        qint16 followingSample = 0;
                        qint16 followingError = 0;
                        if (sidx < 10) {
                            followingSample = audioInData[sidx + 2];
                            followingError = audioInErrorData[sidx + 2];
                        } else {
                            if (index < 96) {
                                followingSample = audioSectionIn.frame(index + 2).data()[0];
                                followingError = audioSectionIn.frame(index + 2).errorData()[0];
                            } else {
                                // We are at the end of the section, use the preceding sample
                                followingSample = precedingError;
                                followingError = precedingError;
                            }
                        }

                        // Do we have valid preceding and following samples?
                        if (precedingError == 0 && followingError == 0) {
                            // We have valid preceding and following samples
                            audioInData[sidx] = (precedingSample + followingSample) / 2;
                            if (m_showDebug) {
                                qDebug().nospace()
                                        << "AudioCorrection::processQueue(): Concealing sample "
                                        << sidx << " in frame " << index
                                        << " with preceding sample " << precedingSample
                                        << " and following sample " << followingSample
                                        << " by replacing with average " << audioInData[sidx];
                            }
                            ++m_concealedSamplesCount;
                        } else {
                            // We don't have valid preceding and following samples
                            if (m_showDebug) {
                                qDebug().nospace()
                                        << "AudioCorrection::processQueue(): Silencing sample "
                                        << sidx << " in frame " << index
                                        << " as preceding/following samples are invalid";
                            }
                            audioInData[sidx] = 0;
                            ++m_silencedSamplesCount;
                        }
                    } else {
                        // All frame samples are valid
                        ++m_validSamplesCount;
                    }
                }
            } else {
                // All frame samples are valid
                m_validSamplesCount += 12;
            }

            // Put the resulting data into an Audio frame and push it to the output buffer
            Audio audio;
            audio.setData(audioInData);
            audio.setErrorData(audioInErrorData);

            audioSectionOut.pushFrame(audio);
        }

        // Save the last sample of the section in case it's needed for the next section
        m_lastSectionLeftSample = audioSectionOut.frame(97).data()[10];
        m_lastSectionRightSample = audioSectionOut.frame(97).data()[11];
        m_lastSectionLeftError = audioSectionOut.frame(97).errorData()[10];
        m_lastSectionRightError = audioSectionOut.frame(97).errorData()[11];

        // Add the section to the output buffer
        m_outputBuffer.enqueue(audioSectionOut);
    }
}

void AudioCorrection::showStatistics()
{
    qInfo() << "Audio correction statistics:";
    qInfo().nospace() << "  Total mono samples: "
                      << m_validSamplesCount + m_concealedSamplesCount + m_silencedSamplesCount;
    qInfo().nospace() << "  Valid mono samples: " << m_validSamplesCount;
    qInfo().nospace() << "  Concealed mono samples: " << m_concealedSamplesCount;
    qInfo().nospace() << "  Silenced mono samples: " << m_silencedSamplesCount;
}