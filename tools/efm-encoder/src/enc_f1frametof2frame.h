/************************************************************************

    enc_f1frametof2frame.h

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

#ifndef ENC_F1FRAMETOF2FRAME_H
#define ENC_F1FRAMETOF2FRAME_H

#include "encoders.h"
#include "delay_lines.h"
#include "interleave.h"
#include "inverter.h"
#include "reedsolomon.h"

class F1FrameToF2Frame : Encoder {
public:
    F1FrameToF2Frame();
    void push_frame(F1Frame f1_frame);
    F2Frame pop_frame();
    bool is_ready() const;
    uint32_t get_valid_output_frames_count() const override { return valid_f2_frames_count; };

private:
    void process_queue();

    QQueue<F1Frame> input_buffer;
    QQueue<F2Frame> output_buffer;

    ReedSolomon circ;

    DelayLines delay_line1;
    DelayLines delay_line2;
    DelayLines delay_lineM;

    Interleave interleave;
    Inverter inverter;

    uint32_t valid_f2_frames_count;
};

#endif // ENC_F1FRAMETOF2FRAME_H
