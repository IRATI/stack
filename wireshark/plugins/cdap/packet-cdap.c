#include "config.h"

#include <epan/packet.h>

static int proto_cdap = -1;

static int hf_cdap_abs_syntax   = -1;
static int hf_cdap_opcode       = -1;
static int hf_cdap_invokeid     = -1;
static int hf_cdap_flags        = -1;
static int hf_cdap_objclass     = -1;
static int hf_cdap_objname      = -1;
static int hf_cdap_objinst      = -1;
static int hf_cdap_objvalue     = -1;
static int hf_cdap_result       = -1;
static int hf_cdap_scope        = -1;
static int hf_cdap_filter       = -1;
static int hf_cdap_authmech     = -1;
static int hf_cdap_authvalue    = -1;
static int hf_cdap_dstaeinst    = -1;
static int hf_cdap_dstaename    = -1;
static int hf_cdap_dstapinst    = -1;
static int hf_cdap_dstapname    = -1;
static int hf_cdap_srcaeinst    = -1;
static int hf_cdap_srcaename    = -1;
static int hf_cdap_srcapinst    = -1;
static int hf_cdap_srcapname    = -1;
static int hf_cdap_resultreason = -1;
static int hf_cdap_version      = -1;

static gint ett_cdap = -1;

static void
dissect_cdap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "CDAP");
        /* Clear out stuff in the info column */
        col_clear(pinfo->cinfo,COL_INFO);

        if (tree) { /* we are being asked for details */
                proto_item *ti = NULL;
                proto_tree *cdap_tree = NULL;

                ti = proto_tree_add_item(tree, proto_cdap, tvb, 0, -1, ENC_NA);
                cdap_tree = proto_item_add_subtree(ti, ett_cdap);

                /* Add them to the tree here */
                proto_tree_add_item(cdap_tree, hf_cdap_abs_syntax, tvb, 0, 4, ENC_LITTLE_ENDIAN); 
                     
        }
}

void
proto_register_cdap(void)
{
    static hf_register_info hf[] = {
            { &hf_cdap_abs_syntax,
              { "Abstract syntax", "cdap.abssyntax",
                FT_INT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_opcode,
              { "Opcode", "cdap.opcode",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_invokeid,
              { "Invoke ID", "cdap.invokeid",
                FT_INT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_flags,
              { "Flags", "cdap.flags",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objclass,
              { "Object class", "cdap.objclass",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objname,
              { "Object name", "cdap.objname",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objinst,
              { "Object instance", "cdap.objinst",
                FT_INT64, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objvalue,
              { "Object value", "cdap.objvalue",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_result,
              { "Result of operation", "cdap.result",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_scope,
              { "Scope of read/write operation", "cdap.scope",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_filter,
              { "Filter script", "cdap.filter",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_authmech,
              { "Authentication mechanism", "cdap.authmech",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_authvalue,
              { "Authentication information", "cdap.authvalue",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstaeinst,
              { "Destination application entity instance name", "cdap.dstaeinst",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstaename,
              { "Destination application entity name", "cdap.dstaename",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstapinst,
              { "Destination application instance name", "cdap.dstapinst",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstapname,
              { "Destination Application Name", "cdap.dstapname",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcaeinst,
              { "Source application entity instance name", "cdap.srcaeinst",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcaename,
              { "Source application entity name", "cdap.srcaename",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcapinst,
              { "Source application instance name", "cdap.srcapinst",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcapname,
              { "Source Application Name", "cdap.srcapname",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_resultreason,
              { "Explanation of result", "cdap.resultreason",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_version,
              { "RIB/class version", "cdap.version",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_cdap
    };

    proto_cdap = proto_register_protocol (
        "Common Distributed Application Protocol",   /* name       */
        "CDAP",                                      /* short name */
        "cdap"                                       /* abbrev     */
        );

    proto_register_field_array(proto_cdap, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_cdap(void)
{
    static dissector_handle_t cdap_handle;

    cdap_handle = create_dissector_handle(dissect_cdap, proto_cdap);

    /* TODO: Hook into management PDU here */
}
