#ifndef _GRAPH_PATH_
#define _GRAPH_PATH_

#include <vector>
#include <unordered_map>
#include <string>

class MicroServPathNode;

class MicroServPathNode
{
	private:
		/*** micro service path info ***/
		// name of the micro service type that this node corresponds to
		// ATTENTION: not instance name!
		std::string servName;
		// an annotation field to distinguish same service that serves different functionality
		std::string servFunc;
		// set to - 1 when no path is specified
		// TODO: needs to check that servPathId can only equal -1
		// when this node travels throught the whole micro-service (start = 0, end = -1)
		// otherwise needs splitting into 2 paths for safety
		int codePath;
		// the start & end stages that this node will travel through in the micro service
		// startStage is the stage that the job enters when it reaches the job
		// endStage is the first stage along code path a request is issued by the job and responses needs synchronized
		int startStage;
		int endStage;

		/*** micro service path & sync info ***/
		unsigned pathId;
		unsigned nodeId;

		// child nodes in path graph
		std::vector<MicroServPathNode*> childs;

		// if this node has a child group that needs to synchronized at the same micro service instance
		// and the job halts waiting for responses in the same stage as the stage where requests were issued
		bool sync;
		unsigned syncNodeId;
		MicroServPathNode* syncNode;

		// # input edge connecting to this node
		// MicroServPath model requires that all incoming edges connecting to the same node (in graph path) should be synchronized
		unsigned syncNum;

		// if this node is created in execution temporarily (for chunk processing)
		bool temporary;

	public:
		MicroServPathNode(const std::string& serv_name, const std::string& serv_func, int code_path, int start_stg, int end_stg, 
			unsigned path_id, unsigned node_id, bool sync, unsigned sync_node);
		~MicroServPathNode() {}
		void setChilds(std::vector<MicroServPathNode*> child);

		/** intra micro service **/
		std::string getServName();
		std::string getServDomain();
		int getCodePath();
		int getStartStg();
		int getEndStg();

		/** micro service path **/
		unsigned getPathId();
		unsigned getNodeId();

		std::vector<MicroServPathNode*> getChilds() const;

		/** synchronization **/
		bool needSync();
		// sync at current node
		void setSyncNum(unsigned sync_num);
		unsigned getSyncNum();
		// sync at sync node (the node waiting for resp from requests issued at current node)
		unsigned getSyncNodeId();
		void setSyncNode(MicroServPathNode* sync_node);
		MicroServPathNode* getSyncNode();

		// check if nodeId is reachable from this node in the path
		bool checkConnected(unsigned targNode);

		// temporary nde
		bool isTemp();
		void setTemp();
};

class MicroServPath
{
	private:
		unsigned id;
		std::unordered_map<unsigned, MicroServPathNode*> graph;
		unsigned entryNodeId;

	public:
		MicroServPath(unsigned pathId, unsigned entry);
		~MicroServPath() {}

		void addNode(MicroServPathNode* node);
		void setSyncNode();
		void setSyncNum(unsigned nodeId, unsigned syncNum);
		void setChilds(unsigned node, std::vector<unsigned> childs);
		MicroServPathNode* getNode(unsigned id);
		MicroServPathNode* getEntry();

		void check();
};

#endif