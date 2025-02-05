/************************************************************************

    dec_tvaluestochannel.cpp

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

#include "dec_tvaluestochannel.h"

TvaluesToChannel::TvaluesToChannel() {
    invalid_t_values_count = 0;
    valid_t_values_count = 0;
}

void TvaluesToChannel::push_frame(QByteArray data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

QString TvaluesToChannel::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool TvaluesToChannel::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void TvaluesToChannel::process_queue() {
    // Process the input buffer
    QString bit_string = "";

    while (!input_buffer.isEmpty()) {
        QByteArray t_values = input_buffer.dequeue();
        bit_string += tvalues.tvalues_to_bit_string(t_values);
    }

    if (!bit_string.isEmpty()) {
        // Add the bit string to the output buffer
        output_buffer.enqueue(bit_string);
    }
}

void TvaluesToChannel::show_statistics() {
    qInfo() << "T-values to channel statistics:";
    qInfo() << "  Valid T-values:" << tvalues.get_valid_t_values_count();
    qInfo() << "  Invalid T-values:" << tvalues.get_invalid_high_t_values_count() + tvalues.get_invalid_low_t_values_count();
    qInfo() << "    >11 T-values:" << tvalues.get_invalid_high_t_values_count();
    qInfo() << "    < 3 T-values:" << tvalues.get_invalid_low_t_values_count();
}