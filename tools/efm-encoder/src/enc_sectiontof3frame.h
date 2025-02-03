/************************************************************************

    enc_sectiontof3frame.h

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

#ifndef ENC_SECTIONTOF3FRAME_H
#define ENC_SECTIONTOF3FRAME_H

#include "encoders.h"
#include "section.h"
#include "subcode.h"

class SectionToF3Frame : Encoder {
public:
    SectionToF3Frame();
    void push_section(Section section);
    QVector<F3Frame> pop_frames();
    bool is_ready() const;
    uint32_t get_valid_output_frames_count() const override { return valid_f3_frames_count; };

private:
    void process_queue();

    QQueue<Section> input_buffer;
    QQueue<QVector<F3Frame>> output_buffer;

    uint32_t valid_f3_frames_count;
};

#endif // ENC_SECTIONTOF3FRAME_H
