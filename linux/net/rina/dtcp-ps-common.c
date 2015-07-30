/*
 * Common implementation of a default policy set for DTCP
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>

#define RINA_PREFIX "dtcp-ps-common"

#include "logs.h"
#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "dtp.h"
#include "dtcp.h"
#include "dt-utils.h"
#include "debug.h"

int
common_lost_control_pdu(struct dtcp_ps * ps)
{
        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

#if 0
        struct pdu * pdu_ctrl;
        seq_num_t last_rcv_ctrl, snd_lft, snd_rt;

        last_rcv_ctrl = last_rcv_ctrl_seq(dtcp);
        snd_lft       = snd_lft_win(dtcp);
        snd_rt        = snd_rt_wind_edge(dtcp);
        pdu_ctrl      = pdu_ctrl_ack_create(dtcp,
                                            last_rcv_ctrl,
                                            snd_lft,
                                            snd_rt);
        if (!pdu_ctrl) {
                LOG_ERR("Failed Lost Control PDU policy");
                return -1;
        }

        if (dtcp_pdu_send(dtcp, pdu_ctrl))
                return -1;
#endif

        LOG_DBG("Default lost control pdu policy");

        return 0;
}
EXPORT_SYMBOL(common_lost_control_pdu);

int common_rcvr_ack(struct dtcp_ps * instance, const struct pci * pci)
{
        struct dtcp * dtcp = instance->dm;
        seq_num_t     seq;

        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }
        seq = pci_sequence_number_get(pci);

        return dtcp_ack_flow_control_pdu_send(dtcp, seq);
}
EXPORT_SYMBOL(common_rcvr_ack);

int common_rcvr_ack_atimer(struct dtcp_ps * instance, const struct pci * pci)
{ return 0; }
EXPORT_SYMBOL(common_rcvr_ack_atimer);

int
common_sender_ack(struct dtcp_ps * ps, seq_num_t seq_num)
{
        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        if (ps->rtx_ctrl) {
                struct rtxq * q;

                q = dt_rtxq(dtcp_dt(dtcp));
                if (!q) {
                        LOG_ERR("Couldn't find the Retransmission queue");
                        return -1;
                }
                rtxq_ack(q, seq_num, dt_sv_tr(dtcp_dt(dtcp)));
        }

        return 0;
}
EXPORT_SYMBOL(common_sender_ack);

int
common_sending_ack(struct dtcp_ps * ps, seq_num_t seq)
{
        struct dtp *       dtp;
        const struct pci * pci;

        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        dtp = dt_dtp(dtcp_dt(dtcp));
        if (!dtp) {
                LOG_ERR("No DTP from dtcp->parent");
                return -1;
        }

        /* Invoke delimiting and update left window edge */

        pci = process_A_expiration(dtp, dtcp);

        return dtcp_sv_update(ps->dm, pci);
}
EXPORT_SYMBOL(common_sending_ack);

int
common_receiving_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        struct pdu * pdu;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }
        pdu = pdu_ctrl_generate(dtcp, PDU_TYPE_FC);
        if (!pdu)
                return -1;

        LOG_DBG("DTCP Sending FC (CPU: %d)", smp_processor_id());
        dump_we(dtcp, pdu_pci_get_rw(pdu));

        if (dtcp_pdu_send(dtcp, pdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(common_receiving_flow_control);

int
common_rcvr_flow_control(struct dtcp_ps * ps, const struct pci * pci)
{
        struct dtcp * dtcp = ps->dm;
        seq_num_t LWE;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }
        if (!pci) {
                LOG_ERR("No PCI passed, cannot run policy");
                return -1;
        }

        LWE = dt_sv_rcv_lft_win(dtcp_dt(dtcp));
        update_rt_wind_edge(dtcp);

        LOG_DBG("DTCP: %pK", dtcp);
        LOG_DBG("LWE: %u  RWE: %u", LWE, rcvr_rt_wind_edge(dtcp));

        return 0;
}
EXPORT_SYMBOL(common_rcvr_flow_control);

int
common_rate_reduction(struct dtcp_ps * ps)
{
        struct dtcp * dtcp = ps->dm;

        if (!dtcp) {
                LOG_ERR("No instance passed, cannot run policy");
                return -1;
        }

        LOG_MISSING;

        return 0;
}
EXPORT_SYMBOL(common_rate_reduction);

int common_rtt_estimator(struct dtcp_ps * ps, seq_num_t sn)
{
        struct dtcp *       dtcp;
        struct dt *         dt;
        uint_t              rtt, new_rtt, srtt, rttvar, trmsecs;
        timeout_t           start_time;
        int                 abs;
        struct rtxq_entry * entry;

        if (!ps)
                return -1;
        dtcp = ps->dm;
        if (!dtcp)
                return -1;
        dt = dtcp_dt(dtcp);
        if (!dt)
                return -1;

        LOG_DBG("RTT Estimator...");

        entry = rtxq_entry_peek(dt_rtxq(dt), sn);
        if (!entry) {
                LOG_ERR("Could not retrieve timestamp of Seq num: %u for RTT "
                        "estimation", sn);
                return -1;
        }

        /* if it is a retransmission we do not consider it*/
        if (rtxq_entry_retries(entry) != 0) {
                LOG_DBG("RTTestimator PDU %u has been retransmitted %u",
                        sn, rtxq_entry_retries(entry));
                rtxq_entry_destroy(entry);
                return 0;
        }

        start_time = rtxq_entry_timestamp(entry);
        new_rtt    = jiffies_to_msecs(jiffies - start_time);

        /* NOTE: the acking process has alrady deleted old entries from rtxq
         * except for the one with the sn we need, here we have to detroy just
         * the one we use */
        rtxq_entry_destroy(entry);

        rtt        = dtcp_rtt(dtcp);
        srtt       = dtcp_srtt(dtcp);
        rttvar     = dtcp_rttvar(dtcp);

        if (!rtt) {
                rttvar = new_rtt >> 1;
                srtt   = new_rtt;
        } else {
                abs = srtt - new_rtt;
                abs = abs < 0 ? -abs : abs;
                rttvar = ((3 * rttvar) >> 2) + (((uint_t)abs) >> 2);
        }

        /*FIXME: k, G, alpha and betha should be parameters passed to the policy
         * set. Probably moving them to ps->priv */

        /* k * rttvar = 4 * rttvar */
        trmsecs  = rttvar << 2;
        /* G is 0.1s according to RFC6298, then 100ms */
        trmsecs  = 100 > trmsecs ? 100 : trmsecs;
        trmsecs += srtt + jiffies_to_msecs(dt_sv_a(dt));
        /* RTO (tr) less than 1s? (not for the common policy) */
        /*trmsecs  = trmsecs < 1000 ? 1000 : trmsecs;*/

        dtcp_rtt_set(dtcp, new_rtt);
        dtcp_rttvar_set(dtcp, rttvar);
        dtcp_srtt_set(dtcp, srtt);
        dt_sv_tr_set(dt, msecs_to_jiffies(trmsecs));
        LOG_DBG("TR set to %u msecs", trmsecs);

        return 0;
}
EXPORT_SYMBOL(common_rtt_estimator);

int dtcp_ps_common_set_policy_set_param(struct ps_base * bps,
                                        const char    * name,
                                        const char    * value)
{
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);

        (void) ps;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}
EXPORT_SYMBOL(dtcp_ps_common_set_policy_set_param);
