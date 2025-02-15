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
}

bool EfmProcessor::process(QString input_filename, QString output_filename) {
    qDebug() << "EfmProcessor::process(): Decoding EFM from file: " << input_filename << " to file: " << output_filename;

    // Prepare the input file
    QFile input_file(input_filename);

    if (!input_file.open(QIODevice::ReadOnly)) {
        qDebug() << "EfmProcessor::process(): Failed to open input file: " << input_filename;
        return false;
    }

    // Prepare the output files...
    if (!is_output_data_wav) writer_data.open(output_filename);
    else writer_wav.open(output_filename);

    if (output_wav_metadata && is_output_data_wav) {
        // Prepare the metadata output file
        // If the output filename ends in .wav, replace it with .metadata
        // otherwise append .metadata to the output filename
        QString metadata_filename = output_filename;
        if (metadata_filename.endsWith(".wav")) {
            metadata_filename.replace(".wav", ".metadata");
        } else {
            metadata_filename.append(".metadata");
        }

        writer_wav_metadata.open(metadata_filename);
    }

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

        process_pipeline();
    }

    // We are out of data flush the pipeline
    qInfo() << "Flushing decoding pipelines";
    f2_section_correction.flush();
    process_pipeline();

    // Show summary
    qInfo() << "Decoding complete";

    t_values_to_channel.show_statistics(); qInfo() << "";
    channel_to_f3.show_statistics(); qInfo() << "";
    f3_frame_to_f2_section.show_statistics(); qInfo() << "";
    f2_section_correction.show_statistics(); qInfo() << "";
    f2_section_to_f1_section.show_statistics(); qInfo() << "";
    f1_section_to_data24_section.show_statistics(); qInfo() << "";
    if (is_output_data_wav) {
        data24_to_audio.show_statistics(); qInfo() << "";
    }
    if (is_output_data_wav && !no_wav_correction) {
        audio_correction.show_statistics(); qInfo() << "";
    }
    
    // Close the input file
    input_file.close();

    // Close the output files
    if (!is_output_data_wav) writer_data.close();
    else writer_wav.close();
    if (output_wav_metadata && is_output_data_wav) writer_wav_metadata.close();

    qInfo() << "Encoding complete";
    return true;
}

void EfmProcessor::process_pipeline() {
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
    }

    // Note: Once we have F2 frames, we have to group them into 98 frame
    // sections due to the Q-channel metadata which is spread over 98 frames
    // otherwise we can't keep track of the metadata as we decode the data...

    // Are there any F2 sections ready?
    while(f3_frame_to_f2_section.is_ready()) {
        F2Section section = f3_frame_to_f2_section.pop_section();
        f2_section_correction.push_section(section);
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
    }

    if (!is_output_data_wav) {
        // Output is data, so we write the Data24 sections directly to the output file

        // Are there any data24 frames ready?
        while(f1_section_to_data24_section.is_ready()) {
            Data24Section data24section = f1_section_to_data24_section.pop_section();

            // Write the Data24 frames to the output file as raw data
            writer_data.write(data24section);

            if (showData24) {
                data24section.show_data();
            }
        }
    } else {
        // Output is audio, so we convert the Data24 sections to audio sections
        
        // Are there any data24 frames ready?
        while(f1_section_to_data24_section.is_ready()) {
            Data24Section data24section = f1_section_to_data24_section.pop_section();
            data24_to_audio.push_section(data24section);

            if (showData24) {
                data24section.show_data();
            }
        }

        if (no_wav_correction) {
            // Are there any audio frames ready?
            // If so we write them to the output file
            while(data24_to_audio.is_ready()) {
                AudioSection audio_section = data24_to_audio.pop_section();

                // Write the Audio frames to the output file as wav data
                writer_wav.write(audio_section);
                if (output_wav_metadata) writer_wav_metadata.write(audio_section);
            }
        } else {
            // Are there any audio frames ready?
            // If so, attempt to correct them
            while(data24_to_audio.is_ready()) {
                AudioSection audio_section = data24_to_audio.pop_section();
                audio_correction.push_section(audio_section);
            }

            // Are there any corrected audio frames ready?
            // If so we write them to the output file
            while(audio_correction.is_ready()) {
                AudioSection audio_section = audio_correction.pop_section();

                // Write the Audio frames to the output file as wav data
                writer_wav.write(audio_section);
                if (output_wav_metadata) writer_wav_metadata.write(audio_section);
            }
        }
    }
}

void EfmProcessor::set_show_data(bool _showAudio, bool _showData24, bool _showF1, bool _showF2, bool _showF3) {
    showAudio = _showAudio;
    showData24 = _showData24;
    showF1 = _showF1;
    showF2 = _showF2;
    showF3 = _showF3;
}

// Set the output data type (true for WAV, false for raw)
void EfmProcessor::set_output_type(bool _wavOutput, bool _outputWavMetadata, bool _noWavCorrection) {
    is_output_data_wav = _wavOutput;
    no_wav_correction = _noWavCorrection;
    output_wav_metadata = _outputWavMetadata;
}

void EfmProcessor::set_debug(bool tvalue, bool channel, bool f3, bool f2, bool f1, bool data24, bool audio, bool audioCorrection) {
    // Set the debug flags
    t_values_to_channel.set_show_debug(tvalue);
    channel_to_f3.set_show_debug(channel);
    f3_frame_to_f2_section.set_show_debug(f3);
    f2_section_correction.set_show_debug(f2);
    f2_section_to_f1_section.set_show_debug(f1);
    f1_section_to_data24_section.set_show_debug(data24);
    data24_to_audio.set_show_debug(audio);
    audio_correction.set_show_debug(audioCorrection);
}