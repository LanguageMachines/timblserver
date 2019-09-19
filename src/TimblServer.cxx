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
#include "timbl/GetOptClass.h"
#include "ticcutils/json.hpp"
#include "ticcutils/ServerBase.h"
#include "ticcutils/XMLtools.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;
using namespace nlohmann;

using TiCC::operator<<;

#define DBG *TiCC::Dbg(myLog)
#define LOG *TiCC::Log(myLog)

inline void usage_full(void){
  cerr << "usage: timblserver [TiMBLoptions] [ServerOptions]" << endl << endl;
  cerr << "for an overwiew of all TiMBLoptions, use 'timbl -h'" << endl;
  cerr << endl;
  cerr << "Server options" << endl;
  cerr << "--config=<f> read server settings from file <f>" << endl;
  cerr << "--pidfile=<f> store pid in file <f>" << endl;
  cerr << "--logfile=<f> log server activity in file <f>" << endl;
  cerr << "--daemonize=[yes|no] (default yes)" << endl << endl;
  cerr << "--protocol=[tcp|http|json] (default tcp)" << endl << endl;
  cerr << "-S <port> : run as a server on <port>" << endl;
  cerr << "-C <num>  : accept a maximum of 'num' parallel connections (default 10)" << endl;
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
  map<string,string> allvals;
  if ( server->config->hasSection("experiments") )
    allvals = server->config->lookUpAll("experiments");
  else {
    allvals = server->config->lookUpAll("global");
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

  auto experiments = new map<string, TimblExperiment*>();
  server->callback_data = experiments;

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
	  server->myLog << "trainName = " << trainName << endl;
	  result = run->Learn( trainName );
	}
	else {
	  server->myLog << "treeName = " << treeName << endl;
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
	(*experiments)[exp_name] = run->grabAndDisconnectExp();
	delete run;
	server->myLog << "started experiment " << exp_name
		      << " with parameters: " << it->second << endl;
      }
      else {
	server->myLog << "FAILED to start experiment " << exp_name
		      << " with parameters: " << it->second << endl;
      }
    }
    else {
      server->myLog << "missing '-i' or '-f' option in serverconfig entry: '"
		    << exp_name << "=" << it->second << "'" << endl;
    }
    ++it;
  }
  if ( experiments->size() == 0 ){
    server->myLog << "Unable to start a server. "
		    << "No valid Timbls could be instantiated" << endl;
    exit(EXIT_FAILURE);
  }
}

class TcpServer : public TcpServerBase {
public:
  void callback( childArgs* );
  explicit TcpServer( const TiCC::Configuration *c ): TcpServerBase( c ){};
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
  bool read_json( istream&, json& );
};

TimblExperiment *createClient( const TimblExperiment *exp,
			       childArgs *args ){
  TimblExperiment *result = exp->clone();
  *result = *exp;
  if ( !result->connectToSocket( &(args->os() ) ) ){
    cerr << "unable to create working client" << endl;
    return 0;
  }
  if ( exp->getOptParams() ){
    result->setOptParams( exp->getOptParams()->Clone( &(args->os() ) ) );
  }
  result->setExpName(string("exp-")+TiCC::toString( args->id() ) );
  return result;
}

enum CommandType { UnknownCommand, Classify, Base,
		   Query, Set, Exit, Comment };

CommandType check_command( const string& com ){
  CommandType result = UnknownCommand;
  if ( compare_nocase_n( com, "CLASSIFY" ) )
    result = Classify;
  else if ( compare_nocase_n( com, "QUERY" ) )
    result = Query;
  else if ( compare_nocase_n( com, "BASE") )
    result = Base;
  else if ( compare_nocase_n( com, "SET") )
    result = Set;
  else if ( compare_nocase_n( com, "EXIT" ) )
    result = Exit;
  else if ( com[0] == '#' )
    result = Comment;
  return result;
}

inline void Split( const string& line, string& com, string& rest ){
  vector<string> parts = TiCC::split( line, 2 );
  if ( parts.size() == 2 ){
    com = parts[0];
    rest = parts[1];
  }
  else {
    com = parts[0];
    rest = "";
  }
}

class TimblThread {
public:
  TimblThread( TimblExperiment *, childArgs * );
  ~TimblThread(){ delete _exp; };
  bool classifyLine( const string& ) const;
  json classify_to_json( const vector<string>& ) const;
  bool setOptions( const string& param );
  TimblExperiment *_exp;
private:
  TiCC::LogStream& myLog;
  bool doDebug;
  ostream& os;
  istream& is;
};

TimblThread::TimblThread( TimblExperiment *exp,
			  childArgs* args ):
  myLog(args->logstream()),
  doDebug(args->debug()),
  os(args->os()),
  is(args->is())
{
  if ( doDebug ){
    myLog.setlevel(LogHeavy);
  }
  _exp = exp->clone();
  *_exp = *exp;
  if ( !_exp->connectToSocket( &(args->os() ) ) ){
    throw logic_error( "unable to create working client" );
  }
  if ( exp->getOptParams() ){
    _exp->setOptParams( exp->getOptParams()->Clone( &(args->os() ) ) );
  }
  _exp->setExpName(string("exp-")+TiCC::toString( args->id() ) );
}

bool TimblThread::setOptions( const string& param ){
  if ( _exp->SetOptions( param )
       && _exp->ConfirmOptions() ){
    return true;
  }
  return false;
}

bool TimblThread::classifyLine( const string& params ) const {
  double Distance;
  string Distrib;
  string Answer;
  if ( _exp->Classify( params, Answer, Distrib, Distance ) ){
    DBG << _exp->ExpName() << ":" << params << " --> "
		<< Answer << " " << Distrib
		<< " " << Distance << endl;
    os << "CATEGORY {" << Answer << "}";
    if ( os.good() ){
      if ( _exp->Verbosity(DISTRIB) ){
	os << " DISTRIBUTION " <<Distrib;
      }
    }
    if ( os.good() ){
      if ( _exp->Verbosity(DISTANCE) ){
	os << " DISTANCE {" << Distance << "}";
      }
    }
    if ( os.good() ){
      if ( _exp->Verbosity(MATCH_DEPTH) ){
	os << " MATCH_DEPTH {" << _exp->matchDepth() << "}";
      }
    }
    if ( os.good() ){
      if ( _exp->Verbosity(CONFIDENCE) ){
	os << " CONFIDENCE {" <<_exp->confidence() << "}";
      }
    }
    if ( os.good() ){
      if ( _exp->Verbosity(NEAR_N) ){
	os << " NEIGHBORS" << endl;
	_exp->showBestNeighbors( os );
	os << "ENDNEIGHBORS";
      }
    }
    if ( os.good() )
      os << endl;
    return os.good();
  }
  else {
    DBG << _exp->ExpName() << ": Classify Failed on '"
		<< params << "'" << endl;
    return false;
  }
}

json TimblThread::classify_to_json( const vector<string>& params ) const {
  DBG << "classify_to_json(" << params << ")" << endl;
  double distance;
  string distrib;
  string answer;
  json result;
  if ( params.size() > 1 ){
    result = json::array();
  }
  for ( const auto& param : params ){
    json out_json;
    if ( _exp->Classify( param, answer, distrib, distance ) ){
      DBG << _exp->ExpName() << ":" << param << " --> "
	  << answer << " " << distrib << " " << distance << endl;
      out_json["category"] = answer;
      if ( _exp->Verbosity(DISTRIB) ){
	out_json["distribution"] = distrib;
      }
      if ( _exp->Verbosity(DISTANCE) ){
	out_json["distance"] = distance;
      }
      if ( _exp->Verbosity(MATCH_DEPTH) ){
	out_json["match_depth"] = _exp->matchDepth();
      }
      if ( _exp->Verbosity(CONFIDENCE) ){
	out_json["confidence"] = _exp->confidence();
      }
      if ( _exp->Verbosity(NEAR_N) ){
	json tmp = _exp->best_neighbors_to_JSON();
	if ( !tmp.empty() ){
	  out_json["neighbors"] = tmp;
	}
      }
    }
    else {
      DBG << _exp->ExpName() << ": Classify Failed on '"
	  << param << "'" << endl;
      out_json["error"] = "timbl:classify(" + param + ") failed";
    }
    DBG << "created json: " << out_json.dump(2) << endl;
    if ( params.size() > 1 ){
      result.push_back( out_json );
    }
    else {
      result = out_json;
    }
  }
  return result;
}


void TcpServer::callback( childArgs *args ){
  string Line;
  int sockId = args->id();
  TimblThread *client = 0;
  map<string, TimblExperiment*> *experiments =
    static_cast<map<string, TimblExperiment*> *>(callback_data);

  int result = 0;
  args->os() << "Welcome to the Timbl server." << endl;
  if ( experiments->size() == 1
       && experiments->find("default") != experiments->end() ){
    DBG << " Voor Create Default Client " << endl;
    client = new TimblThread( (*experiments)["default"], args );
    DBG << " Na Create Client " << endl;
    // report connection to the server terminal
    //
  }
  else {
    args->os() << "available bases: ";
    map<string,TimblExperiment*>::const_iterator it = experiments->begin();
    while ( it != experiments->end() ){
      args->os() << it->first << " ";
      ++it;
    }
    args->os() << endl;
  }
  if ( getline( args->is(), Line ) ){
    DBG << "TcpServer::FirstLine='" << Line << "'" << endl;
    string Command, Param;
    bool go_on = true;
    DBG << "TcpServer::running FromSocket: " << sockId << endl;

    do {
      Line = TiCC::trim( Line );
      DBG << "TcpServer::Line='" << Line << "'" << endl;
      Split( Line, Command, Param );
      DBG << "TcpServer::Command='" << Command << "'" << endl;
      DBG << "TcpServer::Param='" << Param << "'" << endl;
      switch ( check_command(Command) ){
      case Base:{
	map<string,TimblExperiment*>::const_iterator it
	  = experiments->find(Param);
	if ( it != experiments->end() ){
	  args->os() << "selected base: '" << Param << "'" << endl;
	  if ( client )
	    delete client;
	  DBG << "TcpServer::before Create Default Client " << endl;
	  client = new TimblThread( it->second, args );
	  DBG << " TcpServer::After Create Client " << endl;
	  // report connection to the server terminal
	  //
	  char line[256];
	  sprintf( line, "Thread %lu, on Socket %d",
		   (uintptr_t)pthread_self(), sockId );
	  LOG << line << ", started." << endl;
	}
	else {
	  args->os() << "ERROR { Unknown basename: " << Param << "}" << endl;
	}
      }
	break;
      case Set:
	if ( !client ){
	  args->os() << "you haven't selected a base yet!" << endl;
	}
	else if ( client->setOptions( Param ) ){
	  DBG << "TcpServer::setOptions: " << Param << endl;
	  args->os() << "OK" << endl;
	}
	else {
	  args->os() << "ERROR { set options failed: " << Param << "}" << endl;
	}
	break;
      case Query:
	if ( !client )
	  args->os() << "you haven't selected a base yet!" << endl;
	else {
	  args->os() << "STATUS" << endl;
	  client->_exp->ShowSettings( args->os() );
	  args->os() << "ENDSTATUS" << endl;
	}
	break;
      case Exit:
	args->os() << "OK Closing" << endl;
	go_on = false;
	break;
      case Classify:
	if ( !client ){
	  args->os() << "you haven't selected a base yet!" << endl;
	}
	else {
	  if ( client->classifyLine( Param ) ){
	    result++;
	  }
	  go_on = true; // HACK?
	}
	break;
      case Comment:
	args->os() << "SKIP '" << Line << "'" << endl;
	break;
      default:
	DBG << sockId << "TcpServer::Don't understand '"
		    << Line << "'" << endl;
	args->os() << "ERROR { Illegal instruction:'" << Command
		   << "' in line:" << Line << "}" << endl;
	break;
      }
    }
    while ( go_on && getline( args->is(), Line ) );
  }
  delete client;
  LOG << "Thread " << (uintptr_t)pthread_self()
	      << " terminated, " << result
	      << " instances processed " << endl;
}

#define IS_DIGIT(x) (((x) >= '0') && ((x) <= '9'))
#define IS_HEX(x) ((IS_DIGIT(x)) || (((x) >= 'a') && ((x) <= 'f')) || \
            (((x) >= 'A') && ((x) <= 'F')))


string urlDecode( const string& s ) {
  string result;
  size_t len=s.size();
  for ( size_t i=0; i<len ; ++i ) {
    int cc=s[i];
    if (cc == '+') {
      result += ' ';
    }
    else if ( cc == '%' &&
	      ( i < len-2 &&
		( IS_HEX(s[i+1]) ) &&
		( IS_HEX(s[i+2]) ) ) ){
      std::istringstream ss( "0x"+s.substr(i+1,2) );
      int tmp;
      ss >> std::showbase >> std::hex;
      ss >> tmp;
      result = result + (char)tmp;
      i += 2;
    }
    else {
      result += cc;
    }
  }
  return result;
}

void HttpServer::callback( childArgs *args ){
  // process the test material
  // report connection to the server terminal
  //
  args->socket()->setNonBlocking();
  map<string, TimblExperiment*> *experiments =
    static_cast<map<string, TimblExperiment*> *>(callback_data);
  char logLine[256];
  sprintf( logLine, "Thread %lu, on Socket %d", (uintptr_t)pthread_self(),
	   args->id() );
  LOG << logLine << ", started." << endl;
  string Line;
  int timeout = 1;
  if ( nb_getline( args->is(), Line, timeout ) ){
    DBG << "HttpServer::FirstLine='" << Line << "'" << endl;
    if ( Line.find( "HTTP" ) != string::npos ){
      // skip HTTP header
      string tmp;
      timeout = 1;
      while ( ( nb_getline( args->is(), tmp, timeout ), !tmp.empty()) ){
	//	    cerr << "skip: read:'" << tmp << "'" << endl;;
      }
      string::size_type spos = Line.find( "GET" );
      if ( spos != string::npos ){
	string::size_type epos = Line.find( " HTTP" );
	string line = Line.substr( spos+3, epos - spos - 3 );
	DBG << "HttpServer::Line='" << line << "'" << endl;
	epos = line.find( "?" );
	string basename;
	if ( epos != string::npos ){
	  basename = line.substr( 0, epos );
	  string qstring = line.substr( epos+1 );
	  epos = basename.find( "/" );
	  if ( epos != string::npos ){
	    basename = basename.substr( epos+1 );
	    map<string,TimblExperiment*>::const_iterator it= experiments->find(basename);
	    if ( it != experiments->end() ){
	      TimblExperiment *api = createClient( it->second, args );
	      if ( api ){
		TiCC::LogStream LS( &myLog );
		TiCC::LogStream DS( &myLog );
		DS.message(logLine);
		LS.message(logLine);
		DS.setstamp( StampBoth );
		LS.setstamp( StampBoth );
		TiCC::XmlDoc doc( "TiMblResult" );
		xmlNode *root = doc.getRoot();
		TiCC::XmlSetAttribute( root, "algorithm", TiCC::toString(api->Algorithm()) );
		vector<string> avs = TiCC::split_at( qstring, "&" );
		if ( !avs.empty() ){
		  multimap<string,string> acts;
		  for ( const auto& av : avs ){
		    vector<string> parts = TiCC::split_at( av, "=", 2 );
		    if ( parts.size() == 2 ){
		      acts.insert( make_pair(parts[0], parts[1]) );
		    }
		    else {
		      LS << "unknown word in query "
			 << av << endl;
		    }
		  }
		  typedef multimap<string,string>::const_iterator mmit;
		  pair<mmit,mmit> range = acts.equal_range( "set" );
		  mmit it = range.first;
		  while ( it != range.second ){
		    string opt = it->second;
		    if ( !opt.empty() && opt[0] != '-' && opt[0] != '+' )
		      opt = string("-") + opt;
		    if ( doDebug() )
		      DS << "set :" << opt << endl;
		    if ( api->SetOptions( opt ) ){
		      if ( !api->ConfirmOptions() ){
			args->os() << "set " << opt << " failed" << endl;
		      }
		    }
		    else {
		      LS << ": Don't understand set='"
			 << opt << "'" << endl;
		      args->os() << ": Don't understand set='"
			 << it->second << "'" << endl;
		    }
		    ++it;
		  }
		  range = acts.equal_range( "show" );
		  it = range.first;
		  while ( it != range.second ){
		    if ( it->second == "settings" ){
		      xmlNode *tmp = api->settingsToXML();
		      xmlAddChild( root, tmp );
		    }
		    else if ( it->second == "weights" ){
		      xmlNode *tmp = api->weightsToXML();
		      xmlAddChild( root, tmp );
		    }
		    else
		      LS << "don't know how to SHOW: "
			 << it->second << endl;

		    ++it;
		  }
		  range = acts.equal_range( "classify" );
		  it = range.first;
		  while ( it != range.second ){
		    string params = it->second;
		    params = urlDecode(params);
		    int len = params.length();
		    if ( len > 2 ){
		      DS << "params=" << params << endl
			 << "params[0]='"
			 << params[0] << "'" << endl
			 << "params[len-1]='"
			 << params[len-1] << "'"
			 << endl;

		      if ( ( params[0] == '"' && params[len-1] == '"' )
			   || ( params[0] == '\'' && params[len-1] == '\'' ) )
			params = params.substr( 1, len-2 );
		    }
		    DS << "base='" << basename << "'"
		       << endl
		       << "command='classify'"
		       << endl;
		    string distrib, answer;
		    double distance;
		    if ( doDebug() )
		      LS << "Classify(" << params << ")" << endl;
		    if ( api->Classify( params, answer, distrib, distance ) ){

		      if ( doDebug() )
			LS << "resultaat: " << answer
			   << ", distrib: " << distrib
			   << ", distance " << distance
			   << endl;

		      xmlNode *cl = TiCC::XmlNewChild( root, "classification" );
		      TiCC::XmlNewTextChild( cl, "input", params );
		      TiCC::XmlNewTextChild( cl, "category", answer );
		      if ( api->Verbosity(DISTRIB) ){
			TiCC::XmlNewTextChild( cl, "distribution", distrib );
		      }
		      if ( api->Verbosity(DISTANCE) ){
			TiCC::XmlNewTextChild( cl, "distance",
					       TiCC::toString<double>(distance) );
		      }
		      if ( api->Verbosity(CONFIDENCE) ){
		       	TiCC::XmlNewTextChild( cl, "confidence",
		       			       TiCC::toString<double>( api->confidence() ) );
		      }
		      if ( api->Verbosity(MATCH_DEPTH) ){
			TiCC::XmlNewTextChild( cl, "match_depth",
					       TiCC::toString<double>( api->matchDepth()) );
		      }
		      if ( api->Verbosity(NEAR_N) ){
			xmlNode *nb = api->bestNeighborsToXML();
			xmlAddChild( cl, nb );
		      }
		    }
		    else {
		      DS << "classification failed" << endl;
		    }
		    ++it;
		  }
		}
		string tmp = doc.toString();
		// cerr << "THE DOCUMENT for sending!" << endl << tmp << endl;
		int timeout=10;
		nb_putline( args->os(), tmp , timeout );
		delete api;
	      }
	    }
	    else {
	      DBG << "HttpServer::invalid BASE! '" << basename
			  << "'" << endl;
	      args->os() << "invalid basename: '" << basename << "'" << endl;
	    }
	    args->os() << endl;
	  }
	}
      }
    }
  }
}

bool JsonServer::read_json( istream& is,
			    json& the_json ){
  the_json.clear();
  string json_line;
  if ( getline( is, json_line ) ){
    try {
      the_json = json::parse( json_line );
    }
    catch ( const exception& e ){
      cerr << "json parsing failed on '" << json_line + "':"
	  << e.what() << endl;
    }
    DBG << "Read JSON: " << the_json << endl;
    return true;
  }
  return false;
}

void JsonServer::callback( childArgs *args ){
  JsonServer *theServer = dynamic_cast<JsonServer*>( args->mother() );
  int sockId = args->id();
  TimblThread *client = 0;
  map<string, TimblExperiment*> *experiments =
    static_cast<map<string, TimblExperiment*> *>(callback_data);

  int result = 0;
  json out_json;
  out_json["status"] = "ok";
  if ( experiments->size() == 1
       && experiments->find("default") != experiments->end() ){
    DBG << "Before Create Default Client " << endl;
    client = new TimblThread( (*experiments)["default"], args );
    DBG << "After Create Client " << endl;
    // report connection to the server terminal
    //
  }
  else {
    json arr = json::array();
    for ( const auto& it : *experiments ){
      arr.push_back( it.first );
    }
    out_json["available_bases"] = arr;
  }
  DBG << "send JSON: " << out_json.dump(2) << endl;
  args->os() << out_json << endl;

  json in_json;
  bool go_on = true;
  while ( go_on && theServer->read_json( args->is(), in_json ) ){
    if ( in_json.empty() ){
      continue;
    }
    DBG << "handling JSON: " << in_json.dump(2) << endl;
    DBG << "running FromSocket: " << sockId << endl;
    string command;
    string param;
    vector<string> params;
    if ( in_json.find("command") != in_json.end() ){
      command = in_json["command"];
    }
    if ( command.empty() ){
      DBG << sockId << " Don't understand '" << in_json << "'" << endl;
      json out_json;
      out_json["error"] = "Illegal instruction:'" + in_json.dump() + "'";
      args->os() << out_json << endl;
    }
    else {
      if ( in_json.find("param") != in_json.end() ){
	param = in_json["param"];
      }
      else if ( in_json.find("params") != in_json.end() ){
	json pars = in_json["params"];
	if ( pars.is_array() ){
	  for ( size_t i=0; i < pars.size(); ++i ){
	    params.push_back( pars[i].get<std::string>() );
	  }
	}
      }
      DBG << sockId << " Command='" << command << "'" << endl;
      if ( param.empty() ){
	DBG << sockId << " Params='" << params << "'" << endl;
      }
      else {
	DBG << sockId << " Param='" << param << "'" << endl;
      }
      if ( command == "base" ){
	if ( param.empty() ){
	  json out_json;
	  out_json["error"] = "missing 'param' for base command ";
	  args->os() << out_json << endl;
	}
	else {
	  map<string,TimblExperiment*>::const_iterator it
	    = experiments->find(param);
	  if ( it != experiments->end() ){
	    //	  args->os() << "selected base: '" << Params << "'" << endl;
	    if ( client ){
	      delete client;
	    }
	    DBG << sockId << " before Create Default Client " << endl;
	    client = new TimblThread( it->second, args );
	    DBG << sockId << " after Create Client " << endl;
	    // report connection to the server terminal
	    //
	    char line[256];
	    sprintf( line, "Thread %lu, on Socket %d",
		     (uintptr_t)pthread_self(), sockId );
	    DBG << sockId << " " << line << ", started." << endl;
	    json out_json;
	    out_json["base"] = param;
	    args->os() << out_json << endl;
	  }
	  else {
	    json out_json;
	    out_json["error"] = "Unknown basename: " + param;
	    args->os() << out_json << endl;
	  }
	}
      }
      else if ( command == "set" ){
	if ( !client ){
	  json out_json;
	  out_json["error"] = "'set' failed: you haven't selected a base yet!";
	  args->os() << out_json << endl;
	}
	else {
	  if ( param.empty() ){
	    json out_json;
	    out_json["error"] = "missing 'param' for set command ";
	    args->os() << out_json << endl;
	  }
	  else {
	    json out_json;
	    if ( client->setOptions( param ) ){
	      DBG << sockId << " setOptions: " << param << endl;
	      out_json["status"] = "ok";
	    }
	    else {
	      DBG << sockId<< " Don't understand set(" << param << ")" << endl;
	      out_json["error"] = "set( " + param + ") failed";
	    }
	    args->os() << out_json << endl;
	  }
	}
      }
      else if ( command == "query"
		|| command == "show" ){
	if ( !client ){
	  json out_json;
	  out_json["error"] = "'show' failed: no base selected";
	  args->os() << out_json << endl;
	}
	if ( param.empty() ){
	  json out_json;
	  out_json["error"] = "missing 'param' for " + command + " command ";
	  args->os() << out_json << endl;
	}
	else {
	  if ( param == "settings" ){
	    json out_json = client->_exp->settings_to_JSON();
	    args->os() << out_json << endl;
	  }
	  else if ( param == "weights" ){
	    json out_json = client->_exp->weights_to_JSON();
	    args->os() << out_json << endl;
	  }
	  else {
	    json out_json;
	    out_json["error"] = "'show' failed, unknown parameters: ";// + params;
	    args->os() << out_json << endl;
	  }
	}
      }
      else if ( command == "exit" ){
	json out_json;
	out_json["status"] = "closed";
	args->os() << out_json << endl;
	go_on = false;
      }
      else if ( command == "classify" ){
	if ( !client ){
	  json out_json;
	  out_json["error"] = "'classify' failed: you haven't selected a base yet!";
	  args->os() << out_json << endl;
	}
	else {
	  if ( params.empty() ){
	    if ( param.empty() ){
	      json out_json;
	      out_json["error"] = "missing 'param' or 'params' for 'classify'";
	      args->os() << out_json << endl;
	    }
	    else {
	      params.push_back( param );
	    }
	  }
	  else if ( !param.empty() ){
	    json out_json;
	    out_json["error"] = "both 'param' and 'params' found";
	    args->os() << out_json << endl;
	  }
	  if ( !params.empty() ){
	    json out_json = client->classify_to_json( params );
	    DBG << "JsonServer::sending JSON:" << endl << out_json << endl;
	    args->os() << out_json << endl;
	    if ( out_json.find("error") == out_json.end() ){
	      result += out_json.size();
	    }
	  }
	}
      }
      else {
	json out_json;
	out_json["error"] = "Unknown command: '" + command + "'";
	args->os() << out_json << endl;
      }
    }
  }
  delete client;
  LOG << sockId << " Thread " << (uintptr_t)pthread_self()
	      << " terminated, " << result
	      << " instances processed " << endl;
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
      server->myLog.message("tcp_server");
    }
    else if ( protocol == "http" ){
      server = new HttpServer( config );
      server->myLog.message("http_server");
    }
    else if ( protocol == "json" ){
      server = new JsonServer( config );
      server->myLog.message("json_server");
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
