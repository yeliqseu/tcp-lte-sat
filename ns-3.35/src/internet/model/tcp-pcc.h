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
#ifndef TCPPCC_H
#define TCPPCC_H

#include "tcp-congestion-ops.h"
#include "ns3/traced-value.h"
#include <iostream>
#include <vector>
using namespace std ;

namespace ns3 {

class TcpSocketState;

class TcpPcc : public TcpNewReno
{
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);

    /**
     * Create an unbound tcp socket.
     */
    TcpPcc (void);

    /**
     * \brief Copy constructor
     * \param sock the object to copy
     */
    TcpPcc (const TcpPcc& sock);

    virtual ~TcpPcc (void) override;

    // Inherited
    virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                            const Time& rtt) override;
    virtual std::string GetName () const override;
    virtual Ptr<TcpCongestionOps> Fork () override;

    virtual void IncreaseWindow(Ptr<TcpSocketState> tcb , uint32_t segmentsAcked)override;


private:
    void    ProcessSlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) ;

    void   ProcessRCT(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) ;

    void   ProcessFastRCT(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) ;

    void   ProcessRateChange(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt) ;


    double  SetPacingRate(double rate , Ptr<TcpSocketState> tcb , const Time& rtt , double gain) ;

    double  GetReward(double rate , double send_bytes , double recv_bytes) ;

    bool   Check(vector<double>& tmp_reward , Ptr<TcpSocketState> tcb,const Time& rtt ) ;

    bool   is_slow_start{true} ;
    bool   is_making_decision{false} ;
    bool   is_rate_change{false} ;
    // 用于慢启动阶段 和 速率调整阶段
    double  m_start_time{0.0} ;
    double  m_latest_time{0.0} ;
    double  m_send_bytes{0.0} ;
    double  m_recv_bytes{0.0} ;

    double  m_sending_rate{0.0} ;
    double  m_last_sending_rate{0.0} ;
    double  m_last_reward{0.0} ;

    int     m_cycle_index{0} ;
    double  m_epislon{0.01} ;

    double  m_dir{-1} ;
    double  m_successive_cnt{1} ;

    vector<double> sender_endline_time ;
    vector<double> sender_start_time ;
    vector<double> recv_bytes;
    vector<double> send_bytes;
    vector<double> rate_vec;

    // 快速RCT方式 
    double  m_rct_send_rate0{0.0} ;
    double  m_rct_send_rate1{0.0} ;
    double  m_rct_start_time0{0.0} ;
    double  m_rct_start_time1{0.0} ;
    double  m_rct_end_time0{0.0} ;
    double  m_rct_end_time1{0.0} ;
    double  m_rct_send_bytes0{0.0} ;
    double  m_rct_send_bytes1{0.0} ;
    double  m_rct_recv_bytes0{0.0} ;
    double  m_rct_recv_bytes1{0.0} ;

    int    m_inc{1} ;
    int    m_dec{1} ;

    uint32_t   m_max_cwnd{0} ;
    
};

} // namespace ns3

#endif // TcpPcc_H
