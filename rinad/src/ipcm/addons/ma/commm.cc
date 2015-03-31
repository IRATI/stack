/*
 * Communication task for the MA daemon
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *
 * This program is free software{} you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation{} either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program{} if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "commm.h"

// CLASS Manager
Manager::Manager()
{}

Manager::~Manager()
{}

const rina::ApplicationProcessNamingInformation& Manager::get_name() const
{
  return name_;
}

// CLASS CACEPStateMachine
CACEPStateMachine::CACEPStateMachine()
{
  cacep_state_ = NO_CONNECTION;
}

CACEPStateMachine::~CACEPStateMachine()
{
}

CACEPStateMachine::State CACEPStateMachine::getCurrentState() const
{
  return cacep_state_;
}

bool CACEPStateMachine::changeState(rina::CDAPMessage::Opcode op_code,
                                    Operation op)
{
  bool transition_completed = false;
  switch (op_code) {
    case rina::CDAPMessage::M_CONNECT:
      if (cacep_state_ == NO_CONNECTION) {
        if (op == SENT)
          cacep_state_ = WAIT_CONNECT_RESPONSE;
        else
          cacep_state_ = CONNECT_REQUEST_RECEIVED;
        transition_completed = true;
      }
      break;
    case rina::CDAPMessage::M_CONNECT_R:
      if (cacep_state_ == WAIT_CONNECT_RESPONSE && op == RECEIVED) {
        cacep_state_ = CONNECTED;
        transition_completed = true;
      } else if (cacep_state_ == CONNECT_REQUEST_RECEIVED && op == SENT) {
        cacep_state_ = CONNECTED;
        transition_completed = true;
      }
      break;
    default:
      break;

  }
  return transition_completed;
}

// CLASS CACEPManager
CACEPManager::CACEPManager(const Manager &manager)
    : manager_(manager)
{
  (void)manager_;
}

CACEPManager::~CACEPManager()
{
}

void CACEPManager::intiateCACEP(/*Enrollment info*/)
{

}

void CACEPManager::connect(int invoke_id,
                           rina::CDAPSessionDescriptor *session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

void CACEPManager::connectResponse(
    int result, const std::string& result_reason,
    rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) result;
  (void) result_reason;
  (void) session_descriptor;
}

void CACEPManager::release(int invoke_id,
                           rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

void CACEPManager::releaseResponse(
    int result, const std::string& result_reason,
    rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) result;
  (void) result_reason;
  (void) session_descriptor;
}

CACEPStateMachine::State CACEPManager::getCurrentState() const
{
  return CACEPStateMachine::NO_CONNECTION;
}

//CLASS CACEPManagerFactory
CACEPManagerFactory::~CACEPManagerFactory()
{
  for (std::map<std::string, CACEPManager*>::iterator it = active_managers_
      .begin(); it != active_managers_.end(); ++it) {
    delete it->second;
  }
  active_managers_.clear();
}

CACEPStateMachine::State CACEPManagerFactory::getInstance(
    const Manager &manager, CACEPManager *cacep_manager)
{
  std::map<std::string, CACEPManager*>::iterator it = active_managers_.find(
      manager.get_name().processName);
  if (it == active_managers_.end())
    cacep_manager = it->second;
  else {
    cacep_manager = new CACEPManager(manager);
    active_managers_[manager.get_name().processName] = cacep_manager;
  }
  return cacep_manager->getCurrentState();
}

void CACEPManagerFactory::destroyInstance(const Manager &manager)
{
  std::map<std::string, CACEPManager*>::iterator it = active_managers_.find(
      manager.get_name().processName);
  delete it->second;
  active_managers_.erase(it);
}
void CACEPManagerFactory::destroyInstance(CACEPManager *cacep_manager)
{
  for (std::map<std::string, CACEPManager*>::iterator it = active_managers_
      .begin(); it != active_managers_.end(); ++it) {
    if (it->second == cacep_manager) {
      delete it->second;
      active_managers_.erase(it);
      break;
    }
  }
}

// CLASS EnrollmentStateMachine
EnrollmentStateMachine::EnrollmentStateMachine()
{
  enrollment_state_ = NO_ENROLLED;
}

EnrollmentStateMachine::~EnrollmentStateMachine()
{
}

EnrollmentStateMachine::State EnrollmentStateMachine::getCurrentState() const
{
  return enrollment_state_;
}

bool EnrollmentStateMachine::changeState(rina::CDAPMessage::Opcode op_code)
{
  (void) op_code;
  return true;
}

/// CLASS EnrollmentManager
EnrollmentManager::EnrollmentManager()
{
}

EnrollmentManager::~EnrollmentManager()
{
}

void EnrollmentManager::intiateEnrollment()
{
}

void EnrollmentManager::start(int invoke_id,
                              rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

void EnrollmentManager::startResponse(
    int invoke_id, rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

void EnrollmentManager::stop(int invoke_id,
                             rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

void EnrollmentManager::stopResponse(
    int invoke_id, rina::CDAPSessionDescriptor * session_descriptor)
{
  (void) invoke_id;
  (void) session_descriptor;
}

/// CLASS CommunicationManager
CommunicationManager::CommunicationManager()
{
}

CommunicationManager::~CommunicationManager()
{
}

bool CommunicationManager::initiateCommunicationSession()
{
  return true;
}
