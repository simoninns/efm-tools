/************************************************************************

    main.cpp

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

#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>
#include <QThread>

#include "logging.h"
#include "efm_processor.h"

int main(int argc, char *argv[])
{
    // Set 'binary mode' for stdin and stdout on windows
    setBinaryMode();
    // Install the local debug message handler
    setDebug(true);
    qInstallMessageHandler(debugOutputHandler);

    QCoreApplication a(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("efm-encoder");
    QCoreApplication::setApplicationVersion(
            QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
            "efm-encoder - EFM data encoder\n"
            "\n"
            "(c)2025 Simon Inns\n"
            "GPLv3 Open-Source - github: https://github.com/simoninns/efm-tools");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add the standard debug options --debug and --quiet
    addStandardDebugOptions(parser);

    // Option to specify input data file type
    QCommandLineOption inputTypeOption(
            "wav-input", QCoreApplication::translate("main", "Treat input data as WAV file"));
    parser.addOption(inputTypeOption);

    // Group of options for Q-Channel data
    QList<QCommandLineOption> qChannelOptions = {
        QCommandLineOption("qmode-1",
                           QCoreApplication::translate("main", "Set Q-Channel mode 1 (default)")),
        QCommandLineOption("qmode-4", QCoreApplication::translate("main", "Set Q-Channel mode 4")),
        QCommandLineOption(
                "qmode-audio",
                QCoreApplication::translate("main", "Set Q-Channel control to audio (default)")),
        QCommandLineOption("qmode-data",
                           QCoreApplication::translate("main", "Set Q-Channel control to data")),
        QCommandLineOption("qmode-copy",
                           QCoreApplication::translate(
                                   "main", "Set Q-Channel control to copy permitted (default)")),
        QCommandLineOption(
                "qmode-nocopy",
                QCoreApplication::translate("main", "Set Q-Channel control to copy prohibited")),
        QCommandLineOption("qmode-nopreemp",
                           QCoreApplication::translate(
                                   "main", "Set Q-Channel audio pre-emphasis to off (default)")),
        QCommandLineOption(
                "qmode-preemp",
                QCoreApplication::translate("main", "Set Q-Channel audio pre-emphasis to on")),
        QCommandLineOption(
                "qmode-2ch",
                QCoreApplication::translate("main", "Set Q-Channel audio to stereo (default)")),
        QCommandLineOption("qmode-4ch",
                           QCoreApplication::translate("main", "Set Q-Channel audio to 4 channel")),
    };
    parser.addOptions(qChannelOptions);

    // Group of options for showing frame data
    QList<QCommandLineOption> displayFrameDataOptions = {
        QCommandLineOption("show-f1", QCoreApplication::translate("main", "Show F1 frame data")),
        QCommandLineOption("show-f2", QCoreApplication::translate("main", "Show F2 frame data")),
        QCommandLineOption("show-f3", QCoreApplication::translate("main", "Show F3 frame data")),
        QCommandLineOption("show-input", QCoreApplication::translate("main", "Show input data")),
    };
    parser.addOptions(displayFrameDataOptions);

    // Group of options for corrupting data
    QList<QCommandLineOption> corruptionOptions = {
        QCommandLineOption("corrupt-tvalues",
                           QCoreApplication::translate(
                                   "main", "Corrupt t-values with specified symbol frequency"),
                           "symbol-frequency"),
        QCommandLineOption(
                "corrupt-start",
                QCoreApplication::translate(
                        "main",
                        "Add the specified number of random t-value symbols before actual data"),
                "symbols"),
        QCommandLineOption(
                "corrupt-f3sync",
                QCoreApplication::translate("main", "Corrupt F3 Frame 24-bit sync patterns"),
                "frame-frequency"),
        QCommandLineOption(
                "corrupt-subcode-sync",
                QCoreApplication::translate("main", "Corrupt subcode sync0 and sync1 patterns"),
                "section-frequency"),
    };
    parser.addOptions(corruptionOptions);

    // Positional argument to specify input data file
    parser.addPositionalArgument("input",
                                 QCoreApplication::translate("main", "Specify input data file"));

    // Positional argument to specify output audio file
    parser.addPositionalArgument("output",
                                 QCoreApplication::translate("main", "Specify output EFM file"));

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Get the filename arguments from the parser
    QString input_filename;
    QString output_filename;

    QStringList positional_arguments = parser.positionalArguments();

    if (positional_arguments.count() != 2) {
        qWarning() << "You must specify the input data filename and the output EFM filename";
        return 1;
    }
    input_filename = positional_arguments.at(0);
    output_filename = positional_arguments.at(1);

    // Check for input data type options
    bool wav_input = parser.isSet(inputTypeOption);

    // Check for Q-Channel options
    bool qmode_1 = parser.isSet("qmode-1");
    bool qmode_4 = parser.isSet("qmode-4");
    bool qmode_audio = parser.isSet("qmode-audio");
    bool qmode_data = parser.isSet("qmode-data");
    bool qmode_copy = parser.isSet("qmode-copy");
    bool qmode_nocopy = parser.isSet("qmode-nocopy");
    bool qmode_no_preemp = parser.isSet("qmode-nopreemp");
    bool qmode_preemp = parser.isSet("qmode-preemp");
    bool qmode_2ch = parser.isSet("qmode-2ch");
    bool qmode_4ch = parser.isSet("qmode-4ch");

    // Apply default Q-Channel options
    if (!qmode_1 && !qmode_4)
        qmode_1 = true;
    if (!qmode_audio && !qmode_data)
        qmode_audio = true;
    if (!qmode_copy && !qmode_nocopy)
        qmode_copy = true;
    if (!qmode_2ch && !qmode_4ch)
        qmode_2ch = true;
    if (!qmode_no_preemp && !qmode_preemp)
        qmode_no_preemp = true;

    // Check for frame data options
    bool showF1 = parser.isSet("show-f1");
    bool showF2 = parser.isSet("show-f2");
    bool showF3 = parser.isSet("show-f3");
    bool showInput = parser.isSet("show-input");

    // Check for data corruption options
    bool corrupt_tvalues = parser.isSet("corrupt-tvalues");
    bool corrupt_start = parser.isSet("corrupt-start");
    bool corrupt_f3sync = parser.isSet("corrupt-f3sync");
    bool corrupt_subcode_sync = parser.isSet("corrupt-subcode-sync");

    // Get the corruption parameters
    int corrupt_tvalues_frequency = parser.value("corrupt-tvalues").toInt();
    int corrupt_start_symbols = parser.value("corrupt-start").toInt();
    int corrupt_f3sync_frequency = parser.value("corrupt-f3sync").toInt();
    int corrupt_subcode_sync_frequency = parser.value("corrupt-subcode-sync").toInt();

    // Perform the processing
    EfmProcessor efm_processor;

    bool qmode_options_ok = efm_processor.set_qmode_options(
            qmode_1, qmode_4, qmode_audio, qmode_data, qmode_copy, qmode_nocopy, qmode_no_preemp,
            qmode_preemp, qmode_2ch, qmode_4ch);
    efm_processor.set_show_data(showInput, showF1, showF2, showF3);
    bool corruption_options_ok = efm_processor.set_corruption(
            corrupt_tvalues, corrupt_tvalues_frequency, corrupt_start, corrupt_start_symbols,
            corrupt_f3sync, corrupt_f3sync_frequency, corrupt_subcode_sync,
            corrupt_subcode_sync_frequency);
    efm_processor.set_input_type(wav_input);

    if (!corruption_options_ok || !qmode_options_ok) {
        return 1;
    }

    if (!efm_processor.process(input_filename, output_filename)) {
        return 1;
    }

    // Quit with success
    return 0;
}