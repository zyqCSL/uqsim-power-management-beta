#ifndef __CHUNK_MODEL_HH__
#define __CHUNK_MODEL_HH__

#include <random>
#include <chrono>
#include <cmath>
#include <assert.h>

class ChunkModel 
{
	public:
		virtual ~ChunkModel() {}
		virtual unsigned chunkNum() = 0;

};

class ConstChunkModel: public ChunkModel
{
	private:
		unsigned num;
	
	public:
		ConstChunkModel(unsigned n);
		unsigned chunkNum() override;
};


class ExpoChunkModel: public ChunkModel
{
	private:
		double chunkLambda;

		std::exponential_distribution<double> chunkDist;
		std::default_random_engine chunkGen;
	
	public:
		ExpoChunkModel(unsigned num);
		unsigned chunkNum() override;
};

#endif