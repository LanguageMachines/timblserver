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

#include <exception>
#include <vector>
#include <string>
#include <cstdlib>

#include "ticcutils/CommandLine.h"
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/Timer.h"
#include "timbl/TimblAPI.h"
#include "ticcutils/ServerBase.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;

using TiCC::operator<<;

inline void usage_full(void){
  cerr << "usage: timblserver [TiMBLoptions] [ServerOptions]" << endl << endl;
  cerr << "for an overwiew of all TiMBLoptions, use 'timbl -h'" << endl;
  cerr << endl;
  ServerBase::server_usage();
}

inline void usage(void){
  cerr << "usage:  timblserver --config=config-file"
       << endl;
  cerr << "or      timblserver -f data-file {-S socket} {-C num}"
       << endl;
  cerr << "or see: timblserver -h" << endl;
  cerr << "        for more options" << endl;
  cerr << endl;
}


void startExperiments( ServerBase *server ){
  map<string,TimblExperiment*> *experiments
    = static_cast<map<string, TimblExperiment*> *>(server->callback_data());
  TiCC::LogStream &s_log = server->logstream();
  map<string,string> allvals;
  if ( server->config()->hasSection("experiments") )
    allvals = server->config()->lookUpAll("experiments");
  else {
    allvals = server->config()->lookUpAll("global");
    // old style, everything is global
    // remove all already processed stuff
    map<string,string>::iterator it = allvals.begin();
    while ( it != allvals.end() ){
      if ( it->first == "port" ||
	   it->first == "protocol" ||
	   it->first == "logfile" ||
	   it->first == "debug" ||
	   it->first == "pidfile" ||
	   it->first == "daemonize" ||
	   it->first == "configDir" ||
	   it->first == "maxconn" ){
	allvals.erase(it++);
      }
      else {
	++it;
      }
    }
  }
  if ( allvals.empty() ){
    string mess = "TimblServer: Unable to initalize at least one experiment\n";
    mess += "please check your commandline or configuration file";
    throw runtime_error( mess );
  }

  map<string,string>::iterator it = allvals.begin();
  while ( it != allvals.end() ){
    string exp_name = it->first;
    TiCC::CL_Options opts;
    opts.set_short_options( timbl_short_opts + serv_short_opts );
    opts.set_long_options( timbl_long_opts + serv_long_opts );
    opts.init( it->second );
    string treeName;
    string trainName;
    string MatrixInFile = "";
    string WgtInFile = "";
    Weighting WgtType = GR;
    Algorithm algorithm = IB1;
    string ProbInFile = "";
    string value;
    if ( opts.is_present( 'a', value ) ){
      // the user gave an algorithm
      if ( !string_to( value, algorithm ) ){
	string mess = exp_name + ":start(): illegal -a value: " + value;
	throw runtime_error( mess );
      }
    }
    if ( !opts.extract( 'f', trainName ) )
      opts.extract( 'i', treeName );
    if ( opts.extract( 'u', ProbInFile ) ){
      if ( algorithm == IGTREE ){
	string mess = exp_name + ":start(): -u option is useless for IGtree";
	throw runtime_error( mess );
      }
    }
    if ( opts.extract( 'w', value ) ){
      Weighting W;
      if ( !string_to( value, W ) ){
	// No valid weighting, so assume it also has a filename
	vector<string> parts;
	size_t num = TiCC::split_at( value, parts, ":" );
	if ( num == 2 ){
	  if ( !string_to( parts[1], W ) ){
	    string mess = exp_name + ":start(): invalid weighting option:"
	      + value;
	    throw runtime_error( mess );
	  }
	  WgtInFile = parts[0];
	  WgtType = W;
	}
	else if ( num == 1 ){
	  WgtInFile = value;
	}
	else {
	  string mess = exp_name + ":start(): invalid weighting option:"
	    + value;
	  throw runtime_error( mess );
	}
      }
    }
    opts.extract( "matrixin", MatrixInFile );
    if ( !( treeName.empty() && trainName.empty() ) ){
      TimblAPI *run = new TimblAPI( opts, exp_name );
      bool result = false;
      if ( run && run->Valid() ){
	if ( treeName.empty() ){
	  s_log << "trainName = " << trainName << endl;
	  result = run->Learn( trainName );
	}
	else {
	  s_log << "treeName = " << treeName << endl;
	  result = run->GetInstanceBase( treeName );
	}
	if ( result && WgtInFile != "" ) {
	  result = run->GetWeights( WgtInFile, WgtType );
	}
	if ( result && ProbInFile != "" )
	  result = run->GetArrays( ProbInFile );
	if ( result && MatrixInFile != "" ) {
	  result = run->GetMatrices( MatrixInFile );
	}
      }
      if ( result ){
	run->initExperiment();
	TimblExperiment *exp = run->grabAndDisconnectExp();
	(*experiments)[exp_name] = exp;
	delete run;
	s_log << "started experiment " << exp_name
	      << " with parameters: " << it->second << endl;
      }
      else {
	s_log << "FAILED to start experiment " << exp_name
	      << " with parameters: " << it->second << endl;
      }
    }
    else {
      s_log << "missing '-i' or '-f' option in serverconfig entry: '"
	    << exp_name << "=" << it->second << "'" << endl;
    }
    ++it;
  }
  if ( experiments->size() == 0 ){
    s_log << "Unable to start a server. "
	  << "No valid Timbls could be instantiated" << endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]){
  try {
    // Start.
    //
    cerr << "TiMBL Server " << TimblServer::Version()
	 << " (c) CLTS/ILK/CLIPS 1998 - 2019.\n"
	 << "Tilburg Memory Based Learner\n"
	 << "Centre for Language and Speech Technology, Radboud University\n"
	 << "Induction of Linguistic Knowledge Research Group, Tilburg University\n"
	 << "CLiPS Computational Linguistics Group, University of Antwerp\n"
	 << "based on " << Timbl::VersionName() << endl;
    cerr << TiCC::Timer::now();
    if ( argc <= 1 ){
      usage();
      return 1;
    }

    TiCC::CL_Options opts;
    opts.set_short_options( timbl_short_opts + serv_short_opts );
    opts.set_long_options( timbl_long_opts + serv_long_opts );
    opts.init( argc, argv );
    if ( opts.is_present( 'h' )
	 || opts.is_present( "help" ) ){
      usage_full();
      exit(EXIT_SUCCESS);
    }
    if ( opts.is_present( 'V' )
	 || opts.is_present( "version" ) ){
      cerr << "TiMBL server " << ServerBase::VersionInfo( true ) << endl;
      cerr << "Based on TiMBL " << TimblAPI::VersionInfo( true ) << endl;
      exit(EXIT_SUCCESS);
    }

    opts.insert( 'v', "F", true );
    opts.insert( 'v', "S", false );
    TiCC::Configuration *config = initServerConfig( opts );
    if ( !config ){
      exit(EXIT_FAILURE);
    }
    ServerBase *server = 0;
    string protocol = config->lookUp( "protocol" );
    if ( protocol.empty() ){
      protocol = "tcp";
    }
    if ( protocol == "tcp" ){
      server = new TcpServer( config );
      server->logstream().message("tcp_server");
    }
    else if ( protocol == "http" ){
      server = new HttpServer( config );
      server->logstream().message("http_server");
    }
    else if ( protocol == "json" ){
      server = new JsonServer( config );
      server->logstream().message("json_server");
    }
    else {
      cerr << "unknown protocol " << protocol << endl;
      exit(EXIT_FAILURE);
    }
    startExperiments( server );
    return server->Run(); // returns EXIT_SUCCESS or EXIT_FAIL
  }
  catch( const std::bad_alloc& ){
    cerr << "ran out of memory somewhere" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  catch( const std::exception& e ){
    cerr << "a standard exception was raised: '" << e.what() << "'" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  catch(...){
    cerr << "some unhandled exception was raised" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  return 0;
}
