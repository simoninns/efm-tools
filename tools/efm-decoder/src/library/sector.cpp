/************************************************************************

    sector.cpp

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

#include "sector.h"

RawSector::RawSector()
    : m_data(QByteArray(2340, 0)) // 2352 bytes - 12 byte sync pattern
{}

void RawSector::pushData(const QByteArray &inData)
{
    m_data = inData;
}

QByteArray RawSector::data() const
{
    return m_data;
}

quint32 RawSector::size() const
{
    return m_data.size();
}

void RawSector::showData()
{
    qDebug() << "RawSector::showData(): Data size:" << m_data.size();
}