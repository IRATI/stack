/* c-basic-offset: 8; tab-width: 8; indent-tabs-mode: nil
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <epan/packet.h>
#include <epan/wmem/wmem.h>

static const value_string opcodes[] = {
        { 0, "M_CONNECT" },
        { 1, "M_CONNECT_R" }, 
        { 2, "M_RELEASE" }, 
        { 3, "M_RELEASE_R" }, 
        { 4, "M_CREATE" }, 
        { 5, "M_CREATE_R" }, 
        { 6, "M_DELETE" }, 
        { 7, "M_DELETE_R" }, 
        { 8, "M_READ" }, 
        { 9, "M_READ_R" }, 
        { 10, "M_CANCELREAD" }, 
        { 11, "M_CANCELREAD_R" }, 
        { 12, "M_WRITE" }, 
        { 13, "M_WRITE_R" }, 
        { 14, "M_START" }, 
        { 15, "M_START_R" }, 
        { 16, "M_STOP" }, 
        { 17, "M_STOP_R" } 
};

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

static size_t 
cdap_get_length_varint(size_t len, const guint8 *data)
{
        size_t i;
        
        for (i = 0; i < len; i++) {
                if ((data[i] & 0x80) == 0)
                        break;
        }

        return i+1;
}

/* Caller needs to free the return value */
static guint8 *
cdap_parse_int32(size_t len, const guint8 *data)
{
        guint8 *rv;
        guint i;

        rv = (guint8 *) wmem_alloc0(wmem_packet_scope(), 32);
        for (i = 0; i < len; i++) {
                rv[i] = (data[i] & 0x7f);
        }

        return rv;
}

/* Caller needs to free the return value */
static guint8 *
cdap_parse_int64(size_t len, const guint8 *data)
{
        guint8 *rv;
        guint i;

        rv = (guint8 *) wmem_alloc0(wmem_packet_scope(), 64);
        for (i = 0; i < len; i++) {
                rv[i] = (data[i] & 0x7f);
        }

        return rv;
}


static void
dissect_cdap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "CDAP");
        if (tree) { /* we are being asked for details */
                proto_item *ti = NULL;
                proto_tree *cdap_tree = NULL;
                guint buff_length;
                guint offset;
                guint8 protofield;
                gboolean stop;
                /* Contains either the string length, or the varint length */
                volatile size_t len;
                guint8 *buf;
                guint8 *buf2;
                tvbuff_t *new_tvb;
                guint i;
                guint8 *str;

                ti = proto_tree_add_item(tree, proto_cdap, tvb, 0, -1, ENC_NA);
                cdap_tree = proto_item_add_subtree(ti, ett_cdap);

                /* Add them to the tree here */
                offset = 0;
                buff_length = tvb_length(tvb);
                stop = FALSE;
                protofield = 0;
                len = 0;
                i = 0;
                new_tvb = NULL;
                str = NULL;
                buf = (guint8 *) wmem_alloc0(wmem_packet_scope(), 8);
                while (!stop && offset < buff_length) {
                        /*
                         * Each key in the streamed message is a varint with 
                         * the value (field_number << 3) | wire_type – in 
                         * other words, the last three bits of the number 
                         * store the wire type. 
                         */
                        TRY {
                                protofield = tvb_get_guint8(tvb, offset);
                                offset++;
                                protofield = protofield >> 3;

                                /* Check if the MSB is 1, in which case we need the second byte too */
                                if (protofield & 0x10) {
                                        protofield = (tvb_get_guint8(tvb, offset) << 4) | protofield;
                                        offset++;
                                }

                                /* String */
                                if ((protofield >= 19 && protofield <= 27) ||
                                    (protofield >= 5 && protofield <= 6)) {  
                                        for (i = 0; i < 4; i++) { 
                                                /* TODO: Check if we don't go beyond the packet length */
                                                buf[i] = tvb_get_guint8(tvb, offset+i);
                                        }
                                        len = cdap_get_length_varint(4, buf);
                                        offset += len;

                                        buf2 = cdap_parse_int32(len, buf);
                                        len = (buf2[3] << 24) | (buf2[2] << 16) | (buf2[1] << 8) | buf2[0]; 
                                        
                                        str = (guint8 *) wmem_alloc0(wmem_packet_scope(), len+1);
                                        str[len]= '\0';

                                        for (i = 0; i < len; i++) {
                                                str[i] = tvb_get_guint8(tvb, offset+i);
                                        }
                                }
                                /* Int 32 */        
                                else if (protofield <= 4 ||
                                         protofield == 9 ||
                                         protofield == 10 ||
                                         protofield == 17) {
                                        /* Get data from tvbuff */
                                        /* TODO: Use tvb_memcpy or tvp_ptr */
                                        for (i = 0; i < 4; i++) { 
                                                /* TODO: Check if we don't go beyond the packet length */
                                                buf[i] = tvb_get_guint8(tvb, offset+i);
                                        }
                                        len = cdap_get_length_varint(4, buf);
                                        buf2 = cdap_parse_int32(len, buf);
                                        offset+= len;

                                        new_tvb = tvb_new_child_real_data(tvb, buf2, 4, 4);
                                        add_new_data_source(pinfo, new_tvb, "Decoded Int32");
                                } 
                                /* Int 64 */
                                else if (protofield == 7 ||
                                         protofield == 28) {
                                        /* Get data from tvbuff */
                                        /* TODO: Use tvb_memcpy */
                                        for (i = 0; i < 8; i++) { 
                                                /* TODO: Check if we don't go beyond the packet length */
                                                buf[i] = tvb_get_guint8(tvb, offset+i);
                                        }
                                        len = cdap_get_length_varint(8, buf);
                                        buf2 = cdap_parse_int64(len, buf);
                                        new_tvb = tvb_new_child_real_data(tvb, buf2, 8, 8);
                                        add_new_data_source(pinfo, new_tvb, "Decoded Int64");
                                        offset+= len;
                                } 
                                /* Message in message or bytes */
                                else if (protofield == 8 ||
                                         protofield == 11 ||
                                         protofield == 18) {
                                        for (i = 0; i < 4; i++) { 
                                                /* TODO: Check if we don't go beyond the packet length */
                                                buf[i] = tvb_get_guint8(tvb, offset+i);
                                        }
                                        len = cdap_get_length_varint(4, buf);
                                        offset += len;

                                        buf2 = cdap_parse_int32(len, buf);
                                        len = (buf2[3] << 24) | (buf2[2] << 16) | (buf2[1] << 8) | buf2[0]; 
                                        offset += len;
                                }
                        }
                        CATCH_ALL {
                                stop = TRUE;
                                break;
                        }
                        ENDTRY;

                        printf("Protofield: %d\n", protofield);
                       
                        /* Processing based on field */
                        switch (protofield) {
                        case 1:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_abs_syntax, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 2:
                                /* Enum are also gint32 */
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_opcode, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 3:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_invokeid, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 4:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_flags, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 5:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_objclass, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;                       
                                break;
                        case 6:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_objname, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 7:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_objinst, 
                                                    new_tvb, 0, 8, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 8:
                                /* For now just increase the offset */
                                break;
                        case 9:  
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_result, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 10:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_scope, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 11:
                                /* For now just increase the offset */
                                break; 
                        case 17:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_authmech, 
                                                    new_tvb, 0, 4, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        case 18:
                                /* For now just increase the offset */
                                break;
                        case 19:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_dstaeinst, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 20:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_dstaename, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 21:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_dstapinst, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 22:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_dstapname, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 23:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_srcaeinst, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 24:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_srcaename, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 25:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_srcapinst, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 26: 
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_srcapname, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 27:
                                proto_tree_add_string(cdap_tree, 
                                                      hf_cdap_resultreason, 
                                                      tvb, offset, len, 
                                                      str);
                                offset+= len;
                                break;
                        case 28:
                                proto_tree_add_item(cdap_tree, 
                                                    hf_cdap_version, 
                                                    new_tvb, 0, 8, 
                                                    ENC_LITTLE_ENDIAN);
                                break;
                        default:
                                stop = TRUE;
                                printf("CDAP got an unknown field, stopping\n");
                                printf("Field: %d\n\n", protofield);
                                break;
                        }

                         
                }
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
                VALS(opcodes), 0x0,
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
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objname,
              { "Object name", "cdap.objname",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_objinst,
              { "Object instance", "cdap.objinst",
                FT_INT64, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            /* TODO: Can be any value, check best type */
            { &hf_cdap_objvalue,
              { "Object value", "cdap.objvalue",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_result,
              { "Result of operation", "cdap.result",
                FT_INT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_scope,
              { "Scope of read/write operation", "cdap.scope",
                FT_INT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_filter,
              { "Filter script", "cdap.filter",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_authmech,
              { "Authentication mechanism", "cdap.authmech",
                FT_UINT32, BASE_DEC,
                NULL, 0x0,
                NULL, HFILL }
            },
            /* Can be a lot of values */
            { &hf_cdap_authvalue,
              { "Authentication information", "cdap.authvalue",
                FT_BYTES, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstaeinst,
              { "Destination application entity instance name", "cdap.dstaeinst",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstaename,
              { "Destination application entity name", "cdap.dstaename",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstapinst,
              { "Destination application instance name", "cdap.dstapinst",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_dstapname,
              { "Destination Application Name", "cdap.dstapname",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcaeinst,
              { "Source application entity instance name", "cdap.srcaeinst",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcaename,
              { "Source application entity name", "cdap.srcaename",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcapinst,
              { "Source application instance name", "cdap.srcapinst",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_srcapname,
              { "Source Application Name", "cdap.srcapname",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_resultreason,
              { "Explanation of result", "cdap.resultreason",
                FT_STRING, BASE_NONE,
                NULL, 0x0,
                NULL, HFILL }
            },
            { &hf_cdap_version,
              { "RIB/class version", "cdap.version",
                FT_INT64, BASE_DEC,
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

    register_dissector("cdap", dissect_cdap, proto_cdap);
}

void
proto_reg_handoff_cdap(void)
{}

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
