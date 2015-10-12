#pragma once

#include <deque>
#include <sstream>
#include <string>

class OptimizerState;

struct TerminationCriterion {
	virtual double GetValue(const OptimizerState& state, std::ostream& message) = 0;
};

class RelativeMeanImprovementCriterion : public TerminationCriterion {
	const int numItersToAvg;
	std::deque<double> prevVals;

public:
	RelativeMeanImprovementCriterion(int numItersToAvg = 5) : numItersToAvg(numItersToAvg) {}

	double GetValue(const OptimizerState& state, std::ostream& message);
};
