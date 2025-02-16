/************************************************************************

    enc_f2sectiontof3frames.h

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

#ifndef ENC_F2SECTIONTOF3FRAMES_H
#define ENC_F2SECTIONTOF3FRAMES_H

#include "encoders.h"
#include "section.h"
#include "subcode.h"

class F2SectionToF3Frames : Encoder
{
public:
    F2SectionToF3Frames();
    void push_section(F2Section f2_section);
    QVector<F3Frame> pop_frames();
    bool is_ready() const;
    uint32_t validOutputSectionsCount() const override { return valid_f3_frames_count; };

private:
    void process_queue();

    QQueue<F2Section> input_buffer;
    QQueue<QVector<F3Frame>> output_buffer;

    uint32_t valid_f3_frames_count;
};

#endif // ENC_F2SECTIONTOF3FRAMES_H
