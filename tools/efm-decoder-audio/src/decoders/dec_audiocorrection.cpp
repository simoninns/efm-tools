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

    for (int i=0; i < 98; ++i) {
        Audio audio = audioSection.frame(i);

        QVector<qint16> correctedAudioData;
        QVector<bool> correctedAudioErrorData;

        for (int j=0; j < 12; ++j) {
            qint16 sample = audio.data().at(j);
            bool error = audio.errorData().at(j);

            if (error) {
                // Error in the sample
                correctedAudioData.append(0);
                correctedAudioErrorData.append(true);
                ++m_invalidSamplesCount;
            } else {
                // No error in the sample
                correctedAudioData.append(sample);
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