/************************************************************************

    efm.h

    EFM-library - EFM Frame type classes
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

#include "efm.h"

Efm::Efm() {
}

// Note: There are 257 EFM symbols: 0 to 255 and two additional sync0 and sync1 symbols
uint16_t Efm::fourteen_to_eight(QString efm) {
    // Convert the EFM symbol to an 8-bit value
    uint16_t index = efm_lut.indexOf(efm);
    if (index == -1) {
        qFatal("Efm::fourteen_to_eight(): EFM symbol not found.");
    }
    return index;
}

QString Efm::eight_to_fourteen(uint16_t value) {
    if (value < 258) {
        return efm_lut[value];
    } else {
        qFatal("Efm::eight_to_fourteen(): Value must be in the range 0 to 257.");
    }
}