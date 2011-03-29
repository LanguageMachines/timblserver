/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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

#ifndef TIMBL_SERVER_API_H
#define TIMBL_SERVER_API_H

#include <string>
#include <vector>
#include "timbl/TimblAPI.h"
#include "timblserver/SocketBasics.h" //expose Sockets to the world

namespace Timbl{

  class TimblServer;

  class TimblServerAPI {
    friend class TimblServer;
    friend class TimblExperiment;
  public:
    TimblServerAPI( TimblOpts * );
    TimblServerAPI( const TimblAPI& );
    ~TimblServerAPI();
    bool Valid() const;
    bool StartServer( const int, const int=10 );
    bool StartMultiServer( const std::string& );
    bool Prepare( const std::string& = "" );
    bool Learn( const std::string& = "" );
    std::string ExpName() const;
    static std::string VersionInfo( bool = false );
    bool GetWeights( const std::string& = "", Weighting = UNKNOWN_W  );
    Weighting CurrentWeighting() const;
    Weighting GetCurrentWeights( std::vector<double>& ) const;
    bool GetInstanceBase( const std::string& = "" );
    bool GetArrays( const std::string& = "" );
    bool GetMatrices( const std::string& = "" );
    Algorithm Algo() const;
    InputFormatType getInputFormat() const;
    static int Default_Max_Feats();
    bool initExperiment();
  private:
    TimblServerAPI();
    TimblServerAPI& operator=( const TimblAPI& ); // so nobody may use them
    TimblServer *pimpl;
    bool i_am_fine;
  }; 

  int daemonize( int , int  );

}
#endif // TIMBL_SERVER_API_H
