/************************************************************************

    efm_processor.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#include "efm_processor.h"

EfmProcessor::EfmProcessor() { }

bool EfmProcessor::process(const QString &inputFileName, const QString &outputFileName)
{
    qInfo() << "Encoding EFM data from file:" << inputFileName << "to file:" << outputFileName;

    // Open the input file
    QFile inputFile(inputFileName);

    if (!inputFile.open(QIODevice::ReadOnly)) {
        qDebug() << "EfmProcessor::process(): Failed to open input file:" << inputFileName;
        return false;
    }

    // Calculate the total size of the input file for progress reporting
    qint64 totalSize = inputFile.size();
    qint64 processedSize = 0;
    int lastReportedProgress = 0;

    if (m_isInputDataWav) {
        // Input data is a WAV file, so we need to verify the WAV file format
        // by reading the WAV header
        // Read the WAV header
        QByteArray header = inputFile.read(44);
        if (header.size() != 44) {
            qWarning() << "Failed to read WAV header from input file:" << inputFileName;
            return false;
        }

        // Check the WAV header for the correct format
        if (header.mid(0, 4) != "RIFF" || header.mid(8, 4) != "WAVE") {
            qWarning() << "Invalid WAV file format:" << inputFileName;
            return false;
        }

        // Check the sampling rate (44.1KHz)
        quint32 sampleRate = *reinterpret_cast<const quint32 *>(header.mid(24, 4).constData());
        if (sampleRate != 44100) {
            qWarning() << "Unsupported sample rate:" << sampleRate << "in file:" << inputFileName;
            return false;
        }

        // Check the bit depth (16 bits)
        uint16_t bitDepth = *reinterpret_cast<const quint16 *>(header.mid(34, 2).constData());
        if (bitDepth != 16) {
            qWarning() << "Unsupported bit depth:" << bitDepth << "in file:" << inputFileName;
            return false;
        }

        // Check the number of channels (stereo)
        uint16_t numChannels = *reinterpret_cast<const quint16 *>(header.mid(22, 2).constData());
        if (numChannels != 2) {
            qWarning() << "Unsupported number of channels:" << numChannels
                       << "in file:" << inputFileName;
            return false;
        }
    }

    // Prepare the output file
    QFile outputFile(outputFileName);

    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmProcessor::process(): Failed to open output file:" << outputFileName;
        return false;
    }

    // Check for the pad start corruption option
    if (m_corruptStart) {
        qInfo() << "Corrupting output: Padding start of file with" << m_corruptStartSymbols
                << "t-value symbols";
        // Pad the start of the file with the specified number of symbols
        srand(static_cast<unsigned int>(time(nullptr))); // Seed the random number generator
        for (quint32 i = 0; i < m_corruptStartSymbols; i++) {
            // Pick a random value between 3 and 11
            uint8_t value = (rand() % 9) + 3;
            char val = static_cast<char>(value);
            outputFile.write(&val, sizeof(val));
        }
    }

    // Check for the corrupt t-values option
    if (m_corruptTvalues) {
        if (m_corruptTvaluesFrequency < 2) {
            qWarning() << "Corrupting output: Corrupt t-values frequency must be at least 2";
            return false;
        }
        qInfo() << "Corrupting output: Corrupting t-values with a frequency of"
                << m_corruptTvaluesFrequency;
    }

    // Prepare the encoders
    Data24SectionToF1Section data24SectionToF1Section;
    F1SectionToF2Section f1SectionToF2Section;
    F2SectionToF3Frames f2SectionToF3Frames;
    F3FrameToChannel f3FrameToChannel;

    f3FrameToChannel.setCorruption(m_corruptF3sync, m_corruptF3syncFrequency,
                                       m_corruptSubcodeSync, m_corruptSubcodeSyncFrequency);

    // Channel data counter
    quint32 channelByteCount = 0;

    // Process the input audio data 24 bytes at a time
    // The time, type and track number are set to default values for now
    uint8_t track_number = 1;
    SectionType sectionType(SectionType::UserData);
    SectionTime sectionTime;
    quint32 data24SectionCount = 0;

    // We have to fill a Data24Section with 98 * 24 bytes of data
    // If we can't fill a section, then we have run out of usable data

    QVector<uint8_t> inputData(98 * 24);
    uint64_t bytesRead = inputFile.read(reinterpret_cast<char *>(inputData.data()), 98 * 24);

    while (bytesRead >= 98 * 24) {
        // Create a Data24Section object
        Data24Section data24Section;
        m_sectionMetadata.setSectionType(sectionType);
        m_sectionMetadata.setSectionTime(sectionTime);
        m_sectionMetadata.setAbsoluteSectionTime(sectionTime);
        m_sectionMetadata.setTrackNumber(track_number);
        data24Section.metadata = m_sectionMetadata;

        for (int index = 0; index < 98; ++index) {
            // Create a Data24 object and set the data
            Data24 data24;
            data24.setData(inputData.mid(index * 24, 24));
            data24Section.pushFrame(data24);
        }
        if (m_showInput)
            data24Section.showData();

        // Push the data to the first converter
        data24SectionToF1Section.pushSection(data24Section);
        data24SectionCount++;

        // Adjust the section time
        sectionTime++;

        // Are there any F1 sections ready?
        while (data24SectionToF1Section.isReady()) {
            // Pop the F1 frame, count it and push it to the next converter
            F1Section f1Section = data24SectionToF1Section.popSection();
            if (m_showF1)
                f1Section.showData();
            f1SectionToF2Section.pushSection(f1Section);
        }

        // Are there any F2 sections ready?
        while (f1SectionToF2Section.isReady()) {
            // Pop the F2 frame, count it and push it to the next converter
            F2Section f2Section = f1SectionToF2Section.popSection();
            if (m_showF2)
                f2Section.showData();
            f2SectionToF3Frames.pushSection(f2Section);
        }

        // Are there any F3 frames ready?
        while (f2SectionToF3Frames.isReady()) {
            // Pop the section, count it and push it to the next converter
            QVector<F3Frame> f3Frames = f2SectionToF3Frames.popFrames();

            for (int i = 0; i < f3Frames.size(); i++) {
                if (m_showF3)
                    f3Frames[i].showData();
                f3FrameToChannel.pushFrame(f3Frames[i]);
            }
        }

        // Is there any channel data ready?
        while (f3FrameToChannel.isReady()) {
            // Pop the channel data, count it and write it to the output file
            QVector<uint8_t> channelData = f3FrameToChannel.popFrame();

            // Check for the corrupt t-values option
            if (m_corruptTvalues) {
                // We have to take the channel_byte_count and the size of channel data
                // and figure out if we need to corrupt the t-values in channel_data based
                // on the corrupt_tvalues_frequency
                //
                // Any t-values that should be corrupted will be set to a random
                // value between 3 and 11
                for (int j = 0; j < channelData.size(); j++) {
                    if ((channelByteCount + j) % m_corruptTvaluesFrequency == 0) {
                        uint8_t new_value;
                        // Make sure the new value is different from the current value
                        do {
                            new_value = (rand() % 9) + 3;
                        } while (new_value == channelData[j]);
                        channelData[j] = new_value;
                        // qDebug() << "Corrupting t-value at" << (channel_byte_count + j) << "with
                        // value" << channel_data[j];
                    }
                }
            }

            channelByteCount += channelData.size();

            // Write the channel data to the output file
            outputFile.write(reinterpret_cast<const char *>(channelData.data()),
                              channelData.size());
        }

        // Update the processed size
        processedSize += bytesRead;

        // Calculate the current progress
        int currentProgress = static_cast<int>((processedSize * 100) / totalSize);

        // Report progress every 5%
        if (currentProgress >= lastReportedProgress + 5) {
            qInfo() << "Progress:" << currentProgress << "%";
            lastReportedProgress = currentProgress;
        }

        // Read the next 98 * 24 bytes
        bytesRead = inputFile.read(reinterpret_cast<char *>(inputData.data()), 98 * 24);
    }

    // Close the output file
    outputFile.close();

    // Output some statistics
    double totalBytes = data24SectionCount * 98 * 24;
    QString sizeUnit;
    double sizeValue;

    if (totalBytes < 1024) {
        sizeUnit = "bytes";
        sizeValue = totalBytes;
    } else if (totalBytes < 1024 * 1024) {
        sizeUnit = "Kbytes";
        sizeValue = totalBytes / 1024;
    } else {
        sizeUnit = "Mbytes";
        sizeValue = totalBytes / (1024 * 1024);
    }

    qInfo().noquote() << "Processed" << data24SectionCount << "data24 sections totalling"
                      << sizeValue << sizeUnit;
    qInfo().noquote() << "Final time was" << sectionTime.toString();

    qInfo() << data24SectionToF1Section.validOutputSectionsCount() << "F1 sections";
    qInfo() << f1SectionToF2Section.validOutputSectionsCount() << "F2 sections";
    qInfo() << f2SectionToF3Frames.validOutputSectionsCount() << "F3 frames";
    qInfo() << f3FrameToChannel.validOutputSectionsCount() << "Channel frames";
    qInfo() << f3FrameToChannel.getTotalTValues() << "T-values";
    qInfo() << channelByteCount << "channel bytes";

    // Show corruption warnings
    if (m_corruptTvalues) {
        qWarning() << "Corruption applied-> Corrupted t-values with a frequency of"
                   << m_corruptTvaluesFrequency;
    }
    if (m_corruptStart) {
        qWarning() << "Corruption applied-> Padded start of file with" << m_corruptStartSymbols
                   << "random t-value symbols";
    }
    if (m_corruptF3sync) {
        qWarning() << "Corruption applied-> Corrupted F3 Frame 24-bit sync patterns with a frame "
                      "frequency of"
                   << m_corruptF3syncFrequency;
    }
    if (m_corruptSubcodeSync) {
        qWarning() << "Corruption applied-> Corrupted subcode sync0 and sync1 patterns with a "
                      "section frequency of"
                   << m_corruptSubcodeSyncFrequency;
    }

    qInfo() << "Encoding complete";

    return true;
}

bool EfmProcessor::setQmodeOptions(bool qmode1, bool qmode4, bool qmodeAudio, bool qmodeData,
                                bool qmodeCopy, bool qmodeNocopy, bool qmodeNopreemp,
                                bool qmodePreemp, bool qmode2ch, bool qmode4ch)
{
    if (qmode1 && qmode4) {
        qInfo() << "You can only specify one Q-Channel mode with --qmode-1 or --qmode-4";
        return false;
    }
    if (qmodeAudio && qmodeData) {
        qInfo() << "You can only specify one Q-Channel data type with --qmode-audio or "
                   "--qmode-data";
        return false;
    }
    if (qmodeCopy && qmodeNocopy) {
        qFatal("You can only specify one Q-Channel copy type with --qmode-copy or --qmode-nocopy");
        return false;
    }
    if (qmode2ch && qmode4ch) {
        qInfo() << "You can only specify one Q-Channel channel type with --qmode-2ch or "
                   "--qmode-4ch";
        return false;
    }
    if (qmodeNopreemp && qmodePreemp) {
        qInfo() << "You can only specify one Q-Channel preemphasis type with --qmode-preemp or "
                   "--qmode-nopreemp";
        return false;
    }

    if (qmode1) {
        m_sectionMetadata.setQMode(SectionMetadata::QMode1);
        qInfo() << "Q-Channel mode set to: QMode1";
    } else if (qmode4) {
        m_sectionMetadata.setQMode(SectionMetadata::QMode4);
        qInfo() << "Q-Channel mode set to: QMode4";
    }

    if (qmodeAudio && qmodeCopy && qmodePreemp && qmode2ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(false);
        m_sectionMetadata.setPreemphasis(true);
        m_sectionMetadata.set2Channel(true);
        qInfo() << "Q-Channel control mode set to: AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED";
    }
    if (qmodeAudio && qmodeCopy && qmodeNopreemp && qmode2ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(false);
        m_sectionMetadata.setPreemphasis(false);
        m_sectionMetadata.set2Channel(true);
        qInfo() << "Q-Channel control mode set to: AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED";
    }
    if (qmodeAudio && qmodeNocopy && qmodePreemp && qmode2ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(true);
        m_sectionMetadata.setPreemphasis(true);
        m_sectionMetadata.set2Channel(true);
        qInfo() << "Q-Channel control mode set to: AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED";
    }
    if (qmodeAudio && qmodeNocopy && qmodeNopreemp && qmode2ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(true);
        m_sectionMetadata.setPreemphasis(false);
        m_sectionMetadata.set2Channel(true);
        qInfo() << "Q-Channel control mode set to: AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED";
    }

    if (qmodeAudio && qmodeCopy && qmodePreemp && qmode4ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(false);
        m_sectionMetadata.setPreemphasis(true);
        m_sectionMetadata.set2Channel(false);
        qInfo() << "Q-Channel control mode set to: AUDIO_4CH_PREEMPHASIS_COPY_PERMITTED";
    }
    if (qmodeAudio && qmodeCopy && qmodeNopreemp && qmode4ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(false);
        m_sectionMetadata.setPreemphasis(false);
        m_sectionMetadata.set2Channel(false);
        qInfo() << "Q-Channel control mode set to: AUDIO_4CH_NO_PREEMPHASIS_COPY_PERMITTED";
    }
    if (qmodeAudio && qmodeNocopy && qmodePreemp && qmode4ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(true);
        m_sectionMetadata.setPreemphasis(true);
        m_sectionMetadata.set2Channel(false);
        qInfo() << "Q-Channel control mode set to: AUDIO_4CH_PREEMPHASIS_COPY_PROHIBITED";
    }
    if (qmodeAudio && qmodeNocopy && qmodeNopreemp && qmode4ch) {
        m_sectionMetadata.setAudio(true);
        m_sectionMetadata.setCopyProhibited(true);
        m_sectionMetadata.setPreemphasis(false);
        m_sectionMetadata.set2Channel(false);
        qInfo() << "Q-Channel control mode set to: AUDIO_4CH_NO_PREEMPHASIS_COPY_PROHIBITED";
    }

    if (qmodeData && qmodeCopy) {
        m_sectionMetadata.setAudio(false);
        m_sectionMetadata.setCopyProhibited(false);
        qInfo() << "Q-Channel control mode set to: DIGITAL_COPY_PERMITTED";
    }
    if (qmodeData && qmodeNocopy) {
        m_sectionMetadata.setAudio(false);
        m_sectionMetadata.setCopyProhibited(true);
        qInfo() << "Q-Channel control mode set to: DIGITAL_COPY_PROHIBITED";
    }

    return true;
}

void EfmProcessor::setShowData(bool showInput, bool showF1, bool showF2, bool showF3)
{
    m_showInput = showInput;
    m_showF1 = showF1;
    m_showF2 = showF2;
    m_showF3 = showF3;
}

// Set the input data type (true for WAV, false for raw)
void EfmProcessor::setInputType(bool wavInput)
{
    m_isInputDataWav = wavInput;
}

// Note: The corruption options are to assist in generating test data
// used to verify the decoder.  They are not intended to be used in
// normal operation.
bool EfmProcessor::setCorruption(bool corruptTvalues, quint32 corruptTvaluesFrequency,
                              bool corruptStart, quint32 corruptStartSymbols,
                              bool corruptF3sync, quint32 corruptF3syncFrequency,
                              bool corruptSubcodeSync, quint32 corruptSubcodeSyncFrequency)
{

    m_corruptTvalues = corruptTvalues;
    m_corruptTvaluesFrequency = corruptTvaluesFrequency;

    if (m_corruptTvalues && m_corruptTvaluesFrequency < 2) {
        qInfo() << "Corrupting output: Corrupt t-values frequency must be at least 2";
        return false;
    }

    m_corruptStart = corruptStart;
    m_corruptStartSymbols = corruptStartSymbols;

    if (m_corruptStart && m_corruptStartSymbols < 1) {
        qInfo() << "Corrupting output: Pad start symbols must be at least 1";
        return false;
    }

    m_corruptF3sync = corruptF3sync;
    m_corruptF3syncFrequency = corruptF3syncFrequency;

    if (m_corruptF3sync && m_corruptF3syncFrequency < 2) {
        qInfo() << "Corrupting output: Corrupt F3 sync frequency must be at least 2";
        return false;
    }

    m_corruptSubcodeSync = corruptSubcodeSync;
    m_corruptSubcodeSyncFrequency = corruptSubcodeSyncFrequency;

    if (m_corruptSubcodeSync && m_corruptSubcodeSyncFrequency < 2) {
        qInfo() << "Corrupting output: Corrupt subcode sync frequency must be at least 2";
        return false;
    }

    return true;
}