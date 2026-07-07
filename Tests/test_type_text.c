#include "../Client/type_text.h"

#include <assert.h>
#include <linux/uinput.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define XKB_EVDEV_OFFSET 8

struct expected_sequence {
	uint32_t codepoint;
	uint16_t key;
	uint16_t modifiers[2];
	size_t modifier_count;
};

static bool has_modifier(const struct ydotool_type_sequence *sequence, uint16_t key) {
	for (size_t i = 0; i < sequence->modifier_count; i++) {
		if (sequence->modifiers[i] == key) {
			return true;
		}
	}

	return false;
}

static uint32_t replay_sequence(struct xkb_keymap *keymap,
				const struct ydotool_type_sequence *sequence) {
	struct xkb_state *state = xkb_state_new(keymap);
	uint32_t codepoint;

	assert(state);

	for (size_t i = 0; i < sequence->modifier_count; i++) {
		xkb_state_update_key(state, sequence->modifiers[i] + XKB_EVDEV_OFFSET, XKB_KEY_DOWN);
	}

	codepoint = xkb_state_key_get_utf32(state, sequence->key + XKB_EVDEV_OFFSET);
	xkb_state_update_key(state, sequence->key + XKB_EVDEV_OFFSET, XKB_KEY_DOWN);
	xkb_state_update_key(state, sequence->key + XKB_EVDEV_OFFSET, XKB_KEY_UP);

	for (size_t i = sequence->modifier_count; i > 0; i--) {
		xkb_state_update_key(state, sequence->modifiers[i - 1] + XKB_EVDEV_OFFSET, XKB_KEY_UP);
	}

	xkb_state_unref(state);
	return codepoint;
}

static void assert_historical_sequence(uint32_t codepoint,
				       uint16_t key,
				       const uint16_t *modifiers,
				       size_t modifier_count) {
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	ydotool_type_resolver_init(&resolver);
	assert(ydotool_type_resolve_codepoint(&resolver, codepoint, &sequence) == YDOTOOL_TYPE_OK);

	assert(sequence.key == key);
	assert(sequence.modifier_count == modifier_count);
	for (size_t i = 0; i < modifier_count; i++) {
		assert(sequence.modifiers[i] == modifiers[i]);
	}

	ydotool_type_resolver_destroy(&resolver);
}

static void assert_xkb_codepoint(struct ydotool_type_resolver *resolver,
				 uint32_t codepoint,
				 struct ydotool_type_sequence *sequence) {
	assert(ydotool_type_resolve_codepoint(resolver, codepoint, sequence) == YDOTOOL_TYPE_OK);
	assert(sequence->key > 0);
	assert(replay_sequence(resolver->keymap, sequence) == codepoint);
}

static void assert_xkb_string(struct ydotool_type_resolver *resolver, const char *str) {
	struct ydotool_type_utf8_decoder decoder;
	struct ydotool_type_sequence sequence;
	uint32_t codepoint = 0;
	bool complete = false;

	ydotool_type_utf8_init(&decoder);

	for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
		assert(ydotool_type_utf8_feed(&decoder, *p, &codepoint, &complete) == YDOTOOL_TYPE_OK);
		if (complete) {
			assert_xkb_codepoint(resolver, codepoint, &sequence);
		}
	}

	assert(ydotool_type_utf8_finish(&decoder) == YDOTOOL_TYPE_OK);
}

static void assert_utf8_ok(const unsigned char *bytes, size_t len, uint32_t expected) {
	struct ydotool_type_utf8_decoder decoder;
	uint32_t codepoint = 0;
	bool complete = false;

	ydotool_type_utf8_init(&decoder);
	for (size_t i = 0; i < len; i++) {
		assert(ydotool_type_utf8_feed(&decoder, bytes[i], &codepoint, &complete) == YDOTOOL_TYPE_OK);
	}

	assert(complete);
	assert(codepoint == expected);
	assert(ydotool_type_utf8_finish(&decoder) == YDOTOOL_TYPE_OK);
}

static void assert_utf8_invalid(const unsigned char *bytes, size_t len) {
	struct ydotool_type_utf8_decoder decoder;
	uint32_t codepoint = 0;
	bool complete = false;
	int rc = YDOTOOL_TYPE_OK;

	ydotool_type_utf8_init(&decoder);
	for (size_t i = 0; i < len; i++) {
		rc = ydotool_type_utf8_feed(&decoder, bytes[i], &codepoint, &complete);
		if (rc != YDOTOOL_TYPE_OK) {
			break;
		}
	}

	assert(rc == YDOTOOL_TYPE_INVALID_UTF8);
}

static void test_historical_ascii(void) {
	const uint16_t shift[] = {KEY_LEFTSHIFT};
	const struct expected_sequence expected[] = {
		{'H', KEY_H, {KEY_LEFTSHIFT}, 1},
		{'e', KEY_E, {0}, 0},
		{'l', KEY_L, {0}, 0},
		{'o', KEY_O, {0}, 0},
		{' ', KEY_SPACE, {0}, 0},
		{'W', KEY_W, {KEY_LEFTSHIFT}, 1},
		{'r', KEY_R, {0}, 0},
		{'d', KEY_D, {0}, 0},
		{'1', KEY_1, {0}, 0},
		{'2', KEY_2, {0}, 0},
		{'3', KEY_3, {0}, 0},
		{'a', KEY_A, {0}, 0},
		{'q', KEY_Q, {0}, 0},
		{'w', KEY_W, {0}, 0},
		{'z', KEY_Z, {0}, 0},
		{'m', KEY_M, {0}, 0},
		{'!', KEY_1, {KEY_LEFTSHIFT}, 1}
	};

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
		assert_historical_sequence(expected[i].codepoint,
					   expected[i].key,
					   expected[i].modifier_count ? expected[i].modifiers : NULL,
					   expected[i].modifier_count);
	}

	assert_historical_sequence('A', KEY_A, shift, ARRAY_SIZE(shift));
}

static void test_xkb_us(void) {
	struct ydotool_xkb_names names = {.layout = "us"};
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	assert(ydotool_type_resolver_init_keymap(&resolver, &names, true) == YDOTOOL_TYPE_OK);

	assert_xkb_string(&resolver, "aqwzAZ09?!:_");

	assert_xkb_codepoint(&resolver, '?', &sequence);
	assert(sequence.key == KEY_SLASH);
	assert(has_modifier(&sequence, KEY_LEFTSHIFT));

	assert_xkb_codepoint(&resolver, '!', &sequence);
	assert(sequence.key == KEY_1);
	assert(has_modifier(&sequence, KEY_LEFTSHIFT));

	assert(ydotool_type_resolve_codepoint(&resolver, 0x00e9, &sequence) == YDOTOOL_TYPE_NOT_TYPEABLE);

	ydotool_type_resolver_destroy(&resolver);
}

static void test_xkb_fr_layout_aware_ascii(void) {
	struct ydotool_xkb_names us_names = {.layout = "us"};
	struct ydotool_xkb_names fr_names = {.layout = "fr"};
	struct ydotool_type_resolver us;
	struct ydotool_type_resolver fr;
	struct ydotool_type_sequence us_sequence;
	struct ydotool_type_sequence fr_sequence;
	const char *sensitive = "aqwzm";
	const uint16_t expected_fr_keys[] = {KEY_Q, KEY_A, KEY_Z, KEY_W, KEY_SEMICOLON};

	assert(ydotool_type_resolver_init_keymap(&us, &us_names, true) == YDOTOOL_TYPE_OK);
	assert(ydotool_type_resolver_init_keymap(&fr, &fr_names, true) == YDOTOOL_TYPE_OK);

	for (size_t i = 0; sensitive[i]; i++) {
		uint32_t codepoint = (unsigned char)sensitive[i];

		assert_xkb_codepoint(&us, codepoint, &us_sequence);
		assert_xkb_codepoint(&fr, codepoint, &fr_sequence);
		assert(fr_sequence.key == expected_fr_keys[i]);
		assert(fr_sequence.key != us_sequence.key);
	}

	ydotool_type_resolver_destroy(&us);
	ydotool_type_resolver_destroy(&fr);
}

static void test_xkb_fr_latin9_text(void) {
	struct ydotool_xkb_names names = {.layout = "fr", .variant = "latin9"};
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	assert(ydotool_type_resolver_init_keymap(&resolver, &names, true) == YDOTOOL_TYPE_OK);

	assert_xkb_string(&resolver, "Élève déjà à Lomé");
	assert_xkb_string(&resolver, "français où êtes-vous");
	assert_xkb_string(&resolver, "À É È Ç Ù");
	assert_xkb_string(&resolver, "œ Œ");

	assert_xkb_codepoint(&resolver, 0x00c9, &sequence);
	assert(has_modifier(&sequence, KEY_LEFTSHIFT));

	ydotool_type_resolver_destroy(&resolver);
}

static void test_xkb_fr_oss_level3(void) {
	struct ydotool_xkb_names names = {.layout = "fr", .variant = "oss"};
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	assert(ydotool_type_resolver_init_keymap(&resolver, &names, true) == YDOTOOL_TYPE_OK);

	assert_xkb_codepoint(&resolver, 0x00ea, &sequence);
	assert(has_modifier(&sequence, KEY_RIGHTALT));
	assert_xkb_codepoint(&resolver, 0x0153, &sequence);
	assert(has_modifier(&sequence, KEY_RIGHTALT));

	ydotool_type_resolver_destroy(&resolver);
}

static void test_xkb_us_altgr_intl(void) {
	struct ydotool_xkb_names names = {.layout = "us", .variant = "altgr-intl"};
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	assert(ydotool_type_resolver_init_keymap(&resolver, &names, true) == YDOTOOL_TYPE_OK);

	assert_xkb_string(&resolver, "é œ É Œ");

	assert_xkb_codepoint(&resolver, 0x00e9, &sequence);
	assert(has_modifier(&sequence, KEY_RIGHTALT));
	assert_xkb_codepoint(&resolver, 0x0153, &sequence);
	assert(has_modifier(&sequence, KEY_RIGHTALT));

	ydotool_type_resolver_destroy(&resolver);
}

static void test_utf8_decoder(void) {
	const unsigned char eacute[] = {0xc3, 0xa9};
	const unsigned char oe[] = {0xc5, 0x93};
	const unsigned char continuation[] = {0x80};
	const unsigned char overlong[] = {0xc0, 0xaf};
	const unsigned char surrogate[] = {0xed, 0xa0, 0x80};
	const unsigned char too_large[] = {0xf4, 0x90, 0x80, 0x80};
	const unsigned char truncated[] = {0xe2, 0x82};
	struct ydotool_type_utf8_decoder decoder;
	uint32_t codepoint = 0;
	bool complete = false;

	assert_utf8_ok(eacute, sizeof(eacute), 0x00e9);
	assert_utf8_ok(oe, sizeof(oe), 0x0153);
	assert_utf8_invalid(continuation, sizeof(continuation));
	assert_utf8_invalid(overlong, sizeof(overlong));
	assert_utf8_invalid(surrogate, sizeof(surrogate));
	assert_utf8_invalid(too_large, sizeof(too_large));

	ydotool_type_utf8_init(&decoder);
	for (size_t i = 0; i < sizeof(truncated); i++) {
		assert(ydotool_type_utf8_feed(&decoder, truncated[i], &codepoint, &complete) == YDOTOOL_TYPE_OK);
	}
	assert(!complete);
	assert(ydotool_type_utf8_finish(&decoder) == YDOTOOL_TYPE_TRUNCATED_UTF8);

	ydotool_type_utf8_init(&decoder);
	assert(ydotool_type_utf8_feed(&decoder, 0xc3, &codepoint, &complete) == YDOTOOL_TYPE_OK);
	assert(!complete);
	assert(ydotool_type_utf8_feed(&decoder, 0xa9, &codepoint, &complete) == YDOTOOL_TYPE_OK);
	assert(complete);
	assert(codepoint == 0x00e9);
}

static void test_errors(void) {
	struct ydotool_xkb_names us_names = {.layout = "us"};
	struct ydotool_xkb_names bad_layout = {.layout = "not-a-real-layout"};
	struct ydotool_xkb_names bad_variant = {.layout = "us", .variant = "not-a-real-variant"};
	struct ydotool_type_resolver resolver;
	struct ydotool_type_sequence sequence;

	assert(ydotool_type_resolver_init_keymap(&resolver, &us_names, true) == YDOTOOL_TYPE_OK);
	assert(ydotool_type_resolve_codepoint(&resolver, 0x1f600, &sequence) == YDOTOOL_TYPE_NOT_TYPEABLE);
	ydotool_type_resolver_destroy(&resolver);

	assert(ydotool_type_resolver_init_keymap(&resolver, &bad_layout, true) == YDOTOOL_TYPE_KEYMAP_ERROR);
	assert(ydotool_type_resolver_init_keymap(&resolver, &bad_variant, true) == YDOTOOL_TYPE_KEYMAP_ERROR);
}

int main(void) {
	test_historical_ascii();
	test_xkb_us();
	test_xkb_fr_layout_aware_ascii();
	test_xkb_fr_latin9_text();
	test_xkb_fr_oss_level3();
	test_xkb_us_altgr_intl();
	test_utf8_decoder();
	test_errors();
	return 0;
}
