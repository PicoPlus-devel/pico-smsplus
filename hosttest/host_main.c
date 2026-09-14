// Linux host harness for the smsplus core (SMS / Game Gear / SG-1000).
//
// Usage: sms_host <rom> <frames> <dump-every> [options]
//   -o DIR     directory for frame_NNNNN.ppm dumps (default hosttest/out)
//   -l FILE    write the final frame to FILE (PPM)
//   -b FRAME   start periodic dumps at FRAME (default 0)
//   -i SCRIPT  input script, e.g. "B1:300-320,PAUSE:900"
//              names: UP DOWN LEFT RIGHT B1 B2 PAUSE START (player 1);
//              a single frame number holds the input for 6 frames
//   -s FRAME   at FRAME save a state, tear down, re-init and load it back
//              (the same sequence as Emulator_LoadState in state.cpp)
//   -r         print the VDP registers and status after the last frame
//   -v FILE    write the 16 KB VRAM to FILE after the last frame
//   -V         with every periodic frame dump, also write vram_NNNNN.bin
//
// dump-every 0 disables periodic dumps.
// Env: HOST_HEAP_LIMIT=<bytes> aborts when the core's heap use would exceed
// the limit. Allocation failure always aborts, like the Pico SDK's malloc.
//
// Cart type comes from the extension, case-sensitive like main.cpp:
// ".gg" Game Gear, ".sg" SG-1000, anything else Master System.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "shared.h"

#define FRAME_W 256
#define FRAME_H 192

static uint8_t frame_rgb[FRAME_H * FRAME_W * 3];
static uint8_t pal_rgb[PALETTE_SIZE][3];

/* ---- callbacks normally provided by main.cpp / pico_shared ------------- */

void sms_render_line(int line, const uint8_t *buffer)
{
	if (line < 0 || line >= FRAME_H) return;
	uint8_t *row = &frame_rgb[line * FRAME_W * 3];
	if (!buffer) {
		memset(row, 0, FRAME_W * 3);
		return;
	}
	for (int x = 0; x < FRAME_W; x++) {
		const uint8_t *c = pal_rgb[buffer[x] & (PALETTE_SIZE - 1)];
		*row++ = c[0];
		*row++ = c[1];
		*row++ = c[2];
	}
}

void sms_palette_sync(int index)
{
	pal_rgb[index][0] = ((vdp.cram[index] >> 0) & 3) * 85;
	pal_rgb[index][1] = ((vdp.cram[index] >> 2) & 3) * 85;
	pal_rgb[index][2] = ((vdp.cram[index] >> 4) & 3) * 85;
}

void sms_palette_syncGG(int index)
{
	pal_rgb[index][0] = ((vdp.cram[(index << 1) | 0] >> 0) & 15) * 17;
	pal_rgb[index][1] = ((vdp.cram[(index << 1) | 0] >> 4) & 15) * 17;
	pal_rgb[index][2] = ((vdp.cram[(index << 1) | 1] >> 0) & 15) * 17;
}

void sms_palette_syncSG(int index)
{
	memcpy(pal_rgb[index], tms_palette_rgb[index & 15], 3);
}

void system_load_sram(void)
{
}

/* Heap accounting. A small header in front of each block records its size. */
typedef struct { size_t size; size_t pad; } heap_hdr;
static size_t heap_now, heap_peak, heap_limit;

void *frens_f_malloc(size_t size)
{
	if (heap_limit && heap_now + size > heap_limit) {
		fprintf(stderr, "PANIC: heap limit %zu exceeded (in use %zu, request %zu)\n",
			heap_limit, heap_now, size);
		abort();
	}
	heap_hdr *h = malloc(sizeof(heap_hdr) + size);
	if (!h) {
		fprintf(stderr, "PANIC: malloc(%zu) failed\n", size);
		abort();
	}
	h->size = size;
	heap_now += size;
	if (heap_now > heap_peak) heap_peak = heap_now;
	return h + 1;
}

void frens_f_free(void *ptr)
{
	if (!ptr) return;
	heap_hdr *h = (heap_hdr *)ptr - 1;
	heap_now -= h->size;
	free(h);
}

/* ---- input script --------------------------------------------------------- */

typedef struct { int pad, sys, start, end; } input_event;
static input_event events[64];
static int n_events;

static void parse_input_script(const char *script)
{
	static const struct { const char *name; int pad, sys; } names[] = {
		{"UP", INPUT_UP, 0},       {"DOWN", INPUT_DOWN, 0},
		{"LEFT", INPUT_LEFT, 0},   {"RIGHT", INPUT_RIGHT, 0},
		{"B1", INPUT_BUTTON1, 0},  {"B2", INPUT_BUTTON2, 0},
		{"PAUSE", 0, INPUT_PAUSE}, {"START", 0, INPUT_START},
	};
	char *copy = strdup(script);
	for (char *tok = strtok(copy, ","); tok && n_events < 64; tok = strtok(NULL, ",")) {
		char name[16];
		int start, end;
		int n = sscanf(tok, "%15[A-Z0-9]:%d-%d", name, &start, &end);
		if (n < 2) {
			fprintf(stderr, "bad input token '%s'\n", tok);
			exit(2);
		}
		if (n == 2) end = start + 5;
		size_t i;
		for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
			if (strcmp(name, names[i].name) == 0) break;
		}
		if (i == sizeof(names) / sizeof(names[0])) {
			fprintf(stderr, "unknown input '%s'\n", name);
			exit(2);
		}
		events[n_events++] = (input_event){names[i].pad, names[i].sys, start, end};
	}
	free(copy);
}

static void apply_input(int frame)
{
	input.pad[0] = input.pad[1] = input.system = 0;
	for (int i = 0; i < n_events; i++) {
		if (frame >= events[i].start && frame <= events[i].end) {
			input.pad[0] |= events[i].pad;
			input.system |= events[i].sys;
		}
	}
}

/* ---- helpers --------------------------------------------------------------- */

static void write_ppm(const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) {
		perror(path);
		return;
	}
	fprintf(f, "P6\n%d %d\n255\n", FRAME_W, FRAME_H);
	fwrite(frame_rgb, 1, sizeof(frame_rgb), f);
	fclose(f);
}

static void write_vram(const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) {
		perror(path);
		return;
	}
	fwrite(vdp.vram, 1, sizeof(vdp.vram), f);
	fclose(f);
}

static int endswith(const char *s, const char *suffix)
{
	size_t ls = strlen(s), lx = strlen(suffix);
	return ls > lx && strcmp(s + ls - lx, suffix) == 0;
}

static int state_round_trip(const char *path)
{
	FIL fil;
	if (f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return 0;
	bool saved = system_save_state(&fil);
	f_close(&fil);
	if (!saved) return 0;

	system_shutdown();
	system_init(SMS_AUD_RATE);
	if (f_open(&fil, path, FA_READ) != FR_OK) return 0;
	bool loaded = system_load_state(&fil);
	f_close(&fil);
	return loaded;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <rom> <frames> <dump-every> [-o dir] [-l last.ppm] [-i script] [-s frame]\n", argv[0]);
		return 2;
	}
	const char *rom_path = argv[1];
	int total_frames = atoi(argv[2]);
	int dump_every = atoi(argv[3]);
	const char *outdir = "hosttest/out";
	const char *last_path = NULL;
	int state_at = -1;
	int dump_regs = 0;
	int dump_vram_periodic = 0;
	int dump_from = 0;
	const char *vram_path = NULL;

	for (int i = 4; i < argc; i++) {
		if (!strcmp(argv[i], "-r")) dump_regs = 1;
		else if (!strcmp(argv[i], "-V")) dump_vram_periodic = 1;
		else if (!strcmp(argv[i], "-v") && i + 1 < argc) vram_path = argv[++i];
		else if (!strcmp(argv[i], "-o") && i + 1 < argc) outdir = argv[++i];
		else if (!strcmp(argv[i], "-l") && i + 1 < argc) last_path = argv[++i];
		else if (!strcmp(argv[i], "-b") && i + 1 < argc) dump_from = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-i") && i + 1 < argc) parse_input_script(argv[++i]);
		else if (!strcmp(argv[i], "-s") && i + 1 < argc) state_at = atoi(argv[++i]);
		else {
			fprintf(stderr, "unknown option '%s'\n", argv[i]);
			return 2;
		}
	}
	const char *limit = getenv("HOST_HEAP_LIMIT");
	if (limit) heap_limit = strtoul(limit, NULL, 0);

	FILE *f = fopen(rom_path, "rb");
	if (!f) {
		perror(rom_path);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	/* Pad to whole 16 KB pages (min 64 KB) with 0xFF, like erased flash. The
	   ROM image is not on the heap on the device, so it is not accounted. */
	size_t bufsize = size < 0x10000 ? 0x10000 : (size_t)((size + 0x3FFF) & ~0x3FFF);
	uint8_t *rom = malloc(bufsize);
	memset(rom, 0xFF, bufsize);
	if (fread(rom, 1, size, f) != (size_t)size) {
		fprintf(stderr, "short read on %s\n", rom_path);
		return 1;
	}
	fclose(f);

	int type = endswith(rom_path, ".gg") ? TYPE_GG : endswith(rom_path, ".sg") ? TYPE_SG : TYPE_SMS;
	static const char *type_names[] = {"SMS", "GG", "SG"};

	load_rom((uintptr_t)rom, (int)size, type);
	system_init(SMS_AUD_RATE);
	system_reset();
	printf("cart: type=%s size=%d pages=%d\n", type_names[type], cart.size, cart.pages);

	mkdir(outdir, 0755);
	char path[1024];
	for (int frame = 0; frame < total_frames; frame++) {
		if (frame == state_at) {
			snprintf(path, sizeof(path), "%s/state.bin", outdir);
			int ok = state_round_trip(path);
			printf("state round-trip at frame %d: %s\n", frame, ok ? "ok" : "FAILED");
			if (!ok) return 1;
		}
		apply_input(frame);
		uint8_t adaptor = sms.port_3F;
		sms_frame(0);
		if (type == TYPE_SG && sms.port_3F != adaptor)
			printf("frame %d: RAM adaptor pages now 0x%02X\n", frame, sms.port_3F);
		if (dump_every > 0 && frame + 1 >= dump_from && (frame + 1) % dump_every == 0) {
			snprintf(path, sizeof(path), "%s/frame_%05d.ppm", outdir, frame + 1);
			write_ppm(path);
			if (dump_vram_periodic) {
				snprintf(path, sizeof(path), "%s/vram_%05d.bin", outdir, frame + 1);
				write_vram(path);
			}
		}
	}
	if (last_path) write_ppm(last_path);
	if (dump_regs) {
		printf("vdp reg:");
		for (int r = 0; r < 11; r++) printf(" %02X", vdp.reg[r]);
		printf("  status %02X\n", vdp.status);
	}
	if (vram_path) write_vram(vram_path);

	system_shutdown();
	printf("heap: peak=%zu in-use-after-shutdown=%zu\n", heap_peak, heap_now);
	free(rom);
	return 0;
}
