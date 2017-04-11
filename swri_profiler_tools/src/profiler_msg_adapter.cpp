#include <swri_profiler_tools/profiler_msg_adapter.h>
#include <swri_profiler_tools/util.h>
#include <QDebug>

namespace swri_profiler_tools
{
	ProfilerMsgAdapter::ProfilerMsgAdapter()
	{  	}

	ProfilerMsgAdapter::~ProfilerMsgAdapter()
	{	}

	void ProfilerMsgAdapter::processIndex(const swri_profiler_msgs::ProfileIndex &msg)
	{

		const QString ros_node_name =
				normalizeNodePath(QString::fromStdString(msg.header.frame_id));

		// An index message contains the entire index table for the message,
		// so we wipe out any existing index to make sure we are completely
		// in sync.
		index_[ros_node_name].clear();

		for (auto const &item : msg.index)
		{
			QString label = normalizeNodePath(QString::fromStdString(item.label));

			// This is a special case to handle nodelets nicely, and it works
			// when users design their labels intelligently by wrapping each
			// ROS callback with a SWRI_PROFILE(getName()).  If the
			// nodelet is run as part of a nodelet manager, the node name and
			// nodelet name will differ and we want to append the node name to
			// allow us to guage the relative runtimes of all the instrumented
			// nodelets in that manager.  If the nodelet is run standalone,
			// then the nodelet name and node name will be the same and we
			// don't need to duplicate it.
			if (!label.startsWith(ros_node_name))
			{
				label = ros_node_name + label;
			}

			index_[ros_node_name][item.id] = label;
		}
	}

	bool ProfilerMsgAdapter::processData(
	  NewProfileDataVector &out_data,
	  const swri_profiler_msgs::ProfileData &msg)
	{
		//qWarning("Inside Process Data");
		const QString node_name(QString::fromStdString(msg.header.frame_id));

		if (index_.count(node_name) == 0)
		{
			//qWarning("No index for node '%s'. Dropping data update.", qPrintable(node_name));
			return false;
		}

		int timestamp_sec = std::round(msg.header.stamp.toSec());

		for (auto const &thread : msg.threads)
		{
			NewProfileDataVector out;
			out.reserve(thread.events.size());
			for (auto const &event: thread.events)
			{
				uint32_t blockID = event.event_id & ~0x01;
				bool openEvent = !(event.event_id & 0x01);

				//qWarning("Index ID: %s", qPrintable(node_name));
				if (index_[node_name].count(blockID) == 0)
				{
					qWarning("No index for block %d of %s. Dropping all data "
						"because index is probably invalid.",
						blockID, qPrintable(node_name));
					return false;
				}

				out.emplace_back();
				out.back().ros_node = node_name;
				out.back().name = index_[node_name][blockID];
				out.back().blockID = blockID;
				out.back().wall_stamp_sec = timestamp_sec;
				out.back().ros_stamp_ns = event.stamp.toNSec();
				out.back().open = openEvent;
				out.back().thread_id = thread.id;

			}

			out_data.insert(out_data.end(), out.begin(), out.end());
		}
		return true;
	}

	void ProfilerMsgAdapter::reset()
	{
		index_.clear();
	}
};  // namespace swri_profiler_tools
