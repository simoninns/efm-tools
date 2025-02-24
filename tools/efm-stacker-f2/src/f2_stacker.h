/************************************************************************

    f2_stacker.h

    efm-stacker-f2 - EFM F2 Section stacker
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

#ifndef F2_STACKER_H
#define F2_STACKER_H

#include <QString>
#include <QVector>
#include <QDebug>
#include <QFile>

#include "reader_f2section.h"

class F2Stacker
{
public:
    F2Stacker();

    bool process(const QVector<QString> &inputFilenames, const QString &outputFilename);

private:
    QVector<ReaderF2Section*> m_inputFiles;
    QFile m_outputFile;
};

#endif // F2_STACKER_H
