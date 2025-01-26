/************************************************************************

    delay_lines.cpp

    EFM-library - Delay line functions
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

#include <QVector>
#include <QQueue>
#include <QtGlobal>
#include <QDebug>
#include <iostream>

#include "delay_lines.h"

DelayLines::DelayLines(QVector<uint32_t> _delay_lengths) {
    for (int32_t i = 0; i < _delay_lengths.size(); i++) {
        delay_lines.append(DelayLine(_delay_lengths[i]));
    }
}

QVector<uint8_t> DelayLines::push(QVector<uint8_t> input_data) {
    if (input_data.size() != delay_lines.size()) {
        qFatal("Input data size does not match the number of delay lines.");
    }

    QVector<uint8_t> output_data;
    for (int32_t i = 0; i < delay_lines.size(); i++) {
        output_data.append(delay_lines[i].push(input_data[i]));
    }

    // Check if the delay lines are ready and, if not, return an empty vector
    if (!is_ready()) {
        return QVector<uint8_t>{};
    }

    return output_data;
}

bool DelayLines::is_ready() {
    for (int32_t i = 0; i < delay_lines.size(); i++) {
        if (!delay_lines[i].is_ready()) {
            return false;
        }
    }
    return true;
}

void DelayLines::flush() {
    for (int32_t i = 0; i < delay_lines.size(); i++) {
        delay_lines[i].flush();
    }
}

// DelayLine class implementation
DelayLine::DelayLine(int32_t _delay_length) {
    buffer = new uint8_t[_delay_length];
    delay_length = _delay_length;
    flush();
}

uint8_t DelayLine::push(uint8_t input_datum) {
    if (delay_length == 0) {
        return input_datum;
    }

    uint8_t output_datum = buffer[0];
    std::copy(buffer + 1, buffer + delay_length, buffer);
    buffer[delay_length - 1] = input_datum;

    // Check if the delay line is ready
    if (push_count >= delay_length) {
        ready = true;
    } else {
        push_count++;
    }

    return output_datum;
}

bool DelayLine::is_ready() {
    return ready;
}

void DelayLine::flush() {
    if (delay_length > 0) {
        std::fill(buffer, buffer + delay_length, 0);
        ready = false;
    } else {
        ready = true;
    }
    push_count = 0;
}