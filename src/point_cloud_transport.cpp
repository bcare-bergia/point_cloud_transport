// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Czech Technical University in Prague .. 2019, paplhjak .. 2009, Willow Garage, Inc.

/*
 *
 * BSD 3-Clause License
 *
 * Copyright (c) Czech Technical University in Prague
 * Copyright (c) 2019, paplhjak
 * Copyright (c) 2009, Willow Garage, Inc.
 *
 *        All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 *        modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 *       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *       AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *       IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *       DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *       FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *       DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *       CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *       OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *       OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string>
#include <vector>

#include <boost/algorithm/string/erase.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>

#include <pluginlib/class_loader.h>
#include <pluginlib/exceptions.hpp>
#include <ros/forwards.h>
#include <ros/node_handle.h>
#include <sensor_msgs/PointCloud2.h>

#include <point_cloud_transport/loader_fwds.h>
#include <point_cloud_transport/point_cloud_transport.h>
#include <point_cloud_transport/publisher_plugin.h>
#include <point_cloud_transport/single_subscriber_publisher.h>
#include <point_cloud_transport/subscriber_plugin.h>
#include <point_cloud_transport/transport_hints.h>

namespace point_cloud_transport
{

struct PointCloudTransport::Impl
{
  ros::NodeHandle nh_;
  point_cloud_transport::PubLoaderPtr pub_loader_;
  point_cloud_transport::SubLoaderPtr sub_loader_;

  explicit Impl(const ros::NodeHandle& nh) : nh_(nh),
      pub_loader_(boost::make_shared<PubLoader>("point_cloud_transport", "point_cloud_transport::PublisherPlugin")),
      sub_loader_(boost::make_shared<SubLoader>("point_cloud_transport", "point_cloud_transport::SubscriberPlugin"))
  {
  }
};

PointCloudTransport::PointCloudTransport(const ros::NodeHandle& nh)
    : impl_(new Impl(nh))
{
}

Publisher PointCloudTransport::advertise(const std::string& base_topic, uint32_t queue_size, bool latch)
{
  return advertise(base_topic, queue_size, {}, {}, {}, latch);
}

Publisher PointCloudTransport::advertise(const std::string& base_topic, uint32_t queue_size,
                                         const point_cloud_transport::SubscriberStatusCallback& connect_cb,
                                         const point_cloud_transport::SubscriberStatusCallback& disconnect_cb,
                                         const ros::VoidPtr& tracked_object, bool latch)
{
  return {impl_->nh_, base_topic, queue_size, connect_cb, disconnect_cb, tracked_object, latch, impl_->pub_loader_};
}

Subscriber PointCloudTransport::subscribe(
    const std::string& base_topic, uint32_t queue_size,
    const boost::function<void(const sensor_msgs::PointCloud2ConstPtr&)>& callback,
    const ros::VoidPtr& tracked_object, const point_cloud_transport::TransportHints& transport_hints)
{
  return {impl_->nh_, base_topic, queue_size, callback, tracked_object, transport_hints, impl_->sub_loader_};
}

std::vector<std::string> PointCloudTransport::getDeclaredTransports() const
{
  auto transports = impl_->sub_loader_->getDeclaredClasses();

  // Remove the "_sub" at the end of each class name.
  for (auto& transport : transports)
  {
    transport = boost::erase_last_copy(transport, "_sub");
  }
  return transports;
}

std::vector<std::string> PointCloudTransport::getLoadableTransports() const
{
  std::vector<std::string> loadableTransports;

  for (const auto& transportPlugin : impl_->sub_loader_->getDeclaredClasses())
  {
    // If the plugin loads without throwing an exception, add its transport name to the list of valid plugins,
    // otherwise ignore it.
    try
    {
      auto sub = impl_->sub_loader_->createInstance(transportPlugin);
      // Remove the "_sub" at the end of each class name.
      loadableTransports.push_back(boost::erase_last_copy(transportPlugin, "_sub"));
    }
    catch (const pluginlib::LibraryLoadException& e)
    {
    }
    catch (const pluginlib::CreateClassException& e)
    {
    }
  }

  return loadableTransports;
}

}