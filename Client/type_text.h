#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct xkb_context;
struct xkb_keymap;

#define YDOTOOL_TYPE_MAX_MODIFIERS 8

enum ydotool_type_result {
	YDOTOOL_TYPE_OK = 0,
	YDOTOOL_TYPE_INVALID_UTF8 = -1,
	YDOTOOL_TYPE_TRUNCATED_UTF8 = -2,
	YDOTOOL_TYPE_NOT_TYPEABLE = -3,
	YDOTOOL_TYPE_KEYMAP_ERROR = -4,
	YDOTOOL_TYPE_UNSUPPORTED_MODIFIERS = -5
};

struct ydotool_type_sequence {
	uint16_t modifiers[YDOTOOL_TYPE_MAX_MODIFIERS];
	size_t modifier_count;
	uint16_t key;
};

struct ydotool_xkb_names {
	const char *rules;
	const char *model;
	const char *layout;
	const char *variant;
	const char *options;
};

struct ydotool_type_resolver {
	struct xkb_context *xkb_context;
	struct xkb_keymap *keymap;
	struct ydotool_xkb_names names;
	bool force_xkb;
};

struct ydotool_type_utf8_decoder {
	uint32_t codepoint;
	uint32_t min_codepoint;
	unsigned int remaining;
};

struct ydotool_type_options {
	int key_hold_ms;
	int key_delay_ms;
};

typedef void (*ydotool_type_emit_fn)(uint16_t code, int32_t value, void *userdata);

void ydotool_type_resolver_init(struct ydotool_type_resolver *resolver);
void ydotool_type_resolver_set_xkb(struct ydotool_type_resolver *resolver,
				   const struct ydotool_xkb_names *names,
				   bool force_xkb);
int ydotool_type_resolver_init_keymap(struct ydotool_type_resolver *resolver,
				      const struct ydotool_xkb_names *names,
				      bool force_xkb);
void ydotool_type_resolver_destroy(struct ydotool_type_resolver *resolver);

void ydotool_type_utf8_init(struct ydotool_type_utf8_decoder *decoder);
int ydotool_type_utf8_feed(struct ydotool_type_utf8_decoder *decoder,
			   uint8_t byte,
			   uint32_t *codepoint,
			   bool *complete);
int ydotool_type_utf8_finish(const struct ydotool_type_utf8_decoder *decoder);

int ydotool_type_resolve_codepoint(struct ydotool_type_resolver *resolver,
				   uint32_t codepoint,
				   struct ydotool_type_sequence *sequence);
int ydotool_type_emit_codepoint(struct ydotool_type_resolver *resolver,
				uint32_t codepoint,
				const struct ydotool_type_options *options,
				bool delay,
				ydotool_type_emit_fn emit,
				void *userdata);
