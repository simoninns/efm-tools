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
    m_silencedSamplesCount(0),
    m_validSamplesCount(0),
    m_invalidSamplesCount(0)
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
    // Pop the input buffer
    AudioSection audioSection = m_inputBuffer.dequeue();

    AudioSection correctedAudioSection;
    correctedAudioSection.metadata = audioSection.metadata;

    for (int frameOffset=0; frameOffset < 98; ++frameOffset) {
        Audio audio = audioSection.frame(frameOffset);

        QVector<qint16> correctedAudioData;
        QVector<bool> correctedAudioErrorData;

        QVector<qint16> audioData = audio.data();
        QVector<bool> audioErrorData = audio.errorData();

        for (int sampleOffset=0; sampleOffset < 12; ++sampleOffset) {
            if (audioErrorData.at(sampleOffset)) {
                if (m_showDebug) {
                    // Calculate the sample number (according to Audacity)
                    qint64 minutes = audioSection.metadata.absoluteSectionTime().minutes();
                    qint64 seconds = audioSection.metadata.absoluteSectionTime().seconds();
                    qint64 frames = audioSection.metadata.absoluteSectionTime().frameNumber();

                    // Function to convert CDDA timestamp to sample number (Fixed at 44.1 kHz)
                    qint64 sampleNumberMono = ((minutes * 60 + seconds) * 44100 * 2) + 
                        (12 * frameOffset + sampleOffset) + 
                        ((frames - 1) * 1176);
                    qint64 sampleNumberStereo = sampleNumberMono / 2; // Audaicty uses stereo sample pairs when counting samples

                    // Function to convert CDDA timestamp to seconds+milliseconds
                    // Calculate precise time in seconds
                    double timeInSeconds = (minutes * 60.0) + seconds + 
                        (frames - 1) / 75.0 + 
                        (frameOffset * 12.0 + sampleOffset) / (75.0 * 98.0 * 12.0);
                    
                    QString subSection = QString("%1-%2").arg(frameOffset, 2, 10, QChar('0')).arg(sampleOffset/2, 2, 10, QChar('0'));

                    // If sampleOffset is even, then the sample is the left channel
                    // If sampleOffset is odd, then the sample is the right channel
                    QString channel = "R";
                    if (sampleOffset % 2 == 0) channel = "L";

                    qDebug().nospace().noquote() << "AudioCorrection::processQueue(): Silencing "
                        << audioSection.metadata.absoluteSectionTime().toString() << " (" << subSection << ") #"
                        << sampleNumberStereo << " " << channel;
                }

                // Error in the sample
                correctedAudioData.append(32767);
                correctedAudioErrorData.append(true);
                ++m_invalidSamplesCount;
            } else {
                // No error in the sample
                correctedAudioData.append(audioData.at(sampleOffset));
                correctedAudioErrorData.append(false);
                ++m_validSamplesCount;
            }
        }

        Audio correctedAudio;
        correctedAudio.setData(correctedAudioData);
        correctedAudio.setErrorData(correctedAudioErrorData);

        correctedAudioSection.pushFrame(correctedAudio);
    }
    
    // Put the corrected audio in the output buffer
    m_outputBuffer.enqueue(correctedAudioSection);
}

void AudioCorrection::showStatistics()
{
    qInfo().nospace() << "Audio correction statistics:";
    qInfo().nospace() << "  Silenced mono samples: " << m_silencedSamplesCount;
    qInfo().nospace() << "  Valid mono samples: " << m_validSamplesCount;
    qInfo().nospace() << "  Invalid mono samples: " << m_invalidSamplesCount;
}