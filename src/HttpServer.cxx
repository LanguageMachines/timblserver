/*
  Copyright (c) 1998 - 2024
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
#include "ticcutils/ServerBase.h"
#include "timbl/TimblAPI.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;
using namespace TiCCServer;

using TiCC::operator<<;

#define LOG *TiCC::Log(logstream())
#define DBG *TiCC::Dbg(logstream())

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
  string logLine = "Thread " + to_string( (uintptr_t)pthread_self() )
    + " on Socket " + to_string( args->id() );
  LOG << logLine << " started." << endl;
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
	    auto exp_it = experiments.find(basename);
	    if ( exp_it != experiments.end() ){
	      TimblThread *client = new TimblThread( exp_it->second, args );
	      if ( client ){
		TiCC::LogStream LS( &logstream() );
		TiCC::LogStream DS( &logstream() );
		DS.set_message(logLine);
		LS.set_message(logLine);
		DS.setstamp( StampBoth );
		LS.setstamp( StampBoth );
		TiCC::XmlDoc doc( "TiMblResult" );
		xmlNode *root = doc.getRoot();
		TiCC::XmlSetAttribute( root, "algorithm",
				       TiCC::toString(client->_exp->Algorithm()) );
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
		  auto range = acts.equal_range( "set" );
		  auto it = range.first;
		  while ( it != range.second ){
		    string opt = it->second;
		    if ( !opt.empty() && opt[0] != '-' && opt[0] != '+' ){
		      opt = string("-") + opt;
		    }
		    if ( doDebug() ){
		      DS << "set :" << opt << endl;
		    }
		    if ( !client->setOptions( opt ) ){
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
		      xmlNode *node = client->_exp->settingsToXML();
		      xmlAddChild( root, node );
		    }
		    else if ( it->second == "weights" ){
		      xmlNode *node = client->_exp->weightsToXML();
		      xmlAddChild( root, node );
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
			   || ( params[0] == '\'' && params[len-1] == '\'' ) ){
			params = params.substr( 1, len-2 );
		      }
		    }
		    DS << "base='" << basename << "'"
		       << endl
		       << "command='classify'"
		       << endl;
		    string distrib, answer;
		    double distance;
		    if ( doDebug() ){
		      LS << "Classify(" << params << ")" << endl;
		    }
		    if ( client->_exp->Classify( params, answer, distrib, distance ) ){

		      if ( doDebug() ){
			LS << "resultaat: " << answer
			   << ", distrib: " << distrib
			   << ", distance " << distance
			   << endl;
		      }
		      xmlNode *cl = TiCC::XmlNewChild( root, "classification" );
		      TiCC::XmlNewTextChild( cl, "input", params );
		      TiCC::XmlNewTextChild( cl, "category", answer );
		      if ( client->_exp->Verbosity(DISTRIB) ){
			TiCC::XmlNewTextChild( cl, "distribution", distrib );
		      }
		      if ( client->_exp->Verbosity(DISTANCE) ){
			TiCC::XmlNewTextChild( cl, "distance",
					       TiCC::toString<double>(distance) );
		      }
		      if ( client->_exp->Verbosity(CONFIDENCE) ){
		       	TiCC::XmlNewTextChild( cl, "confidence",
		       			       TiCC::toString<double>( client->_exp->confidence() ) );
		      }
		      if ( client->_exp->Verbosity(MATCH_DEPTH) ){
			TiCC::XmlNewTextChild( cl, "match_depth",
					       TiCC::toString<double>( client->_exp->matchDepth()) );
		      }
		      if ( client->_exp->Verbosity(NEAR_N) ){
			xmlNode *nb = client->_exp->bestNeighborsToXML();
			xmlAddChild( cl, nb );
		      }
		    }
		    else {
		      DS << "classification failed" << endl;
		    }
		    ++it;
		  }
		}
		string out_line = doc.toString();
		timeout=10;
		nb_putline( args->os(), out_line , timeout );
		delete client;
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
