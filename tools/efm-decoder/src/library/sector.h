/************************************************************************

    sector.h

    EFM-library - EFM Section classes
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

#ifndef SECTOR_H
#define SECTOR_H

#include <QDebug>
#include <QByteArray>

class RawSector
{
public:
    RawSector();
    void pushData(const QByteArray &inData);
    void pushErrorData(const QByteArray &inData);
    QByteArray data() const;
    QByteArray errorData() const;
    quint32 size() const;
    void showData();

    //SectorMetadata metadata;

private:
    QByteArray m_data;
    QByteArray m_errorData;
};

#endif // SECTOR_H
