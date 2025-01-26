/************************************************************************

    efm_processor.cpp

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

#include <QDebug>
#include <QString>
#include <QFile>

#include "efm_processor.h"
#include "decoders.h"

EfmProcessor::EfmProcessor() {
}

bool EfmProcessor::process(QString input_filename, QString output_filename) {
    qDebug() << "EfmProcessor::process(): Decoding EFM from file: " << input_filename << " to file: " << output_filename;

    // Prepare the input file
    QFile input_file(input_filename);

    if (!input_file.open(QIODevice::ReadOnly)) {
        qDebug() << "EfmProcessor::process(): Failed to open input file: " << input_filename;
        return false;
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmEncoder::process(): Failed to open output file: " << output_filename;
        return false;
    }

    // If we are outputting to a WAV file, we need to leave space for the header
    if (is_output_data_wav) {
        qDebug() << "EfmProcessor::process(): Outputting to WAV file, adding header";
        QByteArray header(44, 0);
        output_file.write(header);
    }

    // Prepare the decoders
    TvaluesToChannel t_values_to_channel;
    ChannelToF3Frame channel_to_f3;
    F3FrameToSection f3_frame_to_section;
    SectionToF2Frame section_to_f2;
    F2FrameToF1Frame f2_frame_to_f1;
    F1FrameToData24 f1_frame_to_data24;

    uint32_t data24_count = 0;
    uint32_t f1_frame_count = 0;
    uint32_t f2_frame_count = 0;
    uint32_t f3_frame_count = 0;
    uint32_t section_count = 0;

    // Process the EFM data in chunks of 100 T-values
    bool end_of_data = false;
    while (!end_of_data) {
        // Read 100 T-values from the input file
        QByteArray t_values = input_file.read(100);
        if (t_values.isEmpty()) {
            end_of_data = true;
            qDebug() << "EfmEncoder::process(): End of data";
        } else {
            t_values_to_channel.push_frame(t_values);
        }

        // Are there any T-values ready?
        if (t_values_to_channel.is_ready()) {
            QString channel_data = t_values_to_channel.pop_frame();
            channel_to_f3.push_frame(channel_data);
        }

        // Are there any F3 frames ready?
        if (channel_to_f3.is_ready()) {
            F3Frame f3_frame = channel_to_f3.pop_frame();
            if (showF3) f3_frame.show_data();
            f3_frame_to_section.push_frame(f3_frame);
            f3_frame_count++;
        }

        // Are there any sections ready?
        if (f3_frame_to_section.is_ready()) {
            Section section = f3_frame_to_section.pop_section();
            section_to_f2.push_frame(section);
            section_count++;
        }

        // Are there any F2 frames ready?
        if (section_to_f2.is_ready()) {
            QVector<F2Frame> f2_frames = section_to_f2.pop_frames();

            for (int i = 0; i < f2_frames.size(); i++) {
                if (showF2) f2_frames[i].show_data();
                f2_frame_to_f1.push_frame(f2_frames[i]);
                f2_frame_count++;
            }
        }

        // Are there any F1 frames ready?
        if (f2_frame_to_f1.is_ready()) {
            F1Frame f1_frame = f2_frame_to_f1.pop_frame();
            if (showF1) f1_frame.show_data();
            f1_frame_to_data24.push_frame(f1_frame);
            f1_frame_count++;
        }

        // Are there any data frames ready?
        if (f1_frame_to_data24.is_ready()) {
            QByteArray data = f1_frame_to_data24.pop_frame();
            output_file.write(data);
            data24_count += 1;

            if (showOutput) {
                QString dataString;
                for (int i = 0; i < data.size(); ++i) {
                    uint8_t byte = static_cast<uint8_t>(data[i]);
                    dataString.append(QString("%1 ").arg(byte, 2, 16, QChar('0')));
                }
                qInfo().noquote() << "Output data:" << dataString.trimmed();
            }
        }
    }

    // Show summary
    qInfo() << "Decoding complete";
    qInfo() << "Processed" << t_values_to_channel.get_valid_t_values_count() << "Valid T-Values and" << t_values_to_channel.get_invalid_t_values_count() << "Invalid T-Values";
    qInfo() << "Processed" << channel_to_f3.get_valid_channel_frames_count() << "Valid Channel Frames and" << channel_to_f3.get_invalid_channel_frames_count() << "Invalid Channel Frames";
    qInfo() << "Processed" << f3_frame_to_section.get_valid_f3_frames_count() << "Valid F3 Frames and" << f3_frame_to_section.get_invalid_f3_frames_count() << "Invalid F3 Frames";
    qInfo() << "Processed" << section_to_f2.get_valid_sections_count() << "Valid sections and" << section_to_f2.get_invalid_sections_count() << "Invalid sections";
    qInfo() << "Processed" << f2_frame_to_f1.get_valid_f2_frames_count() << "Valid F2 Frames and" << f2_frame_to_f1.get_invalid_f2_frames_count() << "Invalid F2 Frames";
    qInfo() << "Processed" << f1_frame_to_data24.get_valid_f1_frames_count() << "Valid F1 Frames and" << f1_frame_to_data24.get_invalid_f1_frames_count() << "Invalid F1 Frames";

    // Get the statistics for the C1 and C2 decoders
    int32_t valid_c1s, fixed_c1s, error_c1s;
    f2_frame_to_f1.get_c1_circ_stats(valid_c1s, fixed_c1s, error_c1s);
    qInfo() << "C1 Decoder: Valid:" << valid_c1s << "- Fixed:" << fixed_c1s << "- Error:" << error_c1s
        << "- Total:" << valid_c1s + fixed_c1s + error_c1s << "- Total errors:" << fixed_c1s + error_c1s;

    int32_t valid_c2s, fixed_c2s, error_c2s;
    f2_frame_to_f1.get_c2_circ_stats(valid_c2s, fixed_c2s, error_c2s);
    qInfo() << "C2 Decoder: Valid:" << valid_c2s << "- Fixed:" << fixed_c2s << "- Error:" << error_c2s
        << "- Total:" << valid_c2s + fixed_c2s + error_c2s << "- Total errors:" << fixed_c2s + error_c2s;
    
    qInfo() << "Processed" << f1_frame_count << "F1 Frames," << f2_frame_count << "F2 Frames," << f3_frame_count << "F3 Frames";

    // Should we add a wav header to the output data?
    if (is_output_data_wav) {
        qDebug() << "EfmProcessor::process(): Adding WAV header to output data";
        // WAV file header
        struct WAVHeader {
            char riff[4] = {'R', 'I', 'F', 'F'};
            uint32_t chunkSize;
            char wave[4] = {'W', 'A', 'V', 'E'};
            char fmt[4] = {'f', 'm', 't', ' '};
            uint32_t subchunk1Size = 16; // PCM
            uint16_t audioFormat = 1; // PCM
            uint16_t numChannels = 2; // Stereo
            uint32_t sampleRate = 44100; // 44.1kHz
            uint32_t byteRate;
            uint16_t blockAlign;
            uint16_t bitsPerSample = 16; // 16 bits
            char data[4] = {'d', 'a', 't', 'a'};
            uint32_t subchunk2Size;
        };

        WAVHeader header;
        header.chunkSize = 36 + output_file.size();
        header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
        header.blockAlign = header.numChannels * header.bitsPerSample / 8;
        header.subchunk2Size = output_file.size();

        // Move to the beginning of the file to write the header
        output_file.seek(0);
        output_file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
    }

    // Close the input and output files
    input_file.close();
    output_file.close();

    qInfo() << "Encoding complete";
    return true;
}

void EfmProcessor::set_show_data(bool _showOutput, bool _showF1, bool _showF2, bool _showF3) {
    showOutput = _showOutput;
    showF1 = _showF1;
    showF2 = _showF2;
    showF3 = _showF3;
}

// Set the output data type (true for WAV, false for raw)
void EfmProcessor::set_output_type(bool _wavOutput) {
    is_output_data_wav = _wavOutput;
}   