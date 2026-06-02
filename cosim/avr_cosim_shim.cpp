/* avr_cosim_shim.cpp — d_cosim co-simulation shim for VioAVR
 *
 * Shared library loaded by ngspice's d_cosim XSPICE code model.
 * Embeds 1..MAX_CHIPS VioAVR ISS instances in-process. No daemon or SHM.
 *
 * Netlist port vector size = NUM_CHIPS * 32.
 *   Single-chip: 32 d_in + 32 d_out (sim_args=["mcu"] or ["mcu","hex"])
 *   Dual-chip:   64 d_in + 64 d_out (sim_args=["m1","h1","m2","h2"])
 *   N-chip:     N*32 d_in + N*32 d_out (sim_args=["m1","h1",...,"mN","hN"])
 *
 * Pin mapping per chip (local ID 0-31):
 *   PORTA=0-7, PORTB=8-15, PORTC=16-23, PORTD=24-31
 * Global pin ID = chip_index * 32 + local_id.
 */

extern "C" {
#include "ngspice/cmtypes.h"
#include "ngspice/cosim.h"
}
#include "vioavr/core/viospice_c.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define MAX_CHIPS     8
#define PINS_PER_CHIP 32
#define MAX_PINS      (MAX_CHIPS * PINS_PER_CHIP)
#define MAX_TIMED_INJECTS 16

static const char* PORT_NAMES[4] = {"PORTA", "PORTB", "PORTC", "PORTD"};

struct ChipState {
    VioSpiceHandle avr;
    double         last_vtime;
    double         accumulated_time;
    Digital_t      prev_out[PINS_PER_CHIP];
};

/* Converts a hex nibble char to its value (0-15). Returns -1 on error. */
static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* Converts a hex string to raw bytes. Returns byte count, or -1 on error. */
static int hex_to_bytes(const char* hex, uint8_t* out, int max_out)
{
    int len = 0;
    while (*hex && len < max_out) {
        if (*hex == ' ' || *hex == '\t') { hex++; continue; }
        int hi = hex_nibble(hex[0]);
        int lo = hex_nibble(hex[1]);
        if (hi < 0 || lo < 0) return -1;
        out[len++] = (uint8_t)((hi << 4) | lo);
        hex += 2;
    }
    return len;
}

struct MultiCosimInst {
    struct ChipState chips[MAX_CHIPS];
    int              num_chips;
    /* Timed HC-05 data injections (from inject_at= sim_args) */
    double   inject_vtime[MAX_TIMED_INJECTS];
    uint8_t  inject_data[MAX_TIMED_INJECTS][256];
    int      inject_len[MAX_TIMED_INJECTS];
    int      inject_count;
    /* Live PTY for interactive Bluetooth data injection */
    int      pty_fd;
    char     pty_link_path[256]; /* symlink path to unlink on cleanup */
};

/* Process-global active handle (chip 0, for avr_adc_bridge) */
static VioSpiceHandle g_active_handle = NULL;

extern "C" VioSpiceHandle vioavr_cosim_get_active_handle(void)
{
    return g_active_handle;
}

static void shim_step(struct co_info* info)
{
    struct MultiCosimInst* multi = (struct MultiCosimInst*)info->handle;
    if (!multi) return;

    for (int c = 0; c < multi->num_chips; c++) {
        struct ChipState* chip = &multi->chips[c];
        double delta = info->vtime - chip->last_vtime;
        if (delta > 0) {
            chip->last_vtime = info->vtime;
            chip->accumulated_time += delta;
            const double min_step = 1.0e-6;
            if (chip->accumulated_time >= min_step) {
                vioavr_step_duration(chip->avr, chip->accumulated_time);
                chip->accumulated_time = 0.0;

                /* Read from live PTY (interactive Bluetooth injection) */
                if (multi->pty_fd >= 0) {
                    uint8_t ptybuf[64];
                    int r = (int)read(multi->pty_fd, ptybuf, sizeof(ptybuf));
                    if (r > 0) {
                        vioavr_hc05_inject_data(chip->avr, ptybuf, (uint16_t)r);
                    } else if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        fprintf(stderr, "[COSIM] PTY read error: %s\n", strerror(errno));
                    }
                }

                /* Fire any timed HC-05 injections whose vtime has been reached */
                if (multi->inject_count > 0) {
                    for (int i = 0; i < multi->inject_count; i++) {
                        double vt = multi->inject_vtime[i];
                        if (vt >= 0.0 && vt <= info->vtime) {
                            vioavr_hc05_inject_data(chip->avr,
                                multi->inject_data[i],
                                (uint16_t)multi->inject_len[i]);
                            fprintf(stderr, "[COSIM] HC-05 timed inject %d at %.6f s (%d bytes)\n",
                                    i, info->vtime, multi->inject_len[i]);
                            multi->inject_vtime[i] = -1.0; /* fire once */
                        }
                    }
                }

                int base_id = c * PINS_PER_CHIP;
                VioAvrPinChange changes[PINS_PER_CHIP * 2];
                int n = vioavr_consume_pin_changes(chip->avr, changes, PINS_PER_CHIP * 2);
                for (int i = 0; i < n; i++) {
                    unsigned int id = changes[i].external_id;
                    {}  // pin change callback
                    if (id >= (unsigned int)PINS_PER_CHIP) continue;

                    Digital_t val;
                    val.state    = (changes[i].level == VIOAVR_LEVEL_HIGH) ? ONE : ZERO;
                    val.strength = STRONG;

                    unsigned int global_id = base_id + id;
                    if (val.state != chip->prev_out[id].state ||
                        val.strength != chip->prev_out[id].strength) {
                        chip->prev_out[id] = val;
                        (*info->out_fn)(info, global_id, &val);
                    }
                }
            }
        }
    }
}

static void shim_input(struct co_info* info, unsigned int bit, Digital_t* value)
{
    struct MultiCosimInst* multi = (struct MultiCosimInst*)info->handle;
    if (!multi) return;

    VioAvrPinLevel level = (value->state == ONE)
                           ? VIOAVR_LEVEL_HIGH
                           : VIOAVR_LEVEL_LOW;

    int total_pins = multi->num_chips * PINS_PER_CHIP;
    if (bit >= (unsigned int)total_pins) return;

    int chip_idx = (int)bit / PINS_PER_CHIP;
    int local_id = (int)bit % PINS_PER_CHIP;
    vioavr_set_external_pin(multi->chips[chip_idx].avr, (unsigned int)local_id, level);
}

static void shim_cleanup(struct co_info* info)
{
    struct MultiCosimInst* multi = (struct MultiCosimInst*)info->handle;
    g_active_handle = NULL;
    if (multi) {
        if (multi->pty_fd >= 0) {
            close(multi->pty_fd);
            if (multi->pty_link_path[0])
                unlink(multi->pty_link_path);
        }
        for (int c = 0; c < multi->num_chips; c++) {
            if (multi->chips[c].avr) vioavr_destroy(multi->chips[c].avr);
        }
        free(multi);
        info->handle = NULL;
    }
}

static void nop_step(struct co_info* info) { (void)info; }
static void nop_input(struct co_info* info, unsigned int bit, Digital_t* value)
{ (void)info; (void)bit; (void)value; }
static void nop_cleanup(struct co_info* info) { (void)info; }

/* Parse sim_args:
 *   Array format: ["mcu1","hex1", "mcu2","hex2", ..., "mcuN","hexN"]
 *     sim_argc must be even, each pair is (mcu, hex).
 *     Optional jit flag as last odd arg: ["...","0"] or ["...","1"]
 *     Optional adc voltage as next arg:  ["...","...","3.3"]
 *
 *   Comma-string fallback: "mcu1,hex1,mcu2,hex2,...,mcuN,hexN"
 *
 * Returns number of chips parsed, or 0 on error.
 */
struct SimConfig {
    const char* mcu[MAX_CHIPS];
    const char* hex[MAX_CHIPS];
    bool        jit_enabled;
    bool        hc05_enabled;
    double      adc_voltage;
    int         num_chips;
    const char* data_hex;  /* hex-encoded Bluetooth data to inject, or NULL */
    const char* pty_path;  /* PTY device path for live injection, or NULL */
    int         inject_count;
    const char* inject_args[MAX_TIMED_INJECTS]; /* "vtime:hex" strings */
};

static int parse_sim_args(struct co_info* info, struct SimConfig* cfg)
{
    cfg->mcu[0] = "atmega328p";
    cfg->hex[0] = "firmware.hex";
    for (int i = 1; i < MAX_CHIPS; i++) {
        cfg->mcu[i] = NULL;
        cfg->hex[i] = NULL;
    }
    cfg->jit_enabled = true;
    cfg->hc05_enabled = false;
    cfg->adc_voltage = -1.0;
    cfg->num_chips = 0;
    cfg->data_hex = NULL;
    cfg->pty_path = NULL;
    cfg->inject_count = 0;
    for (int i = 0; i < MAX_TIMED_INJECTS; i++) cfg->inject_args[i] = NULL;

    if (info->sim_argc == 0) {
        cfg->num_chips = 1;
        return 1;
    }

    /* Array format: pairs of (mcu, hex) args, with optional trailing flags */
    if (info->sim_argc >= 2 && info->sim_argv[0] && info->sim_argv[1]) {
        int argc = info->sim_argc;
        int flags = 0;
        /* Scan trailing args for optional flags */
        for (int i = argc - 1; i >= 2; i--) {
            const char* a = info->sim_argv[i];
            if (!a) continue;
            if (a[0] == '1' && a[1] == '\0') { flags++; continue; }
            if (a[0] == '0' && a[1] == '\0') { flags++; continue; }
            /* Numeric voltage (e.g. "3.0") - must be non-negative, <= 5.0 */
            if (a[0] >= '0' && a[0] <= '5') {
                char* end = NULL;
                double v = strtod(a, &end);
                if (end != a && *end == '\0' && v >= 0.0 && v <= 5.0) { flags++; continue; }
            }
            if (strcmp(a, "hc05") == 0) { flags++; continue; }
            if (strncmp(a, "data=", 5) == 0) { flags++; continue; }
            if (strncmp(a, "inject_at=", 10) == 0) { flags++; continue; }
            if (strncmp(a, "pty_", 4) == 0) { flags++; continue; }
            break;
        }
        int possible = (argc - flags) / 2;
        if (possible > MAX_CHIPS) possible = MAX_CHIPS;
        if (possible >= 1) {
            int ci = 0;
            for (int i = 0; i < possible; i++) {
                int arg_idx = i * 2;
                if (arg_idx + 1 >= argc - flags) break;
                cfg->mcu[ci] = info->sim_argv[arg_idx];
                cfg->hex[ci] = info->sim_argv[arg_idx + 1];
                ci++;
            }
            if (ci > 0) {
                int next = ci * 2;
                while (next < argc && info->sim_argv[next]) {
                    const char* a = info->sim_argv[next];
                    if (a[0] == '1' && a[1] == '\0') {
                        cfg->jit_enabled = true;
                    } else if (a[0] == '0' && a[1] == '\0') {
                        cfg->jit_enabled = false;
                    } else if (strcmp(a, "hc05") == 0) {
                        cfg->hc05_enabled = true;
                    } else if (strncmp(a, "data=", 5) == 0) {
                        cfg->data_hex = a + 5;
                    } else if (strncmp(a, "inject_at=", 10) == 0) {
                        if (cfg->inject_count < MAX_TIMED_INJECTS)
                            cfg->inject_args[cfg->inject_count++] = a + 10;
                    } else if (strncmp(a, "pty_", 4) == 0) {
                        cfg->pty_path = a + 4;
                    } else {
                        double v = atof(a);
                        if (v >= 0.0 && v <= 5.0) cfg->adc_voltage = v;
                    }
                    next++;
                }
                cfg->num_chips = ci;
                return ci;
            }
        }
    }

    /* Fallback: comma-separated single string */
    if (info->sim_argv[0]) {
        const char* s = info->sim_argv[0];
        char parts[MAX_CHIPS * 2][256];
        int nparts = 0;
        int pos = 0;
        for (const char* p = s; nparts < MAX_CHIPS * 2; p++) {
            if (*p == ',' || *p == '\0') {
                parts[nparts][pos] = '\0';
                nparts++;
                pos = 0;
                if (*p == '\0') break;
            } else {
                if (pos < 255) parts[nparts][pos++] = *p;
            }
        }
        int pairs = nparts / 2;
        if (pairs > MAX_CHIPS) pairs = MAX_CHIPS;
        if (pairs >= 1) {
            int ci = 0;
            for (int i = 0; i < pairs && ci < MAX_CHIPS; i++) {
                if (parts[i * 2][0] == '\0' || parts[i * 2 + 1][0] == '\0')
                    break;
                char* m = (char*)malloc(strlen(parts[i * 2]) + 1);
                char* h = (char*)malloc(strlen(parts[i * 2 + 1]) + 1);
                if (!m || !h) { free(m); free(h); break; }
                strcpy(m, parts[i * 2]);
                strcpy(h, parts[i * 2 + 1]);
                cfg->mcu[ci] = m;
                cfg->hex[ci] = h;
                ci++;
            }
            if (ci > 0) {
                if (nparts >= ci * 2 + 1 && parts[ci * 2][0])
                    cfg->jit_enabled = (parts[ci * 2][0] == '1');
                cfg->num_chips = ci;
                return ci;
            }
        }
    }

    cfg->num_chips = 1;
    return 1;
}

static int setup_chip(struct ChipState* chip, const char* mcu,
                      const char* hex, bool jit, int chip_index)
{
    chip->avr = vioavr_create(mcu);
    if (!chip->avr) return 0;

    vioavr_enable_jit(chip->avr, jit);

    for (int p = 0; p < 4; p++)
        for (int b = 0; b < 8; b++)
            vioavr_add_pin_mapping(chip->avr, PORT_NAMES[p], b, p * 8 + b);

    if (!vioavr_load_hex(chip->avr, hex)) {
        vioavr_destroy(chip->avr);
        chip->avr = NULL;
        return 0;
    }

    chip->last_vtime = 0.0;
    chip->accumulated_time = 0.0;
    for (int i = 0; i < PINS_PER_CHIP; i++) {
        chip->prev_out[i].state    = ZERO;
        chip->prev_out[i].strength = STRONG;
    }

    vioavr_reset(chip->avr);

    VioAvrPinChange drain[64];
    while (vioavr_consume_pin_changes(chip->avr, drain, 64) > 0) {}

    return 1;
}

void Cosim_setup(struct co_info* info)
{
    info->handle      = NULL;
    info->in_count    = 0;
    info->out_count   = 0;
    info->inout_count = 0;
    info->step        = nop_step;
    info->in_fn       = nop_input;
    info->cleanup     = nop_cleanup;
    info->method      = Normal;

    struct SimConfig cfg;
    int n = parse_sim_args(info, &cfg);
    if (n <= 0) {
        fprintf(stderr, "[COSIM] No chips configured\n");
        return;
    }

    struct MultiCosimInst* multi = (struct MultiCosimInst*)
        calloc(1, sizeof(struct MultiCosimInst));
    if (!multi) return;

    multi->num_chips = 0;

    for (int c = 0; c < n; c++) {
        if (!setup_chip(&multi->chips[c], cfg.mcu[c], cfg.hex[c],
                        cfg.jit_enabled, c)) {
            for (int i = 0; i < c; i++) {
                if (multi->chips[i].avr) vioavr_destroy(multi->chips[i].avr);
            }
            free(multi);
            return;
        }
        multi->num_chips++;
    }

    if (cfg.adc_voltage >= 0.0 && multi->chips[0].avr) {
        vioavr_set_external_voltage(multi->chips[0].avr, 0, cfg.adc_voltage);
    }

    multi->inject_count = 0;
    for (int i = 0; i < MAX_TIMED_INJECTS; i++) multi->inject_vtime[i] = -1.0;

    if (cfg.hc05_enabled && multi->chips[0].avr) {
        vioavr_enable_hc05(multi->chips[0].avr);
        fprintf(stderr, "[COSIM] Native HC-05 enabled on chip 0\n");
        if (cfg.data_hex) {
            uint8_t hexbuf[2048];
            int n = hex_to_bytes(cfg.data_hex, hexbuf, (int)sizeof(hexbuf));
            if (n > 0) {
                vioavr_hc05_inject_data(multi->chips[0].avr, hexbuf, (uint16_t)n);
                fprintf(stderr, "[COSIM] Injected %d bytes of Bluetooth data\n", n);
            }
        }
        /* Parse timed inject_at= sim_args */
        for (int i = 0; i < cfg.inject_count && i < MAX_TIMED_INJECTS; i++) {
            const char* s = cfg.inject_args[i];
            if (!s) break;
            const char* colon = strchr(s, ':');
            if (!colon) {
                fprintf(stderr, "[COSIM] Warning: invalid inject_at format (missing ':'): %s\n", s);
                continue;
            }
            char vtime_str[64];
            int vlen = (int)(colon - s);
            if (vlen >= (int)sizeof(vtime_str)) vlen = sizeof(vtime_str) - 1;
            memcpy(vtime_str, s, vlen);
            vtime_str[vlen] = '\0';
            double vt = atof(vtime_str);
            if (vt < 0.0) {
                fprintf(stderr, "[COSIM] Warning: inject_at vtime < 0: %s\n", vtime_str);
                continue;
            }
            const char* hex_part = colon + 1;
            int idx = multi->inject_count;
            multi->inject_vtime[idx] = vt;
            int n = hex_to_bytes(hex_part, multi->inject_data[idx], 256);
            if (n <= 0) {
                fprintf(stderr, "[COSIM] Warning: inject_at hex parse error: %s\n", hex_part);
                multi->inject_vtime[idx] = -1.0;
                continue;
            }
            multi->inject_len[idx] = n;
            multi->inject_count = idx + 1;
            fprintf(stderr, "[COSIM] Timed inject %d: at %.6f s, %d bytes\n", idx, vt, n);
        }
    }

    /* Open PTY for live interactive Bluetooth data injection */
    multi->pty_fd = -1;
    multi->pty_link_path[0] = '\0';
    if (cfg.pty_path && cfg.hc05_enabled && multi->chips[0].avr) {
        int master = posix_openpt(O_RDWR | O_NOCTTY);
        if (master < 0) {
            fprintf(stderr, "[COSIM] PTY: posix_openpt() failed: %s\n", strerror(errno));
        } else {
            if (grantpt(master) != 0 || unlockpt(master) != 0) {
                fprintf(stderr, "[COSIM] PTY: grantpt/unlockpt failed: %s\n", strerror(errno));
                close(master);
            } else {
                const char* slave = ptsname(master);
                if (!slave) {
                    fprintf(stderr, "[COSIM] PTY: ptsname() failed\n");
                    close(master);
                } else {
                    unlink(cfg.pty_path);
                    if (symlink(slave, cfg.pty_path) != 0) {
                        fprintf(stderr, "[COSIM] PTY: symlink(%s -> %s) failed: %s\n",
                                cfg.pty_path, slave, strerror(errno));
                        close(master);
                    } else {
                        int flags = fcntl(master, F_GETFL, 0);
                        fcntl(master, F_SETFL, flags | O_NONBLOCK);
                        multi->pty_fd = master;
                        vioavr_set_hc05_pty_fd(multi->chips[0].avr, master);
                        size_t plen = strlen(cfg.pty_path);
                        if (plen >= sizeof(multi->pty_link_path))
                            plen = sizeof(multi->pty_link_path) - 1;
                        memcpy(multi->pty_link_path, cfg.pty_path, plen);
                        multi->pty_link_path[plen] = '\0';
                        fprintf(stderr, "[COSIM] PTY open: %s -> %s  (connect with: screen %s)\n",
                                cfg.pty_path, slave, cfg.pty_path);
                    }
                }
            }
        }
    }

    g_active_handle = multi->chips[0].avr;

    int total_pins = multi->num_chips * PINS_PER_CHIP;

    info->handle      = multi;
    info->in_count    = (unsigned int)total_pins;
    info->out_count   = (unsigned int)total_pins;
    info->step        = shim_step;
    info->in_fn       = shim_input;
    info->cleanup     = shim_cleanup;
}
