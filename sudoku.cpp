//
// sudoku.cpp
// Created by Kim Hammar & Mallu Goswami on 2017-03-28.
//

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/gist.hh>
#include <stdlib.h>

using namespace Gecode;

//
// Help functions for ID2204, Assignment 1, Task 5
//

/* Sudoku specifications
 *
 * Each specification gives the initial positions that are filled in,
 * with blank squares represented as zeroes.
 */

static int examples[][9][9] = {
        {
                {0, 0, 0, 2, 0, 5, 0, 0, 0},
                {0, 9, 0, 0, 0, 0, 7, 3, 0},
                {0, 0, 2, 0, 0, 9, 0, 6, 0},

                {2, 0, 0, 0, 0, 0, 4, 0, 9},
                {0, 0, 0, 0, 7, 0, 0, 0, 0},
                {6, 0, 9, 0, 0, 0, 0, 0, 1},

                {0, 8, 0, 4, 0, 0, 1, 0, 0},
                {0, 6, 3, 0, 0, 0, 0, 8, 0},
                {0, 0, 0, 6, 0, 8, 0, 0, 0}
        },
        {
                {3, 0, 0, 9, 0, 4, 0, 0, 1},
                {0, 0, 2, 0, 0, 0, 4, 0, 0},
                {0, 6, 1, 0, 0, 0, 7, 9, 0},

                {6, 0, 0, 2, 4, 7, 0, 0, 5},
                {0, 0, 0, 0, 0, 0, 0, 0, 0},
                {2, 0, 0, 8, 3, 6, 0, 0, 4},

                {0, 4, 6, 0, 0, 0, 2, 3, 0},
                {0, 0, 9, 0, 0, 0, 6, 0, 0},
                {5, 0, 0, 3, 0, 9, 0, 0, 8}
        },
        {
                {0, 0, 0, 0, 1, 0, 0, 0, 0},
                {3, 0, 1, 4, 0, 0, 8, 6, 0},
                {9, 0, 0, 5, 0, 0, 2, 0, 0},

                {7, 0, 0, 1, 6, 0, 0, 0, 0},
                {0, 2, 0, 8, 0, 5, 0, 1, 0},
                {0, 0, 0, 0, 9, 7, 0, 0, 4},

                {0, 0, 3, 0, 0, 4, 0, 0, 6},
                {0, 4, 8, 0, 0, 6, 9, 0, 7},
                {0, 0, 0, 0, 8, 0, 0, 0, 0}
        },
        {    // Fiendish puzzle April 21, 2005 Times London
                {0, 0, 4, 0, 0, 3, 0, 7, 0},
                {0, 8, 0, 0, 7, 0, 0, 0, 0},
                {0, 7, 0, 0, 0, 8, 2, 0, 5},

                {4, 0, 0, 0, 0, 0, 3, 1, 0},
                {9, 0, 0, 0, 0, 0, 0, 0, 8},
                {0, 1, 5, 0, 0, 0, 0, 0, 4},

                {1, 0, 6, 9, 0, 0, 0, 3, 0},
                {0, 0, 0, 0, 2, 0, 0, 6, 0},
                {0, 2, 0, 4, 0, 0, 5, 0, 0}
        },
        {    // This one requires search
                {0, 4, 3, 0, 8, 0, 2, 5, 0},
                {6, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 1, 0, 9, 4},

                {9, 0, 0, 0, 0, 4, 0, 7, 0},
                {0, 0, 0, 6, 0, 8, 0, 0, 0},
                {0, 1, 0, 2, 0, 0, 0, 0, 3},

                {8, 2, 0, 5, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0, 0, 0, 5},
                {0, 3, 4, 0, 9, 0, 7, 1, 0}
        },
        {    // Hard one from http://www.cs.mu.oz.au/671/proj3/node5.html
                {0, 0, 0, 0, 0, 3, 0, 6, 0},
                {0, 0, 0, 0, 0, 0, 0, 1, 0},
                {0, 9, 7, 5, 0, 0, 0, 8, 0},

                {0, 0, 0, 0, 9, 0, 2, 0, 0},
                {0, 0, 8, 0, 7, 0, 4, 0, 0},
                {0, 0, 3, 0, 6, 0, 0, 0, 0},

                {0, 1, 0, 0, 0, 2, 8, 9, 0},
                {0, 4, 0, 0, 0, 0, 0, 0, 0},
                {0, 5, 0, 1, 0, 0, 0, 0, 0}
        },
        { // Puzzle 1 from http://www.sudoku.org.uk/bifurcation.htm
                {1, 0, 0, 9, 0, 7, 0, 0, 3},
                {0, 8, 0, 0, 0, 0, 0, 7, 0},
                {0, 0, 9, 0, 0, 0, 6, 0, 0},
                {0, 0, 7, 2, 0, 9, 4, 0, 0},
                {4, 1, 0, 0, 0, 0, 0, 9, 5},
                {0, 0, 8, 5, 0, 4, 3, 0, 0},
                {0, 0, 3, 0, 0, 0, 7, 0, 0},
                {0, 5, 0, 0, 0, 0, 0, 4, 0},
                {2, 0, 0, 8, 0, 6, 0, 0, 9}
        },
        { // Puzzle 2 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 0, 3, 0, 2, 0, 0, 0},
                {0, 5, 0, 7, 9, 8, 0, 3, 0},
                {0, 0, 7, 0, 0, 0, 8, 0, 0},
                {0, 0, 8, 6, 0, 7, 3, 0, 0},
                {0, 7, 0, 0, 0, 0, 0, 6, 0},
                {0, 0, 3, 5, 0, 4, 1, 0, 0},
                {0, 0, 5, 0, 0, 0, 6, 0, 0},
                {0, 2, 0, 4, 1, 9, 0, 5, 0},
                {0, 0, 0, 8, 0, 6, 0, 0, 0}
        },
        { // Puzzle 3 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 0, 8, 0, 0, 0, 0, 6},
                {0, 0, 1, 6, 2, 0, 4, 3, 0},
                {4, 0, 0, 0, 7, 1, 0, 0, 2},
                {0, 0, 7, 2, 0, 0, 0, 8, 0},
                {0, 0, 0, 0, 1, 0, 0, 0, 0},
                {0, 1, 0, 0, 0, 6, 2, 0, 0},
                {1, 0, 0, 7, 3, 0, 0, 0, 4},
                {0, 2, 6, 0, 4, 8, 1, 0, 0},
                {3, 0, 0, 0, 0, 5, 0, 0, 0}
        },
        { // Puzzle 4 from http://www.sudoku.org.uk/bifurcation.htm
                {3, 0, 5, 0, 0, 4, 0, 7, 0},
                {0, 7, 0, 0, 0, 0, 0, 0, 1},
                {0, 4, 0, 9, 0, 0, 0, 3, 0},
                {4, 0, 0, 0, 5, 1, 0, 0, 6},
                {0, 9, 0, 0, 0, 0, 0, 4, 0},
                {2, 0, 0, 8, 4, 0, 0, 0, 7},
                {0, 2, 0, 0, 0, 7, 0, 6, 0},
                {8, 0, 0, 0, 0, 0, 0, 9, 0},
                {0, 6, 0, 4, 0, 0, 2, 0, 8}
        },
        { // Puzzle 5 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 0, 7, 0, 0, 3, 0, 0},
                {0, 6, 0, 0, 0, 0, 5, 7, 0},
                {0, 7, 3, 8, 0, 0, 4, 1, 0},
                {0, 0, 9, 2, 8, 0, 0, 0, 0},
                {5, 0, 0, 0, 0, 0, 0, 0, 9},
                {0, 0, 0, 0, 9, 3, 6, 0, 0},
                {0, 9, 8, 0, 0, 7, 1, 5, 0},
                {0, 5, 4, 0, 0, 0, 0, 6, 0},
                {0, 0, 1, 0, 0, 9, 0, 0, 0}
        },
        { // Puzzle 6 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 0, 6, 0, 0, 0, 0, 4},
                {0, 3, 0, 0, 9, 0, 0, 2, 0},
                {0, 6, 0, 8, 0, 0, 7, 0, 0},
                {0, 0, 5, 0, 6, 0, 0, 0, 1},
                {6, 7, 0, 3, 0, 1, 0, 5, 8},
                {9, 0, 0, 0, 5, 0, 4, 0, 0},
                {0, 0, 6, 0, 0, 3, 0, 9, 0},
                {0, 1, 0, 0, 8, 0, 0, 6, 0},
                {2, 0, 0, 0, 0, 6, 0, 0, 0}
        },
        { // Puzzle 7 from http://www.sudoku.org.uk/bifurcation.htm
                {8, 0, 0, 0, 0, 1, 0, 4, 0},
                {2, 0, 6, 0, 9, 0, 0, 1, 0},
                {0, 0, 9, 0, 0, 6, 0, 8, 0},
                {1, 2, 4, 0, 0, 0, 0, 0, 9},
                {0, 0, 0, 0, 0, 0, 0, 0, 0},
                {9, 0, 0, 0, 0, 0, 8, 2, 4},
                {0, 5, 0, 4, 0, 0, 1, 0, 0},
                {0, 8, 0, 0, 7, 0, 2, 0, 5},
                {0, 9, 0, 5, 0, 0, 0, 0, 7}
        },
        { // Puzzle 8 from http://www.sudoku.org.uk/bifurcation.htm
                {6, 5, 2, 0, 4, 8, 0, 0, 7},
                {0, 7, 0, 2, 0, 5, 4, 0, 0},
                {0, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 6, 4, 1, 0, 0, 0, 7, 0},
                {0, 0, 0, 0, 8, 0, 0, 0, 0},
                {0, 8, 0, 0, 0, 4, 5, 6, 0},
                {0, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 8, 6, 0, 7, 0, 2, 0},
                {2, 0, 0, 8, 9, 0, 7, 5, 1}
        },
        { // Puzzle 9 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 6, 0, 0, 2, 0, 0, 9},
                {1, 0, 0, 5, 0, 0, 0, 2, 0},
                {0, 4, 7, 3, 0, 6, 0, 0, 1},
                {0, 0, 0, 0, 0, 8, 0, 4, 0},
                {0, 3, 0, 0, 0, 0, 0, 7, 0},
                {0, 1, 0, 6, 0, 0, 0, 0, 0},
                {4, 0, 0, 8, 0, 3, 2, 1, 0},
                {0, 6, 0, 0, 0, 1, 0, 0, 4},
                {3, 0, 0, 4, 0, 0, 9, 0, 0}
        },
        { // Puzzle 10 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 4, 0, 5, 0, 9, 0, 0},
                {0, 0, 0, 0, 7, 0, 0, 0, 6},
                {3, 7, 0, 0, 0, 0, 0, 0, 2},
                {0, 0, 9, 5, 0, 0, 0, 8, 0},
                {0, 0, 1, 2, 0, 4, 3, 0, 0},
                {0, 6, 0, 0, 0, 9, 2, 0, 0},
                {2, 0, 0, 0, 0, 0, 0, 9, 3},
                {1, 0, 0, 0, 4, 0, 0, 0, 0},
                {0, 0, 6, 0, 2, 0, 7, 0, 0}
        },
        { // Puzzle 11 from http://www.sudoku.org.uk/bifurcation.htm
                {0, 0, 0, 0, 3, 0, 7, 9, 0},
                {3, 0, 0, 0, 0, 0, 0, 0, 5},
                {0, 0, 0, 4, 0, 7, 3, 0, 6},
                {0, 5, 3, 0, 9, 4, 0, 7, 0},
                {0, 0, 0, 0, 7, 0, 0, 0, 0},
                {0, 1, 0, 8, 2, 0, 6, 4, 0},
                {7, 0, 1, 9, 0, 8, 0, 0, 0},
                {8, 0, 0, 0, 0, 0, 0, 0, 1},
                {0, 9, 4, 0, 1, 0, 0, 0, 0}
        },
        { // From http://www.sudoku.org.uk/discus/messages/29/51.html?1131034031
                {2, 5, 8, 1, 0, 4, 0, 3, 7},
                {9, 3, 6, 8, 2, 7, 5, 1, 4},
                {4, 7, 1, 5, 3, 0, 2, 8, 0},

                {7, 1, 5, 2, 0, 3, 0, 4, 0},
                {8, 4, 9, 6, 7, 5, 3, 2, 1},
                {3, 6, 2, 4, 1, 0, 0, 7, 5},

                {1, 2, 4, 9, 0, 0, 7, 5, 3},
                {5, 9, 3, 7, 4, 2, 1, 6, 8},
                {6, 8, 7, 3, 5, 1, 4, 9, 2}
        }
};

/**
 * SudokuOptions for choosing which sudoku to solve by providing command-line options
 */
class SudokuOptions : public Options {
private:
    Driver::UnsignedIntOption _sudoku;
public :
    SudokuOptions(const char *e) :
            Options(e),
            _sudoku("-sudoku", "sudoku number [0,17", 0) {
        add(_sudoku);
    }
    void parse(int &argc, char *argv[]) {
        Options::parse(argc, argv);
    }
    int sudoku(void) const {
        return _sudoku.value();
    }
};

/**
 * ComputationSpace/Script for the sudoku problem,
 * contains variables, posting constraints and branching strategies
 */
class Sudoku : public Script {
public:
    //One IntVar per position in sudoku
    IntVarArray sudokuPositions;

    Sudoku(const SudokuOptions &opt) :
            ScriptBase(opt),
            sudokuPositions(*this, 9 * 9, 1, 9) {

        Matrix<IntVarArray> sudokuMatrix(sudokuPositions, 9, 9);

        //Add constraints for the pre-filled positions
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                int value = examples[opt.sudoku()][i][j]; //picked A1
                if (value != 0) //Found a non-blank
                    rel(*this, sudokuMatrix(j, i) == value);
            }
        }

        //Distinct row and distinct column constraints
        for (int i = 0; i < 9; i++) {
            distinct(*this, sudokuMatrix.row(i), opt.ipl());
            distinct(*this, sudokuMatrix.col(i), opt.ipl());
        }
        //Each 3x3 square should have all digits 1-9 constraint
        for (int i = 0; i < 9; i += 3) {
            for (int j = 0; j < 9; j += 3) {
                distinct(*this, sudokuMatrix.slice(i, i + 3, j, j + 3), opt.ipl());
            }
        }

        //Branching strategy, first fail
        branch(*this, sudokuPositions, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
    }

    //Copy-constructor for backtracking
    Sudoku(bool share, Sudoku &space) : Script(share, space) {
        sudokuPositions.update(*this, share, space.sudokuPositions);
    }

    //Auxillary function for copying
    virtual Sudoku *copy(bool share) {
        return new Sudoku(share, *this);
    }

    //Print sudokuPositions
    virtual void print(std::ostream &os) const {
        os << "-------------------------" << std::endl;
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 3; j++) {
                os << "|" << sudokuPositions[i * 9 + j];
            }
            os << "  ";
            for (int j = 3; j < 6; j++) {
                os << "|" << sudokuPositions[i * 9 + j];
            }
            os << "|  ";
            for (int j = 6; j < 9; j++) {
                os << "|" << sudokuPositions[i * 9 + j];
            }
            os << "|" << std::endl;
            if (i == 2 || i == 5)
                os << std::endl;

        }
        os << "-------------------------" << std::endl;
    }
};

/**
 * Program entrypoint, parses commandline options and initializes search engine with root-node.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    // commandline options
    SudokuOptions opt("Sudoku");

    //Default options
    opt.solutions(1);
    opt.mode(ScriptMode::SM_SOLUTION);
    opt.ipl(IPL_DEF);

    //parse cmd (potentially overwrite default options)
    opt.parse(argc, argv);

    //run script with DFS engine
    Script::run<Sudoku, DFS, SudokuOptions>(opt);

    /**
     * Example cmd to solve sudoku number 0 with different options, for more options see Gecode.org:
     * ./bin/sudoku -sudoku 0 -mode gist -ipl dom
     * ./bin/sudoku -sudoku 0 -mode solution -ipl speed
     * ./bin/sudoku -sudoku 0 -mode time -ipl def
     * ./bin/sudoku -sudoku 0 -mode stat -ipl memory
     *
     * or with default (0, solution, def):
     * ./bin/sudoku
     */

    return 0;
}
