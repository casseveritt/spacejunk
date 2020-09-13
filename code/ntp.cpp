/*
 *  ntp - compute bias for system clock based on ntp servers
 *        This is important for satellite flyovers which cannot tolerate much error in time...
 */

/* 
 Copyright (c) 2010 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following
 disclaimer.
 
 * Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials
 provided with the distribution.
 
 * The names of contributors to this software may not be used
 to endorse or promote products derived from this software
 without specific prior written permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
 
 
 Cass Everitt
 */

#include "ntp.h"
#include "r3/socket.h"
#include "r3/thread.h"
#include "r3/time.h"
#include "r3/output.h"
#include "r3/var.h"

#include <math.h>

using namespace std;
using namespace r3;

using r3::uint;

VarBool app_useNtpTimeOffset( "app_useNtpTimeOffset", "use an NTP determined time offset to compensate for clock inacacuracy", Var_Archive, true );
VarFloat app_ntpTimeOffset( "app_ntpTimeOffset", "the offset computed by NTP pings to adjust for innacurate clock", Var_Archive, 0.0f );


namespace {

	
	const uint secondsFrom1900To1970 = 0x83aa7e80;
	const double scale = double( 1LL << 32 );

	struct NtpTime {
		void Set( double t ) {
			ipart = uint( uint64( t ) + secondsFrom1900To1970 );
			fpart = uint( uint64( ( t - floor( t ) ) * scale ) );
		}
		double Get() {
			return ( ipart - secondsFrom1900To1970 ) + fpart / scale;
		}
		uint ipart;
		uint fpart;		
	};

	struct NtpPacket {
		
		void ToNetwork() {
			uint *p = (uint *)this;
			int prec = precision;
			p[0] = ( 
					( 0 << 30 ) | 
					( 3 << 27 ) |
					( 3 << 24 ) |
					( 0 << 16 ) |
					( 4 << 8  ) |
					( (prec & 0xff ) ) 
				  );
			p[1] = p[2] = 1 << 16;
			for ( int i = 0; i < 11; i++ ) {
				p[i] = htonl( p[i] );
			}
		}

		void ToHost() {
			uint *p = (uint *)this;
			for ( int i = 0; i < 11; i++ ) {
				p[i] = ntohl( p[i] );
			}
		}
		
		
		byte status;
		byte type;
		short precision;
		uint estError;
		uint estDrift;
		uint refClockId;
		NtpTime refTimestamp;
		NtpTime originateTimestamp;
		NtpTime receiveTimestamp;
		NtpTime transmitTimestamp;		
	};

	
	struct NtpThread : public r3::Thread {
		
        NtpThread() :r3::Thread( "ntp" ) {}
        
		void Run() {
			Socket s;

			NtpPacket pkt;
			memset( & pkt, 0, sizeof( pkt ) );
			
			uint addr = GetIpAddress( "pool.ntp.org" );
			
			pkt.precision = -7;
			double t0 = GetTime();
			pkt.transmitTimestamp.Set( t0 );
			
			pkt.ToNetwork();
			
			if( ! s.SendTo( addr, 123, (char *)& pkt, sizeof( pkt ) ) ) {
				Output( "NTP: SendTo failed to send to 0x%x", addr );				
				return;
			} 
			
			int bytes = s.Recv( (char *)& pkt, sizeof( pkt ) );
			if ( bytes != sizeof( pkt ) ) {
				Output( "NTP: Recv failed from 0x%x", addr );								
				return;
			}
			double t1 = GetTime();
			uint dt = uint( ( t1 - t0 ) * 1000.0 );
			double cliTime = ( t1 + t0 ) / 2.0;

			pkt.ToHost();
			double srvTime = pkt.transmitTimestamp.Get();
			double delta = srvTime - cliTime;
			Output( "NTP: Received time from host %x in %d msec: diff = %lf", addr, dt, delta );

			app_ntpTimeOffset.SetVal( delta + GetTimeOffset() );
			
			Output( "NTP: Setting time offset to %f", app_ntpTimeOffset.GetVal() );
			SetTimeOffset( app_ntpTimeOffset.GetVal() );
			
			
		}
		
	};

	NtpThread ntpThread;
	
}

namespace star3map {

	void SetTimeOffsetViaNtp() {
		if ( app_useNtpTimeOffset.GetVal() == false ) {
			return;
		}
		// use the stored time offset first, in case no network connection is available now
		Output( "NTP: Setting time offset to %f", app_ntpTimeOffset.GetVal() );
		SetTimeOffset( app_ntpTimeOffset.GetVal() );
		if ( ntpThread.running == false ) {
			ntpThread.Start();
		}
	}
	
}
