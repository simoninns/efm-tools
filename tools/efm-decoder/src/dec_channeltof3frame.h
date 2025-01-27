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

class ChannelToF3Frame : Decoder {
public:
    ChannelToF3Frame();
    void push_frame(QString data);
    F3Frame pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_input_frames_count() const { return invalid_channel_frames_count; }
    uint32_t get_valid_input_frames_count() const { return valid_channel_frames_count; }

private:
    void process_queue();

    QQueue<QString> input_buffer;
    QQueue<F3Frame> output_buffer;

    QString internal_buffer;

    uint32_t invalid_channel_frames_count;
    uint32_t valid_channel_frames_count;

    // Define the 24-bit sync header for the F3 frame
    const QString sync_header = "100000000001000000000010";

    Efm efm;
};

#endif // DEC_CHANNELTOF3FRAME_H