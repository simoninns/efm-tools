/************************************************************************

    dec_f2sectiontof1section.h

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

#ifndef DEC_F2SECTIONTOF1SECTION_H
#define DEC_F2SECTIONTOF1SECTION_H

#include "decoders.h"
#include "reedsolomon.h"
#include "delay_lines.h"
#include "interleave.h"
#include "inverter.h"

class F2SectionToF1Section : public Decoder {
public:
    F2SectionToF1Section();
    void push_section(F2Section f2_section);
    F1Section pop_section();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_queue();

    QQueue<F2Section> input_buffer;
    QQueue<F1Section> output_buffer;

    ReedSolomon circ;

    DelayLines delay_line1;
    DelayLines delay_line2;
    DelayLines delay_lineM;

    DelayLines delay_line1_err;
    DelayLines delay_line2_err;
    DelayLines delay_lineM_err;

    Interleave interleave;
    Inverter inverter;

    Interleave interleave_err;

    uint32_t invalid_input_f2_frames_count;
    uint32_t valid_input_f2_frames_count;
    uint32_t invalid_output_f1_frames_count;
    uint32_t valid_output_f1_frames_count;

    uint32_t input_byte_errors;
    uint32_t output_byte_errors;
};

#endif // DEC_F2SECTIONTOF1SECTION_H