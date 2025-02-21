/************************************************************************

    efm_processor.cpp

    efm-decoder-data - EFM Data24 to data decoder
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

#include "efm_processor.h"

EfmProcessor::EfmProcessor() : 
    m_outputData(false),
    m_outputDataMetadata(false)
{}

bool EfmProcessor::process(const QString &inputFilename, const QString &outputFilename)
{
    qDebug() << "EfmProcessor::process(): Decoding F2 Sections from file:" << inputFilename
             << "to file:" << outputFilename;

    // Prepare the input file reader
    if (!m_readerF2Section.open(inputFilename)) {
        qDebug() << "EfmProcessor::process(): Failed to open input F2 Section file:" << inputFilename;
        return false;
    }

    // Prepare the output files...
    if (m_outputRawAudio) m_writerData.open(outputFilename);
    if (m_outputWav) m_writerWav.open(outputFilename);
    if (m_outputWavMetadata) {
        QString metadataFilename = outputFilename;
        if (metadataFilename.endsWith(".wav")) {
            metadataFilename.replace(".wav", ".metadata");
        } else {
            metadataFilename.append(".metadata");
        }
        m_writerWavMetadata.open(metadataFilename);
    }
    if (m_outputData) m_writerSector.open(outputFilename);
    if (m_outputDataMetadata) {
        QString metadataFilename = outputFilename;
        if (metadataFilename.endsWith(".dat")) {
            metadataFilename.replace(".dat", ".metadata");
        } else {
            metadataFilename.append(".metadata");
        }
        m_writerSectorMetadata.open(metadataFilename);
    }

    // Process the F2 Section data
    for (int index = 0; index < m_readerF2Section.size(); ++index) {
        m_f2SectionToF1Section.pushSection(m_readerF2Section.read());
        processGeneralPipeline();

        // Every 500 sections show progress
        if (index % 500 == 0) {
            // Calculate the percentage complete
            float percentageComplete = (index / static_cast<float>(m_readerF2Section.size())) * 100.0;
            qInfo().nospace().noquote() << "Decoding F2 Section " << index << " of " << m_readerF2Section.size() << " (" << QString::number(percentageComplete, 'f', 2) << "%)";
        }
    }

    // We are out of data flush the pipeline and process it one last time
    qInfo() << "Flushing decoding pipelines";
    // Nothing to do here at the moment...

    qInfo() << "Processing final pipeline data";
    processGeneralPipeline();

    // Show summary
    qInfo() << "Decoding complete";

    // Show statistics
    m_f2SectionToF1Section.showStatistics();
    qInfo() << "";
    m_f1SectionToData24Section.showStatistics();
    qInfo() << "";
    if (m_outputRawAudio || m_outputWav) {
        m_data24ToAudio.showStatistics();
        qInfo() << "";
    }
    if ((m_outputRawAudio || m_outputWav) && !m_noAudioConcealment) {
        m_audioCorrection.showStatistics();
        qInfo() << "";
    }

    if (m_outputData) {
        m_data24ToRawSector.showStatistics();
        qInfo() << "";
        m_rawSectorToSector.showStatistics();
        qInfo() << "";
        m_sectorCorrection.showStatistics();
        qInfo() << "";
    }

    showGeneralPipelineStatistics();
    if (m_outputRawAudio || m_outputWav) {
        showAudioPipelineStatistics();
    }

    if (m_outputData) {
        showDataPipelineStatistics();
    }

    // Close the input file
    m_readerF2Section.close();

    // Close the output files
    if (m_writerData.isOpen()) m_writerData.close();
    if (m_writerWav.isOpen()) m_writerWav.close();
    if (m_writerWavMetadata.isOpen()) m_writerWavMetadata.close();
    if (m_writerSector.isOpen()) m_writerSector.close();

    qInfo() << "Encoding complete";
    return true;
}

void EfmProcessor::processGeneralPipeline()
{
    QElapsedTimer pipelineTimer;

    // F2 to F1 processing
    pipelineTimer.restart();
    while (m_f2SectionToF1Section.isReady()) {
        F1Section f1Section = m_f2SectionToF1Section.popSection();
        if (m_showF1)
            f1Section.showData();
        m_f1SectionToData24Section.pushSection(f1Section);
    }
    m_generalPipelineStats.f1ToData24Time += pipelineTimer.nsecsElapsed();

    // Is output audio or data?
    if (m_outputWav) processAudioPipeline();
    else if (m_outputData) processDataPipeline();
    else {
        // Data24 output processing (Raw audio output)
        while (m_f1SectionToData24Section.isReady()) {
            Data24Section data24Section = m_f1SectionToData24Section.popSection();
            m_writerData.write(data24Section);
            if (m_showData24) {
                data24Section.showData();
            }
        }
    }
}

void EfmProcessor::processAudioPipeline()
{
    QElapsedTimer audioPipelineTimer;

    // Audio processing
    audioPipelineTimer.restart();
    while (m_f1SectionToData24Section.isReady()) {
        Data24Section data24Section = m_f1SectionToData24Section.popSection();
        m_data24ToAudio.pushSection(data24Section);
        if (m_showData24) {
            data24Section.showData();
        }
    }
    m_audioPipelineStats.data24ToAudioTime += audioPipelineTimer.nsecsElapsed();

    if (m_noAudioConcealment) {
        while (m_data24ToAudio.isReady()) {
            AudioSection audioSection = m_data24ToAudio.popSection();
            m_writerWav.write(audioSection);
            if (m_outputWavMetadata)
                m_writerWavMetadata.write(audioSection);
        }
    } else {
        audioPipelineTimer.restart();
        while (m_data24ToAudio.isReady()) {
            AudioSection audioSection = m_data24ToAudio.popSection();
            m_audioCorrection.pushSection(audioSection);
        }
        m_audioPipelineStats.audioCorrectionTime += audioPipelineTimer.nsecsElapsed();

        while (m_audioCorrection.isReady()) {
            AudioSection audioSection = m_audioCorrection.popSection();
            m_writerWav.write(audioSection);
            if (m_outputWavMetadata)
                m_writerWavMetadata.write(audioSection);
        }
    }
}

void EfmProcessor::processDataPipeline()
{
    QElapsedTimer dataPipelineTimer;

    // Data24 to scrambled sector processing
    dataPipelineTimer.restart();
    while (m_f1SectionToData24Section.isReady()) {
        Data24Section data24Section = m_f1SectionToData24Section.popSection();
        m_data24ToRawSector.pushSection(data24Section);
        if (m_showData24) {
            data24Section.showData();
        }
    }
    m_dataPipelineStats.data24ToRawSectorTime += dataPipelineTimer.nsecsElapsed();

    // Raw sector to sector processing
    dataPipelineTimer.restart();
    while (m_data24ToRawSector.isReady()) {
        RawSector rawSector = m_data24ToRawSector.popSector();
        m_rawSectorToSector.pushSector(rawSector);
        if (m_showRawSector)
            rawSector.showData();
    }
    m_dataPipelineStats.rawSectorToSectorTime += dataPipelineTimer.nsecsElapsed();

    // Sector correction processing
    while (m_rawSectorToSector.isReady()) {
        Sector sector = m_rawSectorToSector.popSector();
        m_sectorCorrection.pushSector(sector);
    }

    // Write out the sector data
    while (m_sectorCorrection.isReady()) {
        Sector sector = m_sectorCorrection.popSector();
        m_writerSector.write(sector);
        if (m_outputDataMetadata)
            m_writerSectorMetadata.write(sector);
    }
}

void EfmProcessor::showGeneralPipelineStatistics()
{
    qInfo() << "Decoder processing summary (general):";

    qInfo() << "  F2 to F1 processing time:" << m_generalPipelineStats.f2SectionToF1SectionTime / 1000000 << "ms";
    qInfo() << "  F1 to Data24 processing time:" << m_generalPipelineStats.f1ToData24Time / 1000000 << "ms";

    qint64 totalProcessingTime = m_generalPipelineStats.f2SectionToF1SectionTime +
                                 m_generalPipelineStats.f1ToData24Time;
    float totalProcessingTimeSeconds = totalProcessingTime / 1000000000.0;
    qInfo().nospace() << "  Total processing time: " << totalProcessingTime / 1000000 << " ms ("
            << Qt::fixed << qSetRealNumberPrecision(2) << totalProcessingTimeSeconds << " seconds)";

    qInfo() << "";
}

void EfmProcessor::showAudioPipelineStatistics()
{
    qInfo() << "Decoder processing summary (audio):";
    qInfo() << "  Data24 to Audio processing time:" << m_audioPipelineStats.data24ToAudioTime / 1000000 << "ms";
    qInfo() << "  Audio correction processing time:" << m_audioPipelineStats.audioCorrectionTime / 1000000 << "ms";

    qint64 totalProcessingTime = m_audioPipelineStats.data24ToAudioTime + m_audioPipelineStats.data24ToAudioTime;
    float totalProcessingTimeSeconds = totalProcessingTime / 1000000000.0;
    qInfo().nospace() << "  Total processing time: " << totalProcessingTime / 1000000 << " ms ("
            << Qt::fixed << qSetRealNumberPrecision(2) << totalProcessingTimeSeconds << " seconds)";
}

void EfmProcessor::showDataPipelineStatistics()
{
    qInfo() << "Decoder processing summary (data):";
    qInfo() << "  Data24 to Raw Sector processing time:" << m_dataPipelineStats.data24ToRawSectorTime / 1000000 << "ms";
    qInfo() << "  Raw Sector to Sector processing time:" << m_dataPipelineStats.rawSectorToSectorTime / 1000000 << "ms";

    qint64 totalProcessingTime = m_dataPipelineStats.data24ToRawSectorTime + m_dataPipelineStats.rawSectorToSectorTime;
    float totalProcessingTimeSeconds = totalProcessingTime / 1000000000.0;
    qInfo().nospace() << "  Total processing time: " << totalProcessingTime / 1000000 << " ms ("
            << Qt::fixed << qSetRealNumberPrecision(2) << totalProcessingTimeSeconds << " seconds)";

    qInfo() << "";
}

void EfmProcessor::setShowData(bool showRawSector, bool showAudio, bool showData24, bool showF1)
{
    m_showRawSector = showRawSector;
    m_showAudio = showAudio;
    m_showData24 = showData24;
    m_showF1 = showF1;
}

// Set the output data type (true for WAV, false for raw)
void EfmProcessor::setOutputType(bool outputRawAudio, bool outputWav, bool outputWavMetadata, bool noAudioConcealment, bool outputData, bool outputDataMetadata)
{
    m_outputRawAudio = outputRawAudio;
    m_outputWav = outputWav;
    m_outputWavMetadata = outputWavMetadata;
    m_noAudioConcealment = noAudioConcealment;
    m_outputData = outputData;
    m_outputDataMetadata = outputDataMetadata;

    if (!outputRawAudio && !outputWav && !outputData) {
        qWarning() << "No output type specified, defaulting to wav audio output with no metadata";
        m_outputWav = true;
    }
}

void EfmProcessor::setDebug(bool f1, bool data24, bool audio, bool audioCorrection, bool rawSector,
    bool sector, bool sectorCorrection)
{
    // Set the debug flags
    m_f2SectionToF1Section.setShowDebug(f1);
    m_f1SectionToData24Section.setShowDebug(data24);
    m_data24ToAudio.setShowDebug(audio);
    m_audioCorrection.setShowDebug(audioCorrection);
    m_data24ToRawSector.setShowDebug(rawSector);
    m_rawSectorToSector.setShowDebug(sector);
    m_sectorCorrection.setShowDebug(sectorCorrection);
}