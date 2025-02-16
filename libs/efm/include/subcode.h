/************************************************************************

    subcode.h

    EFM-library - Subcode channel functions
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

#ifndef SUBCODE_H
#define SUBCODE_H

#include <cstdint>
#include <QByteArray>
#include <QString>
#include <QDebug>

#include "section_metadata.h"

class Subcode
{
public:
    Subcode() : show_debug(false) { }

    SectionMetadata from_data(const QByteArray &data);
    QByteArray to_data(const SectionMetadata &section_metadata);
    void set_show_debug(bool _show_debug) { show_debug = _show_debug; }

private:
    void set_bit(QByteArray &data, uint8_t bit_position, bool value);
    bool get_bit(const QByteArray &data, uint8_t bit_position);
    bool is_crc_valid(QByteArray q_channel_data);
    uint16_t get_q_channel_crc(QByteArray q_channel_data);
    void set_q_channel_crc(QByteArray &q_channel_data);
    uint16_t calculate_q_channel_crc16(const QByteArray &data);
    bool repair_data(QByteArray &q_channel_data);

    uint8_t int_to_bcd2(uint8_t value);
    uint8_t bcd2_to_int(uint8_t bcd);

    bool show_debug;
};

#endif // SUBCODE_H