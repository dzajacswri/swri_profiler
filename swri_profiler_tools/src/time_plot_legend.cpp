/*
 * time_plot.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: mint
 */


#include <swri_profiler_tools/time_plot_legend.h>
#include <QPainter>

namespace swri_profiler_tools
{
TimePlotLegend::TimePlotLegend(QWidget *parent)
  :
		QWidget(parent)
{
  setMinimumWidth(25);
  setMaximumWidth(25);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

TimePlotLegend::~TimePlotLegend()
{


}

void TimePlotLegend::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	QRectF win_rect(QPointF(0, 0), QPointF(width(), height()));

	QFont font("noto sans", 9);
	double pixelsHigh =  9;
	painter.setFont(font);
	int i =0;
	double eachGraphHt = win_rect.height()/((double)items.size());
	for(auto item : items)
	{
		painter.setPen(item.second);
		painter.drawText(QPointF(0, ((double) i)*eachGraphHt+eachGraphHt/2.0+pixelsHigh/2.0), item.first);
		i++;
	}
	update();

}

void TimePlotLegend::addItem(QString name, QColor color)
{
	items.push_back(std::make_pair(name, color));
}

void TimePlotLegend::clearList()
{
	items.clear();
}




}
