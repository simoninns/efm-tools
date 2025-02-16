/************************************************************************

    dec_f1sectiontodata24section.h

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

#ifndef DEC_F1SECTIONTODATA24SECTION_H
#define DEC_F1SECTIONTODATA24SECTION_H

#include "decoders.h"
#include "section.h"

class F1SectionToData24Section : public Decoder
{
public:
    F1SectionToData24Section();
    void push_section(F1Section f1_section);
    Data24Section pop_section();
    bool is_ready() const;

    void show_statistics();

private:
    void process_queue();

    QQueue<F1Section> input_buffer;
    QQueue<Data24Section> output_buffer;

    uint32_t invalid_f1_frames_count;
    uint32_t valid_f1_frames_count;
    uint32_t corrupt_bytes_count;
};

#endif // DEC_F1SECTIONTODATA24SECTION_H