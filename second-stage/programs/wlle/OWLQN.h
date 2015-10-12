#pragma once

#include <cstdlib>
#include <vector>
#include <deque>
#include <iostream>

typedef std::vector<double> DblVec;

struct DifferentiableFunction {
	virtual double Eval(const DblVec& input, DblVec& gradient) = 0;
};

#include "TerminationCriterion.h"

class OWLQN {
	bool quiet;
	bool responsibleForTermCrit;

public:
	TerminationCriterion *termCrit;

	OWLQN(bool quiet = false) : quiet(quiet) {
		termCrit = new RelativeMeanImprovementCriterion(5);
		responsibleForTermCrit = true;
	}

	OWLQN(TerminationCriterion *termCrit, bool quiet = false) : quiet(quiet), termCrit(termCrit) { 
		responsibleForTermCrit = false;
	}

	~OWLQN() {
		if (termCrit && responsibleForTermCrit) delete termCrit;
	}

	void Minimize(DifferentiableFunction& function, const DblVec& initial, DblVec& minimum, double l1weight = 1.0, double tol = 1e-4, int m = 10) const;
	void SetQuiet(bool q) { quiet = q; }

};

class OptimizerState {
	friend class OWLQN;

	struct DblVecPtrDeque : public std::deque<DblVec*> {
		~DblVecPtrDeque() {
			for (size_t s = 0; s < size(); ++s) {
				if ((*this)[s] != NULL) delete (*this)[s];
			}
		}
	};

	DblVec x, grad, newX, newGrad, dir;
	DblVec& steepestDescDir; // references newGrad to save memory, since we don't ever use both at the same time
	DblVecPtrDeque sList, yList;
	std::deque<double> roList;
	std::vector<double> alphas;
	double value;
	int iter, m;
	const size_t dim;
	DifferentiableFunction& func;
	double l1weight;
	bool quiet;

	static double dotProduct(const DblVec& a, const DblVec& b);
	static void add(DblVec& a, const DblVec& b);
	static void addMult(DblVec& a, const DblVec& b, double c);
	static void addMultInto(DblVec& a, const DblVec& b, const DblVec& c, double d);
	static void scale(DblVec& a, double b);
	static void scaleInto(DblVec& a, const DblVec& b, double c);

	void MapDirByInverseHessian();
	void UpdateDir();
	double DirDeriv() const;
	void GetNextPoint(double alpha);
	void BackTrackingLineSearch();
	void Shift();
	void MakeSteepestDescDir();
	double EvalL1();
	void FixDirSigns();
	void TestDirDeriv();

	OptimizerState(DifferentiableFunction& f, const DblVec& init, int m, double l1weight, bool quiet) 
		: x(init), grad(init.size()), newX(init), newGrad(init.size()), dir(init.size()), steepestDescDir(newGrad), alphas(m), iter(1), m(m), dim(init.size()), func(f), l1weight(l1weight), quiet(quiet) {
			if (m <= 0) {
				std::cerr << "m must be an integer greater than zero." << std::endl;
				exit(1);
			}
			value = EvalL1();
			grad = newGrad;
	}

public:
	const DblVec& GetX() const { return newX; }
	const DblVec& GetLastX() const { return x; }
	const DblVec& GetGrad() const { return newGrad; }
	const DblVec& GetLastGrad() const { return grad; }
	const DblVec& GetLastDir() const { return dir; }
	double GetValue() const { return value; }
	int GetIter() const { return iter; }
	size_t GetDim() const { return dim; }
};
