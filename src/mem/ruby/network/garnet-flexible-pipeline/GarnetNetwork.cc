/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * GarnetNetwork.cc
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "mem/ruby/network/garnet-flexible-pipeline/GarnetNetwork.hh"
#include "mem/protocol/MachineType.hh"
#include "mem/ruby/network/garnet-flexible-pipeline/NetworkInterface.hh"
#include "mem/ruby/buffers/MessageBuffer.hh"
#include "mem/ruby/network/garnet-flexible-pipeline/Router.hh"
#include "mem/ruby/network/simple/Topology.hh"
#include "mem/ruby/network/simple/SimpleNetwork.hh"
#include "mem/ruby/network/garnet-fixed-pipeline/GarnetNetwork_d.hh"
#include "mem/ruby/network/garnet-flexible-pipeline/NetworkLink.hh"
#include "mem/ruby/common/NetDest.hh"

// calls new to abstract away from the network
Network* Network::createNetwork(int nodes)
{
        NetworkConfig::readNetConfig();
        // Instantiate a network depending on what kind of network is requested
        if(NetworkConfig::isGarnetNetwork())
        {
                if(NetworkConfig::isDetailNetwork())
                        return new GarnetNetwork_d(nodes);
                else
                        return new GarnetNetwork(nodes);
        }
        else
                return new SimpleNetwork(nodes);
}

GarnetNetwork::GarnetNetwork(int nodes)
{
        m_nodes = MachineType_base_number(MachineType_NUM); // Total nodes in network
        m_virtual_networks = NUMBER_OF_VIRTUAL_NETWORKS; // Number of virtual networks = number of message classes in the coherence protocol
        m_ruby_start = 0;

        // Allocate to and from queues
        m_toNetQueues.setSize(m_nodes);         // Queues that are getting messages from protocol
        m_fromNetQueues.setSize(m_nodes);       // Queues that are feeding the protocol
        m_in_use.setSize(m_virtual_networks);
        m_ordered.setSize(m_virtual_networks);
        for (int i = 0; i < m_virtual_networks; i++)
        {
                m_in_use[i] = false;
                m_ordered[i] = false;
        }

        for (int node = 0; node < m_nodes; node++)
        {
                //Setting how many vitual message buffers will there be per Network Queue
                m_toNetQueues[node].setSize(m_virtual_networks);
                m_fromNetQueues[node].setSize(m_virtual_networks);

                for (int j = 0; j < m_virtual_networks; j++)
                {
                        m_toNetQueues[node][j] = new MessageBuffer();   // Instantiating the Message Buffers that interact with the coherence protocol
                        m_fromNetQueues[node][j] = new MessageBuffer();
                }
        }

        // Setup the network switches
        m_topology_ptr = new Topology(this, m_nodes);

        int number_of_routers = m_topology_ptr->numSwitches();
        for (int i=0; i<number_of_routers; i++) {
                m_router_ptr_vector.insertAtBottom(new Router(i, this));
        }

        for (int i=0; i < m_nodes; i++) {
                NetworkInterface *ni = new NetworkInterface(i, m_virtual_networks, this);
                ni->addNode(m_toNetQueues[i], m_fromNetQueues[i]);
                m_ni_ptr_vector.insertAtBottom(ni);
        }
        m_topology_ptr->createLinks(false);  // false because this isn't a reconfiguration
}

GarnetNetwork::~GarnetNetwork()
{
        for (int i = 0; i < m_nodes; i++)
        {
                m_toNetQueues[i].deletePointers();
                m_fromNetQueues[i].deletePointers();
        }
        m_router_ptr_vector.deletePointers();
        m_ni_ptr_vector.deletePointers();
        m_link_ptr_vector.deletePointers();
        delete m_topology_ptr;
}

void GarnetNetwork::reset()
{
        for (int node = 0; node < m_nodes; node++)
        {
                for (int j = 0; j < m_virtual_networks; j++)
                {
                        m_toNetQueues[node][j]->clear();
                        m_fromNetQueues[node][j]->clear();
                }
        }
}

void GarnetNetwork::makeInLink(NodeID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration)
{
        assert(src < m_nodes);

        if(!isReconfiguration)
        {
                NetworkLink *net_link = new NetworkLink(m_link_ptr_vector.size(), link_latency, this);
                m_link_ptr_vector.insertAtBottom(net_link);
                m_router_ptr_vector[dest]->addInPort(net_link);
                m_ni_ptr_vector[src]->addOutPort(net_link);
        }
        else
        {
                ERROR_MSG("Fatal Error:: Reconfiguration not allowed here");
                // do nothing
        }
}

void GarnetNetwork::makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration)
{
        assert(dest < m_nodes);
        assert(src < m_router_ptr_vector.size());
        assert(m_router_ptr_vector[src] != NULL);

        if(!isReconfiguration)
        {
                NetworkLink *net_link = new NetworkLink(m_link_ptr_vector.size(), link_latency, this);
                m_link_ptr_vector.insertAtBottom(net_link);
                m_router_ptr_vector[src]->addOutPort(net_link, routing_table_entry, link_weight);
                m_ni_ptr_vector[dest]->addInPort(net_link);
        }
        else
        {
                ERROR_MSG("Fatal Error:: Reconfiguration not allowed here");
                //do nothing
        }
}

void GarnetNetwork::makeInternalLink(SwitchID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration)
{
        if(!isReconfiguration)
        {
                NetworkLink *net_link = new NetworkLink(m_link_ptr_vector.size(), link_latency, this);
                m_link_ptr_vector.insertAtBottom(net_link);
                m_router_ptr_vector[dest]->addInPort(net_link);
                m_router_ptr_vector[src]->addOutPort(net_link, routing_table_entry, link_weight);
        }
        else
        {
                ERROR_MSG("Fatal Error:: Reconfiguration not allowed here");
                // do nothing
        }

}

void GarnetNetwork::checkNetworkAllocation(NodeID id, bool ordered, int network_num)
{
        ASSERT(id < m_nodes);
        ASSERT(network_num < m_virtual_networks);

        if (ordered)
        {
                m_ordered[network_num] = true;
        }
        m_in_use[network_num] = true;
}

MessageBuffer* GarnetNetwork::getToNetQueue(NodeID id, bool ordered, int network_num)
{
        checkNetworkAllocation(id, ordered, network_num);
        return m_toNetQueues[id][network_num];
}

MessageBuffer* GarnetNetwork::getFromNetQueue(NodeID id, bool ordered, int network_num)
{
        checkNetworkAllocation(id, ordered, network_num);
        return m_fromNetQueues[id][network_num];
}

void GarnetNetwork::clearStats()
{
        m_ruby_start = g_eventQueue_ptr->getTime();
}

Time GarnetNetwork::getRubyStartTime()
{
        return m_ruby_start;
}

void GarnetNetwork::printStats(ostream& out) const
{       double average_link_utilization = 0;
        Vector<double > average_vc_load;
        average_vc_load.setSize(m_virtual_networks*NetworkConfig::getVCsPerClass());

        for(int i = 0; i < m_virtual_networks*NetworkConfig::getVCsPerClass(); i++)
        {
                average_vc_load[i] = 0;
        }

        out << endl;
        out << "Network Stats" << endl;
        out << "-------------" << endl;
        out << endl;
        for(int i = 0; i < m_link_ptr_vector.size(); i++)
        {
                average_link_utilization += m_link_ptr_vector[i]->getLinkUtilization();
                Vector<int > vc_load = m_link_ptr_vector[i]->getVcLoad();
                for(int j = 0; j < vc_load.size(); j++)
                {
                        assert(vc_load.size() == NetworkConfig::getVCsPerClass()*m_virtual_networks);
                        average_vc_load[j] += vc_load[j];
                }
        }
        average_link_utilization = average_link_utilization/m_link_ptr_vector.size();
        out << "Average Link Utilization :: " << average_link_utilization << " flits/cycle" <<endl;
        out << "-------------" << endl;

        for(int i = 0; i < NetworkConfig::getVCsPerClass()*m_virtual_networks; i++)
        {
                average_vc_load[i] = (double(average_vc_load[i]) / (double(g_eventQueue_ptr->getTime()) - m_ruby_start));
                out << "Average VC Load [" << i << "] = " << average_vc_load[i] << " flits/cycle" << endl;
        }
        out << "-------------" << endl;
}

void GarnetNetwork::printConfig(ostream& out) const
{
        out << endl;
        out << "Network Configuration" << endl;
        out << "---------------------" << endl;
        out << "network: GARNET_NETWORK" << endl;
        out << "topology: " << g_NETWORK_TOPOLOGY << endl;
        out << endl;

        for (int i = 0; i < m_virtual_networks; i++)
        {
                out << "virtual_net_" << i << ": ";
                if (m_in_use[i])
                {
                        out << "active, ";
                        if (m_ordered[i])
                        {
                                out << "ordered" << endl;
                        }
                        else
                        {
                                out << "unordered" << endl;
                        }
                }
                else
                {
                        out << "inactive" << endl;
                }
        }
        out << endl;

        for(int i = 0; i < m_ni_ptr_vector.size(); i++)
        {
                m_ni_ptr_vector[i]->printConfig(out);
        }
        for(int i = 0; i < m_router_ptr_vector.size(); i++)
        {
                m_router_ptr_vector[i]->printConfig(out);
        }
        if (g_PRINT_TOPOLOGY)
        {
                m_topology_ptr->printConfig(out);
        }
}

void GarnetNetwork::print(ostream& out) const
{
        out << "[GarnetNetwork]";
}
