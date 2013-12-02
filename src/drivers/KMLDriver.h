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

/*!
    \file KMLDriver.h
    \brief Definition of KMLDriver.
    \author Meteorological Visualisation Section, ECMWF

    Started: Thu Oct 18 18:41:52 2007
*/

#ifndef _KMLDriver_H
#define _KMLDriver_H

#include <BaseDriver.h>
#include <KMLDriverAttributes.h>
#include <XmlNode.h>

namespace magics
{

/*! \class KMLDriver
    \brief This driver produces output for KML
    \ingroup drivers

    The driver produces Keyhole XML (KML) output for usage with 
    Google Earth (http://earth.google.com/) and Google Maps (http://maps.google.com/).
*/
class KMLDriver: public BaseDriver, public KMLDriverAttributes
{

public:
	KMLDriver();
	~KMLDriver();
	void open();
	void close();

	/*!
	  \brief sets a new XML node
	*/
	void set(const XmlNode& node)
	{
		if ( magCompare(node.name(),"kml"))
		{
			XmlNode basic = node;
			basic.name("driver");
			BaseDriver::set(basic);
			basic.name("kml");
			KMLDriverAttributes::set(basic);
		}
	}

	/*!
	  \brief sets a new map
	*/
	void set(const map<string, string>& map)
	{
		BaseDriver::set(map);
		KMLDriverAttributes::set(map);
	}

private:
	MAGICS_NO_EXPORT void startPage() const;
	MAGICS_NO_EXPORT void endPage() const;
	MAGICS_NO_EXPORT void project(const Layout& lay) const;
	MAGICS_NO_EXPORT void redisplay(const StaticLayer&) const;
	MAGICS_NO_EXPORT void redisplay(const NoDataLayer&) const;
	MAGICS_NO_EXPORT void redisplay(const StepLayer&) const;
	MAGICS_NO_EXPORT void newLayer() const;
	MAGICS_NO_EXPORT void closeLayer() const;

	MAGICS_NO_EXPORT void setNewLineWidth(const MFloat) const;
	MAGICS_NO_EXPORT void setNewColour(const Colour &col) const;
	MAGICS_NO_EXPORT void writeColour(const Colour &col) const;
	MAGICS_NO_EXPORT int setLineParameters(const LineStyle style, const MFloat w) const;

	MAGICS_NO_EXPORT void renderPolyline(const int, MFloat *, MFloat *) const;
	MAGICS_NO_EXPORT void renderPolyline2(const int n, MFloat *x, MFloat *y) const;
	MAGICS_NO_EXPORT void renderSimplePolygon(const int, MFloat *, MFloat *) const;
	MAGICS_NO_EXPORT void renderSimplePolygon(const Polyline& line) const;
	MAGICS_NO_EXPORT void renderText(const Text& text) const;
	MAGICS_NO_EXPORT void circle(const MFloat x, const MFloat y, const MFloat r, const int) const;
	MAGICS_NO_EXPORT bool renderPixmap(MFloat,MFloat,MFloat,MFloat,int,int,unsigned char*,int, bool) const;
	MAGICS_NO_EXPORT bool renderCellArray(const Image& obj) const;
	MAGICS_NO_EXPORT void renderSymbols(const Symbol& symbol) const;
	MAGICS_NO_EXPORT void renderWindArrow(const Arrow &arrow) const;
	MAGICS_NO_EXPORT void renderWindFlag(const Flag &flag) const;

	//! Method to print string about this class on to a stream of type ostream (virtual).
	MAGICS_NO_EXPORT void print(ostream&) const;
	MAGICS_NO_EXPORT void debugOutput(const string &s) const;

	/* K M L specific */
	MAGICS_NO_EXPORT void redisplay(const LegendLayout& legend) const;
	MAGICS_NO_EXPORT void closePlacemark() const;
	mutable ofstream	pFile_;    //!< Output stream to file.
	mutable Layout*		layout_;
	mutable string		currentLayerPath_;
	mutable string		currentTimeBegin_;
	mutable string		currentTimeEnd_;
	mutable bool		doKmz_;
	mutable bool		kml_placemark_;
	mutable bool		polyline_begin_;
	mutable bool		polygon_begin_;
	mutable bool		MultiGeometrySet_;
	mutable bool		layer_;
	mutable bool		render_;
	mutable bool		ecmwf_logo_;

	mutable stringarray kml_output_resource_list_;

	//! Copy constructor - No copy allowed
	KMLDriver(const KMLDriver&);
	//! Overloaded << operator to copy - No copy allowed
	KMLDriver& operator=(const KMLDriver&);

	// -- Friends
	//! Overloaded << operator to call print().
	friend ostream& operator<<(ostream& s,const KMLDriver& p)
		{ p.print(s); return s; }
};

} // namespace magics
#endif
