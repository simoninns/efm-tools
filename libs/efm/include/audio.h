/************************************************************************

    audio.h

    EFM-library - Audio frame type class
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

#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>
#include <QtGlobal>
#include <QDebug>

// Audio class
class Audio
{
public:
    void setData(const QVector<qint16> &data);
    QVector<qint16> data() const;
    void setErrorData(const QVector<bool> &errorData);
    QVector<bool> errorData() const;
    quint32 countErrors() const;

    bool isFull() const;
    bool isEmpty() const;

    void showData();
    int frameSize() const;

private:
    QVector<qint16> m_audioData;
    QVector<bool> m_audioErrorData;
};

#endif // AUDIO_H