/*
  Copyright (c) 1998 - 2016
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

#ifndef CLIENTBASE_H
#define CLIENTBASE_H

#include "timbl/TimblAPI.h"
#include "ticcutils/LogStream.h"
#include "ticcutils/SocketBasics.h"

namespace TimblServer {

  class ClientClass : public Timbl::MsgClass {
  public:
    virtual ~ClientClass();
    ClientClass();
    bool connect( const std::string&, const std::string& );
    const std::string& getBase( ) const { return _base; };
    bool setBase( const std::string& );
    const std::set<std::string> baseNames() const { return bases;};
    bool classify( const std::string& );
    bool classifyFile( std::istream&, std::ostream& );
    bool runScript( std::istream&, std::ostream& );
    std::string getClass() const { return Class; };
    std::string getDistance() const { return distance; };
    std::string getDistribution() const { return distribution; };
    const std::vector<std::string>& getNeighbors() const { return neighbors; };
  private:
    bool extractBases( const std::string& );
    bool extractResult( const std::string& );
    int serverPort;
    std::string serverName;
    Sockets::ClientSocket client;
    std::string _base;
    std::set<std::string> bases;
    std::string Class;
    std::string distance;
    std::string distribution;
    std::vector<std::string> neighbors;
  };

}
#endif // CLIENTBASE_H
