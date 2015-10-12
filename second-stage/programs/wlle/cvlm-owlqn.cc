// cvlm-owlqn.cc -- A linear model estimator for a variety of user-selectable loss functions.

const char usage[] =
"cvlm-owlqn -- A cross-validating linear model estimator for a variety of user-selectable loss functions.\n"
"\n"
" Mark Johnson, 21st July 2008\n"
"\n"
" It uses a modified version of L-BFGS developed by Galen Andrew at Microsoft Research\n"
" to be especially efficient for L1-regularized loss functions.\n"
"\n"
" The regularizer weight(s) are set by cross-validation on development data.\n"
"\n"
"Usage: cvlm-owlqn [-h] [-d debug_level] [-c c0] [-C c00] [-p p] [-r r] [-s s] [-t tol]\n"
"                  [-l ltype] [-F f] [-G] [-n ns] [-f feat-file]\n"
"                  [-o weights-file]  [-e eval-file] [-x eval-file2]\n"
"	           < train-file\n"
"\n"
"where:\n"
"\n"
" debug_level > 0 controls the amount of output produced\n"
"\n"
" -c c0 is the initial value for the regularizer constant.\n"
"\n"
" -C c00 multiplies the regularizer constant for the first feature class\n"
" by c00 (this can be used to allow the first feature class to be regularized less).\n"
"\n"
" -l ltype identifies the type of loss function used:\n"
"\n"
"    -l 0 - log loss (c0 ~ 5)\n"
"    -l 1 - EM-style log loss (c0 ~ 5)\n"
"    -l 2 - pairwise log loss \n"
"    -l 3 - exp loss (c0 ~ 25, s ~ 1e-5)\n"
"    -l 4 - log exp loss (c0 ~ 1e-4)\n"
"    -l 5 - maximize expected F-score (c ~ ?)\n"
"\n"
" -r r specifies that the weights are initialized to random values in\n"
"   [-r ... +r],\n"
"\n"
" -t tol specifies the stopping tolerance for the IWLQN optimizer\n"
"\n"
" -F f indicates that a parse should be taken as correct\n"
"   proportional to f raised to its f-score, and\n"
"\n"
" -G indicates that each sentence is weighted by the number of\n"
"   edges in its gold parse.\n"
"\n"
" -n ns is the maximum number of ':' characters in a <featclass>, used to\n"
" determine how features are binned into feature classes (ns = -1 bins\n"
" all features into the same class)\n"
"\n"
" -f feat-file is a file of <featclass> <featuredetails> lines, used for\n"
" cross-validating regularizer weights,\n"
"\n"
" train-file, eval-file and eval-file2 are files from which training and evaluation\n"
" data are read (if eval-file ends in the suffix .bz2 then bzcat is used\n"
" to read it; if no eval-file is specified, then the program tests on the\n"
" training data),\n"
"\n"
" weights-file is a file to which the estimated weights are written,\n"
"\n"
"The function that the program minimizes is:\n"
"\n"
"   Q(w) = s * (- L(w) + c * sum_j pow(fabs(w[j]), p) ), where:\n"
"\n"
"   L(w) is the loss function to be optimized.\n"
"\n"
"With debug = 0, the program writes a single line to stdout:\n"
"\n"
"c p r s it nzeroweights/nweights neglogP/nsentence ncorrect/nsentences\n"
"\n"
"With debug >= 10, the program writes out a histogram of weights as well\n"
"\n"
"Data format:\n"
"-----------\n"
"\n"
"<Data>     --> [S=<NS>] <Sentence>*\n"
"<Sentence> --> [G=<G>] N=<N> <Parse>*\n"
"<Parse>    --> [P=<P>] [W=<W>] <FC>*,\n"
"<FC>       --> <F>[=<C>]\n"
"\n"
"NS is the number of sentences.\n"
"\n"
"Each <Sentence> consists of N <Parse>s.  <G> is the gold standard\n"
"score.  To get parsing precision and recall results, set <G> to the\n"
"number of edges in the gold standard parse.  To get accuracy results,\n"
"set <G> to 1 (the default).\n"
"\n"
"A <Parse> consists of <FC> pairs.  <P> is the parse's possible highest\n"
"score and <W> is the parse's actual score.  To get parsing precision and\n"
"recall results, set <P> to the number of edges in the parse and <W> to\n"
"the number of edges in common between the gold and parse trees.\n"
"\n"
"A <FC> consists of a feature (a non-negative integer) and an optional\n"
"count (a real).\n"
"\n"
"The default for all numbers except <W> is 1.  The default for <W> is 0.\n";

#include "custom_allocator.h"    // must come first
#define _GLIBCPP_CONCEPT_CHECKS  // uncomment this for checking

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

#include "lmdata.h"
#include "powell.h"
#include "utility.h"

#include "OWLQN.h"
#include "TerminationCriterion.h"

typedef std::vector<double> doubles;
typedef std::vector<size_t> size_ts;

int debug_level = 0;

enum loss_type { log_loss, em_log_loss, em_log_loss_noomp, pairwise_log_loss, exp_loss, log_exp_loss, 
		 expected_fscore_loss };

const char* loss_type_name[] = { "log_loss", "em_log_loss", "em_log_loss_noomp", "pairwise_log_loss", "exp_loss", 
				 "log_exp_loss", "expected_fscore_loss" };

void print_histogram(int nx, const double x[], int nbins=20) {
  int nx_nonzero = 0;
  for (int i = 0; i < nx; ++i)
    if (x[i] != 0)
      ++nx_nonzero;

  std::cout << "#   There are " << nx_nonzero << " non-zero values and " 
	    << nx-nx_nonzero << " zero values." << std::endl;

  if (nx_nonzero > 0) {
    std::vector<double> s;
    s.reserve(nx_nonzero);
    for (int i = 0; i < nx; ++i)
      if (x[i] != 0)
	s.push_back(x[i]);
    std::sort(s.begin(), s.end());
    for (int i = 0; i <= nbins; ++i) {
      int j = i*(nx_nonzero-1);
      j /= nbins;
      std::cout << float(i)/float(nbins) << '\t' << s[j] << std::endl;
    }
  }
}  // print_histogram()

// f_df() evaluates the statistics of the corpus, and prints the f-score
//  if required
//
double f_df(loss_type ltype, corpus_type* corpus, const double x[], double df_dx[]) {
  Float sum_g = 0, sum_p = 0, sum_w = 0, L = 0;

  switch (ltype) {
  case log_loss:
    L = corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case em_log_loss:
    L = emll_corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case em_log_loss_noomp:
    L = emll_corpus_stats_noomp(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case pairwise_log_loss:
    L = pwlog_corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case exp_loss:
    L = exp_corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case log_exp_loss:
    L = log_exp_corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    break;
  case expected_fscore_loss:
    L = 1 - fscore_corpus_stats(corpus, &x[0], &df_dx[0], &sum_g, &sum_p, &sum_w);
    for (size_type j = 0; j < corpus->nfeatures; ++j)
      df_dx[j] = -df_dx[j];
    break;
  default:
    L = 0;
    std::cerr << "## Error: unrecognized loss_type loss = " << int(ltype) 
	      << std::endl;
  }  
  
  if (debug_level >= 1000)
    std::cerr << "f score = " << 2*sum_w/(sum_g+sum_p) << ", " << std::flush;
  
  assert(finite(L));
  return L;
}

// LossFn() is the loss function
//
class LossFn : public DifferentiableFunction {
public:

  const loss_type ltype;
  corpus_type* corpus;
  const size_ts& f_c;	//!< feature -> cross-validation class
  const doubles& cs;	//!< cross-validation class -> regularizer factor
  double p, s;
  int it;
  double L, R, Q;

  LossFn(loss_type ltype, corpus_type* corpus, const size_ts& f_c, 
	 const doubles& cs, double p, double s) 
    : ltype(ltype), corpus(corpus), f_c(f_c), cs(cs), p(p), s(s), it(0) 
  { 
    assert(f_c.size() == corpus->nfeatures);
    for (size_type f = 0; f < f_c.size(); ++f)
      assert(f_c[f] < cs.size());
  }

  virtual double Eval(const DblVec& x, DblVec& df_dx) {
    size_t nx = x.size();

    assert(size_t(nx) == corpus->nfeatures);
    assert(f_c.size() == corpus->nfeatures);

    it++;
    if (debug_level >= 1000)      
      std::cerr << "it = " << it << ", " << std::flush;

    L = f_df(ltype, corpus, &x[0], &df_dx[0]);

    if (s != 1) {
      L *= s;

      for (size_t i = 0; i < nx; ++i) {
	assert(finite(df_dx[i]));
	df_dx[i] *= s;
      }
    }

    R = 0;
    if (p != 1) {
      for (size_t i = 0; i < nx; ++i) 
	R +=  cs[f_c[i]] * pow(fabs(x[i]), p);
      R *= s;
      double sp = s * p;
      for (size_t i = 0; i < nx; ++i) 
	df_dx[i] += sp * cs[f_c[i]] * pow(fabs(x[i]), p-1) 
	  * (x[i] >= 0 ? (x[i] == 0 ? 0 : 1) : -1);
    }

    Q = L + R;
    
    if (debug_level >= 1000)
      std::cerr << "Q = " << Q << " = L = " << L << " + R = " << R << std::endl;

    assert(finite(Q));
    
    if (debug_level >= 10000) {
      std::cerr << "Histogram of derivatives:" << std::endl;
      print_histogram(nx, &df_dx[0]);
      std::cerr << "--------------------------" << std::endl;
    }

    for (size_t i = 0; i < nx; ++i)
      assert(finite(df_dx[i]));
    return Q;
  }   // LossFn::operator()

};  // LossFn{}

// The Estimator1 does one round of estimation
//
struct Estimator1 {
  typedef std::vector<size_t> size_ts;
  typedef std::vector<double> doubles;
  
  corpus_type* train;	//!< training data
  size_type nx;		//!< number of features
  corpus_type* eval;	//!< evaluation data
  corpus_type* eval2;	//!< 2nd evaluation data
  loss_type ltype;	//!< type of loss function
  double c0;		//!< default regularizer factor
  double c00;           //!< multiply default regularizer factor for first feature class
  double p;		//!< regularizer power
  double r;		//!< random initialization
  double s;		//!< scale factor
  double tol;           //!< stopping tolerance
  bool opt_fscore;	//!< optimize f-score or - log likelihood

  doubles x;		//!< feature -> weight
  size_ts f_c;		//!< feature -> cross-validation class
  doubles lcs;		//!< cross-validation class -> log factor
  size_type nc;		//!< number of cross-validation classes

  size_type nits;	//!< number of iterations of last round
  size_type sum_nits;	//!< total number of iterations
  size_type nrounds;	//!< number of cross-validation rounds so far
  double best_score;    //!< best score seen so far
  std::string weightsfile; //!< name of weights file

  typedef std::map<std::string,size_t> S_C;
  S_C identifier_regclass; //!< map from feature class identifiers to regularization class
  typedef std::vector<std::string> Ss;
  Ss regclass_identifiers; //!< vector of class identifiers

  Estimator1(loss_type ltype, double c0, double c00, double p, double r, double s,
	     double tol=1e-5, bool opt_fscore = true, std::string weightsfile = "") 
    : train(NULL), nx(0), eval(NULL), eval2(NULL),
      ltype(ltype), c0(c0), c00(c00), p(p), r(r), s(s), tol(tol), 
      opt_fscore(opt_fscore), lcs(1, log(c0)), nc(1), 
      nits(0), sum_nits(0), nrounds(0), best_score(0),
      weightsfile(weightsfile)
  { }  // Estimator1::Estimator1()

  //! set_data() sets the training and evaluation data
  //
  void set_data(corpus_type* t, corpus_type* e, corpus_type* e2) {
    train = t;
    eval = e;
    eval2 = e2;
    nx = train->nfeatures;
    x.resize(nx);
    assert(f_c.size() <= nx);
    assert(eval == NULL || eval->nfeatures <= train->nfeatures);
    assert(eval2 == NULL || eval2->nfeatures <= train->nfeatures);
  } // Estimator1::set_data()

  // operator() actually runs one round of estimation
  //
  double operator() (const doubles& lccs) {
    assert(lccs.size() == nc);
    doubles ccs(nc);
    double L, R, Q;

    for (size_type i = 0; i < nc; ++i)
      ccs[i] = exp(lccs[i]);

    assert(x.size() == nx);
    nits = 0;
    nrounds++;

    if (debug_level >= 10) {
      if (nrounds == 1) 
	std::cerr << "# round	nfeval	L	R	Q	neglogP	f-score	css" << std::endl;
      std::cerr << nrounds << std::flush;
    }
 
    LossFn fn(ltype, train, f_c, ccs, p, s);

    DblVec x0(nx);

    if (r != 0) 
      for (size_type i = 0; i < nx; ++i)
	x0[i] = r*double(random()-RAND_MAX/2)/double(RAND_MAX/2);
    
    OWLQN owlqn(true);
    if (p == 1) {
      assert(ccs.size() == 1);
      owlqn.Minimize(fn, x0, x, ccs[0], tol);
    }
    else
      owlqn.Minimize(fn, x0, x, 0, tol);      
      
    nits = fn.it;
    L = fn.L;
    R = fn.R;
    Q = fn.Q;

    // Clean up, collect stats

    sum_nits += nits;

    if (debug_level >= 10)
      std::cerr << '\t' << nits << '\t' << L << '\t' << R << '\t' << Q;

    double score = evaluate(opt_fscore, true);

    if (debug_level >= 10)
      std::cerr << '\t' << ccs << std::endl;

    return score;

  }  // Estimator1::operator()

  // evaluate() evaluates the current model on the eval data, prints
  // out debugging information if appropriate, and returns either
  // the - log likelihood or 1 - f-score.
  //
  double evaluate(bool opt_fscore = false, bool internal = false) {

    std::vector<double> df_dx(nx);
    Float sum_g = 0, sum_p = 0, sum_w = 0;
    Float neglogP = corpus_stats(eval, &x[0], &df_dx[0], 
				 &sum_g, &sum_p, &sum_w);
    Float fscore = 2*sum_w/(sum_g+sum_p);
    
    if (internal) {  // internal evaluation, use a short print-out
      if (debug_level >= 10) {
	std::cerr << '\t' << neglogP << '\t' << fscore; 
	if (eval2 != NULL) {
	  Float sum_g2 = 0, sum_p2 = 0, sum_w2 = 0;
	  Float neglogP2 = corpus_stats(eval2, &x[0], &df_dx[0], 
					&sum_g2, &sum_p2, &sum_w2);
	  Float fscore2 = 2*sum_w2/(sum_g2+sum_p2);
	  std::cerr << '\t' << neglogP2 << '\t' << fscore2;
	}
      }
    }
    else { // final evaluation, print out more info
    
      int nzeros = 0;
      for (size_type i = 0; i < nx; ++i) 
	if (x[i] == 0)
	  ++nzeros;
  
      std::cerr << "# Regularizer power p = " << p << std::endl;
      std::cerr << "# " << nx-nzeros << " non-zero feature weights of " 
		<< nx << " features." << std::endl;
      std::cerr << "# Eval neglogP = " << neglogP 
		<< ", neglogP/nsentences = " << neglogP/eval->nsentences
		<< std::endl;
      std::cerr << "# Eval precision = " << sum_w/sum_p 
		<< ", recall = " << sum_w/sum_g
		<< ", f-score = " << 2*sum_w/(sum_g+sum_p)
		<< std::endl;
      if (eval2 != NULL) {
	Float sum_g = 0, sum_p = 0, sum_w = 0;
	Float neglogP = corpus_stats(eval2, &x[0], &df_dx[0], 
				     &sum_g, &sum_p, &sum_w);
	std::cerr << "# Eval2 neglogP = " << neglogP 
		  << ", neglogP/nsentences = " << neglogP/train->nsentences
		  << std::endl;
	std::cerr << "# Eval2 precision = " << sum_w/sum_p 
		  << ", recall = " << sum_w/sum_g
		  << ", f-score = " << 2*sum_w/(sum_g+sum_p)
		  << std::endl;
      }
      {
	Float sum_g = 0, sum_p = 0, sum_w = 0;
	Float neglogP = corpus_stats(train, &x[0], &df_dx[0], 
				     &sum_g, &sum_p, &sum_w);
	std::cerr << "# Train neglogP = " << neglogP 
		  << ", neglogP/nsentences = " << neglogP/train->nsentences
		  << std::endl;
	std::cerr << "# Train precision = " << sum_w/sum_p 
		  << ", recall = " << sum_w/sum_g
		  << ", f-score = " << 2*sum_w/(sum_g+sum_p)
		  << std::endl;
      }

      std::cerr << "# regclass_identifiers = " << regclass_identifiers << std::endl;
      std::cerr << "# lcs = " << lcs << std::endl;
      {
	doubles cs(nc);
	for (size_type i = 0; i < nc; ++i)
	  cs[i] = exp(lcs[i]);
	std::cerr << "# cs = " << cs << std::endl;
      }
      if (debug_level >= 100) {
	std::cerr << "# Cumulative distribution of feature weights:" << std::endl;
	print_histogram(nx, &x[0]);
      }
    }

    double score = (opt_fscore ? 1 - fscore : neglogP);

    if (nrounds == 1 || score < best_score) {
      best_score = score;

      // Write out weights file
    
      if (!weightsfile.empty()) {
	FILE* out = fopen(weightsfile.c_str(), "w");
	// fprintf(out, "%d@", nx-nzeros);
	for (size_type i = 0; i < x.size(); ++i) 
	  if (x[i] != 0) {
	    fprintf(out, "%d", i);
	    if (x[i] != 1)
	      fprintf(out, "=%g", x[i]);
	    fprintf(out, "\n");
	  }
	fclose(out);
      }      
    }
    return score;
  } // Estimator1::evaluate()

  //! fc_bin() maps a feature count to its corresponding bin
  //
  static int fc_bin(double feature_count_base, int feature_count) {
    if (feature_count <= 4)
      return feature_count;
    else
      return lrint(4.0 + pow(feature_count_base, 
			     lrint(log(feature_count-4)/log(feature_count_base))));
  }  // Estimator1::fc_bin()

  // read_featureclasses() reads the feature classes from a feature file
  //
  void read_featureclasses(const char* filename, 
			   int nseparators = 1,
			   const char* separators = ":") {
    
    const char* filesuffix = strrchr(filename, '.');
    bool popen_flag = false;
    FILE *in;
    if (strcasecmp(filesuffix, ".bz2") == 0) {
      std::string command("bzcat ");
      command += filename;
      in = popen(command.c_str(), "r");
      if (in == NULL) {
	perror("## Error in cvlm-owlqn: ");
	std::cerr << "## popen(\"" << command << "\", \"r\") failed, usage = " << resource_usage() << std::endl;
      }
      popen_flag = true;
    }
    else if (strcasecmp(filesuffix, ".gz") == 0) {
      std::string command("zcat ");
      command += filename;
      errno = 0;
      in = popen(command.c_str(), "r");
      if (in == NULL) {
	perror("## Error in cvlm-owlqn: ");
	std::cerr << "## popen(\"" << command << "\", \"r\") failed, usage = " << resource_usage() << std::endl;
      }
      popen_flag = true;
    }
    else
      in = fopen(filename, "r");
    if (in == NULL) {
      std::cerr << "## Couldn't open evalfile " << filename
		<< ", errno = " << errno << "\n" 
		<< usage << std::endl;
      exit(EXIT_FAILURE);
    }

    size_type featno;

    // read feature number 

    while (fscanf(in, " %u ", &featno) == 1) {
      int c = ':';
      
      // read the prefix of the feature class identifier
      
      std::string identifier;
      int iseparators = 0;
      if (nseparators >= 0)
	while ((c = getc(in)) != EOF && !isspace(c)) {
	  if (index(separators, c) != NULL)
	    if (++iseparators > nseparators)
	      break;
	  identifier.push_back(c);
	}
      
      // skip the rest of the line

      while ((c = getc(in)) != EOF && c != '\n')
	;

      // insert the prefix into the prefix -> regularization class map
      
      S_C::iterator it 
	= identifier_regclass.insert(S_C::value_type(identifier, 
						     identifier_regclass.size())).first;
      
      size_type cl = it->second;    // regularization class

      f_c.resize(featno+1);
      f_c[featno] = cl;          // set feature's regularization class
    }
      
    nc = identifier_regclass.size();   // set nc
    lcs.resize(nc, log(c0));           // set each regularizer class' factor to c0
    lcs[0] += log(c00);                // increment first regularizer class' factor by c00

    // construct regclass_identifiers
    
    regclass_identifiers.resize(nc);
    cforeach (S_C, it, identifier_regclass) {
      assert(it->second < regclass_identifiers.size());
      regclass_identifiers[it->second] = it->first;
    }

    if (debug_level >= 0) 
      std::cerr << "# Regularization classes: " << regclass_identifiers << std::endl;

    if (popen_flag)
      pclose(in);
    else
      fclose(in);
  }  // Estimator1::read_featureclasses()
    
  void estimate(const int maxruns=10)
  {
    powell::control cntrl(1e-4, 1e-2, maxruns, maxruns);
    powell::minimize(lcs, *this, log(2), cntrl);

    if (debug_level > 0) {
      std::cerr << "# Regularizer class weights = (";
      for (size_type i = 0; i < lcs.size(); ++i) {
	if (i > 0)
	  std::cerr << ' ';
	std::cerr << exp(lcs[i]);
      }
      std::cerr << ')' << std::endl;
    }

  }  // Estimator1::estimate()

};  // Estimator1{}

//! exit_failure() causes the program to halt immediately
//
inline std::ostream& exit_failure(std::ostream& os) {
  os << std::endl;
  exit(EXIT_FAILURE);
  return os;
}  // util::exit_failure

int main(int argc, char** argv) 
{
  std::ios::sync_with_stdio(false);

  loss_type ltype = log_loss;
  double c0 = 2.0;
  double c00 = 1.0;
  double p = 2.0;
  double r = 0.0;
  double s = 1.0;
  double tol = 1e-5;
  double Pyx_factor = 0.0;
  bool Px_propto_g = false;
  int maxruns = 10;
  int nseparators = 1;
  std::string  feat_file, weights_file, eval_file, eval2_file;
  int opt;
  while ((opt = getopt(argc, argv, "hd:c:C:p:r:s:t:l:F:Gm:n:f:o:e:x:")) != -1) 
    switch (opt) {
    case 'h':
      std::cerr << usage << exit_failure;
      break;
    case 'd':
      debug_level = atoi(optarg);
      break;
    case 'c':
      c0 = atof(optarg);
      break;
    case 'C':
      c00 = atof(optarg);
      break;
    case 'p':
      p = atof(optarg);
      break;
    case 'r':
      r = atof(optarg);
      break;
    case 's':
      s = atof(optarg);
      break;
    case 't':
      tol = atof(optarg);
      break;
    case 'l':
      ltype = loss_type(atoi(optarg));
      break;
    case 'F':
      Pyx_factor = atof(optarg);
      break;
    case 'G':
      Px_propto_g = true;
      break;
    case 'm':
      maxruns = atoi(optarg);
      break;
    case 'n':
      nseparators = atoi(optarg);
      break;
    case 'f':
      feat_file = optarg;
      break;
    case 'o':
      weights_file = optarg;
      break;
    case 'e':
      eval_file = optarg;
      break;
    case 'x':
      eval2_file = optarg;
      break;
    }

  if (debug_level >= 10)
    std::cerr << "#  ltype -l = " << ltype
	      << " (" << loss_type_name[ltype] << ")"
	      << " regularization -c = " << c0
	      << ", c00 -C = " << c00
	      << ", power -p = " << p 
	      << ", scale -s = " << s
	      << ", tol -t = " << tol
	      << ", random init -r = " << r 
	      << ", Pyx_factor -F = " << Pyx_factor
	      << ", Px_propto_g -G = " << Px_propto_g
	      << ", maxruns -m = " << maxruns
	      << ", nseparators -n = " << nseparators
	      << ", feat_file -f = " << feat_file
	      << ", weights_file -o = " << weights_file
	      << ", eval_file -e = " << eval_file
	      << ", eval2_file -x = " << eval2_file
	      << std::endl;

  // I discovered a couple of years after I wrote this program that popen
  // uses fork, which doubles your virtual memory for a short instant!

  Estimator1 e(ltype, c0, c00, p, r, s, tol, true, weights_file);

  if (!feat_file.empty())
    e.read_featureclasses(feat_file.c_str(), nseparators, ":");  

  // Read in eval data first, as that way we may squeeze everything into 4GB
    
  corpusflags_type corpusflags = { Pyx_factor, Px_propto_g };

  corpus_type* evaldata = NULL;
  if (!eval_file.empty()) {
    evaldata = read_corpus_file(&corpusflags, eval_file.c_str());
    if (debug_level >= 10)
      std::cerr << "# read eval_file = " << eval_file 
		<< ", nsentences = " << evaldata->nsentences
		<< std::endl;
  }

  corpus_type* evaldata2 = NULL;
  if (!eval2_file.empty()) {
    evaldata2 = read_corpus_file(&corpusflags, eval2_file.c_str());
    if (debug_level >= 10)
      std::cerr << "# read eval2_file = " << eval2_file
		<< ", nsentences = " << evaldata2->nsentences
		<< std::endl;
  }

  corpus_type* traindata = read_corpus(&corpusflags, stdin);
  int nx = traindata->nfeatures;

  if (errno != 0) {
    perror("## cvlm-owlqn, after reading main corpus, nonzero errno  ");
    errno = 0;
  }

  std::cerr << "# " << nx << " features in training data, " << resource_usage() << std::endl;

  if (evaldata == NULL)
    evaldata = traindata;

  e.set_data(traindata, evaldata, evaldata2);
  e.estimate(maxruns);

}  // main()
