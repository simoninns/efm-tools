/************************************************************************

    dec_channeltof3frame.h

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

#ifndef DEC_CHANNELTOF3FRAME_H
#define DEC_CHANNELTOF3FRAME_H

#include "decoders.h"
#include "efm.h"

class ChannelToF3Frame : public Decoder
{
public:
    ChannelToF3Frame();
    void push_frame(QByteArray data);
    F3Frame pop_frame();
    bool is_ready() const;

    void show_statistics();

private:
    void process_queue();
    F3Frame create_f3_frame(QByteArray data);

    QByteArray tvalues_to_data(QByteArray tvalues);
    uint16_t get_bits(QByteArray data, int start_bit, int end_bit);

    Efm efm;

    QQueue<QByteArray> input_buffer;
    QQueue<F3Frame> output_buffer;

    // Statistics
    uint32_t good_frames;
    uint32_t undershoot_frames;
    uint32_t overshoot_frames;
    uint32_t valid_efm_symbols;
    uint32_t invalid_efm_symbols;
    uint32_t valid_subcode_symbols;
    uint32_t invalid_subcode_symbols;
};

#endif // DEC_CHANNELTOF3FRAME_H