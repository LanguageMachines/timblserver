/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2012
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
#include "timblserver/ClientBase.h"

using namespace std;
using namespace Timbl;
using namespace Sockets;

inline void usage(){
  cerr << "timblclient V0.10" << endl
       << "For demonstration purposes only!" << endl
       << "Usage:" << endl
       << "timblclient -n NodeName -p PortNumber [-i InputFile ][-o OutputFile] [--batch] [-b basename]"
       << endl;
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
    TimblServer::ClientClass client;
    if ( !client.connect( node, port ) ){
      cerr << "connection failed " << endl;
      exit(EXIT_FAILURE);      
    }
    if ( !base.empty() ){
      if ( !client.setBase( base ) ){
	cerr << "setbase failed";
	exit(EXIT_FAILURE);
      }
    }
    if ( c_mode ){
      if ( !client.classifyFile( *Input, *Output ) ){
	cerr << "classification failed." << endl;
	exit(EXIT_FAILURE);
      }
    }
    else {
      if ( !client.runScript( *Input, *Output ) ){
	cerr << "running script failed." << endl;
	exit(EXIT_FAILURE);
      }
    }
    exit(EXIT_SUCCESS);
  }
  else {
    usage();
    exit(EXIT_FAILURE);
  }
}
