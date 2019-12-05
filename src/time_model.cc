#include "time_model.hh"
#include <iostream>

void
TimeModel::reset(Time newLatency) {
	return;
}

/************** ExpoTimeModel ****************/

ConstTimeModel::ConstTimeModel(Time latency): latency(latency), curLatency(latency) {}

Time
ConstTimeModel::lat() {
	return curLatency;
}

void
ConstTimeModel::setFreq(unsigned baseFreq, unsigned curFreq, double scaleFactor) {
	curLatency = (Time)( (1.0 - scaleFactor + scaleFactor * (double)baseFreq/(double)curFreq) * latency );
}

/************** ExpoTimeModel ****************/

ExpoTimeModel::ExpoTimeModel(Time latency): latency(latency),  curLatency(latency), lambda(1.0/latency) {
	int sd = std::chrono::system_clock::now().time_since_epoch().count();
	gen.seed(sd);
	dist = std::exponential_distribution<double> (lambda);
}

void
ExpoTimeModel::reset(Time newLatency) {
	if(curLatency != newLatency) {
		curLatency = newLatency;
		lambda = 1.0/curLatency;

		int sd = std::chrono::system_clock::now().time_since_epoch().count();
		gen.seed(sd);
		dist = std::exponential_distribution<double> (lambda);
	}	
}

void
ExpoTimeModel::setFreq(unsigned baseFreq, unsigned curFreq, double scaleFactor) {
	// std::cout << "ExpoTimeModel curFreq set to " << curFreq << std::endl;
	Time lat = (Time)( latency*(1.0 - scaleFactor + scaleFactor * (double)baseFreq/(double)curFreq) );
	reset(lat);
}

Time
ExpoTimeModel::lat() {
	return (Time)dist(gen);
}