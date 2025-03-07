/************************************************************************

    adfs_verifier.cpp

    vfs-verifier - Acorn VFS (Domesday) image verifier
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    This application is free software: you can redistribute it and/or
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

#include "adfs_verifier.h"

AdfsVerifier::AdfsVerifier()
{}

bool AdfsVerifier::process(const QString &filename)
{
    m_image.open(filename);
    if (!m_image.isValid()) {
        qCritical() << "AdfsVerifier::process() - Could not open image file" << filename;
        return false;
    }

    // Read the free space map
    AdfsFsm adfsFsm(m_image.readSectors(0, 2, true));

    // Read the root directory
    AdfsDirectory adfsDirectory(m_image.readSectors(2, 5, false));

    // Close the image
    m_image.close();
    return true;
}

// Note: The FSM is 2 sectors long, 0 and 1
void AdfsVerifier::readFreeSpaceMap(quint64 sector)
{
    // QByteArray sector0 = readSector(0);
    // QByteArray sector1 = readSector(1);
    // // The free space map is from 0x00 to 0xF5 inclusive (sector 0)
    // // Each free space is 3 bytes.

    // // The length of each free space is from 0x00 to 0xF5 inclusive (sector 1)
    // // Each free space length is 3 bytes.
}

