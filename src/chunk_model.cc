#include "chunk_model.hh"
#include <iostream>

/************** ConstChunkModel ****************/
ConstChunkModel::ConstChunkModel(unsigned n): num(n) {}

unsigned
ConstChunkModel::chunkNum() {
	return num;
}

/************** ExpoChunkModel ****************/

ExpoChunkModel::ExpoChunkModel(unsigned num): chunkLambda(1.0/num) {
	unsigned sd = std::chrono::system_clock::now().time_since_epoch().count();
	chunkGen.seed(sd);
	// std::cout << "chunkLambda = " << chunkLambda << std::endl;
	chunkDist = std::exponential_distribution<double> (chunkLambda);
}

unsigned
ExpoChunkModel::chunkNum() {
	double interm = chunkDist(chunkGen);
	// std::cout << "chunk interm = " << interm << std::endl << std::endl;

	assert(interm >= 0);
	if(interm == 0.0)
		interm = 1.0;
	unsigned chunk = std::ceil(interm);
	return chunk;
}