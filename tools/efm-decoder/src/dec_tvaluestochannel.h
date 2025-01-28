/************************************************************************

    dec_tvaluestochannel.h

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

#ifndef DEC_TVALUESTOCHANNEL_H
#define DEC_TVALUESTOCHANNEL_H

#include "decoders.h"

class TvaluesToChannel : Decoder {
public:
    TvaluesToChannel();
    void push_frame(QByteArray data);
    QString pop_frame();
    bool is_ready() const;
    
    void show_statistics();

private:
    void process_queue();

    QQueue<QByteArray> input_buffer;
    QQueue<QString> output_buffer;
    uint32_t invalid_t_values_count;
    uint32_t valid_t_values_count;
};

#endif // DEC_TVALUESTOCHANNEL_H