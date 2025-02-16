/************************************************************************

    delay_lines.h

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

#ifndef DELAY_LINES_H
#define DELAY_LINES_H

#include <QVector>

class DelayLine
{
public:
    DelayLine(int32_t _delay_length);
    uint8_t push(uint8_t input_datum);
    bool is_ready();
    void flush();

private:
    uint8_t *buffer;
    bool ready;
    int32_t push_count;
    int32_t delay_length;
};

class DelayLines
{
public:
    DelayLines(QVector<uint32_t> _delay_lengths);
    QVector<uint8_t> push(QVector<uint8_t> input_data);
    bool is_ready();
    void flush();

private:
    QVector<DelayLine> delay_lines;
};

#endif // DELAY_LINES_H