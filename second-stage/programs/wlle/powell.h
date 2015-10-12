//! powell.h
//!
//! Powell's algorithm for minimizing an n-dimensional function
//! without derivatives, based on Numerical Recipes in C++ code.
//!
//! Mark Johnson, 29th September 2002, last modified 14th August 2008
//!
//! The main routine is powell::minimize().  Its calling sequence
//! is:
//!
//! Float minimize(              // => value of f at minimum
//!                p,            // <= starting x value, => minimizing x value
//!                f,            // <= function object being minimized
//!                initial_step, // <= initial step length 
//!                cntrl)        // <= control{} structure
//!
//!
//! There is also a specialized version for 1-d minimization
//!
//! Float minimize1d(              // => minimizing x value
//!                  p,            // <= starting x value
//!                  f,            // <= function object being minimized
//!                  initial_step, // <= initial step length 
//!                  cntrl)        // <= control{} structure

#ifndef POWELL_H
#define POWELL_H

#include <cassert>
#include <cmath> 
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace powell {

  typedef double Float;
  typedef std::vector<Float> Floats;
  typedef std::vector<Floats> Array;

  template <typename T>
  inline void error_abort(const T& t) {
    std::cerr << "Error in powell.h: " << t << std::endl;
    abort();
  }

  //! The control class gives the user a way to monitor and
  //! control the minimization process.  You can specialize its
  //! behaviour by deriving your own monitor from this class
  //! and changing its behaviour.
  //
  class control {

  private:
    Float tol;	       	     //!< Stopping function tolerance
    Float linmin_tol;  	     //!< Line minimization stopping tolerance
    int max_nfeval;	     //!< Max number of function evaluations
    int linmin_max_nfeval_;  //!< Max number of function evaluations in brent()
    int debug;               //!< controls tracing, etc

  public:
    Float linmin_rel_tol() const { return linmin_tol; }
    Float linmin_abs_tol() const { return linmin_tol; }
    int linmin_max_nfeval() const { return linmin_max_nfeval_; }
    Float linmin_xinit() const { return 1.0; }
    size_t cache_size() const { return 20; }        //!< function cache size

    // control() sets the defaults for the program
    //
    control(Float tol = 1e-7,            //!< stopping tolerance
	    Float linmin_tol = 1e-7,	 //!< line minimization tolerance
	    int max_nfeval = 0,          //!< max number of fn evalns, 
	                                 //!<  if neg, halt w/ error
	    int linmin_max_nfeval_ = 0,  //!< max number of fn evalns 
	                                 //!<  per line minimization
	    size_t debug = 0) 
      : tol(tol),
	linmin_tol(linmin_tol),
	max_nfeval(max_nfeval),
	linmin_max_nfeval_(linmin_max_nfeval_),
	debug(debug)
    { }  // control()

    //! operator() is called at each powell iteration
    //! If operator() returns true, the powell routine halts.
    //! Specialize its behaviour if you want custom tracing
    //
    bool operator() (const Floats& ps,
		     const Float fx, const Float fx_last,
		     const int iteration,
		     const int nfeval) const 
    {
      const Float TINY = 1.0e-25;
      if (debug >= 1000) {  // print tracing information
	if (iteration == 1)
	  std::cout << "#" << std::setw(7) << "iter" << std::setw(10) << "f(x)" 
		    << std::setw(10) << "nfeval" << std::endl;
	std::cout << std::setw(8) << iteration << ' ' << std::setw(9) << fx
		  << ' ' << std::setw(9) << nfeval << std::endl;

	if (debug >= 100000) {
	  std::cout << "  x = (" << ps[0];
	  for (size_t j = 1; j < ps.size(); ++j)
	    std::cerr << ' ' << ps[j];
	  std::cerr << ')' << std::endl;
	}
      }

      if (fx_last < fx)
	std::cerr << " *** Error in powell::powell() iteration " << iteration 
		  << ", fx = " << fx << " > fx_last = " << fx_last << std::endl;
      
      // This is the termination criterion used by Numerical Recipies
      //
      if (2.0*fabs(fx-fx_last) <= tol*(fabs(fx_last)+fabs(fx))+TINY)
	return true;

      if (max_nfeval != 0 && nfeval >= abs(max_nfeval)) {
	if (max_nfeval > 0) {
	  if (debug >= 100)
	    std::cerr << "*** powell::powell() returning at iteration " << iteration
		      << " in with error " << fx_last-fx 
		      << std::endl;
	  return true;
	}
	else
	  error_abort("Too many iterations in powell::powell");
      }

      return false;
    }  // operator()
  };  // control{}


  template <typename T>
  inline T SQR(T x) { return x*x; }

  inline void shft3(Float& a, Float& b, Float& c, const Float& d)
  {
    a=b;
    b=c;
    c=d;
  }

  inline Float sign(Float a, Float b) {
    return ((b) > 0.0 ? fabs(a) : -fabs(a));
  }

  inline void shift(Float& a, Float& b, Float& c, Float d) { 
    a = b; b = c; c = d; 
  }

  //! mnbrak()
  //!
  //! Given a function f, and given distinct initial points ax and bx,
  //! this routine searches in the downhill direction (defined by the
  //! function as evaluated at the initial points) and returns new points
  //! ax, bx, cx that bracket a minimum of the function. Also returned
  //! are the function values at the three points, fa, fb, and fc.
  //!
  //! The arguments to f() are:
  //!  x     <= point at which to evaluate f
  //!  df    => derivative of f at x
  //!  f()   => value of f at x
  //!
  //! Here GOLD is the default ratio by which successive intervals are
  //! magnified; GLIMIT is the maximum magnification allowed for a
  //! parabolic-fit step.
  //
  template <typename F>
  void mnbrak(Float& ax, Float& bx, Float& cx, 
	      Float& fa, Float& fb, Float& fc, 
	      F& f) 
  { 
    const float GOLD = 1.618034;
    const float GLIMIT = 100.0;
    const double TINY = 1.0e-20;

    Float ulim, u, r, q, fu;
    fa = f(ax); 
    fb = f(bx); 
    if (fb > fa) { 
      // Switch roles of a and b so that we can go downhill in the
      // direction from a to b. 
      std::swap(ax, bx);
      std::swap(fa, fb);
    } 
    cx = bx+GOLD*(bx-ax);       // First guess for c.
    fc = f(cx); 
    while (fb > fc) {           // Keep returning here until we bracket.
      r = (bx-ax)*(fb- fc); 
      // Compute u by parabolic extrapolation from a; b; c. 
      // TINY is used to prevent any possible division by zero. 
      q = (bx-cx)*(fb-fa); 
      u = bx-((bx-cx)*q-(bx-ax)*r)/(2.0*sign(std::max(fabs(q-r),TINY),q-r)); 
      ulim = bx+GLIMIT*(cx-bx); 
      // We won't go farther than this. Test various possibilities: 
      if ((bx-u)*(u-cx) > 0.0) { 
	// Parabolic u is between b and c: try it.
	fu = f(u); 
	if (fu < fc) { 
	  // Got a minimum between b and c. 
	  ax = bx; 
	  bx = u; 
	  fa = fb; 
	  fb = fu; 
	  return; 
	} 
	else if (fu > fb) { 
	  // Got a minimum between between a and u. */
	  cx = u; 
	  fc = fu; 
	  return; 
	} 
	u = cx+GOLD*(cx-bx); 
	// Parabolic fit was no use. Use default magnification. 
	fu = f(u); 
      } 
      else if ((cx-u)*(u-ulim) > 0.0) { 
	// Parabolic fit is between c and its allowed limit. 
	fu = f(u); 
	if (fu < fc) { 
	  bx = cx; cx = u; u = cx+GOLD*(cx-bx);
	  fb = fc; fc = fu; fu = f(u);
	} 
      } 
      else if ((u-ulim)*(ulim-cx) >= 0.0) { 
	// Limit parabolic u to maximum allowed value. 
	u = ulim; 
	fu = f(u); 
      } 
      else { 
	// Reject parabolic u, use default magnification. 
	u = cx+GOLD*(cx-bx); 
	fu = f(u); 
      } 
      // Eliminate oldest point and continue. 
      ax = bx; bx = cx; cx = u;
      fa = fb; fb = fc; fc = fu;
    } 
  } // mnbrak()

  //! brent() was modified so that fxmin is the smallest f() seen so far,
  //! and xmin is the value of x at which fxmin was encountered.
  //
  template <typename F, typename C>
  Float brent(const Float ax, const Float bx, const Float cx, F& f,
	      C& control, Float &xmin)
  {
    const Float CGOLD = 0.3819660;     // Golden ratio
    Float a, b, d=0.0, etemp, fu, fv, fw, fx;
    Float p, q, r, u, v, w, x, xm;
    Float e=0.0;
    Float fxmin;

    a = (ax < cx ? ax : cx);
    b = (ax > cx ? ax : cx);
    fw = fv = fx = f(x = w = v = bx);
    xmin = x;
    fxmin = fx;
    size_t max_nfeval = f.nfeval() + control.linmin_max_nfeval();
    for (size_t iter = 0; iter < 200; iter++) {
      xm = 0.5*(a+b);
      Float tol1 = control.linmin_rel_tol() * fabs(x) + control.linmin_abs_tol();
      Float tol2 = 2.0*tol1;
      if (fabs(x-xm) <= (tol2-0.5*(b-a))) {
	// xmin = x;
	// return fx;
	return fxmin;
      }
      if (control.linmin_max_nfeval() != 0 && f.nfeval() >= max_nfeval) {
	// xmin = x;
	// return fx;
	return fxmin;
      }
      if (fabs(e) > tol1) {
	r = (x-w)*(fx-fv);
	q = (x-v)*(fx-fw);
	p = (x-v)*q-(x-w)*r;
	q = 2.0*(q-r);
	if (q > 0.0) 
	  p = -p;
	q = fabs(q);
	etemp=e;
	e=d;
	if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
	  d=CGOLD*(e=(x >= xm ? a-x : b-x));
	else {
	  d=p/q;
	  u=x+d;
	  if (u-a < tol2 || b-u < tol2)
	    d=sign(tol1,xm-x);
	}
      } else {
	d=CGOLD*(e=(x >= xm ? a-x : b-x));
      }
      u=(fabs(d) >= tol1 ? x+d : x+sign(tol1,d));
      fu=f(u);
      if (fu < fxmin) {
	fxmin = fu;
	xmin = u;
      }
      if (fu <= fx) {
	if (u >= x) a=x; else b=x;
	shift(v, w, x, u);
	shift(fv, fw, fx, fu);
      } else {
	if (u < x) a=u; else b=u;
	if (fu <= fw || w == x) {
	  v=w;
	  w=u;
	  fv=fw;
	  fw=fu;
	} else if (fu <= fv || v == x || v == w) {
	  v=u;
	  fv=fu;
	}
      }
    }
    // xmin = x;
    // return fx;
    return fxmin;
  }  // brent()

  template <typename F>
  struct f1cache {
    const size_t cache_size; //!< number of function evaluations to cache
    F& f;                    //!< original function 
    Floats xs;               //!< cached x values
    Floats fs;               //!< cached f vector
    size_t last;             //!< last position inserted
    size_t nfeval_;          //!< number of function evaluations

    f1cache(F& f, size_t cache_size=2) 
      : cache_size(cache_size), f(f), last(cache_size-1), nfeval_(0) { }

    Float operator() (Float x) {
      for (size_t i = 0; i < cache_size; ++i) {
	if (i >= xs.size()) {
	  Float fx = f(x);
	  xs.push_back(x);
	  fs.push_back(fx);
	  ++nfeval_;
	  return fx;
	}
	if (x == xs[i]) 
	  return fs[i];
      }
      Float fx = f(x);
      if (++last >= xs.size())
	last = 0;
      xs[last] = x;
      fs[last] = fx;
      ++nfeval_;
      return fx;
    } // f1cache::operator()

    size_t nfeval() const { return nfeval_; }

  };  // f1cache{}
    
  //! minimize1d() is the 1d-minimization routine.  It returns the minimizing x value.
  //
  template <typename F>
  Float minimize1d(Float p, F& f, Float initial_step = 1.0, Float tol=1e-7, int max_nfeval=100)
  {
    control c(tol, tol, max_nfeval, max_nfeval);
    f1cache<F> func(f, c.cache_size());
    Float a = p;
    Float x = a + initial_step;
    Float b, fa, fx, fb, xmin;

    mnbrak(a, x, b, fa, fx, fb, func);
    brent(a, x, b, func, c, xmin);  // brent() returns min_f, if you want it
    return xmin;
  }  // minimize1d()

  //! linmin_f{} maps the n-dimensional function into a 1-d function 
  //!  in direction p
  //
  template <typename F>
  struct linmin_f {      /* structure to communicate with linmin */
    F& f;
    Floats& p;
    Floats& xi;
    Floats xt;
    
    linmin_f(F& f, Floats& p, Floats& xi) 
      : f(f), p(p), xi(xi), xt(p.size()) { }
  
    Float operator() (Float x) {
      const size_t n = p.size();
      for (size_t j = 0; j < n; ++j)
	xt[j] = p[j] + x*xi[j];
      Float fx = f(xt);
      return fx;
    }

    size_t nfeval() const { return f.nfeval; }

  };  // linmin_f{}
  
  template <typename F, typename C>
  Float linmin(Floats &p, Floats &xi, F& f, C& control)
  {
    assert(p.size() == xi.size());
    linmin_f<F> f1dim(f, p, xi);

    Float a = 0.0;
    Float x = control.linmin_xinit();
    Float b, fa, fx, fb, xmin;

    mnbrak(a, x, b, fa, fx, fb, f1dim);
    Float fret = brent(a, x, b, f1dim, control, xmin);
    for (size_t j = 0; j < p.size(); j++) {
      xi[j] *= xmin;
      p[j] += xi[j];
    }
    return fret;
  }  // linmin()

  template <typename F>
  struct fcache {
    typedef std::vector<Floats> Fss;
    const size_t cache_size; //!< number of function evaluations to cache
    F& f;                    //!< original function 
    Fss xs;                  //!< cached x vectors
    Floats fs;               //!< cached f vector
    size_t last;             //!< last position inserted
    size_t nfeval;           //!< number of function evaluations

    fcache(F& f, size_t cache_size=2) 
      : cache_size(cache_size), f(f), last(cache_size-1), nfeval(0) { }

    Float operator() (const Floats& x) {
      for (size_t i = 0; i < cache_size; ++i) {
	if (i >= xs.size()) {
	  Float fx = f(x);
	  xs.push_back(x);
	  fs.push_back(fx);
	  ++nfeval;
	  return fx;
	}
	if (x == xs[i]) 
	  return fs[i];
      }
      Float fx = f(x);
      if (++last >= xs.size())
	last = 0;
      xs[last] = x;
      fs[last] = fx;
      ++nfeval;
      return fx;
    } // fcache::operator()

  };  // fcache{}

  // Minimization of a function func of n variables. Input consists of an initial 
  // starting point p[1..n]; an initial matrix xi[1..n][1..n], whose columns contain 
  // the initial set of directions (usually the n unit vectors); and ftol, the 
  // fractional tolerance in the function value such that failure to decrease by 
  // more than this amount on one iteration signals doneness. On output, p is set 
  // to the best point found, xi is the then-current direction set, fret is the 
  // returned function value at p, and iter is the number of iterations taken. 
  // The routine linmin is used.
  //
  template <typename F, typename Control>
  Float minimize_n(Floats& p, F& f, Float initial_step, Control& cntrl)
  {
    fcache<F> func(f, cntrl.cache_size());
    size_t n = p.size();
    
    Array xi(n);                 	// array of conjugate directions
    for (size_t i = 0; i < n; ++i) {  	// initialize to unit vectors
      xi[i].resize(n);
      xi[i][i] = initial_step;
    }

    Float del,fp,fptt,t;

    Floats pt(p);			// Save the initial point
    Floats ptt(n), xit(n);

    Float fret = func(p);

    for (size_t iter = 1; ; ++iter) {
      fp = fret;
      size_t ibig=0;
      del = 0.0;				// Will be the biggest function decrease.
      for (size_t i = 0; i < n; i++) {		// In each iteration, loop over all directions.
	for (size_t j = 0; j < n; j++)		// Copy the direction,
	  xit[j] = xi[j][i];
	fptt = fret;
	fret = linmin(p, xit, func, cntrl); 	//  minimize along it,
	if (fptt-fret > del) {			//  and record it if it is the largest decrease
	  del = fptt-fret;			//   so far.
	  ibig = i+1;
	}
      }

      if (cntrl(p, fret, fp, iter, func.nfeval))
	return fret;				// Normal termination

      for (size_t j = 0; j < n; j++) {	// Construct the extrapolated point and the
	ptt[j] = 2.0*p[j]-pt[j];	//  average direction moved. Save the
	xit[j] = p[j]-pt[j];		//  old starting point.
	pt[j] = p[j];
      }

      fptt = func(ptt);			// Function value at extrapolated point.

      if (fptt < fp) {
	t=2.0*(fp-2.0*fret+fptt)*SQR(fp-fret-del)-del*SQR(fp-fptt);
	if (t < 0.0) {
	  fret = linmin(p, xit, func, cntrl);
	  for (size_t j = 0; j < n; j++) {
	    xi[j][ibig-1] = xi[j][n-1];
	    xi[j][n-1] = xit[j];
	  }
	}
      }
    }
  }  // minimize_n()

  //! minimize() calls minimize_n() if p.size() > 1, otherwise it does a one-dimensional
  //! minimization.
  //
  template <typename F, typename Control>
  Float minimize(Floats& p, F& f, Float initial_step, Control& control) {
    assert(p.size() > 0);
    if (p.size() > 1)
      return minimize_n(p, f, initial_step, control);
    else {    // 1-d minimization
      fcache<F> func(f, control.cache_size());
      Floats xi(1, 1);
      return linmin(p, xi, func, control);
    }
  } // minimize()
    
  //! minimize() is a wrapper that calls the main minimization routine with a default control.
  //
  template <typename F>
  Float minimize(Floats& p, F& f, Float initial_step = 1.0)
  {
    control c;
    return minimize(p, f, initial_step, c);
  }  // minimize()

} //  namespace powell

#endif // POWELL_H

/*

// Code for testing the 1-d minimization routines

#include <cmath>
#include "powell.h"

struct f_type {
  double operator() (double x) const {
    double fx = sin(x);
    std::cout << "x = " << x << ", fx = " << fx << std::endl;
    return fx;
  }
};

int main(int argc, char** argv) {
  f_type f;
  double xmin = powell::minimize1d(0, f, 0.1, 1e-5, 10);
  std::cout << "xmin = " << xmin << std::endl;
}
*/
