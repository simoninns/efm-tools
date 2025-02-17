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

#include "interleave.h"

Interleave::Interleave() { }

QVector<quint8> Interleave::interleave(const QVector<quint8> &inputData)
{
    // Ensure input data is 24 bytes long
    if (inputData.size() != 24) {
        qDebug() << "Interleave::interleave - Input data must be 24 bytes long";
        return inputData;
    }

    // Interleave the input data
    QVector<quint8> outputData(24);

    outputData[0] = inputData[0];
    outputData[1] = inputData[1];
    outputData[6] = inputData[2];
    outputData[7] = inputData[3];
    outputData[12] = inputData[4];
    outputData[13] = inputData[5];
    outputData[18] = inputData[6];
    outputData[19] = inputData[7];
    outputData[2] = inputData[8];
    outputData[3] = inputData[9];
    outputData[8] = inputData[10];
    outputData[9] = inputData[11];
    outputData[14] = inputData[12];
    outputData[15] = inputData[13];
    outputData[20] = inputData[14];
    outputData[21] = inputData[15];
    outputData[4] = inputData[16];
    outputData[5] = inputData[17];
    outputData[10] = inputData[18];
    outputData[11] = inputData[19];
    outputData[16] = inputData[20];
    outputData[17] = inputData[21];
    outputData[22] = inputData[22];
    outputData[23] = inputData[23];

    return outputData;
}

QVector<quint8> Interleave::deinterleave(const QVector<quint8> &inputData)
{
    // Ensure input data is 24 bytes long
    if (inputData.size() != 24) {
        qDebug() << "Interleave::deinterleave - Input data must be 24 bytes long";
        return inputData;
    }

    // De-Interleave the input data
    QVector<quint8> outputData(24);

    outputData[0] = inputData[0];
    outputData[1] = inputData[1];
    outputData[8] = inputData[2];
    outputData[9] = inputData[3];
    outputData[16] = inputData[4];
    outputData[17] = inputData[5];
    outputData[2] = inputData[6];
    outputData[3] = inputData[7];
    outputData[10] = inputData[8];
    outputData[11] = inputData[9];
    outputData[18] = inputData[10];
    outputData[19] = inputData[11];
    outputData[4] = inputData[12];
    outputData[5] = inputData[13];
    outputData[12] = inputData[14];
    outputData[13] = inputData[15];
    outputData[20] = inputData[16];
    outputData[21] = inputData[17];
    outputData[6] = inputData[18];
    outputData[7] = inputData[19];
    outputData[14] = inputData[20];
    outputData[15] = inputData[21];
    outputData[22] = inputData[22];
    outputData[23] = inputData[23];

    return outputData;
}