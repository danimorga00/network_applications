#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
// n2 -------------- n3
//    point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FirstScriptExample");          

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);                          
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);                      
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer nodes;                                
    nodes.Create(4);                                    

    NS_LOG_INFO("aggiunti 2 nodi");                   

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices01;
    devices01 = pointToPoint.Install (nodes.Get(0), nodes.Get(1));

    NetDeviceContainer devices23;
    devices23 = pointToPoint.Install (nodes.Get(2), nodes.Get(3));

    InternetStackHelper stack;                          
    stack.Install(nodes);                               

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces01 = address.Assign (devices01);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces23 = address.Assign (devices23);

    uint16_t port1 = 9; 
    Address serverAddress1 (InetSocketAddress (interfaces01.GetAddress (1), port1));
    PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", serverAddress1);
    ApplicationContainer sinkApps1 = packetSinkHelper1.Install (nodes.Get (1));
    sinkApps1.Start (Seconds (1.0));
    sinkApps1.Stop (Seconds (10.0));

    OnOffHelper client1 ("ns3::TcpSocketFactory", serverAddress1);
    client1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    client1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    client1.SetAttribute ("DataRate", DataRateValue (DataRate ("5Mbps")));
    client1.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps1 = client1.Install (nodes.Get (0));
    clientApps1.Start (Seconds (2.0));
    clientApps1.Stop (Seconds (10.0));

    uint16_t port2 = 10; 
    Address serverAddress2 (InetSocketAddress (interfaces23.GetAddress (1), port2));
    PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", serverAddress2);
    ApplicationContainer sinkApps2 = packetSinkHelper2.Install (nodes.Get (3));
    sinkApps2.Start (Seconds (1.0));
    sinkApps2.Stop (Seconds (10.0));

    OnOffHelper client2 ("ns3::TcpSocketFactory", serverAddress2);
    client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    client2.SetAttribute ("DataRate", DataRateValue (DataRate ("5Mbps")));
    client2.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps2 = client2.Install (nodes.Get (2));
    clientApps2.Start (Seconds (2.0));
    clientApps2.Stop (Seconds (10.0));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds (40.0));
    Simulator::Run();
    
    flowmon->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
        {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        NS_LOG_UNCOND ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
        NS_LOG_UNCOND ("  Tx Bytes:   " << i->second.txBytes);
        NS_LOG_UNCOND ("  Rx Bytes:   " << i->second.rxBytes);
        NS_LOG_UNCOND ("  Throughput: " << i->second.rxBytes * 8.0 / 9 / 1024 / 1024  << " Mbps");
        NS_LOG_UNCOND ("  RTT: " << i->second.delaySum.GetSeconds () / i->second.rxPackets);
        std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
        std::cout << "  Delay: " << i->second.delaySum.GetSeconds () / i->second.rxPackets << "\n";
        }

    Simulator::Destroy();
    return 0;
}