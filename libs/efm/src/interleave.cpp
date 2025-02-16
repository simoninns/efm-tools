/************************************************************************

    interleave.cpp

    EFM-library - Data interleaving functions
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
#include <QDebug>
#include "interleave.h"

Interleave::Interleave() { }

QVector<uint8_t> Interleave::interleave(QVector<uint8_t> input_data)
{
    // Ensure input data is 24 bytes long
    if (input_data.size() != 24) {
        qDebug() << "Interleave::interleave - Input data must be 24 bytes long";
        return input_data;
    }

    // Interleave the input data
    QVector<uint8_t> output_data;
    output_data.resize(24);

    output_data[0] = input_data[0];
    output_data[1] = input_data[1];
    output_data[6] = input_data[2];
    output_data[7] = input_data[3];
    output_data[12] = input_data[4];
    output_data[13] = input_data[5];
    output_data[18] = input_data[6];
    output_data[19] = input_data[7];
    output_data[2] = input_data[8];
    output_data[3] = input_data[9];
    output_data[8] = input_data[10];
    output_data[9] = input_data[11];
    output_data[14] = input_data[12];
    output_data[15] = input_data[13];
    output_data[20] = input_data[14];
    output_data[21] = input_data[15];
    output_data[4] = input_data[16];
    output_data[5] = input_data[17];
    output_data[10] = input_data[18];
    output_data[11] = input_data[19];
    output_data[16] = input_data[20];
    output_data[17] = input_data[21];
    output_data[22] = input_data[22];
    output_data[23] = input_data[23];

    return output_data;
}

QVector<uint8_t> Interleave::deinterleave(QVector<uint8_t> input_data)
{
    // Ensure input data is 24 bytes long
    if (input_data.size() != 24) {
        qDebug() << "Interleave::deinterleave - Input data must be 24 bytes long";
        return input_data;
    }

    // De-Interleave the input data
    QVector<uint8_t> output_data;
    output_data.resize(24);

    output_data[0] = input_data[0];
    output_data[1] = input_data[1];
    output_data[8] = input_data[2];
    output_data[9] = input_data[3];
    output_data[16] = input_data[4];
    output_data[17] = input_data[5];
    output_data[2] = input_data[6];
    output_data[3] = input_data[7];
    output_data[10] = input_data[8];
    output_data[11] = input_data[9];
    output_data[18] = input_data[10];
    output_data[19] = input_data[11];
    output_data[4] = input_data[12];
    output_data[5] = input_data[13];
    output_data[12] = input_data[14];
    output_data[13] = input_data[15];
    output_data[20] = input_data[16];
    output_data[21] = input_data[17];
    output_data[6] = input_data[18];
    output_data[7] = input_data[19];
    output_data[14] = input_data[20];
    output_data[15] = input_data[21];
    output_data[22] = input_data[22];
    output_data[23] = input_data[23];

    return output_data;
}