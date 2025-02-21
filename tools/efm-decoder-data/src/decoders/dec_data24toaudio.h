/************************************************************************

    dec_data24toaudio.h

    ld-efm-decoder - EFM data decoder
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

#ifndef DEC_DATA24TOAUDIO_H
#define DEC_DATA24TOAUDIO_H

#include "decoders.h"
#include "section.h"

class Data24ToAudio : public Decoder
{
public:
    Data24ToAudio();
    void pushSection(const Data24Section &data24Section);
    AudioSection popSection();
    bool isReady() const;

    void showStatistics();

private:
    void processQueue();

    QQueue<Data24Section> m_inputBuffer;
    QQueue<AudioSection> m_outputBuffer;

    // Statistics
    quint32 m_invalidData24FramesCount;
    quint32 m_validData24FramesCount;
    quint32 m_invalidSamplesCount;
    quint32 m_validSamplesCount;

    SectionTime m_startTime;
    SectionTime m_endTime;
};

#endif // DEC_DATA24TOAUDIO_H