/************************************************************************

    delay_lines.h

    EFM-library - Delay line functions
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

#ifndef DELAY_LINES_H
#define DELAY_LINES_H

#include <QVector>
#include <QQueue>
#include <QtGlobal>
#include <QDebug>

class DelayLine
{
public:
    DelayLine(qint32 _delayLength);
    quint8 push(quint8 inputDatum);
    bool isReady();
    void flush();

private:
    quint8 *m_buffer;
    bool m_ready;
    qint32 m_pushCount;
    qint32 m_delayLength;
};

class DelayLines
{
public:
    DelayLines(QVector<qint32> _delayLengths);
    QVector<quint8> push(QVector<quint8> inputData);
    QVector<bool> push(QVector<bool> inputData);
    bool isReady();
    void flush();

private:
    QVector<DelayLine> m_delayLines;
};

#endif // DELAY_LINES_H