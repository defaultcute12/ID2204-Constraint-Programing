//
// square_packing_with_overlap_and_interval.cpp
// Created by Kim Hammar & Mallu Goswami on 2017-05-01.
//

#include <gecode/int.hh>

using namespace Gecode;

using namespace Gecode::Int;

/*
 * Custom brancher for forcing mandatory parts
 *
 */
class IntervalBrancher : public Brancher {
protected:
    // Views for x-coordinates (or y-coordinates)
    ViewArray <IntView> x;
    // Width (or height) of rectangles
    int *w;
    // Percentage for obligatory part
    double p;
    // Cache of first unassigned view
    mutable int start;

    // Description
    class Description : public Choice {
    public:
        // Position of view
        int pos;
        int split;
        // You might need more information, please add here

        /* Initialize description for brancher b, number of
         *  alternatives a, position p, and split-mark.
         */
        Description(const Brancher &b, unsigned int a, int p, int split)
                : Choice(b, a), pos(p), split(split) {}

        // Report size occupied
        virtual size_t size(void) const {
            return sizeof(Description);
        }

        // Archive the choice's information in e
        virtual void archive(Archive &e) const {
            Choice::archive(e);
            // You must also archive the additional information
            e << pos << split;
        }
    };

public:
    // Construct branching
    IntervalBrancher(Home home,
                     ViewArray <IntView> &x0, int w0[], double p0)
            : Brancher(home), x(x0), w(w0), p(p0), start(0) {}

    // Post branching
    static void post(Home home, ViewArray <IntView> &x, int w[], double p) {
        (void) new(home) IntervalBrancher(home, x, w, p);
    }

    // Copy constructor used during cloning of b
    IntervalBrancher(Space &home, bool share, IntervalBrancher &b)
            : Brancher(home, share, b), p(b.p), start(b.start) {
        x.update(home, share, b.x);
        w = home.alloc<int>(x.size());
        for (int i = x.size(); i--;)
            w[i] = b.w[i];
    }

    // Copy brancher
    virtual Actor *copy(Space &home, bool share) {
        return new(home) IntervalBrancher(home, share, *this);
    }

    // Check status of brancher, return true if alternatives left
    virtual bool status(const Space &home) const {
        for (int i = start; i < x.size(); ++i) {
            /**
             * If x already assigned there is no branching to do.
             */
            if (!x[i].assigned()) {
                /**
                 * If x-range has space for an obligatory part of size p*size then we can branch.
                 */
                if ((x[i].min() + w[i] - std::ceil(p * w[i])) < x[i].max()) {
                    start = i; //update variable we are branching on
                    return true;
                }
            }
        }
        return false; //no more branching possible
    }

    // Return choice as description
    virtual const Choice *choice(Space &home) {
        int obligatoryPartSize = std::ceil(p * w[start]);
        int split = x[start].min() + w[start] - obligatoryPartSize;
        int noAlternatives = 2;
        /**
         * Binary branching such that first x-interval is [x.min(), split], which enforces obligatory part
         * second x-interval will thus be (split,  x.max()]
         * obligatoryPart is [x.min(), split ()]
         * start = current variable position we are branching on
         */
        return new Description(*this, noAlternatives, start, split);
    }

    // Construct choice from archive e
    virtual const Choice *choice(const Space &, Archive &e) {
        // Again, you have to take care of the additional information
        int alternative, split;
        e >> alternative >> split;
        return new Description(*this, alternative, p, split);
    }

    // Perform commit for choice c and alternative a
    virtual ExecStatus commit(Space &home, const Choice &c, unsigned int a) {
        const Description &d = static_cast<const Description &>(c);
        /**
         * First alternative, interval [x.min, split], enforces obligatory part to be p % of side size.
         */
        if (a == 0) {
            GECODE_ME_CHECK(x[d.pos].lq(home, d.split));
        }
        /**
         * Second alternative, interval (split - x.max], keep the values that is excluded by first branching
         * to keep the branches disjunctive.
         */
        if (a == 1) {
            GECODE_ME_CHECK(x[d.pos].gr(home, d.split));
        }
        return ES_OK;
    }

    // Print some information on stream o (used by Gist, from Gecode 4.0.1 on)
    virtual void print(const Space &home, const Choice &c, unsigned int b,
                       std::ostream &o) const {

        const Description &d = static_cast<const Description &>(c);

        if (b == 0) {
            o << "First branch-alternative" << std::endl;
            o << "x[" << d.pos << "]" << "| interval: [" << x[d.pos].min() << "," << d.split << "]";
        }
        if (b == 1) {
            o << "Second branch-alternative" << std::endl;
            o << "x[" << d.pos << "]" << "| interval: (" << d.split << "," << x[d.pos].max() << "]";
        }

    }
};

// This posts the interval branching
void interval(Home home, const IntVarArgs &x, const IntArgs &w, double p) {
    // Check whether arguments make sense
    if (x.size() != w.size())
        throw ArgumentSizeMismatch("interval");
    // Never post a branching in a failed space
    if (home.failed()) return;
    // Create an array of integer views
    ViewArray <IntView> vx(home, x);
    // Create an array of integers
    int *wc = static_cast<Space &>(home).alloc<int>(x.size());
    for (int i = x.size(); i--;)
        wc[i] = w[i];
    // Post the brancher
    IntervalBrancher::post(home, vx, wc, p);
}

//
// square_packing_with_overlap.cpp
// Created by Kim Hammar & Mallu Goswami on 2017-04-21.
//

#include <gecode/int.hh>

using namespace Gecode;
using namespace Gecode::Int;

// The no-overlap propagator
class NoOverlap : public Propagator {
protected:
    // The x-coordinates
    ViewArray<IntView> x;
    // The width (array)
    int *w;
    // The y-coordinates
    ViewArray<IntView> y;
    // The heights (array)
    int *h;
public:
    // Create propagator and initialize
    NoOverlap(Home home, ViewArray<IntView> &x0, int w0[], ViewArray<IntView> &y0, int h0[]) :
    //Initialize variables
            Propagator(home),
            x(x0),
            w(w0),
            y(y0),
            h(h0) {
        //Subscription controls the execution of hte propagator
        x.subscribe(home, *this, PC_INT_BND); //Subscribe to changes in the x-view
        y.subscribe(home, *this, PC_INT_BND); //Subscribe to changes in the y-view
    }

    // Post no-overlap propagator. Post function decides whether propagation is necessary and then creates the propagator
    // if needed
    static ExecStatus post(Home home, ViewArray<IntView> &x, int w[], ViewArray<IntView> &y, int h[]) {
        // Only if there is something to propagate
        if (x.size() > 1)
            (void) new(home) NoOverlap(home, x, w, y, h);
        return ES_OK;
    }

    // Copy constructor during cloning
    NoOverlap(Space &home, bool share, NoOverlap &p)
            : Propagator(home, share, p) {
        x.update(home, share, p.x);
        y.update(home, share, p.y);
        // Also copy width and height arrays
        w = home.alloc<int>(x.size());
        h = home.alloc<int>(y.size());
        for (int i = x.size(); i--;) {
            w[i] = p.w[i];
            h[i] = p.h[i];
        }
    }

    // Create copy during cloning
    virtual Propagator *copy(Space &home, bool share) {
        return new(home) NoOverlap(home, share, *this);
    }

    // Re-schedule function after propagator has been re-enabled
    virtual void reschedule(Space &home) {
        x.reschedule(home, *this, PC_INT_BND);
        y.reschedule(home, *this, PC_INT_BND);
    }

    // Return cost (defined as cheap quadratic complexity)
    virtual PropCost cost(const Space &, const ModEventDelta &) const {
        return PropCost::quadratic(PropCost::LO, 2 * x.size());
    }

    // Perform propagation
    virtual ExecStatus propagate(Space &home, const ModEventDelta &) {
        int assigned = 0; //Count how many of the variables are assigned to detect subsumption.
        bool canOverlap = false;
        for (int i = 0; i < x.size(); ++i) {
            bool xCanOverlap = false;
            bool yCanOverlap = false;
            if (x[i].assigned() && y[i].assigned())
                assigned++;
            for (int j = 0; j < x.size(); ++j) {
                if (j != i) {
                    //square i and j overlaps on x-axis so propagate (bounds propagation) that they cant overlap on y-axis
                    if
                            (
                            (x[i].max() <= x[j].min() && x[i].min() + w[i] > x[j].max()) ||
                            (x[j].max() <= x[i].min() && x[j].min() + w[j] > x[i].max())
                            )
                    {
                        if (y[i].max() <= y[j].min())
                            GECODE_ME_CHECK(y[j].gq(home, y[i].min() + h[i]));

                        if (y[i].min() + h[i] > y[j].max())
                            GECODE_ME_CHECK(y[i].gr(home, y[j].min()));

                        if (y[j].max() <= y[i].min())
                            GECODE_ME_CHECK(y[i].gq(home, y[j].min() + h[j]));

                        if (y[j].min() + h[j] > y[i].max())
                            GECODE_ME_CHECK(y[j].gr(home, y[i].min()));
                    }
                    //square i and j overlaps on y-axis so propagate (bounds propagation) that they cant overlap on x-axis
                    if
                            (
                            (y[i].max() <= y[j].min() && y[i].min() + h[i] > y[j].max()) ||
                            (y[j].max() <= y[i].min() && y[j].min() + h[j] > y[i].max())
                            )
                    {
                        if (x[i].max() <= x[j].min())
                            GECODE_ME_CHECK(x[j].gq(home, x[i].min() + w[i]));

                        if (x[i].min() + w[i] > x[j].max())
                            GECODE_ME_CHECK(x[i].gr(home, x[j].min()));

                        if (x[j].max() <= x[i].min())
                            GECODE_ME_CHECK(x[i].gq(home, x[j].min() + w[j]));

                        if (x[j].min() + w[j] > x[i].max())
                            GECODE_ME_CHECK(x[j].gr(home, x[i].min()));
                    }
                    if (!xCanOverlap)
                        xCanOverlap =
                                //!(Condition where x_i and x_j can never overlap)
                                //I.e if x_i and x_j can never overlap, xCanOverlap = false
                                !(
                                        ((x[i].min() > x[j].max()) || (x[i].max() + w[i] <= x[j].min()))
                                        &&
                                        ((x[j].min() > x[i].max()) || (x[j].max() + w[j] <= x[i].min()))

                                );

                    if (!yCanOverlap)
                        yCanOverlap =
                                //!(Condition where y_i and y_j can never overlap)
                                //I.e if y_i and y_j can never overlap, yCanOverlap = false
                                !(
                                        ((y[i].min() > y[j].max()) || (y[i].max() + h[i] <= y[j].min()))
                                        &&
                                        ((y[j].min() > y[i].max()) || (y[j].max() + h[j] <= y[i].min()))

                                );
                }
            }
            if (!canOverlap)//If no previous squares could overlap, update the bool by checking if these 2 squares can overlap.
                canOverlap = xCanOverlap && yCanOverlap;
        }
        if (!canOverlap)
            return home.ES_SUBSUMED(*this); //No variable domains can overlap no matter assignment, no more propagation necessary

        if (assigned == y.size())
            return home.ES_SUBSUMED(*this); //All variables assigned, no more propagation necessary.
        return ES_NOFIX; //Propagator is not idempotent, max and min bounds might change and affect propagation.
    }

    // Dispose propagator and return its size (dispose works as garbage collection, must cancel subscription first).
    virtual size_t dispose(Space &home) {
        x.cancel(home, *this, PC_INT_BND);
        y.cancel(home, *this, PC_INT_BND);
        (void) Propagator::dispose(home);
        return sizeof(*this);
    }
};

/*
 * Post the constraint that the rectangles defined by the coordinates
 * x and y and width w and height h do not overlap.
 *
 * This is the function that you will call from your model. The best
 * is to paste the entire file into your model.
 *
 * Post function checks whether arguments are correct and whether the the space is failed or not before posting the
 * propagator.
 */
void nooverlap(Space &home,
               const IntVarArgs &x, const IntArgs &w,
               const IntVarArgs &y, const IntArgs &h) {
    // Check whether the arguments make sense
    if ((x.size() != y.size()) || (x.size() != w.size()) ||
        (y.size() != h.size()))
        throw ArgumentSizeMismatch("nooverlap");
    // Never post a propagator in a failed space
    if (home.failed()) return;
    // Set up array of views for the coordinates
    ViewArray<IntView> vx(home, x);
    ViewArray<IntView> vy(home, y);
    // Set up arrays (allocated in home) for width and height and initialize
    int *wc = static_cast<Space &>(home).alloc<int>(x.size());
    int *hc = static_cast<Space &>(home).alloc<int>(y.size());
    for (int i = x.size(); i--;) {
        wc[i] = w[i];
        hc[i] = h[i];
    }
    // If posting failed, fail space
    if (NoOverlap::post(home, vx, wc, vy, hc) != ES_OK)
        home.fail();
}

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

using namespace Gecode;

/**
 * ObligatoryPartSizeOptions for choosing how large the obligatory part in interval-branching should be in percentage.
 */
class ObligatoryPartSizeOptions : public Options {
private:
    Driver::DoubleOption _obligatory;
    Driver::UnsignedIntOption _dimension;
public :
    ObligatoryPartSizeOptions(const char *e) :
            Options(e),
            _obligatory("-obligatory", "Obligatory part size in percentage 0.0-1.0", 0.35),
            _dimension("-dimension", "Square dimension integer > 1", 2){
        add(_obligatory);
        add(_dimension);
    }

    void parse(int &argc, char *argv[]) {
        Options::parse(argc, argv);
    }

    double obligatory(void) const {
        return _obligatory.value();
    }

    int dimension(void) const {
        return _dimension.value();
    }
};

class SquarePacking : public Script {

public:
    const int n;
    const double p;
    IntVar s;
    IntVarArray xCoords, yCoords;

    SquarePacking(const ObligatoryPartSizeOptions &opt) :
            Script(opt),
            n(opt.dimension()),
            p(opt.obligatory()),
            s(*this, nSquaresArea(), nSquaresStacked(n)), //Problem decomposition, constraint min and max of s, s will be the first branching to enumerate subproblems.
            xCoords(*this, n - 1, 0, nSquaresStacked(n)),//min coordinate = (0,0) max = (s,s). exclude 1x1 square
            yCoords(*this, n - 1, 0, nSquaresStacked(n))//min coordinate = (0,0) max = (s,s). exclude 1x1 square
    {

        /**
         * Constraint on the origin coordinate of the squares.
         * Square must be within enclosing square (s x s) (>= 0) and must not
         * exceed x or y axis (<= s-size(i).
         */
        for (int i = 0; i < n - 1; ++i) {
            rel(*this, xCoords[i] >= 0);
            rel(*this, xCoords[i] <= s - size(i));
            rel(*this, yCoords[i] >= 0);
            rel(*this, yCoords[i] <= s - size(i));
        }

        /**
         * Apply constraints on coordinates that squares should not overlap (disjoint)
         */
        IntArgs w(n - 1);
        IntArgs h(n - 1);

        for (int i = 0; i < n - 1; i++) {
            w[i] = size(i);
            h[i] = size(i);
        }
        nooverlap(*this, xCoords, w, yCoords, h);

        /**
         * Apply (cumulative) constraints of max sum(squareHeight) on columns and max sum(squareWidth) on rows.
         * Redundant constraints to increase propagation, the non-overlapping coordinates implies this constraint.
         * This redundant constraint have very big impact on performance.
         */
        for (int i = 0; i < s.max(); ++i) {
            BoolVarArgs colOverlap(*this, n - 1, 0, 1);
            BoolVarArgs rowOverlap(*this, n - 1, 0, 1);
            for (int j = 0; j < n - 1; ++j) {
                dom(*this, xCoords[j], i - size(j) + 1, i, colOverlap[j]);//x <= colIndex < x means overlap
                dom(*this, yCoords[j], i - size(j) + 1, i, rowOverlap[j]);//y <= rowIndex < y means overlap
            }
            /**
             * sum of the sizes of the squares occupying space at column x must be less than or equal to s.
             * sum of the sizes of the squares occupying space at row y must be less than or equal to s.
             */
            rel(*this, sum(IntArgs::create(n - 1, n, -1), colOverlap) <= s, opt.ipl());
            rel(*this, sum(IntArgs::create(n - 1, n, -1), rowOverlap) <= s, opt.ipl());
        }

        /**
         * Symmetry breaking. Restrict placement of the largest inside-square (n x n)
         */
        rel(*this, xCoords[0] <= 1 + (s - n) / 2);
        rel(*this, yCoords[0] <= xCoords[0]);

        /**
         * Empty-strip dominance
         */
        int gapLim = n - 1 > 45 ? 45 : n - 1;
        for (int i = 2; i < gapLim; ++i) {
            rel(*this, xCoords[i] != gap_generic(i));
            rel(*this, yCoords[i] != gap_generic(i));
            if (i < 4)
                rel(*this, yCoords[i] != gap_specific(i));
        }

        /**
         * Branching strategy
         */
        branch(*this, s, INT_VAL_MIN()); //Branch first on s

        interval(*this, xCoords, w, p);
        interval(*this, yCoords, w, p);

        //Try larger squares first, larger squares have smaller domains, try small x,y coords first (left-to-right, bottom-to-top)
        branch(*this, xCoords, INT_VAR_SIZE_MIN(), INT_VAL_MIN()); //Assign x-coords first
        branch(*this, yCoords, INT_VAR_SIZE_MIN(), INT_VAL_MIN()); //Assign y-coords second
    }


    /**
     * helper function
     * @return square size of index i
     */
    int size(int i) {
        return n - i;
    }

    /**
     * Get the minimum area of n-squares
     * @return
     */
    int nSquaresArea() {
        return ceil(sqrt(n * (n + 1) * (2 * n + 1) / 6));
    }

    /**
     * Get the area to fit the n squares stacked on top of each other, i.e an upper bound on the size of s.
     *
     * @return
     */
    int nSquaresStacked(int i) {
        if (i == 0)
            return 0;
        return i + nSquaresStacked(i - 1);
    }

    /**
     * Prune variable domains with gaps where equivalent placement have already been investigated.
     *
     * @param i
     * @return gap
     */
    int gap_generic(int i) {
        if (i == 2)
            return 2;
        if (i == 3 || i == 4)
            return 2;
        if (i >= 5 && i <= 8)
            return 3;
        if (i >= 9 && i <= 11)
            return 4;
        if (i >= 12 && i <= 17)
            return 5;
        if (i >= 18 && i <= 21)
            return 6;
        if (i >= 22 && i <= 29)
            return 7;
        if (i >= 30 && i <= 34)
            return 3;
        if (i >= 34 && i <= 44)
            return 9;
        if (i == 45)
            return 10;
        return -1;
    }

    /**
     * Square-packing specific investigated gaps.
     *
     * @param i
     * @return gap
     */
    int gap_specific(int i) {
        if (i == 2)
            return 2;
        if (i == 3)
            return 3;
        return -1;
    }

/// Constructor for cloning
    SquarePacking(bool share, SquarePacking &space) : Script(share, space), n(space.n), p(space.p) {
        s.update(*this, share, space.s);
        xCoords.update(*this, share, space.xCoords);
        yCoords.update(*this, share, space.yCoords);
    }

    /// Perform copying during cloning
    virtual Space *
    copy(bool share) {
        return new SquarePacking(share, *this);
    }

    /// Print solution
    virtual void print(std::ostream &os) const {
        os << "SquarePacking Solution: " << std::endl;
        os << "Enclosing square size: " << s << "x" << s << std::endl;
        os << "Square coordinates (different square 1 solutions are excluded since it can be placed anywhere (almost)):" << std::endl;
        for (int i = 0; i < n - 1; ++i) {
            os << "square" << n - i << ": (" << xCoords[i] << "," << yCoords[i] << ") ";
        }
        os << std::endl;

    }
};

/**
 * Program entrypoint, parses commandline options and initializes search engine with root-node.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {

    //Commandline options
    ObligatoryPartSizeOptions opt("SquarePacking");

    //Default options
    opt.solutions(0);//0 means find all solutions.
    opt.iterations(100); //how many iterations before measuring runtime
    //opt.size(10); //n size
    opt.mode(ScriptMode::SM_SOLUTION); //Solution mode (i.e no GIST) is default
    opt.ipl(IPL_DEF); //Default propagation strength
    opt.parse(argc, argv);


    //parse cmd (potentially overwrite default options)
    opt.parse(argc, argv);

    //run script with DFS engine
    Script::run<SquarePacking, DFS, ObligatoryPartSizeOptions>(opt);

    /**
     * Example cmd to solve:
     * ./bin/square_packing_with_overlap_and_interval -mode gist -ipl dom -solutions 1 -dimension 3 -obligatory 0.35
     * ./bin/square_packing_with_overlap_and_interval -mode solution -ipl speed -solutions 0 -dimension 3 -obligatory 0.35
     * ./bin/square_packing_with_overlap_and_interval -mode time -ipl def -solutions 0 -dimension 3 -obligatory 0.35
     * ./bin/square_packing_with_overlap_and_interval -mode stat -ipl memory -solutions 0 -dimension 3 -obligatory 0.35
     *
     */
    return 0;
}
