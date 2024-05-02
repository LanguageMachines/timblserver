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
#include "timbl/TimblAPI.h"
#include "ticcutils/json.hpp"
#include "ticcutils/ServerBase.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;
using namespace TiCCServer;
using namespace nlohmann;

using TiCC::operator<<;

#define LOG *TiCC::Log(logstream())
#define DBG *TiCC::Dbg(logstream())

#define SDBG *TiCC::Dbg(client->myLog)


json json_error( const string& message ){
  json result;
  result["status"] = "error";
  result["message"] = message;
  return result;
}

json JsonServer::classify_to_json( TimblThread *client,
				   const vector<string>& params ) const {
  SDBG << "classify_to_json(" << params << ")" << endl;
  TimblExperiment *_exp = client->_exp;
  json result;
  if ( params.size() > 1 ){
    result = _exp->classify_to_JSON( params );
  }
  else {
    result = _exp->classify_to_JSON( params[0] );
  }
  SDBG << "created json: " << result.dump(2) << endl;
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
  map<string, TimblExperiment*> my_experiments =
    *(static_cast<map<string, TimblExperiment*> *>(callback_data()));

  int result = 0;
  json out_json;
  out_json["status"] = "ok";
  if ( my_experiments.size() == 1
       && my_experiments.find("default") != my_experiments.end() ){
    DBG << "Before Create Default Client " << endl;
    TimblExperiment *exp = my_experiments["default"];
    client = new TimblThread( exp, args, true );
    DBG << "After Create Client " << endl;
    // report connection to the server terminal
    //
  }
  else {
    json arr = json::array();
    for ( const auto& it : my_experiments ){
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
    if ( in_json.find("command") != in_json.end() ){
      command = in_json["command"];
    }
    if ( command.empty() ){
      DBG << sockId << " Don't understand '" << in_json << "'" << endl;
      json err_json = json_error( "Illegal instruction:'"
				  + in_json.dump() + "'" );
      args->os() << err_json << endl;
    }
    else {
      vector<string> params;
      if ( in_json.find("param") != in_json.end() ){
	param = in_json["param"];
      }
      else if ( in_json.find("params") != in_json.end() ){
	json pars = in_json["params"];
	if ( pars.is_array() ){
	  for ( auto const& par : pars ){
	    params.push_back( par.get<std::string>() );
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
	  json err_json = json_error( "missing 'param' for base command " );
	  args->os() << err_json << endl;
	}
	else {
	  auto it = my_experiments.find(param);
	  if ( it != my_experiments.end() ){
	    //	  args->os() << "selected base: '" << Params << "'" << endl;
	    if ( client ){
	      delete client;
	    }
	    DBG << sockId << " before Create Default Client " << endl;
	    client = new TimblThread( it->second, args, true );
	    DBG << sockId << " after Create Client " << endl;
	    // report connection to the server terminal
	    //
	    DBG << sockId << " Thread " << (uintptr_t)pthread_self()
		<< " on Socket " << sockId << " started." << endl;
	    out_json.clear();
	    out_json["base"] = param;
	    args->os() << out_json << endl;
	  }
	  else {
	    json err_json = json_error( "Unknown basename: '" + param + "'" );
	    args->os() << err_json << endl;
	  }
	}
      }
      else if ( command == "set" ){
	if ( !client ){
	  json err_json = json_error( "'set' failed: you haven't selected a base yet!" );
	  args->os() << err_json << endl;
	}
	else {
	  if ( param.empty() ){
	    json err_json = json_error( "missing 'param' for set command " );
	    args->os() << err_json << endl;
	  }
	  else {
	    out_json.clear();
	    if ( client->setOptions( param ) ){
	      DBG << sockId << " setOptions: " << param << endl;
	      out_json["status"] = "ok";
	      args->os() << out_json << endl;
	    }
	    else {
	      DBG << sockId<< " Don't understand set(" << param << ")" << endl;
	      json err_json = json_error("set( " + param + ") failed" );
	      args->os() << err_json << endl;
	    }
	  }
	}
      }
      else if ( command == "query"
		|| command == "show" ){
	if ( !client ){
	  json err_json = json_error( "'show' failed: no base selected" );
	  args->os() << err_json << endl;
	}
	else if ( param.empty() ){
	  json err_json = json_error( "missing 'param' for " + command + " command " );
	  args->os() << err_json << endl;
	}
	else {
	  out_json.clear();
	  if ( param == "settings" ){
	    out_json = client->_exp->settings_to_JSON();
	  }
	  else if ( param == "weights" ){
	    out_json = client->_exp->weights_to_JSON();
	  }
	  else {
	    out_json = json_error( "'show' failed, unknown parameter: "
				   + param );
	  }
	  args->os() << out_json << endl;
	}
      }
      else if ( command == "exit" ){
	out_json.clear();
	out_json["status"] = "closed";
	args->os() << out_json << endl;
	go_on = false;
      }
      else if ( command == "classify" ){
	if ( !client ){
	  json err_json = json_error( "'classify' failed: you haven't selected a base yet!" );
	  args->os() << err_json << endl;
	}
	else {
	  if ( params.empty() ){
	    if ( param.empty() ){
	      json err_json = json_error( "missing 'param' or 'params' for 'classify'" );
	      args->os() << err_json << endl;
	    }
	    else {
	      params.push_back( param );
	    }
	  }
	  else if ( !param.empty() ){
	    json err_json = json_error( "both 'param' and 'params' found" );
	    args->os() << err_json << endl;
	  }
	  if ( !params.empty() ){
	    out_json = classify_to_json( client, params );
	    DBG << "JsonServer::sending JSON:" << endl << out_json << endl;
	    args->os() << out_json << endl;
	    if ( out_json.find("error") == out_json.end() ){
	      result += out_json.size();
	    }
	  }
	}
      }
      else {
	json err_json = json_error( "Unknown command: '" + command + "'" );
	args->os() << err_json << endl;
      }
    }
  }
  delete client;
  LOG << sockId << " Thread " << (uintptr_t)pthread_self()
	      << " terminated, " << result
	      << " instances processed " << endl;
}
