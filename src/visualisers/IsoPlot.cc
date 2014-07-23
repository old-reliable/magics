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

/*! \file IsoPlot.cc  
 \brief Implementation of the Template class IsoPlot.
 
 Magics Team - ECMWF 2004
 
 Started: Wed 3-Mar-2004
  
 Changes:
 */
 
 
#include "IsoPlot.h"
#include "Factory.h"
#include "MatrixHandler.h"
#include "Timer.h"
#include "UserPoint.h"
#include "IsoHighlight.h"
#include "Colour.h"
#include "AutoLock.h"
#include "ThreadControler.h"
#include "LegendVisitor.h"
#include "Histogram.h"



namespace magics {
  
 static double testLevels;

static MutexCond producerMutex_;

 IsoPlot::IsoPlot() {
    setTag("isoline");
}

 IsoPlot::~IsoPlot() {
}

/*!
 Class information are given to the output-stream.
 */
 void IsoPlot::print(ostream& out) const {
    out << "IsoPlot[";
    IsoPlotAttributes::print(out);
    out << "]";
}


/*! \brief Helper class for Countouring
 * 
 */

class CellBox : public VectorOfPointers<vector<CellBox* > >
{
public:
    CellBox(const CellArray* parent, int row1, int row2, int column1, int column2) :
             parent_(parent), row1_(row1), row2_(row2), column1_(column1), column2_(column2) {
        shape_ = 0;


    }

    CellBox(const CellArray* parent) :
            parent_(parent), row1_(0), row2_(parent->rows_-1), column1_(0), column2_(parent->columns_-1) {
        shape_ = 0;
    }
    
    CellBox() :
            parent_(0), row1_(0), row2_(0), column1_(0), column2_(0) {
    		shape_ = 0;
    }

     ~CellBox() {


    }
   
     double value () {
        return (*parent_)(row1_, column1_)->value(0);
    }
    
     RangeType range()
    {
        // First try to finfd iif the cell is outOfRange...
        bool out = true;
        for (int row = row1_; row <= row2_; row++) {
            for (int column = column1_; column <= column2_; column++) {
                if ((*parent_)(row, column)->range() != outOfRange)
                {
                    out = false;
                    break;
                }
            }
        }
        if ( out )
            return outOfRange;

        bool oneout = false;
        int min = INT_MAX;
        int max = INT_MIN;
        for (int row = row1_; row <= row2_; row++)
        {
           for (int column = column1_; column <= column2_; column++)
           {
             Cell* cell = (*parent_)(row, column);
             RangeType range   = cell->range();
             if ( range == outOfRange) {
                 oneout = true;
            	 continue;
             }

             if (range == multipleRange)
                return multipleRange;

             min = ( min < cell->min_ ) ? min : cell->min_;
             max = ( max > cell->max_ ) ? max : cell->max_;
             if (max-min > 0 ) 
                return multipleRange; 
           }
        }
        if ( max-min == 0 ) {
        	if ( oneout )
        		return multipleRange;
        	return singleRange;
        }

    }

    void reshape(vector<Shape*>& shapes, vector<Shape*>& out)
    {
    	Timer timer("reduce", "reduce");
    }


    void reshape(Shape* cell)
    {
        int index = cell->index_;
        if ( index < 0 ) {
        	delete cell;
        	return;
        }
        if ( cell->points_.size() < 2 ) {

        	delete cell;
        	return;
        }
        cell->points_.push_back(cell->points_.front());

        if (SegmentJoiner::isHole(cell->points_) )
        	std::reverse(cell->points_.begin(), cell->points_.end());



        HelperStruct::iterator entry = helper_.find(index);
        if (entry == helper_.end() ) {
            helper_.insert(make_pair(index, vector<Shape*>())) ;
            entry = helper_.find(index);
        }

        entry->second.push_back(cell);

    }

    void reshape(CellBox* parent)
    {
        if ( parent == this) return;
        for (HelperStruct::iterator entry = helper_.begin(); entry != helper_.end(); ++entry) {
                    for (vector<Shape*>::iterator shape = entry->second.begin(); shape != entry->second.end(); ++shape) {
                            parent->reshape(*shape);                    
                }
        }
        helper_.clear();
    }

    void split();
    
     Shape* shape(int index)
    {
        if (!shape_)
        {
        	shape_ = new Shape();
            Cell* cell;
            
            // bottom 
            for (int column = column1_; column <= column2_; column++) {
                cell = (*parent_)(row1_, column);
                shape_->push_back(cell->column(0), cell->row(0));
            }

            // right
            for (int row = row1_; row <= row2_; row++) {
                cell = (*parent_)(row, column2_);
                shape_->push_back(cell->column(1), cell->row(1));
            }

            for (int column = column2_; column >= column1_; column--) {
                cell = (*parent_)(row2_, column);
                shape_->push_back(cell->column(2), cell->row(2));
            }

            for (int row = row2_; row >= row1_; row--) {
                cell = (*parent_)(row, column1_);    
                shape_->push_back(cell->column(3), cell->row(3));
            }
        }
        shape_->index_= index;
        return shape_;
    }
    
    void shade(const IsoPlot& owner);
    

    void shade(const IsoPlot& owner, CellBox* parent) {
        RangeType def = range();
        switch (def)
        {
          case outOfRange : 
                break;

          case singleRange: 


                parent->reshape(shape(owner.shadingIndex(value())));

                break;

          default:
                split();        
                if (empty()) {
                    assert( row1_ == row2_);
                    assert( column1_ == column2_);
                    owner.isoline(*(*parent_)(row1_, column1_), this);

                }
                else {
                    for (CellBox::iterator cell = begin(); cell != end(); ++cell) {
                        (*cell)->shade(owner, parent);
                        (*cell)->reshape(parent);
                }

                }
        }
    }
    


    void contour(const IsoPlot& owner)
    {
           for (int row = row1_; row <= row2_; row++) {
                        for (int column = column1_; column <= column2_; column++) {                                                    
                                    owner.isoline(*(*parent_)(row, column));
                        }
            }            
    }

    void feed(IsoPlot& owner, BasicGraphicsObjectContainer& out)
    {
    	const Transformation& transformation = out.transformation();
    	for (vector<Polyline*>::iterator poly = polylines_.begin(); poly != polylines_.end(); ++poly) {

    		(*owner.shading_)(*poly);
    		(*poly)->correct();
    		if ( (*owner.shading_).needClipping()  ) {
    			transformation(**poly, out);
    			delete *poly;
    		}
    		else {
    			out.push_back(*poly);
    		}


    	}
    	polylines_.clear();
    }

    void finish()
    {

        Timer timer("Feed", "Feed");
        for ( HelperStruct::iterator index = helper_.begin(); index!= helper_.end(); ++index) {
        	if (index->first == -1) {
				continue;
        	}
        	SegmentJoiner joiner;
        	int count = 0;

        	for ( vector<Shape*>::iterator s = index->second.begin(); s != index->second.end(); ++s) {
        		vector<Point>& points = (*s)->points_;


        		count += (*s)->points_.size();


        	}
        	joiner.reserve(count);


        	// feed the joiner
        	for ( vector<Shape*>::iterator s = index->second.begin(); s !=index->second.end(); ++s) {
        		vector<Point>& points = (*s)->points_;


        		if ( SegmentJoiner::isHole(points) ) {

        			vector<Point>::reverse_iterator from = points.rbegin();
        			vector<Point>::reverse_iterator to = points.rbegin();
        			to++;
        			while ( to != points.rend() ) {

        				joiner.push_back(*from, *to);
        				++from;
        				++to;
        			}
        		}
        		else {

        			vector<Point>::iterator from = points.begin();
        			vector<Point>::iterator to = points.begin();
        			to++;
        			while ( to != points.end() ) {

        				joiner.push_back(*from, *to);
        				++from;
        				++to;
        			}
        		}
        	}




        	vector<vector<Point> > result;
        	vector<vector<Point> > polys;
        	list<vector<Point> > holes;



        	joiner.computePolygonLines(result);

        	Polyline* poly = 0;



            	 for(vector<vector<Point> >::iterator j = result.begin() ; j != result.end(); ++j) {
            	        if(joiner.isHole((*j))) {
            	            holes.push_back(vector<Point>());
            	            std::swap((*j),holes.back());
            	        }
            	        else {
            	            polys.push_back(vector<Point>());
            	            std::swap((*j),polys.back());
            	        }
            	    }

            	 for(vector<vector<Point> >::iterator j = polys.begin() ; j != polys.end(); ++j) {

            		 poly = new Polyline();
            		             	polylines_.push_back(poly);
            		             	poly->reserve(j->size());


            		             		poly->index(index->first);

            		             	for (vector<Point>::iterator point = j->begin(); point != j->end(); ++point)
            		             		poly->push_back(PaperPoint(point->x_, point->y_));


            	        for(std::list<vector<Point> >::iterator h = holes.begin() ; h != holes.end(); ) {
            	            list<vector<Point> >::iterator next = h; next++;


            	            	if ( joiner.pointInPoly((*h).front(), *j) ) {


            	            	poly->newHole();
            	            	for (vector<Point>::iterator point = h->begin(); point != h->end(); ++point)
            	            	   poly->push_back_hole(PaperPoint(point->x_, point->y_));

            	            	 holes.erase(h);
            	            }
            	            h = next;
            	        }

            	 }

        }

        // Release the memory.
        for ( HelperStruct::iterator colour = helper_.begin(); colour!= helper_.end(); ++colour) {
        	for ( vector<Shape*>::iterator s = colour->second.begin(); s !=colour->second.end(); ++s) {
        		delete *s;
        		*s = 0;
        	}
        	colour->second.clear();

        }

        }
    
    Shape* shape_;
    const CellArray* parent_;
    int row1_;
    int row2_;
    int column1_;
    int column2_;

    typedef map<int, vector<Shape*> > HelperStruct;

    HelperStruct helper_;
    vector<Polyline*> polylines_;
};


}  // end magics namespace


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void CellBox::shade(const IsoPlot& owner) {
        shade(owner, this);
}



void CellBox::split()
{
    if ( row1_ == row2_ && column1_ ==  column2_ ) 
            return;

    const int row    = (row2_   + row1_) /2;
    const int column = (column2_+ column1_)/2;

    if (row2_- row1_ > 1&& column2_- column1_ > 1)
    {
       //try first 2 split in columns ...
       CellBox* cell = new CellBox(parent_, row1_, row2_, column1_, column);
       RangeType def = cell->range();
            
       if ( def != multipleRange ) {
             push_back(cell);
       }
       else {
             delete cell;
             push_back(new CellBox(parent_, row1_, row, column1_, column));
             push_back(new CellBox(parent_, row+1, row2_, column1_, column));
       }
       cell = new CellBox(parent_, row1_, row2_, column+1, column2_);
       def = cell->range();

       if ( def != multipleRange ) {
             push_back(cell);
       }
       else {
             delete cell;
             push_back(new CellBox(parent_, row1_, row, column+1, column2_));
             push_back(new CellBox(parent_, row+1, row2_, column+1, column2_));
       }
       return;
    }

    if (row2_- row1_ > 0) {
       push_back(new CellBox(parent_, row1_, row, column1_, column2_));
       push_back(new CellBox(parent_, row+1, row2_, column1_, column2_));
       return;
    }

    if (column2_- column1_ > 0) {
       push_back(new CellBox(parent_, row1_, row2_, column1_, column));
       push_back(new CellBox(parent_, row1_, row2_, column+1, column2_));
       return;
    }
}





struct IsoProducerData {
public:
        IsoProducerData(bool shading, IsoPlot& parent, CellBox& cell):
              shading_(shading), parent_(parent), cell_(cell) {
           more_ = true;
        }  
        bool shading_;
        IsoPlot& parent_;
        CellBox& cell_;
        bool more_;
        MutexCond cond_;
};


class IsoProducer: public Thread {

public:    
    IsoProducer(int n, IsoProducerData& data) : n_(n), objects_(data) {}
    void run()
    {
    	{
    		Timer timer("cell", "shading");
    		( objects_.shading_ ) ? objects_.cell_.shade(objects_.parent_) : objects_.cell_.contour(objects_.parent_);

    	}
        objects_.cell_.finish();
    }
     ~IsoProducer() {}
   
protected:
     //! Method to print string about this class on to a stream of type ostream (virtual).
     void print(ostream&) const {}
    int n_;
    IsoProducerData&  objects_;

private:
    //! Copy constructor - No copy allowed
    IsoProducer(const IsoProducer&);
    //! Overloaded << operator to copy - No copy allowed
    IsoProducer& operator=(const IsoProducer&);

    // -- Friends
    //! Overloaded << operator to call print().
    friend ostream& operator<<(ostream& s,const IsoProducer& p)
        { p.print(s); return s; }
};




void IsoPlot::isoline(Cell& cell, CellBox* box) const
{
   static int cases[3][3][3] = { { { 0, 1, 2 },   { 3, 4, 5 },    { 6, 7, 8 } }, 
                               { { 9, 10, 11 }, { 12, 13, 14 }, { 15, 16, 17 } }, 
                               { { 18, 19, 20 },{ 21, 22, 23 }, { 24, 25, 0 } } };
   int p1, p2, p3;
   int current;
   double x1=0, x2=0, y1=0, y2=0;

   RangeType def = cell.range();

   if ( def == outOfRange )
        return;
   if ( def == singleRange )
   {
        if ( !box  ) // NO Shading == Nothing to do! 
            return;
        else {
            const double contour = levels_[cell.min_];
            Shape* x = new Shape();
            for (int i = 0; i < 4; i++) {
                    x->push_back(cell.column(i), cell.row(i));            
            }
            x->index_  = shading_->shadingIndex(cell.value(0));
            box->reshape(x);
            return;
        }
   }

   for (int p=0; p<2; p++)
   {
        p1 = p;
        p2 = p1+1;
        p3 = 3;

        //int count = 0;
        if (cell.missing(p1) || cell.missing(p2) || cell.missing(p3)) {

        continue;
        }


        vector< std::pair<Shape*, Shape*> > shapes;
        
        // First, build the list of isolines in this triangle...
        vector<int> levels;        

        for (int l = cell.min_; l != cell.max_; ++l) {
            const double contour = levels_[l];
            const int    out     = cases[cell.coef(p1, contour)][cell.coef(p2, contour)][cell.coef(p3, contour)];
            if ( out != 0 ) {
                levels.push_back(l);
            }
        }
        
        if ( levels.empty() && box ) {

            Shape* shape = new Shape(shading_->shadingIndex(cell.value(p1)));
            shape->push_back(cell.column(p1), cell.row(p1));
            shape->push_back(cell.column(p2), cell.row(p2));
            shape->push_back(cell.column(p3), cell.row(p3));


            box->reshape(shape);

        }
        else {

    bool complex = ( levels.size() > 1);
    for (vector<int>::const_iterator l = levels.begin(); l != levels.end(); ++l)
    {
        int level = *l;                
        // First make a quich check to see if there is at l
        
        const double contour=levels_[level];


        current = cases[cell.coef(p1, contour)][cell.coef(p2, contour)][cell.coef(p3, contour)];

        int add = 2;
        Shape* leftcell  = (box) ? new Shape(shading_->leftIndex(contour))  :0;
        Shape* rightcell = (box) ? new Shape(shading_->rightIndex(contour)) :0;
        

        switch (current)
        {
        //-------------------------------------------------------------------------------------------------------
        //     Case 0 -point out!
        //-------------------------------------------------------------------------------------------------------
        case 0:
            add = 0;
            break;
            if ( !box ) break;
            
            if (cell.height(p1, contour) < 0) {
                leftcell->push_back(cell.column(p1), cell.row(p1));
                leftcell->push_back(cell.column(p2), cell.row(p2));
                leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                rightcell->push_back(cell.column(p1), cell.row(p1));
                rightcell->push_back(cell.column(p2), cell.row(p2));
                rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 1 -Single point 3
        //-------------------------------------------------------------------------------------------------------
        case 1:
            x1=cell.column(p3);
            y1=cell.row(p3);
            add = 1;
            if ( !box) break;
            if (cell.height(p1, contour) < 0) {
                leftcell->push_back(cell.column(p1), cell.row(p1));
                leftcell->push_back(cell.column(p2), cell.row(p2));
                leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                rightcell->push_back(cell.column(p1), cell.row(p1));
                rightcell->push_back(cell.column(p2), cell.row(p2));
                rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 2 - Line between sides 2-3 and 3-1
        //-------------------------------------------------------------------------------------------------------
        case 2:
            cell.xysect(p2,p3, contour, x1, y1);
            cell.xysect(p3,p1, contour, x2, y2);
        
            if ( !box ) break;
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(cell.column(p2), cell.row(p2));
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p3), cell.row(p3));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 3 -Single point 2
        //-------------------------------------------------------------------------------------------------------
        case 3:
            x1=cell.column(p2);
            y1=cell.row(p2);
            add = 1;
            if ( !box) break;
            
            if (cell.height(p1, contour) < 0) {
                  leftcell->push_back(cell.column(p1), cell.row(p1));
                  leftcell->push_back(cell.column(p2), cell.row(p2));
                  leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                  rightcell->push_back(cell.column(p1), cell.row(p1));
                  rightcell->push_back(cell.column(p2), cell.row(p2));
                  rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 4 - Line between vertex 2 and vertex 3
        //-------------------------------------------------------------------------------------------------------
        case 4:
            
            x1=cell.column(p2);
            y1=cell.row(p2);
            x2=cell.column(p3);
            y2=cell.row(p3);
            if ( !box ) break;
            
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(cell.column(p2), cell.row(p2));
            leftcell->push_back(cell.column(p3), cell.row(p3));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 5 - Line between vertex 2 and side 3-1
        //-------------------------------------------------------------------------------------------------------
        case 5:            
            x1=cell.column(p2);
            y1=cell.row(p2);
            
            cell.xysect(p3,p1, contour,x2, y2);
            if ( !box ) break;
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p3), cell.row(p3));
            rightcell->push_back(x2, y2);   
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 6 - Line between side 1-2  and side 2.3
        //-------------------------------------------------------------------------------------------------------
        case 6:
            
            cell.xysect(p1,p2, contour, x1, y1);
            cell.xysect(p2,p3, contour, x2, y2);
    
            if ( !box ) break;
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p3), cell.row(p3));
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(x2, y2);
            
            break;
        //-------------------------------------------------------------------------------------------------------
        //     Case 7 - Line between sides 1-2 aanve vertex 3
        //-------------------------------------------------------------------------------------------------------
        case 7:
            cell.xysect(p1,p2, contour, x1, y1);
            x2=cell.column(p3);
            y2=cell.row(p3);
            
            if ( !box ) break;
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);

            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 8 - Line between sides 1-2 and 3-1
        //-------------------------------------------------------------------------------------------------------
        case 8:            
            cell.xysect(p1,p2, contour, x1, y1);
            cell.xysect(p3,p1, contour, x2, y2);
            
            if ( !box ) break;
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);

            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(cell.column(p3), cell.row(p3));
            rightcell->push_back(x2, y2);            
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 9 -single point 1
        //-------------------------------------------------------------------------------------------------------
        case 9:            
            x1=cell.column(p1);
            y1=cell.row(p1);
            add = 1;
            if ( !box) break;
            if (cell.height(p2, contour) < 0) {
                   leftcell->push_back(cell.column(p1), cell.row(p1));
                   leftcell->push_back(cell.column(p2), cell.row(p2));
                   leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                   rightcell->push_back(cell.column(p1), cell.row(p1));
                   rightcell->push_back(cell.column(p2), cell.row(p2));
                   rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 10 - Line between vertex 3 and vertex 1
        //-------------------------------------------------------------------------------------------------------
        case 10:
            
            x1=cell.column(p3);
            y1=cell.row(p3);
            x2=cell.column(p1);
            y2=cell.row(p1);
            if ( !box) break;

            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(cell.column(p2), cell.row(p2));
            leftcell->push_back(cell.column(p3), cell.row(p3));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 11 - Line between side 3-2 and vertex 1
        //-------------------------------------------------------------------------------------------------------
        case 11:
            cell.xysect(p3,p2, contour, x1, y1);
            x2=cell.column(p1);
            y2=cell.row(p1);
            if ( !box ) break;
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(cell.column(p2), cell.row(p2));
            leftcell->push_back(x1, y1);
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p3), cell.row(p3));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 12 - Line between vertex 1 and vertex 2
        //-------------------------------------------------------------------------------------------------------
        case 12:
            x1=cell.column(p1);
            y1=cell.row(p1);
            x2=cell.column(p2);
            y2=cell.row(p2);
            if ( !box ) break;
        
            leftcell->push_back(cell.column(p1), cell.row(p1));
            leftcell->push_back(cell.column(p2), cell.row(p2));
            leftcell->push_back(cell.column(p3), cell.row(p3));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 13 - Flat Area all vertex have the isoline value!
        //-------------------------------------------------------------------------------------------------------
        case 13:
            add = 0;
            if ( !box ) break;
            if (cell.height(p1, contour) < 0) {
                   leftcell->push_back(cell.column(p1), cell.row(p1));
                   leftcell->push_back(cell.column(p2), cell.row(p2));
                   leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                   rightcell->push_back(cell.column(p1), cell.row(p1));
                   rightcell->push_back(cell.column(p2), cell.row(p2));
                   rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 14 - Line between vertex 2 and vertex 1
        //-------------------------------------------------------------------------------------------------------
        case 14:
            x1=cell.column(p2);
            y1=cell.row(p2);
            x2=cell.column(p1);
            y2=cell.row(p1);
            if ( !box ) break;
            
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(cell.column(p3), cell.row(p3));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 15 - Line between vertex 1 and side3-2
        //-------------------------------------------------------------------------------------------------------
        case 15:
            x1=cell.column(p1);
            y1=cell.row(p1);
            cell.xysect(p3,p2, contour, x2, y2);
            if ( !box ) break;

            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p3), cell.row(p3));
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 16 - Line between vertex 1 and vertex 3
        //-------------------------------------------------------------------------------------------------------
        case 16:
            x1=cell.column(p1);
            y1=cell.row(p1);
            x2=cell.column(p3);
            y2=cell.row(p3);
            if ( !box ) break;
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(x2, y2);

            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 17 - single point 1
        //-------------------------------------------------------------------------------------------------------
        case 17:
            x1=cell.column(p1);
            y1=cell.row(p1);
            add = 1;
            if ( !box ) break;
            if (cell.height(p2, contour) < 0) {
                      leftcell->push_back(cell.column(p1), cell.row(p1));
                      leftcell->push_back(cell.column(p2), cell.row(p2));
                      leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                      rightcell->push_back(cell.column(p1), cell.row(p1));
                      rightcell->push_back(cell.column(p2), cell.row(p2));
                      rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 18 - Line between side3-1 and side1-2
        //-------------------------------------------------------------------------------------------------------
        case 18:
            cell.xysect(p3,p1, contour, x1, y1);
            cell.xysect(p1,p2, contour, x2, y2);
            
            if ( !box ) break;
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p2), cell.row(p2));
            leftcell->push_back(cell.column(p3), cell.row(p3));
        
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(x2, y2);
        
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 19 - Line between vertex 3 and side1-2
        //-------------------------------------------------------------------------------------------------------
        case 19:
            x1=cell.column(p3);
            y1=cell.row(p3);
        
            cell.xysect(p1,p2, contour, x2, y2);
            
            if ( !box ) break;
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p2), cell.row(p2));
        
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 20 - Line between side3-2 and side1-2
        //-------------------------------------------------------------------------------------------------------
        case 20:
            cell.xysect(p3,p2, contour, x1, y1);
            cell.xysect(p1,p2, contour, x2, y2);
            
            if ( !box ) break;
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p2), cell.row(p2));
        
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p3), cell.row(p3));
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 21 - Line between side 3-1 and vertex 2 
        //-------------------------------------------------------------------------------------------------------
        case 21:
            cell.xysect(p3,p1, contour, x1, y1);
            
            x2=cell.column(p2);
            y2=cell.row(p2);
            if ( !box ) break;
            
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p3), cell.row(p3));
                
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(cell.column(p2), cell.row(p2));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 22 - Line between vertex 3 and vertex 2
        //-------------------------------------------------------------------------------------------------------
        case 22:
            x1=cell.column(p3);
            y1=cell.row(p3);
            x2=cell.column(p2);
            y2=cell.row(p2);
            if ( !box) break;
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(cell.column(p3), cell.row(p3));
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 23 - single point 2
        //-------------------------------------------------------------------------------------------------------
        case 23:
            x1=cell.column(p2);
            y1=cell.row(p2);
            add = 1;
            if ( !box) break;
            if (cell.height(p1, contour) < 0) {
                     leftcell->push_back(cell.column(p1), cell.row(p1));
                     leftcell->push_back(cell.column(p2), cell.row(p2));
                     leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                     rightcell->push_back(cell.column(p1), cell.row(p1));
                     rightcell->push_back(cell.column(p2), cell.row(p2));
                     rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 24 - Line between side1-3  and side3-2
        //-------------------------------------------------------------------------------------------------------
        case 24:
            cell.xysect(p1,p3, contour, x1, y1);
            cell.xysect(p3,p2, contour, x2, y2);
            if ( !box) break;
            leftcell->push_back(x1, y1);
            leftcell->push_back(x2, y2);
            leftcell->push_back(cell.column(p3), cell.row(p3));
            
            rightcell->push_back(x1, y1);
            rightcell->push_back(cell.column(p1), cell.row(p1));
            rightcell->push_back(cell.column(p2), cell.row(p2));
            rightcell->push_back(x2, y2);
            break;

        //-------------------------------------------------------------------------------------------------------
        //     Case 25 - single point C
        //-------------------------------------------------------------------------------------------------------
        case 25:
            x1=cell.column(p3);
            y1=cell.row(p3);
            add = 1;
            if ( !box) break;
            if (cell.height(p1, contour) < 0) {
                     leftcell->push_back(cell.column(p1), cell.row(p1));
                     leftcell->push_back(cell.column(p2), cell.row(p2));
                     leftcell->push_back(cell.column(p3), cell.row(p3));
            }
            else {
                     rightcell->push_back(cell.column(p1), cell.row(p1));
                     rightcell->push_back(cell.column(p2), cell.row(p2));
                     rightcell->push_back(cell.column(p3), cell.row(p3));
            }
            break;
            
        default:
            break;
        }
        
        if (add == 2 && needIsolines() )
        {
            // here we have 2 Points to add! 
            // We send it to a thread! 
            const int t = (*l) % threads_;
            {     
              AutoLock<MutexCond> lockproducer(producerMutex_);
              {
                 AutoLock<MutexCond> lock(segments_[t]->cond_);
                 segments_[t]->segments_.push_back(make_pair(levels_[*l], 
                                                   std::make_pair(make_pair(x1, y1), std::make_pair(x2, y2))));
                 if ( segments_[t]->segments_.size() >= 2000 ) 
                       segments_[t]->cond_.signal();
                 }
                 producerMutex_.signal();
              }
            }

            if ( !box) {
                continue;
            }
            
            if ( !complex ) {

            	box->reshape(leftcell);
                box->reshape(rightcell);
            }
           else {
                shapes.push_back(make_pair(leftcell, rightcell));
           }
        } // end of levels...
        
        if ( shapes.empty() ) continue;
        

        // Now we reduce the shape! 

        vector< std::pair<Shape*, Shape*> >::iterator current = shapes.begin();    
        vector< std::pair<Shape*, Shape*> >::iterator next = shapes.begin();
        box->reshape(current->first);

        next++;
        while ( next != shapes.end() ) {
            current->second->intersection(*next->second);
            box->reshape(current->second);
            delete next->first;
            current++;
            next++ ;
        }
        box->reshape(current->second);

        }
    } // step to next triangle
}




void IsoPlot::isoline(MatrixHandler& data, BasicGraphicsObjectContainer& parent)
{

    const Transformation& transformation = parent.transformation();
    levels_.clear();
    if ( levelSelection_->empty() )
    	return;
    // Find the used levels!
    const vector<double>::const_iterator end = (*levelSelection_).end();
    vector<double>::const_iterator previous = (*levelSelection_).end();
    vector<double>::const_iterator last = (*levelSelection_).end();

    double min = data.min();
    double max = data.max();


    if ( (*levelSelection_).front()  > min )
            levels_.push_back(min);

    vector<double>::const_iterator level = levelSelection_->begin();

    while (*level < min && level != end ) {
    	last = level;
    	++level;
    }
    if ( last != end)
    	levels_.push_back(*last);
    while (*level < max && level != end ) {
    	levels_.push_back(*level);
    	++level;
    }
    if ( level != end ) {
    	levels_.push_back(*level);
    }
    else
    	levels_.push_back(max);



    
    missing_ = data.missing();



    IntervalMap<int> range;
    int r= 1;
    for (vector<double>::const_iterator level = levels_.begin(); level != levels_.end(); ++level) {
       MagLog::debug()  << " level " << *level << endl;
       if (level+1!= levels_.end() )
          range.insert(make_pair(Interval(*level, *(level+1)), r++));
       }
       if ( shading_->shadingMode())
    	   range.insert(make_pair(Interval(levels_.back(), levels_.back()+epsilon), r));
       CellArray* array = shading_->array(data, range, transformation, parent.widthResolution(), parent.heightResolution(),
	   		resolution_, technique_);
       CellBox view(array);

       threads_ = (needIsolines())  ? 4: 0;

       vector<IsoHelper*> consumers_;
       vector<IsoProducer* >  producers_;

       {
        VectorOfPointers<vector<ThreadControler *>  > consumers;
        VectorOfPointers<vector<ThreadControler *>  > producers;
        segments_.clear();
        colourShapes_.clear();
        lines_.clear();

        for (int c = 0; c < threads_; c++) {
            vector<Polyline* >* lines = new vector<Polyline*>();
            lines_.push_back(lines);    
            segments_.push_back(new IsoData()); 
            consumers_.push_back(new IsoHelper(c, *lines,*(segments_.back())));
            consumers.push_back(new ThreadControler(consumers_.back(), false));
            consumers.back()->start();
        }
        
        view.split();

        // let's start 4 producers...
        int c = 0;
        VectorOfPointers<vector<IsoProducerData*> > datas;
        //for (CellBox::iterator cell = view.begin(); cell != view.end(); ++cell)
        for ( int i = 0; i < view.size(); i++)

        {

           IsoProducerData* data = new IsoProducerData(shading_->shadingMode(), *this, *(view[i]));
           datas.push_back(data);
           producers_.push_back(new IsoProducer(c, *data));
           producers.push_back(new ThreadControler(producers_.back(), false));
           producers.back()->start();
           c++;

        }

        for (vector<ThreadControler *>::iterator producer = producers.begin(); 
           producer != producers.end(); ++producer) {
           (*producer)->wait();
        }

        // No more
        { 
           for (int i = 0; i < threads_; i++) {
                 AutoLock<MutexCond> lock(segments_[i]->cond_);
                 segments_[i]->more_ = false;
                 segments_[i]->cond_.signal();
           }     
        }

        for (vector<ThreadControler *>::iterator consumer = consumers.begin(); consumer != consumers.end(); ++consumer) {
             (*consumer)->wait();
        }
       }

       for (CellBox::iterator cell = view.begin(); cell != view.end(); ++cell) {
           (*cell)->feed(*this,parent);        

       }

       delete array;
       for ( vector<IsoData*>::iterator segment = segments_.begin(); segment != segments_.end(); ++segment)  {
       	   delete *segment;
       	   *segment = 0;
       }
       segments_.clear();

}


 bool IsoPlot::prepare(MatrixHandler& data)
{
	//double replaceMissing_ = 0;
	//double min = std::min(replaceMissing_, data.min());
	//double max = std::max(replaceMissing_, data.max());
	double min = data.min();
	double max = data.max();
    (*levelSelection_).clear();
    (*levelSelection_).calculate(min , max , shading_->shadingMode());
    (*label_).prepare(*levelSelection_, (*colour_).name());
    return (*shading_)(*levelSelection_);
}


/*!
 * Get the triangles list ...
 * Create the isolines...
 */
 void IsoPlot::operator()(MatrixHandler& data, BasicGraphicsObjectContainer& parent)
{
    prepare(data);
    if ( legend_only_ ) return;
    
    {
        Timer timer("contouring", "Time spent in contouring");
        isoline(data, parent);
    }

#ifdef ISOPLOT_DEBUG
    vector<Colour> colours;    
    colours.push_back(Colour("red"));
    colours.push_back(Colour("green"));
    colours.push_back(Colour("blue"));
    colours.push_back(Colour("orange"));
    vector<Colour>::iterator colour = colours.begin();
#endif

    (*shading_)(data, parent);
    (*highlight_).prepare(*levelSelection_);

    rainbowMethod_->set(*this);
    rainbowMethod_->prepare(*levelSelection_);

    // Now we feed the task...   
    for (vector<vector<Polyline* >* >::const_iterator lines = lines_.begin(); lines != lines_.end(); ++lines)
    {
      for (vector<Polyline* >::const_iterator poly = (*lines)->begin(); poly != (*lines)->end(); ++poly)
      {
        if ( (*poly)->empty() ) continue;
        (*poly)->setLineStyle(style_);
        (*poly)->setThickness(thickness_);

        if ( !rainbow_ )
        	(*poly)->setColour(*colour_);
        else
        	(*poly)->setColour((*rainbowMethod_)((*poly)->front().value()));
#ifdef ISOPLOT_DEBUG
        (*poly)->setColour(*colour);
        colour++;
        if ( colour == colours.end()) 
            colour = colours.begin();
#endif
        (*highlight_)(*(*poly));
        if ( rainbow_ ) {
        	(*poly)->setColour((*rainbowMethod_)((*poly)->front().value()));
        }
        (*label_)(**poly, (*poly)->front().value());



        parent.push_back(*poly);
      }
      delete *lines;

    }
    lines_.clear();
}



 void NoIsoPlot::operator()(MatrixHandler& data, BasicGraphicsObjectContainer& parent)
{
    // Create the isolines...        
    if ( !prepare(data) ) {    
        if ( legend_only_ ) return;
        (*shading_)(data, parent);
        // do not send the isolines...  
        return;
    }
    if ( legend_only_ ) return;
    // The shading needs the isolines..
    // WE will calculate them but will not send them to the printer     
    {
        Timer timer("contouring", "Time spent in contouring");
        isoline(data, parent);
    }

    (*shading_)(data, parent);

    // Now we feed the task...
        for (vector<vector<Polyline* >* >::const_iterator lines = lines_.begin(); lines != lines_.end(); ++lines)
        {
          for (vector<Polyline* >::const_iterator poly = (*lines)->begin(); poly != (*lines)->end(); ++poly)
          {
            if ( (*poly)->empty() ) continue;
            (*poly)->setThickness(0);
            (*poly)->setColour(Colour("black"));
            (*label_)(**poly, (*poly)->front().value());
            parent.push_back(*poly);
          }
        }
        lines_.clear();
}



void IsoPlot::visit(Data& data, LegendVisitor& legend) {

	if ( magCompare(legend_special_, "spaghetti") ) {
		Polyline* blue = new Polyline();
		blue->setColour(Colour("blue"));
		blue->setLineStyle(M_DASH);
		Polyline* red = new Polyline();
		red->setColour(Colour("red"));
		red->setLineStyle(M_DASH);
		Polyline* grey = new Polyline();
		grey->setColour(Colour("grey"));
		grey->setLineStyle(M_SOLID);
		legend.add(new DoubleLineEntry("Det", blue,0));
		legend.add(new DoubleLineEntry("Control", red,0));
		legend.add(new DoubleLineEntry("EPS members", grey,0));
		return;
	}
	switch (legend.legendType())  {
		case LegendMethod::CONTINUOUS :
		case LegendMethod::DISJOINT: {
			(*shading_).visit(legend);
			if (shading_->hasLegend() ) return;

			Polyline* line1 = new Polyline();
			Polyline* line2 = 0;
			line1->setColour(*colour_);
			line1->setLineStyle(style_);
			line1->setThickness(thickness_);
    
			highlight_->visit(line2);
			legend.add(new DoubleLineEntry(legend_text_, line1, line2));
			// Should do something for the wrep legend!!!
			if ( legend.size() < 3 && legend.wrep_ ) {
				legend.add(new EmptyEntry());
			}
			break;
		}
		case LegendMethod::HISTOGRAM: {
			legend.newLegend();
			IntervalMap<Colour> beans;
			vector<double>::iterator from = levelSelection_->begin();
			vector<double>::iterator to = levelSelection_->begin();
			++to;
			for (;  to != levelSelection_->end(); ++to){
				Colour colour = *colour_;
				shading_->colour(*from, colour);
				beans.insert(make_pair(Interval(*from, *to), colour ));
				++from;
			}
			Histogram helper;
			const Transformation& transformation = legend.transformation();
			IntervalMap<int>& histogram = helper.histogram(beans, data.points(transformation,false));
			int total = 0;
			for (IntervalMap<int>::const_iterator  interval = histogram.begin(); interval != histogram.end(); ++interval){
				total+=interval->second;
			}
			bool first = true;
			for ( IntervalMap<Colour>::const_iterator interval = beans.begin(); interval != beans.end(); ++interval) {
				   Polyline* box = new Polyline();

				   double min =interval->first.min_;
				   double max = interval->first.max_;

				   box->setShading(new FillShadingProperties());
				   box->setFillColour(interval->second);
				   box->setFilled(true);
				   BoxEntry* entry = new BoxEntry(min, max, box);
				   int count = histogram.find(min, 0);
				   entry->population(count);
				   entry->totalPopulation(total);
				   if (first) {
					   entry->first();
					   first = false;
				   }


				   legend.add(entry);

			}

			legend.last();
		}
	}
    
}


 void NoIsoPlot::visit(Data& data, LegendVisitor& legend) {

    switch (legend.legendType())  {
    		case LegendMethod::CONTINUOUS :
    		case LegendMethod::DISJOINT: {
    			(*shading_).visit(legend);
    			break;
    		}
    		case LegendMethod::HISTOGRAM: {
    			legend.newLegend();
    			IntervalMap<Colour> beans;
    			vector<double>::iterator from = levelSelection_->begin();
    			vector<double>::iterator to = levelSelection_->begin();
    			++to;
    			for (;  to != levelSelection_->end(); ++to){
    				Colour colour = *colour_;
    				shading_->colour(*from, colour);
    				beans.insert(make_pair(Interval(*from, *to), colour ));
    				++from;
    			}
    			Histogram helper;
    			IntervalMap<int>& histogram = helper.histogram(beans, data.points(legend.transformation(), true));
    			int total = 0;
    			for (IntervalMap<int>::const_iterator  interval = histogram.begin(); interval != histogram.end(); ++interval){
    				total+=interval->second;
    			}
    			bool first = true;
    			for ( IntervalMap<Colour>::const_iterator interval = beans.begin(); interval != beans.end(); ++interval) {
    				   Polyline* box = new Polyline();

    				   double min =interval->first.min_;
    				   double max = interval->first.max_;

    				   box->setShading(new FillShadingProperties());
    				   box->setFillColour(interval->second);
    				   box->setFilled(true);
    				   BoxEntry* entry = new BoxEntry(min, max, box);
    				   int count = histogram.find(min, 0);
    				   entry->population(count);
    				   entry->totalPopulation(total);
    				   if (first) {
    					   entry->first();
    					   first = false;
    				   }


    				   legend.add(entry);

    			}

    			legend.last();
    		}
    	}
}



void IsoPlot::visit(Data& data, PointsHandler& points, HistoVisitor& visitor)
{
    IntervalMap<Colour> beans;
    if ( !visitor.basic() ) {

        vector<double>::iterator from = levelSelection_->begin();
        vector<double>::iterator to = levelSelection_->begin();
        ++to;
        for (;  to != levelSelection_->end(); ++to){
            Colour colour = *colour_;
            shading_->colour(*from, colour);
            beans.insert(make_pair(Interval(*from, *to), colour ));
            ++from;
        }
    }
    Histogram helper;
    helper.visit(beans, data, points, visitor);
}

CellArray::CellArray(MatrixHandler& data, IntervalMap<int>& range, const Transformation& transformation, int width, int height, float resol, const string& technique) :
		data_(data), rangeFinder_(range)
{
	Timer timer("CellArray", "CellArray");
	int r = height/resol;
	int c = (int) width/resol;
	rows_ = r;
	columns_ = c;


	points_.set(rows_+1, columns_+1);
	reserve(rows_* columns_);

//	int i = 0;

	missing_ = data.missing();
	double firstx = transformation.getMinPCX();
	double firsty = transformation.getMinPCY();

	double stepx =  ( transformation.getMaxPCX() -  transformation.getMinPCX() )/ (columns_);
	double stepy =  ( transformation.getMaxPCY() -  transformation.getMinPCY() )/ (rows_);

	{
		Timer timer("matrix", "prepare");

		vector< std::pair<double, double> > xypoints;
		vector< std::pair<double, double> > geopoints;
		double x = firstx, y = firsty;
		xypoints.reserve(rows_+1 * columns_+1);
		for (int row = 0; row <= rows_; row++) {
			    x = firstx;
				y = firsty + (row*stepy);    // multiplication here avoids accumulation of errors
			    points_.rowsAxis().push_back(y);
				for (int column = 0; column <= columns_; column++) {
					x = firstx + (column*stepx);  // multiplication here avoids accumulation of errors
					xypoints.push_back(make_pair(x, y));
					if ( row == 0) {
						points_.columnsAxis().push_back(x);
					}
				}
		}
		transformation.revert(xypoints, geopoints);
		vector< std::pair<double, double> >::iterator geo= geopoints.begin();
		double min =  data.min();
		double max =  data.max();
		double missing =  data.missing();

		MagLog::dev() << "min = " << data.min() << "  max = " << data.max() << endl;
		for (vector< std::pair<double, double> >::iterator xy = xypoints.begin(); xy != xypoints.end(); ++xy) {

					double value;
					if  ( geo->second == -1000) {

						value = missing;
					}
					else {
						value = (magCompare(technique, "nearest")) ?
							data.nearest(geo->second, geo->first):data.interpolate(geo->second, geo->first);
						//value =data.nearest(geo->second, geo->first);
					}
					if (value != missing) {
						if (value < min)
							value = min;
						if (value > max)
							value=max;
					}
					//else value = 0;



					points_.push_back(value);
					++geo;
		}
		points_.setMapsAxis();

	}

	for (int row = 0; row < rows_; row++)
		for (int column = 0; column < columns_; column++) {
			push_back(new Cell(*this, row, column));
		}
}


CellArray::CellArray(MatrixHandler& data, IntervalMap<int>& range) : data_(data), rangeFinder_(range)
{

}

GridArray::GridArray(MatrixHandler& data, IntervalMap<int>& range, const Transformation& transformation, int width, int height, float resol, const string& technique) : CellArray(data, range)
{
	Timer timer("GridArray", "GridArray");
	if ( magCompare(technique, "middle") ) {
		rows_ = data.rows();
		columns_ = data.columns();
	}
	else {
		rows_ = data.rows() -1;
		columns_ = data.columns() -1;
	}
	reserve(rows_*columns_);
	for (int row = 0; row < rows_; row++)
		for (int column = 0; column < columns_; column++) {
			push_back(new GridCell(*this, row, column, transformation, technique));
		}
}


GridCell::GridCell(const CellArray& data, int row, int column, const Transformation& transformation, const string& technique):
		Cell(data), transformation_(transformation)  {

		row_ = row;
		column_ = column;
		int row1 = (row) ? row - 1 : 0;
		int row2 = ( row == data.data_.rows() -1 ) ? row : row + 1;
		missing_ = data.missing_;
		value_ = data.data_(row_, column_);
		min_ = max_ = value_;
		int column1 = column ? column - 1 : 0;
		int column2 = ( column == data.data_.columns() -1 ) ? column : column + 1;


		range_ = data.rangeFinder_.find(value_, -1);
		min_ = max_ = range_;
		if ( range_ == -1 )
			outOfRange_ = 4;

		if ( magCompare(technique, "middle")) {

	        double rc1 = data.data_.column(row, column1);
	        double crc = data.data_.column(row, column);
	        double rc2 = data.data_.column(row, column2);
	        double r1c = data.data_.row(row1, column);
	        double r2c = data.data_.row(row2, column);
	        double rrc = data.data_.row(row, column);

			columns_[0] = (rc1 + crc)/2;
			columns_[1] = (crc + rc2)/2;
			columns_[2] = (crc + rc2)/2;
			columns_[3] = (rc1 + crc)/2;
			rows_[0] = (r1c + rrc)/2;
			rows_[1] = (r1c + rrc)/2;
			rows_[2] = (rrc + r2c)/2;
			rows_[3] = (rrc + r2c)/2;
		}

		else {

			columns_[0] = data.data_.column(row, column);
			columns_[1] = data.data_.column(row, column2);
			columns_[2] = data.data_.column(row, column2);
			columns_[3] = data.data_.column(row, column);
			rows_[0] = data.data_.row(row, column);
			rows_[1] = data.data_.row(row, column);
			rows_[2] = data.data_.row(row2, column);
			rows_[3] =  data.data_.row(row2, column);
			}

		for (int i = 0; i < 4; i++) {

			transformation.fast_reproject(columns_[i], rows_[i]);


		}

	}
void Shape::intersection(Shape& other) {

	vector<Point > opoints = other.points_;

		std::reverse(opoints.begin(), opoints.end());



	vector<Point  >::iterator p1= points_.begin();

	Point top;


	while (p1!= points_.end() ) {
		vector<Point  >::iterator p2= opoints.begin();
		while ( p2!= opoints.end()) {
			if ( *p1 == *p2) {
				std::rotate(opoints.begin(), p2, opoints.end());
				p1++;

				points_.insert(p1, opoints.begin(), opoints.end());

				//now we need to take off the duplicates!

				stack<Point > helper;
				vector<Point  >::const_iterator point = points_.begin();


				for (point = points_.begin();  point != points_.end(); ++point) {
					if ( helper.empty() ) {
						helper.push(*point);
						continue;
					}

					top =  helper.top();
					helper.pop();

					if ( !( top == *point ) ) {
						helper.push(top);
						helper.push(*point);
					}

				}
				clean();



				while ( !helper.empty() ) {
					push_back(helper.top().x_, helper.top().y_);
					helper.pop();
				}

				if ( SegmentJoiner::isHole(points_) ) {

				       std::reverse(points_.begin(), points_.end());

				      }

				return;
			}
			p2++;
		}
		p1++;
	}


}
