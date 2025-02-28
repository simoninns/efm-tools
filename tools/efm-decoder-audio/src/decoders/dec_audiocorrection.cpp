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

AudioCorrection::AudioCorrection() :
    m_concealedSamplesCount(0),
    m_silencedSamplesCount(0),
    m_validSamplesCount(0),
    m_firstSectionFlag(true)
{}

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
    // TODO: this will never correct the very first and last sections

    // Pop a section from the input buffer
    m_correctionBuffer.append(m_inputBuffer.dequeue());

    // Perform correction on the section in the middle of the correction buffer
    if (m_correctionBuffer.size() == 3) {
        AudioSection correctedSection;

        // Process all 98 frames in the section
        for (int fIdx = 0; fIdx < 98; ++fIdx) {
            Audio correctedFrame;

            // Get the preceding, correcting and following frames
            Audio precedingFrame, correctingFrame, followingFrame;
            if (fIdx == 0) {
                // If this is the first frame, use the first frame in the section as the preceding frame
                precedingFrame = m_correctionBuffer.at(0).frame(97);
            } else {
                precedingFrame = m_correctionBuffer.at(1).frame(fIdx - 1);
            }

            correctingFrame = m_correctionBuffer.at(1).frame(fIdx);

            if (fIdx == 97) {
                // If this is the last frame, use the last frame in the section as the following frame
                followingFrame = m_correctionBuffer.at(2).frame(0);
            } else {
                followingFrame = m_correctionBuffer.at(1).frame(fIdx + 1);
            }

            // Sample correction
            QVector<qint16> correctedLeftSamples;
            QVector<qint16> correctedLeftErrorSamples;
            QVector<qint16> correctedRightSamples;
            QVector<qint16> correctedRightErrorSamples;

            for (int sIdx = 0; sIdx < 6; ++sIdx) {
                // Get the preceding, correcting and following left samples
                qint16 precedingLeftSample, correctingLeftSample, followingLeftSample;
                qint16 precedingLeftSampleError, correctingLeftSampleError, followingLeftSampleError;

                if (sIdx == 0) {
                    precedingLeftSample = precedingFrame.leftChannel.data().at(5);
                    precedingLeftSampleError = precedingFrame.leftChannel.errorData().at(5);
                } else {
                    precedingLeftSample = correctingFrame.leftChannel.data().at(sIdx - 1);
                    precedingLeftSampleError = correctingFrame.leftChannel.errorData().at(sIdx - 1);
                }

                correctingLeftSample = correctingFrame.leftChannel.data().at(sIdx);
                correctingLeftSampleError = correctingFrame.leftChannel.errorData().at(sIdx);

                if (sIdx == 5) {
                    followingLeftSample = followingFrame.leftChannel.data().at(0);
                    followingLeftSampleError = followingFrame.leftChannel.errorData().at(0);
                } else {
                    followingLeftSample = correctingFrame.leftChannel.data().at(sIdx + 1);
                    followingLeftSampleError = correctingFrame.leftChannel.errorData().at(sIdx + 1);
                }

                if (correctingLeftSampleError != 0) {
                    // Do we have a valid preceding and following sample?
                    if (precedingLeftSampleError != 0 && followingLeftSampleError != 0) {
                        // Silence the sample
                        qDebug().noquote().nospace() << "AudioCorrection::processQueue() -  Left  Silencing: "
                            << "Section address " << m_correctionBuffer.at(1).metadata.absoluteSectionTime().toString()
                            << " - Frame " << fIdx << ", sample " << sIdx;
                        correctedLeftSamples.append(0);
                        correctedLeftErrorSamples.append(1);
                        ++m_silencedSamplesCount;
                    } else {
                        // Conceal the sample
                        qDebug().noquote().nospace() << "AudioCorrection::processQueue() -  Left Concealing: "
                            << "Section address " << m_correctionBuffer.at(1).metadata.absoluteSectionTime().toString()
                            << " - Frame " << fIdx << ", sample " << sIdx
                            << " - Preceding = " << precedingLeftSample << ", Following = " << followingLeftSample
                            << ", Average = " << (precedingLeftSample + followingLeftSample) / 2;
                        correctedLeftSamples.append((precedingLeftSample + followingLeftSample) / 2);
                        correctedLeftErrorSamples.append(1);
                        ++m_concealedSamplesCount;
                    }
                } else {
                    // The sample is valid - just copy it
                    correctedLeftSamples.append(correctingLeftSample);
                    correctedLeftErrorSamples.append(0);
                    ++m_validSamplesCount;
                }

                // ===========

                // Get the preceding, correcting and following right samples
                qint16 precedingRightSample, correctingRightSample, followingRightSample;
                qint16 precedingRightSampleError, correctingRightSampleError, followingRightSampleError;

                if (sIdx == 0) {
                    precedingRightSample = precedingFrame.rightChannel.data().at(5);
                    precedingRightSampleError = precedingFrame.rightChannel.errorData().at(5);
                } else {
                    precedingRightSample = correctingFrame.rightChannel.data().at(sIdx - 1);
                    precedingRightSampleError = correctingFrame.rightChannel.errorData().at(sIdx - 1);
                }

                correctingRightSample = correctingFrame.rightChannel.data().at(sIdx);
                correctingRightSampleError = correctingFrame.rightChannel.errorData().at(sIdx);

                if (sIdx == 5) {
                    followingRightSample = followingFrame.rightChannel.data().at(0);
                    followingRightSampleError = followingFrame.rightChannel.errorData().at(0);
                } else {
                    followingRightSample = correctingFrame.rightChannel.data().at(sIdx + 1);
                    followingRightSampleError = correctingFrame.rightChannel.errorData().at(sIdx + 1);
                }

                if (correctingRightSampleError != 0) {
                    // Do we have a valid preceding and following sample?
                    if (precedingRightSampleError != 0 && followingRightSampleError != 0) {
                        // Silence the sample
                        qDebug().noquote().nospace() << "AudioCorrection::processQueue() - Right  Silencing: "
                            << "Section address " << m_correctionBuffer.at(1).metadata.absoluteSectionTime().toString()
                            << " - Frame " << fIdx << ", sample " << sIdx;
                        correctedRightSamples.append(0);
                        correctedRightErrorSamples.append(1);
                        ++m_silencedSamplesCount;
                    } else {
                        // Conceal the sample
                        qDebug().noquote().nospace() << "AudioCorrection::processQueue() - Right Concealing: "
                            << "Section address " << m_correctionBuffer.at(1).metadata.absoluteSectionTime().toString()
                            << " - Frame " << fIdx << ", sample " << sIdx
                            << " - Preceding = " << precedingRightSample << ", Following = " << followingRightSample
                            << ", Average = " << (precedingRightSample + followingRightSample) / 2;
                        correctedRightSamples.append((precedingRightSample + followingRightSample) / 2);
                        correctedRightErrorSamples.append(1);
                        ++m_concealedSamplesCount;
                    }
                } else {
                    // The sample is valid - just copy it
                    correctedRightSamples.append(correctingRightSample);
                    correctedRightErrorSamples.append(0);
                    ++m_validSamplesCount;
                }
            }

            // Write the channel data back to the correction buffer's frame
            correctedFrame.leftChannel.setData(correctedLeftSamples);
            correctedFrame.leftChannel.setErrorData(correctedLeftErrorSamples);
            correctedFrame.rightChannel.setData(correctedRightSamples);
            correctedFrame.rightChannel.setErrorData(correctedRightErrorSamples);

            correctedSection.pushFrame(correctedFrame);
        }

        correctedSection.metadata = m_correctionBuffer.at(1).metadata;
        m_correctionBuffer[1] = correctedSection;

        // Write the first section in the correction buffer to the output buffer
        m_outputBuffer.enqueue(m_correctionBuffer.at(0));
        m_correctionBuffer.removeFirst();
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