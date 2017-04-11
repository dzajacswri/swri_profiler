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
#include <swri_profiler_tools/profile_tree_widget.h>

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QMenu>

#include <swri_profiler_tools/profile_database.h>
#include <swri_profiler_tools/qcustomplot.h>
#include <swri_profiler_tools/partition_widget.h>

namespace swri_profiler_tools
{
enum ProfileTreeTypes {
  ProfileNodeType = QTreeWidgetItem::UserType,
};

enum ProfileTreeRoles {
  ProfileKeyRole = Qt::UserRole,
  FlatKeyRole,
};

ProfileTreeWidget::ProfileTreeWidget(QWidget *parent)
  :
  QWidget(parent),
  db_(NULL)
{
  tree_widget_ = new QTreeWidget(this);
  tree_widget_->setFont(QFont("Ubuntu Mono", 9));
  tree_widget_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_widget_->setExpandsOnDoubleClick(false);
  
  QObject::connect(tree_widget_, SIGNAL(customContextMenuRequested(const QPoint&)),
                   this, SLOT(handleTreeContextMenuRequest(const QPoint&)));

  QObject::connect(tree_widget_, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
                   this, SLOT(handleItemActivated(QTreeWidgetItem*, int)));
  
  auto *main_layout = new QVBoxLayout();  
  main_layout->addWidget(tree_widget_);
  main_layout->setContentsMargins(0,0,0,0);
  setLayout(main_layout);
}

ProfileTreeWidget::~ProfileTreeWidget()
{
}

void ProfileTreeWidget::setDatabase(ProfileDatabase *db)
{
  if (db_) {
    // note(exjohnson): we can implement this later if desired, but
    // currently no use case for it.
    qWarning("ProfileTreeWidget: Cannot change the profile database.");
    return;
  }

  db_ = db;
  
  synchronizeWidget();

  QObject::connect(db_, SIGNAL(profileModified(int)),
                   this, SLOT(handleProfileAdded(int)));
  QObject::connect(db_, SIGNAL(profileAdded(int)),
                   this, SLOT(handleProfileAdded(int)));
  QObject::connect(db_, SIGNAL(nodesAdded(int)),
                   this, SLOT(handleNodesAdded(int)));
}

void ProfileTreeWidget::handleProfileAdded(int profile_key)
{
  // We can optimize these to be specific later if necessary.
  synchronizeWidget();
}

void ProfileTreeWidget::handleNodesAdded(int profile_key)
{
  // We can optimize these to be specific later if necessary.
  synchronizeWidget();
}

void ProfileTreeWidget::synchronizeWidget()
{
  tree_widget_->clear();
  items_.clear();

  if (!db_) {
    return;
  }

  std::vector<int> keys = db_->profileKeys();
  for (auto key : keys) {
    addProfile(key);
  }
}

void ProfileTreeWidget::addProfile(int profile_key)
{

  const Profile &profile = db_->profile(profile_key);
  if (!profile.isValid()) {
    qWarning("Invalid profile for key %d.", profile_key);
    return;
  }
  QTreeWidgetItem *item = new QTreeWidgetItem(ProfileNodeType);
  
  item->setText(0, profile.name());    
  item->setData(0, ProfileKeyRole, profile_key);
  item->setData(0, FlatKeyRole, profile.rootNode()->flatKey());
  tree_widget_->addTopLevelItem(item);
  DatabaseKey key = DatabaseKey(profile.profileKey(), profile.rootNode()->flatKey());
  items_[key] = item;

  for (auto child_kvp : profile.rootNode()->childKeys()) {
	  addNode(item, child_kvp.second, profile);
  }

  if (active_key_.isValid()) {
    if (items_.count(active_key_)) {
      markItemActive(active_key_);
    } else {
      active_key_ = DatabaseKey();
      emit activeNodeChanged(-1,-1);
    }
  }

}


void ProfileTreeWidget::addNode(QTreeWidgetItem *parent,
								const ProfileNode* node,
					            const Profile &profile)
{
	QTreeWidgetItem *item;
	if(node->isValid())
	{
	  item = new QTreeWidgetItem(ProfileNodeType);
	  item->setText(0, node->nameForNode());
	  item->setData(0, ProfileKeyRole, profile.profileKey());
	  item->setData(0, FlatKeyRole, node->flatKey());
	  parent->addChild(item);
	  DatabaseKey dbk = DatabaseKey(profile.profileKey(), node->flatKey());
	  items_[dbk] = item;  //todo fix this
	}
	else
	{
		item = parent;
	}

  for (auto child_kvp : node->childKeys()) {
    addNode(item, child_kvp.second, profile);
  }
}

void ProfileTreeWidget::handleTreeContextMenuRequest(const QPoint &pos)
{
  QTreeWidgetItem *item = tree_widget_->itemAt(pos);

  auto menu = new QMenu(this);
  auto expand_all_action = menu->addAction("Expand All");
  QObject::connect(expand_all_action, SIGNAL(triggered()),
                   tree_widget_, SLOT(expandAll()));
  
  auto collapse_all_action = menu->addAction("Collapse All");
  QObject::connect(collapse_all_action, SIGNAL(triggered()),
                   tree_widget_, SLOT(collapseAll()));

  menu->popup(tree_widget_->mapToGlobal(pos));  
  QObject::connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));

  if (!item) {
    qWarning("No item under mouse");
  } else if (item->type() == ProfileNodeType) {
    int profile_key = item->data(0, ProfileKeyRole).toInt();
    int node_key = item->data(0, FlatKeyRole).toInt();
    qWarning("node %d--%d", profile_key, node_key);    
  } else {
    qWarning("Unknown item type: %d", item->type());
  }    
}

void ProfileTreeWidget::handleItemActivated(QTreeWidgetItem *item, int column)
{

  if (item->type() == ProfileNodeType) {
    int flat_key = item->data(0, FlatKeyRole).toInt();
    int profile_key = item->data(0, ProfileKeyRole).toInt();
    setActiveNode(profile_key, flat_key);
  }  

}

QString ProfileTreeWidget::nameForKey(const DatabaseKey &key) const
{

  if (!key.isValid()) {
    return "<INVALID KEY>";
  }

  const Profile &profile = db_->profile(key.profileKey());
  if (key.flatKey() == profile.rootNode()->flatKey()) {
    return profile.name();
  } else {
    return profile.flatIndex(key.flatKey())->nameForNode();
  }
}

void ProfileTreeWidget::markItemActive(const DatabaseKey &key)
{
  if (items_.count(key) == 0) {
	  qWarning("Key Not Found");
    return;
  }

  items_.at(key)->setText(0, "[" + nameForKey(key) + "]");
}

void ProfileTreeWidget::markItemInactive(const DatabaseKey &key)
{ 
  if (items_.count(key) == 0) {
    return;
  }

  items_.at(key)->setText(0, nameForKey(key));
}

void ProfileTreeWidget::setActiveNode(int profile_key, int node_flat_key)
{
  const DatabaseKey new_key(profile_key, node_flat_key);
  if (new_key == active_key_) {
    return;
  }
  markItemInactive(active_key_);
  active_key_ = new_key;
  markItemActive(active_key_);
  emit activeNodeChanged(profile_key, node_flat_key);
}
}  // namespace swri_profiler_tools
