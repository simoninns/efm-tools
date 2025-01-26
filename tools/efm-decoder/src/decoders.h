/************************************************************************

    decoders.h

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

#ifndef DECODERS_H
#define DECODERS_H

#include <QVector>
#include <QQueue>
#include <QByteArray>
#include <cstdint>

#include "frame.h"
#include "delay_lines.h"
#include "reedsolomon.h"
#include "interleave.h"
#include "inverter.h"
#include "section.h"

class TvaluesToChannel {
public:
    TvaluesToChannel();
    void push_frame(QByteArray data);
    QString pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_t_values_count() const { return invalid_t_values_count; }
    uint32_t get_valid_t_values_count() const { return valid_t_values_count; }

private:
    void process_queue();

    QQueue<QByteArray> input_buffer;
    QQueue<QString> output_buffer;
    uint32_t invalid_t_values_count;
    uint32_t valid_t_values_count;
};

class ChannelToF3Frame {
public:
    ChannelToF3Frame();
    void push_frame(QString data);
    F3Frame pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_channel_frames_count() const { return invalid_channel_frames_count; }
    uint32_t get_valid_channel_frames_count() const { return valid_channel_frames_count; }

private:
    void process_queue();
    uint16_t convert_efm_to_8bit(QString efm);

    QQueue<QString> input_buffer;
    QQueue<F3Frame> output_buffer;

    QString internal_buffer;

    uint32_t invalid_channel_frames_count;
    uint32_t valid_channel_frames_count;

    static const QString sync_header;
    static const QStringList efm_lut;
};

class F3FrameToSection {
public:
    F3FrameToSection();
    void push_frame(F3Frame data);
    Section pop_section();
    bool is_ready() const;
    uint32_t get_invalid_f3_frames_count() const { return invalid_f3_frames_count; }
    uint32_t get_valid_f3_frames_count() const { return valid_f3_frames_count; }

private:
    void process_queue();

    QQueue<F3Frame> input_buffer;
    QQueue<Section> output_buffer;

    uint32_t invalid_f3_frames_count;
    uint32_t valid_f3_frames_count;
};

class SectionToF2Frame {
public:
    SectionToF2Frame();
    void push_frame(Section data);
    QVector<F2Frame> pop_frames();
    bool is_ready() const;
    uint32_t get_invalid_sections_count() const { return invalid_sections_count; }
    uint32_t get_valid_sections_count() const { return valid_sections_count; }

private:
    void process_queue();

    QQueue<Section> input_buffer;
    QQueue<QVector<F2Frame>> output_buffer;

    uint32_t invalid_sections_count;
    uint32_t valid_sections_count;
};

class F2FrameToF1Frame {
public:
    F2FrameToF1Frame();
    void push_frame(F2Frame data);
    F1Frame pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_f2_frames_count() const { return invalid_f2_frames_count; }
    uint32_t get_valid_f2_frames_count() const { return valid_f2_frames_count; }

    void get_c1_circ_stats(int32_t &valid_c1s, int32_t &fixed_c1s, int32_t &error_c1s);
    void get_c2_circ_stats(int32_t &valid_c2s, int32_t &fixed_c2s, int32_t &error_c2s);

private:
    void process_queue();

    QQueue<F2Frame> input_buffer;
    QQueue<F1Frame> output_buffer;

    ReedSolomon circ;

    DelayLines delay_line1;
    DelayLines delay_line2;
    DelayLines delay_lineM;

    Interleave interleave;
    Inverter inverter;

    uint32_t invalid_f2_frames_count;
    uint32_t valid_f2_frames_count;
};

class F1FrameToData24 {
public:
    F1FrameToData24();
    void push_frame(F1Frame data);
    QByteArray pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_f1_frames_count() const { return invalid_f1_frames_count; }
    uint32_t get_valid_f1_frames_count() const { return valid_f1_frames_count; }

private:
    void process_queue();

    QQueue<F1Frame> input_buffer;
    QQueue<QByteArray> output_buffer;

    uint32_t invalid_f1_frames_count;
    uint32_t valid_f1_frames_count;
};

#endif // DECODERS_H