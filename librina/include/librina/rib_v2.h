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
class RIB;
class RIBDaemonProxy;
class RIBOpsRespHandlers;

/// RIB version has been already registered for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBAlreadyRegistered);

/// Could not find a valid RIB version for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

/// RIB is not registered in this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotRegistered);

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
	virtual std::string get_type() const = 0;
	bool operator=(const AbstractEncoder &other) const;
	bool operator!=(const AbstractEncoder &other) const;
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

/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class RIBObj {

public:
	virtual ~RIBObj(){};

	virtual std::string get_displayable_value();
	// FIXME fix object data displayable

	///
	/// Remote invocations, resulting from CDAP messages
	///

	///
	/// Process a remote create
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* create(const std::string& name, const std::string clas,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);
	///
	/// Process a remote delete operation
	///
	/// @param name FQN of the object
	///
	virtual cdap_rib::res_info_t* delete_(const std::string& name);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @obj_reply Serialized object to be returned.
	///
	virtual cdap_rib::res_info_t* read(const std::string& name,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a cancel remote read operation
	///
	/// @param name FQN of the object
	///
	virtual cdap_rib::res_info_t* cancelRead(const std::string& name);

	///
	///
	/// Process a remote write operation
	///
	/// @param name FQN of the object
	/// @param obj_req Serialized object from the request
	/// @param obj_reply Optional serialized object to be returned.
	///                  Will only be decoded by the RIB library if size != 0.
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* write(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* start(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* stop(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	virtual const std::string& get_class() const;
	virtual const std::string& get_name() const;
	virtual long get_instance() const;
	virtual AbstractEncoder* get_encoder() const = 0;
protected:
	std::string class_;
	std::string name_;
	unsigned long instance_;
private:
	void operation_not_supported();
};

/**
 * RIB library result codes
 */
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

class RIBSchema {

public:
	friend class RIB;
	RIBSchema(const cdap_rib::vers_info_t *version, char separator);
	~RIBSchema();
	rib_schema_res ribSchemaDefContRelation(const std::string& cont_class_name,
			const std::string& class_name,
			const bool mandatory,
			const unsigned max_objs);
	char get_separator() const;
	const cdap_rib::vers_info_t& get_version() const;
private:
	bool validateAddObject(const RIBObj* obj);
	bool validateRemoveObject(const RIBObj* obj,
			const RIBObj* parent);
	const cdap_rib::vers_info_t *version_;
	std::map<std::string, RIBSchemaObject*> rib_schema_;
	char separator_;
};

//
// EmptyClass
//
class EmptyClass {

};

class EmptyEncoder : public rib::Encoder<EmptyClass> {

public:
	virtual void encode(const EmptyClass &obj, cdap_rib::SerializedObject& serobj){
		(void)serobj;
		(void)obj;
	};
	virtual void decode(const cdap_rib::SerializedObject &serobj,
			EmptyClass& des_obj){
		(void)serobj;
		(void)des_obj;
	};
	std::string get_type() const{
		return "EmptyClass";
	};
};

//fwd decl
class RIBDaemon;

//
// RIBDaemon Proxy class
//
class RIBDaemonProxy{

	///
	/// Register a RIB to an AE
	///
	void registerRIB(RIB* rib, const std::string& ae_name);

	///
	/// List registered RIB versions
	///
	std::list<uint64_t> listVersions(void);

	///
	/// Retrieve a pointer to the RIB
	///
	/// @param version RIB version
	/// @param Application Entity Name
	///
	/// @ret A pointer to the RIB object or NULL.
	///
	RIB* get(uint64_t version, const std::string& ae_name);

	///
	/// Retrieve a list of pointers to the RIB of a certain version
	///
	/// @param version RIB version
	///
	/// @ret a list of pointers to RIB objects
	///
	//std::list<RIB*> get(uint64_t version);

	///
	/// Retrieve a list of pointers to the RIB(s) registered in an AE
	///
	/// @param  RIB version
	///
	/// @ret a list of pointers to RIB objects
	///
	//std::list<RIB*> get(const std::string& ae_name);


	/// Unregister a RIB
	///
	/// Unregisters a RIB from the library. This method does NOT
	/// destroy the RIB instance.
	///
	/// @warning Current API does NOT guarantee that the RIB object can
	/// be removed, even if it is not registered in any other AE.
	///
	/// @ret On success, it returns the pointer to the RIB instance
	///
	RIB* unregisterRIB(RIB* rib, const std::string& ae_name);


	//
	// RIB Client
	//

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
			const cdap_rib::src_info_t &src,
			const cdap_rib::dest_info_t &dest,
			const cdap_rib::auth_info &auth, int port_id);

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

	//Constructor
	RIBDaemonProxy(RIBDaemon* ribd_);

	friend RIBDaemonProxy* RIBDaemonProxyFactory();

	RIBDaemon* ribd;
};


} //namespace rib
} //namespace rina
#endif /* RIB_PROVIDER_H_ */
