#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/lte-rrc-sap.h"
#include "ns3/friis-spectrum-propagation-loss.h"
// sns3-satellite related headers
#include <ns3/satellite-helper.h>
#include <ns3/satellite-stats-helper-container.h>
#include <ns3/satellite-enums.h>

#include "ns3/rng-seed-manager.h"
#include "ns3/random-variable-stream.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("SatForEpcExample");

static void 
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();

  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << foo
            << "\tposx:\t"<< pos.x
            << "\tposy:\t" << pos.y
            << "\tposz:\t" << pos.z
            << "\tvelx\t" << vel.x
            << "\tvely\t" << vel.y
            << "\tvelz\t" << vel.z
            << std::endl ;
}

static void 
TraceCourseChange(void)
{
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));
}

static const double SpectralEfficiencyForMcs[32] = {
  0.15, 0.19, 0.23, 0.31, 0.38, 0.49, 0.6, 0.74, 0.88, 1.03, 1.18,
  1.33, 1.48, 1.7, 1.91, 2.16, 2.41, 2.57,
  2.73, 3.03, 3.32, 3.61, 3.9, 4.21, 4.52, 4.82, 5.12, 5.33, 5.55,
  0, 0, 0
};

static const double SpectralEfficiencyForCqi[16] = {
  0.0, // out of range
  0.15, 0.23, 0.38, 0.6, 0.88, 1.18,
  1.48, 1.91, 2.41,
  2.73, 3.32, 3.9, 4.52, 5.12, 5.55
};

// 拥塞窗口的打印信息
static void
CwndChange(std::string node , uint32_t oldval , uint32_t newval)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\toldcwnd\t" << oldval
              << "\tnewcwnd\t" << newval
              << std::endl ;
}

void 
CwndChange2(Ptr<OutputStreamWrapper> stream , std::string node , uint32_t oldval , uint32_t newval)
{
    
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\toldcwnd\t" << oldval
              << "\tnewcwnd\t" << newval
              << std::endl ;
     *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << node << "\t"  << oldval << "\t" << newval <<  std::endl;
}


static void 
TraceCwnd(void)
{
    Config::Connect("/NodeList/4/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow" , MakeCallback(&CwndChange)) ;
}

static void
TraceCwnd2(void)
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("cwnd.tr");
    Config::ConnectFailSafe("/NodeList/14/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow" , MakeBoundCallback(&CwndChange2 , stream)) ;
}

// Rtt 打印信息
static void
RttChange(std::string node, Time oldval , Time newval)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << node
            << "\toldrtt\t" << oldval.GetSeconds()
            << "\tnewrtt\t" << newval.GetSeconds()
            << std::endl ;
}

static void 
TraceRTT(void)
{
  Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/RTT" , MakeCallback(&RttChange));
}


// 打印丢包信息
static void
DropChange(Ptr<const Packet> packet)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tdrop\t"
            << packet->GetSize()
            << "\tbytes\t"
            << std::endl ;
}


static void
RxChange(std::string content ,Ptr<const Packet> p , const Address & address)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds() 
            << "\tnode\t"<< content 
            << "\trxsize\t" << p->GetSize() 
            << std::endl ;
}

static void
BytesInFlightChange(std::string node , uint32_t oldValue, uint32_t newValue)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds() 
            << "\tnode\t" << node 
            << "\toldbytesInFlight\t" << oldValue
            << "\tnewbytesInFlight\t" << newValue
            << std::endl ; 
}

static void
TraceBytesInFlight(void)
{
  Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/BytesInFlight" , MakeCallback(&BytesInFlightChange) ) ;
}

static void
TraceRx(void)
{
  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",MakeCallback(&RxChange)) ;
}



static void
ReportCurrentCellRsrpSinrChange(std::string node , uint16_t cellId , uint16_t rnti , double rsrp , double sinr , uint8_t componentCarrier)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\tcellId\t" << (uint32_t) cellId
              << "\trnti\t" << (uint32_t) rnti
              << "\trsrp\t" << rsrp
              << "\tsinr\t" << sinr
              << "\tcomponentCarrier\t" << (uint32_t) componentCarrier 
              << std::endl ;
}
static void
TraceReportCurrentCellRsrpSinr(void)
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/$ns3::LteUeNetDevice/ComponentCarrierMapUe/*/LteUePhy/ReportCurrentCellRsrpSinr",MakeCallback(&ReportCurrentCellRsrpSinrChange));
}

// 将用户节点 与 RNTI 相结合看下
static void
Notify(NetDeviceContainer ueLteDevs , NodeContainer ueNodes , NodeContainer enbNodes , Ptr<Node> pgw)
{
    for(int i = 0 ; i < ueLteDevs.GetN() ; i ++)
    {
        Ptr<NetDevice> ue_device = ueLteDevs.Get(i) ;
        Ptr<LteUeRrc> ueRrc = ue_device->GetObject<LteUeNetDevice> ()->GetRrc ();
        uint16_t rnti = ueRrc->GetRnti ();
        std::cout <<"node-rnti\t"<< "\tnode id\t" << ueNodes.Get(i)->GetId() << "\trnti\t" << (uint32_t) rnti << std::endl ;
    }

    for(int i = 0 ; i < enbNodes.GetN() ; i ++)
    {
      std::cout << "enb nodeid\t" << enbNodes.Get(i)->GetId() << std::endl ;
    }

    std::cout <<"pgw nodid=\t" << pgw->GetId() << std::endl ;
}

int
GetCqiFromSpectralEfficiency (double s)
{
  NS_LOG_FUNCTION (s);
  NS_ASSERT_MSG (s >= 0.0, "negative spectral efficiency = " << s);
  int cqi = 0;
  while ((cqi < 15) && (SpectralEfficiencyForCqi[cqi + 1] < s))
    {
      ++cqi;
    }
  NS_LOG_LOGIC ("cqi = " << cqi);
  return cqi;
}

// 这个应该是下行用户
static void
UlPhytransmissionChange(uint32_t nodeid , const PhyTransmissionStatParameters params)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds() 
            << "\tnode\t" << nodeid
            << "\tccid\t" << (uint32_t)params.m_ccId
            << "\tcellId\t" << (uint32_t) params.m_cellId
            << "\timsi\t" << (uint32_t) params.m_imsi
            << "\tlayer\t" << (uint32_t) params.m_layer
            << "\tmcs\t" << (uint32_t) params.m_mcs << "\t" ;
  double efficiency = SpectralEfficiencyForMcs[(int)params.m_mcs] ;
  int cqi = GetCqiFromSpectralEfficiency(efficiency) ;

  std::cout << "cqi\t" << cqi 
            << "\trnti\t" << (uint32_t) params.m_rnti
            << "\tm_ndi\t" << (uint32_t) params.m_ndi
            << "\trv\t" << (uint32_t) params.m_rv
            << "\tsize\t" << (uint32_t) params.m_size
            << "\tm_timestamp\t" <<(double) params.m_timestamp/1000.0
            << "\tm_txmode\t" << (uint32_t)params.m_txMode
            << "\tUlPhytransmission\t"
            << std::endl ;
}

static void
DlPhytransmissionChange(uint32_t nodeid , const PhyTransmissionStatParameters params)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds() 
            << "\tnode\t" << nodeid
            << "\tccid\t" << (uint32_t)params.m_ccId
            << "\tcellId\t" << (uint32_t) params.m_cellId
            << "\timsi\t" << (uint32_t) params.m_imsi
            << "\tlayer\t" << (uint32_t) params.m_layer
            << "\tmcs\t" << (uint32_t) params.m_mcs << "\t" ;
  double efficiency = SpectralEfficiencyForMcs[(int)params.m_mcs] ;
  int cqi = GetCqiFromSpectralEfficiency(efficiency) ;

  std::cout << "cqi\t" << cqi 
            << "\trnti\t" << (uint32_t) params.m_rnti
            << "\tm_ndi\t" << (uint32_t) params.m_ndi
            << "\trv\t" << (uint32_t) params.m_rv
            << "\tsize\t" << (uint32_t) params.m_size
            << "\tm_timestamp\t" <<(double) params.m_timestamp/1000.0
            << "\tm_txmode\t" << (uint32_t)params.m_txMode
            << "\tDlPhytransmission\t"
            << std::endl ;
}




static void
TxChange(std::string node , Ptr<const Packet> p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << node
            << "\tbulksend\t" << p->GetSize()
            << std::endl ;
}

static void
TraceBulkSend(void)
{
  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::BulkSendApplication/Tx" , MakeCallback(&TxChange)) ;
}

static void
ReportInterferenceChange(std::string node , uint16_t cellId, Ptr< SpectrumValue > spectrumValue)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << node
            << "\tcellId\t" << (uint32_t) cellId 
            <<"\tInterference\t" ;
  Values::const_iterator it = spectrumValue->ConstValuesBegin () ;
  for(; it != spectrumValue->ConstValuesEnd ()  ; it ++)
  {
      std::cout << (*it) << "\t" ;
  }
  std::cout <<"\t" << std::endl ;
}

static void TraceReportInterference(void)
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteEnbNetDevice/ComponentCarrierMap/*/LteEnbPhy/ReportInterference" , MakeCallback(&ReportInterferenceChange)) ;
}

static void
ReportUeSinrChange(std::string node , uint16_t cellId, uint16_t rnti, double sinrLinear, uint8_t componentCarrierId)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds() 
            << "\tReportUeSinr"
            << "\tnode\t" << node
            << "\tcellId\t" << (uint32_t)cellId
            << "\trnti\t" << (uint32_t) rnti
            << "\tsinrLinear\t" << sinrLinear
            << "\tcomponentCarrierId\t" << (uint32_t) componentCarrierId
            << std::endl ;
}

static void
TraceReportUeSinr(void)
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteEnbNetDevice/ComponentCarrierMap/*/LteEnbPhy/ReportUeSinr" , MakeCallback(&ReportUeSinrChange)) ;

}

static void
DlRxEndErrorChange(uint32_t nodeid , Ptr< const Packet > packet)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << nodeid
              << "\tDlUePhy Recv error\t" << packet->GetSize()
              << std::endl ;
}

static void
DlRxEndOkChange(uint32_t nodeid , Ptr< const Packet > packet)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << nodeid
              << "\tDlUePhy Recv ok\t" << packet->GetSize()
              << std::endl ;
}

static void
RxStartChange(uint32_t nodeid ,Ptr< const PacketBurst > burst)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << nodeid
            << "\tstart rx\t"
            << "\tpackets\t" << burst->GetNPackets()
            << "\tsize\t" << burst->GetSize()
            << std::endl ; 
}

static void
UlTxStartChange(uint32_t nodeid , Ptr< const PacketBurst > burst)
{
  if(burst)
  {
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\tTxStart\t"
            << "\tpackets\t" << burst->GetNPackets()
            << "\tsize\t" << burst->GetSize()
            << std::endl ;
  }
  
}

static void
UlTxEndChange(uint32_t nodeid , Ptr< const PacketBurst > burst)
{
  if(burst)
  {
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnodeid\t" << nodeid
              << "\tTxEnd\t"
              << "\tpackets\t" << burst->GetNPackets()
              << "\tsize\t" << burst->GetSize()
              << std::endl ;
  }
}

static void
DlPhyReceptionChange(uint32_t nodeid , const PhyReceptionStatParameters params)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << nodeid 
            << "\tDlPhyReception\t"
            << "\tcellId\t" << (uint32_t) params.m_cellId
            << "\timsi\t" << (uint32_t) params.m_imsi
            << "\trnti\t" << (uint32_t) params.m_rnti
            << "\tTBS\t" << (uint32_t) params.m_size
            << "\tmcs\t" << (uint32_t) params.m_mcs
            << "\tTB IsCorrect\t" << (uint32_t) params.m_correctness
            << std::endl ;
}

static void
EnbDlPhyReceptionChange(uint32_t nodeid , const PhyReceptionStatParameters params)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tenbDlPhyReceptionnode\t" << nodeid 
            << "\tcellId\t" << (uint32_t) params.m_cellId
            << "\timsi\t" << (uint32_t) params.m_imsi
            << "\trnti\t" << (uint32_t) params.m_rnti
            << "\tTBS\t" << (uint32_t) params.m_size
            << "\tmcs\t" << (uint32_t) params.m_mcs
            << "\tTB IsCorrect\t" << (uint32_t) params.m_correctness
            << std::endl ;
}

static void
HandoverStartFun(uint32_t nodeid , uint64_t imsi, uint16_t cellId, uint16_t rnti, uint16_t otherCid)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tue node\t" << nodeid
            << "\tstart handover\t"
            << "\tfrom\t" << (uint32_t) cellId
            << "\tto\t" << (uint32_t) otherCid
            << std::endl ;
}

static void
RandomAccessSuccessfulFun(uint32_t nodeid , uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tue\t" << nodeid
            << "\tRandomAccessSuccessful\t"
            << "\tcellId\t" << (uint32_t) cellId
            << std::endl ;
}


static void
HandoverEndErrorFun(uint32_t nodeid , uint64_t imsi, uint16_t cellId, uint16_t rnti )
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tue node\t" << nodeid
            << "\thandover connected to\t" << cellId
            << "\tcell\t" << std::endl ;
}

static void
PathLossChange(std::string node , Ptr< const SpectrumPhy > txPhy, Ptr< const SpectrumPhy > rxPhy, double lossDb)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\ttxnodeid\t" << txPhy->GetDevice()->GetNode()->GetId()
              << "\trxnodeid\t" << rxPhy->GetDevice()->GetNode()->GetId()
              << "\tlossDb\t" << lossDb
              << std::endl ;
}

static void
TracePathloss(void)
{
  Config::Connect("/ChannelList/*/$ns3::SpectrumChannel/PathLoss",MakeCallback(&PathLossChange)) ;
}


static void
ReportUeMeasurementsChange(uint32_t nodeid , uint16_t rnti, uint16_t cellId, double rsrp, double rsrq, bool isServingCell, uint8_t componentCarrierId)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\trnti\t" << (uint32_t) rnti
            << "\tcellId\t" << (uint32_t) cellId
            << "\trsrp\t" << (uint32_t) EutranMeasurementMapping::Db2RsrqRange(rsrp)
            << "\trsrq\t" << (uint32_t) EutranMeasurementMapping::Db2RsrqRange(rsrq)
            << "\tisServingCell\t" << isServingCell
            << "\tcomponentCarrierId\t" << (uint32_t) componentCarrierId
            << std::endl ;
}

static void
HandoverEndOkFun(uint32_t nodeid , uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tue node\t" << nodeid
              << "\tHandover finish\t"
              << "\tcellId\t" << (uint32_t) cellId
              << std::endl ;
}

static void
RtoChange(std::string node , Time oldValue , Time newValue)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\toldvalue\t" << oldValue.GetSeconds()
              << "\tnewvalue\t" << newValue.GetSeconds()
              << std::endl; 
}

static void
TraceRto(void)
{
    Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/RTO" , MakeCallback(&RtoChange)) ;
}

static void
ReportPuschTxPowerChange(uint32_t nodeid , uint16_t cellid , uint16_t rnti , double PuschTxPower )
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tue node\t" << nodeid
              << "\tcellid\t" << (uint32_t) cellid
              << "\trnti\t" << (uint32_t) rnti
              << "\tPuschTxPower\t" << PuschTxPower
              << std::endl ;
}

static void
RecvMeasurementReportChange(uint32_t nodeid , const uint64_t imsi, const uint16_t cellId, const uint16_t rnti, const LteRrcSap::MeasurementReport report )
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode id\t" << nodeid
              << "\timsi\t" << (uint32_t) imsi
              << "\tcellId\t" << (uint32_t) cellId
              << "\trnti\t" << (uint32_t) rnti
              << "\trsrp\t" << (uint32_t) report.measResults.rsrpResult
              << "\trsrq\t" << (uint32_t) report.measResults.rsrqResult 
              << "\trecvMeasurementReport\t"
              << std::endl ;
}

static void
UlSchedulingChange(std::string node , const uint32_t frame, const uint32_t subframe, const uint16_t rnti, const uint8_t mcs, const uint16_t tbsSize , const uint8_t component_id )
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\trnti\t" << (uint32_t) rnti
              << "\tmcs\t" << (uint32_t) mcs
              << "\ttbsize\t" << (uint32_t) tbsSize
              << "\tframe\t" << frame
              << "\tsubframe\t" << subframe
              << "\tUlScheduling\t"
              << std::endl ;
}

static void
TraceUlScheduling(void)
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/$ns3::LteEnbNetDevice/ComponentCarrierMap/*/LteEnbMac/UlScheduling" , MakeCallback(&UlSchedulingChange)) ;
}

static void
PdcpRxChange(std::string node , const uint16_t rnti, const uint8_t lcid, const uint32_t size, const uint64_t delay)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << node
            << "\tpdcp rxsize\t" << size
            << "\tdelay\t" << delay
            << "\tPdcpRxChange\t"
            << std::endl ;
}

static void
TraceRxPdu(void)
{
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/$ns3::LteUeNetDevice/LteUeRrc/DataRadioBearerMap/*/LtePdcp/RxPDU12" , MakeCallback(&PdcpRxChange) ) ;
}


void
ClientRx (uint32_t nodeid , Ptr<const Packet> packet, const Address &address)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid 
            << "\tclient recv\t" << packet->GetSize()
            << "\tbytes from\t" << address 
            << std::endl;
}

void
ClientTx (uint32_t nodeid , Ptr<const Packet> packet)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid 
            << "\tclient request\t" << packet->GetSize()
            << std::endl;
}

void
ClientEmbeddedObjectReceived (uint32_t nodeid ,Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
        std::cout << "Time\t" << Simulator::Now().GetSeconds()
                  << "\tnodeid\t" << nodeid
                  << "\tClient has successfully received an embedded object of\t"
                  << p->GetSize() << "\tbyts" << std::endl ;
    }
  else
    {
      std::cout << "Time\t" << Simulator::Now().GetSeconds()
                << "\tnodeid\t" << nodeid
                << "\tClient failed to parse an embedded object\t"
                << p->GetSize() << "\tbyts" << std::endl ;
    }
}

void
ClientMainObjectReceived (uint32_t nodeid , Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
  Ptr<Packet> p = packet->Copy ();
  ThreeGppHttpHeader header;
  p->RemoveHeader (header);
  if (header.GetContentLength () == p->GetSize ()
      && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
    {
        std::cout << "Time\t" << Simulator::Now().GetSeconds()
                  << "\tnodeid\t" << nodeid
                  << "\tClient has successfully received a main object of\t"
                  << p->GetSize() << "\tbyts" << std::endl ;
    }
    else
    {
        std::cout << "Time\t" << Simulator::Now().GetSeconds()
                  << "\tnodeid\t" << nodeid
                  << "\tClient failed to parse a main object.\t"
                  << p->GetSize() << "\tbyts" << std::endl ;
    }
}

static void
DlTxStartChange(uint32_t nodeid , Ptr< const PacketBurst > burst)
{
  if(burst)
  {
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tenb nodeid\t" << nodeid
              << "\tTxStart\t"
              << "\tpackets\t" << burst->GetNPackets()
              << "\tsize\t" << burst->GetSize()
              << std::endl ;
  }
}


static void
DlTxEndChange(uint32_t nodeid , Ptr< const PacketBurst > burst)
{
  if(burst)
  {
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tenb nodeid\t" << nodeid
              << "\tTxEnd\t"
              << "\tpackets\t" << burst->GetNPackets()
              << "\tsize\t" << burst->GetSize()
              << std::endl ;
  }
}

static void
EnbRxEndOkChange(uint32_t nodeid , Ptr< const Packet > p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tenbnodeid\t" << nodeid
            << "\tRxEndOk\t" << p->GetSize()
            << "\tbytes\t" << std::endl ;
}

static void
EnbRxEndErrorChange(uint32_t nodeid , Ptr< const Packet > p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tenbnodeid\t" << nodeid
            << "\tRxEndError\t" << p->GetSize()
            << "\tbytes\t" << std::endl ;
}

static void
PgwPhyTxEnd(uint32_t nodeid ,Ptr<const Packet> p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tpgw nodeid\t" << nodeid
            << "\tPGW PhyTxEnd\t" << p->GetSize()
            << std::endl ;
}

static void
PgwPhyRxEnd(uint32_t nodeid ,Ptr<const Packet> p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tpgw nodeid\t" << nodeid
            << "\tPGW PhyRxEnd\t" << p->GetSize()
            << std::endl ;
}

static void
PgwPhyRxDrop(uint32_t nodeid , Ptr<const Packet> p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tpgw nodeid\t" << nodeid
            << "\tPGW PhyRxDrop\t" << p->GetSize()
            << std::endl ;
}

static void
PgwPhyTxDrop(uint32_t nodeid , Ptr<const Packet> p)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tpgw nodeid\t" << nodeid
            << "\tPGW PhyTxDrop\t" << p->GetSize()
            << std::endl ;
}


static void
TcpRxChange(std::string node , const Ptr< const Packet > packet, const TcpHeader &header, const Ptr< const TcpSocketBase > socket)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnode\t" << node
            << "\trecv packet\t" << packet->GetSize()
            << "\tfrom IP\t" << header.GetSequenceNumber()
            << "\tack\t" << header.GetAckNumber()
            << std::endl ;
}

static void
TraceTcpRx(void)
{
    Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Rx" , MakeCallback(&TcpRxChange) ) ;
}


static void
TcpTxChange(std::string node , const Ptr< const Packet > packet, const TcpHeader &header, const Ptr< const TcpSocketBase > socket)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnode\t" << node
              << "\tsend packet\t" << packet->GetSize()
              << "\tto IP\t" << header.GetSequenceNumber()
              << "\tack\t" << header.GetAckNumber()
              << std::endl ;
}


static void
TraceTcpTx(void)
{
    Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/Tx" , MakeCallback(&TcpTxChange)) ;
}


void
ServerConnectionEstablished (uint32_t nodeid , Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\tClient has established a connection to the server.\t"
            << std::endl ;
}

void
MainObjectGenerated (uint32_t nodeid ,uint32_t size)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\tServer generated a main object of\t" << size << "\tbytes."
            << std::endl ;
}

void
EmbeddedObjectGenerated (uint32_t nodeid , uint32_t size)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\tServer generated a embedded object of\t" << size << "\tbytes."
            << std::endl ;
}

void
ServerTx (uint32_t nodeid , Ptr<const Packet> packet)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid
            << "\tServer sent a packet of\t" << packet->GetSize()
            << "\tbytes" << std::endl ;
}

void
ServerRx (uint32_t nodeid , Ptr<const Packet> packet, const Address &address)
{
  std::cout << "Time\t" << Simulator::Now().GetSeconds()
            << "\tnodeid\t" << nodeid 
            << "\tserver recv\t" << packet->GetSize()
            << "\tbytes from\t" << address 
            << std::endl;
}

void 
ServerRxDelay(uint32_t nodeid , const Time &delay, const Address &from)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnodeid\t" << nodeid
              << "\tServerRxdelay\t" << delay.GetSeconds()
              << "\tfrom\t" << from
              << std::endl ;
}

void 
ClientRxDelay(uint32_t nodeid , const Time &delay, const Address &from)
{
    
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnodeid\t" << nodeid
              << "\tClientRxdelay\t" << delay.GetSeconds()
              << "\tfrom\t" << from
              << std::endl ;
}

void
ClientRxRtt(uint32_t nodeid , const Time& rtt, const Address &from)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tnodeid\t" << nodeid
              << "\tClientRxRtt\t" << rtt.GetSeconds()
              << std::endl ;
}

void
ClientCwndChange(uint32_t nodeid , uint32_t oldval , uint32_t newval)
{
    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\tClient nodeid\t" << nodeid
              << "\tClient Cwnd\t" << oldval
              << "\tupdate\t" << newval
              << std::endl ;
}

void
DisplayConfigure(void)
{
    Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
    Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
    Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
    ConfigStore outputConfig2;
    outputConfig2.ConfigureDefaults ();
    outputConfig2.ConfigureAttributes ();
}

int
main (int argc, char *argv[])
{
    // 首先，设置LTE网络
    std::string transport_prot = "TcpBbr" ;
    uint16_t numNodePairs = 1;           // 基站数目
    uint16_t per_enb_ue_num = 1  ;     // 一个基站对应的用户的数目
    Time simTime = Seconds (50);         // 脚本仿真时间
    double distance = 2000.0;
    uint32_t  buffer = 5000000 ;
    uint32_t  satqueue = 1000 ;
    bool is_random = false ;
    bool is_handover = false ;
    bool disableDl = true;               // 是否启动下行链路传输 远端主机 ---> 本地与基站相连接的各个主机
    bool disableUl = false;              // 是否启动上行链路传输 本地与基站相连接的各个主机 --->远端主机
    bool useHelper = false;              // 是否搭建 PointToPointEpcHelper 链路脚本
    std::string app = "BulkSend";                    // http / bulksend

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue ("transport_prot", "Transport protocol to use: TcpBbr, TcpHybla, TcpCubic, TcpCopa", transport_prot);
    cmd.AddValue ("eNodeB_num", "Number of eNodeBs + UE pairs", numNodePairs);
    cmd.AddValue ("Per_enb_ue_num", "Number of eNodeBs + UE pairs", per_enb_ue_num);
    cmd.AddValue ("SimTime", "Total duration of the simulation", simTime);
    cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
    cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
    cmd.AddValue ("useHelper", "Build the backhaul network using the helper or it is built in the example", useHelper);
    cmd.AddValue ("buffer" , "Set Socket sndbuffer , recv_buffer" , buffer) ;
    cmd.AddValue ("application" , "Set application used by simulation" , app) ;
    cmd.AddValue ("RandomPosition" , "Whether enable random distributed Ues in eNodeB" , is_random) ;
    cmd.AddValue ("SatQueue" , "Set SatQueue Max Packets" , satqueue) ;
    cmd.Parse (argc, argv);
    
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults ();

    std::cout <<"参数: "
              << "\t应用\t" << app << std::endl ;

  
   
    // 设置协议仿真协议类型 以及 发送端 与 接收端 的缓冲区大小 TcpCubic::GetTypeId() TypeId::LookupByName (transport_prot))
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCp::GetTypeId()) ) ;
    Config::SetDefault("ns3::TcpSocket::RcvBufSize" , UintegerValue(buffer) ); // 5m
    Config::SetDefault("ns3::TcpSocket::SndBufSize" , UintegerValue(buffer) ); // 5m
    //Config::SetDefault("ns3::BulkSendApplication::MaxBytes" , UintegerValue(0)) ;
    
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity" , UintegerValue(160)) ; // 可以设置一基站可连接的最多用户数目
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue (LteEnbRrc::RLC_UM_ALWAYS)) ; // 选择UM发送模式
    Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(25) ) ;
    Config::SetDefault("ns3::RadioBearerStatsCalculator::EpochDuration" ,  TimeValue(Seconds (0.01))) ;
    
    Config::SetDefault("ns3::LteHelper::PathlossModel" ,  TypeIdValue(FriisPropagationLossModel::GetTypeId ()) ) ;//也没有影响。
    
    
    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<EpcHelper> epcHelper;
    if (!useHelper)
    {   
        /*
            函数AddEnb: 1.创建 服务器套接字(ipv4 + ipv6){包含套接字创建,地址绑定,等待连接的Connect,明面上好像没看到listen函数}
                        2.安装应用 EpcEnbApplication
                        3.启动EpcX2 ?
            函数AddX2Interface : 1。搭建两个基站节点间的点对点链路 ，NewNetWork 为两个基站节点分配地址
                                2.  启动两个EpcX2实体？
            函数AddUe: 1.mme 和 pgw 添加用户
            函数ActivateEpsBearer ：
            函数DoActivateEpsBearerForUe：Simulator::ScheduleNow (&EpcUeNas::ActivateEpsBearer, ueLteDevice->GetNas (), bearer, tft) ？
            函数AddS1Interface：
        */
        epcHelper = CreateObject<NoBackhaulEpcHelper> ();
    }
    else
    {
        epcHelper = CreateObject<PointToPointEpcHelper> ();
    }
    lteHelper->SetEpcHelper (epcHelper);

  
    NodeContainer ueNodes;                  // 创建用户节点
    NodeContainer enbNodes;                 // 创建基站节点
    enbNodes.Create (numNodePairs);         // 创建 numNodePairs 数目的基站节点
    ueNodes.Create (numNodePairs * per_enb_ue_num);  // 根据前面的每个基站对应的参数 创建对应数目的用户节点

    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm") ;
    lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold" , UintegerValue (30)) ;
    lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",UintegerValue (1));

    if (!is_handover)
    {
        // listPositionAllocator 将 x , y , z 三维坐标以vector为最基本结构存入 
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        for (uint16_t i = 0; i < numNodePairs; i++)
        {
            positionAlloc->Add (Vector (distance * i, 0 , 10));     // 存储 enbNodes 的节点位置
        }
        MobilityHelper mobility;
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");   // 相当于固定位置,不移动
        mobility.SetPositionAllocator (positionAlloc);
        mobility.Install (enbNodes);
    }
    else{
        
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        positionAlloc->Add(Vector(0 , 0 , 10)) ;
        //positionAlloc->Add(Vector(4000 , 0 , 10)) ;
        // positionAlloc->Add(Vector(8000 , 0 , 10)) ;
        // for (uint16_t i = 0; i < numNodePairs; i++)
        // {
        //     positionAlloc->Add (Vector (distance * i , distance * i , 10));     // 存储 enbNodes 的节点位置
        // }
        
        MobilityHelper mobility;
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");   // 相当于固定位置,不移动
        mobility.Install (enbNodes);
    }
    /*
        Box (double _xMin, double _xMax,double _yMin, double _yMax , double _zMin, double _zMax);
        假设半径以米为单位，基站的范围是300米
    */
    if(is_random)
    {
        Box UeBox = Box (-3000 , 3000 , -3000 , 3000 , 1.5 , 2) ;
    
        MobilityHelper uemobility ;
        // 分别为 用户节点设置随机的位置 在 [-3000 , 3000] , [-3000 , 3000] , [10 , 40] 的长方体位置
        for(uint32_t id = 0 ;  id < numNodePairs * per_enb_ue_num ; id ++)
        {
            RngSeedManager::SetSeed (1) ; // 设置随机数种子
            Ptr<RandomBoxPositionAllocator> randomUePositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
            Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
            xVal->SetAttribute ("Min", DoubleValue (UeBox.xMin));
            xVal->SetAttribute ("Max", DoubleValue (UeBox.xMax));
            randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
            Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
            yVal->SetAttribute ("Min", DoubleValue (UeBox.yMin));
            yVal->SetAttribute ("Max", DoubleValue (UeBox.yMax));
            randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
            Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
            zVal->SetAttribute ("Min", DoubleValue (UeBox.zMin));
            zVal->SetAttribute ("Max", DoubleValue (UeBox.zMax));
            randomUePositionAlloc->SetAttribute ("Z", PointerValue (zVal));
            uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");   // 相当于固定位置,不移动
            uemobility.SetPositionAllocator (randomUePositionAlloc);
            // uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
            //                             "Mode", StringValue ("Time"),
            //                             "Time", StringValue ("0.1s"),
            //                             "Direction",StringValue("ns3::ConstantRandomVariable[Constant=0.78]"),
            //                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=100.0]"),
            //                             "Bounds", RectangleValue (Rectangle (-8000.0, 8000.0, -8000.0 , 8000.0)));
            uemobility.Install (ueNodes.Get(id));

            
            Ptr<MobilityModel> model = ueNodes.Get(id)->GetObject<MobilityModel> ();
            Vector pos = model->GetPosition() ;
            std::cout << "node id\t" << ueNodes.Get(id)->GetId() << "\tX:\t" << xVal->GetValue() 
                                                                << "\tY:\t" << yVal->GetValue() 
                                                                << "\tZ:\t" << zVal->GetValue() 
                                                                << std::endl ;
        }
    }
    else{
        Ptr<ListPositionAllocator> ue_positionAlloc = CreateObject<ListPositionAllocator> ();
        ue_positionAlloc->Add(Vector(1000,1000,1.74)) ;
        ue_positionAlloc->Add(Vector(4000 , 1000 , 1.74)) ;
        // ue_positionAlloc->Add(Vector(8000 , 1000 , 1.74)) ;
        MobilityHelper uemobility;
        uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");   // 相当于固定位置,不移动
        uemobility.SetPositionAllocator (ue_positionAlloc);
        uemobility.Install (ueNodes);
        // else{
        //     MobilityHelper uemobility;
        //     uemobility.SetPositionAllocator (ue_positionAlloc);
        //     uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
        //                                 "Mode", StringValue ("Time"),
        //                                 "Time", StringValue ("0.1s"),
        //                                 "Direction",StringValue("ns3::ConstantRandomVariable[Constant=0]"),
        //                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=100.0]"),
        //                                 "Bounds", RectangleValue (Rectangle (0.0, 20000.0, 0.0, 20000.0)));
        //     uemobility.Install (ueNodes.Get(0));

        //     uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel") ;
        //     uemobility.Install (ueNodes.Get(1));

        //     // uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
        //     //                             "Mode", StringValue ("Time"),
        //     //                             "Time", StringValue ("0.1s"),
        //     //                             "Direction",StringValue("ns3::ConstantRandomVariable[Constant=0]"),
        //     //                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=100.0]"),
        //     //                             "Bounds", RectangleValue (Rectangle (0.0, 20000.0, 0.0, 20000.0)));
        //     // uemobility.Install (ueNodes.Get(2));

        // }
        
    }
    
   
    AsciiTraceHelper asciiTraceHelper1;

    /* 
        SGW node :  完成MME控制下进行数据包的路由和转发，即将接收到的用户数据转发给指定的PGW的网元，
                    又因为接收和发送的均为GTP协议数据包，无需进行数据分组的格式转化，也就是GTP数据分组的双向传输通道
    */
    Ptr<Node> sgw = epcHelper->GetSgwNode ();
    Ptr<ListPositionAllocator> sgwpositionAlloc = CreateObject<ListPositionAllocator> ();
    sgwpositionAlloc->Add (Vector (0.0,  50.0, 0.0));
    MobilityHelper sgwmobility;
    sgwmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    sgwmobility.SetPositionAllocator (sgwpositionAlloc);
    sgwmobility.Install (sgw);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);  // 该函数被调用前必须要设置mobility模型
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);     // 该函数被调用前必须要设置mobility模型

    if(!useHelper)
    {
        // 此时 核心骨干网 为 非回传链路拓扑网络
      Ipv4AddressHelper s1uIpv4AddressHelper;

      // Create networks of the S1 interfaces
      s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.252");

      for (uint16_t i = 0; i < numNodePairs; ++i)
        {
            Ptr<Node> enb = enbNodes.Get (i) ;
            // Create a point to point link between the eNB and the SGW with
            // the corresponding new NetDevices on each side
            PointToPointHelper p2ph;
            DataRate s1uLinkDataRate = DataRate ("10Gb/s");
            uint16_t s1uLinkMtu = 2000;
            Time s1uLinkDelay = Time (0);
            p2ph.SetDeviceAttribute ("DataRate", DataRateValue (s1uLinkDataRate));
            p2ph.SetDeviceAttribute ("Mtu", UintegerValue (s1uLinkMtu));
            p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));
            NetDeviceContainer sgwEnbDevices = p2ph.Install (sgw, enb);
            
            Ipv4InterfaceContainer sgwEnbIpIfaces = s1uIpv4AddressHelper.Assign (sgwEnbDevices);
            s1uIpv4AddressHelper.NewNetwork ();
            
            Ipv4Address sgwS1uAddress = sgwEnbIpIfaces.GetAddress (0) ;
            Ipv4Address enbS1uAddress = sgwEnbIpIfaces.GetAddress (1) ;
            // Create S1 interface between the SGW and the eNB
            epcHelper->AddS1Interface (enb, enbS1uAddress, sgwS1uAddress);
        }
    }

  // Install the IP stack on the UEs
  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  // 静态路由：如路标一样，传输线路固定，无论网络状况如何改变并且重组，都不会被改变
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway pgw节点 for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      // SetDefaultRoute 将epcHelper->GetUeDefaultGatewayAddress () 所表述的 address of the tun device 作为下一跳路由
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numNodePairs; i++)
  {
    for(uint16_t j = 0 ; j < per_enb_ue_num ; j ++)
    {
      lteHelper->Attach (ueLteDevs.Get(i * per_enb_ue_num + j), enbLteDevs.Get(i));
      // side effect: the default EPS bearer will be activated
    }
  }

  // Add X2 interface
  lteHelper->AddX2Interface (enbNodes);

  Ptr<RadioBearerStatsCalculator> pdcp = lteHelper->GetPdcpStats() ;
    // 接下来，创建卫星链路部分
    // 设置SatHelper的trace相关参数
    Config::SetDefault ("ns3::SatHelper::ScenarioCreationTraceEnabled", BooleanValue (false));
    Config::SetDefault ("ns3::SatHelper::PacketTraceEnabled", BooleanValue (false));
    // 命名本次仿真，对应trace将存储的文件夹名称
    Config::SetDefault ("ns3::SatEnvVariables::SimulationCampaignName", StringValue ("sat-for-epc"));
    Config::SetDefault ("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue (true));
    Config::SetDefault ("ns3::SatPhy::EnableStatisticsTags" , BooleanValue(true)) ;
    // 设置卫星排队最大分组数目 // The maximum number of packets accepted by this SatQueue. ---> 确实有用
    Config::SetDefault("ns3::SatQueue::MaxPackets" , UintegerValue (satqueue)) ;
    //Config::SetDefault("ns3::SatQueue::MaxPackets" , UintegerValue (1000)) ;
    
    Config::SetDefault("ns3::SatNetDevice::EnableStatisticsTags" ,  BooleanValue(true)) ;
    /**
     * The packet shall be received by all the receivers in the channel. The
     * intended receiver shall receive the packet, while other receivers in the
     * channel see the packet as co-channel interference. Note, that ALL_BEAMS mode
     * is needed by the PerPacket interference. 
    */
    // Config::SetDefault("ns3::SatChannel::ForwardingMode" , EnumValue (SatChannel::ALL_BEAMS)) ;  存在互信道干扰
    // 创建卫星通信场景，采用SIMPLE，包括一个UT，一个GW
    SatHelper::PreDefinedScenario_t satScenario = SatHelper::SIMPLE;
    Ptr<SatHelper> satHelper = CreateObject<SatHelper> ();
    satHelper->CreatePredefinedScenario (satScenario);
    // 将卫星的UT user与LTE的pgw相连
    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    Ptr<NetDevice> pgw_device = pgw->GetDevice(0) ;

    

    Ptr<Node> utUser = satHelper->GetUtUsers ().Get (0);
    // 创建pgw与utUser间的P2P连接
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));
    
    NetDeviceContainer internetDevices = p2ph.Install (pgw, utUser);

    Ptr<PointToPointNetDevice> pgw_ptp_device = DynamicCast<PointToPointNetDevice>(internetDevices.Get(0)) ;

    if(pgw_ptp_device)
    {
      pgw_ptp_device->TraceConnectWithoutContext("PhyTxEnd" , MakeBoundCallback(&PgwPhyTxEnd , pgw->GetId())) ;
      pgw_ptp_device->TraceConnectWithoutContext("PhyRxEnd" , MakeBoundCallback(&PgwPhyRxEnd , pgw->GetId())) ;
      pgw_ptp_device->TraceConnectWithoutContext("PhyRxDrop" , MakeBoundCallback(&PgwPhyRxDrop, pgw->GetId())) ;
      pgw_ptp_device->TraceConnectWithoutContext("PhyTxDrop" , MakeBoundCallback(&PgwPhyTxDrop, pgw->GetId())) ;
    }

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

    // 设置卫星链路节点往LTE方向的路由信息
    Ptr<Ipv4StaticRouting> hostRouting;
    hostRouting = ipv4RoutingHelper.GetStaticRouting (utUser->GetObject<Ipv4> ());
    hostRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 2);
    hostRouting = ipv4RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv4> ());
    hostRouting->SetDefaultRoute (Ipv4Address ("10.0.0.2"), 3);
    Ptr<Ipv4> ipv4;
    // UT node
    ipv4 = satHelper->UtNodes ().Get (0)->GetObject<Ipv4> ();
    hostRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
    hostRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), "10.1.0.2", 1);
    // GW node
    ipv4 = satHelper->GwNodes ().Get (0)->GetObject<Ipv4> ();
    hostRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
    hostRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), "40.1.0.2", 1);
    // GW router
    ipv4 = satHelper->GetUserHelper ()->GetRouter ()->GetObject<Ipv4> ();
    hostRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
    hostRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), "90.1.0.1", 1);
    // GW user (i.e., remote server host)
    ipv4 = satHelper->GetGwUsers ().Get (0)->GetObject<Ipv4> ();
    hostRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
    hostRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), "90.2.0.1", 1);

    // 在UEs和卫星远端GW user上分别安装应用
    // Install and start applications on UEs and remote host
    
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    Ptr<Node> remoteHost = satHelper->GetGwUsers ().Get (0);
    Ipv4Address remoteHostAddr = satHelper->GetUserAddress (remoteHost);

    if(app == "http" || app == "HTTP")
    {
      for (uint32_t u = 0; u < ueNodes.GetN() ; u ++)
      {
        if (!disableUl)
        {
            ++ulPort ;
            // HTTP服务器
            ThreeGppHttpServerHelper serverHelper(remoteHostAddr) ;
            serverHelper.SetAttribute("LocalPort" , UintegerValue (ulPort)) ;
            serverApps = serverHelper.Install(remoteHost) ;

            Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

            // Example of connecting to the trace sources
            httpServer->TraceConnectWithoutContext ("ConnectionEstablished",
                                                    MakeBoundCallback (&ServerConnectionEstablished , remoteHost->GetId()));
            httpServer->TraceConnectWithoutContext ("MainObject", MakeBoundCallback (&MainObjectGenerated , remoteHost->GetId()));
            httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeBoundCallback (&EmbeddedObjectGenerated , remoteHost->GetId()));
            httpServer->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&ServerTx , remoteHost->GetId()));
            httpServer->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&ServerRx , remoteHost->GetId()));
            httpServer->TraceConnectWithoutContext ("RxDelay", MakeBoundCallback (&ServerRxDelay , remoteHost->GetId())) ;

            //HTTP相关参数
            PointerValue ptr ;
            serverHelper.SetAttribute("Variables" , ptr) ;
            Ptr<ThreeGppHttpVariables> httpvariables = ptr.Get<ThreeGppHttpVariables> () ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::MainObjectSizeMin" , UintegerValue (1000000)) ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::MainObjectSizeMax" , UintegerValue (10000000)) ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::MainObjectSizeMean" , UintegerValue (5000000)) ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::MainObjectSizeStdDev" , UintegerValue (500)) ;

            Config::SetDefault("ns3::ThreeGppHttpVariables::EmbeddedObjectSizeMin" , UintegerValue (500000)) ; // 500k
            Config::SetDefault("ns3::ThreeGppHttpVariables::EmbeddedObjectSizeMax" , UintegerValue (3000000)) ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::EmbeddedObjectSizeMean" , UintegerValue (1250000)) ;
            Config::SetDefault("ns3::ThreeGppHttpVariables::EmbeddedObjectSizeStdDev" , UintegerValue (1000)) ;

            //httpvariables->SetMainObjectSizeMean (102400); // 100kB
            // httpvariables->SetMainObjectSizeStdDev (40960); // 40kB

            // HTTP客户器
            ThreeGppHttpClientHelper clientHelper(remoteHostAddr) ;
            clientHelper.SetAttribute("RemoteServerPort" , UintegerValue(ulPort)) ;
            clientApps = clientHelper.Install(ueNodes.Get(u)) ;

            Ptr<ThreeGppHttpClient> httpClient = clientApps.Get(0) -> GetObject<ThreeGppHttpClient> () ;
            httpClient->TraceConnectWithoutContext ("RxMainObject", MakeBoundCallback (&ClientMainObjectReceived ,  ueNodes.Get(u)->GetId()));
            httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeBoundCallback (&ClientEmbeddedObjectReceived , ueNodes.Get(u)->GetId()));
            httpClient->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&ClientRx ,ueNodes.Get(u)->GetId()));
            httpClient->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&ClientTx ,ueNodes.Get(u)->GetId()));
            httpClient->TraceConnectWithoutContext ("RxDelay" , MakeBoundCallback (&ClientRxDelay ,ueNodes.Get(u)->GetId())) ;
            httpClient->TraceConnectWithoutContext ("RxRtt" , MakeBoundCallback (&ClientRxRtt , ueNodes.Get(u)->GetId())) ;

            Ptr<Socket> http_client_socket =  httpClient -> GetSocket() ;

            if(http_client_socket){
                http_client_socket -> TraceConnectWithoutContext("CongestionWindow" , MakeBoundCallback(&ClientCwndChange , ueNodes.Get(u)->GetId())) ;
            } 
            else{
                std::cout << "http_client_socket is  NUll" << std::endl ;
            }
        }

        if (!disableDl)
        {
            ++dlPort;
            ThreeGppHttpServerHelper ulserver(remoteHostAddr) ;
            ulserver.SetAttribute("LocalPort",UintegerValue(dlPort)) ;
            serverApps.Add (ulserver.Install (remoteHost));
            
            ThreeGppHttpClientHelper ulclient(remoteHostAddr) ;
            ulclient.SetAttribute("RemoteServerPort" , UintegerValue(dlPort)) ;
            clientApps.Add (ulclient.Install (ueNodes.Get (u))) ;
        }
      }
    }
    else if(app == "udp")
    {
      LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
      for (uint32_t u = 0; u < ueNodes.GetN() ; u ++)
      {
        // 下行链路
          if (!disableUl)
          {
              ++ulPort ;
              UdpServerHelper server(ulPort) ;
              serverApps = server.Install(ueNodes.Get(u)) ;

              uint32_t MaxPacketSize = 1460;
              Time interPacketInterval = Seconds (0.0001);
              uint32_t maxPacketCount = 9999999999;
              UdpClientHelper client (ueIpIface.GetAddress(u), ulPort);
              client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
              client.SetAttribute ("Interval", TimeValue (interPacketInterval));
              client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
              clientApps = client.Install (remoteHost);
          }

          if (!disableDl)
          {
              ++dlPort;
              UdpServerHelper server(dlPort) ;
              serverApps = server.Install(remoteHost) ;
              uint32_t MaxPacketSize = 1460;
              Time interPacketInterval = Seconds (0.1);
              uint32_t maxPacketCount = 9999999999;
              UdpClientHelper client (remoteHostAddr, dlPort);
              client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
              client.SetAttribute ("Interval", TimeValue (interPacketInterval));
              client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
              clientApps = client.Install (ueNodes.Get(u));
          }
      }
    }
    else{
        for (uint32_t u = 0; u < ueNodes.GetN() ; u ++)
        {
            // 下行链路
            if (!disableUl)
            {
                ++ulPort ;
                BulkSendHelper bulkHelper("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress(u),ulPort));
                clientApps = bulkHelper.Install(remoteHost) ;

                PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress(u),ulPort));
                serverApps = sinkHelper.Install(ueNodes.Get(u)) ; 
            }

            if (!disableDl)
            {
                ++dlPort;
                BulkSendHelper bulkHelper("ns3::TcpSocketFactory", InetSocketAddress (remoteHostAddr,ulPort));
                clientApps = bulkHelper.Install(ueNodes.Get(u)) ;

                PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (remoteHostAddr,ulPort));
                serverApps = sinkHelper.Install(remoteHost) ;
            }
        }
    }

    
    clientApps.Start (Seconds (0));
    serverApps.Start (Seconds (0));

    
    if(app == "BulkSend")
    {
        Simulator::Schedule(Seconds(0.01) , &TraceRTT) ;
        Simulator::Schedule(Seconds(0.1) , &TraceCwnd) ;
        Simulator::Schedule(Seconds(0.01) , &TraceRto) ;
        Simulator::Schedule(Seconds(0.01) , &TraceBytesInFlight) ;
        Simulator::Schedule(Seconds(1.0) ,  &TraceCourseChange) ;
        Simulator::Schedule(Seconds(0.01) , &TraceReportCurrentCellRsrpSinr);
        Simulator::Schedule(Seconds(1.00) , &Notify , ueLteDevs , ueNodes , enbNodes , pgw) ;
        Simulator::Schedule(Seconds(0.01) , &TraceReportInterference) ; // 能用但没必要
        Simulator::Schedule(Seconds(0.01) , &TraceReportUeSinr) ;
        Simulator::Schedule(Seconds(0.01) , &TracePathloss) ;
        Simulator::Schedule(Seconds(0.01) , &TraceUlScheduling) ;
        Simulator::Schedule(Seconds(0.01) , &TraceTcpRx) ;
        Simulator::Schedule(Seconds(0.01) , &TraceTcpTx) ;
        Simulator::Schedule(Seconds(0.01) , &TraceRx) ;
        Simulator::Schedule(Seconds(0.01) , &TraceBulkSend);
    }
    /*
      输出数据---分析
    */
    lteHelper->EnablePhyTraces ();
    lteHelper->EnableMacTraces ();
    lteHelper->EnableRlcTraces ();
    lteHelper->EnablePdcpTraces () ;

    for(int i = 0 ; i < ueLteDevs.GetN() ; i ++)
    {
        Ptr<LteUePhy> phy = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy() ;
        Ptr<LteSpectrumPhy> dlspectrumphy = phy -> GetDlSpectrumPhy () ;
        Ptr<LteSpectrumPhy> ulspectrumphy = phy -> GetUlSpectrumPhy () ;
        phy->TraceConnectWithoutContext ("UlPhyTransmission",MakeBoundCallback(&UlPhytransmissionChange , ueNodes.Get(i)->GetId()));
        phy->TraceConnectWithoutContext ("ReportUeMeasurements" , MakeBoundCallback(&ReportUeMeasurementsChange , ueNodes.Get(i)->GetId())) ;
        dlspectrumphy->TraceConnectWithoutContext ("RxEndError" , MakeBoundCallback(&DlRxEndErrorChange , ueNodes.Get(i)->GetId()) ) ;
        dlspectrumphy->TraceConnectWithoutContext ("RxEndOk" , MakeBoundCallback(&DlRxEndOkChange , ueNodes.Get(i)->GetId()) ) ;
        dlspectrumphy->TraceConnectWithoutContext ("RxStart" , MakeBoundCallback(&RxStartChange , ueNodes.Get(i)->GetId()) ) ;
        dlspectrumphy->TraceConnectWithoutContext ("DlPhyReception" , MakeBoundCallback(&DlPhyReceptionChange , ueNodes.Get(i)->GetId()) ) ;
        ulspectrumphy->TraceConnectWithoutContext ("TxStart" , MakeBoundCallback(&UlTxStartChange , ueNodes.Get(i)->GetId()) ) ;
        ulspectrumphy->TraceConnectWithoutContext ("TxEnd" , MakeBoundCallback(&UlTxEndChange , ueNodes.Get(i)->GetId()) ) ;

        Ptr<LteUeRrc> ueRrc = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc ();
        ueRrc->TraceConnectWithoutContext("HandoverStart" , MakeBoundCallback(&HandoverStartFun , ueNodes.Get(i)->GetId()) ) ;
        ueRrc->TraceConnectWithoutContext("RandomAccessSuccessful" , MakeBoundCallback(&RandomAccessSuccessfulFun , ueNodes.Get(i)->GetId()) ) ;
        ueRrc->TraceConnectWithoutContext("HandoverEndOk" , MakeBoundCallback(&HandoverEndOkFun , ueNodes.Get(i)->GetId()) ) ;
        ueRrc->TraceConnectWithoutContext("HandoverEndError" , MakeBoundCallback(&HandoverEndErrorFun , ueNodes.Get(i)->GetId()) ) ;

    }

    for(int i = 0 ; i < enbLteDevs.GetN() ; i ++)
    {
        Ptr<LteEnbPhy> phy = enbLteDevs.Get(i)->GetObject<LteEnbNetDevice> ()->GetPhy() ;
        phy->TraceConnectWithoutContext ("DlPhyTransmission",MakeBoundCallback(&DlPhytransmissionChange , enbNodes.Get(i)->GetId()));

        Ptr<LteSpectrumPhy> dlspectrumphy = phy -> GetDlSpectrumPhy () ;
        Ptr<LteSpectrumPhy> ulspectrumphy = phy -> GetUlSpectrumPhy () ;

        dlspectrumphy->TraceConnectWithoutContext ("TxStart" , MakeBoundCallback(&DlTxStartChange , enbNodes.Get(i)->GetId()) ) ;
        dlspectrumphy->TraceConnectWithoutContext ("TxEnd" , MakeBoundCallback(&DlTxEndChange , enbNodes.Get(i)->GetId()) ) ;

        ulspectrumphy->TraceConnectWithoutContext ("RxEndOk" , MakeBoundCallback(&EnbRxEndOkChange , enbNodes.Get(i)->GetId()) ) ;
        ulspectrumphy->TraceConnectWithoutContext ("RxEndError" , MakeBoundCallback(&EnbRxEndErrorChange , enbNodes.Get(i)->GetId()) ) ;
        ulspectrumphy->TraceConnectWithoutContext ("DlPhyReception" , MakeBoundCallback(&EnbDlPhyReceptionChange , enbNodes.Get(i)->GetId()) ) ;
      
        Ptr<LteEnbRrc> enbRrc = enbLteDevs.Get(i)->GetObject<LteEnbNetDevice> ()->GetRrc ();
        enbRrc -> TraceConnectWithoutContext ("RecvMeasurementReport" , MakeBoundCallback(&RecvMeasurementReportChange , enbNodes.Get(i)->GetId()) ) ;
    }

    Simulator::Schedule(Seconds(0) , &DisplayConfigure) ;
    
    Simulator::Stop(Seconds(500)) ;
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
