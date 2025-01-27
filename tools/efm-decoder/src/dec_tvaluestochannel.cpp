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
    QString bit_string;

    while (!input_buffer.isEmpty()) {
        QByteArray t_values = input_buffer.dequeue();

        for (int32_t i = 0; i < t_values.size(); i++) {
            // Convert the T-value to a bit string
            
            // Range check
            if (static_cast<int>(t_values[i]) > 11) {
                invalid_t_values_count++;
                t_values[i] = 11;
            } else if (static_cast<int>(t_values[i]) < 3) {
                invalid_t_values_count++;
                t_values[i] = 3;
            } else {
                valid_t_values_count++;
            }

            // T3 = 100, T4 = 1000, ... , T11 = 10000000000
            bit_string += "1";
            for (int32_t j = 1; j < t_values[i]; j++) {
                bit_string += "0";
            }   
        }
    }

    if (!bit_string.isEmpty()) {
        // Add the bit string to the output buffer
        output_buffer.enqueue(bit_string);
    }
}