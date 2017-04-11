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

#ifndef SWRI_PROFILER_TOOLS_PROFILE_H_
#define SWRI_PROFILER_TOOLS_PROFILE_H_

#include <deque>
#include <set>
#include <map>
#include <unordered_map>

#include <QObject>
#include <QString>
#include <QStringList>
#include <swri_profiler_tools/new_profile_data.h>
//#include <swri_profiler_tools/NodeKey.h>

namespace swri_profiler_tools
{
	class ProfileDatabase;

	// todo: convert this to a std::pair if nothing else ends up being added
	struct Span
	{
		uint64_t start_;
		uint64_t end_;
		uint64_t duration_;
	};


	class ProfileEntry
	{

		public:
			// projected is for internal use in the profiler.  This flag
			// indicates that the item's current data was projected from a
			// previous data point.  This is used to keep the profile data
			// consistent despite missing or late data.  This flag only applies
			// to data in measured nodes.

			bool projected;
			bool open;
			uint64_t ros_timestamp;

			ProfileEntry()
			:
				projected(false),
				open(false),
				ros_timestamp(0)
			{}
	};  // class ProfileEntry


	class ProfileNode
	{
		// This is the node's unique db key.
		int flat_key_;

		int node_key_;

		std::deque<Span> spans_;

		// only used if this node is a rosNodeType
		ProfileNode* currentNode_;

		// The name of this node.  This is the last element of the path (c
		// in a/b/c).
		QString name_;

		// The full path of the node in the call tree.  (e.g. a/b/c)
		QString path_;

		// Nodes can be either measured or inferred.  Measured nodes have
		// their inclusive timing information provided by the profiler.
		// These are your typical blocks measured by
		// SWRI_PROFILE(). Inferred nodes are nodes that were created to
		// fill in the call tree.  These are typically your root node and
		// nodes corresponding to ROS namespaces.
		bool measured_;

		// The data stored by the node.  The array is managed by the
		// profile.  Each element corresponds to a time which is determined
		// by the Profile's min_time and max_time.
		std::deque<ProfileEntry> data_;

		// The node's depth in the tree.
		int depth_;

		// The key of this node's parent node.  This will be invalid
		// (negative) for the root node.
		ProfileNode* parent_;

		// They node's children, in alphabetical order according to their
		// paths.
		std::map<int, ProfileNode*> children_;

		bool open_;

		// The ProfileNode is a "dumb" data storage object with read-only
		// access to the rest of the world.  The node is managed and
		// manipulated directly by the profile.
		friend class Profile;

		public:
			ProfileNode()
		:
			flat_key_(-1),
			measured_(false),
			depth_(-1),
			cumulative_call_count(0),
			cumulative_inclusive_duration_ns(0),
			incremental_inclusive_duration_ns(0),
			cumulative_exclusive_duration_ns(0),
			incremental_exclusive_duration_ns(0),
			incremental_max_duration_ns(0)
		{}

			ProfileNode(int key)
		:
			flat_key_(key),
			measured_(false),
			depth_(-1),
			cumulative_call_count(0),
			cumulative_inclusive_duration_ns(0),
			incremental_inclusive_duration_ns(0),
			cumulative_exclusive_duration_ns(0),
			incremental_exclusive_duration_ns(0),
			incremental_max_duration_ns(0),
			min_duration_ns(std::numeric_limits<uint64_t>::max())
		{}



			bool isValid() const { return true; } //node_key_.isValid(); }// todo fix this.
			int flatKey() const { return flat_key_; }
			const QString& name() const { return name_; }
			const QString& path() const { return path_; }
			bool isMeasured() const { return measured_; }
			const std::deque<ProfileEntry>& data() const { return data_; }
			int depth() const { return depth_; }
			ProfileNode* parentKey() const { return parent_; }
			const std::map<int, ProfileNode*> childKeys() const { return children_; }
			bool hasChildren() const { return children_.size() > 0; }
			std::deque<Span> spans() const { return spans_; }
			QString nameForNode() const
			{
				return QString("%1. %2").arg(flat_key_).arg(name_);
			}

			uint64_t cumulative_call_count;
			uint64_t cumulative_inclusive_duration_ns;
			uint64_t incremental_inclusive_duration_ns;
			uint64_t cumulative_exclusive_duration_ns;
			uint64_t incremental_exclusive_duration_ns;
			uint64_t incremental_max_duration_ns;
			uint64_t min_duration_ns;

	};  // class ProfileNode

	class Profile : public QObject
	{
		Q_OBJECT;

		ProfileNode* root_node_;

		typedef std::pair<int, QString> ThreadRosNode_t;

		std::map<ThreadRosNode_t, ProfileNode*> rosNodes_;

		int nextInt_;

		int nextInt() { return nextInt_++; }

		int nextKey_;

		int nextKey() { return nextKey_++; }


		// The key of this profile in the database.  This is negative for
		// an invalid profile.
		int profile_key_;

		// Name of the profile.  This is initialized by the source and may
		// be modified by the user.
		QString name_;

		// All node data is stored in dense arrays of the same size.  The
		// min_time_s_ and max_time_s_ correspond to the timespan currently
		// covered by the array.  They are inclusive and exclusive,
		// respectively (index 0 => min_time_s, index size() => max_time_s.
//		uint64_t min_time_s_;
//		uint64_t max_time_s_;

		// Nodes are stored in an unordered_map so that we can provide
		// persistent keys with fast look ups.  We could use the node's path
		// as the key (though we need an adapter hash a QString rather than
		// std::string), but they we're constantly hashing very long
		// strings.  Instead, we assign a unique integer key when the node
		// is added.  This map provides reasonable reverse-lookups.  We
		// could actually change nodes_ to a vector or deque and still have
		// persistent indices as long as we don't allow nodes to be deleted.
		std::map<QString, int> node_key_from_path_;

		// The flat index stores all the profile's nodes in alphabetical by
		// path order.  Traversing in order corresponds to visiting the call
		// tree in a depth-first pattern.
		std::map<int, ProfileNode*> flat_index_;

		// The ProfileDatabase is the only place we want to create valid
		// profiles.  A valid profile is created by initializing a default
		// profile.  Initialization is only allowed to happen once.
		friend class ProfileDatabase;
		void initialize(int profile_key, const QString &name);

		void expandTimeline(const uint64_t sec);
		void addDataToAllNodes(const bool back, const size_t count);

		bool touchNode(const QString &path);


		void storeItemData(ProfileNode *node, NewProfileData const &item);

		void storeItemData(std::set<uint64_t> &modified_times,
				const int node_key,
				const NewProfileData &item);

		void updateItemData();

//		size_t indexFromSec(const uint64_t secs) const { return secs - min_time_s_; }
//		uint64_t secFromIndex(const uint64_t index) const { return index + min_time_s_; }

		void rebuildIndices();
		void rebuildFlatIndex();
		void rebuildTreeIndex();

//		void printTree(RosNodeEntry &rosNode, ProfileNode& node, int depth);
//		void printTree(ThreadEntry &thread, int depth);
		void printTree(ProfileNode *node, int depth);

		void updateDerivedData();
		void updateDerivedDataInternal(ProfileNode *node);


		public:
			Profile();
			~Profile();

			void addData(const NewProfileDataVector &data);
			const bool isValid() const { return profile_key_ >= 0; }
			const int profileKey() const { return profile_key_; }

			const QString& name() const { return name_; }
			void setName(const QString &name);

//			const ProfileNode* rootKey() const { return root_node_; }
//			const ProfileNode &node(NodeKey nk) const { return nodes_.at(nk); }
			const ProfileNode *flatIndex(int nk) const { return flat_index_.at(nk); }

			const ProfileNode* rootNode() const { return root_node_; }

			Q_SIGNALS:
			// Emitted when the profile is renamed.
			void profileModified(int profile_key);
			void nodesAdded(int profile_key);
			void dataAdded(int profile_key);



	};  // class Profile

}  // namespace swri_profiler_tools


#endif  // SWRI_PROFILER_TOOLS_PROFILE_H_
