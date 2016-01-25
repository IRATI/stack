/*
 * Utilities
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_UTILS_HH
#define IPCP_UTILS_HH

namespace rinad {

class SysfsHelper {
public:
	static int get_value_as_string(std::string& file_name, std::string & value);
	static int get_value_as_int(std::string& file_name, int & value);
	static int get_value_as_uint(std::string& file_name, unsigned int & value);
	static int get_value_as_long(std::string& file_name, long & value);
	static int get_value_as_ulong(std::string& file_name, unsigned long & value);

	const static std::string ipcp_prefix;
	const static std::string connections;
	const static std::string separator;
	const static std::string rmt_ports;

	static int get_dtp_tx_bytes(int ipcp_id,
				    int src_cep_id,
				    unsigned long & value);

	static int get_dtp_rx_bytes(int ipcp_id,
				    int src_cep_id,
				    unsigned long & value);

	static int get_dtp_tx_pdus(int ipcp_id,
				   int src_cep_id,
				   unsigned int & value);

	static int get_dtp_rx_pdus(int ipcp_id,
				   int src_cep_id,
				   unsigned int & value);

	static int get_dtp_drop_pdus(int ipcp_id,
				     int src_cep_id,
				     unsigned int & value);

	static int get_dtp_error_pdus(int ipcp_id,
				      int src_cep_id,
				      unsigned int & value);

	static int get_rmt_queued_pdus(int ipcp_id,
				       int port_id,
				       unsigned int & value);

	static int get_rmt_drop_pdus(int ipcp_id,
				     int port_id,
				     unsigned int & value);

	static int get_rmt_error_pdus(int ipcp_id,
				      int port_id,
				      unsigned int & value);

	static int get_rmt_tx_pdus(int ipcp_id,
				   int port_id,
				   unsigned int & value);

	static int get_rmt_rx_pdus(int ipcp_id,
				   int port_id,
				   unsigned int & value);

	static int get_rmt_tx_bytes(int ipcp_id,
				    int port_id,
				    unsigned long & value);

	static int get_rmt_rx_bytes(int ipcp_id,
				    int port_id,
				    unsigned long & value);
};

} //namespace rinad

#endif //IPCP_UTILS_HH
