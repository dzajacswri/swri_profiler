/*
 * time_plot.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: mint
 */


#include <swri_profiler_tools/time_plot.h>
#include <QtGui/QPushButton>
#include <QtGui/QScrollBar>

namespace swri_profiler_tools
{
TimePlot::TimePlot(QWidget *parent)
  :
		QGridLayout(parent),
  db_(NULL)
{
//  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//  view_animator_ = new VariantAnimation(this);
//  view_animator_->setEasingCurve(QEasingCurve::InOutCubic);
//  QObject::connect(view_animator_, SIGNAL(valueChanged(const QVariant &)),
//                   this, SLOT(update()));
//  scroll_bar_ptr_ = sb;


	QScrollBar *timelineScroll = new QScrollBar(parent);
	timelineScroll->setObjectName(QString::fromUtf8("timelineScroll"));
	timelineScroll->setOrientation(Qt::Horizontal);
    addWidget(timelineScroll, 1, 0, 1, 2);

	legend_ = new TimePlotLegend(parent);
	addWidget(legend_, 0, 0, 1, 1);

    stop_ = new QPushButton(parent);
    stop_->setMaximumSize(QSize(30, 16777215));
    stop_->setText(">");
    addWidget(stop_, 2, 0, 1, 2);

	timePlot_ = new TimePlotWidget(parent, timelineScroll, legend_);
    timePlot_->setObjectName(QString::fromUtf8("timePlot"));
	addWidget(timePlot_, 0, 1, 1, 1);

	QObject::connect(this, SIGNAL(setActiveNode(int,int)),
					timePlot_, SLOT(setActiveNode(int,int)));

	QObject::connect(stop_, SIGNAL(clicked(bool)),
					timePlot_, SLOT(pause()));

	QObject::connect(timelineScroll, SIGNAL(valueChanged(int)),
					timePlot_, SLOT(timeLineSliderMoved(int)));

}

TimePlot::~TimePlot()
{


}

void TimePlot::setDatabase(ProfileDatabase *db)
{
	  if (db_) {
	    // note(exjohnson): we can implement this later if desired, but
	    // currently no use case for it.
	    qWarning("ProfileTreeWidget: Cannot change the profile database.");
	    return;
	  }

	  db_ = db;
	  timePlot_->setDatabase(db_);

	  //synchronizeWidget();

//	  QObject::connect(db_, SIGNAL(dataAdded(int)),    timePlot_, SLOT(updateData()));
//	  QObject::connect(db_, SIGNAL(profileAdded(int)), timePlot_, SLOT(updateData()));
//	  QObject::connect(db_, SIGNAL(nodesAdded(int)),   timePlot_, SLOT(updateData()));
}

};
