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

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <map>

using namespace std;

#include "timbl/TimblAPI.h"
#include "timblserver/SocketBasics.h"

using namespace std;
using namespace Timbl;
using namespace Sockets;

inline void usage(){
  cerr << "timblclient V0.10" << endl
       << "For demonstration purposes only!" << endl
       << "Usage:" << endl
       << "timblclient -n NodeName -p PortNumber [-i InputFile ][-o OutputFile] [--BATCH] [-b basename]"
       << endl;
}

const int TCP_BUFFER_SIZE = 2048;     // length of Internet inputbuffers,
enum CodeType { UnknownCode, Result, Error, OK, Echo, Skip,
		Neighbors, EndNeighbors, Status, EndStatus };


bool check_for_neigbors( const string& line ){
  return line.find( "NEIGHBORS" ) != string::npos;
}

inline void Split( const string& line, string& com, string& rest ){
  string::const_iterator b_it = line.begin();
  while ( b_it != line.end() && isspace( *b_it ) ) ++b_it;
  string::const_iterator m_it = b_it;
  while ( m_it != line.end() && !isspace( *m_it ) ) ++m_it;
  com = string( b_it, m_it );
  while ( m_it != line.end() && isspace( *m_it) ) ++m_it;
  rest = string( m_it, line.end() );
}  

CodeType get_code( const string& com ){
  CodeType result = UnknownCode;
  if ( compare_nocase( com, "CATEGORY" ) )
    result = Result;
  else if ( compare_nocase( com, "ERROR" ) )
    result = Error;
  else if ( compare_nocase( com, "OK" ) )
    result = OK;
  else if ( compare_nocase( com, "AVAILABLE" ) )
    result = Echo;
  else if ( compare_nocase( com, "SELECTED" ) )
    result = Echo;
  else if ( compare_nocase( com, "SKIP" ) )
    result = Skip;
  else if ( compare_nocase( com, "NEIGHBORS" ) )
    result = Neighbors;
  else if ( compare_nocase( com, "ENDNEIGHBORS" ) )
    result = EndNeighbors;
  else if ( compare_nocase( com, "STATUS" ) )
    result = Status;
  else if ( compare_nocase( com, "ENDSTATUS" ) )
    result = EndStatus;
  return result;
}


void RunClient( istream& Input, ostream& Output, 
		const string& NODE, const string& TCP_PORT, 
		bool classify_mode, const string& base ){
  bool Stop_C_Flag = false;
  cout << "Starting Client on node:" << NODE << ", port:" 
       << TCP_PORT << endl;
  ClientSocket client;
  if ( client.connect(NODE, TCP_PORT) ){
    string TestLine, ResultLine;
    string Code, Rest;
    if ( client.read( ResultLine ) ){
      cout << ResultLine << endl;
      cout << "Start entering commands please:" << endl;
      if ( !base.empty() ){
	client.write( "base " + base + "\n" );
      }
      while( !Stop_C_Flag &&
	     getline( Input, TestLine ) ){ 
	if ( classify_mode )
	  client.write( "c " );
	if ( client.write( TestLine + "\n" ) ){
	repeat:
	  if ( client.read( ResultLine ) ){
	    if ( ResultLine == "" ) goto repeat;
	    Split( ResultLine, Code, Rest );
	    switch ( get_code( Code ) ){
	    case OK:
	      Output << "OK" << endl;
	      break;
	    case Echo:
	      Output << ResultLine << endl;
	      break;
	    case Skip:
	      Output << "Skipped " << Rest << endl;
	      break;
	    case Error:
	      Output << ResultLine << endl;
	      break;
	    case Result: {
	      bool also_neighbors = check_for_neigbors( ResultLine );
	      if ( classify_mode )
		Output << TestLine << " --> ";
	      Output << ResultLine << endl;
	      if ( also_neighbors )
		while ( client.read( ResultLine ) ){
		  Split( ResultLine, Code, Rest );
		  Output << ResultLine << endl;
		  if ( get_code( Code ) == EndNeighbors )
		    break;
		}
	      break;
	    }
	    case Status:
	      Output << ResultLine << endl;
	      while ( client.read( ResultLine ) ){
		Split( ResultLine, Code, Rest );
		Output << ResultLine << endl;
		if ( get_code( Code ) == EndStatus )
		  break;
	      }
	      break;
	    default:
	      Output << "Client is confused?? " << ResultLine << endl;
	      Output << "Code was '" << Code << "'" << endl;
	      break;
	    }
	  }
	  else
	    Stop_C_Flag = true;
	}
	else 
	  Stop_C_Flag = true;
      }
    }
  }
  else {
    cerr << "connection failed: " + client.getMessage() << endl;
  }
}

int main(int argc, char *argv[] ){
  // the following trick makes it possible to parse lines from cin
  // as well from a user supplied file.
  istream *Input = &cin;
  ostream *Output = &cout;
  ifstream input_file;
  ofstream output_file;
  bool c_mode = false;
  string base;
  string node;
  string port;
  TimblOpts opts( argc, argv );
  string value;

  if ( opts.Find( "i", value ) ){
    if ( (input_file.open( value.c_str(), ios::in ), !input_file.good() ) ){
      cerr << argv[0] << " - couldn't open inputfile " << value << endl;
      exit(EXIT_FAILURE);
    }
    cout << "reading input from: " << value << endl;
    Input = &input_file;
  }
  if ( opts.Find( "o", value ) ){
    if ( (output_file.open( value.c_str(), ios::out ), !output_file.good() ) ){
      cerr << argv[0] << " - couldn't open outputfile " << value << endl;
      exit(EXIT_FAILURE);
    }
    cout << "writing output to: " << value << endl;
    Output = &output_file;
  }
  if ( opts.Find( "batch", value ) ){
    c_mode = true;
  }
  if ( opts.Find( "p", value ) ){
    port = value;
  }
  if ( opts.Find( "n", value ) ){
    node = value;
  }
  if ( opts.Find( "b", value ) ){
    base = value;
  }
  if ( !node.empty() && !port.empty() ){
    RunClient( *Input, *Output, node, port, c_mode, base );
    exit(EXIT_SUCCESS);
  }
  else {
    usage();
    exit(EXIT_FAILURE);
  }
}
