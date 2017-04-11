#include <QMetaType>
#include <swri_profiler_msgs/ProfileIndex.h>
#include <swri_profiler_msgs/ProfileData.h>

namespace swri_profiler_tools
{
void registerMetaTypes()
{
  // We need to register our ROS message types with Qt to be able to
  // pass them in Qt queued signals/slots (across threads).
  qRegisterMetaType<swri_profiler_msgs::ProfileIndex>("swri_profiler_msgs::ProfileIndex");
  qRegisterMetaType<swri_profiler_msgs::ProfileData>("swri_profiler_msgs::ProfileData");
}
}  // namespace swri_profiler_tools

