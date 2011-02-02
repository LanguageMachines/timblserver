/*
  Copyright (c) 1998 - 2011
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Timbl

  Timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/

#include <exception>
#include <list>
#include <vector>
#include <iosfwd>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>

#include "timbl/TimblAPI.h"
#include "timbl/Options.h"
#include "timblserver/ServerBase.h"
#include "timblserver/TimblServerAPI.h"

using namespace std;
using namespace Timbl;

static list<string> ind_lines;

Algorithm algorithm;

bool Do_Server = false;
bool Do_Multi_Server = false;
int ServerPort = -1;
int Max_Connections = 10;

string I_Path;
string Q_value;
string dataFile;
string ServerConfigFile;
string MatrixInFile;
string TreeInFile;
string WgtInFile;
Weighting WgtType = UNKNOWN_W;
string ProbInFile;

inline void usage_full(void){
  cerr << "usage: TimblServer [TiMBLoptions] [ServerOptions]" << endl << endl;
  cerr << "for an overwiew of all TiMBLoptions, use 'Timbl -h'" << endl;
  cerr << endl;
  cerr << "Server options" << endl;
  cerr << "--config=<f> read server settings from file <f>" << endl; 
  cerr << "--pidfile=<f> store pid in file <f>" << endl; 
  cerr << "--logfile=<f> log server activity in file <f>" << endl; 
  cerr << "--daemonize=[yes|no] (default yes)" << endl << endl;
  cerr << "-S <port> : run as a server on <port>" << endl;
  cerr << "-C <num>  : accept a maximum of 'num' parallel connections (default 10)" << endl;
}

inline void usage(void){
  cerr << "usage:  TimblServer --config=config-file"
       << endl;
  cerr << "or      TimblServer -f data-file {-S socket} {-C num}"
       << endl;
  cerr << "or see: TimblServer -h" << endl;
  cerr << "        for more options" << endl;
  cerr << endl;
}

void get_command_lines( const string& value, list<string>& result ){
  result.clear();
  ifstream ind( value.c_str()+1 ); // skip @ 
  if ( ind.bad() ){
    cerr << "Problem reading command-lines from file '" 
	 << value << "'" << endl;
    throw( "command line failure" );
  }
  string Buf;
  while ( getline( ind, Buf ) ){
    if ( Buf.empty() )
      continue;
    result.push_back( Buf );
  }
}

string correct_path( const string& filename,
		     const string& path,
		     bool keep_origpath ){  
  // if filename contains pathinformation, it is replaced with path, except
  // when keep_origpath is true. 
  // if filename contains NO pathinformation, path is always appended.
  // of course we don't append if the filename is empty or just '-' !
  
  if ( path != "" && filename != "" && filename[0] != '-' ){
    bool add_slash = path[path.length()] != '/';
    string tmp;
    string::size_type pos = filename.rfind( '/' );
    if ( pos == string::npos ){
      tmp = path;
      if ( add_slash )
	tmp += "/";
      tmp += filename;
    }
    else { 
      tmp = path;
      if ( add_slash )
	tmp += "/";
      if ( !keep_origpath ){
	tmp += filename.substr( pos+1 );
      }
      else 
	tmp += filename;
    }
    return tmp;
  }
  else
    return filename;
}

class softExit : public exception {};

void Preset_Values( TimblOpts& Opts ){
  bool mood;
  string value;
  if ( Opts.Find( 'h', value, mood ) ){
    usage_full();
    throw( softExit() );
  }
  if ( Opts.Find( 'V', value, mood ) ){
    cerr << "TiMBL server " << TimblServerAPI::VersionInfo( true ) << endl;
    cerr << "Based on TiMBL " << TimblAPI::VersionInfo( true ) << endl;
    throw( softExit() );
  }
  if ( Opts.Find( 'a', value, mood ) ){
    // the user gave an algorithm
    if ( !string_to( value, algorithm ) ){
      cerr << "illegal -a value: " << value << endl;
      throw( softExit() ); // no chance to proceed
    }
    Opts.Delete( 'a' );
  }
  else
    algorithm = IB1; // general default
  Opts.Add( 'a', to_string( algorithm ), false );
  if ( Opts.Find( 'P', value, mood ) ){
    I_Path = value;
    Opts.Delete( 'P' );
  }
  if ( Opts.Find( 'f', value, mood ) ){
    dataFile = correct_path( value, I_Path, true );
    Opts.Delete( 'f' );
  }
  if ( Opts.Find( 'q', value, mood ) ){
    Q_value = value;
  }
  Opts.Add( 'v', "F", true );
  Opts.Add( 'v', "S", false );
  if ( Opts.Find( "config", value, mood ) ){
    ServerConfigFile = correct_path( value, I_Path, true );
    Opts.Delete( "config" );
    Do_Multi_Server = true;
  }
  if ( Opts.Find( 'S', value, mood ) ){
    if ( Do_Multi_Server ){
      cerr << "options -S conflicts with option --config" << endl;
      throw( softExit() );
    }
    else {
      Do_Server = true;
      ServerPort = stringTo<int>( value );
      if ( ServerPort < 1 || ServerPort > 100000 ){
	cerr << "-S option, portnumber invalid: " << ServerPort << endl;
	throw( softExit() );
      }
    }
  }
  else {
    if ( Do_Multi_Server ){
      Opts.Add( 'S', "0", true );
      // hack to signal GetOptClass that we are going into server mode
    }
  }
  if ( Opts.Find( 'C', value, mood ) ){
    if ( Do_Multi_Server ){
      cerr << "-C must be specified in the configfile" << endl;
      throw( softExit() );
    }
    if ( !Do_Server ){
      cerr << "-C option invalid without -S" << endl;
      throw( softExit() );
    }
    Max_Connections = stringTo<int>( value );
    if ( Max_Connections < 1 || Max_Connections > 1000 ){
      cerr << "-C options, max number of connection invalid: " 
	   << Max_Connections << endl;
    }
    Opts.Delete( 'C' );
  }
  Weighting W = GR;
  // default Weighting = GainRatio
  if ( Opts.Find( 'w', value, mood ) ){
    // user specified weighting
    if ( !string_to( value, W ) )
      // no valid weight, hopefully a filename
      return;
    else
      // valid Weight, but maybe a number, so replace
      Opts.Delete( 'w' );
  }
  Opts.Add( 'w', to_string(W), false );
}

void Adjust_Default_Values( TimblOpts& Opts ){
  bool mood;
  string value;
  if ( !Opts.Find( 'm', value, mood ) ){
    Opts.Add( 'm', "O", false );
    // Default Metric = Overlap
  }
}

bool next_test( string& line ){
  bool result = false;
  line = "";
  if ( !ind_lines.empty() ){
    line = ind_lines.front();
    ind_lines.pop_front();
    result = true;
  }
  return result;
}

bool get_file_names( TimblOpts& Opts ){
  MatrixInFile = "";
  TreeInFile = "";
  WgtInFile = "";
  WgtType = UNKNOWN_W;
  ProbInFile = "";
  string value;
  bool mood;
  if ( Opts.Find( 'P', value, mood ) ||
       Opts.Find( 'f', value, mood ) ){
    cerr << "illegal option, value = " << value << endl;
    return false;
  }
  if ( Opts.Find( "matrixin", value, mood ) ){
    MatrixInFile = correct_path( value, I_Path, true );
    Opts.Delete( "matrixin" );
  }
  if ( Opts.Find( 'i', value, mood ) ){
    TreeInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'i' );
  }
  if ( Opts.Find( 'u', value, mood ) ){
    if ( algorithm == IGTREE ){
      cerr << "-u option is useless for IGtree" << endl;
      return false;
    }
    ProbInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'u' );
  }
  if ( Opts.Find( 'w', value, mood ) ){
    Weighting W;
    if ( !string_to( value, W ) ){
      // No valid weighting, so assume it also has a filename
      vector<string> parts;
      size_t num = split_at( value, parts, ":" );
      if ( num == 2 ){
	if ( !string_to( parts[1], W ) ){
	  cerr << "invalid weighting option: " << value << endl;
	  return false;
	}
	WgtInFile = correct_path( parts[0], I_Path, true );
	WgtType = W;
	Opts.Delete( 'w' );
      }
      else if ( num == 1 ){
	WgtInFile = correct_path( value, I_Path, true );
	Opts.Delete( 'w' );
      } 
      else {
	cerr << "invalid weighting option: " << value << endl;
	return false;
      }
    }
  }
  return true;
}

bool checkInputFile( const string& name ){
  if ( !name.empty() ){
    ifstream is( name.c_str() );
    if ( !is.good() ){
      cerr << "unable to find or use input file '" << name << "'" << endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]){
  try {
    struct tm *curtime;
    time_t Time;
    // Start.
    //
    cerr << "TiMBL Server " << TimblServerAPI::VersionInfo()
	 << " (c) ILK 1998 - 2011.\n" 
	 << "Tilburg Memory Based Learner\n"
	 << "Induction of Linguistic Knowledge Research Group, Tilburg University\n"
	 << "CLiPS Computational Linguistics Group, University of Antwerp" << endl;
    time(&Time);
    curtime = localtime(&Time);
    cerr << asctime(curtime) << endl;
    if ( argc <= 1 ){
      usage();
      return 1;
    }
    TimblOpts Opts( argc, argv );
    Preset_Values( Opts );
    Adjust_Default_Values( Opts );
    if ( !get_file_names( Opts ) )
      return 2;
    TimblServerAPI *Run = new TimblServerAPI( &Opts );
    if ( !Run->Valid() ){
      delete Run;
      usage();
      return 3;
    }
    if ( Do_Server ){
      // Special case:   running a classic Server
      if ( !checkInputFile( TreeInFile ) ||
	   !checkInputFile( dataFile ) ||
	   !checkInputFile( WgtInFile ) ||
	   !checkInputFile( MatrixInFile ) ||
	   !checkInputFile( ProbInFile ) ){
	delete Run;
	return 3;
      }
      if ( TreeInFile != "" ){
	if ( !Run->GetInstanceBase( TreeInFile ) ){
	  delete Run;
	  return 3;
	}
      }
      else {
	if ( !Run->Learn( dataFile ) ){
	  delete Run;
	  return 3;
	}
      }
      if ( WgtInFile != "" ) {
	Run->GetWeights( WgtInFile, WgtType );
      }
      if ( ProbInFile != "" )
	Run->GetArrays( ProbInFile );
      if ( MatrixInFile != "" ) {
	Run->GetMatrices( MatrixInFile );
      }
      if ( Run->StartServer( ServerPort, Max_Connections ) ){
	delete Run;
	return 0;
      }
      else
	cerr << "starting a server failed" << endl;
    }
    else if ( Do_Multi_Server ){
      if ( !checkInputFile( ServerConfigFile ) ){
	delete Run;
	return 3;
      }
      if ( !Run->StartMultiServer( ServerConfigFile ) ){
	cerr << "starting a MultiServer failed" << endl;
      }
      else
	delete Run;
    }
    return 0;
  }
  catch(std::bad_alloc){
    cerr << "ran out of memory somewhere" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl;
  }
  catch( softExit& e ){
    return 0;
  }
  catch(std::string& what){
    cerr << "an exception was raised: '" << what << "'" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
  catch(std::exception& e){
    cerr << "a standard exception was raised: '" << e.what() << "'" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
  catch(...){
    cerr << "some exception was raised" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
  return 0;
}
