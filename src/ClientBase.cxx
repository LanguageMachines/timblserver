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

#include <string>
#include <cerrno>
#include <cstdlib>
#include <csignal>
#include "ticcutils/StringOps.h"
#include "timblserver/ClientBase.h"

using namespace std;

namespace TimblServer {

  const string TimblEntree = "Welcome to the Timbl server.";

  enum code_t { UnknownCode, Result, Err, OK, Echo, Skip,
		Neighbors, EndNeighbors, Status, EndStatus };

  ClientClass::ClientClass() {
    serverPort = -1;
  }

  ClientClass::~ClientClass(){
  }

  bool ClientClass::extractBases( const string& line ){
    string::size_type pos = line.find( "available bases:" );
    if ( pos == 0 ){
      string names = line.substr( 17 );
      //      cerr << "step 1 '" << names << "'" << endl;
      vector<string> name;
      if ( TiCC::split_at( names, name, " " ) > 0 ){
	for ( size_t i=0; i < name.size(); ++i ){
	  //	  cerr << "step 2 i= " << i << " '" << name[i] << "'" << endl;
	  bases.insert( TiCC::trim(name[i]) );
	}
	return true;
      }
    }
    cerr << "unable to extract basenames from: '" << line << "'" << endl;
    return false;
  }

  bool ClientClass::connect( const string& node,  const string& port ){
    cout << "Starting Client on node:" << node << ", port:"
	 << port << endl;
    string line;
    if ( client.connect( node, port) ){
      serverPort = TiCC::stringTo<int>( port );
      serverName = node;
      if ( client.read( line ) ){
	//	cout << "read line " << line << endl;
	if ( line == TimblEntree ){
	  client.setNonBlocking();
	  if ( client.read( line, 1 ) ){
	    // see if the server spits out information about available bases
	    // assume that these will come within a second
	    //	    cout << "next line = " << line << endl;
	    client.setBlocking();
	    if ( extractBases( line ) )
	      return true;
	  }
	  else {
	    client.setBlocking();
	    return true;
	  }
	}
      }
      else {
	cerr << client.getMessage() << endl;
      }
    }
    else {
      cerr << client.getMessage() << endl;
    }
    return false;
  }

  bool ClientClass::setBase( const string& base ){
    cout << "Setting base to :" << base << endl;
    if ( bases.find( base ) == bases.end() ){
      cerr << "'" << base << "' is not a valid base." << endl;
      return false;
    }
    string line;
    if ( client.isValid() ) {
      if ( client.write( "base " + base + "\n" ) ){
	if ( client.read( line ) ){
	  if ( line.find("selected base") != string::npos &&
	       line.find( base ) != string::npos ){
	    _base = base;
	    return true;
	  }
	  else {
	    cerr << "unexpected line = " << line << endl;
	  }
	}
      }
    }
    return false;
  }

  code_t toCode( const string& command ){
    string com = TiCC::uppercase( command );
    code_t result = UnknownCode;
    if ( com == "CATEGORY" )
      result = Result;
    else if ( com == "ERROR" )
      result = Err;
    else if ( com == "OK" )
      result = OK;
    else if ( com == "AVAILABLE" )
      result = Echo;
    else if ( com == "SELECTED" )
      result = Echo;
    else if ( com == "SKIP" )
      result = Skip;
    else if ( com == "NEIGHBORS" )
      result = Neighbors;
    else if ( com == "ENDNEIGHBORS" )
      result = EndNeighbors;
    else if ( com == "STATUS" )
      result = Status;
    else if ( com == "ENDSTATUS" )
      result = EndStatus;
    return result;
  }

  code_t Split( const string& line, string& rest ){
    string code;
    string::const_iterator b_it = line.begin();
    while ( b_it != line.end() && isspace( *b_it ) ) ++b_it;
    string::const_iterator m_it = b_it;
    while ( m_it != line.end() && !isspace( *m_it ) ) ++m_it;
    code = string( b_it, m_it );
    while ( m_it != line.end() && isspace( *m_it) ) ++m_it;
    rest = string( m_it, line.end() );
    return toCode( code );
  }

  bool ClientClass::extractResult( const string& line ){
    string cls;
    string dist;
    string db;
    string::size_type pos1 = line.find( "{" );
    if ( pos1 != string::npos ) {
      string::size_type pos2 = line.find( "}", pos1 );
      if ( pos2 != string::npos ){
	cls = line.substr( pos1+1, pos2 - pos1 -1 );
	pos1 = line.find( "DISTRIBUTION" );
	if ( pos1 != string::npos ) {
	  pos1 = line.find( "{", pos1 + 13 );
	  if ( pos1 != string::npos ) {
	    pos2 = line.find( "}", pos1 );
	    if ( pos2 != string::npos ){
	      db = line.substr( pos1, pos2 - pos1 + 1 );
	    }
	    else
	      return false;
	  }
	  else
	    return false;
	}
	pos1 = line.find( "DISTANCE" );
	if ( pos1 != string::npos ) {
	  pos1 = line.find( "{", pos1 + 9 );
	  if ( pos1 != string::npos ) {
	    pos2 = line.find( "}", pos1 );
	    if ( pos2 != string::npos ){
	      dist = line.substr( pos1+1, pos2 - pos1-1 );
	    }
	    else
	      return false;
	  }
	  else
	    return false;
	}
	pos1 = line.find( "NEIGHBORS" );
	if ( pos1 != string::npos ) {
	  string line;
	  while ( client.read( line ) ){
	    string rest;
	    code_t code = Split( line, rest );
	    if ( code != EndNeighbors ){
	      neighbors.push_back( line );
	    }
	    else
	      break;
	  }
	}
	Class = cls;
	distribution = db;
	distance = dist;
	return true;
      }
    }
    return false;
  }

  bool ClientClass::classify( const string& line ){
    Class.clear();
    distribution.clear();
    distance.clear();
    neighbors.clear();
    if ( client.isValid() ) {
      string request;
      request = "classify " + line + "\n";
      if ( client.write( request ) ){
	string response;
	while ( client.read( response ) ){
	  //	  cerr << "result line " << response << endl;
	  if ( response.empty() )
	    continue;
	  string rest;
	  code_t code = Split( response, rest );
	  switch( code ){
	  case Result:
	    return extractResult( rest );
	    break;
	  default:
	    cerr << "unexpected response '" << response << "'" << endl;
	    return false;
	  }
	}
      }
    }
    return false;
  }

  bool ClientClass::classifyFile( istream& is, ostream& os ){
    if ( client.isValid() ) {
      string line;
      while( getline( is, line) ){
	//      cerr << "Test line " << line << endl;
	if ( classify( line ) ){
	  os << line << " --> CATEGORY {" << Class << "}";
	  if ( !distribution.empty() )
	    os << " DISTRIBUTION " << distribution;
	  if ( !distance.empty() )
	    os << " DISTANCE {" << distance << "}";
	  if ( neighbors.size() > 0 ){
	    os << " NEIGHBORS " << endl;
	    for ( size_t i=0; i < neighbors.size(); ++i ){
	      os << neighbors[i] << endl;
	    }
	    os << "ENDNEIGHBORS ";
	  }
	  os << endl;
	}
	else {
	  os << line << " ==> ERROR" << endl;
	}
      }
      return true;
    }
    else {
      return false;
    }
  }

  bool ClientClass::runScript( istream& is, ostream& os ){
    if ( client.isValid() ) {
      string request;
      while( getline( is, request ) ){
	//	cerr << "script line " << request << endl;
	if ( client.write( request + "\n" ) ){
	  string response;
	  bool more = true;
	  while ( more && client.read( response ) ){
	    //	    cerr << "response line " << response << endl;
	    if ( response.empty() ){
	      continue;
	    }
	    more = false;
	    string rest;
	    code_t code = Split( response, rest );
	    switch ( code ) {
	    case OK:
	      os << "OK" << endl;
	      break;
	    case Echo:
	      os << response << endl;
	      break;
	    case Skip:
	      os << "Skipped " << rest << endl;
	      break;
	    case Err:
	      os << response << endl;
	      break;
	    case Result: {
	      bool also_neighbors = (response.find( "NEIGHBORS" ) != string::npos );
	      os << response << endl;
	      if ( also_neighbors )
		while ( client.read( response ) ){
		  code = Split( response, rest );
		  os << response << endl;
		  if ( code == EndNeighbors )
		    break;
		}
	      break;
	    }
	    case Status:
	      os << response << endl;
	      while ( client.read( response ) ){
		code = Split( response, rest );
		os << response << endl;
		if ( code == EndStatus )
		  break;
	      }
	      break;
	    default:
	      os << "Client is confused?? " << response << endl;
	      os << "Code was '" << code << "'" << endl;
	      break;
	    }
	  }
	}
	else
	  return false;
      }
      return true;
    }
    else
      return false;
  }

}
