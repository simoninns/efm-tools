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
    // Initialise the hash table
    for (uint16_t i = 0; i < 256 + 2; ++i) {
        efmHash.insert(efm_lut[i], i);
    }
}

// Note: There are 257 EFM symbols: 0 to 255 and two additional sync0 and sync1 symbols
// A value of 300 is returned for an invalid EFM symbol
uint16_t Efm::fourteen_to_eight(uint16_t efm) {
    // Find the value efm in the efm_lut array
    for (uint16_t i = 0; i < 256 + 2; ++i) {
        if (efm_lut[i] == efm) {
            return i;
        }
    }

    //qDebug().noquote() << "Efm::fourteen_to_eight(): EFM symbol not found -" << efm;
    return 300;
}

uint16_t Efm::eight_to_fourteen(uint16_t value) {
    if (value < 258) {
        return efm_lut[value];
    } else {
        qFatal("Efm::eight_to_fourteen(): Value must be in the range 0 to 257.");
    }
}