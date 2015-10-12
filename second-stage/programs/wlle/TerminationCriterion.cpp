#include "TerminationCriterion.h"

#include "OWLQN.h"

#include <limits>
#include <iomanip>
#include <cmath>

using namespace std;

double RelativeMeanImprovementCriterion::GetValue(const OptimizerState& state, std::ostream& message) {
	double retVal = numeric_limits<double>::infinity();

	if (prevVals.size() > 5) {
		double prevVal = prevVals.front();
		if (prevVals.size() == 10) prevVals.pop_front();
		double averageImprovement = (prevVal - state.GetValue()) / prevVals.size();
		double relAvgImpr = averageImprovement /(1+fabs(state.GetValue()));  // !!!! modified MJ, 26th Aug 2008
		message << setprecision(4) << scientific << right;
		message << "  (" << setw(10) << relAvgImpr << ") " << flush;
		retVal = relAvgImpr;
	} else {
		message << "  (wait for five iters) " << flush;
	}

	prevVals.push_back(state.GetValue());
	return retVal;
}
