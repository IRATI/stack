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
  {extern void proto_register_ams (void); proto_register_ams ();}
  {extern void proto_register_ecat (void); proto_register_ecat ();}
  {extern void proto_register_ecat_mailbox (void); proto_register_ecat_mailbox ();}
  {extern void proto_register_esl (void); proto_register_esl ();}
  {extern void proto_register_ethercat_frame (void); proto_register_ethercat_frame ();}
  {extern void proto_register_ioraw (void); proto_register_ioraw ();}
  {extern void proto_register_nv (void); proto_register_nv ();}
}

WS_DLL_PUBLIC_NOEXTERN void
plugin_reg_handoff(void)
{
  {extern void proto_reg_handoff_ams (void); proto_reg_handoff_ams ();}
  {extern void proto_reg_handoff_ecat (void); proto_reg_handoff_ecat ();}
  {extern void proto_reg_handoff_ecat_mailbox (void); proto_reg_handoff_ecat_mailbox ();}
  {extern void proto_reg_handoff_esl (void); proto_reg_handoff_esl ();}
  {extern void proto_reg_handoff_ethercat_frame (void); proto_reg_handoff_ethercat_frame ();}
  {extern void proto_reg_handoff_ioraw (void); proto_reg_handoff_ioraw ();}
  {extern void proto_reg_handoff_nv (void); proto_reg_handoff_nv ();}
}
#endif
