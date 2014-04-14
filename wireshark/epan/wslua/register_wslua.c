/* This file is automatically genrated by make-reg.pl do not edit */

#include "config.h"
#include "wslua.h"

void wslua_register_classes(lua_State* L) { 
	ByteArray_register(L);
	Tvb_register(L);
	TvbRange_register(L);
	Int64_register(L);
	UInt64_register(L);
	Pref_register(L);
	Prefs_register(L);
	ProtoField_register(L);
	Proto_register(L);
	Dissector_register(L);
	DissectorTable_register(L);
	TreeItem_register(L);
	NSTime_register(L);
	Address_register(L);
	Column_register(L);
	Columns_register(L);
	PrivateTable_register(L);
	Pinfo_register(L);
	Listener_register(L);
	ProgDlg_register(L);
	TextWindow_register(L);
	Dir_register(L);
	FieldInfo_register(L);
	Field_register(L);
	PseudoHeader_register(L);
	Dumper_register(L);
	luaopen_bit(L);
}

void wslua_register_functions(lua_State* L) {
	WSLUA_REGISTER_FUNCTION(register_postdissector); 
	WSLUA_REGISTER_FUNCTION(gui_enabled); 
	WSLUA_REGISTER_FUNCTION(register_menu); 
	WSLUA_REGISTER_FUNCTION(new_dialog); 
	WSLUA_REGISTER_FUNCTION(retap_packets); 
	WSLUA_REGISTER_FUNCTION(copy_to_clipboard); 
	WSLUA_REGISTER_FUNCTION(open_capture_file); 
	WSLUA_REGISTER_FUNCTION(get_filter); 
	WSLUA_REGISTER_FUNCTION(set_filter); 
	WSLUA_REGISTER_FUNCTION(set_color_filter_slot); 
	WSLUA_REGISTER_FUNCTION(apply_filter); 
	WSLUA_REGISTER_FUNCTION(reload); 
	WSLUA_REGISTER_FUNCTION(browser_open_url); 
	WSLUA_REGISTER_FUNCTION(browser_open_data_file); 
	WSLUA_REGISTER_FUNCTION(get_version); 
	WSLUA_REGISTER_FUNCTION(format_date); 
	WSLUA_REGISTER_FUNCTION(format_time); 
	WSLUA_REGISTER_FUNCTION(report_failure); 
	WSLUA_REGISTER_FUNCTION(critical); 
	WSLUA_REGISTER_FUNCTION(warn); 
	WSLUA_REGISTER_FUNCTION(message); 
	WSLUA_REGISTER_FUNCTION(info); 
	WSLUA_REGISTER_FUNCTION(debug); 
	WSLUA_REGISTER_FUNCTION(loadfile); 
	WSLUA_REGISTER_FUNCTION(dofile); 
	WSLUA_REGISTER_FUNCTION(persconffile_path); 
	WSLUA_REGISTER_FUNCTION(datafile_path); 
	WSLUA_REGISTER_FUNCTION(register_stat_cmd_arg); 
	WSLUA_REGISTER_FUNCTION(all_field_infos); 
}

