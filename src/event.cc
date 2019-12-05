#include <assert.h>
#include <iomanip> // setprecision
#include <sstream> // stringstream

#include <iostream>

#include "event.hh"

Event::Event(EventType type):
	    type(type), time(0), core(nullptr), threadId(0), servCompl(false), criSec(false), thdCnt(nullptr), 
	    del(false), freq(0) {}

std::string
Event::present() {
	std::string s;

	std::stringstream stream;
	stream << std::fixed << std::setprecision(3) << time;
	std::string time = stream.str();

	std::string job_list = "";
	for(auto j: jobList)
		job_list += std::to_string(j->id) + ", ";
	job_list = job_list.substr(0, job_list.size());

	if(type == EventType::JOB_RECV) 
		s = "JOB_RECV, job: " + job_list + " at time: " + time + " ns"; 
	else if(type == EventType::COMPUT_COMPLETE)
		s = "COMPUT_COMPLETE, job: " + job_list + " at time: " + time + " ns";
	else if(type == EventType::INC_FULL_FREQ)
		s = "INC_FULL_FREQ at time: " + time + " ns";
	else if(type == EventType::INC_FREQ)
		s = "INC_FREQ at time: " + time + " ns";
	else if(type == EventType::DEC_FREQ) 
		s = "DEC_FREQ at time: " + time + " ns";
	else if(type == EventType::SET_FREQ)
		s = "SET_FREQ to " + std::to_string(freq);
	return s;
}

void
Event::registerCallBack(std::function<bool (Time)> cb) {
	callbacks.push_back(cb);
}

void
Event::run(Time globalTime) {
	// run one callback each time
	assert(!callbacks.empty());
	auto func = *(callbacks.begin());
	func(globalTime);
	callbacks.pop_front();
}

void
EventQueue::filterUp(unsigned idx) {
	if(idx == 0) 
		return;
	
	unsigned next_idx = (idx - 1)/2;
	if(queue[next_idx]->time <= queue[idx]->time)
		return;
	else {
		Event* temp = queue[idx];
		queue[idx] = queue[next_idx];
		queue[next_idx] = temp;
		filterUp(next_idx);
	}
}

void
EventQueue::filterDown(unsigned idx) {
	unsigned left_child = 2 * idx + 1;
	unsigned right_child = 2 * idx + 2;
	unsigned min_child = left_child;
	if(left_child >= numEvents)
		return;
	else if(right_child < numEvents && queue[left_child]->time > queue[right_child]->time)
		min_child = right_child;
	if(queue[min_child]->time < queue[idx]->time) {
		Event* temp = queue[idx];
		queue[idx] = queue[min_child];
		queue[min_child] = temp;
		filterDown(min_child);
	} else	
		return;
}

void
EventQueue::enqueue(Event* event) {
	unsigned idx = numEvents;
	++numEvents;
	if(numEvents > queue.size())
		queue.push_back(event);
	else
		queue[idx] = event;
	filterUp(idx);
}

Event*
EventQueue::pop() {
	if(numEvents == 0)
		return nullptr;
	Event* next = queue[0];
	--numEvents;
	queue[0] = queue[numEvents];
	queue[numEvents] = nullptr;
	filterDown(0);
	return next;
}

Time
EventQueue::nextEventTime() {
	if(numEvents == 0)
		return INVALID_TIME;
	else {
		assert(queue.size() > 0);
		assert(queue[0] != nullptr);
		// std::cout << "next cluster event: " << queue[0]->present() << std::endl;
		return queue[0]->time;
	}
}

void
EventQueue::show() {
	for(unsigned i = 0; i < numEvents; ++i)
		std::cout << queue[i]->present() << std::endl;
	// std::cout << std::endl;
}