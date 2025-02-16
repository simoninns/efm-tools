/************************************************************************

    dec_audiocorrection.h

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

#ifndef DEC_AUDIOCORRECTION_H
#define DEC_AUDIOCORRECTION_H

#include "decoders.h"
#include "section.h"

class AudioCorrection : public Decoder
{
public:
    AudioCorrection();
    void push_section(AudioSection audio_section);
    AudioSection pop_section();
    bool is_ready() const;

    void show_statistics();

private:
    void process_queue();

    QQueue<AudioSection> input_buffer;
    QQueue<AudioSection> output_buffer;

    // Statistics
    uint32_t concealed_samples_count;
    uint32_t silenced_samples_count;
    uint32_t valid_samples_count;

    int16_t last_section_left_sample;
    int16_t last_section_right_sample;
    int16_t last_section_left_error;
    int16_t last_section_right_error;
};

#endif // DEC_AUDIOCORRECTION_H