/************************************************************************

    sector_processor.h

    ld-efm-sector-decoder - EFM sector decoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-sector-decoder is free software: you can redistribute it and/or
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

#ifndef SECTOR_PROCESSOR_H
#define SECTOR_PROCESSOR_H

#include <QString>
#include <QDebug>

class SectorProcessor
{
public:
    SectorProcessor();

    bool process(const QString &inputFilename, const QString &outputFilename);
};

#endif // EFM_PROCESSOR_H