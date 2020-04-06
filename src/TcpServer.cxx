/*
  Copyright (c) 1998 - 2020
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
#include "timbl/TimblAPI.h"
#include "ticcutils/ServerBase.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TiCCServer;
using namespace TimblServer;

using TiCC::operator<<;

#define LOG *TiCC::Log(logstream())
#define DBG *TiCC::Dbg(logstream())

#define SDBG *TiCC::Dbg(client->myLog)

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


bool TcpServer::classifyLine( TimblThread *client,
			      const string& params ) const {
  double Distance;
  string Distrib;
  string Answer;
  TimblExperiment *_exp = client->_exp;
  ostream *os = &client->os;
  if ( _exp->Classify( params, Answer, Distrib, Distance ) ){
    SDBG << _exp->ExpName() << ":" << params << " --> "
		<< Answer << " " << Distrib
		<< " " << Distance << endl;
    *os << "CATEGORY {" << Answer << "}";
    if ( os->good() ){
      if ( _exp->Verbosity(DISTRIB) ){
	*os << " DISTRIBUTION " <<Distrib;
      }
    }
    if ( os->good() ){
      if ( _exp->Verbosity(DISTANCE) ){
	*os << " DISTANCE {" << Distance << "}";
      }
    }
    if ( os->good() ){
      if ( _exp->Verbosity(MATCH_DEPTH) ){
	*os << " MATCH_DEPTH {" << _exp->matchDepth() << "}";
      }
    }
    if ( os->good() ){
      if ( _exp->Verbosity(CONFIDENCE) ){
	*os << " CONFIDENCE {" <<_exp->confidence() << "}";
      }
    }
    if ( os->good() ){
      if ( _exp->Verbosity(NEAR_N) ){
	*os << " NEIGHBORS" << endl;
	_exp->showBestNeighbors( *os );
	*os << "ENDNEIGHBORS";
      }
    }
    if ( os->good() )
      *os << endl;
    return os->good();
  }
  else {
    SDBG << _exp->ExpName() << ": Classify Failed on '"
		<< params << "'" << endl;
    return false;
  }
}

void TcpServer::callback( childArgs *args ){
  string Line;
  int sockId = args->id();
  TimblThread *client = 0;
  map<string, TimblExperiment*> experiments =
    *(static_cast<const map<string, TimblExperiment*> *>(callback_data()));

  int result = 0;
  args->os() << "Welcome to the Timbl server." << endl;
  if ( experiments.size() == 1
       && experiments.find("default") != experiments.end() ){
    DBG << " Voor Create Default Client " << endl;
    TimblExperiment *exp = experiments["default"];
    client = new TimblThread( exp, args );
    DBG << " Na Create Client " << endl;
    // report connection to the server terminal
    //
  }
  else {
    args->os() << "available bases: ";
    map<string,TimblExperiment*>::const_iterator it = experiments.begin();
    while ( it != experiments.end() ){
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
	  = experiments.find(Param);
	if ( it != experiments.end() ){
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
	  if ( classifyLine( client, Param ) ){
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
