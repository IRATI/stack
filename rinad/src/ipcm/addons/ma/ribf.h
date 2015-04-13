/*
 * RIB factory
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#ifndef __RINAD_RIBM_H__
#define __RINAD_RIBM_H__

#include <list>
#include <librina/concurrency.h>
#include <inttypes.h>
#include <librina/rib_v2.h>

namespace rinad{
namespace mad{

/**
 * @file ribf.h
 * @author Marc Sune<marc.sune (at) bisdn.de>
 *
 * @brief RIB Factory
 */

//
// RIB factory exceptions
//

/**
 * Duplicated RIB exception
 */
DECLARE_EXCEPTION_SUBCLASS(eDuplicatedRIB);

/**
 * RIB not found exception
 */
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

/**
 * @brief RIB manager
 */
class RIBFactory :public rina::Lockable {

	public:
		//Constructors
		RIBFactory(std::list<uint64_t> supported_versions);
		virtual ~RIBFactory(void) throw();

		/**
		 * Get a reference to a RIB
		 * @throws eRIBNotFound
		 */
		rina::rib::RIBDNorthInterface& getRIB(uint64_t version);

#if 0
		//TODO: if ever necessary

		/**
		 * @brief Destroy a RIB instance
		 *
		 * This call will stop all communications that are using this RIB
		 * instance
		 */
		void destroyRIB(uint64_t version);
#endif

	protected:
		/**
		 * Create a RIB instance
		 * @throwseDuplicatedRIB
		 */
		void createRIB(uint64_t version);

	private:

		//Map with the current RIB instances
		std::map<uint64_t, rina::rib::RIBDNorthInterface*> rib_inst_;
		rina::rib::RIBDFactory factory_;

		/*
		 * Internal methods
		 */
		//TODO
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */
