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

#include <exception>
#include <vector>
#include <string>
#include <cstdlib>

#include "ticcutils/CommandLine.h"
#include "ticcutils/Timer.h"
#include "timbl/TimblAPI.h"
#include "timbl/GetOptClass.h"
#include "ticcutils/ServerBase.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;

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
    cerr << "Unable to initalize at least one tagger" << endl
	 << "please check your commandline and/or your confiuration file"
	 << endl;
    exit(EXIT_FAILURE);
  }

  auto experiments = new map<string, TimblExperiment*>();
  server->callback_data = experiments;

  map<string,string>::iterator it = allvals.begin();
  while ( it != allvals.end() ){
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
	cerr << "illegal -a value: " << value << endl;
	exit(EXIT_FAILURE);
      }
    }
    if ( !opts.extract( 'f', trainName ) )
      opts.extract( 'i', treeName );
    if ( opts.extract( 'u', ProbInFile ) ){
      if ( algorithm == IGTREE ){
	cerr << "-u option is useless for IGtree" << endl;
	exit(EXIT_FAILURE);
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
	    cerr << "invalid weighting option: " << value << endl;
	    exit(1);
	  }
	  WgtInFile = parts[0];
	  WgtType = W;
	}
	else if ( num == 1 ){
	  WgtInFile = value;
	}
	else {
	  cerr << "invalid weighting option: " << value << endl;
	  exit(1);
	}
      }
    }
    opts.extract( "matrixin", MatrixInFile );
    if ( !( treeName.empty() && trainName.empty() ) ){
      TimblAPI *run = new TimblAPI( opts, it->first );
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
	(*experiments)[it->first] = run->grabAndDisconnectExp();
	delete run;
	server->myLog << "started experiment " << it->first
		      << " with parameters: " << it->second << endl;
      }
      else {
	server->myLog << "FAILED to start experiment " << it->first
		      << " with parameters: " << it->second << endl;
      }
    }
    else {
      server->myLog << "missing '-i' or '-f' option in serverconfig entry: '"
		    << it->first << "=" << it->second << "'" << endl;
    }
    ++it;
  }
}

class TcpServer : public TcpServerBase {
public:
  void callback( childArgs* );
  TcpServer( const TiCC::Configuration *c ): TcpServerBase( c ){};
};

class HttpServer : public HttpServerBase {
public:
  void callback( childArgs* );
  HttpServer( const TiCC::Configuration *c ): HttpServerBase( c ){};
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
  string::size_type pos = line.find_first_of(" \t");
  if ( pos != string::npos ){
    com = line.substr(0,pos);
    rest = line.substr(pos+1);
  }
  else {
    com = line;
    rest = "";
  }
}

class TimblClient {
public:
  TimblClient( TimblExperiment *, childArgs * );
  ~TimblClient(){ delete _exp; };
  bool classifyLine( const string& );
  void showSettings(){ _exp->ShowSettings( os ); };
  bool setOptions( const string& param );
private:
  TiCC::LogStream& myLog;
  bool doDebug;
  TimblExperiment *_exp;
  ostream& os;
  istream& is;
};

TimblClient::TimblClient( TimblExperiment *exp,
			  childArgs* args ):
  myLog(args->logstream()),
  doDebug(args->debug()),
  os(args->os()),
  is(args->is())
{
  if ( doDebug )
    myLog.setlevel(LogHeavy);
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

bool TimblClient::setOptions( const string& param ){
  if ( _exp->SetOptions( param ) ){
    DBG << "setOptions: " << param << endl;
    if ( _exp->ConfirmOptions() )
      os << "OK" << endl;
    else
      os << "ERROR { set options failed: " << param << "}" << endl;
  }
  else {
    DBG << ": Don't understand '" << param << "'" << endl;
    os << "ERROR { set options failed: " << param << "}" << endl;
  }
  return true;
}

bool TimblClient::classifyLine( const string& params ){
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
      if ( os.good() ){
	if ( _exp->Verbosity(DISTANCE) ){
	  os << " DISTANCE {" << Distance << "}";
	}
	if ( os.good() ){
	  if ( _exp->Verbosity(MATCH_DEPTH) ){
	    os << " MATCH_DEPTH {" << _exp->matchDepth() << "}";
	  }
	  if ( os.good() ){
	    if ( _exp->Verbosity(NEAR_N) ){
	      os << " NEIGHBORS" << endl;
	      _exp->showBestNeighbors( os );
	      os << "ENDNEIGHBORS";
	    }
	  }
	}
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


void TcpServer::callback( childArgs *args ){
  string Line;
  int sockId = args->id();
  TimblClient *client = 0;
  map<string, TimblExperiment*> *experiments =
    static_cast<map<string, TimblExperiment*> *>(callback_data);

  int result = 0;
  string baseName;
  args->os() << "Welcome to the Timbl server." << endl;
  if ( experiments->size() == 1
       && experiments->find("default") != experiments->end() ){
    baseName = "default";
    DBG << " Voor Create Default Client " << endl;
    client = new TimblClient( (*experiments)[baseName], args );
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
    DBG << "FirstLine='" << Line << "'" << endl;
    string Command, Param;
    bool go_on = true;
    DBG << "running FromSocket: " << sockId << endl;

    do {
      Line = TiCC::trim( Line );
      DBG << "Line='" << Line << "'" << endl;
      Split( Line, Command, Param );
      DBG << "Command='" << Command << "'" << endl;
      DBG << "Param='" << Param << "'" << endl;
      switch ( check_command(Command) ){
      case Base:{
	map<string,TimblExperiment*>::const_iterator it
	  = experiments->find(Param);
	if ( it != experiments->end() ){
	  baseName = Param;
	  args->os() << "selected base: '" << Param << "'" << endl;
	  if ( client )
	    delete client;
	  DBG << " Voor Create Default Client " << endl;
	  client = new TimblClient( it->second, args );
	  DBG << " Na Create Client " << endl;
	  // report connection to the server terminal
	  //
	  char line[256];
	  sprintf( line, "Thread %zd, on Socket %d",
		   (uintptr_t)pthread_self(), sockId );
	  LOG << line << ", started." << endl;
	}
	else {
	  args->os() << "ERROR { Unknown basename: " << Param << "}" << endl;
	}
      }
	break;
      case Set:
	if ( !client )
	  args->os() << "you haven't selected a base yet!" << endl;
	else {
	  client->setOptions( Param );
	}
	break;
      case Query:
	if ( !client )
	  args->os() << "you haven't selected a base yet!" << endl;
	else {
	  args->os() << "STATUS" << endl;
	  client->showSettings( );
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
	DBG << sockId << ": Don't understand '"
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
    int cc;
    string result;
    int len=s.size();
    for (int i=0; i<len ; ++i ) {
      cc=s[i];
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
  sprintf( logLine, "Thread %zd, on Socket %d", (uintptr_t)pthread_self(),
	   args->id() );
  LOG << logLine << ", started." << endl;
  string Line;
  int timeout = 1;
  if ( nb_getline( args->is(), Line, timeout ) ){
    DBG << "FirstLine='" << Line << "'" << endl;
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
	DBG << "Line='" << line << "'" << endl;
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
		vector<string> avs;
		int avNum = TiCC::split_at( qstring, avs, "&" );
		if ( avNum > 0 ){
		  multimap<string,string> acts;
		  for ( int i=0; i < avNum; ++i ){
		    vector<string> parts;
		    int num = TiCC::split_at( avs[i], parts, "=" );
		    if ( num == 2 ){
		      acts.insert( make_pair(parts[0], parts[1]) );
		    }
		    else if ( num > 2 ){
		      string tmp = parts[1];
		      for( int i=2; i < num; ++i )
			tmp += string("=")+parts[i];
		      acts.insert( make_pair(parts[0], tmp ) );
		    }
		    else {
		      LS << "unknown word in query "
			 << avs[i] << endl;
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
	      DBG << "invalid BASE! '" << basename
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

int main(int argc, char *argv[]){
  try {
    // Start.
    //
    cerr << "TiMBL Server " << TimblServer::Version()
	 << " (c) CLTS/ILK/CLIPS 1998 - 2016.\n"
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
    string value;
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
    if ( protocol.empty() )
      protocol = "tcp";
    if ( protocol == "tcp" )
      server = new TcpServer( config );
    else if ( protocol == "http" )
      server = new HttpServer( config );
    else {
      cerr << "unknown protocol " << protocol << endl;
      exit(EXIT_FAILURE);
    }
    server->myLog.message("timblserver");
    startExperiments( server );
    return server->Run(); // returns EXIT_SUCCESS or EXIT_FAIL
  }
  catch(std::bad_alloc){
    cerr << "ran out of memory somewhere" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  catch(std::string& what){
    cerr << "an exception was raised: '" << what << "'" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  catch(std::exception& e){
    cerr << "a standard exception was raised: '" << e.what() << "'" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  catch(...){
    cerr << "some exception was raised" << endl;
    cerr << "timblserver terminated, Sorry for that" << endl;
  }
  return 0;
}
