#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/spectrum-module.h"
#include <chrono>

using namespace std::chrono;
using namespace ns3;
//const uint64_t frequency = 5180;

NS_LOG_COMPONENT_DEFINE ("DualApStaSpectrumChannel");

void RunSimulation(bool useSpectrum, std::string modelName) {
    // Enable logging for useful components
    //LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    //LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create nodes for APs and STAs
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (2);
    NodeContainer wifiApNodes;
    wifiApNodes.Create (2);

    // Configure the Wi-Fi channel and PHY layer
    Ptr<SpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(5.180e6);
    spectrumChannel->AddPropagationLossModel(lossModel);

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
    CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);
    
    SpectrumWifiPhyHelper spectrumPhy;
    spectrumPhy.SetChannel(spectrumChannel);
    spectrumPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    //spectrumPhy.Set("TxPowerStart", DoubleValue(1)); // dBm  (1.26 mW)
    //spectrumPhy.Set("TxPowerEnd", DoubleValue(1));
    
    YansWifiChannelHelper yansChannel = YansWifiChannelHelper::Default ();
    yansChannel.AddPropagationLoss("ns3::FriisPropagationLossModel",
                                "Frequency",
                                DoubleValue(5.180e6));
    yansChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    YansWifiPhyHelper yansPhy;
    yansPhy.SetChannel(yansChannel.Create ());
    //yansPhy.Set("TxPowerStart", DoubleValue(1)); // dBm (1.26 mW)
    //yansPhy.Set("TxPowerEnd", DoubleValue(1));

    // Configure Wi-Fi standard and setup
    WifiHelper wifi;
    //wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
    wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns3-ssid");

    // Configure STA devices
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices;
    if (useSpectrum) {
        staDevices.Add(wifi.Install (spectrumPhy, mac, wifiStaNodes.Get(0)));
        staDevices.Add(wifi.Install (spectrumPhy, mac, wifiStaNodes.Get(1)));
    } else {
        staDevices.Add(wifi.Install (yansPhy, mac, wifiStaNodes.Get(0)));
        staDevices.Add(wifi.Install (yansPhy, mac, wifiStaNodes.Get(1)));
    }

    // Configure AP devices
    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices;
    if (useSpectrum) {
        apDevices.Add(wifi.Install (spectrumPhy, mac, wifiApNodes.Get(0)));
        apDevices.Add(wifi.Install (spectrumPhy, mac, wifiApNodes.Get(1)));
    } else {
        apDevices.Add(wifi.Install (yansPhy, mac, wifiApNodes.Get(0)));
        apDevices.Add(wifi.Install (yansPhy, mac, wifiApNodes.Get(1)));
    }

    // Install the Internet stack
    InternetStackHelper stack;
    stack.Install (wifiStaNodes);
    stack.Install (wifiApNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase ("10.10.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterfaces = address.Assign (apDevices);
    Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);

    // Set up mobility
    /*
    MobilityHelper mobilityAP;
    mobilityAP.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (0.0),
                                    "MinY", DoubleValue (0.0),
                                    "DeltaX", DoubleValue (10.0),
                                    "DeltaY", DoubleValue (0.0),
                                    "GridWidth", UintegerValue (3),
                                    "LayoutType", StringValue ("RowFirst"));

    mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityAP.Install (wifiApNodes);

    MobilityHelper mobilitySTA;
    mobilitySTA.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue(Rectangle(-10, 10, -10, 10)));
    mobilitySTA.Install(wifiStaNodes);
*/
// Mobility configuration

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0)); // AP 1 position
    positionAlloc->Add (Vector (5.0, 0.0, 0.0)); // AP 2 position
    positionAlloc->Add (Vector (0.0, 1.0, 0.0)); // STA 1 position
    positionAlloc->Add (Vector (5.0, 1.0, 0.0)); // STA 2 position
    mobility.SetPositionAllocator (positionAlloc);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (wifiApNodes);
    mobility.Install (wifiStaNodes);

    UdpEchoClientHelper echoClient1 (apInterfaces.GetAddress (0), 9);
    //echoClient1.SetAttribute ("MaxPackets", UintegerValue (1000));
    echoClient1.SetAttribute ("Interval", TimeValue (MilliSeconds (5)));
    echoClient1.SetAttribute ("PacketSize", UintegerValue (10000));
    ApplicationContainer clientApps1 = echoClient1.Install (wifiStaNodes.Get (0));

    UdpEchoClientHelper echoClient2 (apInterfaces.GetAddress (1), 9);
    //echoClient2.SetAttribute ("MaxPackets", UintegerValue (1000));
    echoClient2.SetAttribute ("Interval", TimeValue (MilliSeconds (13)));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (10000));
    ApplicationContainer clientApps2 = echoClient2.Install (wifiStaNodes.Get (1));

    // Enable Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop (Seconds (40.0));

    auto start = high_resolution_clock::now();
    Simulator::Run ();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    // Print Flow Monitor statistics
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    std::cout << "\n Results for " << modelName << " model:" << std::endl;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
        {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1024 / 1024  << " Mbps\n";
        std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
        std::cout << "  Delay: " << i->second.delaySum.GetSeconds () / i->second.rxPackets << "\n";
        }
    std::cout << "Simulation time: " << duration.count() << " microseconds\n\n " << std::endl;

    // Clean up
    Simulator::Destroy ();
}

int main (int argc, char *argv[])
{
    RunSimulation(false, "Yans");
    RunSimulation(true, "Spectrum");
    return 0;
}