/*
 * Do not modify this file. Changes will be overwritten.
 *
 * Generated automatically from ../../tools/make-dissector-reg.py.
 */

#include "config.h"

#include <gmodule.h>

#include "moduleinfo.h"

/* plugins are DLLs */
#define WS_BUILD_DLL
#include "ws_symbol_export.h"

#ifndef ENABLE_STATIC
WS_DLL_PUBLIC_NOEXTERN const gchar version[] = VERSION;

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_NOEXTERN void
plugin_register (void)
{
  {extern void proto_register_cmctrl_tlv (void); proto_register_cmctrl_tlv ();}
  {extern void proto_register_docsis (void); proto_register_docsis ();}
  {extern void proto_register_docsis_bintrngreq (void); proto_register_docsis_bintrngreq ();}
  {extern void proto_register_docsis_bpkmattr (void); proto_register_docsis_bpkmattr ();}
  {extern void proto_register_docsis_bpkmreq (void); proto_register_docsis_bpkmreq ();}
  {extern void proto_register_docsis_bpkmrsp (void); proto_register_docsis_bpkmrsp ();}
  {extern void proto_register_docsis_cmctrlreq (void); proto_register_docsis_cmctrlreq ();}
  {extern void proto_register_docsis_cmctrlrsp (void); proto_register_docsis_cmctrlrsp ();}
  {extern void proto_register_docsis_cmstatus (void); proto_register_docsis_cmstatus ();}
  {extern void proto_register_docsis_dbcack (void); proto_register_docsis_dbcack ();}
  {extern void proto_register_docsis_dbcreq (void); proto_register_docsis_dbcreq ();}
  {extern void proto_register_docsis_dbcrsp (void); proto_register_docsis_dbcrsp ();}
  {extern void proto_register_docsis_dccack (void); proto_register_docsis_dccack ();}
  {extern void proto_register_docsis_dccreq (void); proto_register_docsis_dccreq ();}
  {extern void proto_register_docsis_dccrsp (void); proto_register_docsis_dccrsp ();}
  {extern void proto_register_docsis_dcd (void); proto_register_docsis_dcd ();}
  {extern void proto_register_docsis_dpvreq (void); proto_register_docsis_dpvreq ();}
  {extern void proto_register_docsis_dpvrsp (void); proto_register_docsis_dpvrsp ();}
  {extern void proto_register_docsis_dsaack (void); proto_register_docsis_dsaack ();}
  {extern void proto_register_docsis_dsareq (void); proto_register_docsis_dsareq ();}
  {extern void proto_register_docsis_dsarsp (void); proto_register_docsis_dsarsp ();}
  {extern void proto_register_docsis_dscack (void); proto_register_docsis_dscack ();}
  {extern void proto_register_docsis_dscreq (void); proto_register_docsis_dscreq ();}
  {extern void proto_register_docsis_dscrsp (void); proto_register_docsis_dscrsp ();}
  {extern void proto_register_docsis_dsdreq (void); proto_register_docsis_dsdreq ();}
  {extern void proto_register_docsis_dsdrsp (void); proto_register_docsis_dsdrsp ();}
  {extern void proto_register_docsis_intrngreq (void); proto_register_docsis_intrngreq ();}
  {extern void proto_register_docsis_map (void); proto_register_docsis_map ();}
  {extern void proto_register_docsis_mdd (void); proto_register_docsis_mdd ();}
  {extern void proto_register_docsis_mgmt (void); proto_register_docsis_mgmt ();}
  {extern void proto_register_docsis_regack (void); proto_register_docsis_regack ();}
  {extern void proto_register_docsis_regreq (void); proto_register_docsis_regreq ();}
  {extern void proto_register_docsis_regreqmp (void); proto_register_docsis_regreqmp ();}
  {extern void proto_register_docsis_regrsp (void); proto_register_docsis_regrsp ();}
  {extern void proto_register_docsis_regrspmp (void); proto_register_docsis_regrspmp ();}
  {extern void proto_register_docsis_rngreq (void); proto_register_docsis_rngreq ();}
  {extern void proto_register_docsis_rngrsp (void); proto_register_docsis_rngrsp ();}
  {extern void proto_register_docsis_sync (void); proto_register_docsis_sync ();}
  {extern void proto_register_docsis_tlv (void); proto_register_docsis_tlv ();}
  {extern void proto_register_docsis_type29ucd (void); proto_register_docsis_type29ucd ();}
  {extern void proto_register_docsis_uccreq (void); proto_register_docsis_uccreq ();}
  {extern void proto_register_docsis_uccrsp (void); proto_register_docsis_uccrsp ();}
  {extern void proto_register_docsis_ucd (void); proto_register_docsis_ucd ();}
  {extern void proto_register_docsis_vsif (void); proto_register_docsis_vsif ();}
}

WS_DLL_PUBLIC_NOEXTERN void
plugin_reg_handoff(void)
{
  {extern void proto_reg_handoff_cmctrl_tlv (void); proto_reg_handoff_cmctrl_tlv ();}
  {extern void proto_reg_handoff_docsis (void); proto_reg_handoff_docsis ();}
  {extern void proto_reg_handoff_docsis_bintrngreq (void); proto_reg_handoff_docsis_bintrngreq ();}
  {extern void proto_reg_handoff_docsis_bpkmattr (void); proto_reg_handoff_docsis_bpkmattr ();}
  {extern void proto_reg_handoff_docsis_bpkmreq (void); proto_reg_handoff_docsis_bpkmreq ();}
  {extern void proto_reg_handoff_docsis_bpkmrsp (void); proto_reg_handoff_docsis_bpkmrsp ();}
  {extern void proto_reg_handoff_docsis_cmctrlreq (void); proto_reg_handoff_docsis_cmctrlreq ();}
  {extern void proto_reg_handoff_docsis_cmctrlrsp (void); proto_reg_handoff_docsis_cmctrlrsp ();}
  {extern void proto_reg_handoff_docsis_cmstatus (void); proto_reg_handoff_docsis_cmstatus ();}
  {extern void proto_reg_handoff_docsis_dbcack (void); proto_reg_handoff_docsis_dbcack ();}
  {extern void proto_reg_handoff_docsis_dbcreq (void); proto_reg_handoff_docsis_dbcreq ();}
  {extern void proto_reg_handoff_docsis_dbcrsp (void); proto_reg_handoff_docsis_dbcrsp ();}
  {extern void proto_reg_handoff_docsis_dccack (void); proto_reg_handoff_docsis_dccack ();}
  {extern void proto_reg_handoff_docsis_dccreq (void); proto_reg_handoff_docsis_dccreq ();}
  {extern void proto_reg_handoff_docsis_dccrsp (void); proto_reg_handoff_docsis_dccrsp ();}
  {extern void proto_reg_handoff_docsis_dcd (void); proto_reg_handoff_docsis_dcd ();}
  {extern void proto_reg_handoff_docsis_dpvreq (void); proto_reg_handoff_docsis_dpvreq ();}
  {extern void proto_reg_handoff_docsis_dpvrsp (void); proto_reg_handoff_docsis_dpvrsp ();}
  {extern void proto_reg_handoff_docsis_dsaack (void); proto_reg_handoff_docsis_dsaack ();}
  {extern void proto_reg_handoff_docsis_dsareq (void); proto_reg_handoff_docsis_dsareq ();}
  {extern void proto_reg_handoff_docsis_dsarsp (void); proto_reg_handoff_docsis_dsarsp ();}
  {extern void proto_reg_handoff_docsis_dscack (void); proto_reg_handoff_docsis_dscack ();}
  {extern void proto_reg_handoff_docsis_dscreq (void); proto_reg_handoff_docsis_dscreq ();}
  {extern void proto_reg_handoff_docsis_dscrsp (void); proto_reg_handoff_docsis_dscrsp ();}
  {extern void proto_reg_handoff_docsis_dsdreq (void); proto_reg_handoff_docsis_dsdreq ();}
  {extern void proto_reg_handoff_docsis_dsdrsp (void); proto_reg_handoff_docsis_dsdrsp ();}
  {extern void proto_reg_handoff_docsis_intrngreq (void); proto_reg_handoff_docsis_intrngreq ();}
  {extern void proto_reg_handoff_docsis_map (void); proto_reg_handoff_docsis_map ();}
  {extern void proto_reg_handoff_docsis_mdd (void); proto_reg_handoff_docsis_mdd ();}
  {extern void proto_reg_handoff_docsis_mgmt (void); proto_reg_handoff_docsis_mgmt ();}
  {extern void proto_reg_handoff_docsis_regack (void); proto_reg_handoff_docsis_regack ();}
  {extern void proto_reg_handoff_docsis_regreq (void); proto_reg_handoff_docsis_regreq ();}
  {extern void proto_reg_handoff_docsis_regreqmp (void); proto_reg_handoff_docsis_regreqmp ();}
  {extern void proto_reg_handoff_docsis_regrsp (void); proto_reg_handoff_docsis_regrsp ();}
  {extern void proto_reg_handoff_docsis_regrspmp (void); proto_reg_handoff_docsis_regrspmp ();}
  {extern void proto_reg_handoff_docsis_rngreq (void); proto_reg_handoff_docsis_rngreq ();}
  {extern void proto_reg_handoff_docsis_rngrsp (void); proto_reg_handoff_docsis_rngrsp ();}
  {extern void proto_reg_handoff_docsis_sync (void); proto_reg_handoff_docsis_sync ();}
  {extern void proto_reg_handoff_docsis_tlv (void); proto_reg_handoff_docsis_tlv ();}
  {extern void proto_reg_handoff_docsis_type29ucd (void); proto_reg_handoff_docsis_type29ucd ();}
  {extern void proto_reg_handoff_docsis_uccreq (void); proto_reg_handoff_docsis_uccreq ();}
  {extern void proto_reg_handoff_docsis_uccrsp (void); proto_reg_handoff_docsis_uccrsp ();}
  {extern void proto_reg_handoff_docsis_ucd (void); proto_reg_handoff_docsis_ucd ();}
  {extern void proto_reg_handoff_docsis_vsif (void); proto_reg_handoff_docsis_vsif ();}
}
#endif
