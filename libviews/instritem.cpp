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
 * Items of instruction view.
 */

#include "instritem.h"

#include <QPixmap>
#include <QPainter>
#include <QPolygon>

#include "globalconfig.h"
#include "listutils.h"
#include "instrview.h"


// InstrItem

// for messages
InstrItem::InstrItem(InstrView* iv, Q3ListView* parent,
		     Addr addr, const QString& msg)
    : Q3ListViewItem(parent)
{
  _view = iv;
  _addr = addr;
  _instr = 0;
  _instrCall = 0;
  _instrJump = 0;
  _inside = false;

  setText(0, addr.pretty());
  setText(6, msg);

  updateGroup();
  updateCost();
}

// for code lines
InstrItem::InstrItem(InstrView* iv, Q3ListView* parent,
		     Addr addr, bool inside,
		     const QString& code, const QString& cmd,
		     const QString& args, TraceInstr* instr)
    : Q3ListViewItem(parent)
{
  _view = iv;
  _addr = addr;
  _instr = instr;
  _instrCall = 0;
  _instrJump = 0;
  _inside = inside;

  if (args == "...")
      setText(0, args);
  else
      setText(0, addr.pretty());
  setText(4, code);
  setText(5, cmd);
  setText(6, args);

  TraceLine* l;
  if (instr && (l = instr->line()))
      setText(7, l->name());

  updateGroup();
  updateCost();
}

// for call lines
InstrItem::InstrItem(InstrView* iv, Q3ListViewItem* parent, Addr addr,
		     TraceInstr* instr, TraceInstrCall* instrCall)
    : Q3ListViewItem(parent)
{
  _view = iv;
  _addr = addr;
  _instr = instr;
  _instrCall = instrCall;
  _instrJump = 0;
  _inside = true;

  //qDebug("InstrItem: (file %d, line %d) Linecall to %s",
  //       fileno, lineno, _lineCall->call()->called()->prettyName().ascii());

  SubCost cc = _instrCall->callCount();
  QString callStr = "  ";
  if (cc==0)
    callStr += QObject::tr("Active call to '%1'").arg(_instrCall->call()->calledName());
  else
    callStr += QObject::tr("%n call(s) to '%2'", "", (uint64)cc)
                .arg(_instrCall->call()->calledName());

  TraceFunction* calledF = _instrCall->call()->called();
  calledF->addPrettyLocation(callStr);

  setText(6, callStr);

  updateGroup();
  updateCost();
}

// for jump lines
InstrItem::InstrItem(InstrView* iv, Q3ListViewItem* parent, Addr addr,
		     TraceInstr* instr, TraceInstrJump* instrJump)
    : Q3ListViewItem(parent)
{
  _view = iv;
  _addr = addr;
  _inside = true;
  _instr = instr;
  _instrCall = 0;
  _instrJump = instrJump;

  //qDebug("SourceItem: (file %d, line %d) Linecall to %s",
  //       fileno, lineno, _lineCall->call()->called()->prettyName().ascii());

  QString jStr;
  if (_instrJump->isCondJump())
      jStr = QObject::tr("Jump %1 of %2 times to 0x%3")
            .arg(_instrJump->followedCount().pretty())
            .arg(_instrJump->executedCount().pretty())
            .arg(_instrJump->instrTo()->addr().toString());
  else
      jStr = QObject::tr("Jump %1 times to 0x%2")
            .arg(_instrJump->executedCount().pretty())
            .arg(_instrJump->instrTo()->addr().toString());

  setText(6, jStr);

  updateGroup();
  updateCost();
}


void InstrItem::updateGroup()
{
  if (!_instrCall) return;

  TraceFunction* f = _instrCall->call()->called();
  QColor c = GlobalConfig::functionColor(_view->groupType(), f);
  setPixmap(6, colorPixmap(10, 10, c));
}

void InstrItem::updateCost()
{
  _pure = SubCost(0);
  _pure2 = SubCost(0);

  if (!_instr) return;
  if (_instrJump) return;

  ProfileCostArray* instrCost = _instrCall ?
      (ProfileCostArray*)_instrCall : (ProfileCostArray*)_instr;

  // do not show any cost inside of cycles
  if (_instrCall &&
      ((_instrCall->call()->inCycle()>0) ||
       (_instrCall->call()->isRecursion()))) {
    QString str;
    QPixmap p;

    QString icon = "edit-undo";
#if 0 // TODO
    KIconLoader* loader = KIconLoader::global();
    p= loader->loadIcon(icon, KIconLoader::Small, 0,
                        KIconLoader::DefaultState, QStringList(), 0, true);
#endif
    if (p.isNull())
      str = QObject::tr("(cycle)");

    setText(1, str);
    setPixmap(1, p);
    setText(2, str);
    setPixmap(2, p);
    return;
  }

  ProfileCostArray* totalCost;
  if (GlobalConfig::showExpanded())
      totalCost = _instr->function()->inclusive();
  else
      totalCost = _instr->function()->data();

  EventType *ct = _view->eventType();
  _pure = ct ? instrCost->subCost(ct) : SubCost(0);
  if (_pure == 0) {
    setText(1, QString());
    setPixmap(1, QPixmap());
  }
  else {
    double total = totalCost->subCost(ct);
    double pure  = 100.0 * _pure / total;

    if (GlobalConfig::showPercentage())
      setText(1, QString("%1")
	      .arg(pure, 0, 'f', GlobalConfig::percentPrecision()));
    else
      setText(1, _pure.pretty());

    setPixmap(1, costPixmap(ct, instrCost, total, false));
  }

  EventType *ct2 = _view->eventType2();
  _pure2 = ct2 ? instrCost->subCost(ct2) : SubCost(0);
  if (_pure2 == 0) {
    setText(2, QString());
    setPixmap(2, QPixmap());
  }
  else {
    double total = totalCost->subCost(ct2);
    double pure  = 100.0 * _pure2 / total;

    if (GlobalConfig::showPercentage())
      setText(2, QString("%1")
	      .arg(pure, 0, 'f', GlobalConfig::percentPrecision()));
    else
      setText(2, _pure2.pretty());

    setPixmap(2, costPixmap(ct2, instrCost, total, false));
  }
}


int InstrItem::compare(Q3ListViewItem * i, int col, bool ascending ) const
{
  const InstrItem* ii1 = this;
  const InstrItem* ii2 = (InstrItem*) i;

  // we always want descending order
  if (((col>0) && ascending) ||
      ((col==0) && !ascending) ) {
    ii1 = ii2;
    ii2 = this;
  }

  if (col==1) {
    if (ii1->_pure < ii2->_pure) return -1;
    if (ii1->_pure > ii2->_pure) return 1;
    return 0;
  }
  if (col==2) {
    if (ii1->_pure2 < ii2->_pure2) return -1;
    if (ii1->_pure2 > ii2->_pure2) return 1;
    return 0;
  }
  if (col==0) {
    if (ii1->_addr < ii2->_addr) return -1;
    if (ii1->_addr > ii2->_addr) return 1;

    // Same address: code gets above calls/jumps
    if (!ii1->_instrCall && !ii1->_instrJump) return -1;
    if (!ii2->_instrCall && !ii2->_instrJump) return 1;

    // calls above jumps
    if (ii1->_instrCall && !ii2->_instrCall) return -1;
    if (ii2->_instrCall && !ii1->_instrCall) return 1;

    if (ii1->_instrCall && ii2->_instrCall) {
	// Two calls: desending sort according costs
	if (ii1->_pure < ii2->_pure) return 1;
	if (ii1->_pure > ii2->_pure) return -1;

	// Two calls: sort according function names
	TraceFunction* f1 = ii1->_instrCall->call()->called();
	TraceFunction* f2 = ii2->_instrCall->call()->called();
	if (f1->prettyName() > f2->prettyName()) return 1;
	return -1;
    }

    // Two jumps: descending sort according target address
    if (ii1->_instrJump->instrTo()->addr() <
	ii2->_instrJump->instrTo()->addr())
	return -1;
    if (ii1->_instrJump->instrTo()->addr() >
	ii2->_instrJump->instrTo()->addr())
	return 1;
    return 0;

  }
  return Q3ListViewItem::compare(i, col, ascending);
}

void InstrItem::paintCell( QPainter *p, const QColorGroup &cg,
			   int column, int width, int alignment )
{
  QColorGroup _cg( cg );

  QColor color;
  if ( !_inside || ((column==1) || (column==2)))
    color = cg.color( QPalette::Button );
  else if ((_instrCall || _instrJump) && column>2)
    color = cg.color( QPalette::Midlight );
  if (color.isValid())
    _cg.setColor( listView()->viewport()->backgroundRole(), color);

  if (column == 3)
    paintArrows(p, _cg, width);
  else
    Q3ListViewItem::paintCell( p, _cg, column, width, alignment );
}

void InstrItem::setJumpArray(const QVector<TraceInstrJump*>& a)
{
    _jump = a;
}

void InstrItem::paintArrows(QPainter *p, const QColorGroup &cg, int width)
{
  Q3ListView *lv = listView();
  if ( !lv ) return;
  InstrView* iv = (InstrView*) lv;

  QPalette pal = cg;
  const QPalette::ColorRole crole = lv->viewport()->backgroundRole();
  if (pal.brush(crole) != lv->palette().brush(crole))
    p->fillRect(0, 0, width, height(), pal.brush(crole));
  else
    iv->paintEmptyArea( p, QRect( 0, 0, width, height() ) );

  if ( isSelected() && lv->allColumnsShowFocus() )
    p->fillRect( 0, 0, width, height(), cg.brush( QPalette::Highlight ) );

  int marg = lv->itemMargin();
  int yy = height()/2, y1, y2;
  QColor c;

  int start = -1, end = -1;

  // draw line borders, detect start/stop of a line
  for(int i=0;i< (int)_jump.size();i++) {
      if (_jump[i] == 0) continue;

      y1 = 0;
      y2 = height();
      if ((_instrJump == _jump[i]) &&
	  (_jump[i]->instrFrom()->addr() == _addr)) {

	  //qDebug() << "InstrItem " << _addr.toString() << ": start " << i;
	  if (start<0) start = i;
	  if (_jump[i]->instrTo()->addr() <= _addr)
	      y2 = yy;
	  else
	      y1 = yy;
      }
      else if (!_instrJump && !_instrCall &&
	       (_jump[i]->instrTo()->addr() == _addr)) {

	  //qDebug() << "InstrItem " << _addr.toString() << ": end " << i;
	  if (end<0) end = i;
	  if (_jump[i]->instrFrom()->addr() < _addr)
	      y2 = yy;
	  else
	      y1 = yy;
      }

      c = _jump[i]->isCondJump() ? Qt::red : Qt::blue;
#if 0
      if (_jump[i] == ((TraceItemView*)_view)->selectedItem()) {
	  p->fillRect( marg + 6*i-2, (y1==0) ? y1: y1-2,
		       8, (y2-y1==height())? y2:y2+2,
		       cg.brush( QPalette::Highlight ) );
	  c = lv->colorGroup().highlightedText();
      }
#endif
      p->fillRect( marg + 6*i, y1, 4, y2, c);
      p->setPen(c.light());
      p->drawLine( marg + 6*i, y1, marg + 6*i, y2);
      p->setPen(c.dark());
      p->drawLine( marg + 6*i +3, y1, marg + 6*i +3, y2);
  }

  // draw start/stop horizontal line
  int x, y = yy-2, w, h = 4;
  if (start >= 0) {
#if 0
      if (_jump[start] == ((TraceItemView*)_view)->selectedItem()) {
	  c = lv->colorGroup().highlightedText();
      }
#endif
      c = _jump[start]->isCondJump() ? Qt::red : Qt::blue;
      x = marg + 6*start;
      w = 6*(iv->arrowLevels() - start) + 10;
      p->fillRect( x, y, w, h, c);
      p->setPen(c.light());
      p->drawLine(x, y, x+w-1, y);
      p->drawLine(x, y, x, y+h-1);
      p->setPen(c.dark());
      p->drawLine(x+w-1, y, x+w-1, y+h-1);
      p->drawLine(x+1, y+h-1, x+w-1, y+h-1);
  }
  if (end >= 0) {
      c = _jump[end]->isCondJump() ? Qt::red : Qt::blue;
      x = marg + 6*end;
      w = 6*(iv->arrowLevels() - end) + 10;

      QPolygon a;
      a.putPoints(0, 8, x,y+h,
		  x,y, x+w-8,y, x+w-8,y-2,
		  x+w,yy,
		  x+w-8,y+h+2, x+w-8,y+h,
		  x,y+h);
      p->setBrush(c);
      p->drawConvexPolygon(a);

      p->setPen(c.light());
      p->drawPolyline(a.constData(), 5);
      p->setPen(c.dark());
      p->drawPolyline(a.constData() + 4, 2);
      p->setPen(c.light());
      p->drawPolyline(a.constData() + 5, 2);
      p->setPen(c.dark());
      p->drawPolyline(a.constData() + 6, 2);
  }

  // draw inner vertical line for start/stop
  // this overwrites borders of horizontal line
  for(int i=0;i< (int)_jump.size();i++) {
      if (_jump[i] == 0) continue;

      c = _jump[i]->isCondJump() ? Qt::red : Qt::blue;

      if (_jump[i]->instrFrom()->addr() == _addr) {
	  bool drawUp = true;
	  if (_jump[i]->instrTo()->addr() == _addr)
	      if (start<0) drawUp=false;
	  if (_jump[i]->instrTo()->addr() > _addr) drawUp=false;
	  if (drawUp)
	      p->fillRect( marg + 6*i +1, 0, 2, yy, c);
	  else
	      p->fillRect( marg + 6*i +1, yy, 2, height()-yy, c);
      }
      else if (_jump[i]->instrTo()->addr() == _addr) {
	  if (end<0) end = i;
	  if (_jump[i]->instrFrom()->addr() < _addr)
	      p->fillRect( marg + 6*i +1, 0, 2, yy, c);
	  else
	      p->fillRect( marg + 6*i +1, yy, 2, height()-yy, c);
      }
  }

}

int InstrItem::width( const QFontMetrics& fm,
                      const Q3ListView* lv, int c ) const
{
  if (c != 3) return Q3ListViewItem::width(fm, lv, c);

  InstrView* iv = (InstrView*) lv;
  int levels = iv->arrowLevels();

  if (levels == 0) return 0;

  // 10 pixels for the arrow
  return 10 + 6*levels + lv->itemMargin() * 2;
}

