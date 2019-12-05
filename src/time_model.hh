#ifndef __TIME_MODEL_HH__
#define __TIME_MODEL_HH__

#include <random>
#include <chrono>

#include "util.hh"

class TimeModel 
{
	public:
		virtual ~TimeModel() {}
		virtual Time lat() = 0;
		virtual void reset(Time newLatency);

		virtual void setFreq(unsigned baseFreq, unsigned curFreq, double scaleFactor) = 0;
		virtual Time getLat() = 0;

};

class ConstTimeModel: public TimeModel
{
	protected:
		Time latency;
		Time curLatency;
	
	public:
		ConstTimeModel(Time latency);
		Time lat() override;
		void setFreq(unsigned baseFreq, unsigned curFreq, double scaleFactor) override;
		Time getLat() override {return curLatency;}
};

class ExpoTimeModel: public TimeModel
{
	protected:
		Time latency;
		Time curLatency;
		double lambda;

		std::exponential_distribution<double> dist;
		std::default_random_engine gen;
	
	public:
		ExpoTimeModel(Time latency);
		void reset(Time newLatency) override;
		Time lat() override;
		void setFreq(unsigned baseFreq, unsigned curFreq, double scaleFactor) override;
		Time getLat() override {return curLatency;}
};

#endif