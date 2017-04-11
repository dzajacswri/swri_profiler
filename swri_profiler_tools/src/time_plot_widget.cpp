// *****************************************************************************
//
// Copyright (c) 2015, Southwest Research Institute® (SwRI®)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Southwest Research Institute® (SwRI®) nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Southwest Research Institute® BE LIABLE 
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// *****************************************************************************
#include <swri_profiler_tools/time_plot_widget.h>

#include <QPainter>
#include <QMouseEvent>
#include <qlabel.h>

#include <swri_profiler_tools/profile_database.h>
#include <swri_profiler_tools/partition_widget.h>
#include <swri_profiler_tools/variant_animation.h>

namespace swri_profiler_tools
{
TimePlotWidget::TimePlotWidget(QWidget *parent)
  :
		QWidget(parent),
  db_(NULL)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  view_animator_ = new VariantAnimation(this);
  view_animator_->setEasingCurve(QEasingCurve::InOutCubic);
  QObject::connect(view_animator_, SIGNAL(valueChanged(const QVariant &)),
                   this, SLOT(update()));




}

TimePlotWidget::~TimePlotWidget()
{
}

QSize TimePlotWidget::sizeHint() const
{
  return QSize(200, 200);
}

void TimePlotWidget::setDatabase(ProfileDatabase *db)
{
  if (db_) {
    // note(exjohnson): we can implement this later if desired, but
    // currently no use case for it.
    qWarning("ProfileTreeWidget: Cannot change the profile database.");
    return;
  }

  db_ = db;

  synchronizeWidget();

  QObject::connect(db_, SIGNAL(dataAdded(int)),    this, SLOT(updateData()));
  QObject::connect(db_, SIGNAL(profileAdded(int)), this, SLOT(updateData()));
  QObject::connect(db_, SIGNAL(nodesAdded(int)),   this, SLOT(updateData()));

}

void TimePlotWidget::synchronizeWidget()
{

}
void TimePlotWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  QRectF data_rect = view_animator_->currentValue().toRectF();
  QRectF win_rect(QPointF(30, 0), QPointF(width()-1, height()-1));
  win_rect = win_rect.adjusted(1,1,-1,-1);
  win_from_data_ = getTransform(win_rect, data_rect);
  painter.setBrush(Qt::white);
  painter.drawRect(win_rect.adjusted(-1,1,-1,-1));

  const Profile &profile = db_->profile(active_key_.profileKey());
  renderLayout(painter, win_rect, win_from_data_, current_layout_, profile);

}

QTransform TimePlotWidget::getTransform(const QRectF &win_rect,
                                         const QRectF &data_rect)
{
  double sx = win_rect.width() / data_rect.width();
  double sy = win_rect.height() / data_rect.height();
  double tx = win_rect.topLeft().x() - sx*data_rect.topLeft().x();
  double ty = win_rect.topLeft().y() - sy*data_rect.topLeft().y();


  QTransform win_from_data(sx, 0.0, 0.0,
                           0.0, sy, 0.0,
                           tx, ty, 1.0);
  return win_from_data;
}
void TimePlotWidget::renderYAxis(QPainter &painter,
                                   const QRectF &win_rect,
                                   int numNodes)
{
	QFont font("noto sans", 9);
	QFontMetrics fm(font);
	int pixelsHigh = fm.height();
	painter.setFont(font);
	int maxWidth = 0;
	int i =0;
	for(auto item : current_layout_)
	{
		painter.setPen(colorFromString(item.node_->name()));
		QString text = QString("[%1]").arg(item.node_->flatKey());
		maxWidth = std::max(fm.width(text), maxWidth);
		painter.drawText(QPointF(0, pixelsHigh+( ((double) i)*( win_rect.height()/numNodes ) ) ), text);
		i++;
	}
//	painter.setPen(QColor(Qt::black));
//	painter.drawLine(QLineF(QPointF(maxWidth+PAD, win_rect.top()), QPointF((double)maxWidth+PAD,win_rect.bottom())));
}

void TimePlotWidget::renderXAxis(QPainter &painter,
                                   const QRectF &win_rect,
                                   int numNodes)
{
	painter.setBrush(QColor(Qt::black));
	//painter.drawLine(win_from_data.map(QLineF(QPointF(0.05,0.95),QPointF(1.0,0.95))));
}




void TimePlotWidget::renderLayout(QPainter &painter,
		                           const QRectF &win_rect,
                                   const QTransform &win_from_data,
                                   const Layout &layout,
                                   const Profile &profile)
{
  // Set painter to use a single-pixel black pen.
  painter.setPen(Qt::black);

  renderYAxis(painter,  win_rect, layout.size());
  renderXAxis(painter,  win_rect, layout.size());


  for (auto const &item : layout)
  {

  	QColor color = colorFromString(item.node_->name());
  	painter.setPen(color);

    for (QLineF span : item.spans_)
    {

		QLineF win_line = win_from_data.map(span);
		painter.drawLine(win_line);

    }
  }
}

// todo merge genric plot widget stuff into a parent class
// shared by time_plot_widget and partition_widget
void TimePlotWidget::updateData()
{
	  if (!active_key_.isValid()) {
		  return;
	  }

	  const Profile &profile = db_->profile(active_key_.profileKey());
	  Layout layout = layoutProfile(profile);
	  current_layout_ = layout;
	  AdjustTimeline();
	  update();
}


QRectF TimePlotWidget::dataRect(const Layout &layout) const
{
//  if (layout.empty()) {
//    return QRectF(0.0, 0.0, 1.0, 1.0);
//  }
//
//  double right = layout.back().rect.right();
//
//  for (auto const &item : layout) {
//    if (active_key_.flatKey() == item.node_flat_key && item.exclusive == false) {
//      QRectF rect = item.rect;
//
//      rect.setLeft(std::max(0.0, rect.left()-0.2));
//      rect.setRight(right);
//
//      double margin = 0.05 * rect.height();
//      rect.setTop(std::max(0.0, rect.top() - margin));
//      rect.setBottom(std::min(1.0, rect.bottom() + margin));
//      return rect;
//    }
//  }
//
//  qWarning("Active node key was not found in layout");
//  return QRectF(QPointF(0.0, 0.0), QPointF(right, 1.0));
}


TimePlotWidget::Layout TimePlotWidget::layoutProfile(const Profile &profile)
{
	  const ProfileNode *root_node = profile.rootNode();
	  if (!root_node->isValid()) {
	    qWarning("Profile returned invalid root node.");
	    return Layout();
	  }

	  nodes_ = 0;
	  Layout layout;
	  return layoutProfile(profile.flatIndex(active_key_.flatKey()), layout);

}

TimePlotWidget::Layout TimePlotWidget::layoutProfile(const ProfileNode *node, Layout &layout)
{

	LayoutItem item;

	// for right now, include all the spans
	// todo: evalute if we want all the spans painted.
	item.node_ = node;

	//item.node_flat_key = node->flatKey();
	item.index_ = nodes_++;

	uint64_t maxDuration = 0;

	for(Span span : node->spans())
	{
		float height = (1.0-((float)span.duration_)/((float) (node->incremental_max_duration_ns-node->min_duration_ns))) + (item.index_);
		printf("duration_:%lu max_duration:%lu relative:%f index:%i height:%f\n",
				span.duration_,
				node->incremental_max_duration_ns,
				(((float)span.duration_)/((float)node->incremental_max_duration_ns)),
				item.index_,
				height);

    	QLineF horz = QLineF(QPointF(span.start_, height), QPointF(span.end_, height));
    	QLineF start = QLineF(QPointF(span.start_, item.index_), QPointF(span.start_, item.index_+1));
    	QLineF end = QLineF(QPointF(span.end_, item.index_), QPointF(span.end_, item.index_+1));

    	item.spans_.push_back(horz);
		item.spans_.push_back(start);
		item.spans_.push_back(end);
		maxDuration = std::max(maxDuration, span.duration_);
	}




	layout.push_back(item);

	for (auto child_kvp : node->childKeys())
	{
	  layout = layoutProfile(child_kvp.second, layout);
	}
	return layout;
}

void TimePlotWidget::addGraph(int profile_key, int node_flat_key)
{

	const Profile &profile = db_->profile(active_key_.profileKey());


}

void TimePlotWidget::setActiveNode(int profile_key, int node_flat_key)
{
	  const DatabaseKey new_key(profile_key, node_flat_key);

	  if (new_key == active_key_) {
	    return;
	  }

	  bool first = true;
	  if (active_key_.isValid()) {
	    first = false;
	  }

	  active_key_ = new_key;

	  const Profile &profile = db_->profile(active_key_.profileKey());
	  Layout layout = layoutProfile(profile);

	  QRectF data_rect;
	  if(profile.flatIndex(node_flat_key)->spans().size() > 0)
	  {
		  data_rect = QRectF(profile.flatIndex(node_flat_key)->spans().back().start_,
				  0.0, profile.flatIndex(node_flat_key)->spans().back().duration_, layout.size());
	  }
	  else
	  {
		  qWarning("No Spans for active node");
		  data_rect = QRectF(0.0, 0.0, 1.0, 1.0);
	  }

	  current_layout_ = layout;

	  if (!first) {
	    view_animator_->stop();
	    view_animator_->setStartValue(view_animator_->endValue());
	    view_animator_->setEndValue(data_rect);
	    view_animator_->setDuration(500);
	    view_animator_->start();
	  } else {
	    view_animator_->setStartValue(data_rect);
	    view_animator_->setEndValue(data_rect);
	  }
	  emit activeNodeChanged(profile_key, node_flat_key);
}

void TimePlotWidget::AdjustTimeline()
{

	  bool first = true;
	  if (active_key_.isValid()) {
	    first = false;
	  }

	  const Profile &profile = db_->profile(active_key_.profileKey());
	  Layout layout = layoutProfile(profile);

	  QRectF data_rect;
	  int key = active_key_.flatKey();
	  if(profile.flatIndex(key)->spans().size() > 0)
	  {
		  data_rect = QRectF(profile.flatIndex(key)->spans().back().start_,
				  0.0, profile.flatIndex(key)->spans().back().duration_, layout.size());
	  }
	  else
	  {
		  qWarning("No Spans for active node");
		  data_rect = QRectF(0.0, 0.0, 1.0, 1.0);
	  }

	  current_layout_ = layout;

	  if (!first) {
	    view_animator_->stop();
	    view_animator_->setStartValue(view_animator_->endValue());
	    view_animator_->setEndValue(data_rect);
	    view_animator_->setDuration(500);
	    view_animator_->start();
	  } else {
	    view_animator_->setStartValue(data_rect);
	    view_animator_->setEndValue(data_rect);
	  }
}

void TimePlotWidget::enterEvent(QEvent *event)
{
}

void TimePlotWidget::leaveEvent(QEvent *event)
{
}

void TimePlotWidget::mouseMoveEvent(QMouseEvent *event)
{
}

void TimePlotWidget::mousePressEvent(QMouseEvent *event)
{
}

void TimePlotWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
}

void TimePlotWidget::setSpan(int val)
{

}

//void TimePlotWidget::paintEvent(QPaintEvent *)
//{
//  QPainter painter(this);
//
//  painter.setPen(Qt::NoPen);
//  painter.fillRect(0, 0, width(), height(), QColor(100, 100, 100));
//  painter.setPen(Qt::black);
//
//}
}// namespace swri_profiler_tools


