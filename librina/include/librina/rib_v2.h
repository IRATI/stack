/*
 * RIB API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
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
#ifndef RIB_PROVIDER_H_
#define RIB_PROVIDER_H_
#include "cdap_rib_structures.h"
#include <string>
#include <inttypes.h>
#include <list>
#include <map>
#include <algorithm>
#include "librina/concurrency.h"
#include "librina/exceptions.h"

namespace rina{
namespace cacep {

// FIXME: this class is only used in enrollment, it must go in a different file that rib
class AppConHandlerInterface {

public:
	virtual ~AppConHandlerInterface(){};

	/// A remote Connect request has been received.
	virtual void connect(int invoke_id, const cdap_rib::con_handle_t &con) = 0;
	/// A remote Connect response has been received.
	virtual void connectResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con) = 0;
	/// A remote Release request has been received.
	virtual void release(int invoke_id, const cdap_rib::con_handle_t &con) = 0;
	/// A remote Release response has been received.
	virtual void releaseResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con) = 0;
};

}//namespace cacep

namespace rib {

//fwd decl
class RIBDaemonProxy;
class RIBOpsRespHandlers;

//
// Schema exceptions
//

/// A schema with the same version has been already created
DECLARE_EXCEPTION_SUBCLASS(eSchemaExists);

/// The schema cannot removed because there are RIBs using it
DECLARE_EXCEPTION_SUBCLASS(eSchemaInUse);

/// The schema does not exist
DECLARE_EXCEPTION_SUBCLASS(eSchemaNotFound);

/// The class name is invalid
DECLARE_EXCEPTION_SUBCLASS(eSchemaInvalidClass);

/// The callback for that class / class&fqn is already registered
DECLARE_EXCEPTION_SUBCLASS(eSchemaCBRegExists);



//
// RIB&AE management
//

/// RIB version has been already registered for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBAlreadyAssociated);

/// Could not find a valid RIB version for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

/// RIB is being used by an AE
DECLARE_EXCEPTION_SUBCLASS(eRIBInUse);

/// RIB is not registered in this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotAssociated);

/// RIB is not registered in this AE
DECLARE_EXCEPTION_SUBCLASS(eNotImplemented);

//
// RIB object management
//

/// Invalid object
DECLARE_EXCEPTION_SUBCLASS(eObjInvalid);

/// Invalid object name
DECLARE_EXCEPTION_SUBCLASS(eObjInvalidName);

/// Validation error; the operation was rejected by the rules defined in
/// the schema.
DECLARE_EXCEPTION_SUBCLASS(eOpValidation);

/// An object already exists in the same position of the tree
DECLARE_EXCEPTION_SUBCLASS(eObjExists);

/// Parent's object does not exist
DECLARE_EXCEPTION_SUBCLASS(eObjNoParent);

/// Object cannot be deleted because it has children (yes, we are nice people)
DECLARE_EXCEPTION_SUBCLASS(eObjHasChildren);

/// Object does not support this operation
DECLARE_EXCEPTION_SUBCLASS(eObjOpNotSupported);

/// The object does not exist in that position of the tree
DECLARE_EXCEPTION_SUBCLASS(eObjDoesNotExist);

/// The object exists but the class name mismatches
DECLARE_EXCEPTION_SUBCLASS(eObjClassMismatch);





///
/// Initialize the RIB library (RIBDaemon)
///
/// This method initializes the state of the RIB library. It does:
///
/// * Initialize internal state of the RIB library (RIBDaemon)
/// * Intiialize the CDAP provider
///
///
void init(cacep::AppConHandlerInterface *app_con_callback,
		RIBOpsRespHandlers* remote_handlers,
		cdap_rib::cdap_params params);

//
// Get a proxy object to interface the RIBDaemon
//
// @ret A proxy object to the RIBDaemon
//
RIBDaemonProxy* RIBDaemonProxyFactory();


///
/// Destroy the RIB library state
///
void fini(void);

//
// RIB operations response handlers
//
class RIBOpsRespHandlers {

public:
	virtual ~RIBOpsRespHandlers(){};

	virtual void remoteCreateResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteDeleteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteCancelReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteWriteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteStartResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteStopResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
};

class AbstractEncoder {

public:
	virtual ~AbstractEncoder();
};

template<class T>
class Encoder: public AbstractEncoder {

public:
	virtual ~Encoder(){}

	/// Converts an object to a byte array, if this object is recognized by the encoder
	/// @param object
	/// @throws exception if the object is not recognized by the encoder
	/// @return
	virtual void encode(const T &obj, cdap_rib::ser_obj_t& serobj) = 0;
	/// Converts a byte array to an object of the type specified by "className"
	/// @param byte[] serializedObject
	/// @param objectClass The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the
	/// encoder can recognize, or the byte array value doesn't correspond to an
	/// object of the type "className"
	/// @return
	virtual void decode(const cdap_rib::ser_obj_t &serobj,
			T& des_obj) = 0;
};

///
/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class RIBObj{

public:

	///
	/// Constructor
	///
	/// @param user User-specific context (type associated)
	/// @param fqn Fully qualifed name (
	///
	RIBObj(const std::string& class_) : delegates(false),
					parent_inst_id(-1),
					class_name(class_){};

	/// Fully qualified name
	const std::string fqn;

	/// Destructor
	virtual ~RIBObj(){};

protected:
	///
	/// Remote invocations, resulting from CDAP messages
	///

	///
	/// Process a remote create
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void create(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);
	///
	/// Process a remote delete operation
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	/// @ret True if the object has to be deleted after the callback has
	///      returned
	///
	virtual bool delete_(const cdap_rib::con_handle_t &con,
					const std::string& fqn,
					const std::string& class_,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id,
					cdap_rib::res_info_t& res);

	///
	///
	/// Process a remote read operation
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @obj_reply Serialized object to be returned.
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void read(const cdap_rib::con_handle_t &con,
					const std::string& fqn,
					const std::string& class_,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id,
					cdap_rib::SerializedObject &obj_reply,
					cdap_rib::res_info_t& res);

	///
	///
	/// Process a cancel remote read operation
	///
	/// @param fqn FQN of the object
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void cancelRead(const cdap_rib::con_handle_t &con,
					const std::string& fqn,
					const std::string& class_,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id,
					cdap_rib::res_info_t& res);

	///
	///
	/// Process a remote write operation
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param obj_req Serialized object from the request
	/// @param obj_reply Optional serialized object to be returned.
	///                  Will only be decoded by the RIB library if size != 0.
	///                  Initialized to size = 0 by default.
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void write(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	///
	///
	/// Process a remote read operation
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void start(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	///
	///
	/// Process a remote read operation
	///
	/// @param con Connection handle
	/// @param fqn FQN of the object
	/// @param class_ Class name
	/// @param filt Filter parameters
	/// @param invoke_id Invoke id
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	/// @param res Result. The result code shall be set by the callback.
	///            In case of error, a human readable string can be
	///            be optionally added
	///
	virtual void stop(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	///
	/// Get the class name.
	///
	/// Method that inheriting classes MUST implementing returing the
	/// class name
	///
	///
	virtual const std::string& get_class() const{
		return class_name;
	};

	///
	/// Throw not supported exception
	///
	void operation_not_supported(cdap_rib::res_info_t& res);

	///Rwlock
	rina::ReadWriteLockable rwlock;

	//Flag used to identify objects which delegates a portion of the tree
	bool delegates;

	//Instance id of the parent
	int64_t parent_inst_id;

	//Class name
	const std::string class_name;

	//Them too; promiscuous?
	friend class RIB;
};


///
/// Root object instance ID
///
#define RIB_ROOT_INST_ID 0

///
/// Root object class
///
#define RIB_ROOT_CN "Root"

///
/// @internal Root object class
///
class RootObj : public RIBObj{
private:
	RootObj(void) : RIBObj(RIB_ROOT_CN){ };
	~RootObj(void){};

	//Only the RIB can instantiate a RootObj
	friend class RIB;
};

///
/// Delegation object class
///
#define RIB_DELEG_CN "DelegationObj"


///
/// This class is used to capture operations on objects in a part of the tree
/// without having to add explicitely the objects (catch all)
///
class DelegationObj : public RIBObj{

public:
	/// Constructor
	DelegationObj(void) : RIBObj(RIB_DELEG_CN) {
		delegates = true;
	};

	//Destructor
	~DelegationObj(void){};
};

///
/// RIB library result codes
///
enum rib_schema_res {

	RIB_SUCCESS,
	/* The RIB schema file extension is unknown */
	RIB_SCHEMA_EXT_ERR = -3,
	/* Error during RIB scheema file parsing */
	RIB_SCHEMA_FORMAT_ERR = -4,
	/* General validation error (unknown) */
	RIB_SCHEMA_VALIDATION_ERR = -5,
	/* Validation error, missing mandatory object */
	RIB_SCHEMA_VAL_MAN_ERR = -6,
	//
	// Misc
	//
	//TODO: Other error codes
};


///
/// RIB handle type
///
typedef int64_t rib_handle_t;

///
/// Schema's create callback prototype
///
typedef void (*create_cb_t)(const rib_handle_t rib,
				const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

///
/// RIB Schema Object
///
class RIBSchemaObject {

public:
	RIBSchemaObject(const std::string& class_name, const bool mandatory,
			const unsigned max_objs);
	void addChild(RIBSchemaObject *object);
	const std::string& get_class_name() const;
	unsigned get_max_objs() const;
private:
	std::string class_name_;
	RIBSchemaObject *parent_;
	std::list<RIBSchemaObject*> children_;
	bool mandatory_;
	unsigned max_objs_;
};


//fwd decl
class RIBDaemon;

//
// RIBDaemon Proxy class
//
class RIBDaemonProxy{

public:

	//-------------------------------------------------------------------//
	//                         Local RIBs                                //
	//-------------------------------------------------------------------//


	///
	/// Create a RIB schema
	///
	///
	/// @throws eSchemaExists and Exception
	///
	void createSchema(const cdap_rib::vers_info_t& version,
						const char separator = '/');
	///
	/// List registered RIB versions
	///
	std::list<cdap_rib::vers_info_t> listVersions(void);

	///
	/// Register a callback for CREATE operations
	///
	/// This method registers a callback method for CREATE operations. The
	/// callback can be registered either:
	///
	/// * For a class name *and* fully qualified name, in many locations in
	///   the tree as needed. (specific)
	/// * For a class name (generic)
	///
	/// Specific registrations always have preference over a generic.
	///
	/// @param version Schema version
	/// @param class_ Mandatory classhandle The handle of the RIB
	/// @param fqn Fully qualified name (position in the tree)
	/// @param cb Pointer to the callback method
	///
	/// @throws eSchemaNotFound, eSchemaInvalidClass and eSchemaCBRegExists
	///
	void addCreateCallbackSchema(const cdap_rib::vers_info_t& version,
						const std::string& class_,
						const std::string& fqn_,
						create_cb_t cb);

	///
	/// Destroys a RIB schema
	///
	/// This method destroy a previously created schema. The schema shall
	/// not be currently used by any RIB instance or eSchemaInUse exception
	/// will be thrown.
	///
	/// @throws eSchemaInUse, eSchemaNotFound and Exception
	///
	void destroySchema(const cdap_rib::vers_info_t& version);

	///
	/// Create a RIB
	///
	/// This method creates an empty RIB and returns a handle to it. The
	/// RIB instance won't be operational until it has been associated to
	/// one or more Application Entities (AEs).
	///
	/// @ret The RIB handle
	/// @throws Exception on failure
	///
	rib_handle_t createRIB(const cdap_rib::vers_info_t& version);

	///
	/// Destroy a RIB instance
	///
	/// Destroys a previously created RIB instance. The instance shall not
	/// be assocated to any AE or it will throw eRIBInUse
	///
	/// @throws eRIBInUse, eRIBNotFound or Exception on failure
	void destroyRIB(const rib_handle_t& handle);

	///
	/// Associate a RIB to an Applicatin Entity (AE)
	///
	/// @throws eRIBNotFound, eRIBAlreadyAssociated or Exception on failure
	///
	void associateRIBtoAE(const rib_handle_t& handle,
						const std::string& ae_name);

	/// Deassociate RIB from an Application Entity
	///
	/// This method deassociates a RIB from an AE. This method does NOT
	/// destroy the RIB instance.
	///
	/// @throws eRIBNotFound, eRIBNotAssociated and Exception
	///
	void deassociateRIBfromAE(const rib_handle_t& handle,
						const std::string& ae_name);

	///
	/// Retrieve the handle to a RIB
	///
	/// @param version RIB version
	/// @param Application Entity Name
	///
	/// @ret A handle to a RIB
	/// @throws eRIBNotFound
	///
	rib_handle_t get(const cdap_rib::vers_info_t& version,
						const std::string& ae_name);

	///
	/// Add an object to a RIB
	///
	/// This method attempts to add an object to the existing RIB (handle).
	/// On success, *obj is set to NULL and the callee shall not retain any
	/// copy of that pointer.
	///
	/// On failure the adequate exception is thrown, and no changes to obj
	/// will be made.
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully qualified name (position in the tree)
	/// @param obj A pointer (to a pointer) to the object, that derives
	/// from RIBObj.
	///
	/// @ret The instance id of the object created
	/// @throws eRIBNotFound, eObjExists, eObjInvalid, eObjNoParent
	///
	template<typename T>
	int64_t addObjRIB(const rib_handle_t& handle, const std::string& fqn,
							 T** obj){
		RIBObj** obj_;
		//Recover the base class
		try{
			obj_ = reinterpret_cast<RIBObj**>(obj);
		}catch(...){
			throw eObjInvalid();
		}
		return __addObjRIB(handle, fqn, obj_);
	}

	///
	/// Retrieve the instance ID of an object given its fully
	/// qualified name.
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully Qualified Name of the object
	/// @param class__ Optional parameter. When defined (!=""), the class
	/// name of the object is checked to be strictly equl to class_
	///
	/// @ret The instance id of the object
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	/// if class_ is defined.
	///
	int64_t getObjInstId(const rib_handle_t& handle,
					const std::string& fqn,
					const std::string& class_="");

	///
	/// Get parent's fully qualified name
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully Qualified Name of the child object
	///
	/// @ret Parent's Fqn
	/// @throws eRIBNotFound, eObjDoesNotExist
	///
	std::string getObjParentFqn(const rib_handle_t& handle,
						const std::string& fqn);

	///
	/// Retrieve the fully qualified name given the instance ID of an
	/// object
	///
	/// @param handle The handle of the RIB
	/// @param inst_id Object's instance id
	/// @param class__ Optional parameter. When defined (!=""), the class
	///
	/// @ret The fully qualified name
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	/// if class_ is defined
	///
	std::string getObjFqn(const rib_handle_t& handle,
					const int64_t inst_id,
					const std::string& class_="");
	///
	/// Retrieve the class name of an object given the instance ID.
	///
	/// @param handle The handle of the RIB
	/// @param inst_id Object's instance id
	/// @ret The class name
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	///
	std::string getObjClass(const rib_handle_t& handle,
					const int64_t inst_id);
	///
	/// Remove an object to a RIB
	///
	/// This method removes an object previously added to the RIB
	///
	/// On failure the adequate exception is thrown and no changes will
	/// be performed in the RIB.
	///
	/// @param handle The handle of the RIB
	/// @param inst_id The object instance ID
	///
	/// @throws eRIBNotFound, eObjDoesNotExist
	///
	void removeObjRIB(const rib_handle_t& handle, const int64_t inst_id);


	//-------------------------------------------------------------------//
	//                         RIB Client                                //
	//-------------------------------------------------------------------//

	///
	/// Establish a CDAP connection to a remote RIB
	///
	/// @param ver RIB version
	/// @param src Application source information
	/// @param dst Application dst information
	/// @param auth CDAP Authentication context
	/// @param port_id Flow port id to be used
	/// @ret A CDAP connection handle
	///
	cdap_rib::con_handle_t remote_open_connection(
			const cdap_rib::vers_info_t &ver,
			const cdap_rib::ep_info_t &src,
			const cdap_rib::ep_info_t &dest,
			const cdap_rib::auth_policy &auth, int port_id);

	///
	/// Close a CDAP connection to a remote RIB
	///
	/// @ret success/failure
	///
	int remote_close_connection(unsigned int port);

	///
	/// Perform a create operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_create(unsigned int port,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt);

	///
	/// Perform a delete operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_delete(unsigned int port,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt);

	///
	/// Perform a read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_read(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt);
	///
	/// Perform a cancel read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_cancel_read(unsigned int port,
				       const cdap_rib::flags_t &flags,
				       int invoke_id);

	///
	/// Perform a write operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_write(unsigned int port,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt);

	///
	/// Perform a start operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_start(unsigned int port,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt);

	///
	/// Perform a stop operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_stop(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt);

private:
	///@internal
	int64_t __addObjRIB(const rib_handle_t& h, const std::string& fqn,
								 RIBObj** o);
	//Constructor
	RIBDaemonProxy(RIBDaemon* ribd_);

	friend RIBDaemonProxy* RIBDaemonProxyFactory();

	RIBDaemon* ribd;
};


} //namespace rib
} //namespace rina
#endif /* RIB_PROVIDER_H_ */
