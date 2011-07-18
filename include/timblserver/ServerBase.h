/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
  ILK   - Tilburg University
  CLiPS - University of Antwerp
 
  This file is part of timblserver

  timblserver is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  timblserver is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

#ifndef SERVERBASE_H
#define SERVERBASE_H

#include "timbl/TimblAPI.h"
#include "timbl/LogStream.h"
#include "timblserver/SocketBasics.h"

namespace TimblServer {

  class ServerClass : public Timbl::MsgClass {
    friend class TimblServerAPI;
    friend ServerClass *CreateServerPimpl( Timbl::AlgorithmType, 
					   Timbl::GetOptClass * );
  public:
    LogStream myLog;
    bool doDebug() { return debug; };
    bool doSetOptions( Timbl::TimblExperiment *, const std::string&  );
    bool classifyOneLine( Timbl::TimblExperiment *, const std::string& );
    Timbl::TimblExperiment *theExp(){ return exp; };
    virtual ~ServerClass();
    static std::string VersionInfo( bool );
    static int daemonize( int , int );
  protected:
    ServerClass();
    bool getConfig( const std::string& );
    bool startClassicServer( int, int=0 );
    bool startMultiServer( const std::string& );
    void RunClassicServer();
    void RunHttpServer();
    Timbl::TimblExperiment *splitChild() const;
    void setDebug( bool d ){ debug = d; };
    Sockets::ServerSocket *TcpSocket() const { return tcp_socket; };
    Timbl::TimblExperiment *exp;
    std::string logFile;
    std::string pidFile;
    bool doDaemon;
  private:
    bool debug;
    int maxConn;
    int serverPort;
    Sockets::ServerSocket *tcp_socket;
    std::string serverProtocol;
    std::string serverConfigFile;
    std::map<std::string, std::string> serverConfig;
  };

  Timbl::TimblExperiment *createClient( const Timbl::TimblExperiment *,
					Sockets::ServerSocket* );

  class IB1_Server: public ServerClass {
  public:
    IB1_Server( Timbl::GetOptClass * );
  };
  
  class IG_Server: public ServerClass {
  public:
    IG_Server( Timbl::GetOptClass * );
  };
 
  class TRIBL_Server: public ServerClass {
  public:
    TRIBL_Server( Timbl::GetOptClass * );
  };
  
  class TRIBL2_Server: public ServerClass {
  public:
    TRIBL2_Server( Timbl::GetOptClass * );
  };

}
#endif // SERVERBASE_H
