/*! \file SceneVisitor.h
    \brief Definition of the Template class SceneVisitor.
    
    Magics Team - ECMWF 2008
    
    Started: Mon 29-Dec-2008
    
    Changes:
    
*/

#ifndef SceneVisitor_H
#define SceneVisitor_H

#include "magics.h"
#include "BasicGraphicsObject.h"
#include "BasicSceneObject.h"

namespace magics {
class Layout;


class SceneVisitor  {

public:
	SceneVisitor();
	virtual ~SceneVisitor();
	bool reproject(BasicGraphicsObjectContainer&) const;
	virtual void visit(BasicSceneObject& object) = 0;
	virtual void visit(BasicGraphicsObjectContainer&) {}
protected:
     //! Method to print string about this class on to a stream of type ostream (virtual).
	 virtual void print(ostream&) const; 

private:
    //! Copy constructor - No copy allowed
	SceneVisitor(const SceneVisitor&);
    //! Overloaded << operator to copy - No copy allowed
	SceneVisitor& operator=(const SceneVisitor&);

// -- Friends
    //! Overloaded << operator to call print().
	friend ostream& operator<<(ostream& s,const SceneVisitor& p)
		{ p.print(s); return s; }

};




class LayoutVisitor: public SceneVisitor
{
public:
	LayoutVisitor(): layout_(0), current_(0)  {}
	~LayoutVisitor() { if ( layout_ ) delete layout_; }
	void redisplay(const BaseDriver& driver) const;
	
	virtual void reset() {};
	virtual void set(MagnifierCollector&) {}
	virtual void newLayout() const;

	Layout& layout() const;
	Layout* layoutPtr() const;
	Layout* mainLayout() const;
	
	
	const Transformation& transformation() const { 
	    	ASSERT(layout_); 
	    	return layout_->transformation(); 
	    }
	void push_back(BasicGraphicsObject* object);
	
	void parent(BasicGraphicsObjectContainer* parent) 
		{  ASSERT(layout_); layout_->parent(parent); }
	double minX()  const 
		{ ASSERT(layout_);  return layout_->minX(); }
	double maxX()  const 
		{  ASSERT(layout_); return layout_->maxX(); }
	double minY()  const
		{   ASSERT(layout_); return layout_->minY(); }
	double maxY()  const 
		{  ASSERT(layout_); return layout_->maxY(); }
	void width(double width)   
		{  ASSERT(layout_); return layout_->width(width); }
	void height(double height)  
		{  ASSERT(layout_); return layout_->height(height); }
	void widthResolution(double width)   
		{  ASSERT(layout_); return layout_->widthResolution(width); }
	void heightResolution(double height)  
		{  ASSERT(layout_); return layout_->heightResolution(height); }
	void x(double x) 
		{  ASSERT(layout_); return layout_->x(x); }
	void y(double y) 
		{  ASSERT(layout_); return layout_->y(y); }
	 void transformation(Layout& layout) const
	    { layout.transformation(layout_->transformation_);  }
	void transformation(Transformation* transformation)   
		{  ASSERT(layout_); return layout_->transformation(transformation); }
	void id(const string& id)   {  ASSERT(layout_); return layout_->id(id); }
	void zoomable(bool zoomable)   {  ASSERT(layout_); return layout_->zoomable(zoomable); }
	void zoomLevels(int zl)   {  ASSERT(layout_); return layout_->zoomLevels(zl); }
	void zoomCurrentLevel(int level)   {  ASSERT(layout_); return layout_->zoomCurrentLevel(level); }
	void frameIt()   {  ASSERT(layout_); return layout_->frameIt(); }
	void frame(Layout& layout) { ASSERT(layout_); layout_->frame(layout); }
protected:
	void print(ostream&) const; 
	Layout* layout_; 	 
	mutable Layout* current_;
};


class DrawingVisitor: public LayoutVisitor
{
public:
	DrawingVisitor();
	~DrawingVisitor();
	void visit(BasicSceneObject& object) { object.visit(*this); }
	void set(MagnifierCollector&);
	//Layout* execute(AnimationStep&, const Layout* visitor);
	
	
protected:
	
};


class HorizontalAxisVisitor: public LayoutVisitor
{
public:
	HorizontalAxisVisitor(const DrawingVisitor&);
	~HorizontalAxisVisitor();
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
		virtual double offsetTickLabel(double, double);
		virtual double angleTickLabel();	
		virtual double angleTitle();
		virtual double offsetTitle(int);
		virtual double angleTip();
		virtual double offsetTip();
		virtual VerticalAlign textAlignment(const string&) { return MBOTTOM; }
	virtual Justification justificationTickLabel(const string&) { return MCENTRE; }
protected:
	
	 
};
class VerticalAxisVisitor: public LayoutVisitor
{
public:
	VerticalAxisVisitor(const DrawingVisitor&);
	~VerticalAxisVisitor();
	virtual double offsetLine() { return 10; }
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
	virtual double offsetTickLabel(double, double);
	virtual double angleTickLabel();	
	virtual double angleTitle();
	virtual double offsetTitle(int);
	virtual double angleTip();
	virtual double offsetTip();
	virtual Justification justificationTickLabel(const string&);
	virtual VerticalAlign textAlignment(const string&) { return MHALF; }
protected:
	
	 
};
class LeftAxisVisitor: public VerticalAxisVisitor
{
public:
	LeftAxisVisitor(const DrawingVisitor&);
	~LeftAxisVisitor();
	void visit(BasicSceneObject& object) { object.visit(*this); }
	virtual Justification justificationTickLabel(const string&);
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
	virtual double offsetTickLabel(double, double);
	virtual double angleTickLabel();	
	virtual double angleTitle();
	virtual double offsetTitle(int);
	virtual double angleTip();
	virtual double offsetTip();
	virtual VerticalAlign textAlignment(const string&);
protected:
	
	 
};

class RightAxisVisitor: public VerticalAxisVisitor
{
public:
	RightAxisVisitor(const DrawingVisitor&);
	~RightAxisVisitor();
	void visit(BasicSceneObject& object) { object.visit(*this); }
	virtual Justification justificationTickLabel(const string&);
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
	virtual double offsetTickLabel(double, double);
	virtual double angleTickLabel();	
	virtual double angleTitle();
	virtual double offsetTitle(int);
	virtual double angleTip();
	virtual double offsetTip();
	virtual VerticalAlign textAlignment(const string&);
protected:
	
	
};



class TopAxisVisitor: public HorizontalAxisVisitor
{
public:
	TopAxisVisitor(const DrawingVisitor&);
	~TopAxisVisitor();
	void visit(BasicSceneObject& object) { object.visit(*this); }
	virtual Justification justificationTickLabel(const string&);
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
	virtual double offsetTickLabel(double, double);
	virtual double angleTickLabel();	
	virtual double angleTitle();
	virtual double offsetTitle(int);
	virtual double angleTip();
	virtual double offsetTip();
	virtual VerticalAlign textAlignment(const string&);
	
protected:
	
};

class BottomAxisVisitor: public HorizontalAxisVisitor
{
public:
	BottomAxisVisitor(const DrawingVisitor&);
	~BottomAxisVisitor();
	void visit(BasicSceneObject& object) { object.visit(*this); }
	virtual void tick(double&, double&, bool);
	virtual void minortick(double&, double&, bool);
	virtual Justification justificationTickLabel(const string&);
	virtual double offsetTickLabel(double, double);
	virtual double angleTickLabel();	
	virtual double angleTitle();
	virtual double offsetTitle(int);
	virtual double angleTip();
	virtual double offsetTip();
	virtual VerticalAlign textAlignment(const string&);
protected:
	
	
};

} // namespace magics
#endif
