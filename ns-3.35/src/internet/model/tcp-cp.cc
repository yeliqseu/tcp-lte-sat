#include "tcp-cp.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("TcpCp") ;
NS_OBJECT_ENSURE_REGISTERED (TcpCp) ;

TypeId
TcpCp::GetTypeId (void) {
    static TypeId tid = TypeId ("TcpCp") 
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpCp> ()
    .SetGroupName("Internet")
    ;
    return tid ;
}

TcpCp::TcpCp ()
    : TcpNewReno()
{
    NS_LOG_FUNCTION(this) ;
}

TcpCp::TcpCp (const TcpCp &sock)
    : TcpNewReno (sock)
{
    NS_LOG_FUNCTION (this) ;
}

TcpCp::~TcpCp ()
{
    NS_LOG_FUNCTION (this) ; 
}

void 
TcpCp::IncreaseWindow(Ptr<TcpSocketState> tcb , uint32_t segmentsAcked)
{
   
}

void
TcpCp::Update_Velocity_Parameter(double now , double rtt , double win)
{
    if(m_last_time == -1)
    {
        m_last_time = now ;
        m_last_win = win ;
        m_last_win_len = 3 * rtt ;
        m_velocity_parameter = 1.0 ;
    }
    else if( now >= (m_last_win_len + m_last_time) )
    {
        m_velocity_parameter = (m_last_win <= win) ? m_velocity_parameter * 2 : 1.0 ;

        m_last_time = now ;
        m_last_win = win ;
        m_last_win_len = 3 * rtt ;
    }

    m_velocity_parameter = std::min(m_velocity_parameter , 32.0) ;

}

void 
TcpCp::PktsAcked(Ptr<TcpSocketState> tcb ,uint32_t segmentsAcked , const Time &rtt)
{
    Time now = Simulator::Now() ;
    // 更新 最小RTT
    if (    (m_probe_rtt_timestamp == -1) || 
            (now.GetSeconds() >= (m_probe_rtt_timestamp + m_probe_min_rtt_len)) ||
            (rtt.GetSeconds() <= m_min_rtt &&  (now.GetSeconds() < (m_probe_rtt_timestamp + m_probe_min_rtt_len))) )
    {
        m_min_rtt = rtt.GetSeconds() ;
        m_probe_rtt_timestamp = Simulator::Now().GetSeconds() ;
    }
    // 更新 rtt_standing
    if( (m_probe_rtt_standing_timestamp == -1) ||
        (now.GetSeconds() >= (m_probe_rtt_standing_timestamp + m_rtt_standing_len )) || 
        (rtt.GetSeconds() <= m_rtt_standing && (now.GetSeconds() < (m_probe_rtt_standing_timestamp + m_rtt_standing_len )))
     )
    {
        m_rtt_standing = rtt.GetSeconds() ;
        m_rtt_standing_len = 0.5 * rtt.GetSeconds() ;
        m_probe_rtt_standing_timestamp = Simulator::Now().GetSeconds() ;
    }
    // RTTstanding - minRTT
    m_delay = m_rtt_standing - m_min_rtt ;
    m_delay = std::max(0.0 , m_delay) ;
    
    m_sending_rate =  (double)tcb->m_cWnd / m_rtt_standing ;

    if(m_delay)
    {
        m_expected_sending_rate = 1.0 / (m_delta * m_delay) * 1500 ;
        m_last_expected_sending_rate = m_expected_sending_rate ;
    }
    else{
        m_expected_sending_rate = m_last_expected_sending_rate ;
    }

    Update_Velocity_Parameter(Simulator::Now().GetSeconds() , rtt.GetSeconds() , (double)tcb->m_cWnd) ;

     // 调整拥塞窗口 在 一个RTT 改变 v / delta 的 分组
    if(m_sending_rate <= m_expected_sending_rate)
    {
        std::cout << "Time\t"
                  << Simulator::Now().GetSeconds() 
                  << "\tm_sending_rate <= m_expected_sending_rate\t"
                  << "\tCopaRate\t" << std::endl ;
        double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize * m_velocity_parameter / m_delta) / tcb->m_cWnd.Get ();
        adder = std::max (1.0, adder);
        tcb->m_cWnd += static_cast<uint32_t> (adder);
    }
    else{
        std::cout << "Time\t"
                  << Simulator::Now().GetSeconds() 
                  << "\tm_sending_rate > m_expected_sending_rate\t"
                  << "\tCopaRate\t" << std::endl ;
        double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize * m_velocity_parameter / m_delta) / tcb->m_cWnd.Get ();
        adder = std::max (1.0, adder) ;
        if(tcb->m_cWnd <= static_cast<uint32_t> (10 * tcb->m_segmentSize))
        {
            tcb->m_cWnd = static_cast<uint32_t> (10 * tcb->m_segmentSize) ;
        }
        else{
            tcb->m_cWnd -= static_cast<uint32_t> (adder);
        }
    }

    
    std::cout   << "Time\t" << Simulator::Now().GetSeconds()
                << "\tCongestionWindow\t" << tcb->m_cWnd 
                << "\trtt\t" << rtt.GetSeconds()
                << "\tsengdingRate\t" << m_sending_rate
                << "\texpectedrate\t" << m_expected_sending_rate
                << "\tdelay\t" << m_delay
                << "\trttstarnding\t"<< m_rtt_standing
                << "\tinrttt" << m_min_rtt
                << "\tparam\t" << m_velocity_parameter
                << std::endl ;
}

Ptr<TcpCongestionOps>
TcpCp::Fork (void)
{
    return CopyObject<TcpCp> (this) ;
}

std::string
TcpCp::GetName() const 
{
    return "TcpCp" ;
}

}