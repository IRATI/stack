/*
 * API to the RIB Daemon and Base RIB Objects
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_RIB_H
#define LIBRINA_RIB_H

#ifdef __cplusplus

#include "application.h"
#include "cdap.h"

namespace rina {

class RIBNamingConstants {
public:
	static const std::string DAF;
	static const std::string DIF_REGISTRATIONS;
	static const std::string IRM;
	static const std::string MANAGEMENT;
	static const std::string N_MINUS_ONE_FLOWS;
	static const std::string NEIGHBORS;
	static const std::string SEPARATOR;
};

/// Encodes and Decodes an object to/from bytes)
class EncoderInterface {
public:
        virtual ~EncoderInterface(){};
         /// Converts an object to a byte array, if this object is recognized by the encoder
         /// @param object
         /// @throws exception if the object is not recognized by the encoder
         /// @return
        virtual const SerializedObject* encode(const void* object) = 0;
         /// Converts a byte array to an object of the type specified by "className"
         /// @param byte[] serializedObject
         /// @param objectClass The type of object to be decoded
         /// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
         /// byte array value doesn't correspond to an object of the type "className"
         /// @return
        virtual void* decode(const ObjectValueInterface * serialized_object) const = 0;
};

/// Factory that provides and Encoder instance
class EncoderFactoryInterface {
public:
        virtual ~EncoderFactoryInterface(){};
        virtual EncoderInterface* createEncoderInstance() = 0;
};

/// Contains the data of an object in the RIB
class RIBObjectData{
public:
        RIBObjectData();
        RIBObjectData(std::string clazz, std::string name,
                        long long instance);
        bool operator==(const RIBObjectData &other) const;
        bool operator!=(const RIBObjectData &other) const;
#ifndef SWIG
        const std::string& get_class() const;
        void set_class(const std::string& clazz);
        unsigned long get_instance() const;
        void set_instance(unsigned long  instance);
        const std::string& get_name() const;
        void set_name(const std::string& name);
        const std::string& get_displayable_value() const;
        void set_displayable_value(const std::string& displayable_value);
#endif

        /** The class (type) of object */
        std::string class_;

        /** The name of the object (unique within a class)*/
        std::string name_;

        /** A synonim for clazz+name (unique within the RIB) */
        unsigned long instance_;

        /**
         * The value of the object, encoded in an string for
         * displayable purposes
         */
        std::string displayable_value_;
};

class IRIBDaemon;
class IEncoder;

/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class BaseRIBObject {
public:
        virtual ~BaseRIBObject(){};
        BaseRIBObject(IRIBDaemon * rib_daemon, const std::string& object_class,
                      long object_instance,
                      const std::string& object_name);
        RIBObjectData get_data();
        virtual std::string get_displayable_value();
        virtual const void* get_value() const = 0;

        /// Parent-child management operations
        const std::list<BaseRIBObject*>& get_children() const;
        void add_child(BaseRIBObject * child);
        void remove_child(const std::string& objectName);

        /// Local invocations
        virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue);
        virtual void deleteObject(const void* objectValue);
        virtual BaseRIBObject * readObject();
        virtual void writeObject(const void* object_value);
        virtual void startObject(const void* object);
        virtual void stopObject(const void* object);

        /// Remote invocations, resulting from CDAP messages
        virtual void remoteCreateObject(void * object_value, const std::string& object_name,
                        int invoke_id, CDAPSessionDescriptor * session_descriptor);
        virtual void remoteDeleteObject(int invoke_id,
                        CDAPSessionDescriptor * session_descriptor);
        virtual void remoteReadObject(int invoke_id,
                        CDAPSessionDescriptor * session_descriptor);
        virtual void remoteCancelReadObject(int invoke_id,
                        CDAPSessionDescriptor * cdapSessionDescriptor);
        virtual void remoteWriteObject(void * object_value, int invoke_id,
                        CDAPSessionDescriptor * cdapSessionDescriptor);
        virtual void remoteStartObject(void * object_value, int invoke_id,
                        CDAPSessionDescriptor * cdapSessionDescriptor);
        virtual void remoteStopObject(void * object_value, int invoke_id,
                        CDAPSessionDescriptor * cdapSessionDescriptor);

        std::string class_;
        std::string name_;
        long instance_;
        BaseRIBObject * parent_;
        IRIBDaemon * base_rib_daemon_;
        IEncoder * encoder_;

private:
        std::list<BaseRIBObject*> children_;
        void operation_not_supported();
        void operation_not_supported(const void* object);
        void operation_not_supported(const CDAPMessage * cdapMessage,
                        CDAPSessionDescriptor * cdapSessionDescriptor);
        void operartion_not_supported(const std::string& objectClass,
                                      const std::string& objectName,
                        const void* objectValue);
};

/// Common interface for update strategies implementations. Can be on demand, scheduled, periodic
class IUpdateStrategy {
public:
        virtual ~IUpdateStrategy(){};
};

/// Part of the RIB Daemon API to control if the changes have to be notified
class NotificationPolicy {
public:
        NotificationPolicy(const std::list<int>& cdap_session_ids);
        std::list<int> cdap_session_ids_;
};

///Identifies a remote Process by address
///or N-1 port-id of a flow we have in common with him
class RemoteProcessId {
public:
        RemoteProcessId();
        bool use_address_;
        int port_id_;
        unsigned int address_;
};

class RIBObjectValue {
public:
        enum objectType{
                notype,
                inttype,
                longtype,
                stringtype,
                complextype,
                floattype,
                doubletype,
                booltype,
                bytestype
        };

        RIBObjectValue();

        objectType type_;
        int int_value_;
        bool bool_value_;
        long long_value_;
        double double_value_;
        float float_value_;
        std::string string_value_;
        void * complex_value_;
        SerializedObject bytes_value_;
};

/// Interface that provides the RIB Daemon API
class IRIBDaemon : public ApplicationEntity {
public:
	IRIBDaemon() : ApplicationEntity(ApplicationEntity::RIB_DAEMON_AE_NAME) { };
        virtual ~IRIBDaemon(){};

        /// Add an object to the RIB
        /// @param ribHandler
        /// @param objectName
        /// @throws Exception
        virtual void addRIBObject(BaseRIBObject * ribObject) = 0;

        /// Remove an object from the RIB. Ownership is passed to the RIB daemon,
        /// who will delete the memory associated to the object.
        /// @param ribObject
        /// @throws Exception
        virtual void removeRIBObject(BaseRIBObject * ribObject) = 0;

        /// Remove an object from the RIB by objectname. Ownership is passed to the RIB daemon,
        /// who will delete the memory associated to the object.
        /// @param objectName
        /// @throws Exception
        virtual void removeRIBObject(const std::string& objectName) = 0;

        /// Send an information update, consisting on a set of CDAP messages, using the updateStrategy update strategy
        /// (on demand, scheduled). Takes ownership of the CDAP messages
        /// @param cdapMessages
        /// @param updateStrategy
        virtual void sendMessages(const std::list<const CDAPMessage*>& cdapMessages,
                        const IUpdateStrategy& updateStrategy) = 0;

        /// Causes a CDAP message to be sent. Takes ownership of the CDAP message
        /// @param cdapMessage the message to be sent
        /// @param sessionId the CDAP session id
        /// @param cdapMessageHandler the class to be called when the response message is received (if required)
        /// @throws Exception
        virtual void sendMessage(const CDAPMessage & cdapMessage,
                                 int sessionId,
                                 ICDAPResponseMessageHandler * cdapMessageHandler) = 0;

        /// Causes a CDAP message to be sent. Takes ownership of the CDAPMessage
        /// @param cdapMessage the message to be sent
        /// @param address the address of the IPC Process to send the Message To
        /// @param cdapMessageHandler the class to be called when the response message is received (if required)
        /// @throws Exception
        virtual void sendAData(const CDAPMessage & cdapMessage,
                                          unsigned int address,
                                          ICDAPResponseMessageHandler * cdapMessageHandler) = 0;

        /// The RIB Daemon has to process the CDAP message and,
        /// if valid, it will either pass it to interested subscribers and/or write to storage and/or modify other
        /// tasks data structures. It may be the case that the CDAP message is not addressed to an application
        /// entity within this IPC process, then the RMT may decide to rely the message to the right destination
        /// (after consulting an adequate forwarding table).
        ///
        /// This operation takes ownership of the message argument.
        ///
        /// @param message the encoded CDAP message
        /// @param length the length of the encoded message
        /// @param portId the portId the message came from
        virtual void cdapMessageDelivered(char* message,
                                          int length,
                                          int portId) = 0;

        /// Create or update an object in the RIB
        /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @param objectValue the value of the object
        /// @param notify if not null notify some of the neighbors about the change
        /// @throws Exception
        virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue,
                                  const NotificationPolicy * notificationPolicy) = 0;

        /// Delete an object from the RIB
        /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @param object the value of the object
        /// @param notify if not null notify some of the neighbors about the change
        /// @throws Exception
        virtual void deleteObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue,
                                  const NotificationPolicy * notificationPolicy) = 0;

        /// Read an object from the RIB
        /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @return a RIB object
        /// @throws Exception
        virtual BaseRIBObject * readObject(const std::string& objectClass,
                        const std::string& objectName) = 0;

        /// Update the value of an object in the RIB
    /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @param objectValue the new value of the object
        /// @param notify if not null notify some of the neighbors about the change
        /// @throws Exception
        virtual void writeObject(const std::string& objectClass,
                                 const std::string& objectName,
                                 const void* objectValue) = 0;

        /// Start an object at the RIB
        /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @param objectValue the new value of the object
        /// @throws Exception
        virtual void startObject(const std::string& objectClass,
                                 const std::string& objectName,
                                 const void* objectValue) = 0;

        /// Stop an object at the RIB
        /// @param objectClass the class of the object
        /// @param objectName the name of the object
        /// @param objectInstance the instance of the object
        /// @param objectValue the new value of the object
        /// @throws Exception
        virtual void stopObject(const std::string& objectClass,
                                const std::string& objectName,
                                const void* objectValue) = 0;

        virtual std::list<BaseRIBObject *> getRIBObjects() = 0;

        /// Request the establishment of an application connection with another IPC Process
        /// @param auth_mech
        /// @param auth_value
        /// @param dest_ae_inst
        /// @param dest_ae_name
        /// @param dest_ap_inst
        /// @param dest_ap_name
        /// @param src_ae_inst
        /// @param src_ae_name
        /// @param src_ap_inst
        /// @param src_ap_name
        /// @param remote_id
        virtual void openApplicationConnection(
                        const rina::AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                        const std::string &dest_ae_name, const std::string &dest_ap_inst,
                        const std::string &dest_ap_name, const std::string &src_ae_inst,
                        const std::string &src_ae_name, const std::string &src_ap_inst,
                        const std::string &src_ap_name, const RemoteProcessId& remote_id) = 0;

        /// Request an application connection to be closed
        /// @param remote_id
        /// @param response_handler
        virtual void closeApplicationConnection(const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a create operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteCreateObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a delete operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteDeleteObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a read operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteReadObject(const std::string& object_class, const std::string& object_name,
                        int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a write operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteWriteObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a start operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteStartObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Invoke a stop operation on an object in the RIB of a remote IPC Process
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param scope
        /// @param remote_id
        /// @param response_handler
        virtual void remoteStopObject(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler) = 0;

        /// Causes the RIB Daemon to send an open connection response message to the IPC Process identified
        /// by remote_id
        /// @param auth_mech
        /// @param auth_value
        /// @param dest_ae_inst
        /// @param dest_ae_name
        /// @param dest_ap_inst
        /// @param dest_ap_name
        /// @param result
        /// @param result_reason
        /// @param src_ae_inst
        /// @param src_ae_name
        /// @param src_ap_inst
        /// @param src_ap_name
        /// @param invoke_id
        /// @param remote_id
        virtual void openApplicationConnectionResponse(
                        const rina::AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                        const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
                        int result, const std::string &result_reason, const std::string &src_ae_inst,
                        const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
                        int invoke_id, const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to terminate an application connection with an IPC Process identified
        /// by remote_id
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void closeApplicationConnectionResponse(int result, const std::string result_reason,
                        int invoke_id, const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a create response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a delete response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
                        int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a read response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param result
        /// @param result_reason
        /// @param read_incomplete
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, bool read_incomplete,
                        int invoke_id, const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a start response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
                        int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a start response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id) = 0;

        /// Causes the RIB Daemon to send a start response message to the IPC Process identified
        /// by remote_id
        /// @param object_class
        /// @param object_name
        /// @param object_value
        /// @param result
        /// @param result_reason
        /// @param invoke_id
        /// @param remote_id
        virtual void remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id) = 0;
};

/// Generates unique object instances
class ObjectInstanceGenerator: public rina::Lockable {
public:
        ObjectInstanceGenerator();
        long getObjectInstance();
private:
        long instance_;
};

/// Make Object instance generator singleton
extern Singleton<ObjectInstanceGenerator> objectInstanceGenerator;

/// A simple RIB object that just acts as a wrapper. Represents an object in the RIB that just
/// can be read or written, and whose read/write operations have no side effects other than
/// updating the value of the object
class SimpleRIBObject: public BaseRIBObject {
public:
        SimpleRIBObject(IRIBDaemon * rib_daemon,
                        const std::string& object_class,
                        const std::string& object_name,
                        const void* object_value);
        virtual const void* get_value() const;
        virtual void        writeObject(const void* object);

        /// Create has the semantics of update
        virtual void createObject(const std::string& objectClass,
                                  const std::string& objectName,
                                  const void* objectValue);
private:
        const void* object_value_;
};

/// Class SimpleSetRIBObject. A RIB object that is a set and has no side effects
class SimpleSetRIBObject: public SimpleRIBObject {
public:
        SimpleSetRIBObject(IRIBDaemon * rib_daemon,
                           const std::string& object_class,
                           const std::string& set_member_object_class,
                           const std::string& object_name);
        void createObject(const std::string& objectClass,
                          const std::string& objectName,
                          const void* objectValue);

private:
        std::string set_member_object_class_;
};

/// Class SimpleSetMemberRIBObject. A RIB object that is member of a set
class SimpleSetMemberRIBObject: public SimpleRIBObject {
public:
        SimpleSetMemberRIBObject(IRIBDaemon * rib_daemon,
                                 const std::string& object_class,
                                 const std::string& object_name,
                                 const void* object_value);
        virtual void deleteObject(const void* objectValue);
};

/// A simple RIB implementation, based on a hashtable of RIB objects
/// indexed by object name
class RIB : public rina::Lockable {
public:
        RIB();
        ~RIB() throw();

        /// Given an objectname of the form "substring\0substring\0...substring" locate
        /// the RIBObject that corresponds to it
        /// @param objectName
        /// @return
        BaseRIBObject* getRIBObject(const std::string& objectClass,
                        const std::string& objectName, bool check);
        void addRIBObject(BaseRIBObject* ribObject);
        BaseRIBObject * removeRIBObject(const std::string& objectName);
        std::list<BaseRIBObject*> getRIBObjects();

private:
        std::map<std::string, BaseRIBObject*> rib_;
};

class IEncoder {
public:
        virtual ~IEncoder(){};

        /// Converts an object of the type specified by "className" to a byte array.
        /// @param object
        /// @return
        virtual void encode(const void* object, rina::CDAPMessage * cdapMessage) = 0;

        /// Converts a byte array to an object of the type specified by "className"
        /// @param serializedObject
        /// @param The type of object to be decoded
        /// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
        /// byte array value doesn't correspond to an object of the type "className"
        /// @return
        virtual void* decode(const rina::CDAPMessage * cdapMessage) = 0;
};

class IMasterEncoder: public rina::IEncoder {
public:
	virtual ~IMasterEncoder(){};
	/// Set the class that serializes/unserializes an object class
	/// @param objectClass The object class
	/// @param serializer

	virtual void addEncoder(const std::string& object_class, rina::EncoderInterface *encoder) = 0;
	/// Converts an object of the type specified by "className" to a byte array.
	/// @param object
	/// @return

	virtual void encode(const void* object, rina::CDAPMessage * cdapMessage) = 0;

	/// Converts a byte array to an object of the type specified by "className"
	/// @param serializedObject
	/// @param The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
	/// byte array value doesn't correspond to an object of the type "className"
	/// @return
	virtual void* decode(const rina::CDAPMessage * cdapMessage) = 0;
};

// /A RIB Daemon partial implementation, that internally uses the RIB
/// implementation provided by the RIB class. Complete implementations have
/// to extend this class to adapt it to the environment they are operating
class RIBDaemon : public IRIBDaemon {
public:
        RIBDaemon();
        void initialize(const std::string& separator, IEncoder * encoder,
                        CDAPSessionManagerInterface * cdap_session_manager_,
                        CACEPHandler * cacep_handler_);
        void addRIBObject(BaseRIBObject * ribObject);
        void removeRIBObject(BaseRIBObject * ribObject);
        void removeRIBObject(const std::string& objectName);
        std::list<BaseRIBObject *> getRIBObjects();
        void createObject(const std::string& objectClass, const std::string& objectName,
                        const void* objectValue, const NotificationPolicy * notificationPolicy);
        void deleteObject(const std::string& objectClass, const std::string& objectName,
                        const void* objectValue, const NotificationPolicy * notificationPolicy);
        BaseRIBObject * readObject(const std::string& objectClass,
                                const std::string& objectName);
        void writeObject(const std::string& objectClass, const std::string& objectName,
                                const void* objectValue);
        void startObject(const std::string& objectClass, const std::string& objectName,
                        const void* objectValue);
        void stopObject(const std::string& objectClass, const std::string& objectName,
                                const void* objectValue);
        void sendMessage(const CDAPMessage & cdapMessage, int sessionId,
                                ICDAPResponseMessageHandler * cdapMessageHandler);
        void sendAData(const CDAPMessage & cdapMessage, unsigned int address,
        		ICDAPResponseMessageHandler * cdapMessageHandler);
        void sendMessages(const std::list<const CDAPMessage*>& cdapMessages,
                                const IUpdateStrategy& updateStrategy);

        void openApplicationConnection(
                                const AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                                const std::string &dest_ae_name, const std::string &dest_ap_inst,
                                const std::string &dest_ap_name, const std::string &src_ae_inst,
                                const std::string &src_ae_name, const std::string &src_ap_inst,
                                const std::string &src_ap_name, const RemoteProcessId& remote_id);
        void closeApplicationConnection(const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteCreateObject(const std::string& object_class, const std::string& object_name,
                                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteDeleteObject(const std::string& object_class, const std::string& object_name,
                                int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteReadObject(const std::string& object_class, const std::string& object_name,
                                int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteWriteObject(const std::string& object_class, const std::string& object_name,
                                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteStartObject(const std::string& object_class, const std::string& object_name,
                                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void remoteStopObject(const std::string& object_class, const std::string& object_name,
                                RIBObjectValue& object_value, int scope, const RemoteProcessId& remote_id,
                                ICDAPResponseMessageHandler * response_handler);
        void openApplicationConnectionResponse(
                                const rina::AuthPolicy &auth_policy, const std::string &dest_ae_inst,
                                const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
                                int result, const std::string &result_reason, const std::string &src_ae_inst,
                                const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
                                int invoke_id, const RemoteProcessId& remote_id);
        void closeApplicationConnectionResponse(int result, const std::string result_reason,
                                int invoke_id, const RemoteProcessId& remote_id);
        void remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id);
        void remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
                        int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id);
        void remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, bool read_incomplete,
                        int invoke_id, const RemoteProcessId& remote_id);
        void remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
                                int result, const std::string result_reason, int invoke_id, const RemoteProcessId& remote_id);
        void remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id);
        void remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
                        RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
                        const RemoteProcessId& remote_id);

        /// Send a message to another Process. Takes ownership of the CDAPMessage.
        /// @param useAddress true if we're sending a message to an address, false if
        /// we are directly using the N-1 port-id
        /// @param cdapMessage the message to be sent
        /// @param sessionId the CDAP session id to be used
        /// @param address the address of the destination IPC Process
        /// @param cdapMessageHandler pointer to the class that will be handling the response
        /// message (if any)
        virtual void sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
                        unsigned int address, ICDAPResponseMessageHandler * cdapMessageHandler) = 0;

        void processIncomingCDAPMessage(const rina::CDAPMessage * cdapMessage,
        				rina::CDAPSessionDescriptor * descriptor,
        				const std::string& session_state);

        void encodeObject(RIBObjectValue& object_value, rina::CDAPMessage * message);

        /// CDAP Message handlers that have sent a CDAP message and are waiting for a reply
        ThreadSafeMapOfPointers<int, ICDAPResponseMessageHandler> handlers_waiting_for_reply_;

        CDAPSessionManagerInterface * cdap_session_manager_;

protected:
	void remote_operation_on_object(rina::CDAPMessage::Opcode opcode,
			const std::string& object_class, const std::string& object_name,
			int scope, const RemoteProcessId& remote_id,
			ICDAPResponseMessageHandler * response_handler);

	void remote_operation_on_object_with_value(rina::CDAPMessage::Opcode opcode,
			const std::string& object_class, const std::string& object_name,
			RIBObjectValue& object_value, int scope,
			const RemoteProcessId& remote_id,
			ICDAPResponseMessageHandler * response_handler);

	void remote_operation_response_with_value(rina::CDAPMessage::Opcode opcode,
			const std::string& object_class, const std::string& object_name,
			RIBObjectValue& object_value, int result, const std::string result_reason,
			int invoke_id, const RemoteProcessId& remote_id, rina::CDAPMessage::Flags flags);

	void remote_operation_response(rina::CDAPMessage::Opcode opcode,
			const std::string& object_class, const std::string& object_name,
			int result, const std::string result_reason,
			int invoke_id, const RemoteProcessId& remote_id);

private:
        RIB rib_;
        IEncoder * encoder_;
        CACEPHandler * cacep_handler_;
        std::string separator_;

        //Get a CDAP Message Handler waiting for a response message
        ICDAPResponseMessageHandler * getCDAPMessageHandler(const rina::CDAPMessage * cdapMessage);

        /// Process an incoming CDAP request message
        void processIncomingRequestMessage(const rina::CDAPMessage * cdapMessage,
                                rina::CDAPSessionDescriptor * cdapSessionDescriptor);

        /// Process an incoming CDAP response message
        void processIncomingResponseMessage(const rina::CDAPMessage * cdapMessage,
                        rina::CDAPSessionDescriptor * cdapSessionDescriptor);

        /// Finds out of the candidate number is on the list
        /// @param candidate
        /// @param list
        /// @return true if candidate is on list, false otherwise
        bool isOnList(int candidate, std::list<int> list);

        void sendMessageToProcess(const rina::CDAPMessage & cdapMessage, const RemoteProcessId& remote_id,
                        ICDAPResponseMessageHandler * response_handler);

        void assign_invoke_id_if_needed(CDAPMessage * message, bool invoke_id);
};

///Object exchanged between applications processes that
///contains the source and destination addresses of the processes
///and optional authentication information, as well as an
///encoded CDAP Message. It is used to exchange CDAP messages
///between APs without having a CDAP session previously established
///(it can be seen as a one message session)
class ADataObject {
public:
	static const std::string A_DATA;
	static const std::string A_DATA_OBJECT_CLASS;
	static const std::string A_DATA_OBJECT_NAME;

	ADataObject();
	ADataObject(unsigned int source_address,
			unsigned int dest_address);
	~ADataObject();

	//The address of the source AP (or IPCP)
	unsigned int source_address_;

	//The address of the destination AP (or IPCP)
	unsigned int dest_address_;

	const SerializedObject * encoded_cdap_message_;
};


}

#endif

#endif
