/*
 * (C) Copyright 1996-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/*! \file Polyline.cc
    \brief Implementation of polyline graphics class (template).
    
    Magics Team - ECMWF 2004
    
    Started: Jan 2004
    
    Changes:
    
*/
#include "Polyline.h"
#include "Transformation.h"
#include "MagClipper.h"


using namespace magics;



Polyline::Polyline()
{

}


Polyline::~Polyline() 
{
}

void Polyline::reserve(double x)
{
	//polygon_.reserve(x);
}

bool Polyline::reproject(BasicGraphicsObjectContainer& out) const
{
	const Transformation& transformation = out.transformation();
	transformation(*this, out);	
	return false;	
}




Polyline* Polyline::getNew() const
{
	Polyline* poly = new Polyline();
	poly->copy(*this);
	return poly;
}


void Polyline::print(ostream& out) const 
{
	out << "Polyline[";
	out << ", nb_points = " << this->size();
	  if ( this->size() < 20 ) {
            out << " Outer [";
            string sep = "";
	    const unsigned int nb = size();
            for (unsigned int i = 0; i < nb; i++) {
                out << sep << get(i);
                sep = ", ";
            }
            out << "]";
        }
        else {
            unsigned int nb = size();
            out << " Outer[" << get(0) << ", " << get(1) << ", " << get(2);
            out << "...." << get(nb-3) << ", " << get(nb-2)  << ", " << get(nb-1);
            out << "(" << nb << " elements)]";
        }
	out << "]";
}


/*
Polyline* Polyline::simplify(double factor)
{
	BoostPoly simplified;
	boost::geometry::simplify(polygon_, simplified, factor);
	polygon_ = simplified;
	return this;
}
*/




void Polyline::redisplay(const BaseDriver& driver) const 
{
    if ( polygon_.size() > 1 ) driver.redisplay(*this);
}

void Polyline::newHole()
{
	holes_.push_back(deque<PaperPoint>());
}

void Polyline::push_back_hole(const PaperPoint& point)
{
	holes_.back().push_back(point);
}


Polyline::Holes::const_iterator Polyline::beginHoles() const
{
	return holes_.begin();
}

Polyline::Holes::const_iterator Polyline::endHoles() const
{
	return holes_.end();
}

void Polyline::hole(Holes::const_iterator hole, vector<double>& x, vector<double>& y) const
{
	x.reserve(hole->size());
	y.reserve(hole->size());

	for (deque<PaperPoint>::const_iterator h = hole->begin(); h != hole->end(); ++h) {
		x.push_back(h->x_);
		y.push_back(h->y_);
	}
}

void Polyline::hole(Holes::const_iterator hole, Polyline& poly) const
{

	for (deque<PaperPoint>::const_iterator h = hole->begin(); h != hole->end(); ++h) {
		poly.push_back(*h);
	}
}

struct SouthCleaner
{
	SouthCleaner() {}

	bool operator()(PaperPoint& point) {
		return point.y_ < -89;
	}
};
struct LonFinder : std::unary_function<PaperPoint,bool>
{
	LonFinder() {}

	bool operator()(PaperPoint& point)  const {
		return ( same(point.x_, -180.)  );
	}
};

void Polyline::southClean()
{
	
	auto from = std::remove_if (polygon_.begin(), polygon_.end(), SouthCleaner());
	polygon_.erase(from, polygon_.end());	
	// rotate ..
	auto it = std::find_if (polygon_.begin(), polygon_.end(), LonFinder() );
  	if ( it != polygon_.end()) {
    	std::rotate(polygon_.begin(), it, polygon_.end());
  	}

} 

void Polyline::newHole(const Polyline& poly)
{
	holes_.push_back(deque<PaperPoint>());
	for (auto point = poly.begin(); point != poly.end(); ++point) {
		holes_.back().push_back(*point);
	}
}


struct ReprojectHelper
{
	ReprojectHelper(const Transformation& transformation) : transformation_(transformation) {}
	const Transformation& transformation_;
	bool operator()(PaperPoint& point) {
		return !transformation_.fast_reproject(point.x_, point.y_);
	}
};

void Polyline::reproject(const Transformation& transformation)
{
	
	auto from = std::remove_if (polygon_.begin(), polygon_.end(), ReprojectHelper(transformation));
	polygon_.erase(from, polygon_.end());


// Now the holes!
	for (Holes::iterator hole = holes_.begin(); hole != holes_.end(); ++hole)  {
		for (auto h = hole->begin(); h != hole->end(); ++h) {
			transformation.fast_reproject(h->x_, h->y_);
		}
	}
}

Polyline*  Polyline::clone() const
{

	Polyline* to = getNew();

	for (auto point = begin(); point != end(); ++point) {
		to->push_back(*point);
	}

// Now the holes!
	for (Holes::const_iterator hole = holes_.begin(); hole != holes_.end(); ++hole)  {
		to->newHole();
		for (auto h = hole->begin(); h != hole->end(); ++h) {
			to->push_back_hole(*h);
		}

	}

	return to;
}

void Polyline::intersect(const Polyline& poly, vector<Polyline*> & out) const
{
// Use of a MagClipper 
	MagClipper helper;
	
	helper.clip(poly, *this, out);
	
	/*
	vector<BoostPoly> clip;
try {
		boost::geometry::intersection(this->polygon_, poly.polygon_, clip);

		for (vector<BoostPoly>::iterator c = clip.begin(); c != clip.end(); ++c) {
			out.push_back(Polyline());
			out.back().copy(*this);
			out.back().polygon_ = *c;
		}
	}
	catch(...) {
		MagLog::debug() << " pb intersect! " << endl;
		MagLog::debug() << *this << endl;
		MagLog::debug() << " with-> " << endl;
		MagLog::debug() << poly << endl;
		out.push_back(poly);
	}
	*/
}

void Polyline::clip(const Polyline& poly, vector<Polyline>& out) const
{
	// Use of a MagClipper 
	assert(false);
	/*
//  convert the outer in a stringline! and then clip...
	boost::geometry::model::linestring<PaperPoint> line;

	for (deque<PaperPoint>::const_iterator p = poly.polygon_.begin();
			p != poly.polygon_.end(); ++p) {
		if ( p->x_ == begin()->x_ )
			line.push_back(PaperPoint(p->x_+1, p->y_));
		else
			line.push_back(*p);

	}

	vector<boost::geometry::model::linestring<PaperPoint> > clip;
	boost::geometry::intersection(line, polygon_,  clip);
	vector<boost::geometry::model::linestring<PaperPoint> >::iterator c;
	for (c = clip.begin(); c != clip.end(); ++c) {
		out.push_back(Polyline());
		out.back().copy(poly);
		for (boost::geometry::model::linestring<PaperPoint>::iterator p = c->begin(); p != c->end(); ++p) {
			out.back().polygon_.push_back(*p);
		}

	}
	*/
}

// Is the poyline included in the "other" polyline"
bool Polyline::in(const Polyline& other)
{
	assert(false);
	return false;
	//return !boost::geometry::disjoint(this->polygon_, other.polygon_);
}

// Is the pointincluded in the polyline"
bool Polyline::in(const PaperPoint& point)
{
	assert(false);
	return false;

	//return boost::geometry::covered_by(point, this->polygon_);
}

void Polyline::push_front(Polyline& other)
{
	other.polygon_.pop_back();
	polygon_.insert(polygon_.begin(),
			other.polygon_.begin(), other.polygon_.end());
}

void Polyline::push_back(Polyline& other)
{
	other.polygon_.pop_front();
		polygon_.insert(polygon_.end(),
				other.polygon_.begin(), other.polygon_.end());
}

/*! \brief Routine to check (and correct) integrity of the inner holes of polygons read from shape files.
*/
void Polyline::correct()
{
	assert(false);
	//boost::geometry::correct(polygon_);
}
bool Polyline::sanityCheck()
{
	assert(false);
    /*
    // input polygon _should_ have an outer ring larger than all its inner
    // (hole) rings. This routine enforces this rule and ensures all the
    // orientations are correct (i.e. outer ring clockwise, inners all
    // anti-clockwise).
    //
    // output flag true if input polygon modified.
	boost::geometry::correct(polygon_);
    bool io_rbModified = false;
    if (polygon_.empty())
        return io_rbModified;

    // we construct a "largest" polygon by iterating through the input.
    // the largest ring we find will be assigned to its outer,
    // the remaining rings will all be assigned to its inner holes.

    BoostPoly largest = polygon_;
    largest.inners().resize(0);

    for (int i = 0; i < holes_.size(); i++)
    {
        // Note that anticlockwise "inner" polygons that are mistakenly in the "outer"
        // position will return negativve areas. By taking the highest area we
        // guard against inner polygons as outer rings and guard against any
        // erroneous clockwise inner polygons that we might encounter.
        if (boost::geometry::area(holes_[i]) > boost::geometry::area(largest.outer()))
        {
            // we have an inner ring larger than the current outer ring.

            // move the outer ring to the set of inner rings
            largest.inners().push_back(largest.outer());
            // assign the larger ring as a new outer ring
            boost::geometry::assign(largest, holes_[i]);

            io_rbModified = true;
        }
        else
        {
            // simple copy of the inner ring
            largest.inners().push_back(holes_[i]);
        }

    }

    // if any swapping has taken place, correct and assign the output polygon
    if (io_rbModified)
    {
        // set the orientations using the boost method
        boost::geometry::correct(largest);
        polygon_ = largest;
    }

    return io_rbModified;
    */
}


double PaperPoint::distance(const PaperPoint& other) const
{
	assert (false);
	return 0;
	//return boost::geometry::distance(*this, other);
}

void Polyline::box(const PaperPoint& ll, const PaperPoint& ur)
{
	push_back(ll);
	push_back(ll.x(), ur.y());
	push_back(ur);
	push_back(ur.x(), ll.y());
	push_back(ll);
	//boost::geometry::correct(polygon_);
}
bool Polyline::concatenate(Polyline&)
{
	return false;
}
void Polyline::intersection(Polyline&)
{

}
bool Polyline::within(const PaperPoint& point) const 
{
	assert(false);
	return true;
}