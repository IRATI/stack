/*
 * Communication task for the MA daemon
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef ENROLLT_H_
#define ENROLLT_H_

#ifdef __cplusplus

#include <librina/cdap.h>
#include <librina/rib.h>

/// @class Manager
class Manager
{
 public:
  Manager();
  ~Manager();

  const rina::ApplicationProcessNamingInformation& get_name() const;
 private:
  rina::ApplicationProcessNamingInformation name_;  ///< The IPC Process name of the neighbor
  rina::ApplicationProcessNamingInformation supporting_dif_name_;  ///< The name of the supporting DIF used to exchange data
  std::list<rina::ApplicationProcessNamingInformation> supporting_difs_;  ///< The names of all the supporting DIFs of this neighbor
  //unsigned int address_;  ///< The address
};

/// @class CACEPStateMachine
/// @brief The CACEP state machine
class CACEPStateMachine
{
 public:
  enum Operation
  {
    SENT,
    RECEIVED
  };
  enum State
  {
    NO_CONNECTION,
    WAIT_CONNECT_RESPONSE,
    CONNECT_REQUEST_RECEIVED,
    CONNECTED
  };  ///< Possible states of the CACEP STATE Machines

  /// A Default Constructor
  CACEPStateMachine();
  /// A Destructor
  ~CACEPStateMachine();
  /// Get the current State of the state machine.
  /// @return The current state.
  State getCurrentState() const;
  /// Change the state of the state machine
  /// @param The opcode of the received CDAP message
  /// @return If the state transition is valid return true, otherwise return false.
  bool changeState(rina::CDAPMessage::Opcode op_code, Operation op);

 private:
  State cacep_state_;  ///< The current state of the state machine.
};

class CACEPManagerFactory;

/// @class CACEPManager
/// Manages the CACEP between the MA and one Manager
class CACEPManager : public rina::IApplicationConnectionHandler
{
  friend class CACEPManagerFactory;
 public:
  /// Initiate the CACEP.
  /// @ param
  void intiateCACEP(/*Enrollment info*/);

  /// Handle the connect request.
  /// @param invoke_id identifier of the CDAP message.
  /// @param session_descriptor information of the CDAP session.
  void connect(int invoke_id, rina::CDAPSessionDescriptor *session_descriptor);

  /// Handle the connect response.
  /// @param result the result of the request.
  /// @param result_reason the result reason.
  /// @param session_descriptor information of the CDAP session.
  void connectResponse(int result, const std::string& result_reason,
                       rina::CDAPSessionDescriptor * session_descriptor);

  /// Handle the release request.
  /// @param invoke_id identifier of the CDAP message.
  /// @param session_descriptor information of the CDAP session.
  void release(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor);

  /// Handle the release response.
  /// @param result the result of the request.
  /// @param result_reason the result reason.
  /// @param session_descriptor information of the CDAP session.
  void releaseResponse(int result, const std::string& result_reason,
                       rina::CDAPSessionDescriptor * session_descriptor);

  /// Get the current state of the CACEP state machine
  CACEPStateMachine::State getCurrentState() const;
 private:
  /// A default Constructor.
  CACEPManager(const Manager &manager);
  /// A Destructor.
  ~CACEPManager();

  CACEPStateMachine state_machine_;
  const Manager &manager_;
};

class CACEPManagerFactory
{
 public:
  ~CACEPManagerFactory();
  CACEPStateMachine::State getInstance(const Manager &manager,
                                       CACEPManager *cacep_manager);
  void destroyInstance(const Manager &manager);
  void destroyInstance(CACEPManager *cacep_manager);
 private:
  std::map<std::string, CACEPManager*> active_managers_;
};

/// @class EnrollmentStateMachine
/// The Enrollment state machine
class EnrollmentStateMachine
{
 public:
  enum State
  {
    NO_ENROLLED,
    WAIT_START_RESPONSE,
    START_REQUEST_RECEIVED,
    WAIT_STOP_REQUEST,
    START_RESPONSE_RECEIVED,
    WAIT_STOP_RESPONSE,
    STOP_REQUEST_RECEIVED,
    ENROLLED
  };  ///< Possible states of the Enrollment STATE Machines

  /// A default constructor.
  EnrollmentStateMachine();
  // A destructor
  ~EnrollmentStateMachine();
  /// Get the current State of the state machine.
  /// @return The current state.
  State getCurrentState() const;
  /// Change the state of the state machine
  /// @param The opcode of the received CDAP message
  /// @return If the state transition is valid return true, otherwise return false.
  bool changeState(rina::CDAPMessage::Opcode op_code);

 private:
  State enrollment_state_;  ///< The current state of the state machine.
};

/// @class EnrollmentManager
/// Manages the Enrollment between the MA and one Manager
class EnrollmentManager
{
 public:
  /// A default constructor
  EnrollmentManager();
  /// A destructor
  ~EnrollmentManager();

  /// Initiate the enrollment
  void intiateEnrollment();

  /// Handle the start request.
  /// @param invoke_id identifier of the CDAP message.
  /// @param session_descriptor information of the CDAP session.
  void start(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor);

  /// Handle the start response.
  /// @param result the result of the request.
  /// @param result_reason the result reason.
  /// @param session_descriptor information of the CDAP session.
  void startResponse(int invoke_id,
                     rina::CDAPSessionDescriptor * session_descriptor);

  /// Handle the stop request.
  /// @param invoke_id identifier of the CDAP message.
  /// @param session_descriptor information of the CDAP session.
  void stop(int invoke_id, rina::CDAPSessionDescriptor * session_descriptor);

  /// Handle the stop response.
  /// @param result the result of the request.
  /// @param result_reason the result reason.
  /// @param session_descriptor information of the CDAP session.
  void stopResponse(int invoke_id,
                    rina::CDAPSessionDescriptor * session_descriptor);

  EnrollmentStateMachine::State getCurrentState() const;
 private:
  EnrollmentStateMachine state_machine_;
  Manager *manager_;
};

/// @class ConmunicationManager
/// Manages the Enrollment between the MA and one Manager.
class CommunicationManager
{
 public:
  /// A default constructor.
  CommunicationManager();
  /// A destructor.
  ~CommunicationManager();
  /// Initiate the communication betwen this Ma and a Manager
  bool initiateCommunicationSession();

 private:
  //CACEPManager *cacep_manager_;  ///< The CACEP manager.
  EnrollmentManager enrollment_manager_;  /// the enrollment manager.
  Manager neighbor_;
};
#endif
#endif /* ENROLLT_H_ */
