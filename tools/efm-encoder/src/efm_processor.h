/************************************************************************

    efm_processor.h

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

#ifndef EFMPROCESSOR_H
#define EFMPROCESSOR_H

#include <QString>
#include "frame_metadata.h"

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(QString input_fileName, QString output_fileName);
    bool set_qmode_options(bool _qmode_1, bool _qmode_4, bool _qmode_audio, bool _qmode_data, bool _qmode_copy, bool _qmode_nocopy, bool _qmode_nopreemp, bool _qmode_preemp, bool _qmode_2ch, bool _qmode_4ch);
    void set_show_data(bool _showInput, bool _showF1, bool _showF2, bool _showF3);
    void set_input_type(bool _wavInput);
    bool set_corruption(bool _corrupt_tvalues, uint32_t _corrupt_tvalues_frequency, bool _corrupt_start, uint32_t _corrupt_start_symbols,
        bool _corrupt_f3sync, uint32_t _corrupt_f3sync_frequency, bool _corrupt_subcode_sync, uint32_t _corrupt_subcode_sync_frequency);

private:
    FrameMetadata frame_metadata;

    // Show data flags
    bool showInput;
    bool showF1;
    bool showF2;
    bool showF3;

    // Input type flag
    bool is_input_data_wav;

    // Corruption flags
    bool corrupt_tvalues;
    uint32_t corrupt_tvalues_frequency;
    bool corrupt_start;
    uint32_t corrupt_start_symbols;
    bool corrupt_f3sync;
    uint32_t corrupt_f3sync_frequency;
    bool corrupt_subcode_sync;
    uint32_t corrupt_subcode_sync_frequency;
};

#endif // EFMPROCESSOR_H