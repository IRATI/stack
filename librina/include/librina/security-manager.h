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
#include <openssl/rsa.h>

#include "librina/application.h"
#include "librina/rib_v2.h"
#include "librina/internal-events.h"
#include "librina/timer.h"
#include "librina/configuration.h"

namespace rina {

//Contains data related to a particular security context
class ISecurityContext {
public:
	ISecurityContext(int id_, const std::string& name_)
		: id(id_), auth_policy_name(name_) { };
	virtual ~ISecurityContext() { };

	PolicyConfig crcPolicy;
	PolicyConfig ttlPolicy;
	cdap_rib::con_handle_t con;
	int id;
	std::string auth_policy_name;
};

class IAuthPolicySet : public IPolicySet {
public:
	enum AuthStatus {
		IN_PROGRESS, SUCCESSFULL, FAILED
	};

	static const std::string AUTH_NONE;
	static const std::string AUTH_PASSWORD;
	static const std::string AUTH_SSH2;
	static const std::string AUTH_TLSHAND;

	IAuthPolicySet(const std::string& type_);
	virtual ~IAuthPolicySet() { };

	/// get auth_policy
	virtual cdap_rib::auth_policy_t get_auth_policy(int session_id,
							const cdap_rib::ep_info_t& peer_ap,
					   	 	const AuthSDUProtectionProfile& profile) = 0;

	/// initiate the authentication of a remote AE. Any values originated
	/// from authentication such as sesion keys will be stored in the
	/// corresponding security context
	virtual AuthStatus initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
						   const AuthSDUProtectionProfile& profile,
						   const cdap_rib::ep_info_t& peer_ap,
						   int session_id) = 0;

	/// Process an incoming CDAP message
	virtual int process_incoming_message(const cdap::CDAPMessage& message,
					     int session_id) = 0;

	//Called when the crypto state has been updated on a certain port, if the call
	//to the Security Manager's "update crypto state" was asynchronous
	virtual AuthStatus crypto_state_updated(int port_id) = 0;

	// The type of authentication policy
	std::string type;
};

/// MAIN security manager PS, mainly dealing with access control policies
/// for the DAF (joining the DAF) and the DAP's RIB.
class ISecurityManagerPs : public IPolicySet {
// This class is used by the IPCP to access the plugin functionalities
public:
	/// Decide if an IPC Process is allowed to join a DIF.
	/// 0 success, < 0 error
	virtual int isAllowedToJoinDAF(const cdap_rib::con_handle_t & con,
				       const Neighbor& newMember,
				       cdap_rib::auth_policy_t & auth) = 0;

	//Validate and store access control credentials.0 success, < 0 error.
	virtual int storeAccessControlCreds(const cdap_rib::auth_policy_t & auth,
					    const cdap_rib::con_handle_t & con) = 0;

	virtual int getAccessControlCreds(cdap_rib::auth_policy_t & auth,
					  const cdap_rib::con_handle_t & con) = 0;

	virtual void checkRIBOperation(const cdap_rib::auth_policy_t & auth,
				       const cdap_rib::con_handle_t & con,
				       const cdap::cdap_m_t::Opcode opcode,
				       const std::string obj_name,
				       cdap_rib::res_info_t& res) = 0;

        virtual ~ISecurityManagerPs() {}
};

class ISecurityManager;

class AuthNonePolicySet : public IAuthPolicySet {
public:
	AuthNonePolicySet(ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_NONE), sec_man(sm) { };
	virtual ~AuthNonePolicySet() { };
	cdap_rib::auth_policy_t get_auth_policy(int session_id,
						const cdap_rib::ep_info_t& peer_ap,
				   	 	const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
					   const AuthSDUProtectionProfile& profile,
					   const cdap_rib::ep_info_t& peer_ap,
					   int session_id);
	int process_incoming_message(const cdap::CDAPMessage& message, int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);
	AuthStatus crypto_state_updated(int port_id);

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
	std::string name() const {
		return "cancel-auth";
	}

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

	AuthPasswordPolicySet(rib::RIBDaemonProxy * ribd,
			      ISecurityManager * sec_man);
	~AuthPasswordPolicySet() { };
	cdap_rib::auth_policy_t get_auth_policy(int session_id,
						const cdap_rib::ep_info_t& peer_ap,
				   	        const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
					   const AuthSDUProtectionProfile& profile,
					   const cdap_rib::ep_info_t& peer_ap,
					   int session_id);
	int process_incoming_message(const cdap::CDAPMessage& message,
				     int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);
	AuthStatus crypto_state_updated(int port_id);

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

	rib::RIBDaemonProxy * rib_daemon;
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
};

class CryptoState {
public:
	CryptoState() : port_id(0), enable_crypto_tx(false),
			enable_crypto_rx(false){ };
	bool operator==(const CryptoState &other) const;
	bool operator!=(const CryptoState &other) const;
	static void from_c_crypto_state(CryptoState & cs,
					struct sdup_crypto_state * ccs);
	struct sdup_crypto_state * to_c_crypto_state(void) const;
	std::string toString(void) const;

	int port_id;
	std::string mac_alg;
	std::string encrypt_alg;
	std::string compress_alg;
	bool enable_crypto_tx;
	bool enable_crypto_rx;
	UcharArray encrypt_key_tx;
	UcharArray encrypt_key_rx;
	UcharArray mac_key_tx;
	UcharArray mac_key_rx;
	UcharArray iv_tx;
	UcharArray iv_rx;
};

///Captures all data of the SSHRSA security context
class SSH2SecurityContext : public ISecurityContext {
public:
	SSH2SecurityContext(int session_id) : ISecurityContext(session_id, IAuthPolicySet::AUTH_SSH2),
			state(BEGIN), dh_state(NULL), dh_peer_pub_key(NULL),
			auth_keypair(NULL), auth_peer_pub_key(NULL),
			crypto_tx_enabled(false), crypto_rx_enabled (false),
			timer_task(NULL) { };
	SSH2SecurityContext(int session_id,
			    const std::string& peer_ap_name,
			    const AuthSDUProtectionProfile& profile);
	SSH2SecurityContext(int session_id,
			    const std::string& peer_ap_name,
			    const AuthSDUProtectionProfile& profile,
			    SSH2AuthOptions * options);
	~SSH2SecurityContext();
	CryptoState get_crypto_state(bool enable_crypto_tx,
				     bool enable_crypto_rx,
				     bool isserver);

	static const std::string KEY_EXCHANGE_ALGORITHM;
	static const std::string ENCRYPTION_ALGORITHM;
	static const std::string MAC_ALGORITHM;
	static const std::string COMPRESSION_ALGORITHM;
	static const std::string KEYSTORE_PATH;
	static const std::string KEYSTORE_PASSWORD;
	static const std::string KEY;
	static const std::string PUBLIC_KEY;
	static const std::string KNOWN_IPCPS;

        enum State {
        	BEGIN,
                WAIT_EDH_EXCHANGE,
                REQUESTED_ENABLE_ENCRYPTION_SERVER,
                REQUESTED_ENABLE_DECRYPTION_SERVER,
                REQUESTED_ENABLE_ENCRYPTION_DECRYPTION_CLIENT,
                WAIT_CLIENT_CHALLENGE,
                WAIT_CLIENT_CHALLENGE_REPLY,
                WAIT_SERVER_CHALLENGE_REPLY,
                DONE
        };

        State state;

	/// Negotiated algorithms
	std::string key_exch_alg;
	std::string encrypt_alg;
	std::string mac_alg;
	std::string compress_alg;

	/// Authentication Keystore path and password
	std::string keystore_path;
	std::string keystore_password;

	/// Encryption policy configuration
	PolicyConfig encrypt_policy_config;

	///Diffie-Hellman key exchange state
	DH * dh_state;

	///The EDH public key of the peer
	BIGNUM * dh_peer_pub_key;

	///The shared secret, used to generate the encryption key
	UcharArray shared_secret;

	///The encryption keys
	UcharArray encrypt_key_client;
	UcharArray encrypt_key_server;

	///The hmac keys
	UcharArray mac_key_client;
	UcharArray mac_key_server;

	/// My RSA * key pair (used for authentication)
	RSA * auth_keypair;

	/// Public key of the IPCP I'm authenticating against
	RSA * auth_peer_pub_key;

	bool crypto_tx_enabled;
	bool crypto_rx_enabled;

	/// Application process name of the IPCP I'm authenticating against
	std::string peer_ap_name;

	/// Challenge sent to peer so that he proofs it has the DIF's key
	UcharArray challenge;

	// Owned by a timer
	CancelAuthTimerTask * timer_task;

private:
	//return -1 if options are valid, 0 otherwise
	int validate_options(const SSH2AuthOptions& options);
};

class BoolConditionVariable: public ConditionVariable {
public:
	BoolConditionVariable() : enabled(false) {};
	void wait_until_condition_is_true() {
		lock();
		if (!enabled) {
			doWait();
		}
		unlock();
	}
	void enable_condition() {
		lock();
		enabled = true;
		signal();
		unlock();
	}

private:
	bool enabled;
};

/// Authentication policy set that mimics SSH approach. It is associated to
/// a cryptographic SDU protection policy, which is configured by this Authz policy.
/// It uses the Open SSL crypto library to perform all its functions
/// 1: Negotiation of versions
/// 2: Negotiation of algorithms
/// 3: Encryption key generation
/// 4: Configuration of encryption policy on N-1 port
/// 5: Authentication using public/private key of DIF
class AuthSSH2PolicySet : public IAuthPolicySet {
public:
	static const int DEFAULT_TIMEOUT;
	static const std::string EDH_EXCHANGE;
	static const int MIN_RSA_KEY_PAIR_LENGTH;
	static const std::string CLIENT_CHALLENGE;
	static const std::string CLIENT_CHALLENGE_REPLY;
	static const std::string SERVER_CHALLENGE_REPLY;

	AuthSSH2PolicySet(rib::RIBDaemonProxy * ribd, ISecurityManager * sm);
	virtual ~AuthSSH2PolicySet();
	cdap_rib::auth_policy_t get_auth_policy(int session_id,
						const cdap_rib::ep_info_t& peer_ap,
				   	        const AuthSDUProtectionProfile& profile);
	AuthStatus initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
				           const AuthSDUProtectionProfile& profile,
				           const cdap_rib::ep_info_t& peer_ap,
					   int session_id);
	int process_incoming_message(const cdap::CDAPMessage& message, int session_id);
	int set_policy_set_param(const std::string& name,
	                         const std::string& value);

	//Called when encryption has been enabled on a certain port, if the call
	//to the Security Manager's "enable encryption" was asynchronous
	AuthStatus crypto_state_updated(int port_id);

private:
	AuthStatus decryption_enabled_server(SSH2SecurityContext * sc);
	AuthStatus encryption_enabled_server(SSH2SecurityContext * sc);
	AuthStatus encryption_decryption_enabled_client(SSH2SecurityContext * sc);

	//Convert Big Number to binary
	unsigned char * BN_to_binary(BIGNUM *b, int *len);

	// Load the authentication keys required for this DIF from a file
	// Returns 0 if successful, -1 if there is a failure
	int load_authentication_keys(SSH2SecurityContext * sc);

	/// Initialize parameters (p, g) for DH key exchange
	void edh_init_params();

	/// Initialize keys for DH key exchange with own P and G params.
	/// Returns 0 if successful -1 otherwise.
	int edh_init_keys(SSH2SecurityContext * sc);

	/// Generate the shared secret using the peer's public key.
	/// Returns 0 if successful, -1 otherwise
	int edh_generate_shared_secret(SSH2SecurityContext * sc);

	int process_edh_exchange_message(const cdap::CDAPMessage& message,
					 int session_id);

	/// Generate a random challenge
	/// Return 0 if successful, -1 otherwise
	int generate_random_challenge(SSH2SecurityContext * sc);

	/// Encrypt random challenge with public key.
	/// Return 0 if successful, -1 otherwise
	int encrypt_chall_with_pub_key(SSH2SecurityContext * sc, UcharArray& encrypted_chall);

	int process_client_challenge_message(const cdap::CDAPMessage& message,
					     int session_id);

	/// Encrypt random challenge with public key.
	/// Return 0 if successful, -1 otherwise
	int decrypt_chall_with_priv_key(SSH2SecurityContext * sc,
				        const UcharArray& encrypted_challenge,
				        UcharArray& challenge);

	int decrypt_combine_and_hash(SSH2SecurityContext * sc,
				     const UcharArray& challenge,
				     UcharArray& result);

	int process_client_challenge_reply_message(const cdap::CDAPMessage& message,
						   int session_id);

	int process_server_challenge_reply_message(const cdap::CDAPMessage& message,
						   int session_id);

	int check_challenge_reply(SSH2SecurityContext * sc,
				  UcharArray& received_challenge);

	int generate_and_encrypt_challenge(SSH2SecurityContext * sc,
					   UcharArray& challenge);

	rib::RIBDaemonProxy * rib_daemon;
	ISecurityManager * sec_man;
	Lockable lock;
	BoolConditionVariable encryption_ready_condition;
	DH * dh_parameters;
	Timer timer;
	int timeout;
};

class ISecurityManager: public ApplicationEntity, public InternalEventListener {
public:
	ISecurityManager() : rina::ApplicationEntity(SECURITY_MANAGER_AE_NAME) { };
        virtual ~ISecurityManager();
        int add_auth_policy_set(const std::string& auth_type);
        void add_auth_policy_set(const std::string& auth_type,
        			 IAuthPolicySet * ps);
        int set_policy_set_param(const std::string& path,
                                 const std::string& name,
                                 const std::string& value);
        IAuthPolicySet * get_auth_policy_set(const std::string& auth_type);
        ISecurityContext * get_security_context(int context_id);
        ISecurityContext * remove_security_context(int context_id);
        void destroy_security_context(int context_id);
        void add_security_context(ISecurityContext * context);
        void eventHappened(InternalEvent * event);
        virtual IAuthPolicySet::AuthStatus update_crypto_state(const CryptoState& profile,
        						       IAuthPolicySet * caller) = 0;

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
