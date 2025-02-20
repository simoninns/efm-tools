/************************************************************************

    efm_processor.h

    ld-efm-decoder - EFM data encoder
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

#ifndef EFM_PROCESSOR_H
#define EFM_PROCESSOR_H

#include <QString>
#include <QDebug>
#include <QFile>
#include <QElapsedTimer>

#include "decoders.h"
#include "dec_tvaluestochannel.h"
#include "dec_channeltof3frame.h"
#include "dec_f3frametof2section.h"
#include "dec_f2sectioncorrection.h"
#include "dec_f2sectiontof1section.h"
#include "dec_f1sectiontodata24section.h"
#include "dec_data24toaudio.h"
#include "dec_audiocorrection.h"

#include "dec_data24torawsector.h"
#include "dec_rawsectortosector.h"

#include "writer_data.h"
#include "writer_wav.h"
#include "writer_wav_metadata.h"
#include "writer_sector.h"

#include "reader_data.h"

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(const QString &inputFilename, const QString &outputFilename);
    void setShowData(bool showRawSector, bool showAudio, bool showData24, bool showF1, bool showF2, bool showF3);
    void setOutputType(bool outputRawAudio, bool outputWav, bool outputWavMetadata, bool noAudioConcealment, bool outputData);
    void setDebug(bool tvalue, bool channel, bool f3, bool f2, bool f1, bool data24, bool audio,
                  bool audioCorrection, bool rawSector, bool sector);
    void showStatistics() const;

private:
    bool m_showRawSector;
    bool m_showAudio;
    bool m_showData24;
    bool m_showF1;
    bool m_showF2;
    bool m_showF3;

    // Output options
    bool m_outputRawAudio;
    bool m_outputWav;
    bool m_outputWavMetadata;
    bool m_noAudioConcealment;
    bool m_outputData;

    // IEC 60909-1999 Decoders
    TvaluesToChannel m_tValuesToChannel;
    ChannelToF3Frame m_channelToF3;
    F3FrameToF2Section m_f3FrameToF2Section;
    F2SectionCorrection m_f2SectionCorrection;
    F2SectionToF1Section m_f2SectionToF1Section;
    F1SectionToData24Section m_f1SectionToData24Section;
    Data24ToAudio m_data24ToAudio;
    AudioCorrection m_audioCorrection;

    // ECMA-130 Decoders
    Data24ToRawSector m_data24ToRawSector;
    RawSectorToSector m_rawSectorToSector;

    // Input file readers
    ReaderData m_readerData;

    // Output file writers
    WriterData m_writerData;
    WriterWav m_writerWav;
    WriterWavMetadata m_writerWavMetadata;
    WriterSector m_writerSector;

    // Processing statistics
    struct GeneralPipelineStatistics {
        qint64 tValuesToChannelTime{0};
        qint64 channelToF3Time{0};
        qint64 f3ToF2Time{0};
        qint64 f2CorrectionTime{0};
        qint64 f2ToF1Time{0};
        qint64 f1ToData24Time{0};
    } m_generalPipelineStats;

    struct AudioPipelineStatistics {
        qint64 data24ToAudioTime{0};
        qint64 audioCorrectionTime{0};
    } m_audioPipelineStats;

    struct DataPipelineStatistics {
        qint64 data24ToRawSectorTime{0};
        qint64 rawSectorToSectorTime{0};
    } m_dataPipelineStats;

    void processGeneralPipeline();
    void processAudioPipeline();
    void processDataPipeline();

    void showGeneralPipelineStatistics();
    void showAudioPipelineStatistics();
    void showDataPipelineStatistics();
};

#endif // EFM_PROCESSOR_H