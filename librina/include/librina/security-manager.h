/*
 * Security Manager
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef LIBRINA_CACEP_H
#define LIBRINA_CACEP_H

#ifdef __cplusplus

#include <openssl/dh.h>

#include "librina/application.h"
#include "librina/rib.h"
#include "librina/internal-events.h"
#include "librina/timer.h"
#include "librina/configuration.h"

namespace rina {

//Contains data related to a particular security context
class ISecurityContext {
public:
	ISecurityContext(int id_) : id(id_) { };
	virtual ~ISecurityContext() { };

	PolicyConfig crcPolicy;
	PolicyConfig ttlPolicy;
	int id;
};

class IAuthPolicySet : public IPolicySet {
public:
	enum AuthStatus {
		IN_PROGRESS, SUCCESSFULL, FAILED
	};

	static const std::string AUTH_NONE;
	static const std::string AUTH_PASSWORD;
	static const std::string AUTH_SSH2;

	IAuthPolicySet(const std::string& type_);
	virtual ~IAuthPolicySet() { };

	/// get auth_policy
	virtual AuthPolicy get_auth_policy(int session_id,
					   const AuthSDUProtectionProfile& profile) = 0;

	/// initiate the authentication of a remote AE. Any values originated
	/// from authentication such as sesion keys will be stored in the
	/// corresponding security context
	virtual AuthStatus initiate_authentication(const AuthPolicy& auth_policy,
						   const AuthSDUProtectionProfile& profile,
						   int session_id) = 0;

	/// Process an incoming CDAP message
	virtual int process_incoming_message(const CDAPMessage& message,
					     int session_id) = 0;

	// The type of authentication policy
	std::string type;
};

class ISecurityManager;

class AuthNonePolicySet : public IAuthPolicySet {
public:
	AuthNonePolicySet(ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_NONE), sec_man(sm) { };
	virtual ~AuthNonePolicySet() { };
	AuthPolicy get_auth_policy(int session_id,
				   const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const AuthPolicy& auth_policy,
					   const AuthSDUProtectionProfile& profile,
					   int session_id);
	int process_incoming_message(const CDAPMessage& message, int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);

private:
	ISecurityManager * sec_man;
};

class AuthPasswordPolicySet;

class CancelAuthTimerTask : public TimerTask {
public:
	CancelAuthTimerTask(ISecurityManager * sm,
			int session_id_) : sec_man(sm),
			session_id(session_id_) { };
	~CancelAuthTimerTask() throw() { };
	void run();

	ISecurityManager * sec_man;
	int session_id;
};

class AuthPasswordSecurityContext : public ISecurityContext {
public:
	AuthPasswordSecurityContext(int session_id);
	~AuthPasswordSecurityContext() {
		if (challenge) {
			delete challenge;
		}
	};

	std::string cipher;
	std::string password;
	// Owned by a timer
	CancelAuthTimerTask * timer_task;
	std::string * challenge;
	int challenge_length;
};

/// As defined in PRISTINE's D4.1, online at
/// https://wiki.ict-pristine.eu/wp4/d41/Authentication-mechanisms#The-AuthNPassword-Authentication-Mechanism
class AuthPasswordPolicySet : public IAuthPolicySet {
public:
	static const std::string PASSWORD;
	static const std::string CIPHER;
	static const std::string CHALLENGE_LENGTH;
	static const std::string CHALLENGE_REQUEST;
	static const std::string CHALLENGE_REPLY;
	static const std::string DEFAULT_CIPHER;
	static const int DEFAULT_TIMEOUT;

	AuthPasswordPolicySet(IRIBDaemon * ribd,
			      ISecurityManager * sec_man);
	~AuthPasswordPolicySet() { };
	AuthPolicy get_auth_policy(int session_id,
				   const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const AuthPolicy& auth_policy,
					   const AuthSDUProtectionProfile& profile,
					   int session_id);
	int process_incoming_message(const CDAPMessage& message,
				     int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);

private:
	std::string * generate_random_challenge(int length);
	std::string encrypt_challenge(const std::string& challenge,
				      const std::string password);
	std::string decrypt_challenge(const std::string& encrypted_challenge,
				      const std::string& password);
	int process_challenge_request(const std::string& challenge,
			 	      int session_id);
	int process_challenge_reply(const std::string& encrypted_challenge,
			 	    int session_id);

	IRIBDaemon * rib_daemon;
	std::string cipher;
	ISecurityManager * sec_man;
	Timer timer;
	int timeout;
	Lockable lock;
};

/// Options that then SSH 2 authentication policy has to negotiate with its peer
class SSH2AuthOptions {
public:
	SSH2AuthOptions() { };
	~SSH2AuthOptions() { };

	/// Supported key exchange algorithms
	std::list<std::string> key_exch_algs;

	/// Supported encryption algorithms
	std::list<std::string> encrypt_algs;

	/// Supported MAC algorithms
	std::list<std::string> mac_algs;

	/// Supported compression algorithms
	std::list<std::string> compress_algs;

	/// DH public key
	UcharArray dh_public_key;
	/// DH parameters p and g
	UcharArray dh_param_p;
	UcharArray dh_param_g;
};

///Captures all data of the SSHRSA security context
class SSH2SecurityContext : public ISecurityContext {
public:
	SSH2SecurityContext(int session_id) : ISecurityContext(session_id),
			state(BEGIN), dh_state(NULL), dh_peer_pub_key(NULL) { };
	~SSH2SecurityContext();

        enum State {
        	BEGIN,
                WAIT_EDH_EXCHANGE,
                EDH_COMPLETED
        };

        State state;

	/// Negotiated algorithms
	std::string key_exch_alg;
	std::string encrypt_alg;
	std::string mac_alg;
	std::string compress_alg;

	///Diffie-Hellman key exchange state
	DH * dh_state;

	///The EDH public key of the peer
	BIGNUM * dh_peer_pub_key;

	///The shared secret, used to generate the encryption key
	UcharArray shared_secret;
};

/// Authentication policy set that mimics SSH approach. It is associated to
/// a cryptographic SDU protection policy, which is configured by this Authz policy.
/// It uses the Open SSL crypto library to perform all its functions
/// 1: Negotiation of versions
/// 2: Negotiation of algorithms
/// 3: Encryption key generation and exchange (configuring SDU protection policy)
/// 4: Authentication
class AuthSSH2PolicySet : public IAuthPolicySet {
public:
	static const std::string KEY_EXCHANGE_ALGORITHM;
	static const std::string ENCRYPTION_ALGORITHM;
	static const std::string MAC_ALGORITHM;
	static const std::string COMPRESSION_ALGORITHM;
	static const int DEFAULT_TIMEOUT;
	static const std::string EDH_EXCHANGE;

	AuthSSH2PolicySet(IRIBDaemon * ribd, ISecurityManager * sm);
	~AuthSSH2PolicySet();
	AuthPolicy get_auth_policy(int session_id,
				   const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const AuthPolicy& auth_policy,
				           const AuthSDUProtectionProfile& profile,
					   int session_id);
	int process_incoming_message(const CDAPMessage& message, int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);

private:
	//Convert Big Number to binary
	unsigned char * BN_to_binary(BIGNUM *b, int *len);

	/// Initialize parameters (p, g) for DH key exchange
	void edh_init_params();

	/// Initialize keys for DH key exchange with own P and G params.
	/// Returns 0 if successful -1 otherwise.
	int edh_init_keys(SSH2SecurityContext * sc, BIGNUM * p, BIGNUM * g);

	/// Generate the shared secret using the peer's public key.
	/// Returns 0 if successful, -1 otherwise
	int edh_generate_shared_secret(SSH2SecurityContext * sc);

	int process_edh_exchange_message(const CDAPMessage& message, int session_id);

	IRIBDaemon * rib_daemon;
	ISecurityManager * sec_man;
	DH * dh_parameters;
	Timer timer;
	int timeout;
	Lockable lock;
};

class ISecurityManager: public ApplicationEntity, public InternalEventListener {
public:
	ISecurityManager() : rina::ApplicationEntity(SECURITY_MANAGER_AE_NAME) { };
        virtual ~ISecurityManager();
        int add_auth_policy_set(const std::string& auth_type);
        int set_policy_set_param(const std::string& path,
                                 const std::string& name,
                                 const std::string& value);
        IAuthPolicySet * get_auth_policy_set(const std::string& auth_type);
        ISecurityContext * get_security_context(int context_id);
        ISecurityContext * remove_security_context(int context_id);
        void add_security_context(ISecurityContext * context);
        void eventHappened(InternalEvent * event);

private:
        /// The authentication policy sets, by type
        ThreadSafeMapOfPointers<std::string, IAuthPolicySet> auth_policy_sets;

        /// The security contexts, by session-id (the port-id of the flow
        /// used for enrollment=
        ThreadSafeMapOfPointers<int, ISecurityContext> security_contexts;
};

}

#endif

#endif
