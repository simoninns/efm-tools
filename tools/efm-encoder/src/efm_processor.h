/************************************************************************

    efm_processor.h

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#ifndef EFMPROCESSOR_H
#define EFMPROCESSOR_H

#include <QString>
#include <QDebug>
#include <QFile>

#include "section_metadata.h"
#include "section.h"

#include "encoders.h"
#include "enc_data24sectiontof1section.h"
#include "enc_f1sectiontof2section.h"
#include "enc_f2sectiontof3frames.h"
#include "enc_f3frametochannel.h"

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(const QString &inputFileName, const QString &outputFileName);
    bool setQmodeOptions(bool qmode1, bool qmode4, bool qmodeAudio, bool qmodeData,
                         bool qmodeCopy, bool qmodeNocopy, bool qmodeNopreemp,
                         bool qmodePreemp, bool qmode2ch, bool qmode4ch);
    void setShowData(bool showInput, bool showF1, bool showF2, bool showF3);
    void setInputType(bool wavInput);
    bool setCorruption(bool corruptTvalues, quint32 corruptTvaluesFrequency,
                       bool corruptStart, quint32 corruptStartSymbols, bool corruptF3sync,
                       quint32 corruptF3syncFrequency, bool corruptSubcodeSync,
                       quint32 corruptSubcodeSyncFrequency);

private:
    SectionMetadata m_sectionMetadata;

    // Show data flags
    bool m_showInput;
    bool m_showF1;
    bool m_showF2;
    bool m_showF3;

    // Input type flag
    bool m_isInputDataWav;

    // Corruption flags
    bool m_corruptTvalues;
    quint32 m_corruptTvaluesFrequency;
    bool m_corruptStart;
    quint32 m_corruptStartSymbols;
    bool m_corruptF3sync;
    quint32 m_corruptF3syncFrequency;
    bool m_corruptSubcodeSync;
    quint32 m_corruptSubcodeSyncFrequency;
};

#endif // EFMPROCESSOR_H