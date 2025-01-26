/************************************************************************

    efm_processor.cpp

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

#include <QDebug>
#include <QString>
#include <QFile>

#include "efm_processor.h"
#include "data24.h"
#include "encoders.h"

#include "delay_lines.h"

EfmProcessor::EfmProcessor() {}

bool EfmProcessor::process(QString input_filename, QString output_filename) {
    qInfo() << "Encoding EFM data from file:" << input_filename << "to file:" << output_filename;

    Data24 data24(input_filename, is_input_data_wav);
    if (!data24.is_ready()) {
        qDebug() << "EfmProcessor::process(): Failed to load data file:" << input_filename;
        return false;
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmProcessor::process(): Failed to open output file:" << output_filename;
        return false;
    }

    // Audio data
    QVector<uint8_t> data24_frame;
    uint32_t data24_count = 0;

    // Prepare the encoders
    Data24ToF1Frame data24_to_f1;
    F1FrameToF2Frame f1_frame_to_f2;
    F2FrameToSection f2_frame_to_section;
    SectionToF3Frame section_to_f3;
    F3FrameToChannel f3_frame_to_channel;

    uint32_t f1_frame_count = 0;
    uint32_t f2_frame_count = 0;
    uint32_t f3_frame_count = 0;
    uint32_t section_count = 0;
    uint32_t channel_byte_count = 0;

    // Since we don't really support multi-track yet, we'll just use a single track
    // with a single absolute frame time
    FrameTime frame_time; // 0:00:00 default
    uint8_t track_number = 1;

    // Process the input audio data 24 bytes at a time
    while (!(data24_frame = data24.read()).isEmpty()) {
        data24_count += 1;

        if (showInput) data24.show_data();

        // Push the data to the first converter
        data24_to_f1.push_frame(data24_frame, frame_time, F1Frame::USER_DATA, track_number);

        // Increment the frame time by one frame (1/75th of a second)
        // every 98 data24 frames.  98 Frames is one section and each
        // section represents 1/75th of a second.
        if (data24_count % 98 == 0) {
            frame_time.increment_frame();
        }

        // Are there any F1 frames ready?
        if (data24_to_f1.is_ready()) {
            // Pop the F1 frame, count it and push it to the next converter
            F1Frame f1_frame = data24_to_f1.pop_frame();
            if (showF1) f1_frame.show_data();
            f1_frame_count++;
            f1_frame_to_f2.push_frame(f1_frame);
        }

        // Are there any F2 frames ready?
        if (f1_frame_to_f2.is_ready()) {
            // Pop the F2 frame, count it and push it to the next converter
            F2Frame f2_frame = f1_frame_to_f2.pop_frame();
            if (showF2) f2_frame.show_data();
            f2_frame_count++;
            f2_frame_to_section.push_frame(f2_frame);
        }

        // Are there any sections ready?
        if (f2_frame_to_section.is_ready()) {
            // Pop the section, count it and push it to the next converter
            Section section = f2_frame_to_section.pop_section();
            section_count++;
            section_to_f3.push_section(section);
        }

        // Are there any F3 frames ready?
        if (section_to_f3.is_ready()) {
            // Pop the section, count it and push it to the next converter
            QVector<F3Frame> f3_frames = section_to_f3.pop_frames();

            for (int i = 0; i < f3_frames.size(); i++) {
                if (showF3) f3_frames[i].show_data();
                f3_frame_count++;
                f3_frame_to_channel.push_frame(f3_frames[i]);
            }
        }

        // Is there any channel data ready?
        if (f3_frame_to_channel.is_ready()) {
            // Pop the channel data, count it and write it to the output file
            QVector<uint8_t> channel_data = f3_frame_to_channel.pop_frame();
            channel_byte_count += channel_data.size();

            // Write the channel data to the output file
            output_file.write(reinterpret_cast<const char*>(channel_data.data()), channel_data.size());
        }
    }

    // Close the output file
    output_file.close();

    // Output some statistics
    double total_bytes = data24_count * 24;
    QString size_unit;
    double size_value;

    if (total_bytes < 1024) {
        size_unit = "bytes";
        size_value = total_bytes;
    } else if (total_bytes < 1024 * 1024) {
        size_unit = "Kbytes";
        size_value = total_bytes / 1024;
    } else {
        size_unit = "Mbytes";
        size_value = total_bytes / (1024 * 1024);
    }

    qInfo().noquote() << "Processed" << data24_count << "data24 frames totalling" << size_value << size_unit;
    qInfo().noquote() << "Final time was" << frame_time.to_string();

    qInfo() << f3_frame_to_channel.get_total_t_values() << "T-values," << channel_byte_count << "channel bytes";
    qInfo() << f1_frame_count << "F1 frames," << f2_frame_count << "F2 frames," << section_count << "Sections," << f3_frame_count << "F3 frames,";
    qInfo() << "Encoding complete";

    return true;
}

void EfmProcessor::set_show_data(bool _showInput, bool _showF1, bool _showF2, bool _showF3) {
    showInput = _showInput;
    showF1 = _showF1;
    showF2 = _showF2;
    showF3 = _showF3;
}

// Set the input data type (true for WAV, false for raw)
void EfmProcessor::set_input_type(bool _wavInput) {
    is_input_data_wav = _wavInput;
}   