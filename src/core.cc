#include <assert.h>
#include <iostream>

#include "core.hh"

Core::Core(unsigned id): total_time(0), id(id), stat(IDLE) {}

bool
Core::isAvail() {
	return (stat == IDLE);
}

void
Core::setIdle() {
	// std::cout << "Core " << id << " set idle" << std::endl;
	assert(stat == RUNNING);
	stat = IDLE;
}

void
Core::setRunning() {
	// std::cout << "Core " << id << " set running" << std::endl;
	stat = RUNNING;
}

unsigned
Core::getId() {
	return id;
}