/******************************** LICENSE ********************************

 Copyright 2007 European Centre for Medium-Range Weather Forecasts (ECMWF)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ******************************** LICENSE ********************************/

/*! \file EpsgramDecoder.h
    \brief Definition of the Template class EpsgramDecoder.
    
    Magics Team - ECMWF 2005
    
    Started: Mon 19-Sep-2005
    
    Changes:
    
*/

#ifndef GeoJSon_H
#define GeoJSon_H

#include "magics.h"


#include "GeoJSonAttributes.h"

#include "Decoder.h"
#include "Data.h"
#include "UserPoint.h"
#include "DateTime.h"
#include "json_spirit.h"
#include "PointsHandler.h"

#include <limits>

namespace magics {



class GeoJSon:
			public Data,
			public PointsList,
			public GeoJSonAttributes 
{
public:
	GeoJSon();
	virtual ~GeoJSon();
	
	typedef void (GeoJSon::*Method)(const json_spirit::Value&);


	map<string,  Method> methods_;
	


	
    void points(const Transformation&, vector<UserPoint>&);
    void customisedPoints(const Transformation&, const std::set<string>&, CustomisedPointsList&, bool );
    PointsHandler& points(const Transformation&, bool);


protected:
     //! Method to print string about this class on to a stream of type ostream (virtual).
	 virtual void print(ostream&) const; 
	 void decode();
	 void coordinates(const json_spirit::Value&);
	 void properties(const json_spirit::Value&);
	 void type(const json_spirit::Value&);
	 vector<CustomisedPoint*> points_;

    

private:
    //! Copy constructor - No copy allowed
	GeoJSon(const GeoJSon&);
    //! Overloaded << operator to copy - No copy allowed
	GeoJSon& operator=(const GeoJSon&);

// -- Friends
    //! Overloaded << operator to call print().
	friend ostream& operator<<(ostream& s,const GeoJSon& p)
		{ p.print(s); return s; }
};

} // namespace magics
#endif