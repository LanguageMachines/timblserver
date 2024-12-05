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

#include "ticcutils/PrettyPrint.h"
#include "ticcutils/ServerBase.h"
#include "timbl/TimblAPI.h"
#include "timbl/GetOptClass.h"
#include "timblserver/TimblServer.h"

using namespace std;
using namespace Timbl;
using namespace TimblServer;
using namespace TiCCServer;

using TiCC::operator<<;

#define DBG *TiCC::Dbg(myLog)
#define LOG *TiCC::Log(myLog)

TimblThread::TimblThread( TimblExperiment *exp,
			  childArgs* args,
			  bool json ):
  myLog(args->logstream()),
  doDebug(args->debug()),
  os(args->os()),
  is(args->is())
{
  if ( doDebug ){
    myLog.set_level(LogHeavy);
  }
  _exp = exp->clone();
  *_exp = *exp;
  if ( !_exp->connectToSocket( &(args->os() ) , json ) ){
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
