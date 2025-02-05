/************************************************************************

    tvalues.h

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

#ifndef TVALUES_H
#define TVALUES_H

#include <cstdint>
#include <QString>
#include <QVector>

class Tvalues {
public:
    Tvalues();
    
    QString tvalues_to_bit_string(QByteArray tvalues);

    uint32_t get_invalid_high_t_values_count() const { return invalid_high_t_values_count; }
    uint32_t get_invalid_low_t_values_count() const { return invalid_low_t_values_count; }
    uint32_t get_valid_t_values_count() const { return valid_t_values_count; }

private:
    uint32_t invalid_high_t_values_count;
    uint32_t invalid_low_t_values_count;
    uint32_t valid_t_values_count;
};

#endif // EFM_H