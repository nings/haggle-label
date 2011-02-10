#ifndef _FORWARDERPROPHET_H
#define _FORWARDERPROPHET_H

class ForwarderProphet;

#include "ForwarderAsynchronous.h"
#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

using namespace haggle;

/**
 * LABEL uses string format.
 */
#define LABEL_T string
/**
 * RANK uses long format.
 */
#define RANK_T long

/**
 * bubble_node_id_t is the identity
 */
typedef unsigned long bubble_node_id_t;

#define this_node_id ((bubble_node_id_t) 1)

/**
 * forwarding metrics consist of the LABEL and RANK
 * NOTE: only LABEL has been implemented at the moment
 */
typedef Pair<LABEL_T, RANK_T> bubble_metric_t;

/**
 * forwarding metrics table performs the mapping between the bubble_node_id_t and forwarding metrics
 */
typedef Map<bubble_node_id_t, bubble_metric_t> bubble_rib_t;

#define NF_max (5) // Not sure what a good number is here...

#define HOSTNAME_LEN 64

/**
 * Forwarding module based on LABEL algorithms
 */
class ForwarderProphet : public ForwarderAsynchronous {

#define PROPHET_NAME "Prophet"

#define LABEL_NAME "Null"


private:  

	/**
	 * MyRank
	 */
	LABEL_T myLabel;
	/**
	 * MyLabel
	 */
	RANK_T myRank;
	/**
	 * MyNodeId
	 */
	bubble_node_id_t myNodeId;
	/**
	 * MyNodeStringId
	 */
	string myNodeStr;
	/**
	 * MyHostname
	 */
	char hostname[HOSTNAME_LEN];
	
	HaggleKernel *kernel;

	/**
	 * converting table: convert from the NodeStringId to the NodeId
	 */   
	Map<string, bubble_node_id_t> nodeid_to_id_number;
	/**
	 * converting table: convert from the NodeId to the NodeStringId
	 */
	Map<bubble_node_id_t, string> id_number_to_nodeid;

	/**
	 * The next id number which is free to be used.
	*/
	bubble_node_id_t next_id_number;
	
	/**
	 * This is the local forwarding metrics table.
	*/
	bubble_rib_t rib;

	Timeval rib_timestamp;
	
	/**
	 * getSaveState function gets saved forwarding metrics table
	*/
	size_t getSaveState(RepositoryEntryList& rel);

	/**
	 * getSaveState function saves forwarding metrics table
	*/
	bool setSaveState(RepositoryEntryRef& e);
	
	/**
	 * id_from_string function get the bubble_node_id_t from the NodeStringId.
	 * If the bubble_node_id_t wasn't in the map to begin with,
	 * it is inserted, along with a new id number.
	*/
	bubble_node_id_t id_from_string(const string& nodeid);
	
    /**
            The newRoutingInformation function is used when a data object has come in that has a "Routing" attribute.

            Also called for each such data object that is in the data store on startup.

            Since the format of the data in such a data object is unknown to the
            forwarding manager, it is up to the forwarder to make sure the data is
            in the correct format.

            Also, the given metric data object may have been sent before, due to
            limitations in the forwarding manager.
    */
	bool newRoutingInformation(const Metadata *m);
	

    /**
            The addRoutingInformation function is used to generate the
            Metadata containing routing information which is specific
            for that forwarding module.
     */
	bool addRoutingInformation(DataObjectRef& dObj, Metadata *parent);

    /**
            The _newNeighbor is called when a neighbor node is discovered.
    */
	void _newNeighbor(const NodeRef &neighbor);

    /**
            The _endNeighbor called when a node just ended being a neighbor.
    */
	void _endNeighbor(const NodeRef &neighbor);
	
    /**
            The _generateTargetsFor function generates an EVENT_TYPE_TARGET_NODES event
            to provide all the target nodes that the given node is a good delegate forwarder for.

            This function is given a current neighbor, and answers the question:
            For which nodes is the given node a good delegate forwarder?

            If no nodes are found, no event should be created.
    */
	void _generateTargetsFor(const NodeRef &neighbor);
	
    /**
            The _generateDelegatesFor function generates an EVENT_TYPE_DELEGATE_NODES event
            to provide all the nodes that are good delegate forwarders for the given node.

            This function is given a target to which to send a data object, and
            answers the question: To which delegate forwarders can I send the given
            data object, so that it will reach the given target?

            If no nodes are found, no event should be created.
    */
	void _generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);

#ifdef DEBUG
	/**
		The _printRoutingTable function prints the routing table in debug mode.
	*/
	void _printRoutingTable(void);
#endif

	/**
		The _onForwarderConfig function reads the configuration from config.xml file.
	*/
	void _onForwarderConfig(const Metadata& m);

public:
	ForwarderProphet(ForwardingManager *m = NULL, const EventType type = -1);
	~ForwarderProphet();
};
#endif
