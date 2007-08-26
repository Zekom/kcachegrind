/* This file is part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/*
 * Callgraph View
 */

#ifndef CALLGRAPHVIEW_H
#define CALLGRAPHVIEW_H

#include <qwidget.h>
#include <qmap.h>
#include <qtimer.h>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPixmap>
#include <QFocusEvent>
#include <Q3PointArray>
#include <Q3PtrList>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QMouseEvent>

#include "treemap.h" // for DrawParams
#include "tracedata.h"
#include "traceitemview.h"

class Q3Process;

class KTemporaryFile;
class CanvasNode;
class CanvasEdge;
class GraphEdge;
class CallGraphView;


// sorts according start/end position of a call arc
// this depends on attached CanvasEdge's !
class GraphEdgeList : public Q3PtrList<GraphEdge>
{
public:
	GraphEdgeList();
	void setSortCallerPos(bool b)
	{
		_sortCallerPos = b;
	}

protected:
	int compareItems(Item item1, Item item2);

private:
	bool _sortCallerPos;
};


// temporary parts of call graph to be shown
class GraphNode
{
public:
	GraphNode();

	TraceFunction* function()
	{
		return _f;
	}
	void setFunction(TraceFunction* f)
	{
		_f = f;
	}

	CanvasNode* canvasNode()
	{
		return _cn;
	}
	void setCanvasNode(CanvasNode* cn)
	{
		_cn = cn;
	}

	bool isVisible()
	{
		return _visible;
	}
	void setVisible(bool v)
	{
		_visible = v;
	}

	// keyboard navigation
	TraceCall* visibleCaller();
	TraceCall* visibleCalling();
	void setCalling(GraphEdge*);
	void setCaller(GraphEdge*);
	TraceFunction* nextVisible();
	TraceFunction* priorVisible();
	TraceCall* nextVisibleCaller(GraphEdge*);
	TraceCall* nextVisibleCalling(GraphEdge*);
	TraceCall* priorVisibleCaller(GraphEdge*);
	TraceCall* priorVisibleCalling(GraphEdge*);

	double self, incl;
	GraphEdgeList callers, callings;

private:
	TraceFunction* _f;
	CanvasNode* _cn;
	bool _visible;

	// for keyboard navigation
	int _lastCallerIndex, _lastCallingIndex;
	bool _lastFromCaller;
};


class GraphEdge
{
public:
	GraphEdge();

	CanvasEdge* canvasEdge()
	{
		return _ce;
	}

	void setCanvasEdge(CanvasEdge* ce)
	{
		_ce = ce;
	}

	TraceCall* call()
	{
		return _c;
	}

	void setCall(TraceCall* c)
	{
		_c = c;
	}

	bool isVisible()
	{
		return _visible;
	}

	void setVisible(bool v)
	{
		_visible = v;
	}

	GraphNode* fromNode()
	{
		return _fromNode;
	}

	GraphNode* toNode()
	{
		return _toNode;
	}

	TraceFunction* from()
	{
		return _from;
	}

	TraceFunction* to()
	{
		return _to;
	}

	// has special cases for collapsed edges
	QString prettyName();

	void setCaller(TraceFunction* f)
	{
		_from = f;
	}

	void setCalling(TraceFunction* f)
	{
		_to = f;
	}

	void setCallerNode(GraphNode* n)
	{
		_fromNode = n;
	}

	void setCallingNode(GraphNode* n)
	{
		_toNode = n;
	}

	// keyboard navigation
	TraceFunction* visibleCaller();
	TraceFunction* visibleCalling();
	TraceCall* nextVisible();
	TraceCall* priorVisible();

	double cost, count;

private:
	// we have a _c *and* _from/_to because for collapsed edges,
	// only _to or _from will be unequal NULL
	TraceCall* _c;
	TraceFunction * _from, * _to;
	GraphNode *_fromNode, *_toNode;
	CanvasEdge* _ce;
	bool _visible;
	// for keyboard navigation: have we last reached this edge via a caller?
	bool _lastFromCaller;
};


typedef QMap<TraceFunction*, GraphNode> GraphNodeMap;
typedef QMap<QPair<TraceFunction*, TraceFunction*>, GraphEdge> GraphEdgeMap;


/* Abstract Interface for graph options */
class GraphOptions
{
public:
	enum Layout {TopDown, LeftRight, Circular};
	virtual ~GraphOptions() {}
	virtual double funcLimit() = 0;
	virtual double callLimit() = 0;
	virtual int maxCallerDepth() = 0;
	virtual int maxCallingDepth() = 0;
	virtual bool showSkipped() = 0;
	virtual bool expandCycles() = 0;
	virtual bool clusterGroups() = 0;
	virtual int detailLevel() = 0;
	virtual Layout layout() = 0;

	static QString layoutString(Layout);
	static Layout layout(QString);
};

/* Graph Options Storage */
class StorableGraphOptions: public GraphOptions
{
 public:
    StorableGraphOptions();
	virtual ~StorableGraphOptions(){}
    // implementation of getters
    virtual double funcLimit() { return _funcLimit; }
    virtual double callLimit() { return _callLimit; }
    virtual int maxCallerDepth() { return _maxCallerDepth; }
    virtual int maxCallingDepth() { return _maxCallingDepth; }
    virtual bool showSkipped() { return _showSkipped; }
    virtual bool expandCycles() { return _expandCycles; }
    virtual bool clusterGroups() { return _clusterGroups; }
    virtual int detailLevel() { return _detailLevel; }
    virtual Layout layout() { return _layout; }

    // setters
    void setMaxCallerDepth(int d) { _maxCallerDepth = d; }
    void setMaxCallingDepth(int d) { _maxCallingDepth = d; }
    void setFuncLimit(double l) { _funcLimit = l; }
    void setCallLimit(double l) { _callLimit = l; }
    void setShowSkipped(bool b) { _showSkipped = b; }
    void setExpandCycles(bool b) { _expandCycles = b; }
    void setClusterGroups(bool b) { _clusterGroups = b; }
    void setDetailLevel(int l) { _detailLevel = l; }
    void setLayout(Layout l) { _layout = l; }

 protected:
    double _funcLimit, _callLimit;
    int _maxCallerDepth, _maxCallingDepth;
    bool _showSkipped, _expandCycles, _clusterGroups;
    int _detailLevel;
    Layout _layout;
};

/**
 * GraphExporter
 *
 * Generates a graph file for "dot"
 * Create an instance and
 */
class GraphExporter : public StorableGraphOptions
{
public:
	GraphExporter();
	GraphExporter(TraceData*, TraceFunction*, TraceEventType*,
	              TraceItem::CostType, QString filename = QString());
	virtual ~GraphExporter();

	void reset(TraceData*, TraceItem*, TraceEventType*, TraceItem::CostType,
			QString filename = QString());

	QString filename()
	{
		return _dotName;
	}

	int edgeCount()
	{
		return _edgeMap.count();
	}

	int nodeCount()
	{
		return _nodeMap.count();
	}

	// Set the object from which to get graph options for creation.
	// Default is this object itself (supply 0 for default)
	void setGraphOptions(GraphOptions* go = 0);

	// Create a subgraph with given limits/maxDepths
	void createGraph();

	// calls createGraph before dumping of not already created
	void writeDot();

	// to map back to structures when parsing a layouted graph

	/* <toFunc> is a helper for node() and edge().
	 * Don't use the returned pointer directly, but only with
	 * node() or edge(), because it could be a dangling pointer.
	 */
	TraceFunction* toFunc(QString);
	GraphNode* node(TraceFunction*);
	GraphEdge* edge(TraceFunction*, TraceFunction*);

	/* After CanvasEdges are attached to GraphEdges, we can
	 * sort the incoming and outgoing edges of all nodes
	 * regarding start/end points for keyboard navigation
	 */
	void sortEdges();

private:
	void buildGraph(TraceFunction*, int, bool, double);

	QString _dotName;
	TraceItem* _item;
	TraceEventType* _eventType;
	TraceItem::CostType _groupType;
	KTemporaryFile* _tmpFile;
	double _realFuncLimit, _realCallLimit;
	int _maxDepth;
	bool _graphCreated;

	GraphOptions* _go;

	// optional graph attributes
	bool _useBox;

	// graph parts written to file
	GraphNodeMap _nodeMap;
	GraphEdgeMap _edgeMap;
};


/**
 * A panner laid over a QGraphicsScene
 */
class PanningView : public QGraphicsView
{
	Q_OBJECT

public:
	PanningView(QWidget * parent = 0);

	void setZoomRect(const QRectF& r);

signals:
	void zoomRectMoved(qreal dx, qreal dy);
	void zoomRectMoveFinished();

protected:
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void drawForeground(QPainter * p, const QRectF&);

	QRectF _zoomRect;
	bool _movingZoomRect;
	QPointF _lastPos;
};


/*
 * Canvas Items:
 * - CanvasNode       (Rectangular Area)
 * - CanvasEdge       (Spline curve)
 * - CanvasEdgeLabel  (Label for edges)
 * - CanvasEdgeArrow  (Arrows at the end of the edge spline)
 * - CanvasFrame      (Grey background blending to show active node)
 */

enum {
	CANVAS_NODE = 1122,
	CANVAS_EDGE, CANVAS_EDGELABEL, CANVAS_EDGEARROW,
	CANVAS_FRAME
};

class CanvasNode : public QGraphicsRectItem, public StoredDrawParams
{
public:
	CanvasNode(CallGraphView*, GraphNode*, int, int, int, int);

	void updateGroup();
	void setSelected(bool);
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

	GraphNode* node()
	{
		return _node;
	}

	int type() const
	{
		return CANVAS_NODE;
	}

private:
	GraphNode* _node;
	CallGraphView* _view;
};


class CanvasEdgeLabel : public QGraphicsRectItem, public StoredDrawParams
{
public:
	CanvasEdgeLabel(CallGraphView*, CanvasEdge*, int, int, int, int);

	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

	CanvasEdge* canvasEdge()
	{
		return _ce;
	}

	int type() const
	{
		return CANVAS_EDGELABEL;
	}

private:
	CanvasEdge* _ce;
	CallGraphView* _view;
};


class CanvasEdgeArrow : public QGraphicsPolygonItem
{
public:
	CanvasEdgeArrow(CanvasEdge*);

	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

	CanvasEdge* canvasEdge()
	{
		return _ce;
	}

	int type() const
	{
		return CANVAS_EDGEARROW;
	}

private:
	CanvasEdge* _ce;
};


class CanvasEdge : public QGraphicsPathItem
{
public:
	CanvasEdge(GraphEdge*);

	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

	void setSelected(bool);

	CanvasEdgeLabel* label()
	{
		return _label;
	}

	void setLabel(CanvasEdgeLabel* l)
	{
		_label = l;
	}

	CanvasEdgeArrow* arrow()
	{
		return _arrow;
	}

	void setArrow(CanvasEdgeArrow* a)
	{
		_arrow = a;
	}

	const Q3PointArray& controlPoints()
	{
		return _points;
	}

	void setControlPoints(const Q3PointArray& a);

	GraphEdge* edge()
	{
		return _edge;
	}

	int type() const
	{
		return CANVAS_EDGE;
	}

private:
	GraphEdge* _edge;
	CanvasEdgeLabel* _label;
	CanvasEdgeArrow* _arrow;
	Q3PointArray _points;
};


class CanvasFrame : public QGraphicsRectItem
{
public:
	CanvasFrame(CanvasNode*);

	int type() const
	{
		return CANVAS_FRAME;
	}

	bool hit(const QPoint&) const
	{
		return false;
	}

	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

private:
	static QPixmap* _p;
};


class CallGraphTip;

/**
 * A QGraphicsView showing a part of the call graph
 * and another zoomed out CanvasView in a border acting as
 * a panner to select to visible part (only if needed)
 */
class CallGraphView : public QGraphicsView, public TraceItemView,
    public StorableGraphOptions
{
	Q_OBJECT

public:
	enum ZoomPosition {TopLeft, TopRight, BottomLeft, BottomRight, Auto};

	CallGraphView(TraceItemView* parentView, QWidget* parent=0,
	        const char* name = 0);
	~CallGraphView();

	void readViewConfig(KConfig*, QString prefix, QString postfix, bool);
	void saveViewConfig(KConfig*, QString prefix, QString postfix, bool);

	QWidget* widget()
	{
		return this;
	}

	QString whatsThis() const;

	ZoomPosition zoomPos() const
	{
		return _zoomPosition;
	}

	static ZoomPosition zoomPos(QString);
	static QString zoomPosString(ZoomPosition);

public slots:
	void zoomRectMoved(qreal, qreal);
	void zoomRectMoveFinished();

	void showRenderWarning();
	void stopRendering();
	void readDotOutput();
	void dotExited();

protected:
	void resizeEvent(QResizeEvent*);
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseDoubleClickEvent(QMouseEvent*);
	void contextMenuEvent(QContextMenuEvent*);
	void keyPressEvent(QKeyEvent*);
	void focusInEvent(QFocusEvent*);
	void focusOutEvent(QFocusEvent*);
	void scrollContentsBy(int dx, int dy);

private:
	void updateSizes(QSize s = QSize(0,0));
	TraceItem* canShow(TraceItem*);
	void doUpdate(int);
	void refresh();
	void makeFrame(CanvasNode*, bool active);
	void clear();
	void showText(QString);

	QGraphicsScene *_scene;
	int _xMargin, _yMargin;
	PanningView *_panningView;
	double _panningZoom;

	CallGraphTip* _tip;

	bool _isMoving;
	QPoint _lastPos;

	GraphExporter _exporter;

	GraphNode* _selectedNode;
	GraphEdge* _selectedEdge;

	// widget options
	ZoomPosition _zoomPosition, _lastAutoPosition;

	// background rendering
	Q3Process* _renderProcess;
	QTimer _renderTimer;
	GraphNode* _prevSelectedNode;
	QPoint _prevSelectedPos;
	QString _unparsedOutput;
};


#endif
