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

class SectionToF2Frame : Decoder {
public:
    SectionToF2Frame();
    void push_frame(Section data);
    QVector<F2Frame> pop_frames();
    bool is_ready() const;
    uint32_t get_invalid_input_frames_count() const { return invalid_sections_count; }
    uint32_t get_valid_input_frames_count() const { return valid_sections_count; }

private:
    void process_queue();

    QQueue<Section> input_buffer;
    QQueue<QVector<F2Frame>> output_buffer;

    uint32_t invalid_sections_count;
    uint32_t valid_sections_count;
};

#endif // DEC_SECTIONTOF2FRAME_H