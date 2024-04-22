/*
	This file is part of libswirl
        original code by flyinghead
*/
#include "license/bsd"



#pragma once
#include <map>
#include <stdio.h>
#include "types.h"
#include "gamepad.h"

class InputMapping
{
public:
	InputMapping() {}
	InputMapping(const InputMapping& other) {
		name = other.name;
	    buttons = other.buttons;
	    axes = other.axes;
	    axes_inverted = other.axes_inverted;
	}

	std::string name;

	DreamcastKey get_button_id(u32 code)
	{
		auto it = buttons.find(code);
		if (it != buttons.end())
			return it->second;
		else
			return EMU_BTN_NONE;
	}
	void set_button(DreamcastKey id, u32 code);
	u32 get_button_code(DreamcastKey key);

	DreamcastKey get_axis_id(u32 code)
	{
		auto it = axes.find(code);
		if (it != axes.end())
			return it->second;
		else
			return EMU_AXIS_NONE;
	}
	bool get_axis_inverted(u32 code)
	{
		auto it = axes_inverted.find(code);
		if (it != axes_inverted.end())
			return it->second;
		else
			return false;
	}
	u32 get_axis_code(DreamcastKey key);
	void set_axis(DreamcastKey id, u32 code, bool inverted);

	void load(FILE* fp);
	bool save(const char *name);

	bool is_dirty() { return dirty; }

	static InputMapping *LoadMapping(const char *name);

protected:
	bool dirty = false;

private:
	std::map<u32, DreamcastKey> buttons;
	std::map<u32, DreamcastKey> axes;
	std::map<u32, bool> axes_inverted;

	static std::map<std::string, InputMapping *> loaded_mappings;
};

class IdentityInputMapping : public InputMapping
{
public:
	IdentityInputMapping() {
		name = "Default";
		for (int i = 0; i < 16; i++)
			set_button((DreamcastKey)(1 << i), 1 << i);
		set_axis(DC_AXIS_X, DC_AXIS_X, false);
		set_axis(DC_AXIS_Y, DC_AXIS_Y, false);
		set_axis(DC_AXIS_LT, DC_AXIS_LT, false);
		set_axis(DC_AXIS_RT, DC_AXIS_RT, false);
		set_axis(DC_AXIS_X2, DC_AXIS_X2, false);
		set_axis(DC_AXIS_Y2, DC_AXIS_Y2, false);
	}
};
