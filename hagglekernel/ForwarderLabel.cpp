#include <libcpphaggle/Platform.h>
#include "ForwarderLabel.h"
#include "XMLMetadata.h"

#include <math.h>

ForwarderLabel::ForwarderLabel(ForwardingManager *m, const EventType type) :
	ForwarderAsynchronous(m, type, PROPHET_NAME),
	myRank(1), kernel(getManager()->getKernel()), next_id_number(1),
	rib_timestamp(Timeval::now())
{
	myNodeStr=kernel->getThisNode()->getIdStr();
	// Ensure that the local node's forwarding id is 1:
	myNodeId=id_from_string(myNodeStr);
	
	myLabel=string("temp");
	
	if (gethostname(hostname, HOSTNAME_LEN) != 0) {
			HAGGLE_ERR("Could not get hostname\n");
	}

	// Always zero terminate in case the hostname didn't fit
	hostname[HOSTNAME_LEN-1] = '\0';
	
	HAGGLE_DBG("Forwarding module \'%s\' hostname %s initialization.\n", getName(), hostname); 
}

ForwarderLabel::~ForwarderLabel()
{
}

size_t ForwarderLabel::getSaveState(RepositoryEntryList& rel)
{
	for (bubble_rib_t::iterator it = rib.begin(); it != rib.end(); it++) {
		char value[256];
		snprintf(value, 256, "%s:%ld", (*it).second.first.c_str(), (*it).second.second);
		HAGGLE_DBG("HAGGLE_DBG: Repository value is %s\n", value);
		rel.push_back(new RepositoryEntry(getName(), id_number_to_nodeid[(*it).first].c_str(), value));
	}
	
	return rel.size();
}

bool ForwarderLabel::setSaveState(RepositoryEntryRef& e)
{
	if (strcmp(e->getAuthority(), getName()) != 0)
		return false;
	
	string value = e->getValueStr();
	size_t pos = value.find(':');
	
	bubble_metric_t& metric = rib[id_from_string(e->getKey())];
	
	// The first part of the value is the label metric
	metric.first = value.substr(0, pos).c_str();
	// The second part is the rank
	metric.second = atol(value.substr(pos + 1).c_str());
	
	HAGGLE_DBG("HAGGLE_DBG: orig string=%s label=%s, rank=%ld\n", 
		e->getValue(), metric.first.c_str(), metric.second);
	
	rib_timestamp = Timeval::now();
	
	return true;
}

//get the id number from id sting
bubble_node_id_t ForwarderLabel::id_from_string(const string& nodeid)
{
	//reference to second
	bubble_node_id_t &retval = nodeid_to_id_number[nodeid];

	//if this is a new id
	if (retval == 0) {
		retval = next_id_number;
		id_number_to_nodeid[retval] = nodeid;
		next_id_number++;
	}

	return retval;
}

/**
 Add routing information to a data object.
 The parameter "parent" is the location in the data object where the routing 
 information should be inserted.
 */ 
bool ForwarderLabel::addRoutingInformation(DataObjectRef& dObj, Metadata *parent)
{
	if (!dObj || !parent)
		return false;
	
	// Add first our own node ID.
	parent->setParameter("node_id", kernel->getThisNode()->getIdStr());
	
	Metadata *mm = parent->addMetadata("Metric", myNodeStr);
	
	mm->setParameter("hostid",hostname);
	
	mm->setParameter("label", myLabel);
	
	char tmp[32];
	sprintf(tmp,"%ld",myRank);
	mm->setParameter("rank", tmp);
	
	HAGGLE_DBG("HAGGLE_DBG: Sending metric node:%s label:%s rank:%s \n",parent->getParameter("node_id"),
	mm->getParameter("label"),mm->getParameter("rank"));
				
	dObj->setCreateTime(rib_timestamp);
	
	return true;
}

bool ForwarderLabel::newRoutingInformation(const Metadata *m)
{	
	if (!m || m->getName() != getName())
		return false;
	
	bubble_node_id_t node_b_id = id_from_string(m->getParameter("node_id"));
	
	HAGGLE_DBG("HAGGLE_DBG: New %s routing information received from %s number %ld \n",
		   getName(),
		   m->getParameter("node_id"), id_from_string(m->getParameter("node_id")));
	
	const Metadata *mm = m->getMetadata("Metric");
	
	while (mm) {
		
		//bubble_node_id_t node_c_id = id_from_string(mm->getParameter("node_id"));
		
		LABEL_T node_b_label = mm->getParameter("label");
		
		const char* node_b_hostid = mm->getParameter("hostid");
		
		const char* tmp = mm->getParameter("rank");
		
		RANK_T node_b_rank = atol(tmp);
		
		rib[node_b_id].first = node_b_label;
		
		rib[node_b_id].second = node_b_rank;

		
		HAGGLE_DBG("Routing received from %s label:%s rank:%ld \n",node_b_hostid , rib[node_b_id].first.c_str(), rib[node_b_id].second);
		
		//HAGGLE_DBG("HAGGLE_DBG: Routing received from node:%s label:%s rank:%ld \n",id_number_to_nodeid[node_b_id].c_str(), rib[node_b_id].first.c_str(), rib[node_b_id].second);
		
		mm= m->getNextMetadata();
	
	}
	
	bubble_rib_t::iterator jt = rib.begin();
	
	while (jt != rib.end()) {

		//bool isNeighbor = getKernel()->getNodeStore()->stored(id_number_to_nodeid[jt->first], true);
		
		HAGGLE_DBG("HAGGLE_DBG: Printing node_id_num:%ld label:%s rank:%ld \n", jt->first, jt->second.first.c_str() , jt->second.second);
		jt++;
	}
		
	rib_timestamp = Timeval::now();
	
	return true;
}

void ForwarderLabel::_newNeighbor(const NodeRef &neighbor)
{
	// We don't handle routing to anything but other haggle nodes:
	if (neighbor->getType() != Node::TYPE_PEER)
		return;
	
	// Update our private metric regarding this node:
	bubble_node_id_t neighbor_id = id_from_string(neighbor->getIdStr());
	
	HAGGLE_DBG("HAGGLE_DBG: _newNeighbor new node found: node %ld %s \n", neighbor_id, neighbor->getIdStr());
	
	rib_timestamp = Timeval::now();
}

void ForwarderLabel::_endNeighbor(const NodeRef &neighbor)
{
	// We don't handle routing to anything but other haggle nodes:
	if (neighbor->getType() != Node::TYPE_PEER)
		return;
	
	// Update our private metric regarding this node:
	bubble_node_id_t neighbor_id = id_from_string(neighbor->getIdStr());
	
	HAGGLE_DBG("HAGGLE_DBG: _endNeighbor node left: node %ld %s \n", neighbor_id, neighbor->getIdStr());

	rib_timestamp = Timeval::now();
}

static void sortedNodeListInsert(List<Pair<NodeRef, bubble_metric_t> >& list, NodeRef& node, bubble_metric_t metric)
{
	list.push_back(make_pair(node, metric));
}

void ForwarderLabel::_generateTargetsFor(const NodeRef &neighbor)
{
	
}

void ForwarderLabel::_generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets)
{
						
	List<Pair<NodeRef, bubble_metric_t> > sorted_delegate_list;
	
	// Figure out which node to look for:
	bubble_node_id_t target_id = id_from_string(target->getIdStr());
	
	LABEL_T targetLabel = rib[target_id].first;
	//RANK_T targetRank = rib[target_id].second;
	
	HAGGLE_DBG("HAGGLE_DBG:_generateDelegatesFor node %d string %s label %s\n", target_id, target->getIdStr(), targetLabel.c_str());


	for (bubble_rib_t::iterator it = rib.begin();it != rib.end(); it++)
	{
		if (it->first != this_node_id && it->first != target_id) {

			NodeRef delegate = kernel->getNodeStore()->retrieve(id_number_to_nodeid[it->first], true);

			if (delegate && !isTarget(delegate, other_targets)) {
				
				LABEL_T &neighborLabel = it->second.first;
				RANK_T &neighborRank = it->second.second;
				
				HAGGLE_DBG("HAGGLE_DBG: _generateDelegatesFor neighborLabel=%s, targetLabel=%s\n", neighborLabel.c_str(), targetLabel.c_str());
				
				if (neighborLabel.compare(targetLabel)==0)
				{
					//NodeRef delegate = Node::create_with_id(Node::TYPE_PEER, id_number_to_nodeid[it->first].c_str(), "Label delegate node");
					sortedNodeListInsert(sorted_delegate_list, delegate, it->second);
					HAGGLE_DBG("HAGGLE_DBG: _generateDelegatesFor Label same: Node '%s' is a good delegate for target '%s' [label=%s, rank=%ld]\n", 
					delegate->getName().c_str(), target->getName().c_str(), neighborLabel.c_str(), neighborRank);
					
				}
			}
		}
	}
	
	// Add up to max_generated_delegates delegates to the result in order of decreasing metric
	if (!sorted_delegate_list.empty()) {
		
		NodeRefList delegates;
		unsigned long num_delegates = max_generated_delegates;
		
		while (num_delegates && sorted_delegate_list.size()) {
			NodeRef delegate = sorted_delegate_list.front().first;
			sorted_delegate_list.pop_front();
			delegates.push_back(delegate);
			num_delegates--;
		}
		kernel->addEvent(new Event(EVENT_TYPE_DELEGATE_NODES, dObj, target, delegates));
		HAGGLE_DBG("HAGGLE_DBG: Forward Generated %lu delegates for target %s\n", delegates.size(), target->getName().c_str());
	} else {
                HAGGLE_DBG("No delegates found for target %s\n", target->getName().c_str());
    }

}


void ForwarderLabel::_onForwarderConfig(const Metadata& m)
{
	if (strcmp(getName(), m.getName().c_str()) != 0)
		return;
	
	HAGGLE_DBG("Prophet forwarder configuration\n");
	
	const char *param = m.getParameter("label");
	if (param) {
		
		myLabel = string(param);
		HAGGLE_DBG("%s: Setting label to %s\n", getName(), myLabel.c_str());
	}
	
	param = m.getParameter("rank");
	if (param)
	{
		char *ptr = NULL;
		RANK_T p = strtol(param,&ptr,10);
		
		if (ptr && ptr != param && *ptr == '\0')
		{
			myRank = p;
			HAGGLE_DBG("%s: Setting rank to %ld\n", getName(), myRank);
		}
	}
	
}

void ForwarderLabel::_printRoutingTable(void)
{
	printf("%s routing table:\n", getName());
}
