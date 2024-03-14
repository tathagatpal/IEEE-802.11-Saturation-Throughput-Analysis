//Analysis of throughput vs data rate
//Authors: Thatagat Pal and Kelvin Arana

//Header Files
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/command-line.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/ssid.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/gnuplot.h"
#include "ns3/txop.h"
#include <sys/stat.h>
//#include "gnuplot-iostream.h"
#include <iostream>
#include <fstream>
#include <vector>  
#include <filesystem>

// Network Topology
//
//   Wifi 10.1.1.0
// Rcv              
//  *    *    *    *    *    *    *  		*
//  |    |    |    |    |    |    |  ---------	| 
// n0   n1   n2   n3   n4   n5   n6 		nN
// For this topology every Node transmismitting will be separated
// diagonally (increasing in x and y) by a value of 1

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("Wifi");

int main (int argc, char *argv[])
    {
    char ip[10] = "10.1.1.0";    
    uint32_t nWifi = 20; //Fixed Value
    uint64_t cfpMaxDurationUs = 65536; //microseconds
    uint32_t cwmin=1; //Min Contention Window
    uint32_t cwmax=1023; //Max Contention Window
    double simulationTime = 1; //seconds
    int count = 0;
    int max_it = 10;
    int stepsize = 10; 
    double R = 20;	//Rate in Mbps
    double throughput[max_it] = {};
    double pernodeThroughput[max_it] = {};
    uint32_t numNodes[max_it] = {};
    uint32_t rate_val[max_it] = {};
    numNodes[0]= nWifi;
    rate_val[0]=R;

    CommandLine cmd (__FILE__);
    //cmd.AddValue ("nWifi", "Intitial number of wifi Transmitter Nodes", nWifi);
    cmd.AddValue ("R", "Rate in Mbps", R);
    cmd.AddValue ("stepsize", "Increment in the number of wifi Transmitter Nodes",stepsize);
    cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue ("cwmin", "Backoff Window Min", cwmin);
    cmd.AddValue ("cwmax", "Backoff Window Max", cwmax);
    cmd.Parse (argc, argv);

    std::string fileNameWithNoExtension1 = "Node_Throughput_Plot";
    std::string fileNameWithNoExtension2 = "NumPerNodePlot";
    std::string graphicsFileName1        = fileNameWithNoExtension1 + ".png";
    std::string graphicsFileName2        = fileNameWithNoExtension2 + ".png";
    std::string plotFileName1            = fileNameWithNoExtension1 + ".plt";
    std::string plotFileName2            = fileNameWithNoExtension2 + ".plt";
   
 std::cout << "-----Initial Simulation Values----\n"<< std::endl;
 std::cout << "Number of Nodes:" << nWifi << std::endl;
 std::cout << "Given Min Backoff Window :" <<cwmin << std::endl;
 std::cout << "Given Max Backoff Window :" <<cwmax << std::endl;
 std::cout << "Initial Rate:" << R << " Mbps" << std::endl;
 std::cout << "Given Stepsize:" << stepsize << std::endl;
 std::cout << "Given Simulation Time:" <<simulationTime << std::endl;
 std::cout << "--------------------------"<< std::endl;

    while (count<max_it)
        {
        std::cout<<"Simulation Values\n"<<std::endl;
        //Create Nodes that will send data
        NodeContainer wifiStaNodes;
        wifiStaNodes.Create (nWifi);

        // RX AP that will get data from n Nodes
        NodeContainer wifiApNode;
        wifiApNode.Create (1);
        
        // Physical Layer Definition
        YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
        YansWifiPhyHelper phy; //= YansWifiPhyHelper::Default ();
        phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.SetChannel (channel.Create ());

        WifiHelper wifi;
	    wifi.SetStandard (WIFI_STANDARD_80211p);
        WifiMacHelper mac;

        Ssid ssid = Ssid ("wifi");
        wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"), "ControlMode", StringValue ("OfdmRate24Mbps"));

        NetDeviceContainer staDevices;
        mac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid));
                     staDevices = wifi.Install (phy, mac, wifiStaNodes);

        mac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "CfpMaxDuration", TimeValue (MicroSeconds (cfpMaxDurationUs)));

        NetDeviceContainer apDevice;
        apDevice = wifi.Install (phy, mac, wifiApNode);

        //Defintion of Mobility
        MobilityHelper mobility;

        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

        //Loop for Postion of N transmitters
        for (uint32_t i = 0; i < nWifi; i++)
            {
            positionAlloc->Add (Vector (i, i, 0.0)); //TX will diagonally get apart
            }
        positionAlloc->Add (Vector (0.0, 0.0, 0.0)); //RX Position


        mobility.SetPositionAllocator (positionAlloc);

        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

        mobility.Install (wifiApNode);
        mobility.Install (wifiStaNodes);

        //Install of Internet Stack
        InternetStackHelper stack;
        stack.Install (wifiApNode);
        stack.Install (wifiStaNodes);

        //Assignement of IP Address
        Ipv4AddressHelper address;
        ip[5] = char(ip[5]+count);
        address.SetBase (ip, "255.255.255.0");
        Ipv4InterfaceContainer StaInterface;
        StaInterface = address.Assign (staDevices);
        Ipv4InterfaceContainer ApInterface;
        ApInterface = address.Assign (apDevice);


        ApplicationContainer sourceApplications, sinkApplications;
               
            uint32_t portNumber = 9;
            for (uint32_t index = 0; index < nWifi; ++index)
                {
                auto ipv4 = (true) ? wifiApNode.Get (0)->GetObject<Ipv4> () : wifiStaNodes.Get (index)->GetObject<Ipv4> ();
                const auto address = ipv4->GetAddress (1, 0).GetLocal ();
                InetSocketAddress sinkSocket (address, portNumber++);
                OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkSocket);
                onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
                onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
                onOffHelper.SetAttribute ("DataRate", DataRateValue (R*1000000)); //Mbps
                onOffHelper.SetAttribute ("PacketSize", UintegerValue (512)); //bytes
                PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
                
                    sourceApplications.Add (onOffHelper.Install (wifiStaNodes.Get (index)));
                    sinkApplications.Add (packetSinkHelper.Install (wifiApNode.Get (0)));
                    
                }
            sinkApplications.Start (Seconds (0.0));
            sinkApplications.Stop (Seconds (simulationTime + 1));
            sourceApplications.Start (Seconds (1.0));
            sourceApplications.Stop (Seconds (simulationTime + 1));
            
            // Pcap for storing data
            //phy.EnablePcap ("Nodes_Throughtput_WiFi", apDevice.Get (0));
        

        Simulator::Stop (Seconds (simulationTime + 1));
        Simulator::Run ();

        //Calculation of Throughput
        for (uint32_t index = 0; index < sinkApplications.GetN (); ++index)
            {
            //Get total of packets
            uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApplications.Get (index))->GetTotalRx ();
            //Throughtput=Total_pkt/Time
            throughput[count] += ((totalPacketsThrough * 8) / (simulationTime * 1000000.0)); //Mbit/s
            }
        pernodeThroughput[count] = throughput[count]/nWifi;
        numNodes[count] = nWifi;
        rate_val[count]=R;
	
	    std::cout << "Number of Nodes:" << nWifi << std::endl;
        std::cout << "Throughput: " << throughput[count] << " Mbit/s" << std::endl;
        std::cout << "PerNodeThroughput: " << pernodeThroughput[count] << " Mbit/s" << std::endl;
        std::cout << "Rate: " << rate_val[count] << " Mbit/s" << std::endl;

        R+=stepsize;
        count+=1;  
        }
    Simulator::Destroy () ;

    ////Plotting Area
    
    // Set directory path to save the PNG file
    //This has to be modified to run properlo in other PC
    std::string plotDirectory = "/home/kelvin/ns-allinone-3.33/ns-3.33/";
    
    // Specify the folder name
    std::string folderName = "Plots_Simulation_Rate";

    // Combine the directory path with the folder name
    std::string fullDirectoryPath = plotDirectory + folderName + "/";

   // Create the folder if it doesn't exist
   
    if (mkdir(fullDirectoryPath.c_str(), 0777) != 0) {
        std::cerr << "Error creating directory." << std::endl;
        //return 1;
    }
    
    // Combine the directory path with the file name and .plt extension
    std::string FilePath = fullDirectoryPath + "Rate-Throughtput.plt";
    
    //Plot 1 Node vs Throughput
    // Instantiate the plot and set its title.
    Gnuplot plot1(FilePath);
    plot1.SetTitle("Rate vs Thoughput");

    // Make the graphics file, which the plot file will create when it
    // is used with Gnuplot, be a PNG file.
    plot1.SetTerminal("pdf");

    // Set the labels for each axis.
    plot1.SetLegend("Rate", "Throughput (Mbps)");

    // Set the Grid for x-axis and y-axis
    plot1.AppendExtra("set xtics nomirror");
    plot1.AppendExtra("set grid xtics");
    plot1.AppendExtra("set ytics nomirror");
    plot1.AppendExtra("set grid ytics");

    // Instantiate the dataset, set its title, and make the points be
    // plotted along with connecting lines.

    Gnuplot2dDataset dataset1;
    dataset1.SetTitle("Throughtput");
    dataset1.SetStyle(Gnuplot2dDataset::LINES_POINTS);
    for(int i=0; i<max_it;i++)
        {
            dataset1.Add(rate_val[i], throughput[i]);
        }

    // Add the dataset to the plot.
    plot1.AddDataset(dataset1);
    // Open the plot file.
    std::ofstream plotFile1(FilePath.c_str());
    // Write the plot file.
    plot1.GenerateOutput(plotFile1);
    
    // Close the plot file.
    plotFile1.close();
    // Run Gnuplot with the generated plot file.
    std::string gnuplotCommand = "gnuplot " + FilePath;
    system(gnuplotCommand.c_str());
    
    
    //Plot 2 -- Troughput per Node
    // Combine the directory path with the file name and .plt extension
    std::string FilePath2 = fullDirectoryPath + "ThroughputPerNode.plt";
    // Instantiate the plot and set its title.
    Gnuplot plot2(FilePath2);
    plot2.SetTitle("Throughput Per Node Simulation");

    // Make the graphics file, which the plot file will create when it
    // is used with Gnuplot, be a PNG file.
    plot2.SetTerminal("pdf");

    // Set the labels for each axis.
    plot2.SetLegend("Node", "Throughput per Node (Mbps)");

    // Set the Grid for x-axis and y-axis
    plot2.AppendExtra("set xtics nomirror");
    plot2.AppendExtra("set grid xtics");
    plot2.AppendExtra("set ytics nomirror");
    plot2.AppendExtra("set grid ytics");

    // Instantiate the dataset, set its title, and make the points be
    // plotted along with connecting lines.

    Gnuplot2dDataset dataset2;
    dataset2.SetTitle("Throughput per Node");
    dataset2.SetStyle(Gnuplot2dDataset::LINES_POINTS);
    for(int i=0; i<max_it;i++)
        {
        dataset2.Add(numNodes[i], pernodeThroughput[i]);
        }

    // Add the dataset to the plot.
    plot2.AddDataset(dataset2);

    // Open the plot file.
    std::ofstream plotFile2(FilePath2.c_str());

    // Write the plot file.
    plot2.GenerateOutput(plotFile2);

    // Close the plot file.
    plotFile2.close();
     // Run Gnuplot with the generated plot file.
    std::string gnuplotCommand2 = "gnuplot " + FilePath2;
    system(gnuplotCommand2.c_str());
    

    return 0;
    }
