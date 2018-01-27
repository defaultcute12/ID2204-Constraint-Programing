//Created By Mallu 2017-05-03
#include <gecode/int.hh>
#include <gecode/driver.hh>
#include <gecode/minimodel.hh> //rel

using namespace Gecode;

int n;

class Square : public Script {
public:
//task 1 
    IntVar s;       // size of square
    IntVarArray xCor,yCor;  // xCor coordinates yCor coordinates


    //static function for size task 1 
    static int size(int i){
        return n-i;
    }
    //to put a value constraint so that xCor cor and yCor cor are equal to n 
    static int sumLength(int n){
        int sum = 0;
        for(int i=0; i<n; i++){
            sum += n-i;
        }
        return sum;
    }
    
    Square(const SizeOptions& opt): Script(opt), xCor(*this, n, 0, sumLength(n)), yCor(*this, n, 0 , sumLength(n)) {
             
        s = IntVar(*this, floor(sqrt(n*(n+1)*(2*n+1)/6)), sumLength(n)); //nsqa sum


        //constraint for squares to be packed in s xcor and ycor values should be less eq s
        for (int i = 0; i < n-1; i++) {
            rel(*this, (xCor[i] + size(i)) <= s);
            rel(*this, (yCor[i] + size(i)) <= s);
        }

        //constraint for packing to be consistent
        IntArgs sizes(n);        
        for (int i = 0; i < (n - 1); i++) {
            sizes[i] = size(i);
        }
        
        //task 2 no pverlap reification constaint
        for (int i=0; i<n-1; i++) {
            for(int j=i+1; j<n-1; j++){
                IntVar LEFT(*this,0, sumLength(n)+size(i));
                IntVar RIGHT(*this,0, sumLength(n)+size(j));
                IntVar ABOVE(*this,0, sumLength(n)+size(i));
                IntVar BELOW(*this,0, sumLength(n)+size(j));
                
                BoolVarArgs b(*this,4,0,1);               

                rel(*this, LEFT == xCor[i]+size(i));
                rel(*this, RIGHT == xCor[j]+size(j));
                rel(*this, ABOVE == yCor[i]+size(i));
                rel(*this, BELOW == yCor[j]+size(j));
                rel(*this, LEFT, IRT_LQ, xCor[j], b[0]);
                rel(*this, RIGHT, IRT_LQ, xCor[i], b[1]); //reifiied
                rel(*this, ABOVE, IRT_LQ, yCor[j], b[2]);
                rel(*this, BELOW, IRT_LQ, yCor[i], b[3]);                
                linear(*this, b, IRT_GQ, 1);
            }
        }  
        //task3 parsing all columns and rows s.max()
        for(int k=0; k<s.max(); k++){
            BoolVarArgs xsum(*this,n,0,1);
            BoolVarArgs ysum(*this,n,0,1);
            for(int i=0; i<n-1; i++){
                dom(*this, xCor[i], k, k+size(i),xsum[i]);
                dom(*this, yCor[i], k, k+size(i),ysum[i]);
            }
            linear(*this, sizes, ysum, IRT_LQ, s);
            linear(*this, sizes, xsum, IRT_LQ, s);
        }

        //task 4 symmetery  part 2
        rel(*this, yCor[0] <= xCor[0]);
        rel(*this, xCor[0] <= 1+((s-size(0))/2));
        rel(*this, yCor[0] <= 1+((s-size(0))/2));

        //task4 part 3 strip dominance initial domain reduction
       if(n == 2) {
            rel(*this, xCor[0], IRT_LE, 2);
            rel(*this, yCor[0], IRT_LE, 2);
        } else if (n == 3) {
            rel(*this, xCor[0], IRT_LE, 3);
            rel(*this, yCor[0], IRT_LE, 3);
        } else if (n == 4) {
            rel(*this, xCor[0], IRT_LE, 2);
            rel(*this, yCor[0], IRT_LE, 2);
        } else if(n >= 5 && n <= 8) {
            rel(*this, xCor[0], IRT_LE, 3);
            rel(*this, yCor[0], IRT_LE, 3);
        } else if (n >= 9 && n <= 11) {
            rel(*this, xCor[0], IRT_LE, 4);
            rel(*this, yCor[0], IRT_LE, 4);
        } else if(n >= 12 && n <= 17) {
            rel(*this, xCor[0], IRT_LE, 5);
            rel(*this, yCor[0], IRT_LE, 5);
        } else if (n >= 18 && n <= 21) {
            rel(*this, xCor[0], IRT_LE, 6);
            rel(*this, yCor[0], IRT_LE, 6);
        }
            
       //task 5 branching start from 
        branch(*this, s, INT_VAL_MIN());
        branch(*this, xCor, INT_VAR_NONE(), INT_VAL_MIN());
        branch(*this, yCor, INT_VAR_NONE(), INT_VAL_MIN());
    }
    
    /// Constructor for cloning
    Square(bool share, Square& sq) : Script(share,sq) {
        xCor.update(*this, share, sq.xCor);
        yCor.update(*this, share, sq.yCor);
        s.update(*this, share, sq.s);
    }
    
    /// Perform copying during cloning
    virtual Space*
    copy(bool share) {
        return new Square(share,*this);
    }
    
    
    virtual void print(std::ostream& os) const {
        os << "\t";
        os << "SIZE = " << s << std::endl << "\t";
        for (int i = 0; i < n; i++) {
            if (i!=n-1){
                os  << size(i) << "\tPosition Coordinates (X,Y) " <<  xCor[i] << "," << yCor[i];
                os << std::endl << "\t";
            } else {
                os << ("1*1 smallest free to be placed") ;
            }
        }        
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[]) {
    SizeOptions opt("Square");
    int N;
    std::cout << "ENTER value of N" << std::endl; //let user specify no of square
    std::cin >> N;
    while(std::cin.fail() || N < 0) {
        std::cout << "ERROR" << std::endl;
        std::cin >> N;
    }
    opt.size(N);
    n = opt.size();
    opt.parse(argc,argv);
    Script::run<Square,BAB,SizeOptions>(opt);
    return 0;
}
