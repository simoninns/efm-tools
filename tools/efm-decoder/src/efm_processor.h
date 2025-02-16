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

#include "decoders.h"
#include "dec_tvaluestochannel.h"
#include "dec_channeltof3frame.h"
#include "dec_f3frametof2section.h"
#include "dec_f2sectioncorrection.h"
#include "dec_f2sectiontof1section.h"
#include "dec_f1sectiontodata24section.h"
#include "dec_data24toaudio.h"
#include "dec_audiocorrection.h"

#include "writer_data.h"
#include "writer_wav.h"
#include "writer_wav_metadata.h"

#include "reader_data.h"

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(QString input_filename, QString output_filename);
    void process_pipeline();
    void set_show_data(bool _showAudio, bool _showData24, bool _showF1, bool _showF2, bool _showF3);
    void set_output_type(bool _wavOutput, bool _outputWavMetadata, bool _noWavCorrection);
    void set_debug(bool tvalue, bool channel, bool f3, bool f2, bool f1, bool data24, bool audio,
                   bool audioCorrection);

private:
    bool showAudio;
    bool showData24;
    bool showF1;
    bool showF2;
    bool showF3;
    bool is_output_data_wav;
    bool no_wav_correction;
    bool output_wav_metadata;

    // Decoders
    TvaluesToChannel t_values_to_channel;
    ChannelToF3Frame channel_to_f3;
    F3FrameToF2Section f3_frame_to_f2_section;
    F2SectionCorrection f2_section_correction;
    F2SectionToF1Section f2_section_to_f1_section;
    F1SectionToData24Section f1_section_to_data24_section;
    Data24ToAudio data24_to_audio;
    AudioCorrection audio_correction;

    // Input file readers
    ReaderData reader_data;

    // Output file writers
    WriterData writer_data;
    WriterWav writer_wav;
    WriterWavMetadata writer_wav_metadata;
};

#endif // EFM_PROCESSOR_H