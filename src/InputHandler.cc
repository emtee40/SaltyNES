/*
Copyright (c) 2012-2017 Matthew Brennan Jones <matthew.brennan.jones@gmail.com>
Copyright (c) 2006-2011 Jamie Sanders
A NES emulator in WebAssembly. Based on vNES.
Licensed under GPLV3 or later
Hosted at: https://github.com/workhorsy/nes_wasm
*/


#include "SaltyNES.h"

const float InputHandler::AXES_DEAD_ZONE = 0.2;
const string InputHandler::KEYS[] = { "up", "down", "right", "left", "start", "select", "a", "b" };
const size_t InputHandler::KEYS_LENGTH = 8;
bool InputHandler::_is_configuring_gamepad = false;
string InputHandler::_configuring_gamepad_button = "";

InputHandler::InputHandler(int id) :
	_id(id),
	_keys(255),
	_map(InputHandler::NUM_KEYS) {

	_is_gamepad_connected = false;
	_is_gamepad_used = false;
	_is_keyboard_used = false;

	_is_input_pressed["up"] = false;
	_is_input_pressed["down"] = false;
	_is_input_pressed["right"] = false;
	_is_input_pressed["left"] = false;
	_is_input_pressed["start"] = false;
	_is_input_pressed["select"] = false;
	_is_input_pressed["a"] = false;
	_is_input_pressed["b"] = false;
}

InputHandler::~InputHandler() {

}

#ifdef NACL
void InputHandler::update_gamepad(PP_GamepadsSampleData gamepad_data) {
	// Get current gamepad data
	for(size_t i=0; i<gamepad_data.length; ++i) {
		PP_GamepadSampleData& pad = gamepad_data.items[i];

		if(!pad.connected)
			continue;

		// Check if we are switching to the gamepad from the keyboard
		for(size_t j=0; j<pad.buttons_length; ++j) {
			if(pad.buttons[j]) {
				_is_gamepad_used = true;
				_is_keyboard_used = false;
			}
		}
		for(size_t j=0; j<pad.axes_length; ++j) {
			if(pad.axes[j] > InputHandler::AXES_DEAD_ZONE || pad.axes[j] < -InputHandler::AXES_DEAD_ZONE) {
				_is_gamepad_used = true;
				_is_keyboard_used = false;
			}
		}

		// Get the vendor, and product id
		if(_is_gamepad_used) {
			char id[128];
			for(int k=0; k<128; ++k) {
				id[k] = pad.id[k];
			}
			string sid = id;
			size_t vendor_pos = sid.find("Vendor: ");
			size_t product_pos = sid.find("Product: ");
			_gamepad_vendor_id = sid.substr(vendor_pos + strlen("Vendor: "), 4);
			_gamepad_product_id = sid.substr(product_pos + strlen("Product: "), 4);
		}

		// Get the key states
		if(_is_gamepad_used) {
			for(size_t j=0; j<InputHandler::KEYS_LENGTH; ++j) {
				string key = InputHandler::KEYS[j];
				_is_input_pressed[key] = false;
				// Button
				for(size_t k=0; k<_input_map_button[key].size(); ++k) {
					size_t number = _input_map_button[key][k];
					if(pad.buttons[number] > 0) {
						_is_input_pressed[key] = true;
					}
				}
				// Positive Axes
				for(size_t k=0; k<_input_map_axes_pos[key].size(); ++k) {
					size_t number = _input_map_axes_pos[key][k];
					if(pad.axes[number] > InputHandler::AXES_DEAD_ZONE) {
						_is_input_pressed[key] = true;
					}
				}
				// Negative Axes
				for(size_t k=0; k<_input_map_axes_neg[key].size(); ++k) {
					size_t number = _input_map_axes_neg[key][k];
					if(pad.axes[number] < -InputHandler::AXES_DEAD_ZONE) {
						_is_input_pressed[key] = true;
					}
				}
			}
		}

		// If we are configuring the gamepad, save the last key pressed
		if(_is_configuring_gamepad) {
			for(size_t j=0; j<pad.buttons_length; ++j) {
				if(pad.buttons[j]) {
					stringstream out;
					out << j;
					_configuring_gamepad_button = out.str();
					return;
				}
			}
			for(size_t j=0; j<pad.axes_length; ++j) {
				if(pad.axes[j] > InputHandler::AXES_DEAD_ZONE) {
					stringstream out;
					out << "axes+" << j;
					_configuring_gamepad_button = out.str();
					return;
				} else if(pad.axes[j] < -InputHandler::AXES_DEAD_ZONE) {
					stringstream out;
					out << "axes-" << j;
					_configuring_gamepad_button = out.str();
					return;
				}
			}
		}
	}
}
#endif

uint16_t InputHandler::getKeyState(int padKey) {
	return static_cast<uint16_t>(_keys[_map[padKey]] ? 0x41 : 0x40);
}

void InputHandler::mapKey(int padKey, int kbKeycode) {
	_map[padKey] = kbKeycode;
}

void InputHandler::poll_for_key_events() {
#ifdef SDL
	int numberOfKeys;
	uint8_t* keystate = 0;// FIXME: SDL_GetKeyState(&numberOfKeys);

	int is_left = EM_ASM_INT({
		return g_get_key_left;
	}, 0);

	int is_right = EM_ASM_INT({
		return g_get_key_right;
	}, 0);

	int is_up = EM_ASM_INT({
		return g_get_key_up;
	}, 0);

	int is_down = EM_ASM_INT({
		return g_get_key_down;
	}, 0);

	int is_b = EM_ASM_INT({
		return g_get_key_b;
	}, 0);

	int is_a = EM_ASM_INT({
		return g_get_key_a;
	}, 0);

	int is_start = EM_ASM_INT({
		return g_get_key_start;
	}, 0);

	_keys[_map[InputHandler::KEY_UP]] =     is_up > 0;
	_keys[_map[InputHandler::KEY_DOWN]] =   is_down > 0;
	_keys[_map[InputHandler::KEY_RIGHT]] =  is_right > 0;
	_keys[_map[InputHandler::KEY_LEFT]] =   is_left > 0;
	_keys[_map[InputHandler::KEY_START]] =  is_start > 0;
	_keys[_map[InputHandler::KEY_SELECT]] = keystate[SDLK_RCTRL];
	_keys[_map[InputHandler::KEY_B]] =      is_b > 0;
	_keys[_map[InputHandler::KEY_A]] =      is_a > 0;
#endif

#ifdef NACL
	_keys[_map[InputHandler::KEY_UP]] =     _is_input_pressed["up"];
	_keys[_map[InputHandler::KEY_DOWN]] =   _is_input_pressed["down"];
	_keys[_map[InputHandler::KEY_RIGHT]] =  _is_input_pressed["right"];
	_keys[_map[InputHandler::KEY_LEFT]] =   _is_input_pressed["left"];
	_keys[_map[InputHandler::KEY_START]] =  _is_input_pressed["start"];
	_keys[_map[InputHandler::KEY_SELECT]] = _is_input_pressed["select"];
	_keys[_map[InputHandler::KEY_B]] =      _is_input_pressed["b"];
	_keys[_map[InputHandler::KEY_A]] =      _is_input_pressed["a"];
#endif

	// Can't hold both left & right or up & down at same time:
	if(_keys[_map[InputHandler::KEY_LEFT]]) {
		_keys[_map[InputHandler::KEY_RIGHT]] = false;
	} else if(_keys[_map[InputHandler::KEY_RIGHT]]) {
		_keys[_map[InputHandler::KEY_LEFT]] = false;
	}

	if(_keys[_map[InputHandler::KEY_UP]]) {
		_keys[_map[InputHandler::KEY_DOWN]] = false;
	} else if(_keys[_map[InputHandler::KEY_DOWN]]) {
		_keys[_map[InputHandler::KEY_UP]] = false;
	}
}

void InputHandler::reset() {
	size_t size = _keys.size();
	_keys.clear();
	_keys.resize(size);
}
