/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "kfa"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "fidm.h"
#include "kfa.h"

struct kfa {
        struct fidm * fidm;
};

struct kfa * kfa_create(void)
{
        struct kfa * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->fidm = fidm_create();
        if (!tmp->fidm) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

int kfa_destroy(struct kfa * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        fidm_destroy(instance->fidm);
        rkfree(instance);

        return 0;
}

struct ipcp_flow {
        struct efcp * efcp;

        /* FIXME: Move KIPCM struct ipcp_flow here */
};

flow_id_t kfa_flow_create(struct kfa * instance)
{ return flow_id_bad();
}

int kfa_flow_bind(struct kfa * instance,
                  flow_id_t    fid,
                  port_id_t    pid)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_flow_id_ok(fid)) {
                LOG_ERR("Bogus flow-id, bailing out");
                return -1;
        }
        if (!is_port_id_ok(fid)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        LOG_MISSING;

        return -1;
}

flow_id_t kfa_flow_unbind(struct kfa * instance,
                          port_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        return flow_id_bad();
}

int kfa_flow_destroy(struct kfa * instance,
                     flow_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_flow_id_ok(id)) {
                LOG_ERR("Bogus flow-id, bailing out");
                return -1;
        }

        LOG_MISSING;

        return -1;
}

int kfa_flow_sdu_post_by_port(struct kfa *  instance,
                              port_id_t     id,
                              struct sdu *  sdu)
{
       if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        LOG_MISSING;

        return -1;
}


struct sdu * kfa_flow_sdu_read(struct kfa *  instance,
                               port_id_t     id)
{
       if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return NULL;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return NULL;
        }

        LOG_MISSING;

        return NULL;
}
