/************************************************************************

    tvalues.cpp

    EFM-library - T-values to bit string conversion
    Copyright (C) 2025 Simon Inns

    This file is part of EFM-Tools.

    This is free software: you can redistribute it and/or
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

#include "tvalues.h"

Tvalues::Tvalues() {
    invalid_high_t_values_count = 0;
    invalid_low_t_values_count = 0;
    valid_t_values_count = 0;
}

QString Tvalues::tvalues_to_bit_string(QByteArray t_values) {
    QString bit_string;

    // For every T-value in the input array reserve 11 bits in the output bit string
    // Note: This is just to increase speed
    bit_string.reserve(t_values.size() * 11);

    for (int32_t i = 0; i < t_values.size(); i++) {
        // Convert the T-value to a bit string
        
        // Range check
        int t_value = static_cast<int>(t_values[i]);
        if (t_value > 11) {
            invalid_high_t_values_count++;
            t_value = 11;
        } else if (t_value < 3) {
            invalid_low_t_values_count++;
            t_value = 3;
        } else {
            valid_t_values_count++;
        }

        // T3 = 100, T4 = 1000, ... , T11 = 10000000000
        bit_string += '1';
        bit_string += QString(t_value - 1, '0');
    }

    return bit_string;
}
