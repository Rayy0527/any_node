/**********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2016, Philipp Leemann
 * All rights reserved.
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
 *   * Neither the name of Robotic Systems Lab nor ETH Zurich
 *     nor the names of its contributors may be used to endorse or
 *     promote products derived from this software without specific
 *     prior written permission.
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
 */

/*!
 * @file	WorkerManager.cpp
 * @author	Philipp Leemann
 * @date	July, 2016
 */

#include "any_worker/WorkerManager.hpp"
#include "message_logger/message_logger.hpp"

namespace any_worker {

WorkerManager::WorkerManager():
    workers_(),
    mutexWorkers_()
{

}

WorkerManager::~WorkerManager() {
    clearWorkers();
}

bool WorkerManager::addWorker(const WorkerOptions& options) {
    std::lock_guard<std::mutex> lock(mutexWorkers_);
    auto insertedElement = workers_.emplace( options.name_, Worker(options) );
    if(!insertedElement.second) {
        MELO_ERROR("Failed to create worker [%s]", options.name_.c_str());
        return false;
    }
    if(options.autostart_) {
      return insertedElement.first->second.start();
    }
    return true;
}

//bool WorkerManager::addWorker(Worker&& worker) {
//    std::lock_guard<std::mutex> lock(mutexWorkers_);
//    auto insertedElement = workers_.emplace( worker.getName(), std::move(worker) );
//    if(!insertedElement.second) {
//        MELO_ERROR("Failed to move worker [%s]", worker.getName().c_str());
//        return false;
//    }
//    return true;
//}

void WorkerManager::startWorker(const std::string& name, const int priority) {
    std::lock_guard<std::mutex> lock(mutexWorkers_);
    auto worker = workers_.find(name);
    if(worker == workers_.end()) {
        MELO_ERROR("Cannot start worker [%s], worker not found", name.c_str());
    }
    worker->second.start(priority);
}

void WorkerManager::startWorkers() {
    for(auto & worker : workers_) {
      worker.second.start();
    }
}

void WorkerManager::stopWorker(const std::string& name, const bool wait) {
    std::lock_guard<std::mutex> lock(mutexWorkers_);
    auto worker = workers_.find(name);
    if(worker == workers_.end()) {
        MELO_ERROR("Cannot stop worker [%s], worker not found", name.c_str());
    }
    worker->second.stop(wait);
}

void WorkerManager::stopWorkers(const bool wait) {
    for(auto & worker : workers_) {
      worker.second.stop(wait);
    }
}

void WorkerManager::setWorkerTimestep(const std::string& name, const double timeStep) {
    std::lock_guard<std::mutex> lock(mutexWorkers_);
    auto worker = workers_.find(name);
    if(worker == workers_.end()) {
        MELO_ERROR("Cannot change timestep of worker [%s], worker not found", name.c_str());
    }
    worker->second.setTimestep(timeStep);
}

void WorkerManager::clearWorkers() {
    std::lock_guard<std::mutex> lock(mutexWorkers_);

    // signal all workers to stop
    for(auto& worker : workers_) {
        worker.second.stop(false);
    }

    // call destructors of all workers, which will join the underlying thread
    workers_.clear();
}

void WorkerManager::cleanDestructibleWorkers() {
    std::lock_guard<std::mutex> lock(mutexWorkers_);
    for(auto it = workers_.begin(); it != workers_.end(); ) {
        if(it->second.isDestructible()) {
            it = workers_.erase(it);
        }else {
            ++it;
        }
    }
}


} // namespace any_worker
