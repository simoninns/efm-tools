/************************************************************************

    dec_f2frametof1frame.h

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

#ifndef DEC_F2FRAMETOF1FRAME_H
#define DEC_F2FRAMETOF1FRAME_H

#include "decoders.h"
#include "reedsolomon.h"
#include "delay_lines.h"
#include "interleave.h"
#include "inverter.h"

class F2FrameToF1Frame : Decoder {
public:
    F2FrameToF1Frame();
    void push_frame(F2Frame data);
    F1Frame pop_frame();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_queue();

    QQueue<F2Frame> input_buffer;
    QQueue<F1Frame> output_buffer;

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

    uint32_t invalid_f2_frames_count;
    uint32_t valid_f2_frames_count;
};

#endif // DEC_F2FRAMETOF1FRAME_H