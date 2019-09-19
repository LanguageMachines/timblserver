/*
  Copyright (c) 1998 - 2019
  CLST  - Radboud University
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
      https://github.com/LanguageMachines/timblserver/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl

*/

#ifndef TIMBLSERVER_H
#define TIMBLSERVER_H

#include "timbl/TimblAPI.h"
#include "ticcutils/LogStream.h"
#include "ticcutils/SocketBasics.h"
#include "ticcutils/json.hpp"

namespace TimblServer {

  class TimblThread {
  public:
    TimblThread( Timbl::TimblExperiment *, childArgs *, bool = false );
    ~TimblThread(){ delete _exp; };
    bool setOptions( const std::string& param );
    Timbl::TimblExperiment *_exp;
    TiCC::LogStream& myLog;
    bool doDebug;
    std::ostream& os;
    std::istream& is;
  };

  class TcpServer : public TcpServerBase {
  public:
    void callback( childArgs* );
    explicit TcpServer( const TiCC::Configuration *c ): TcpServerBase( c ){};
    bool classifyLine( TimblThread *, const std::string& ) const;
  };

  class HttpServer : public HttpServerBase {
  public:
    void callback( childArgs* );
    explicit HttpServer( const TiCC::Configuration *c ): HttpServerBase( c ){};
  };

  class JsonServer : public TcpServerBase {
  public:
    void callback( childArgs* );
    explicit JsonServer( const TiCC::Configuration *c ): TcpServerBase( c ){};
    bool read_json( std::istream&, nlohmann::json& );
    nlohmann::json classify_to_json( TimblThread *,
				     const std::vector<std::string>& ) const;
  };

}

#endif // TIMBLSERVER_H
