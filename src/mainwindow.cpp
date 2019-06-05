#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QValidator>
#include <QInputDialog>

#include <cmath>
#include <chrono>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Unit tests
    runTests();

    if (!generateGrid(4)) {
        qCritical() << "Invalid grid size! Only NxN grids supported.";
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

bool MainWindow::generateGrid(int size) {
    if (size < 4) {
        return false;
    }

    double sizeSqrt = sqrt(size);

    double intpart;
    if (modf(sizeSqrt, &intpart) != 0.0) {
        return false;
    }

    if (!sudokuGridWidgets.isEmpty()) {
        for (auto &widget : sudokuGridWidgets) {
            ui->gridLayoutSudoku->removeWidget(widget);
            delete widget;
        }
        sudokuGridWidgets.clear();

        grid.clear();
    }

    int regionsPerRow = static_cast<int>(intpart);
    int columnsInRegion = size / regionsPerRow;

    // Base size based on 9x9 grid
    int cellSize = 9 * 3 * 2;
    // Scale for other sizes
    if (size < 9) {
        cellSize *= regionsPerRow;
    } else if (size > 9) {
        cellSize /= static_cast<int>(regionsPerRow / 2.0);
    }

    const QIntValidator *validator = new QIntValidator(1, size, this);

    // Initialize all grid rows
    // For later population (different order, as we add region by region)
    grid.reserve(size * size);
    for (int i = 0; i < size; ++i) {
        grid.append(UIGridRow());
    }

    // Add region by region and fill with cells
    // to achieve wanted styling
    for (int si = 0; si < regionsPerRow; ++si) {
        for (int sj = 0; sj < regionsPerRow; ++sj) {
            QFrame *widget = new QFrame(this);
            widget->setFrameShape(QFrame::Box);

            int widgetSize = cellSize * regionsPerRow + 2; // 2 for border spaces
            widget->setMinimumSize(widgetSize, widgetSize);
            widget->setMaximumSize(widgetSize, widgetSize);

            QGridLayout *gridLayout = new QGridLayout(widget);
            gridLayout->setSpacing(0);
            gridLayout->setMargin(0);

            for (int i = 0; i < columnsInRegion; ++i) {
                for (int j = 0; j < columnsInRegion; ++j) {
                    QLineEdit *cell = new QLineEdit(widget);
                    cell->setAlignment(Qt::AlignCenter);
                    cell->setFont(QFont(cell->font().family(), cellSize / 2));
                    cell->setStyleSheet("QLineEdit { border: 1px solid grey; }");

                    cell->setValidator(validator);
                    cell->setMinimumSize(cellSize, cellSize);
                    cell->setMaximumSize(cellSize, cellSize);

                    gridLayout->addWidget(cell, i, j);

                    // Calculate row index from region and cell positions
                    // We add region by region and fill region as it's created
                    // but we have to add to the row in the entire grid
                    int rowIndex = i + si * columnsInRegion;
                    grid[rowIndex].append(cell);

                    connect(cell, &QLineEdit::textEdited, this, &MainWindow::onCellTextEdited);

                    sudokuGridWidgets.append(cell);
                }
            }

            ui->gridLayoutSudoku->addWidget(widget, si, sj);
            sudokuGridWidgets.append(widget);
        }
    }

    return true;
}

void MainWindow::resetGrid() {
    for (auto &row : grid) {
        for (auto &cell : row) {
            cell->setText("");
        }
    }
}

bool MainWindow::solveGrid(double &bench) {
    // Convert input data to primitive data
    // Instantiate DLX solver
    DLX dlx(UIGridToGrid());

    // Solve (convert problem to exact cover problem and solve with DLX)
    auto benchStart = std::chrono::high_resolution_clock::now();
    bool solved = dlx.solve();
    auto benchEnd = std::chrono::high_resolution_clock::now();

    if (solved) {
        // Apply to UI
        gridToUIGrid(dlx.solution());

        bench = std::chrono::duration<double, std::milli>(benchEnd - benchStart).count();
    }

    return solved;
}

void MainWindow::runTests() {
    // [title, input, expected result]
    QList<std::tuple<QString, QString, QString>> tests9x9;
    QList<std::tuple<QString, QString, QString>> tests16x16;

    // Test cases from: http://sudopedia.enjoysudoku.com/Valid_Test_Cases.html
    tests9x9.append({
        "Completed Puzzle",
        "974236158638591742125487936316754289742918563589362417867125394253649871491873625",
        "974236158638591742125487936316754289742918563589362417867125394253649871491873625"
    });
    tests9x9.append({
        "Last Empty Square",
        "2564891733746159829817234565932748617128.6549468591327635147298127958634849362715",
        "256489173374615982981723456593274861712836549468591327635147298127958634849362715"
    });
    tests9x9.append({
        "Naked Singles",
        "3.542.81.4879.15.6.29.5637485.793.416132.8957.74.6528.2413.9.655.867.192.965124.8",
        "365427819487931526129856374852793641613248957974165283241389765538674192796512438"
    });
    tests9x9.append({
        "Hidden Singles",
        "..2.3...8.....8....31.2.....6..5.27..1.....5.2.4.6..31....8.6.5.......13..531.4..",
        "672435198549178362831629547368951274917243856254867931193784625486592713725316489"
    });

    // Test case from https://en.wikipedia.org/wiki/Sudoku_solving_algorithms
    tests9x9.append({
        "Hard to Brute-Force",
        "..............3.85..1.2.......5.7.....4...1...9.......5......73..2.1........4...9",
        "987654321246173985351928746128537694634892157795461832519286473472319568863745219"
    });

    // Test cases from http://magictour.free.fr/topn234
    tests9x9.append({
        "Hard 1",
        "7.8...3.....6.1...5.........4.....263...8.......1...9..9.2....4....7.5...........",
        "768942315934651278512738649147593826329486157856127493693215784481379562275864931"
    });
    tests9x9.append({
        "Hard 2",
        "7.8...3.....6.1...5.........4.....263...8.......1...9..9.2....4....7.5...........",
        "768942315934651278512738649147593826329486157856127493693215784481379562275864931"
    });
    tests9x9.append({
        "Hard 3",
        "7.8...3.....2.1...5.........4.....263...8.......1...9..9.6....4....7.5...........",
        "728946315934251678516738249147593826369482157852167493293615784481379562675824931"
    });
    tests9x9.append({
        "Hard 4",
        "3.7.4...........918........4.....7.....16.......25..........38..9....5...2.6.....",
        "317849265245736891869512473456398712732164958981257634174925386693481527528673149"
    });
    tests9x9.append({
        "Hard 5",
        "5..7..6....38...........2..62.4............917............35.8.4.....1......9....",
        "582743619963821547174956238621479853348562791795318426217635984439287165856194372"
    });
    tests9x9.append({
        "Empty",
        ".................................................................................",
        "any" // Multiple solutions
    });
    tests9x9.append({
        "Single Given",
        "........................................1........................................",
        "any" // 500+ solutions
    });
    tests9x9.append({
        "Insufficient Givens",
        "...........5....9...4....1.2....3.5....7.....438...2......9.....1.4...6..........",
        "any" // 500+ solutions
    });
    tests9x9.append({
        "Duplicate Given - Region",
        "..9.7...5..21..9..1...28....7...5..1..851.....5....3.......3..68........21.....87",
        "none" // No solution
    });
    tests9x9.append({
        "Duplicate Given - Column",
        "6.159.....9..1............4.7.314..6.24.....5..3....1...6.....3...9.2.4......16..",
        "none" // No solution
    });
    tests9x9.append({
        "Duplicate Given - Row",
        ".4.1..35.............2.5......4.89..26.....12.5.3....7..4...16.6....7....1..8..2.",
        "none" // No solution
    });
    tests9x9.append({
        "Unsolvable Square",
        "..9.287..8.6..4..5..3.....46.........2.71345.........23.....5..9..4..8.7..125.3..",
        "none" // No solution
    });
    tests9x9.append({
        "Unsolvable Region",
        ".9.3....1....8..46......8..4.5.6..3...32756...6..1.9.4..1......58..2....2....7.6.",
        "none" // No solution
    });
    tests9x9.append({
        "Unsolvable Column",
        "....41....6.....2...2......32.6.........5..417.......2......23..48......5.1..2...",
        "none" // No solution
    });
    tests9x9.append({
        "Unsolvable Row",
        "9..1....4.14.3.8....3....9....7.8..18....3..........3..21....7...9.4.5..5...16..3",
        "none" // No solution
    });
    tests9x9.append({
        "Not Unique — 2 Solutions",
        ".39...12....9.7...8..4.1..6.42...79...........91...54.5..1.9..3...8.5....14...87.",
        "439658127156927384827431956342516798785294631691783542578149263263875419914362875" // 1st solution (found first by DLX)
        //"439658127156927384827431956642513798785294631391786542578149263263875419914362875" // Second solution

    });
    tests9x9.append({
        "Not Unique — 3 Solutions",
        "..3.....6...98..2.9426..7..45...6............1.9.5.47.....25.4.6...785...........",
        "783542196516987324942631758457296813238714965169853472891325647624178539375469281" // 1st solution (found first by DLX)
        //"783542916516987324942631758457216839238794165169853472891325647624178593375469281" // 2nd solution
        //"783542916516987324942631758457216893238794165169853472891325647624178539375469281" // 3rd solution
    });
    tests9x9.append({
        "Not Unique — 4 Solutions",
        "....9....6..4.7..8.4.812.3.7.......5..4...9..5..371..4.5..6..4.2.17.85.9.........",
        //"178693452623457198945812736716984325384526917592371684857169243231748569469235871" // 1st solution
        //"178693452623457198945812736716984325384526971592371684857169243231748569469235817" // 2nd solution
        "178693452623457198945812736762984315314526987589371624857169243231748569496235871" // 3rd solution (found first by DLX)
        //"178693452623457198945812736786924315314586927592371684857169243231748569469235871" // 4th solution
    });
    tests9x9.append({
        "Not Unique — 10 Solutions",
        "59.....486.8...3.7...2.1.......4.....753.698.....9.......8.3...2.6...7.934.....65",
        "592637148618459327437281596923748651175326984864195273759863412286514739341972865" // 1st solution (found first by DLX)
        //"592637148618459327437281596963748251175326984824195673759863412286514739341972865" // 2nd solution
        //"592637148618459327734281596129748653475326981863195274957863412286514739341972865" // 3rd solution
        //"592637148618459327734281596129748653475326981863195472957863214286514739341972865" // 4th solution
        //"592637148618459327734281596169748253475326981823195674957863412286514739341972865" // 5th solution
        //"592637148618459327734281596829145673175326984463798251957863412286514739341972865" // 6th solution
        //"592637148618459327734281596829145673475326981163798254957863412286514739341972865" // 7th solution
        //"592637148618459327734281596829145673475326981163798452957863214286514739341972865" // 8th solution
        //"592637148618459327734281596869145273175326984423798651957863412286514739341972865" // 9th solution
        //"592637148618459327734281596869145273475326981123798654957863412286514739341972865" // 10th solution
    });
    tests9x9.append({
        "Not Unique — 125 Solutions",
        "...3165..8..5..1...1.89724.9.1.85.2....9.1....4.263..1.5.....1.1..4.9..2..61.8...",
        //"592637148618459327437281596923748651175326984864195273759863412286514739341972865" // 1st solution
        //"274316589893524167615897243931785426562941378748263951359672814187459632426138795" // 2nd solution
        //"274316589893524167615897243931785426762941358548263791359672814187459632426138975" // 3rd solution
        //"274316589893524167615897243931785426762941358548263971359672814187459632426138795" // 4th solution
        //"274316589893524167615897243931785426762941835548263791459672318187439652326158974" // 5th solution
        //"274316589893524167615897243931785426762941835548263971459672318187439652326158794" // 6th solution
        "294316578867524139513897246931785624682941753745263981459632817178459362326178495" // Nth solution (found first by DLX)
        //(additional solutions omitted for brevity)                                          // 7th-120th solution
        //"724316598869524173315897246931785624682941357547263981458632719173459862296178435" // 121th solution
        //"724316598869524173315897246931785624682941735547263981453672819178459362296138457" // 122th solution
        //"724316598869524173315897246931785624682941735547263981458672319173459862296138457" // 123th solution
        //"724316598893524176615897243961785324382941765547263981459632817138479652276158439" // 124th solution
        //"724316598893524176615897243961785324382941765547263981459632817178459632236178459" // 125th solution
    });

    tests9x9.append({
        "Golden Nugget [Extremely Hard]",
        ".......39....1...5..3..58....8..9..6.7..2....1..4.......9..8.5..2....6..4..7.....",
        "751864239892317465643295871238179546974526318165483927319648752527931684486752193"
    });

    // Test cases from http://magictour.free.fr/top44
    tests16x16.append({
        "Hard 1",
        ".63B.EC..A..8....847..A6..B....9.....81.D.G...7E.......7..98...CF.D.....AC..2.......D.....E1..5.CE......6...GF.31A.9...B8G7.4..D2.E...45....69.F.7......E..A...5..94..6......D.....63..F79.5...A....E6.D.1...2.8...3G.FA56.......D.C...9...B1.6..2..B.5C9.....34",
        "10631911121314721585416138475101461121116153299125215811634613101471114161511432710598126113346512711101513149281618152136934121116114751071416121281561054111393111109135161483724121562313147164511811069121515712161019821413634115519411136216153127108141110863141512794516113249141516673131121152108161313241011568791514121251181415139421031166762710812519161514131134"
    });
    tests16x16.append({
        "Hard 2",
        ".B.293.F..C.......7.B..5......C..9..C...247.F...EF..6....9B.3D..F...58G...........B3......2F1.7.....E...1.8..C.D...1...3.D...G..4.6...2.3..9A.8.12..G.86.F......A7....C...419.G......E..5....7437..........B.3.C.8...DF......E96.E.6...9......D8..G..7..C..4...A",
        "61529310811121314415167144781513165101362911123916151112141247861013510111213627416951538141121011758116415931362148164310691412521311171596214131115711081651234513151241237614118161094561171621231415910181312394148613711101251516137141231051581641926111615810191311526121474371494165111368121531210118101614132151312574962316121549141110716135815121358761093164111412"
    });
    tests16x16.append({
        "Hard 3",
        "4...C7B...86....8.7G.A..E3..4D..6....9....1..A3.9......D...4..5FG..A.8F..B4.3....D3...AG..F..17...6..E....2.....7....B....5.E4...1E2A..4D.....8.C.5......A.76..D........329.F....8....65.....B.3.......E91.......B..G.8...A...C.5...F.....3C79E...GD.21.....5368",
        "41011512732139861141516812711513516231114469106132144910815511611123791615361114171012481352159161328116127413510142538161441210111591317614610135715141623128119711141213910685131642153113214101541161612978510155169123841476111213146127816131132951510411189471265115131021614316386541279110214151311127193158135146111021645210151161614413387911213144111021916127155368"
    });
    tests16x16.append({
        "Hard 4",
        ".G4..........B8...E.2.8.1..7..5......B.F26..9..3B..DA....F.9..E.2....A1.....786G561.C..4...32......C......9F.5A.....G6.......9..F...51G..2......D8C....9..E..67..E....F.AD4.C..B...7...D8.......C..E69.B....51...98...E...3....D..G.735...A4F.....35...A...2..4.",
        "10114612793131451516281912133216815141076115141614715105412681191213382511113146312169104157231610915113451214786115619168104117133214121541511814212761691035113713121436115151284916103169125178102116151314415821141039161314121675145613151216273418101191410713116148915512321612101546921614117135138698214131151531614710121311416735101286411159211735814151291012131646"
    });
    tests16x16.append({
        "Hard 5",
        ".EB....A..F18..5..A97........3C..5G...43..B....EC.1....F.A38....26.....1...4.F..5....2.....9...G.3.8.G.6C..F......E...D...5.B.A.E...B8...7.C.4...D.....4....35..B.5F.6.......C.749..A7.5.D.2.GF.8.7...E.6.G..A...G2B5.......4.9.....6....81.7..3.....C..2B.A..GD",
        "37101121261314941815165151316975810116212134141458219431016131511761212416151116147538910132269781451151116412133105111316321578101296114410312841696113147521115114154121013113256169786231138141557111610412978141011112491561335216161251396102418314111574911151673512141021381681573141311264952161011111214537816121510469139104126152161381117145313166510411923714151281"
    });

    double benchSum = 0.0;
    bool allPassed = true;

    // 9x9
    qInfo() << "Running 9x9 Tests:";
    generateGrid(9);
    for (auto &test : tests9x9) {
        runTest(test, benchSum, allPassed);

        resetGrid();
    }

    // 16x16
    qInfo() << "Running 16x16 Tests:";
    generateGrid(16);
    for (auto &test : tests16x16) {
        runTest(test, benchSum, allPassed);
        resetGrid();
    }

    if (allPassed) {
        qInfo() << "All tests PASSED!";
    } else {
        qInfo() << "Some tests FAILED or gave WRONG results!";
    }
    qInfo() << "Average time:" << benchSum / (tests9x9.size() + tests16x16.size()) << "milliseconds";
}

void MainWindow::runTest(std::tuple<QString, QString, QString> &test, double &benchSum, bool &allPassed) {
    stringGridToUIGrid(std::get<1>(test));

    double bench = 0.0;
    bool solved = solveGrid(bench);
    benchSum += bench;

    bool noSolution = std::get<2>(test) == "none";
    if ((solved && !noSolution) || (!solved && noSolution)) {
        QString result = UIGridToStringGrid();

        if (result == std::get<2>(test) || std::get<2>(test) == "any" || noSolution) {
            qInfo() << "- Passed:" << std::get<0>(test) << "(in" << bench << "milliseconds)";
        } else {
            qWarning() << "O Wrong:" << std::get<0>(test) << "(in" << bench << "milliseconds)";
            qInfo() << "  -> Correct:" << result;
            allPassed = false;
        }
    } else {
        qCritical() << "X Failed:" << std::get<0>(test) << "(in" << bench << "milliseconds)";
        allPassed = false;
    }
}

// Converters
Grid MainWindow::UIGridToGrid() const {
    Grid sudoku;
    sudoku.reserve(grid.size());

    for (auto &row : grid) {
        GridRow sudokuRow;
        for (auto &cell : row) {
            sudokuRow.append(cellValue(cell));
        }
        sudoku.append(sudokuRow);
    }

    return sudoku;
}

void MainWindow::gridToUIGrid(Grid sudoku) {
    resetGrid();

    for (int i = 0; i < sudoku.size(); ++i) {
        for (int j = 0; j < sudoku.at(i).size(); ++j) {
            setCellValue(grid.at(i).at(j), sudoku.at(i).at(j));
        }
    }
}

void MainWindow::stringGridToUIGrid(QString gridStr) {
    int i = 0;
    int j = 0;
    for (auto &valueStr : gridStr) {
        setCellValue(grid.at(i).at(j), valueStr.digitValue());

        if (++j >= grid.size()) {
            j = 0;
            ++i;
        }
    }
}

QString MainWindow::UIGridToStringGrid() {
    QString gridStr = "";
    for (auto &row : grid) {
        for (auto &cell : row) {
            int value = cellValue(cell);
            if (value < 1) {
                gridStr.append(".");
            } else {
                gridStr.append(QString::number(value));
            }
        }
    }

    return gridStr;
}

// UI input getters/setters
int MainWindow::cellValue(QLineEdit *cell) const {
    if (cell->text() == "") {
        return -1;
    }
    return cell->text().toInt();
}

void MainWindow::setCellValue(QLineEdit *cell, int value) {
    if (value < 1) {
        cell->setText("");
    } else {
        cell->setText(QString::number(value));
    }
}

// Slots
void MainWindow::onCellTextEdited(const QString &text) {
    // Manual low-bound validation (validator doesn't handle it)
    // Manual high-bound (below 9) validation (validator doesn't handle it)
    int input = text.toInt();
    if (input < 1 || input > grid.size()) {
        int value;
        for (auto &row : grid) {
            for (auto &cell : row) {
                value = cellValue(cell);
                if (value == 0) {
                    cell->setText("1");
                }
                if (value > grid.size()) {
                    cell->setText(QString::number(grid.size()));
                }
            }
        }
    }
}

void MainWindow::on_pushButtonImport_clicked() {
    bool ok;
    QString text = QInputDialog::getText(this, "Sudoku Import", "Input Sudoku problem in format: 53.2..4...", QLineEdit::Normal, nullptr, &ok);
    if (ok && !text.isEmpty()) {
        bool generated = true;
        if (text.size() != grid.size() * grid.size()) {
            double sizeSqrt = sqrt(text.size());
            double intpart;
            if (modf(sizeSqrt, &intpart) == 0.0) {
                generated = generateGrid(static_cast<int>(sqrt(text.size())));
            } else {
                generated = false;
            }
        }

        if (generated) {
            stringGridToUIGrid(text);
            ui->statusBar->showMessage("Imported!");
        } else {
            ui->statusBar->showMessage("Invalid grid size! Only NxN grids supported.");
        }
    } else {
        ui->statusBar->showMessage("Failed to import! Wrong data.");
    }
}

void MainWindow::on_pushButtonSolve_clicked() {
    double bench;
    bool solved = solveGrid(bench);

    if (solved) {
        ui->statusBar->showMessage("Solved in " + QString::number(bench) + " milliseconds!");
        qInfo() << "Solution:" << UIGridToStringGrid();
    } else {
        ui->statusBar->showMessage("No solution!");
    }
}

void MainWindow::on_pushButtonReset_clicked() {
    resetGrid();
}
