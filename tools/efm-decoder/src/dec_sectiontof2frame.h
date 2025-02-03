/************************************************************************

    dec_sectiontof2frame.h

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

#ifndef DEC_SECTIONTOF2FRAME_H
#define DEC_SECTIONTOF2FRAME_H

#include "decoders.h"
#include "frame_metadata.h"

class SectionToF2Frame : Decoder {
public:
    SectionToF2Frame();
    void push_frame(Section data);
    QVector<F2Frame> pop_frames();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_queue();

    QQueue<Section> input_buffer;
    QQueue<QVector<F2Frame>> output_buffer;

    // Tracking variables
    bool has_last_good_frame;
    F2Frame last_good_frame;

    uint32_t current_track;
    FrameTime current_frame_time;
    FrameTime current_absolute_time;
    uint32_t missed_subcodes;

    // Time statistics
    FrameTime absolute_start_time;
    FrameTime absolute_end_time;

    QVector<uint8_t> track_numbers;
    QVector<FrameTime> track_start_times;
    QVector<FrameTime> track_end_times;
};

#endif // DEC_SECTIONTOF2FRAME_H