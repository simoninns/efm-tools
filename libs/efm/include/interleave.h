/************************************************************************

    interleave.h

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

#ifndef INTERLEAVE_H
#define INTERLEAVE_H

#include <QVector>

class Interleave
{
public:
    Interleave();
    QVector<uint8_t> interleave(QVector<uint8_t> input_data);
    QVector<uint8_t> deinterleave(QVector<uint8_t> input_data);
};

#endif // INTERLEAVE_H