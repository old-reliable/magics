/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/*! \file RootSceneNode.h
    \brief Definition of the Template class RootSceneNode.

    Magics Team - ECMWF 2007

    Started: Thu 1-Mar-2007

    Changes:

*/

#ifndef RootSceneNode_H
#define RootSceneNode_H

#include "magics.h"

#include "BasicSceneObject.h"
#include "FortranRootSceneNodeAttributes.h"
#include "WrepRootNodeAttributes.h"
#include "XmlRootNodeAttributes.h"

namespace magics {


class RootSceneNode;

class RootScenePage : public BasicSceneNode {
public:
    RootScenePage();
    RootScenePage(double, double);
    ~RootScenePage();

    BasicSceneNode* clone() { return (width_) ? new RootScenePage(width_, height_) : new RootScenePage(); }
    virtual RootScenePage* newPage() { return (width_) ? new RootScenePage(width_, height_) : new RootScenePage(); }
    void visit(BasicGraphicsObjectContainer& tree) {
        tree.push_back(new StartPage());
        BasicSceneNode::visit(tree);
        tree.push_back(new EndPage());
    }
    BasicSceneNode* newNode(BasicPositionalObject*);
    void root(RootSceneNode* root) { root_ = root; }
    BasicGraphicsObject* visualise();
    void resize(double width, double height);
    void release();

protected:
    RootSceneNode* root_;
    double width_;
    double height_;
};
class MvRootScenePage : public RootScenePage {
public:
    MvRootScenePage();
    ~MvRootScenePage();

    RootScenePage* newPage() { return new MvRootScenePage(); }
    void visit(BasicGraphicsObjectContainer& tree) {
        MagLog::dev() << "visit(BasicGraphicsObjectContainer::MvRootScenePage" << endl;
        BasicSceneNode::visit(tree);
    }
    void getReady();
};
class RootSceneNode : public BasicSceneNode {
public:
    RootSceneNode();

    virtual ~RootSceneNode();
    virtual void setPage(RootScenePage* node);


    double absoluteWidth() const { return absoluteWidth_; }
    double absoluteHeight() const { return absoluteHeight_; }
    virtual void absoluteRootWidth(double width) {
        scale_ = width / absoluteWidth_;
        if (scale_ < 1)
            scale_ = 1;
        absoluteWidth_ = width;
    }
    void absoluteRootHeight(double height) { absoluteHeight_ = height; }
    double scalingFactor() const { return scale_; }  // For Magml and wrep ...
    virtual BasicSceneNode* clone();
    BasicGraphicsObject* root();
    virtual void getReady() {}
    void execute();
    BasicGraphicsObject* visualise();
    BasicGraphicsObject* close();
    void setPage(Layout&);
    void release();
    void newpage() { current_->newpage(); }
    BasicSceneNode* insert(BasicPositionalObject*);

    virtual int rootWidthResolution() const { return widthResolution_; }
    virtual int rootHeightResolution() const { return heightResolution_; }
    // void push_back(BasicSceneObject* item);

    virtual void text(TextVisitor*) {}
    virtual void legend(LegendVisitor*) {}
    virtual MagicsMode mode() { return mode_; }
    void mode(MagicsMode mode) { mode_ = mode; }
    BasicSceneObject* current() { return current_; }
    bool needStartPage();
    bool needEndPage();

protected:
    //! Method to print string about this class on to a stream of type ostream (virtual).
    virtual void print(ostream&) const;
    double absoluteWidth_;
    double absoluteHeight_;

    int widthResolution_;
    int heightResolution_;

    RootScenePage* current_;
    RootScenePage* latest_;
    double scale_;
    MagicsMode mode_;
    bool newpage_;
    bool endpage_;


private:
    //! Copy constructor - No copy allowed
    RootSceneNode(const RootSceneNode&);
    //! Overloaded << operator to copy - No copy allowed
    RootSceneNode& operator=(const RootSceneNode&);

    // -- Friends
    //! Overloaded << operator to call print().
    friend ostream& operator<<(ostream& s, const RootSceneNode& p) {
        p.print(s);
        return s;
    }
};


class FortranRootSceneNode : public RootSceneNode, public FortranRootSceneNodeAttributes {
public:
    FortranRootSceneNode();
    ~FortranRootSceneNode();
    void getReady();
    BasicSceneNode* clone();
    virtual void setPage(RootScenePage* node);
    string theme() const { return theme_; }


protected:
    void print(ostream&) const;
};

class MvRootSceneNode : public RootSceneNode, public FortranRootSceneNodeAttributes {
public:
    MvRootSceneNode();
    ~MvRootSceneNode();
    void getReady();

protected:
    void print(ostream&) const;
};

class XmlRootSceneNode : public RootSceneNode, public XmlRootNodeAttributes {
public:
    XmlRootSceneNode();
    ~XmlRootSceneNode();

    void set(const map<string, string>& map) { XmlRootNodeAttributes::set(map); }

    void set(const XmlNode& node) { XmlRootNodeAttributes::set(node); }

    void getReady();

protected:
    void print(ostream&) const;
};

class WrepRootSceneNode : public XmlRootSceneNode, public WrepRootNodeAttributes {
public:
    WrepRootSceneNode();
    ~WrepRootSceneNode();

    void set(const map<string, string>& map) { WrepRootNodeAttributes::set(map); }

    void set(const XmlNode& node) { WrepRootNodeAttributes::set(node); }

    void absoluteRootWidth(double width);
    void getReady();

protected:
    void print(ostream&) const;
};
class LegacyRootSceneNode : public WrepRootSceneNode {
public:
    LegacyRootSceneNode();
    ~LegacyRootSceneNode();
    void absoluteRootWidth(double width);
    void getReady();

protected:
    void print(ostream&) const;
};

}  // namespace magics
#endif
