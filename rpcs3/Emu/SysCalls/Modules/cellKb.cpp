#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/Io/Keyboard.h"
#include "cellKb.h"

extern Module<> sys_io;

s32 cellKbInit(u32 max_connect)
{
	sys_io.warning("cellKbInit(max_connect=%d)", max_connect);

	if (Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_ALREADY_INITIALIZED;
	if (max_connect > 7)
		return CELL_KB_ERROR_INVALID_PARAMETER;

	Emu.GetKeyboardManager().Init(max_connect);

	return CELL_OK;
}

s32 cellKbEnd()
{
	sys_io.trace("cellKbEnd()");

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;

	Emu.GetKeyboardManager().Close();
	return CELL_OK;
}

s32 cellKbClearBuf(u32 port_no)
{
	sys_io.trace("cellKbClearBuf(port_no=%d)", port_no);

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= Emu.GetKeyboardManager().GetKeyboards().size())
		return CELL_KB_ERROR_INVALID_PARAMETER;

	//?

	return CELL_OK;
}

u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode)
{
	sys_io.trace("cellKbCnvRawCode(arrange=%d,mkey=%d,led=%d,rawcode=%d)", arrange, mkey, led, rawcode);

	// CELL_KB_RAWDAT
	if (rawcode <= 0x03 || rawcode == 0x29 || rawcode == 0x35 || (rawcode >= 0x39 && rawcode <= 0x53) || rawcode == 0x65 || rawcode == 0x88 || rawcode == 0x8A || rawcode == 0x8B)
	{
		return rawcode | 0x8000;
	}

	// CELL_KB_NUMPAD
	if (rawcode >= 0x59 && rawcode <= 0x61) return (rawcode - 0x28) | 0x4000; // '1' - '9'
	if (rawcode == 0x62) return 0x30 | 0x4000;                                // '0'
	if (rawcode == 0x53) return 0x00 | 0x4000;                                // 'Num Lock'
	if (rawcode == 0x54) return 0x2F | 0x4000;                                // '/'
	if (rawcode == 0x55) return 0x2A | 0x4000;                                // '*'
	if (rawcode == 0x56) return 0x2D | 0x4000;                                // '-'
	if (rawcode == 0x57) return 0x2B | 0x4000;                                // '+'
	if (rawcode == 0x58) return 0x0A | 0x4000;                                // '\n'

	// ASCII
	if (rawcode >= 0x04 && rawcode <= 0x1D)                                   // 'A' - 'Z'
	{
		rawcode -= 
			(mkey&(CELL_KB_MKEY_L_SHIFT|CELL_KB_MKEY_R_SHIFT)) ? 
			((led&(CELL_KB_LED_CAPS_LOCK)) ? 0 : 0x20) :
			((led&(CELL_KB_LED_CAPS_LOCK)) ? 0x20 : 0);
		return rawcode + 0x5D;
	}				
	if (rawcode >= 0x1E && rawcode <= 0x26) return rawcode + 0x13;            // '1' - '9'
	if (rawcode == 0x27) return 0x30;                                         // '0'
	if (rawcode == 0x28) return 0x0A;                                         // '\n'
	if (rawcode == 0x2B) return 0x09;                                         // '\t'
	if (rawcode == 0x2C) return 0x20;                                         // ' '
	if (rawcode == 0x2D) return 0x2D;                                         // '-'
	if (rawcode == 0x2E) return 0x3D;                                         // '='
	if (rawcode == 0x36) return 0x2C;                                         // ','
	if (rawcode == 0x37) return 0x2E;                                         // '.'
	if (rawcode == 0x38) return 0x2F;                                         // '/'
	if (rawcode == 0x87) return 0x5C;                                         // '\'

	// (TODO: Add more cases)

	return 0x0000;
}

s32 cellKbGetInfo(vm::ptr<CellKbInfo> info)
{
	sys_io.trace("cellKbGetInfo(info=*0x%x)", info);

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;

	const KbInfo& current_info = Emu.GetKeyboardManager().GetInfo();
	info->max_connect = current_info.max_connect;
	info->now_connect = current_info.now_connect;
	info->info = current_info.info;

	for (u32 i=0; i<CELL_KB_MAX_KEYBOARDS; i++)
	{
		info->status[i] = current_info.status[i];
	}
	
	return CELL_OK;
}

s32 cellKbRead(u32 port_no, vm::ptr<CellKbData> data)
{
	sys_io.trace("cellKbRead(port_no=%d, data=*0x%x)", port_no, data);

	const std::vector<Keyboard>& keyboards = Emu.GetKeyboardManager().GetKeyboards();
	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;

	if (port_no >= keyboards.size())
		return CELL_KB_ERROR_INVALID_PARAMETER;

	KbData& current_data = Emu.GetKeyboardManager().GetData(port_no);
	data->led = current_data.led;
	data->mkey = current_data.mkey;
	data->len = std::min((u32)current_data.len, CELL_KB_MAX_KEYCODES);

	for (s32 i=0; i<current_data.len; i++)
	{
		data->keycode[i] = current_data.keycode[i];
	}

	current_data.len = 0;
	
	return CELL_OK;
}

s32 cellKbSetCodeType(u32 port_no, u32 type)
{
	sys_io.trace("cellKbSetCodeType(port_no=%d,type=%d)", port_no, type);

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;
	
	KbConfig& current_config = Emu.GetKeyboardManager().GetConfig(port_no);
	current_config.code_type = type;
	return CELL_OK;
}

s32 cellKbSetLEDStatus(u32 port_no, u8 led)
{
	UNIMPLEMENTED_FUNC(sys_io);
	return CELL_OK;
}

s32 cellKbSetReadMode(u32 port_no, u32 rmode)
{
	sys_io.trace("cellKbSetReadMode(port_no=%d,rmode=%d)", port_no, rmode);

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;
	
	KbConfig& current_config = Emu.GetKeyboardManager().GetConfig(port_no);
	current_config.read_mode = rmode;

	return CELL_OK;
}

s32 cellKbGetConfiguration(u32 port_no, vm::ptr<CellKbConfig> config)
{
	sys_io.trace("cellKbGetConfiguration(port_no=%d, config=*0x%x)", port_no, config);

	if (!Emu.GetKeyboardManager().IsInited())
		return CELL_KB_ERROR_UNINITIALIZED;

	const KbConfig& current_config = Emu.GetKeyboardManager().GetConfig(port_no);
	config->arrange = current_config.arrange;
	config->read_mode = current_config.read_mode;
	config->code_type = current_config.code_type;

	return CELL_OK;
}

void cellKb_init()
{
	REG_FUNC(sys_io, cellKbInit);
	REG_FUNC(sys_io, cellKbEnd);
	REG_FUNC(sys_io, cellKbClearBuf);
	REG_FUNC(sys_io, cellKbCnvRawCode);
	REG_FUNC(sys_io, cellKbGetInfo);
	REG_FUNC(sys_io, cellKbRead);
	REG_FUNC(sys_io, cellKbSetCodeType);
	REG_FUNC(sys_io, cellKbSetLEDStatus);
	REG_FUNC(sys_io, cellKbSetReadMode);
	REG_FUNC(sys_io, cellKbGetConfiguration);
}
