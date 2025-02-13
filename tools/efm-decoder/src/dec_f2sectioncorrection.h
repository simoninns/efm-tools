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
    void flush();
    
    void show_statistics();

private:
    void process_queue();

    void wait_for_input_to_settle(F2Section& f2_section);
    void waiting_for_section(F2Section& f2_section);
    SectionTime get_expected_absolute_time();

    void correct_internal_buffer();
    void output_sections();

    QQueue<F2Section> input_buffer;
    QQueue<F2Section> leadin_buffer;
    QQueue<F2Section> internal_buffer;
    QQueue<F2Section> output_buffer;

    SectionTime internal_buffer_median_time;

    bool leadin_complete;

    QQueue<F2Section> window;
    uint32_t maximum_gap_size;
    uint32_t maximum_internal_buffer_size;

    // Statistics
    uint32_t total_sections;
    uint32_t corrected_sections;
    uint32_t uncorrectable_sections;
    uint32_t pre_leadin_sections;

    // Time statistics
    SectionTime absolute_start_time;
    SectionTime absolute_end_time;
    QVector<uint8_t> track_numbers;
    QVector<SectionTime> track_start_times;
    QVector<SectionTime> track_end_times;
};

#endif // DEC_F2SECTIONCORRECTION_H