/************************************************************************

    dec_data24toaudio.h

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

#ifndef DEC_DATA24TOAUDIO_H
#define DEC_DATA24TOAUDIO_H

#include "decoders.h"
#include "section.h"

class Data24ToAudio : public Decoder
{
public:
    Data24ToAudio();
    void push_section(Data24Section f1_section);
    AudioSection pop_section();
    bool is_ready() const;

    void show_statistics();

private:
    void process_queue();

    QQueue<Data24Section> input_buffer;
    QQueue<AudioSection> output_buffer;

    // Statistics
    uint32_t invalid_data24_frames_count;
    uint32_t valid_data24_frames_count;
    uint32_t invalid_samples_count;
    uint32_t valid_samples_count;

    SectionTime start_time;
    SectionTime end_time;
};

#endif // DEC_DATA24TOAUDIO_H