/// *****************************************************************************
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
#ifndef SWRI_PROFILER_TOOLS_TIME_PLOT_WIDGET_H_
#define SWRI_PROFILER_TOOLS_TIME_PLOT_WIDGET_H_

#include <QWidget>
#include <swri_profiler_tools/database_key.h>

QT_BEGIN_NAMESPACE
class QHelpEvent;
QT_END_NAMESPACE

namespace swri_profiler_tools
{
class Profile;
class ProfileDatabase;
class VariantAnimation;
class ProfileDatabase;
class TimePlotWidget  : public QWidget
{
  Q_OBJECT;

  static constexpr double PAD = 1.0;
  ProfileDatabase *db_;

  struct LayoutItem
  {
    int index_;
    bool exclusive;
    const ProfileNode *node_;
    std::vector<QLineF> spans_;
    int depth;
  };
  typedef std::vector<LayoutItem> Layout;

  DatabaseKey active_key_;

  VariantAnimation *view_animator_;
  QTransform win_from_data_;

  // number of nodes in the tree view
  int nodes_;


  Layout current_layout_;
  Layout layoutProfile(const ProfileNode *node, Layout &layout);
  Layout layoutProfile(const Profile &profile);
  QRectF dataRect(const Layout &layout) const;
  void addGraph(int profile_key, int node_flat_key);
  QTransform getTransform(const QRectF &win_rect, const QRectF &data_rect);
  void renderLayout(QPainter &painter, const QRectF &win_rect,
                                     const QTransform &win_from_data,
                                     const Layout &layout,
                                     const Profile &profile);
  void renderYAxis(QPainter &painter,
                                     const QRectF &win_rect,
                                     int numNodes);
  void renderXAxis(QPainter &painter,
		  const QRectF &win_rect,
                                     int numNodes);

  void AdjustTimeline();

 public:
  TimePlotWidget(QWidget *parent=0);
  ~TimePlotWidget();

  QSize sizeHint() const;
  
  void setDatabase(ProfileDatabase *db);

 public Q_SLOTS:
  void setActiveNode(int profile_key, int node_key);
  void updateData();
  void setSpan(int val);

 Q_SIGNALS:
  void activeNodeChanged(int profile_key, int node_flat_key);
  void spanChanged(int);

 protected:
  void paintEvent(QPaintEvent *);
  void addPlot(int profile_key, int node_flat_key);

  void synchronizeWidget();

  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
};  // class TimePlotWidget
}  // namespace swri_profiler_tools
#endif // SWRI_PROFILER_TOOLS_TIME_PLOT_WIDGET_H_
