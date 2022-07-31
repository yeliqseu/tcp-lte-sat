#ifndef TcpCopa_H
#define TcpCopa_H

#include "time.h"
#include <queue>
#include <vector>
#include <stack>
#include <algorithm>
#include "ns3/log.h"
#include <cfloat>
#include <string.h>
#include "ns3/simulator.h"
#include "ns3/tcp-congestion-ops.h"

namespace ns3{

    class TcpSocketState ;
    // class TcpSocketBase ;
    
    class TcpCopa : public TcpNewReno 
    {
        public:

            static TypeId GetTypeId(void) ;

            TcpCopa(void) ;

            TcpCopa(const TcpCopa& sock) ;

            virtual ~TcpCopa(void) ;

            std::string GetName() const ;

            virtual void PktsAcked(Ptr<TcpSocketState> tcb , uint32_t segmentsAcked , const Time& rtt) ;

            virtual void IncreaseWindow(Ptr<TcpSocketState> tcb , uint32_t segmentsAcked) ;

            void Update_Velocity_Parameter(double now , double rtt ,  double win) ;

            Ptr<TcpCongestionOps> Fork() ;

        private:

            double        m_sending_rate{0.0} ;
            double        m_expected_sending_rate{0.0} ;
            double        m_last_expected_sending_rate{0.0} ;

            double        m_min_rtt{1000.0} ;
            double        m_probe_min_rtt_len{10.0} ; // minRTT 在 10秒内的最小值
            double        m_probe_rtt_timestamp{-1.0} ;

            double        m_rtt_standing{1000.0} ;
            double        m_rtt_standing_len{0.0} ;
            double        m_probe_rtt_standing_timestamp{-1.0} ;

            double        m_delay{0.0} ;
            double        m_delta{0.5} ;

            double        m_velocity_parameter{1.0} ;

            double        m_last_win {-1.0} ;
            double        m_last_time{-1.0} ;
            double        m_last_win_len{0.0} ;

    };
}
#endif