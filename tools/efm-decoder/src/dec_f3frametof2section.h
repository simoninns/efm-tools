/************************************************************************

    dec_f3frametof2section.h

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

#ifndef DEC_F3FRAMETOF2SECTION_H
#define DEC_F3FRAMETOF2SECTION_H

#include "decoders.h"
#include "section.h"
#include "subcode.h"

class F3FrameToF2Section : public Decoder {
public:
    F3FrameToF2Section();
    void push_frame(F3Frame data);
    F2Section pop_section();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_state_machine();

    QQueue<F3Frame> input_buffer;
    QQueue<F2Section> output_buffer;

    // State machine states
    enum State {
        EXPECTING_SYNC0,
        EXPECTING_SYNC1,
        EXPECTING_SUBCODE,
        PROCESS_SECTION
    };

    State current_state;
    QVector<F3Frame> section_buffer;

    // State machine state processing functions
    State expecting_sync0();
    State expecting_sync1();
    State expecting_subcode();
    State process_section();

    // Statistics
    uint32_t missed_sync0s;
    uint32_t missed_sync1s;
    uint32_t missed_subcodes;
    uint32_t valid_sections;
    uint32_t invalid_sections;
    uint32_t input_f3_frames;
};

#endif // DEC_F3FRAMETOF2SECTION_H