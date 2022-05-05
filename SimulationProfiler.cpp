// SimulationProfiler.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "DispersalManager.h"
#include "DispersalKernel.h"
#include "SimulationProfiler.h"
#include <inttypes.h>
#define __STDC_FORMAT_MACROS

SimulationProfiler::SimulationProfiler() : 
overallTimer(timeTotal), 
simulationTimer(timeSim), 
initialisationTimer(timeInitialise),
interventionTimer(timeInter),
fullRecalculationTimer(timeFullRecalculation),
infectionRateRecalculationTimer(timeInfectionRateRecalculation),
kernelCouplingTimer(timeCoupling),
kernelVSTimer(timeVS)
{

	timers.push_back(&overallTimer);
	timers.push_back(&simulationTimer);
	timers.push_back(&initialisationTimer);
	timers.push_back(&interventionTimer);
	timers.push_back(&fullRecalculationTimer);
	timers.push_back(&infectionRateRecalculationTimer);
	timers.push_back(&kernelCouplingTimer);
	timers.push_back(&kernelVSTimer);

	resetTimers();

}


SimulationProfiler::~SimulationProfiler() {



}

void SimulationProfiler::resetTimers() {

	for (size_t iTimer = 0; iTimer < timers.size(); ++iTimer) {
		timers[iTimer]->stop();
	}

	//Initialise all summary timers/data:
	timeInitialise = 0;
	timeSim = 0;
	timeFullRecalculation = 0;
	nFullRecalculations = 0;
	timeInfectionRateRecalculation = 0;
	nInfectionRateRecalculations = 0;
	timeInter = 0;
	timeTotal = 0;
	totalNoEvents = 0;
	timeCoupling = 0;
	timeVS = 0;
	nCouplingEvents = 0;

	//Debugging:
	nIllegalEvents = 0;

}

void SimulationProfiler::writeSummaryData(char *szSummaryFileName, int64_t nKernelLongAttempts, int64_t nKernelLongSuccesses) {

	overallTimer.stop();

	FILE *pSimFile = fopen(szSummaryFileName, "a");
	if (pSimFile == NULL) { printk("\nERROR: Simulation unable to write summary file %s\n\n", szSummaryFileName); return; }
	//Profiling Data:
	printHeaderText(pSimFile, "Profiling Data:");

	const char *sProfileFormatStr = "%-48s %15.2f (%6.2f%%)\n";

	double fullTime = timeTotal + timeInitialise;

	fprintf(pSimFile, sProfileFormatStr, "Total time (s):", double(fullTime / 1000.0), 100.0*fullTime / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "Time spent initialising data (s):", double(timeInitialise / 1000.0), 100.0*timeInitialise / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "Time spent in Simulation (s):", double(timeSim / 1000.0), 100.0*timeSim / fullTime);
	double timeNormal = timeSim - timeFullRecalculation - timeInfectionRateRecalculation;
	fprintf(pSimFile, sProfileFormatStr, "  in Normal Simulation (s):", double(timeNormal / 1000.0), 100.0*timeNormal / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "  in Full Recalculations (s):", double(timeFullRecalculation / 1000.0), 100.0*timeFullRecalculation / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "  Number of Full Recalculations:", double(nFullRecalculations), 100.0*timeFullRecalculation / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "  in InfectionRate Recalculations (s):", double(timeInfectionRateRecalculation / 1000.0), 100.0*timeInfectionRateRecalculation / fullTime);
	fprintf(pSimFile, sProfileFormatStr, "  Number of InfectionRate Recalculations:", double(nInfectionRateRecalculations), 100.0*timeInfectionRateRecalculation / fullTime);
	int timeTotalDispersal = timeCoupling + timeVS;
	fprintf(pSimFile, sProfileFormatStr, "    Simulation time processing dispersal (s): ", double(timeTotalDispersal / 1000.0), 100.0*timeTotalDispersal / timeSim);
	fprintf(pSimFile, sProfileFormatStr, "      coupling dispersal (s): ", double(timeCoupling / 1000.0), 100.0*timeCoupling / timeTotalDispersal);
	fprintf(pSimFile, sProfileFormatStr, "      VS dispersal (s): ", double(timeVS / 1000.0), 100.0*timeVS / timeTotalDispersal);
	fprintf(pSimFile, sProfileFormatStr, "Time spent in Interventions (s):", double(timeInter / 1000.0), 100.0*timeInter / fullTime);
	//Remove unsuccessful long kernel attempts:
	int64_t nTotalActualEvents = totalNoEvents - nKernelLongAttempts + nKernelLongSuccesses;
	fprintf(pSimFile, "Total events:            %20" PRId64 "\n", totalNoEvents);
	fprintf(pSimFile, "Total productive events: %20" PRId64 "\n", nTotalActualEvents);
	if (timeSim <= 0) {//Handle very short simulations
		timeSim = 1;
	}
	fprintf(pSimFile, "Events per second:               %15.2f\n", 1000.0*double(nTotalActualEvents) / double(timeSim));

	fprintf(pSimFile, "Illegal events:               %15d\n", nIllegalEvents);

	fprintf(pSimFile, "\n");

	//Captain Hindsight:
	//TODO: only easy to do for a single kernel
	//TODO:Need a way to get marginal/cumulative sum of kernel for each range
	//TODO: Need a way to log number of infectiousness changing events - will basically mean putting something in the coupling calculations
	fprintf(pSimFile, "Captain Hindsight:\n");
	fprintf(pSimFile, "(Note that Captain Hindsight assumes that the kernel for population 0 is the most important one and will not work well for more than one dispersal kernel)\n");
	
	//Data gathering:
	double timeSimTotal_ms = timeSim;
	double timeDispersalVS_ms = timeVS;
	double timeDispersalCoupling_ms = timeCoupling;

	DispersalKernel *targetKernel = DispersalManager::getKernel(0);

	int kernelShortRange = targetKernel->getShortRange();
	double kernelVSProportion = targetKernel->getVirtualSporulationProportion();
	int64_t nVS = nKernelLongAttempts;
	int nCoupling = nCouplingEvents;
	int kernelArea = (1 + 2 * kernelShortRange) * (1 + 2 * kernelShortRange);

	int64_t nPotentialVS = 0;
	if (kernelVSProportion > 0.0) {
		nPotentialVS = nVS / kernelVSProportion;
	} else {
		fprintf(pSimFile, "Warning: Captain Hindsight has no data on Virtual Sporulation, so it will not do a good job estimating...\n");
	}


	//Cost = cFixed + cCoupling * numInfectiousnessChangingEvents * kernelArea + cVS * #VirtualSporulations
	//Fit costs given profiling data
	double cFixed = timeSimTotal_ms - timeDispersalVS_ms - timeDispersalCoupling_ms;

	double cCoupling = 0.0;
	if (nCoupling > 0) {
		cCoupling = timeDispersalCoupling_ms / (double(nCoupling) * double(kernelArea));
	}

	double cVS = 0.0;
	if (nVS > 0) {
		cVS = timeDispersalVS_ms / double(nVS);
	}

	fprintf(pSimFile, "CostCoefficientFixed:    %20.6f\n", cFixed);
	fprintf(pSimFile, "CostCoefficientCoupling: %20.6f\n", cCoupling);
	fprintf(pSimFile, "CostCoefficientVS:       %20.6f\n", cVS);
	fprintf(pSimFile, "#PotentialVS:            %20" PRId64 "\n", nPotentialVS);
	fprintf(pSimFile, "#Coupling:               %20d\n", nCoupling);

	//Use the kernel marginal sum data to figure out which choice of kernelShortRange would have been optimal
	auto kernelCumulativeSum = targetKernel->getCumulativeSumByRange();


	std::vector<double> predictedRunTimes(kernelCumulativeSum.size());

	for (int iRange = 0; iRange < kernelCumulativeSum.size(); ++iRange) {
		int trialKernelArea = (1 + 2 * iRange) * (1 + 2 * iRange);
		int64_t trialnVS = nPotentialVS * (1.0 - kernelCumulativeSum[iRange]);
		predictedRunTimes[iRange] = cFixed + cCoupling * nCoupling * trialKernelArea + cVS * trialnVS;
	}

	//Find predicted best range:
	int iBestRange = 0;
	double dBestRunTime = predictedRunTimes[iBestRange];

	for (int iRange = 1; iRange < predictedRunTimes.size(); ++iRange) {
		if (predictedRunTimes[iRange] < dBestRunTime) {
			dBestRunTime = predictedRunTimes[iRange];
			iBestRange = iRange;
		}
	}

	fprintf(pSimFile, "Captain Hindsight predicts:\n");
	fprintf(pSimFile, "Best Coupling range: %6d\n", iBestRange);
	fprintf(pSimFile, "Best VS Proportion:   %12.6f\n", 1.0 - kernelCumulativeSum[iBestRange]);
	fprintf(pSimFile, "Predicted Time (s):   %12.6f\n", predictedRunTimes[iBestRange] / 1000.0);

	if (kernelShortRange == iBestRange) {
		fprintf(pSimFile, "Captain Hindsight agrees with your Kernel Coupling Range\n");
	} else {
		fprintf(pSimFile, "Captain Hindsight thinks you should %s your Kernel Coupling Range (but nobody likes a Captain Hindsight)\n", (iBestRange > kernelShortRange)?("increase"):("decrease"));
	}

	//Report predicted run times for every choice
	fprintf(pSimFile, "Captain Hindsight Full data:\n");
	fprintf(pSimFile, "CH_Range: ");
	for (int iRange = 0; iRange < predictedRunTimes.size(); ++iRange) {
		fprintf(pSimFile, "%d ", iRange);
	}
	fprintf(pSimFile, "\n");
	fprintf(pSimFile, "CH_KernelProportion: ");
	for (int iRange = 0; iRange < kernelCumulativeSum.size(); ++iRange) {
		fprintf(pSimFile, "%.6f ", kernelCumulativeSum[iRange]);
	}
	fprintf(pSimFile, "\n");
	fprintf(pSimFile, "CH_PredictedTime: ");
	for (int iRange = 0; iRange < predictedRunTimes.size(); ++iRange) {
		fprintf(pSimFile, "%.6f ", predictedRunTimes[iRange] / 1000.0);
	}
	fprintf(pSimFile, "\n");

	fprintf(pSimFile, "\n");

	fclose(pSimFile);

	//Reset for next iteration:
	resetTimers();

	//Bit hacky:
	overallTimer.start();

}
