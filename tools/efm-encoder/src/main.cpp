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
    QCoreApplication::setApplicationVersion(QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
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
    QCommandLineOption inputTypeOption("wav-input", QCoreApplication::translate("main", "Treat input data as WAV file"));
    parser.addOption(inputTypeOption);

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
        QCommandLineOption("corrupt-tvalues", QCoreApplication::translate("main", "Corrupt t-values with specified symbol frequency"), "symbol-frequency"),
        QCommandLineOption("pad-start", QCoreApplication::translate("main", "Add the specified number of random t-value symbols before actual data"), "symbols"),
    };
    parser.addOptions(corruptionOptions);

    // Positional argument to specify input data file
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input data file"));

    // Positional argument to specify output audio file
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output EFM file"));

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

    // Check for frame data options
    bool showF1 = parser.isSet("show-f1");
    bool showF2 = parser.isSet("show-f2");
    bool showF3 = parser.isSet("show-f3");
    bool showInput = parser.isSet("show-input");

    // Check for data corruption options
    bool corrupt_tvalues = parser.isSet("corrupt-tvalues");
    bool pad_start = parser.isSet("pad-start");

    // Get the corruption parameters
    int corrupt_tvalues_frequency = parser.value("corrupt-tvalues").toInt();
    int pad_start_symbols = parser.value("pad-start").toInt();

    // Perform the processing
    EfmProcessor efm_processor;

    efm_processor.set_show_data(showInput, showF1, showF2, showF3);
    efm_processor.set_corruption(corrupt_tvalues, corrupt_tvalues_frequency, pad_start, pad_start_symbols);
    efm_processor.set_input_type(wav_input);

    if (!efm_processor.process(input_filename, output_filename)) {
        return 1;
    }

    // Quit with success
    return 0;
}