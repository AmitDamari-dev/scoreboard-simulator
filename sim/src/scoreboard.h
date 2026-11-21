/*
 * scoreboard.h
 *
 * Scoreboard core: the per-cycle pipeline
 *   Issue          - free unit of the type AND no WAW (no pending writer of Fi)
 *   Read Operands  - when Rj and Rk are ready (RAW); first exec cycle
 *   Execute        - lasts <delay> cycles, ends delay-1 cycles after RO
 *   Write Result   - one cycle after exec end, if no WAR hazard
 * plus memory hazards for LD/ST, and the HALT/end-of-simulation rule.
 */

#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "common.h"

/* TODO: declarations */

#endif /* SCOREBOARD_H */
