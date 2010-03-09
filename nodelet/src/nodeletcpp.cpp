/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

/**
@mainpage

\author Radu Bogdan Rusu

@b nodeletcpp is a tool for loading/unloading nodelets to/from a Nodelet manager.
**/
#include <ros/ros.h>
#include "nodelet/nodelet_loader.h"
#include "nodelet/NodeletList.h"
#include "nodelet/NodeletLoad.h"
#include "nodelet/NodeletUnload.h"

////////////////////////////////////////////////////////////////////////////////
/** \brief Parse for a specific given command line argument. */
template <typename Type> int
  parseArguments (int argc, char** argv, const char* str, Type &value)
{
  for (int i = 1; i < argc; ++i)
  {
    if ((strcmp (argv[i], str) == 0) && (++i < argc))
    {
      value = boost::lexical_cast<Type>(argv[i]);
      return (i-1);
    }
  }
  return (-1);
}

class NodeletInterface
{
  public:
    ////////////////////////////////////////////////////////////////////////////////
    /** \brief Unload the nodelet */
    bool
      unloadNodelet (const char *name, const char *manager)
    {
      ROS_INFO_STREAM ("Unloading nodelet " << name << " from manager " << manager);
      
      std::string service_name = std::string (manager) + "/unload_nodelet";
      // Wait until the service is advertised
      ros::ServiceClient client = n_.serviceClient<nodelet::NodeletUnload> (service_name);
      client.waitForExistence ();

      // Call the service
      nodelet::NodeletLoad srv;
      srv.request.name = std::string (name);
      if (!client.call (srv))
      {
        ROS_ERROR ("Failed to call service!");
        return (false);
      }
      return (true);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /** \brief Load the nodelet */
    bool
      loadNodelet (const char *name, const char *type, const char *manager)
    {
      ros::M_string remappings = ros::names::getRemappings ();
      std::vector<std::string> sources (remappings.size ()), targets (remappings.size ());
      ROS_INFO_STREAM ("Loading nodelet " << name << " of type " << type << " to manager " << manager << " with the following remappings:");
      int i = 0;
      for (ros::M_string::iterator it = remappings.begin (); it != remappings.end (); ++it, ++i)
      {
        sources[i] = (*it).first; targets[i] = (*it).second;
        ROS_INFO_STREAM (sources[i] << " -> " << targets[i]);
      }

      // Get and set the parameters
      XmlRpc::XmlRpcValue param;
      std::string node_name = ros::this_node::getName ();
      n_.getParam (node_name, param);
      n_.setParam (name, param);

      std::string service_name = std::string (manager) + "/load_nodelet";

      // Wait until the service is advertised
      ros::ServiceClient client = n_.serviceClient<nodelet::NodeletLoad> (service_name);
      client.waitForExistence ();

      // Call the service
      nodelet::NodeletLoad srv;
      srv.request.name = std::string (name);
      srv.request.type = std::string (type);
      srv.request.remap_source_args = sources;
      srv.request.remap_target_args = targets;
      if (!client.call (srv))
      {
        ROS_ERROR ("Failed to call service!");
        return (false);
      }
      return (true);
    }
  private:
    ros::NodeHandle n_;
};

/* ---[ */
int
  main (int argc, char** argv)
{
  ros::init (argc, argv, "nodeletcpp", ros::init_options::AnonymousName);
  if (argc < 4)
  {
    ROS_ERROR ("Need at least 4 arguments to continue: <name> <type> <manager_node>");
    return (-1);
  }
  bool unload = false; 
  int unload_idx = parseArguments<bool> (argc, argv, "--unload", unload);

  NodeletInterface ni;
  if (unload)
    ni.unloadNodelet (argv[unload_idx], argv[unload_idx+1]);
  else
  {
    ni.loadNodelet (argv[1], argv[2], argv[3]);
    ros::spin ();
  }

  return (0);
}