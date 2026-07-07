#include "type_text.h"

#include <linux/uinput.h>
#include <string.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-names.h>

#define FLAG_UPPERCASE		0x80000000
#define XKB_EVDEV_OFFSET	8

static const int32_t ascii2keycode_map[128] = {
	// 00 - 0f
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,KEY_TAB,KEY_ENTER,-1,-1,-1,-1,-1,

	// 10 - 1f
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,

	// 20 - 2f
	KEY_SPACE,KEY_1|FLAG_UPPERCASE,KEY_APOSTROPHE|FLAG_UPPERCASE,KEY_3|FLAG_UPPERCASE,KEY_4|FLAG_UPPERCASE,KEY_5|FLAG_UPPERCASE,KEY_7|FLAG_UPPERCASE,KEY_APOSTROPHE,
	KEY_9|FLAG_UPPERCASE,KEY_0|FLAG_UPPERCASE,KEY_8|FLAG_UPPERCASE,KEY_EQUAL|FLAG_UPPERCASE,KEY_COMMA,KEY_MINUS,KEY_DOT,KEY_SLASH,

	// 30 - 3f
	KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,
	KEY_8,KEY_9,KEY_SEMICOLON|FLAG_UPPERCASE,KEY_SEMICOLON,KEY_COMMA|FLAG_UPPERCASE,KEY_EQUAL,KEY_DOT|FLAG_UPPERCASE,KEY_SLASH|FLAG_UPPERCASE,

	// 40 - 4f
	KEY_2|FLAG_UPPERCASE,KEY_A|FLAG_UPPERCASE,KEY_B|FLAG_UPPERCASE,KEY_C|FLAG_UPPERCASE,KEY_D|FLAG_UPPERCASE,KEY_E|FLAG_UPPERCASE,KEY_F|FLAG_UPPERCASE,KEY_G|FLAG_UPPERCASE,
	KEY_H|FLAG_UPPERCASE,KEY_I|FLAG_UPPERCASE,KEY_J|FLAG_UPPERCASE,KEY_K|FLAG_UPPERCASE,KEY_L|FLAG_UPPERCASE,KEY_M|FLAG_UPPERCASE,KEY_N|FLAG_UPPERCASE,KEY_O|FLAG_UPPERCASE,

	// 50 - 5f
	KEY_P|FLAG_UPPERCASE,KEY_Q|FLAG_UPPERCASE,KEY_R|FLAG_UPPERCASE,KEY_S|FLAG_UPPERCASE,KEY_T|FLAG_UPPERCASE,KEY_U|FLAG_UPPERCASE,KEY_V|FLAG_UPPERCASE,KEY_W|FLAG_UPPERCASE,
	KEY_X|FLAG_UPPERCASE,KEY_Y|FLAG_UPPERCASE,KEY_Z|FLAG_UPPERCASE,KEY_LEFTBRACE,KEY_BACKSLASH,KEY_RIGHTBRACE,KEY_6|FLAG_UPPERCASE,KEY_MINUS|FLAG_UPPERCASE,

	// 60 - 6f
	KEY_GRAVE,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,
	KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,

	// 70 - 7f
	KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,
	KEY_X,KEY_Y,KEY_Z,KEY_LEFTBRACE|FLAG_UPPERCASE,KEY_BACKSLASH|FLAG_UPPERCASE,KEY_RIGHTBRACE|FLAG_UPPERCASE,KEY_GRAVE|FLAG_UPPERCASE,-1
};

struct find_key_data {
	struct ydotool_type_sequence *sequence;
	xkb_keysym_t target;
	int result;
	bool found_with_unsupported_modifiers;
};

void ydotool_type_resolver_init(struct ydotool_type_resolver *resolver) {
	resolver->xkb_context = NULL;
	resolver->keymap = NULL;
	memset(&resolver->names, 0, sizeof(resolver->names));
	resolver->force_xkb = false;
}

void ydotool_type_resolver_set_xkb(struct ydotool_type_resolver *resolver,
				   const struct ydotool_xkb_names *names,
				   bool force_xkb) {
	ydotool_type_resolver_destroy(resolver);

	if (names) {
		resolver->names = *names;
	} else {
		memset(&resolver->names, 0, sizeof(resolver->names));
	}
	resolver->force_xkb = force_xkb;
}

static int init_keymap(struct ydotool_type_resolver *resolver) {
	struct xkb_rule_names names = {
		.rules = resolver->names.rules,
		.model = resolver->names.model,
		.layout = resolver->names.layout,
		.variant = resolver->names.variant,
		.options = resolver->names.options
	};

	resolver->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!resolver->xkb_context) {
		return YDOTOOL_TYPE_KEYMAP_ERROR;
	}

	resolver->keymap = xkb_keymap_new_from_names(resolver->xkb_context,
						     &names,
						     XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!resolver->keymap) {
		ydotool_type_resolver_destroy(resolver);
		return YDOTOOL_TYPE_KEYMAP_ERROR;
	}

	return YDOTOOL_TYPE_OK;
}

int ydotool_type_resolver_init_keymap(struct ydotool_type_resolver *resolver,
				      const struct ydotool_xkb_names *names,
				      bool force_xkb) {
	ydotool_type_resolver_init(resolver);
	ydotool_type_resolver_set_xkb(resolver, names, force_xkb);

	return init_keymap(resolver);
}

void ydotool_type_resolver_destroy(struct ydotool_type_resolver *resolver) {
	if (resolver->keymap) {
		xkb_keymap_unref(resolver->keymap);
		resolver->keymap = NULL;
	}

	if (resolver->xkb_context) {
		xkb_context_unref(resolver->xkb_context);
		resolver->xkb_context = NULL;
	}
}

void ydotool_type_utf8_init(struct ydotool_type_utf8_decoder *decoder) {
	decoder->codepoint = 0;
	decoder->min_codepoint = 0;
	decoder->remaining = 0;
}

int ydotool_type_utf8_feed(struct ydotool_type_utf8_decoder *decoder,
			   uint8_t byte,
			   uint32_t *codepoint,
			   bool *complete) {
	*complete = false;
	*codepoint = 0;

	if (decoder->remaining == 0) {
		if (byte < 0x80) {
			*codepoint = byte;
			*complete = true;
			return YDOTOOL_TYPE_OK;
		}

		if (byte >= 0xc2 && byte <= 0xdf) {
			decoder->codepoint = byte & 0x1f;
			decoder->min_codepoint = 0x80;
			decoder->remaining = 1;
			return YDOTOOL_TYPE_OK;
		}

		if (byte >= 0xe0 && byte <= 0xef) {
			decoder->codepoint = byte & 0x0f;
			decoder->min_codepoint = 0x800;
			decoder->remaining = 2;
			return YDOTOOL_TYPE_OK;
		}

		if (byte >= 0xf0 && byte <= 0xf4) {
			decoder->codepoint = byte & 0x07;
			decoder->min_codepoint = 0x10000;
			decoder->remaining = 3;
			return YDOTOOL_TYPE_OK;
		}

		return YDOTOOL_TYPE_INVALID_UTF8;
	}

	if ((byte & 0xc0) != 0x80) {
		ydotool_type_utf8_init(decoder);
		return YDOTOOL_TYPE_INVALID_UTF8;
	}

	decoder->codepoint = (decoder->codepoint << 6) | (byte & 0x3f);
	decoder->remaining--;

	if (decoder->remaining == 0) {
		if (decoder->codepoint < decoder->min_codepoint ||
		    decoder->codepoint > 0x10ffff ||
		    (decoder->codepoint >= 0xd800 && decoder->codepoint <= 0xdfff)) {
			ydotool_type_utf8_init(decoder);
			return YDOTOOL_TYPE_INVALID_UTF8;
		}

		*codepoint = decoder->codepoint;
		*complete = true;
		ydotool_type_utf8_init(decoder);
	}

	return YDOTOOL_TYPE_OK;
}

int ydotool_type_utf8_finish(const struct ydotool_type_utf8_decoder *decoder) {
	return decoder->remaining == 0 ? YDOTOOL_TYPE_OK : YDOTOOL_TYPE_TRUNCATED_UTF8;
}

static int ensure_keymap(struct ydotool_type_resolver *resolver) {
	if (resolver->keymap) {
		return YDOTOOL_TYPE_OK;
	}

	return init_keymap(resolver);
}

static bool add_modifier(struct ydotool_type_sequence *sequence, uint16_t key) {
	for (size_t i = 0; i < sequence->modifier_count; i++) {
		if (sequence->modifiers[i] == key) {
			return true;
		}
	}

	if (sequence->modifier_count == YDOTOOL_TYPE_MAX_MODIFIERS) {
		return false;
	}

	sequence->modifiers[sequence->modifier_count++] = key;
	return true;
}

static int mod_name_to_key(const char *name, uint16_t *key) {
	if (strcmp(name, XKB_MOD_NAME_SHIFT) == 0) {
		*key = KEY_LEFTSHIFT;
		return 0;
	}
	if (strcmp(name, XKB_MOD_NAME_CTRL) == 0) {
		*key = KEY_LEFTCTRL;
		return 0;
	}
	if (strcmp(name, XKB_MOD_NAME_ALT) == 0 || strcmp(name, "Alt") == 0 || strcmp(name, "LAlt") == 0) {
		*key = KEY_LEFTALT;
		return 0;
	}
	if (strcmp(name, XKB_MOD_NAME_LOGO) == 0 || strcmp(name, "Super") == 0 || strcmp(name, "Meta") == 0) {
		*key = KEY_LEFTMETA;
		return 0;
	}
	if (strcmp(name, "Mod5") == 0 || strcmp(name, "LevelThree") == 0 ||
	    strcmp(name, "AltGr") == 0 || strcmp(name, "RAlt") == 0) {
		*key = KEY_RIGHTALT;
		return 0;
	}

	return -1;
}

static int mask_to_modifiers(struct xkb_keymap *keymap,
			     xkb_mod_mask_t mask,
			     struct ydotool_type_sequence *sequence) {
	xkb_mod_index_t mod_count = xkb_keymap_num_mods(keymap);

	sequence->modifier_count = 0;

	for (xkb_mod_index_t i = 0; i < mod_count; i++) {
		xkb_mod_mask_t bit;
		const char *name;
		uint16_t key;

		if (i >= sizeof(xkb_mod_mask_t) * 8) {
			break;
		}

		bit = (xkb_mod_mask_t)1 << i;
		if ((mask & bit) == 0) {
			continue;
		}

		name = xkb_keymap_mod_get_name(keymap, i);
		if (!name || mod_name_to_key(name, &key) != 0 || !add_modifier(sequence, key)) {
			return YDOTOOL_TYPE_UNSUPPORTED_MODIFIERS;
		}

		mask &= ~bit;
	}

	if (mask != 0) {
		return YDOTOOL_TYPE_UNSUPPORTED_MODIFIERS;
	}

	return YDOTOOL_TYPE_OK;
}

static void find_key(struct xkb_keymap *keymap, xkb_keycode_t key, void *userdata) {
	struct find_key_data *data = userdata;
	const xkb_keysym_t *syms = NULL;
	xkb_level_index_t levels;

	if (data->result == YDOTOOL_TYPE_OK) {
		return;
	}

	if (key < XKB_EVDEV_OFFSET) {
		return;
	}

	levels = xkb_keymap_num_levels_for_key(keymap, key, 0);
	for (xkb_level_index_t level = 0; level < levels; level++) {
		int syms_count = xkb_keymap_key_get_syms_by_level(keymap, key, 0, level, &syms);

		if (syms_count != 1 || syms[0] != data->target) {
			continue;
		}

		xkb_mod_mask_t masks[8] = {0};
		size_t mask_count = xkb_keymap_key_get_mods_for_level(keymap, key, 0, level,
								      masks,
								      sizeof(masks) / sizeof(masks[0]));

		if (mask_count == 0) {
			data->found_with_unsupported_modifiers = true;
			continue;
		}

		for (size_t i = 0; i < mask_count; i++) {
			data->sequence->key = (uint16_t)(key - XKB_EVDEV_OFFSET);
			if (mask_to_modifiers(keymap, masks[i], data->sequence) == YDOTOOL_TYPE_OK) {
				data->result = YDOTOOL_TYPE_OK;
				return;
			}
			data->found_with_unsupported_modifiers = true;
		}
	}
}

int ydotool_type_resolve_codepoint(struct ydotool_type_resolver *resolver,
				   uint32_t codepoint,
				   struct ydotool_type_sequence *sequence) {
	memset(sequence, 0, sizeof(*sequence));

	if (codepoint < 128 && !resolver->force_xkb) {
		int32_t kdef = ascii2keycode_map[(unsigned char)codepoint];
		if (kdef == -1) {
			return YDOTOOL_TYPE_NOT_TYPEABLE;
		}

		sequence->key = (uint16_t)(kdef & 0xffff);
		if (kdef & FLAG_UPPERCASE) {
			sequence->modifiers[sequence->modifier_count++] = KEY_LEFTSHIFT;
		}
		return YDOTOOL_TYPE_OK;
	}

	int rc = ensure_keymap(resolver);
	if (rc != YDOTOOL_TYPE_OK) {
		return rc;
	}

	xkb_keysym_t keysym = xkb_utf32_to_keysym(codepoint);
	if (keysym == XKB_KEY_NoSymbol) {
		return YDOTOOL_TYPE_NOT_TYPEABLE;
	}

	struct find_key_data data = {
		.sequence = sequence,
		.target = keysym,
		.result = YDOTOOL_TYPE_NOT_TYPEABLE,
		.found_with_unsupported_modifiers = false
	};

	xkb_keymap_key_for_each(resolver->keymap, find_key, &data);

	if (data.result != YDOTOOL_TYPE_OK && data.found_with_unsupported_modifiers) {
		return YDOTOOL_TYPE_UNSUPPORTED_MODIFIERS;
	}

	return data.result;
}

int ydotool_type_emit_codepoint(struct ydotool_type_resolver *resolver,
				uint32_t codepoint,
				const struct ydotool_type_options *options,
				bool delay,
				ydotool_type_emit_fn emit,
				void *userdata) {
	struct ydotool_type_sequence sequence;
	int rc = ydotool_type_resolve_codepoint(resolver, codepoint, &sequence);
	if (rc != YDOTOOL_TYPE_OK) {
		return rc;
	}

	for (size_t i = 0; i < sequence.modifier_count; i++) {
		emit(sequence.modifiers[i], 1, userdata);
	}

	emit(sequence.key, 1, userdata);
	usleep(options->key_hold_ms * 1000);
	emit(sequence.key, 0, userdata);

	for (size_t i = sequence.modifier_count; i > 0; i--) {
		emit(sequence.modifiers[i - 1], 0, userdata);
	}

	if (delay) {
		usleep(options->key_delay_ms * 1000);
	}

	return YDOTOOL_TYPE_OK;
}
