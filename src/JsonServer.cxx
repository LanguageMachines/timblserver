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
#include "timbl/TimblAPI.h"
#include "ticcutils/json.hpp"
#include "ticcutils/ServerBase.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;
using namespace nlohmann;

using TiCC::operator<<;

#define LOG *TiCC::Log(myLog)
#define DBG *TiCC::Dbg(myLog)

#define SDBG *TiCC::Dbg(client->myLog)

json JsonServer::classify_to_json( TimblThread *client,
				   const vector<string>& params ) const {
  SDBG << "classify_to_json(" << params << ")" << endl;
  double distance;
  string distrib;
  string answer;
  TimblExperiment *_exp = client->_exp;
  json result;
  if ( params.size() > 1 ){
    result = json::array();
  }
  for ( const auto& param : params ){
    json out_json;
    if ( _exp->Classify( param, answer, distrib, distance ) ){
      SDBG << _exp->ExpName() << ":" << param << " --> "
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
      SDBG << _exp->ExpName() << ": Classify Failed on '"
	   << param << "'" << endl;
      out_json["error"] = "timbl:classify(" + param + ") failed";
    }
    SDBG << "created json: " << out_json.dump(2) << endl;
    if ( params.size() > 1 ){
      result.push_back( out_json );
    }
    else {
      result = out_json;
    }
  }
  return result;
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
	    json out_json = classify_to_json( client, params );
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
