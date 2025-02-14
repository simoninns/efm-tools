/************************************************************************

    main.cpp

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
    QCoreApplication::setApplicationName("efm-decoder");
    QCoreApplication::setApplicationVersion(QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "efm-decoder - EFM data decoder\n"
        "\n"
        "(c)2025 Simon Inns\n"
        "GPLv3 Open-Source - github: https://github.com/simoninns/efm-tools");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add the standard debug options --debug and --quiet
    addStandardDebugOptions(parser);

    // Option to specify output data file type
    QCommandLineOption outputTypeOption("wav-output", QCoreApplication::translate("main", "Add wav header to output data"));
    parser.addOption(outputTypeOption);

    // Group of options for showing frame data
    QList<QCommandLineOption> displayFrameDataOptions = {
        QCommandLineOption("show-f1", QCoreApplication::translate("main", "Show F1 frame data")),
        QCommandLineOption("show-f2", QCoreApplication::translate("main", "Show F2 frame data")),
        QCommandLineOption("show-f3", QCoreApplication::translate("main", "Show F3 frame data")),
        QCommandLineOption("show-output", QCoreApplication::translate("main", "Show output data")),
    };
    parser.addOptions(displayFrameDataOptions);

    // Group of options for advanced debugging
    QList<QCommandLineOption> advancedDebugOptions = {
        QCommandLineOption("show-tvalues-debug", QCoreApplication::translate("main", "Show T-values to channel decoding debug")),
        QCommandLineOption("show-channel-debug", QCoreApplication::translate("main", "Show channel to F3 decoding debug")),
        QCommandLineOption("show-f3-debug", QCoreApplication::translate("main", "Show F3 to F2 section decoding debug")),
        QCommandLineOption("show-f2-correct-debug", QCoreApplication::translate("main", "Show F2 section correction debug")),
        QCommandLineOption("show-f2-debug", QCoreApplication::translate("main", "Show F2 to F1 decoding debug")),
        QCommandLineOption("show-f1-debug", QCoreApplication::translate("main", "Show F1 to Data24 decoding debug")),
        QCommandLineOption("show-all-debug", QCoreApplication::translate("main", "Show all decoding debug")),
    };
    parser.addOptions(advancedDebugOptions);

    // -- Positional arguments --
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input EFM file"));
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output data file"));

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Check for frame data options
    bool showF1 = parser.isSet("show-f1");
    bool showF2 = parser.isSet("show-f2");
    bool showF3 = parser.isSet("show-f3");
    bool showOutput = parser.isSet("show-output");

    // Check for advanced debug options
    bool showTValuesDebug = parser.isSet("show-tvalues-debug");
    bool showChannelDebug = parser.isSet("show-channel-debug");
    bool showF3Debug = parser.isSet("show-f3-debug");
    bool showF2CorrectDebug = parser.isSet("show-f2-correct-debug");
    bool showF2Debug = parser.isSet("show-f2-debug");
    bool showF1Debug = parser.isSet("show-f1-debug");
    bool showAllDebug = parser.isSet("show-all-debug");

    if (showAllDebug) {
        showTValuesDebug = true;
        showChannelDebug = true;
        showF3Debug = true;
        showF2CorrectDebug = true;
        showF2Debug = true;
        showF1Debug = true;
    }

    // Get the filename arguments from the parser
    QString input_filename;
    QString output_filename;
    QStringList positional_arguments = parser.positionalArguments();
    
    if (positional_arguments.count() != 2) {
        qWarning() << "You must specify the input EFM filename and the output data filename";
        return 1;
    }
    input_filename = positional_arguments.at(0);
    output_filename = positional_arguments.at(1);

    // Check for output data type options
    bool wav_output = parser.isSet(outputTypeOption);

    // Perform the processing
    qInfo() << "Beginning EFM decoding of" << input_filename;
    EfmProcessor efm_processor;

    efm_processor.set_show_data(showOutput, showF1, showF2, showF3);
    efm_processor.set_output_type(wav_output);
    efm_processor.set_debug(showTValuesDebug, showChannelDebug, showF3Debug, showF2CorrectDebug, showF2Debug, showF1Debug);

    if (!efm_processor.process(input_filename, output_filename)) {
        return 1;
    }

    // Quit with success
    return 0;
}