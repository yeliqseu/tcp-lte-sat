/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "tcp-pcc.h"
#include "tcp-socket-state.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include<iostream>
#include<cmath>
using namespace std ;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpPcc");
NS_OBJECT_ENSURE_REGISTERED (TcpPcc);

TypeId
TcpPcc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpPcc")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpPcc> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpPcc::TcpPcc ()
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpPcc::TcpPcc (const TcpPcc &sock)
  : TcpNewReno (sock)
{
    NS_LOG_FUNCTION (this);
}

TcpPcc::~TcpPcc ()
{
    NS_LOG_FUNCTION (this);
}

double  
TcpPcc::SetPacingRate(double rate , Ptr<TcpSocketState> tcb , const Time& rtt, double gain)
{
    double m_rate = (rate == 0)?(double)(3000.0 / rtt.GetSeconds()):(rate * gain);
    tcb->m_pacingRate = DataRate(m_rate) ;
    m_max_cwnd = (uint32_t)max(m_max_cwnd , (uint32_t)(tcb->m_minRtt.GetSeconds() * m_rate)) ;
    tcb->m_cWnd = m_max_cwnd ;

    std::cout << "Time\t" << Simulator::Now().GetSeconds()
              << "\ttcb\t" << tcb
              << "\tCongestionWindow\t" << (double) tcb->m_cWnd
              << std::endl ;
              
    return m_rate ;
}

double
TcpPcc::GetReward(double rate , double send_bytes , double recv_bytes)
{
    double loss_rate = (send_bytes >= recv_bytes && recv_bytes) ? (( send_bytes - recv_bytes) / (send_bytes)) : 0.0 ;
    loss_rate = std::min(0.1 , loss_rate) ;

    double part1 = rate * ( 1 - loss_rate ) * 1.0 / (1 + exp(150 * (loss_rate - 0.05))) ;
    double part2 = rate * loss_rate ;
    return part1 - part2 ;
}

void 
TcpPcc::IncreaseWindow(Ptr<TcpSocketState> tcb , uint32_t segmentsAcked)
{
    // nothing
}

void    
TcpPcc::ProcessSlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) 
{
    Time now = Simulator::Now() ;
    // 如果是 首次收到分组 ， 超过了最晚期限 ， 收到了上个MI所发送的所有分组
    if( m_start_time == 0 || 
        (double) tcb->m_rcvTimestampEchoReply / 1000.0 > m_latest_time || 
        m_send_bytes <= m_recv_bytes)
    {
        if(m_start_time == 0)
        {
            m_start_time = now.GetSeconds() ;
            m_latest_time = now.GetSeconds() + rtt.GetSeconds() ;
            m_sending_rate = SetPacingRate(0.0 , tcb , rtt , 1.0) ;
            m_send_bytes = m_sending_rate * rtt.GetSeconds() ;
            m_recv_bytes = 0 ;
        }
        else{
            double m_reward = GetReward(m_sending_rate , m_send_bytes , m_recv_bytes) ;
            // 奖励值一直在升高
            if(m_last_reward == 0 ||  (is_slow_start  && m_reward >= m_last_reward) )
            {
                m_last_reward = m_reward ;
                // 慢启动阶段
                m_last_sending_rate = m_sending_rate ;
                m_sending_rate = SetPacingRate(m_sending_rate , tcb , rtt , 4.0) ; // 2.0
                m_send_bytes = m_sending_rate * rtt.GetSeconds() ;
                m_latest_time = now.GetSeconds() + rtt.GetSeconds() ;
                m_start_time = now.GetSeconds() ;
            }
            else{
                // m_last_reward > m_reward 奖励值第一次变低
                is_slow_start = false ;
                is_making_decision = true ;
                m_last_reward = m_reward ; 
                m_start_time = m_latest_time = m_send_bytes = 0 ;
                // 调整发送速率为 原来的发送速率
                m_sending_rate = SetPacingRate(m_last_sending_rate , tcb , rtt , 1.0) ;
            }
            m_recv_bytes = 0 ;
        }
    }
    else if( m_recv_bytes < m_send_bytes && 
            (double) tcb->m_rcvTimestampEchoReply / 1000.0 >= m_start_time && 
            (double) tcb->m_rcvTimestampEchoReply / 1000.0 <= m_latest_time
            )
    {
        m_recv_bytes += (double) segmentsAcked * tcb->m_segmentSize ;
    }
    else{
        // 这些都不是 自发送 t1 - t1 + RTT 这个时间段内发送的分组
    }
}

bool
TcpPcc::Check(vector<double>& tmp_reward , Ptr<TcpSocketState> tcb,const Time& rtt )
{
    if(tmp_reward[0] >= tmp_reward[1] && tmp_reward[2] >= tmp_reward[3])
    {
        m_sending_rate = SetPacingRate(m_sending_rate , tcb , rtt , 1 + m_epislon) ;
        m_dir = 1 ;
        m_last_reward = tmp_reward[2] ;
    }
    else if (tmp_reward[0] < tmp_reward[1] && tmp_reward[2] < tmp_reward[3])
    {
        m_sending_rate = SetPacingRate(m_sending_rate ,tcb , rtt , 1 - m_epislon) ;
        m_dir = -1 ;
        m_last_reward = tmp_reward[3] ;
    }
    else{
        m_sending_rate = SetPacingRate(m_sending_rate , tcb , rtt , 1.0) ;
        m_epislon = std::min(m_epislon + 0.01 , 0.05) ;
        return false ;
    }
    return true ;
}

void  
TcpPcc::ProcessFastRCT(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) 
{
    double pre_sending_rate = m_sending_rate ;
    Time now = Simulator::Now() ;
    // 进入RCT第一阶段----- 这里采用快速方法 1 + m_epislon , 1 - m_epislon
    
    if(m_rct_start_time0 == 0.0)
    {
        m_rct_start_time0 = now.GetSeconds() ;
        m_rct_end_time0 = now.GetSeconds() + rtt.GetSeconds() ;
        m_rct_send_rate0 = SetPacingRate(pre_sending_rate , tcb , rtt , 1 + m_inc * m_epislon) ;
        m_rct_send_bytes0 = m_rct_send_rate0 * rtt.GetSeconds() ;
        m_rct_recv_bytes0 = 0 ;
    }
    else if(m_rct_start_time1 == 0 && now.GetSeconds() > m_rct_end_time0)
    {
        m_rct_start_time1 = now.GetSeconds() ;
        m_rct_end_time1 = now.GetSeconds() + rtt.GetSeconds() ;
        m_rct_send_rate1 = SetPacingRate(pre_sending_rate , tcb , rtt , 1 - m_dec * m_epislon) ;
        m_rct_send_bytes1 = m_rct_send_rate1 * rtt.GetSeconds() ;
        m_rct_recv_bytes1 = 0 ;
    }
    else{
        // nothing to do
    }

    bool flag = false ;

    if((double) tcb->m_rcvTimestampEchoReply / 1000.0 >= m_rct_start_time0 && 
       (double) tcb->m_rcvTimestampEchoReply / 1000.0 <= m_rct_end_time0)
       {
           m_rct_recv_bytes0 += (double) segmentsAcked * tcb->m_segmentSize ;
           flag = true ;
       }
    
    if((double) tcb->m_rcvTimestampEchoReply / 1000.0 >= m_rct_start_time1 && 
       (double) tcb->m_rcvTimestampEchoReply / 1000.0 <= m_rct_end_time1)
       {
           m_rct_recv_bytes1 += (double) segmentsAcked * tcb->m_segmentSize ;
           flag = true ;
       }
    
    flag = ((double) tcb->m_rcvTimestampEchoReply / 1000.0 < m_rct_start_time0 )? true : false ;

    if(!flag)
    {
        double temp_reward1 = GetReward(m_rct_send_rate0 , m_rct_send_bytes0 , m_rct_recv_bytes0) ;
        double temp_reward2 = GetReward(m_rct_send_rate1 , m_rct_send_bytes1 , m_rct_recv_bytes1) ;

        if(temp_reward1 >= temp_reward2)
        {
            m_sending_rate = SetPacingRate(m_rct_send_rate0 , tcb, rtt , 1.0) ;
            m_inc ++ , m_dec = 1 ;
        }
        else{
            m_sending_rate = SetPacingRate(m_rct_send_rate1 , tcb , rtt , 1.0) ;
            m_dec ++ , m_inc = 1 ;
        }
 
        m_rct_start_time0 = m_rct_end_time0 = m_rct_send_rate0 = m_rct_send_bytes0 = m_rct_recv_bytes0 = 0.0 ;
      
        m_rct_start_time1 = m_rct_end_time1 = m_rct_send_rate1 = m_rct_send_bytes1 = m_rct_recv_bytes1 = 0.0 ;
    }

}

// 1 + epislon ; 1 - epislon ; 1 + epislon ; 1 - epislon
void
TcpPcc::ProcessRCT(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt)
{
    double pre_sending_rate = m_sending_rate ;
    Time now = Simulator::Now() ;
    // 进入RCT第一阶段
    if(sender_endline_time.empty())
    {
        sender_start_time.push_back(now.GetSeconds()) ;
        sender_endline_time.push_back(now.GetSeconds() + rtt.GetSeconds()) ;
        double temp_rate = SetPacingRate(pre_sending_rate , tcb , rtt , 1 + m_epislon) ;
        recv_bytes.push_back(0) ;
        send_bytes.push_back(temp_rate * rtt.GetSeconds()) ;
        rate_vec.push_back(temp_rate) ;
    }
    else if(sender_endline_time.size() <= 4)
    {
        // 4个连续的MI
        if(sender_endline_time.size() < 4 && now.GetSeconds() > sender_endline_time[sender_endline_time.size() - 1])
        {
            m_cycle_index ++ ;
            sender_endline_time.push_back(now.GetSeconds() + rtt.GetSeconds()) ;
            sender_start_time.push_back(now.GetSeconds()) ;
            recv_bytes.push_back(0.0) ;
            double gain = (m_cycle_index % 2) ? (1 - m_epislon) : (1 + m_epislon) ;
            double tmp_rate = SetPacingRate(pre_sending_rate , tcb , rtt , gain) ;
            send_bytes.push_back(tmp_rate * rtt.GetSeconds()) ;
            rate_vec.push_back(tmp_rate) ;
        }
        else if(sender_endline_time.size() == 4 && now.GetSeconds() >= sender_endline_time[sender_endline_time.size() - 1])
        {
            // 把速率重新调整为 m_sending_rate
            SetPacingRate(pre_sending_rate , tcb , rtt , 1.0) ;
        }

        bool flag = false ;
        int i = 0 ;
        for( ; i <= m_cycle_index ; i ++)
        {
            if( (double) tcb->m_rcvTimestampEchoReply / 1000.0 <= sender_endline_time[i]  &&  
                (double) tcb->m_rcvTimestampEchoReply / 1000.0 >= sender_start_time[i])
            {
                flag = true ;
                recv_bytes[i] += (double) segmentsAcked * tcb->m_segmentSize ;
                break ;
            }
        }
        // 假设接受到的分组不在4个MI中 , 如果最新接收到的分组的发送时间低于第一个MI的开始发送时间 -- 不作数    true
        //                             如果最新接受到的分组的发送时间高于第四个MI的结束发送时间 -- 算 收集齐了 false
        flag = ( (double) tcb->m_rcvTimestampEchoReply / 1000.0 < sender_start_time[0]) ? true : false ;

        if(!flag)
        {
            //说明需要对四组 send_bytes 与 recv_bytes 进行计算
            vector<double> tmp_reward(4 , 0.0) ;
            for(int i = 0 ; i < 4 ; i ++){
                tmp_reward[i] = GetReward(rate_vec[i] , send_bytes[i] , recv_bytes[i]) ;
            }
            bool is_enter_fast_adjust = Check(tmp_reward , tcb , rtt) ;
            // 判断是否需要进入速率调整
            if(is_enter_fast_adjust)
            {
                is_rate_change = true ;
                is_making_decision = false ;
                // 被重新征用
                m_send_bytes = m_recv_bytes = m_start_time = m_latest_time = 0 ;
                m_epislon = 0.01 ;
            }
            else{
                // 仍然在决策阶段
                is_making_decision = true ;
            }
            // 最好都执行这些阶段
            m_cycle_index = 0 ;
            sender_endline_time.clear() ;
            sender_start_time.clear() ;
            recv_bytes.clear() ;
            send_bytes.clear() ;
            rate_vec.clear() ;
        }
    }
}

void   
TcpPcc::ProcessRateChange(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt)
{
    Time now = Simulator::Now() ;
    if(m_start_time == 0 || 
        (double) tcb->m_rcvTimestampEchoReply / 1000.0 > m_latest_time
        || m_recv_bytes >= m_send_bytes)
    {
        if(m_start_time == 0)
        {
            m_start_time = now.GetSeconds() ;
            m_latest_time = now.GetSeconds() + rtt.GetSeconds() ;
            m_last_sending_rate = m_sending_rate ;
            m_sending_rate = SetPacingRate(m_sending_rate , tcb , rtt , 1 + m_successive_cnt * m_dir * m_epislon) ;
            m_send_bytes = rtt.GetSeconds() * m_sending_rate ;
            m_recv_bytes = 0 ;
        }
        else{
            double reward = GetReward(m_sending_rate , m_send_bytes , m_recv_bytes) ;

            if(reward < m_last_reward)
            {
                // 重新进入 MakeDecision
                is_rate_change = false ;
                is_making_decision = true ;
                m_successive_cnt = 1 ;
                m_start_time = m_latest_time = m_send_bytes = m_recv_bytes = 0 ;
                // 将速率调整回原来的发送速率
                m_sending_rate = SetPacingRate(m_last_sending_rate , tcb , rtt , 1.0) ;
            }
            else{
                m_start_time = now.GetSeconds() ;
                m_latest_time = now.GetSeconds() + rtt.GetSeconds() ;
                m_successive_cnt ++ ;
                m_last_sending_rate = m_sending_rate ;
                m_sending_rate = SetPacingRate(m_sending_rate , tcb , rtt , 1 + m_successive_cnt * m_dir * m_epislon) ;
                m_send_bytes = rtt.GetSeconds() * m_sending_rate ;
            }
            m_last_reward = reward ;
            m_recv_bytes = 0 ;
        }
    }
    else if((double) tcb->m_rcvTimestampEchoReply / 1000.0 <= m_latest_time &&
            (double) tcb->m_rcvTimestampEchoReply / 1000.0 >= m_start_time 
            )
            {
                m_recv_bytes += (double) segmentsAcked * tcb->m_segmentSize ;
            }
}

void
TcpPcc::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time &rtt)
{
    NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

    if(!tcb->m_pacing)
    {
        tcb->m_pacing = true ;
    }

    if(is_slow_start)
    {
        ProcessSlowStart(tcb , segmentsAcked , rtt) ;
    }
    else if(is_making_decision)
    {
        ProcessRCT(tcb , segmentsAcked , rtt) ;
        //ProcessFastRCT(tcb , segmentsAcked , rtt) ;
    }
    else if(is_rate_change)
    {
        // 加速收敛那种
        ProcessRateChange(tcb , segmentsAcked , rtt) ;
    }
}

Ptr<TcpCongestionOps>
TcpPcc::Fork (void)
{
  return CopyObject<TcpPcc> (this);
}

std::string
TcpPcc::GetName () const
{
  return "TcpPcc";
}


} // namespace ns3
