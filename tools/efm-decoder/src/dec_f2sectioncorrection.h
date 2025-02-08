/************************************************************************

    dec_f2sectioncorrection.h

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

#ifndef DEC_F2SECTIONCORRECTION_H
#define DEC_F2SECTIONCORRECTION_H

#include "decoders.h"
#include "section_metadata.h"

class F2SectionCorrection : public Decoder {
public:
    F2SectionCorrection();
    void push_section(F2Section data);
    F2Section pop_section();
    bool is_ready() const;
    
    void show_statistics();
    void correct_invalid_sections(F2Section next_good_section);

private:
    void process_queue();
    bool is_section_valid(F2Section current, F2Section last_good, uint32_t window_size);
    void output_section(F2Section section);
    void flush_window();
    void correct_and_flush_window();

    QQueue<F2Section> input_buffer;
    QQueue<F2Section> output_buffer;

    bool has_last_good_section;
    F2Section last_good_section;
    QQueue<F2Section> window;
    uint32_t window_size;

    // Statistics
    uint32_t total_sections;
    uint32_t corrected_sections;
    uint32_t uncorrectable_sections;

    // Time statistics
    SectionTime absolute_start_time;
    SectionTime absolute_end_time;
    uint32_t current_track;
    SectionTime current_section_time;
    SectionTime current_absolute_time;
    QVector<uint8_t> track_numbers;
    QVector<SectionTime> track_start_times;
    QVector<SectionTime> track_end_times;
};

#endif // DEC_F2SECTIONCORRECTION_H