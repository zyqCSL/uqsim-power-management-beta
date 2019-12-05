#include "micro_service_path.hh"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <queue>
#include <stack>
// #include <utility>
#include <algorithm>
#include <string>
#include <iostream>

/*************** MicroServPathNode *****************/
MicroServPathNode::MicroServPathNode(const std::string& serv_name, const std::string& serv_func, int code_path, int start_stg, int end_stg, 
	unsigned path_id, unsigned node_id, bool sync, unsigned sync_node): servName(serv_name), servFunc(serv_func), codePath(code_path), 
	startStage(start_stg), endStage(end_stg), pathId(path_id), nodeId(node_id), sync(sync), syncNodeId(sync_node), syncNode(nullptr), 
	syncNum(1), temporary(false) {}

void
MicroServPathNode::setChilds(std::vector<MicroServPathNode*> child) {
	childs = child;
}

/** intra micro service **/
std::string 
MicroServPathNode::getServName() {
	return servName;
}

std::string
MicroServPathNode::getServDomain() {
	return servFunc;
}

int
MicroServPathNode::getCodePath() {
	return codePath;
}

int 
MicroServPathNode::getStartStg() {
	return startStage;
}

int 
MicroServPathNode::getEndStg() {
	return endStage;
}

/** across micro service path **/
unsigned 
MicroServPathNode::getPathId() {
	return pathId;
}

unsigned
MicroServPathNode::getNodeId() {
	return nodeId;
}

std::vector<MicroServPathNode*>
MicroServPathNode::getChilds() const {
	return childs;
}

bool
MicroServPathNode::needSync() {
	return sync;
}

void
MicroServPathNode::setSyncNum(unsigned sync_num) {
	assert(sync_num >= 1);
	syncNum = sync_num;
}

unsigned
MicroServPathNode::getSyncNum() {
	return syncNum;
}

unsigned
MicroServPathNode::getSyncNodeId() {
	return syncNodeId;
}

void 
MicroServPathNode::setSyncNode(MicroServPathNode* sync_node) {
	assert(sync);
	syncNode = sync_node;
}

MicroServPathNode* 
MicroServPathNode::getSyncNode() {
	return syncNode;
}

bool
MicroServPathNode::checkConnected(unsigned targNode) {
	// printf("checking node %d\n", nodeId);
	// BFS
	std::set<unsigned> visited;
	std::queue<MicroServPathNode*> nodeQ;
	nodeQ.push(this);

	while(!nodeQ.empty()) {
		MicroServPathNode* node = nodeQ.front();
		nodeQ.pop();
		// printf("connected to node %d, targ = %d\n", node->getNodeId(), targNode);
		if(node->getNodeId() == targNode)
			return true;
		else if(visited.find(node->getNodeId()) == visited.end()) {
			// printf("not visited before\n");
			visited.insert(node->getNodeId());
			for(auto childNode: node->getChilds()) 
				nodeQ.push(childNode);
		} else
			continue;
	}

	return false;
}

bool
MicroServPathNode::isTemp() {
	return temporary;
}

void
MicroServPathNode::setTemp() {
	temporary = true;
}

/*************** MicroServPath *****************/
MicroServPath::MicroServPath(unsigned pathId, unsigned entry): id(pathId), entryNodeId(entry) {}

void
MicroServPath::addNode(MicroServPathNode* node) {
	unsigned id = node->getNodeId();
	assert(graph.find(id) == graph.end());
	graph[id] = node;
}

MicroServPathNode*
MicroServPath::getNode(unsigned id) {
	return graph[id];
}

MicroServPathNode*
MicroServPath::getEntry() {
	return graph[entryNodeId];
}

void
MicroServPath::setSyncNode() {
	auto itr = graph.begin();
	while(itr != graph.end()) {
		MicroServPathNode* node = (*itr).second;
		if(node->needSync()) {
			unsigned syncNodeId = node->getSyncNodeId();
			node->setSyncNode(graph[syncNodeId]);
		}
		++itr;
	}
}

void
MicroServPath::setSyncNum(unsigned nodeId, unsigned syncNum) {
	std::cout << "Node: " << nodeId << " syncNum set to " << syncNum << std::endl;
	graph[nodeId]->setSyncNum(syncNum);
}

void
MicroServPath::setChilds(unsigned node, std::vector<unsigned> childs) {
	// std::string c = "";
	// for(unsigned k: childs)
	// 	c += std::to_string(k) + " ";
	// printf("node %d childs set to %s\n", node, c.c_str());

	std::vector<MicroServPathNode*> childNodes;
	for(unsigned childId: childs) {
		// printf("child id = %d\n", graph[id]->getNodeId());
		if(graph.find(childId) == graph.end()) {
			printf("In MicroServPath: %d, node: %d's child: %d doesn't exist in graph\n", id, node, childId);
			exit(-1);
		}
		childNodes.push_back(graph[childId]);
	}
	graph[node]->setChilds(childNodes);
}

void
MicroServPath::check() {
	for(auto itr: graph) {
		MicroServPathNode* node = itr.second;
		// can omit path spec only when this node goes through the entire micro-service
		if(node->getCodePath() == -1) {
			if(! (node->getStartStg() == 0 && node->getEndStg() == -1) ) {
				printf("Path Error: node %d in path %d omits path spec but does not go through the entire micro service\n", 
						node->getNodeId(), node->getPathId());
				exit(-1);
			}
		}

		if(node->needSync()) {
			assert(node->getSyncNode() != nullptr);
			assert(node->getSyncNodeId() != 0);

			// cannot sync at end of the path
			if(node->getEndStg() == -1) {
				printf("Path Error: node %d in path %d waits for response after leaving the micro service (endStage == -1)\n", 
						node->getNodeId(), node->getPathId());
				exit(1);
			}

			// check the sync node is in the same (service, codeP, stg) tuple as the end stage of current node
			MicroServPathNode* syncNode = node->getSyncNode();
			assert(syncNode != nullptr);

			if( !( syncNode->getServName() == node->getServName() && syncNode->getServDomain() == node->getServDomain()
				&& syncNode->getCodePath() == node->getCodePath() ) ) {
				printf("Path Error: in node %d in path %d, syncNode %d is not in the same (servName, servInst, code path) tuple as the current (req issuing node)\n", 
					node->getNodeId(), node->getPathId(), syncNode->getNodeId() );
				exit(1);
			}

			// no child can be in the same micro-service instance as the current node since the job is waiting for resp
			std::vector<MicroServPathNode*> childs = node->getChilds();
			for(auto childNode: childs) {
				if(childNode->getServName() == node->getServName() && childNode->getServDomain() == node->getServDomain()) {
					printf("Path Error: node %d in path %d enters the same micro service inst again at node: %d while watiting for response\n", 
							node->getNodeId(), node->getPathId(), childNode->getNodeId());
					exit(1);
				}
			}

			// check that every child can reach the syncNode
			for(auto childNode: childs) {
				bool conn = childNode->checkConnected(node->getSyncNodeId());
				if(!conn) {
					printf("Path Error: in node %d in path%d, childNode %d can't reach node %d's sync node %d\n", 
						node->getNodeId(), node->getPathId(), childNode->getNodeId(), node->getNodeId(), node->getSyncNodeId());
					exit(1);					
				}
			}
		} else {
			std::vector<MicroServPathNode*> childs = node->getChilds();

			// there must be exactly a child corresponding to the next stage of the same micro service instance
			// if this is not the end of execution at current micro service
			if(node->getEndStg() != -1) {
				unsigned ctr = 0;
				for(auto childNode: childs) {
					if(childNode->getServName() == node->getServName() && childNode->getServDomain() == node->getServDomain()) {
						if(childNode->getStartStg() != node->getEndStg() + 1) {
							printf("Path Error: node %d in path %d has a child node %d bypassing stage in the same micro service instance\n", 
								node->getNodeId(), node->getPathId(), childNode->getNodeId());
							exit(1);
						} else {
							++ctr;
							if(ctr > 1) {
								printf("Path Error: node %d in path %d has > 1 childs going into the same micro service instance as current node\n", 
									node->getNodeId(), node->getPathId());
								exit(1);
							}
						}
					}
				}

				if(ctr != 1) {
					printf("Path Error: node(async) %d in path %d has no childs going into the same micro service instance as current node\n", 
							node->getNodeId(), node->getPathId());
					exit(1);
				}

			}
		}		 
	}

	// checks cycle free
	// DFS, (nodePtr, visited_set)
	std::stack< std::vector<MicroServPathNode*> > nodeStack;
	std::vector<MicroServPathNode*> initPath;
	initPath.push_back(getEntry());
	nodeStack.push(initPath);
	while(!nodeStack.empty()) {
		auto path = nodeStack.top();
		nodeStack.pop();
		assert(path.size() > 0);
		auto itr = --(path.end());
		MicroServPathNode* node = path[path.size() - 1];
		bool visited = !(find(path.begin(), itr, node) == itr);

		if(visited) {
			std::string cycle = "Path " + std::to_string(node->getPathId()) + ": ";
			for(auto item: path)
				cycle += std::to_string(item->getNodeId()) + ", ";
			cycle = cycle.substr(0, cycle.size() - 2);
			printf("Path Error: a cycle detected: %s\n", cycle.c_str());
			exit(1);
		} else {
			for(auto child: node->getChilds()) {
				std::vector<MicroServPathNode*> newPath = path;
				newPath.push_back(child);
				nodeStack.push(newPath);
			}
		}
	}

	printf("Path %d check done\n", id);
}