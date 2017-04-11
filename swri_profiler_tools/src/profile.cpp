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
#include <swri_profiler_tools/profile.h>
#include <swri_profiler_tools/util.h>
#include <algorithm>
#include <set>
#include <QStringList>
#include <QDebug>
#include <inttypes.h>

namespace swri_profiler_tools
{
	// A null object to return for invalid keys.
	static const ProfileNode invalid_node_;

	Profile::Profile()
	:
			  profile_key_(-1),nextInt_(0), nextKey_(0)
	{
		root_node_ = new ProfileNode(nextKey());
		root_node_->name_ = "<root>";
		root_node_->flat_key_ = nextKey();
		flat_index_[root_node_->flat_key_] = root_node_;

	}

	Profile::~Profile()
	{
	}

	void Profile::initialize(int profile_key, const QString &name)
	{
		if (isValid()) {
			qWarning("Re-initializing a valid profile (%d,%s) with (%d,%s). "
					"Something is probably horribly wrong.",
					profile_key_, qPrintable(name_),
					profile_key, qPrintable(name));
		}
		profile_key_ = profile_key;
		name_ = name;
	}

	void Profile::addData(const NewProfileDataVector &data)
	{

		if (profile_key_ < 0)
		{
			qWarning("Attempt to add %zu elements to an invalid profile.", data.size());
			return;
		}

		if (data.size() == 0)
		{
			return;
		}

		std::set<uint64_t> modified_times;

		bool nodes_added = false;

		for (NewProfileData const &item : data)
		{

			if(root_node_->children_.count(item.thread_id) == 0)
			{
				nodes_added = true;
				ProfileNode *newThread = new ProfileNode(nextKey());
				flat_index_[newThread->flatKey()] = newThread;
				newThread->measured_ = false;
				newThread->parent_ = root_node_;
				newThread->node_key_ = item.thread_id;
				newThread->name_ =  QString("Thread #%1").arg(item.thread_id);
				newThread->path_ = newThread->name_;
				root_node_->children_[item.thread_id] = newThread;
			}
			ProfileNode *thread = root_node_->children_[item.thread_id];

			// ros nodes are indexed by string, so they are stored in an int, string
			// xref map.  We can get their pointer from finding it by thread and rosnode.
			ThreadRosNode_t rosKey = ThreadRosNode_t(item.thread_id, item.ros_node);
			if(rosNodes_.count(rosKey) == 0)
			{
				nodes_added = true;
				ProfileNode *newRosNode = new ProfileNode(nextKey());
				flat_index_[newRosNode->flatKey()] = newRosNode;
				newRosNode->name_ = item.ros_node;
				newRosNode->measured_ = false;
				newRosNode->parent_ = root_node_->children_[item.thread_id];
				newRosNode->path_ = item.ros_node;
				// add to thread
				newRosNode->currentNode_ = newRosNode;
				thread->children_[nextInt()] = newRosNode;
				// add to xref as well
				rosNodes_[rosKey] = newRosNode;
			}
			ProfileNode *rosNode = rosNodes_[rosKey];

			ProfileNode *current = rosNode->currentNode_;

			if (item.open)
			{
				ProfileNode *node;
				if (rosNode->currentNode_->children_.count(item.blockID) == 0)
				{
					nodes_added = true;
					node = new ProfileNode(nextKey());
					flat_index_[node->flatKey()] = node;
					node->measured_ = true;
					node->name_ = item.name.split('/').takeLast();
					node->parent_ = rosNode->currentNode_;
					node->path_ = rosNode->currentNode_->path_ + "/" + node->name();
					rosNode->currentNode_->children_[item.blockID] = node;
				}
				else
				{
					node = rosNode->currentNode_->children_[item.blockID];
				}

				rosNode->currentNode_ = node;
				node->open_ = true;
				storeItemData(node, item);

			}
			else if(rosNode->currentNode_ == rosNode)
			{}
			else
			{
				current->open_ = false;
				storeItemData(current, item);
				rosNode->currentNode_ = current->parent_;

			}

		}

		// If nodes were created, we need to update our indices.
		if (nodes_added)
		{
			rebuildIndices();
			Q_EMIT nodesAdded(profile_key_);
		}

		// Finally, we need to update derived data that may have changed
		// from the update.
		updateDerivedData();

		// Notify observers that the profile has new data.
		Q_EMIT dataAdded(profile_key_);

		printTree(root_node_, 0);
	}

	void Profile::storeItemData(ProfileNode *node, NewProfileData const &item)
	{
		ProfileEntry newEntry;
		node->measured_ = true;

		newEntry.projected = false;
		newEntry.ros_timestamp = item.ros_stamp_ns;
		newEntry.open = item.open;

		if((node->data_.size() != 0) && !newEntry.open && node->data_.back().open)
		{
			// generate span
			//uint64_t span = newEntry.ros_timestamp - node->data_.back().ros_timestamp;
			uint64_t start = node->data_.back().ros_timestamp;
			node->spans_.emplace_back();
			node->spans_.back().duration_ = newEntry.ros_timestamp - node->data_.back().ros_timestamp;
			node->spans_.back().start_ = start;
			node->spans_.back().end_ = item.ros_stamp_ns;


			// update node timing
			node->cumulative_call_count ++;
			node->cumulative_inclusive_duration_ns += node->spans_.back().duration_;
			node->incremental_inclusive_duration_ns = node->spans_.back().duration_;
			node->incremental_max_duration_ns = std::max(node->incremental_max_duration_ns, node->spans_.back().duration_);
			node->min_duration_ns = std::min(node->min_duration_ns, node->spans_.back().duration_);
		}

		node->data_.push_back(newEntry);
	}

	void Profile::expandTimeline(const uint64_t sec)
	{
//		if (sec >= min_time_s_ && sec < max_time_s_) {
//			// This time is already in our timeline, so ignore it.
//		} else if (min_time_s_ == max_time_s_) {
//			// The timeline is empty
//			min_time_s_ = sec;
//			max_time_s_ = sec+1;
//			addDataToAllNodes(true, 1);
//		} else if (sec >= max_time_s_) {
//			// New data extends the back of the timeline.
//			size_t new_elements = sec - max_time_s_ + 1;
//			max_time_s_ = sec+1;
//			addDataToAllNodes(true, new_elements);
//		} else {
//			// New data must be at the front of the timeline.  This case
//			// should be rare.
//			size_t new_elements = min_time_s_ - sec;
//			min_time_s_ = sec;
//			addDataToAllNodes(false, new_elements);
//		}
	}

	void Profile::addDataToAllNodes(const bool back, const size_t count)
	{
/*
		if (back) {
			for (auto &it : nodes_) {
				std::deque<ProfileEntry> &data = it.second.data_;
				ProfileEntry initial_value;
				if (!data.empty()) {
					initial_value = data.back();
				}
				initial_value.projected = true;
				data.insert(data.end(), count, initial_value);
			}
		} else {
			ProfileEntry initial_value;
			initial_value.projected = true;
			for (auto &it : nodes_) {
				std::deque<ProfileEntry> &data = it.second.data_;
				data.insert(data.begin(), count, initial_value);
			}
		}
*/
	}

	bool Profile::touchNode(const QString &path)
	{
/*
		// If the node already exists, it's ancestors must already exist
		// too.  We can just lookup the key and return false because no
		// nodes are being added.
		if (node_key_from_path_.count(path)) {
			return false;
		}

		QStringList all_parts = path.split('/');
		if (all_parts.isEmpty()) {
			qWarning("Node path (%s) does not have a root component.  This should never happen.",
					qPrintable(path));
			// This should seriously never happen because even the empty
			// string will come back as a single item list.
			return false;
		}

		if (all_parts.front() != "") {
			qWarning("Split of node path '%s' did not yield root as first element. This should never happend.",
					qPrintable(path));
		} else {
			all_parts.removeFirst();
		}

		QString this_path = "";
		int this_depth = 0;
		int parent_key = 0;

		while (!all_parts.isEmpty()) {
			QString this_name = all_parts.takeFirst();
			this_path = this_path + "/" + this_name;
			this_depth += 1;

			if (node_key_from_path_.count(this_path)) {
				parent_key = node_key_from_path_.at(this_path);
				continue;
			}

			// Otherwise we need to create a new node.
			int this_key = nodes_.size();
			while (nodes_.count(this_key)) { this_key++; }

			node_key_from_path_[this_path] = this_key;
			ProfileNode &this_node = nodes_[this_key];

			this_node.node_key_ = this_key;
			this_node.name_ = this_name;
			this_node.path_ = this_path;
			this_node.measured_ = false;

			ProfileEntry initial_value;
			initial_value.projected = true;
			//this_node.data_.resize(max_time_s_ - min_time_s_, initial_value);

			this_node.depth_ = this_depth;
			this_node.parent_ = parent_key;
			parent_key = this_key;

			// children_ will be set by rebuildChildren.
			//
			// Note: Why not just modify the index as we add nodes?  Mainly
			// for simplicity.  Doing everything at once makes it a little
			// easier to guarantee proper ordering and consistency without
			// having to do a lot of searching/sorting.  Adding nodes should
			// be a very infrequent operation, so the added cost of redoing
			// the work isn't an issue.
		}

*/
		return true;
	}

	void Profile::storeItemData(std::set<uint64_t> &modified_times,
			const int node_key,
			const NewProfileData &item)
	{
/*

		ProfileNode &node = nodes_.at(node_key);

		ProfileEntry newEntry;
		node.measured_ = true;

		newEntry.projected = false;
		newEntry.ros_timestamp = item.ros_stamp_ns;
		newEntry.open = item.open;

		if((node.data_.size() != 0) && !newEntry.open && node.data_.back().open)
		{
			// generate span
			uint64_t span = newEntry.ros_timestamp - node.data_.back().ros_timestamp;

			// update node timing
			node.cumulative_call_count ++;
			node.cumulative_inclusive_duration_ns += span;
			node.incremental_max_duration_ns = std::max(node.incremental_max_duration_ns, span);
		}

		node.data_.push_back(newEntry);



		// Exclusive timing fields are derived data and are set in updateDerivedData().

		// If the subsequent elements are projected data, we should
		// propogate this new data forward until the next firm data point.
	//	for (size_t i = index+1; i < node.data_.size(); i++)
	//	{
	//		if (node.data_[i].projected == false)
	//		{
	//			break;
	//		}
	//
	//		node.data_[i] = node.data_[index];
	//		node.data_[i].projected = true;
	//		modified_times.insert(secFromIndex(i));
	//		qWarning("inside propogate: %u")
	//	}
*/
	}

	void Profile::rebuildIndices()
	{
		rebuildFlatIndex();
		rebuildTreeIndex();
	}

	void Profile::rebuildFlatIndex()
	{
/*
		QStringList paths;
		for (auto const &it : nodes_) {
			paths.append(it.second.path());
		}
		paths.sort();

		flat_index_.clear();
		flat_index_.reserve(paths.size());
		for (int i = 0; i < paths.size(); i++) {
			if (node_key_from_path_.count(paths[i]) == 0) {
				qWarning("Path is missing from node_key_from_path_.  This should never happen.");
				continue;
			}
			flat_index_.push_back(node_key_from_path_.at(paths[i]));
		}
*/
	}

	// Compares the first N items of two string lists.
	static bool compareInitialStringList(
			const QStringList &list1,
			const QStringList &list2)
	{
		int size = std::min(list1.size(), list2.size());

		if (size == 0) {
			return true;
		}

		// Comparing in reverse because, in our use case, the differences
		// are more likely to be at the end of the lists.
		for (int i = size; i > 0; i--) {
			if (list1[i-1] != list2[i-1]) {
				return false;
			}
		}
		return true;
	}

	void Profile::rebuildTreeIndex()
	{
/*
		// Start by clearing out all children
		for (auto &it : nodes_) {
			it.second.children_.clear();
		}

		for (int key : flat_index_) {
			if (nodes_.count(key) == 0) {
				qWarning("Key (%d) in flat index was not found in nodes_ map. This should never happen.", key);
				continue;
			}

			const ProfileNode &node = nodes_.at(key);
			if (node.parent_ < 0) {
				if (key != 0) {
					qWarning("Profile node %d does not have a valid parent. This should never happen.", key);
				}
				continue;
			}

			if (nodes_.count(node.parentKey()) == 0) {
				qWarning("Profile node %d's parent (%d) was not found in nodes_. This should never happen.",
						key, node.parentKey());
				continue;
			}

			ProfileNode &parent = nodes_.at(node.parentKey());
			parent.children_.push_back(key);
		}
*/
	}

	void Profile::updateDerivedData()
	{

		if (root_node_ == NULL) {
			qWarning("Profile is missing root node.");
			return;
		}
		updateDerivedDataInternal(root_node_);

	}

	void Profile::updateDerivedDataInternal(ProfileNode *node)
	{

		uint64_t children_cum_call_count = 0;
		uint64_t children_cum_incl_duration = 0;
		uint64_t children_inc_incl_duration = 0;
		uint64_t children_inc_max_duration = 0;
		uint64_t children_start = std::numeric_limits<uint64_t>::max();
		uint64_t children_end = 0;
		uint64_t children_min_duration = std::numeric_limits<uint64_t>::max();

		// Descend through the tree and compute children.
		for (auto &child_kvp : node->childKeys())
		{
			ProfileNode *child = child_kvp.second;

			updateDerivedDataInternal(child);

			children_cum_call_count += child->cumulative_call_count;
			children_cum_incl_duration += child->cumulative_inclusive_duration_ns;
			children_inc_incl_duration += child->incremental_inclusive_duration_ns;
			children_inc_max_duration = std::max(children_inc_max_duration, child->incremental_max_duration_ns);
			children_start = std::min(children_start, child->spans_.front().start_);
			children_end = std::max(children_end, child->spans_.back().end_);
			children_min_duration =  std::min(children_min_duration, child->incremental_inclusive_duration_ns);

		}

		node->cumulative_exclusive_duration_ns = node->cumulative_inclusive_duration_ns - children_cum_incl_duration;
		node->incremental_exclusive_duration_ns = node->incremental_inclusive_duration_ns - children_inc_incl_duration;

		if (!node->measured_)
		{
			node->cumulative_call_count = children_cum_call_count;
			node->cumulative_inclusive_duration_ns = children_cum_incl_duration;
			node->cumulative_exclusive_duration_ns = 0;
			node->incremental_inclusive_duration_ns = children_inc_incl_duration;
			node->incremental_exclusive_duration_ns = 0;
			node->incremental_max_duration_ns = children_inc_max_duration;
			node->min_duration_ns = children_min_duration;

			// we only need one, so empty first
			node->spans_.clear();
			node->spans_.emplace_back();
			node->spans_.back().start_ = children_start;
			node->spans_.back().end_ = children_end;
			node->spans_.back().duration_ = children_end - children_start;

		}


		// sanity check
	//	if (children_cum_incl_duration > data.cumulative_inclusive_duration_ns)
	//	{
	//		// This case has not been observed yet.
	//		qWarning("Node's (%s) cumulative inclusive duration is less than it's combined"
	//				" children (%zu < %zu). I have not seen this before, so it may or may"
	//				" not be a big issue.",
	//				qPrintable(node.name()),
	//				data.cumulative_inclusive_duration_ns,
	//				children_cum_incl_duration);
	//		data.cumulative_inclusive_duration_ns = children_cum_incl_duration;
	//		data.cumulative_exclusive_duration_ns = 0;
	//	}
	//	else
	//	{
	//		data.cumulative_exclusive_duration_ns = data.cumulative_inclusive_duration_ns - children_cum_incl_duration;
	//	}

	//	if (children_inc_incl_duration > data.incremental_inclusive_duration_ns)
	//	{
	//		// This case was happening very frequently for long running nodes
	//		// with fast running small parts.  The profiler was limiting the
	//		// duration to 1 second while the smaller parts add up to just
	//		// slightly more than a second.  The profiler was updated to help
	//		// compensate by using the real publishing interval instead of the
	//		// expected interval.
	//
	//		// If it still happens, we want to adjust the data to be
	//		// internally consistent.  First, we allow a threshold to permit
	//		// small errors without a warning.  We're using a tenth of a
	//		// millisecond here
	//		if (children_inc_incl_duration - data.incremental_inclusive_duration_ns > 100000)
	//		{
	//			qWarning("Node's (%s) incremental inclusive timing is less than it's combined "
	//					"children (%zu < %zu).  If this happens frequently, something is wrong.",
	//					qPrintable(node.name()),
	//					data.incremental_inclusive_duration_ns,
	//					children_inc_incl_duration);
	//		}
	//
	//		data.incremental_inclusive_duration_ns = children_inc_incl_duration;
	//		data.incremental_exclusive_duration_ns = 0;
	//	}
	//	else
	//	{
	//		data.incremental_exclusive_duration_ns = data.incremental_inclusive_duration_ns - children_inc_incl_duration;
	//	}
	}

	void Profile::setName(const QString &name)
	{
		name_ = name;
		Q_EMIT profileModified(profile_key_);
	}

	void Profile::printTree(ProfileNode *node, int depth)
	{

		if(node->isValid())
		{
			for (int i = 0; i < depth; i++)
				printf(" ");
			printf("N: %34s\tCC: start: %10" PRIu64 " end: %10" PRIu64 "\n",
					qPrintable(node->name()),
					node->spans_.back().start_,
					node->spans_.back().end_);

//			printf("N: %34s\tCC: %5i ID: %10" PRIu64 " ED: %10" PRIu64 " IID: %10" PRIu64 " IED: %10" PRIu64 " IMD: %10" PRIu64 " M: %s fkey=%i\n",
//					qPrintable(node->name()),
//					node->cumulative_call_count,
//					node->cumulative_inclusive_duration_ns,
//					node->cumulative_exclusive_duration_ns,
//					node->incremental_inclusive_duration_ns,
//					node->incremental_exclusive_duration_ns,
//					node->incremental_max_duration_ns,
//					(node->measured_ ? "Yes" : "No"),
//					node->flatKey());
		}

		// Descend through the tree and compute children.
	for (auto &child_kvp : node->childKeys())
		{
			printTree(child_kvp.second, depth+1);
		}
	}


}  // namespace swri_profiler_tools
