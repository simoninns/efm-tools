/************************************************************************

    efm_processor.cpp

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

#include "efm_processor.h"

EfmProcessor::EfmProcessor() { }

bool EfmProcessor::process(const QString &inputFilename, const QString &outputFilename)
{
    qDebug() << "EfmProcessor::process(): Decoding EFM from file:" << inputFilename
             << "to file:" << outputFilename;

    // Prepare the input file reader
    if (!m_readerData.open(inputFilename)) {
        qDebug() << "EfmProcessor::process(): Failed to open input file:" << inputFilename;
        return false;
    }

    // Prepare the output files...
    if (!m_isOutputDataWav)
        m_writerData.open(outputFilename);
    else
        m_writerWav.open(outputFilename);

    if (m_outputWavMetadata && m_isOutputDataWav) {
        // Prepare the metadata output file
        QString metadataFilename = outputFilename;
        if (metadataFilename.endsWith(".wav")) {
            metadataFilename.replace(".wav", ".metadata");
        } else {
            metadataFilename.append(".metadata");
        }

        m_writerWavMetadata.open(metadataFilename);
    }

    // Get the total size of the input file for progress reporting
    qint64 totalSize = m_readerData.size();
    qint64 processedSize = 0;
    int lastProgress = 0;

    // Process the EFM data in chunks of 1024 T-values
    bool endOfData = false;
    while (!endOfData) {
        // Read 1024 T-values from the input file
        QByteArray tValues = m_readerData.read(1024);
        processedSize += tValues.size();

        int progress = static_cast<int>((processedSize * 100) / totalSize);
        if (progress >= lastProgress + 5) { // Show progress every 5%
            qInfo() << "Progress:" << progress << "%";
            lastProgress = progress;
        }

        if (tValues.isEmpty()) {
            endOfData = true;
        } else {
            m_tValuesToChannel.pushFrame(tValues);
        }

        processPipeline();
    }

    // We are out of data flush the pipeline and process it one last time
    qInfo() << "Flushing decoding pipelines";
    m_f2SectionCorrection.flush();

    qInfo() << "Processing final pipeline data";
    processPipeline();

    // Show summary
    qInfo() << "Decoding complete";

    m_tValuesToChannel.showStatistics();
    qInfo() << "";
    m_channelToF3.showStatistics();
    qInfo() << "";
    m_f3FrameToF2Section.showStatistics();
    qInfo() << "";
    m_f2SectionCorrection.showStatistics();
    qInfo() << "";
    m_f2SectionToF1Section.showStatistics();
    qInfo() << "";
    m_f1SectionToData24Section.showStatistics();
    qInfo() << "";
    if (m_isOutputDataWav) {
        m_data24ToAudio.showStatistics();
        qInfo() << "";
    }
    if (m_isOutputDataWav && !m_noWavCorrection) {
        m_audioCorrection.showStatistics();
        qInfo() << "";
    }

    showStatistics();

    // Close the input file
    m_readerData.close();

    // Close the output files
    if (!m_isOutputDataWav)
        m_writerData.close();
    else
        m_writerWav.close();
    if (m_outputWavMetadata && m_isOutputDataWav)
        m_writerWavMetadata.close();

    qInfo() << "Encoding complete";
    return true;
}

void EfmProcessor::processPipeline()
{
    QElapsedTimer timer;

    // T-values to Channel processing
    timer.start();
    while (m_tValuesToChannel.isReady()) {
        QByteArray channelData = m_tValuesToChannel.popFrame();
        m_channelToF3.pushFrame(channelData);
    }
    m_stats.tValuesToChannelTime += timer.nsecsElapsed();

    // Channel to F3 processing
    timer.restart();
    while (m_channelToF3.isReady()) {
        F3Frame f3Frame = m_channelToF3.popFrame();
        if (m_showF3)
            f3Frame.showData();
        m_f3FrameToF2Section.pushFrame(f3Frame);
    }
    m_stats.channelToF3Time += timer.nsecsElapsed();

    // F3 to F2 section processing
    timer.restart();
    while (m_f3FrameToF2Section.isReady()) {
        F2Section section = m_f3FrameToF2Section.popSection();
        m_f2SectionCorrection.pushSection(section);
    }
    m_stats.f3ToF2Time += timer.nsecsElapsed();

    // F2 correction processing
    timer.restart();
    while (m_f2SectionCorrection.isReady()) {
        F2Section f2Section = m_f2SectionCorrection.popSection();
        if (m_showF2)
            f2Section.showData();
        m_f2SectionToF1Section.pushSection(f2Section);
    }
    m_stats.f2CorrectionTime += timer.nsecsElapsed();

    // F2 to F1 processing
    timer.restart();
    while (m_f2SectionToF1Section.isReady()) {
        F1Section f1Section = m_f2SectionToF1Section.popSection();
        if (m_showF1)
            f1Section.showData();
        m_f1SectionToData24Section.pushSection(f1Section);
    }
    m_stats.f2ToF1Time += timer.nsecsElapsed();

    timer.restart();
    if (!m_isOutputDataWav) {
        // Data24 output processing
        timer.restart();
        while (m_f1SectionToData24Section.isReady()) {
            Data24Section data24Section = m_f1SectionToData24Section.popSection();
            m_writerData.write(data24Section);
            if (m_showData24) {
                data24Section.showData();
            }
        }
        m_stats.f1ToData24Time += timer.nsecsElapsed();
    } else {
        // Audio processing
        timer.restart();
        while (m_f1SectionToData24Section.isReady()) {
            Data24Section data24Section = m_f1SectionToData24Section.popSection();
            m_data24ToAudio.pushSection(data24Section);
            if (m_showData24) {
                data24Section.showData();
            }
        }
        m_stats.f1ToData24Time += timer.nsecsElapsed();

        if (m_noWavCorrection) {
            timer.restart();
            while (m_data24ToAudio.isReady()) {
                AudioSection audioSection = m_data24ToAudio.popSection();
                m_writerWav.write(audioSection);
                if (m_outputWavMetadata)
                    m_writerWavMetadata.write(audioSection);
            }
            m_stats.data24ToAudioTime += timer.nsecsElapsed();
        } else {
            timer.restart();
            while (m_data24ToAudio.isReady()) {
                AudioSection audioSection = m_data24ToAudio.popSection();
                m_audioCorrection.pushSection(audioSection);
            }
            m_stats.data24ToAudioTime += timer.nsecsElapsed();

            timer.restart();
            while (m_audioCorrection.isReady()) {
                AudioSection audioSection = m_audioCorrection.popSection();
                m_writerWav.write(audioSection);
                if (m_outputWavMetadata)
                    m_writerWavMetadata.write(audioSection);
            }
            m_stats.audioCorrectionTime += timer.nsecsElapsed();
        }
    }
}

void EfmProcessor::showStatistics()
{
    qInfo() << "Decoder processing summary:";
    qInfo() << "  T-values to Channel processing time:" << m_stats.tValuesToChannelTime / 1000000
            << "ms";
    qInfo() << "  Channel to F3 processing time:" << m_stats.channelToF3Time / 1000000 << "ms";
    qInfo() << "  F3 to F2 section processing time:" << m_stats.f3ToF2Time / 1000000 << "ms";
    qInfo() << "  F2 correction processing time:" << m_stats.f2CorrectionTime / 1000000 << "ms";
    qInfo() << "  F2 to F1 processing time:" << m_stats.f2ToF1Time / 1000000 << "ms";
    qInfo() << "  F1 to Data24 processing time:" << m_stats.f1ToData24Time / 1000000 << "ms";
    if (m_isOutputDataWav) {
        qInfo() << "  Data24 to Audio processing time:" << m_stats.data24ToAudioTime / 1000000 << "ms";
        qInfo() << "  Audio correction processing time:" << m_stats.audioCorrectionTime / 1000000
                << "ms";
    }
    qInfo() << "";

    qint64 totalProcessingTime = m_stats.tValuesToChannelTime + m_stats.channelToF3Time +
                                 m_stats.f3ToF2Time + m_stats.f2CorrectionTime + m_stats.f2ToF1Time +
                                 m_stats.f1ToData24Time + m_stats.data24ToAudioTime +
                                 m_stats.audioCorrectionTime;
    float totalProcessingTimeSeconds = totalProcessingTime / 1000000000.0;
    qInfo().nospace() << "  Total processing time: " << totalProcessingTime / 1000000 << " ms ("
            << Qt::fixed << qSetRealNumberPrecision(2) << totalProcessingTimeSeconds << " seconds)";
}

void EfmProcessor::setShowData(bool showAudio, bool showData24, bool showF1, bool showF2,
                               bool showF3)
{
    m_showAudio = showAudio;
    m_showData24 = showData24;
    m_showF1 = showF1;
    m_showF2 = showF2;
    m_showF3 = showF3;
}

// Set the output data type (true for WAV, false for raw)
void EfmProcessor::setOutputType(bool wavOutput, bool outputWavMetadata, bool noWavCorrection)
{
    m_isOutputDataWav = wavOutput;
    m_noWavCorrection = noWavCorrection;
    m_outputWavMetadata = outputWavMetadata;
}

void EfmProcessor::setDebug(bool tvalue, bool channel, bool f3, bool f2, bool f1, bool data24,
                            bool audio, bool audioCorrection)
{
    // Set the debug flags
    m_tValuesToChannel.setShowDebug(tvalue);
    m_channelToF3.setShowDebug(channel);
    m_f3FrameToF2Section.setShowDebug(f3);
    m_f2SectionCorrection.setShowDebug(f2);
    m_f2SectionToF1Section.setShowDebug(f1);
    m_f1SectionToData24Section.setShowDebug(data24);
    m_data24ToAudio.setShowDebug(audio);
    m_audioCorrection.setShowDebug(audioCorrection);
}