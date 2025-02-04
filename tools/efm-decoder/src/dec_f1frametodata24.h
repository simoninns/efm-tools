/************************************************************************

    dec_f1frametodata24.h

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

#ifndef DEC_F1FRAMETODATA24_H
#define DEC_F1FRAMETODATA24_H

#include "decoders.h"

class F1FrameToData24 : Decoder {
public:
    F1FrameToData24();
    void push_frame(F1Frame data);
    Data24 pop_frame();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_queue();

    QQueue<F1Frame> input_buffer;
    QQueue<Data24> output_buffer;

    uint32_t invalid_f1_frames_count;
    uint32_t valid_f1_frames_count;
    uint32_t corrupt_bytes_count;

    FrameTime start_time;
    FrameTime end_time;
};

#endif // DEC_F1FRAMETODATA24_H