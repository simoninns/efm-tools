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

#include "efm_processor.h"

EfmProcessor::EfmProcessor() {
    data24_frame_count = 0;
    f1_section_count = 0;
    f3_frame_count = 0;
    f2_section_count = 0;
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

    // Set the debug flags
    t_values_to_channel.set_show_debug(false);
    channel_to_f3.set_show_debug(false);
    f3_frame_to_f2_section.set_show_debug(false);
    f2_section_correction.set_show_debug(true);
    f2_section_to_f1_section.set_show_debug(true);
    f1_section_to_data24_section.set_show_debug(false);

    // Get the total size of the input file for progress reporting
    qint64 total_size = input_file.size();
    qint64 processed_size = 0;
    int last_progress = 0;

    // Process the EFM data in chunks of 1024 T-values
    bool end_of_data = false;
    while (!end_of_data) {
        // Read 1024 T-values from the input file
        QByteArray t_values = input_file.read(1024);
        processed_size += t_values.size();

        int progress = static_cast<int>((processed_size * 100) / total_size);
        if (progress >= last_progress + 5) { // Show progress every 5%
            qInfo() << "Progress:" << progress << "%";
            last_progress = progress;
        }

        if (t_values.isEmpty()) {
            end_of_data = true;
        } else {
            t_values_to_channel.push_frame(t_values);
        }

        process_pipeline(output_file);
    }

    // We are out of data flush the pipeline
    qInfo() << "Flushing decoding pipelines";
    f2_section_correction.flush();
    process_pipeline(output_file);

    // Show summary
    qInfo() << "Decoding complete";

    t_values_to_channel.show_statistics(); qInfo() << "";
    channel_to_f3.show_statistics(); qInfo() << "";
    f3_frame_to_f2_section.show_statistics(); qInfo() << "";
    f2_section_correction.show_statistics(); qInfo() << "";
    f2_section_to_f1_section.show_statistics(); qInfo() << "";
    f1_section_to_data24_section.show_statistics(); qInfo() << "";
    
    qInfo() << "Processed" << data24_frame_count << "Data24 Frames," << f1_section_count << "F1 Sections," << f2_section_count << "F2 Sections," << f3_frame_count << "F3 Frames";

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

void EfmProcessor::process_pipeline(QFile& output_file) {
    // Are there any T-values ready?
    while(t_values_to_channel.is_ready()) {
        QByteArray channel_data = t_values_to_channel.pop_frame();
        channel_to_f3.push_frame(channel_data);
    }

    // Are there any F3 frames ready?
    while(channel_to_f3.is_ready()) {
        F3Frame f3_frame = channel_to_f3.pop_frame();
        if (showF3) f3_frame.show_data();
        f3_frame_to_f2_section.push_frame(f3_frame);
        f3_frame_count++;
    }

    // Note: Once we have F2 frames, we have to group them into 98 frame
    // sections due to the Q-channel metadata which is spread over 98 frames
    // otherwise we can't keep track of the metadata as we decode the data...

    // Are there any F2 sections ready?
    while(f3_frame_to_f2_section.is_ready()) {
        F2Section section = f3_frame_to_f2_section.pop_section();
        f2_section_correction.push_section(section);
        f2_section_count++;
    }

    // Are there any corrected F2 sections ready?
    while(f2_section_correction.is_ready()) {
        F2Section f2_section = f2_section_correction.pop_section();

        if (showF2) f2_section.show_data();
        f2_section_to_f1_section.push_section(f2_section);
    }

    // Are there any F1 sections ready?
    while(f2_section_to_f1_section.is_ready()) {
        F1Section f1_section = f2_section_to_f1_section.pop_section();
        if (showF1) f1_section.show_data();
        f1_section_to_data24_section.push_section(f1_section);
        f1_section_count++;
    }

    // Are there any data24 frames ready?
    while(f1_section_to_data24_section.is_ready()) {
        Data24Section data24section = f1_section_to_data24_section.pop_section();

        // Each Data24 section contains 98 frames that we need to write to the output file
        for (int index = 0; index < 98; index++) {
            Data24 data24 = data24section.get_frame(index);
            output_file.write(reinterpret_cast<const char*>(data24.get_data().data()), data24.get_frame_size());
            data24_frame_count += 1;
        }

        if (showOutput) {
            data24section.show_data();
        }
    }
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