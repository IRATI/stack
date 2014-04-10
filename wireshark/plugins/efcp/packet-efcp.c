/* c-basic-offset: 8; tab-width: 8; indent-tabs-mode: nil
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
#include "config.h"

#include <epan/packet.h>

#define ETH_P_RINA 0xD1F0

static int proto_rina = -1;

static int hf_rina_pdu_type     = -1;
static int hf_rina_dst_addr     = -1;
static int hf_rina_src_addr     = -1;
static int hf_rina_dst_cep      = -1;
static int hf_rina_src_cep      = -1;
static int hf_rina_qos_id       = -1;
static int hf_rina_pdu_flags    = -1;
static int hf_rina_seq_num      = -1;
static int hf_rina_ack_nack_seq = -1;
static int hf_rina_new_rt_win   = -1;
static int hf_rina_new_lt_win   = -1;
static int hf_rina_lt_win       = -1;
static int hf_rina_rt_win       = -1;
static int hf_rina_ctrl_seq     = -1;

static gint ett_rina = -1;

static const value_string pdutypenames[] = {
        { 0x8001, "Data transfer" },
        { 0xC000, "Management" },
        { 0, NULL }
};

static void
dissect_rina(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
        gint offset = 0;
        guint pdu_type = 0;

        pdu_type = tvb_get_letoh24(tvb, 0);

        col_set_str(pinfo->cinfo, COL_PROTOCOL, "EFCP");
        /* Clear out stuff in the info column */
        col_clear(pinfo->cinfo,COL_INFO);
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s PDU",
                     val_to_str(pdu_type, pdutypenames, "Unknown (0x%02x)"));

        if (tree) { /* we are being asked for details */
                proto_item *ti = NULL;
                proto_tree *rina_tree = NULL;

                ti = proto_tree_add_item(tree, proto_rina, tvb, 0, 56, ENC_NA);
                proto_item_append_text(ti, ", %s PDU",
                                       val_to_str(pdu_type, 
                                                  pdutypenames, 
                                                  "Unknown (0x%02x)"));
                rina_tree = proto_item_add_subtree(ti, ett_rina);
                proto_tree_add_item(rina_tree, hf_rina_pdu_type, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_dst_addr, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_src_addr, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_dst_cep, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_src_cep, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_qos_id, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_pdu_flags, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_seq_num, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_ack_nack_seq, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_new_rt_win, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_new_lt_win, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_lt_win, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_rt_win, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                proto_tree_add_item(rina_tree, hf_rina_ctrl_seq, 
                                    tvb, offset, 4, ENC_LITTLE_ENDIAN);
        }
}

void
proto_register_rina(void)
{
        static hf_register_info hf[] = {
                { &hf_rina_pdu_type,
                  { "PDU Type", "rina.pdutype",
                    FT_UINT32, BASE_HEX,
                    VALS(pdutypenames), 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_dst_addr,
                  { "Destination address", "rina.dest",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_src_addr,
                  { "Source address", "rina.src",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_dst_cep,
                  { "Destination Connection Endpoint ID", "rina.dstcep",
                    FT_INT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_src_cep,
                  { "Source Connection Endpoint ID", "rina.srccep",
                    FT_INT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_qos_id,
                  { "Quality of Service ID", "rina.qosid",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_pdu_flags,
                  { "PDU Flags", "rina.pduflags",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_seq_num,
                  { "Sequence number", "rina.seq",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_ack_nack_seq,
                  { "ACK/NACK sequence number", "rina.acknackseq",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL } 
                },
                { &hf_rina_new_rt_win,
                  { "New right window edge", "rina.newrtwin",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_new_lt_win,
                  { "New left window edge", "rina.newltwin",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_lt_win,
                  { "Left window edge", "rina.ltwin",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_rt_win,
                  { "Right window edge", "rina.rtwin",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                },
                { &hf_rina_ctrl_seq,
                  { "Last control sequence received", "rina.ctrlseq",
                    FT_UINT32, BASE_DEC,
                    NULL, 0x0,
                    NULL, HFILL }
                }
        };

        /* Setup protocol subtree array */
        static gint *ett[] = {
                &ett_rina
        };

        proto_rina = proto_register_protocol (
                                              "Error and Flow Control Protocol",     /* name       */
                                              "EFCP",                                /* short name */
                                              "efcp"                                 /* abbrev     */
                                              );

        proto_register_field_array(proto_rina, hf, array_length(hf));
        proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_rina(void)
{
        static dissector_handle_t rina_handle;

        rina_handle = create_dissector_handle(dissect_rina, proto_rina);

        dissector_add_uint("ethertype", ETH_P_RINA, rina_handle);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */

