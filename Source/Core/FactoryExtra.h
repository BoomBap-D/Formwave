#pragma once
// Second factory bank: 133 more starting points (200 factory presets in total), written with the
// same builder as FactoryData.h. Grouped by category, each tagged by the style it suits.
#include "FactoryData.h"

namespace fw
{
template <class Add>
inline void addExtraPresets (Add&& add)
{
    using W = AWave; using K = ANoise; using F = AFilt;
    auto click = [] (AuthoringPreset& p, float hz, float l, float dec) { p.noise (K::White, F::Hp, hz, 0.7f, l, dec); };

    // ---- Kicks (+11) ----
    click (add ("Kicks", "Boom-bap Kick").D (62, .34f).osc (W::Sine).osc (W::Tri, .3f).pitchEnv (12, .035f).filter (F::Lp, 1400).drv (1.1f), 2500, .08f, .008f);
    add ("Kicks", "SP Grit Kick").D (58, .3f).osc (W::Tri).pitchEnv (16, .03f).filter (F::Lp, 2200).drv (3.5f);
    add ("Kicks", "G-Funk Kick").D (54, .5f).osc (W::Sine).osc (W::Sine, .4f, 2).pitchEnv (15, .06f).drv (.8f);
    click (add ("Kicks", "Miami Bass Kick").D (44, 1.1f).osc (W::Sine).pitchEnv (22, .08f).drv (1.8f), 2000, .1f, .01f);
    add ("Kicks", "Plush Sub Kick").D (48, .7f).osc (W::Sine).pitchEnv (9, .05f).drv (.3f);
    click (add ("Kicks", "UK Drill Kick").D (52, .26f).osc (W::Sine).pitchEnv (40, .02f).drv (3.2f), 3000, .25f, .005f);
    click (add ("Kicks", "Rage Kick").D (50, .45f).osc (W::Sine).osc (W::Saw, .2f).pitchEnv (34, .03f).drv (4.5f), 3200, .15f, .008f);
    add ("Kicks", "Lofi Thud").D (66, .3f).osc (W::Sine).pitchEnv (7, .04f).filter (F::Lp, 500).noise (K::Pink, F::Lp, 700, .7f, .06f, .25f);
    click (add ("Kicks", "Hard Beater").D (70, .22f).osc (W::Tri).pitchEnv (18, .02f).drv (2.5f), 5000, .35f, .01f);
    add ("Kicks", "Round Kick").D (56, .4f).osc (W::Sine).pitchEnv (10, .045f).drv (.9f);
    add ("Kicks", "Deep Pocket Kick").D (50, .6f).osc (W::Sine).osc (W::Sine, .2f, 2, 0, .1f).pitchEnv (11, .07f).filter (F::Lp, 900).drv (.7f);

    // ---- 808s (+10) ----
    add ("808s", "808 Memphis").D (61.74f, 1.3f).osc (W::Sine).pitchEnv (9, .04f).filter (F::Lp, 700).drv (2.2f);
    add ("808s", "808 Atlanta").D (65.41f, 1.8f).osc (W::Sine).pitchEnv (16, .035f).drv (1.5f);
    add ("808s", "808 Houston Slab").D (55, 2.4f).osc (W::Sine).osc (W::Sine, .3f, 2).pitchEnv (6, .07f).drv (.6f);
    add ("808s", "808 UK Drill Slide").D (58.27f, 1.4f).osc (W::Sine).pitchEnv (-14, .35f).drv (2.4f);
    add ("808s", "808 Chicago Drill").D (65.41f, 1.2f).osc (W::Sine).osc (W::Tri, .25f).pitchEnv (10, .03f).drv (3.4f);
    add ("808s", "808 Clean Sub").D (65.41f, 2).osc (W::Sine).pitchEnv (3, .05f).drv (.15f);
    add ("808s", "808 Overdriven").D (65.41f, 1.6f).osc (W::Sine).osc (W::Square, .12f).pitchEnv (12, .03f).drv (6);
    add ("808s", "808 Click Layer").D (65.41f, 1.3f).osc (W::Sine).pitchEnv (18, .025f).drv (1.4f).noise (K::White, F::Hp, 3000, .7f, .15f, .01f);
    add ("808s", "808 Tight Bounce").D (65.41f, .8f).osc (W::Sine).pitchEnv (14, .03f).drv (1.6f);
    add ("808s", "808 Cowbell Tone").D (65.41f, 1.2f).osc (W::Sine).osc (W::Sine, .35f, 3.01f, 0, .3f).pitchEnv (8, .03f).drv (1.4f);

    // ---- Snares (+8) ----
    add ("Snares", "SP Snare").D (190, .22f).osc (W::Tri, 1, 1, 0, .08f).noise (K::White, F::Bp, 3400, .8f, 1, .13f).drv (2.5f);
    add ("Snares", "Tight Snare").D (210, .15f).osc (W::Tri, 1, 1, 0, .06f).pitchEnv (6, .015f).noise (K::White, F::Hp, 2500, .7f, 1, .1f);
    add ("Snares", "Fat Snare").D (160, .38f).osc (W::Tri, 1, 1, 0, .14f).osc (W::Sine, .5f, 1.5f, 0, .1f).pitchEnv (8, .03f).noise (K::White, F::Bp, 2400, .6f, 1, .28f).drv (1.3f);
    add ("Snares", "Rimshot").D (420, .1f).osc (W::Sine, 1, 1, 0, .05f).osc (W::Tri, .5f, 1.6f, 0, .04f).noise (K::White, F::Bp, 3000, 1, .5f, .05f).drv (1.5f);
    add ("Snares", "Gated Snare").D (200, .3f).osc (W::Tri, 1, 1, 0, .12f).noise (K::White, F::Bp, 2800, .6f, 1.1f, .3f).drv (1.1f).filter (F::Lp, 9000);
    add ("Snares", "Rage Snare").D (230, .26f).osc (W::Sine, 1, 1, 0, .08f).noise (K::White, F::Hp, 3500, .5f, 1.2f, .2f).drv (4.5f);
    add ("Snares", "Lofi Snare").D (175, .3f).osc (W::Tri, 1, 1, 0, .1f).noise (K::Pink, F::Bp, 2000, .7f, .9f, .22f).filter (F::Lp, 5500).drv (.9f);
    add ("Snares", "Brush Snare").D (200, .28f).noise (K::Pink, F::Bp, 4200, .5f, 1, .22f, .02f);

    // ---- Claps (+4) ----
    add ("Claps", "Trap Clap").D (200, .32f).noise (K::White, F::Bp, 1600, 1, 1.1f, .24f).bursts (4, .013f).drv (1.4f);
    add ("Claps", "Wide Clap").D (200, .45f).noise (K::White, F::Bp, 1400, .8f, 1.1f, .36f).bursts (5, .014f);
    add ("Claps", "Snap").D (1800, .08f).osc (W::Sine, 1, 1, 0, .03f).noise (K::White, F::Hp, 3500, 1, .9f, .04f);
    add ("Claps", "Lofi Clap").D (200, .26f).noise (K::Pink, F::Bp, 1300, 1, 1, .18f).bursts (3, .012f).filter (F::Lp, 6000);

    // ---- Hats & cymbals (+8) ----
    add ("Hats", "Pedal Hat").D (200, .045f).sq6 (.9f).filter (F::Hp, 6500, .9f);
    add ("Hats", "Half-open Hat").D (200, .2f).sq6().filter (F::Hp, 7000, .9f);
    add ("Hats", "Lofi Hat").D (200, .08f).sq6().filter (F::Bp, 7500, .6f).drv (.5f);
    add ("Hats", "Sizzle Hat").D (200, .3f).sq6 (1.1f).filter (F::Hp, 8500, .8f).noise (K::White, F::Hp, 9000, .7f, .5f, .25f);
    add ("Hats", "Drill Hat").D (200, .04f).sq6 (1.15f).filter (F::Hp, 9500, .9f).drv (1.4f);
    add ("Hats", "Brush Cymbal").D (200, 1.2f).noise (K::White, F::Hp, 5500, .6f, .7f, 1, .05f);
    add ("Hats", "Splash").D (200, .9f).sq6 (1.3f).filter (F::Hp, 6000, .7f).noise (K::White, F::Hp, 5000, .7f, .8f, .7f);
    add ("Hats", "Dark Ride").D (200, 1.8f).sq6 (1.2f).fixedOsc (W::Sine, 2400, 1.5f, 1.2f).filter (F::Bp, 5500, .5f);

    // ---- Percussion (+10) ----
    click (add ("Perc", "High Tom").D (230, .32f).osc (W::Sine).pitchEnv (9, .05f).drv (.5f), 3000, .08f, .01f);
    add ("Perc", "Floor Tom").D (85, .7f).osc (W::Sine).osc (W::Tri, .2f, 1.5f, 0, .15f).pitchEnv (10, .09f).drv (.6f);
    add ("Perc", "Bongo").D (420, .16f).osc (W::Sine, 1, 1, 0, .12f).osc (W::Tri, .25f, 2, 0, .05f).pitchEnv (3, .02f);
    add ("Perc", "Woodblock").D (900, .1f).osc (W::Sine, 1, 1, 0, .07f).osc (W::Square, .2f, 2.4f, 0, .03f).filter (F::Bp, 1800, 3);
    add ("Perc", "Triangle").D (200, 1.4f).fixedOsc (W::Sine, 2800, 1, 1.2f).fixedOsc (W::Sine, 4400, .6f, 1).fixedOsc (W::Sine, 6100, .4f, .8f);
    add ("Perc", "Rim Tick").D (1500, .05f).osc (W::Sine, 1, 1, 0, .03f).noise (K::White, F::Hp, 4000, .8f, .6f, .02f);
    add ("Perc", "Cabasa").D (200, .14f).noise (K::White, F::Bp, 7500, 1.2f, 1, .09f, .03f);
    add ("Perc", "Timbale").D (500, .28f).osc (W::Sine, 1, 1, 0, .2f).osc (W::Square, .3f, 1.47f, 0, .08f).pitchEnv (3, .02f).filter (F::Bp, 2500, 1.5f);
    add ("Perc", "Agogo").D (200, .5f).fixedOsc (W::Sine, 700, 1, .4f).fixedOsc (W::Sine, 1050, .8f, .3f);
    add ("Perc", "Kalimba Tick").D (700, .35f).osc (W::Sine, 1, 1, 0, .3f).osc (W::Sine, .3f, 5.4f, 0, .06f);

    // ---- Bass (+16) ----
    add ("Bass", "Rubber Bass").N (-2, .003f, .22f, .35f, .1f).osc (W::Tri).osc (W::Square, .4f, 1).filter (F::Lp, 500, 3, 2, .18f);
    add ("Bass", "Square Sub").N (-2, .004f, .1f, .9f, .1f).osc (W::Square).filter (F::Lp, 260, .7f);
    add ("Bass", "Fat Analog Bass").N (-2, .005f, .3f, .75f, .12f).osc (W::Saw, 1, 1, -6).osc (W::Saw, 1, 1, 6).osc (W::Sine, .8f, .5f).filter (F::Lp, 700, 3, 2, .3f);
    add ("Bass", "Acid Squelch").N (-2, .002f, .15f, .5f, .08f).osc (W::Saw).filter (F::Lp, 300, 9, 4, .2f).drv (1.4f);
    add ("Bass", "G-Funk Sine Bass").N (-2, .01f, .15f, .85f, .14f).osc (W::Sine).osc (W::Sine, .25f, 2).slide (.06f);
    add ("Bass", "Dub Sub").N (-2, .02f, .1f, 1, .3f).osc (W::Sine).drv (.3f);
    add ("Bass", "Pluck Sub").N (-2, .002f, .3f, 0, .1f).osc (W::Sine).osc (W::Tri, .4f, 2, 0, .08f);
    add ("Bass", "Growl Bass").N (-2, .01f, .1f, 1, .15f).osc (W::Saw).osc (W::Square, .7f, 1, 12).filter (F::Lp, 500, 5).filterLfo (4, 1800).drv (2);
    add ("Bass", "Neuro Bass").N (-2, .01f, .2f, .9f, .2f).osc (W::Saw, 1, 1, -18).osc (W::Saw, 1, 1, 18).filter (F::Lp, 700, 4).filterLfo (.7f, 1600).drv (2.5f);
    add ("Bass", "Distorted Sub").N (-2, .005f, .15f, .9f, .1f).osc (W::Sine).osc (W::Saw, .2f).drv (4);
    add ("Bass", "FM Pluck Bass").N (-2, .002f, .16f, 0, .08f).osc (W::Sine).fmod (2, 3, .12f);
    add ("Bass", "Moog Sub Bass").N (-2, .004f, .35f, .65f, .1f).osc (W::Saw).osc (W::Sine, 1, .5f).filter (F::Lp, 380, 2, 1.5f, .3f);
    add ("Bass", "Reggae Bass").N (-2, .01f, .25f, .6f, .1f).osc (W::Sine).osc (W::Tri, .5f, 1).filter (F::Lp, 350, .7f);
    add ("Bass", "Slap Bass").N (-1, .001f, .12f, .15f, .06f).osc (W::Saw).osc (W::Square, .4f, 2).filter (F::Bp, 1200, 1.5f, 2, .1f);
    add ("Bass", "Warm Sub Pad Bass").N (-2, .08f, .3f, .9f, .4f).osc (W::Sine).osc (W::Tri, .4f, 1, 4).filter (F::Lp, 400);
    add ("Bass", "Chip Bass").N (-1, .002f, .1f, .8f, .05f).osc (W::Square).vibrato (5, 6, .3f);

    // ---- Keys (+16) ----
    add ("Keys", "Soul EP").N (-1, .002f, 1.2f, .2f, .6f).osc (W::Sine).osc (W::Sine, .25f, 2).fmod (14, .22f, .1f).vibrato (4.5f, 5, .3f);
    add ("Keys", "Dark EP").N (-1, .003f, .8f, .2f, .4f).osc (W::Sine).osc (W::Tri, .3f, 1).fmod (14, .1f, .08f).filter (F::Lp, 1800).drv (.6f);
    add ("Keys", "Tine Bells EP").N (0, .001f, 1.4f, .1f, .7f).osc (W::Sine).fmod (7, .6f, .2f);
    add ("Keys", "Dirty Wurli").N (-1, .002f, .7f, .3f, .2f).osc (W::Tri).osc (W::Square, .2f, 2).drv (3.5f);
    add ("Keys", "Jazz Organ").N (-1, .008f, .01f, 1, .05f).osc (W::Sine, 1, .5f).osc (W::Sine, 1).osc (W::Sine, .5f, 3).osc (W::Sine, .35f, 4).osc (W::Sine, .2f, 6).vibrato (6.5f, 8, .1f);
    add ("Keys", "Church Organ").N (-1, .06f, .01f, 1, .15f).osc (W::Sine, .8f, .5f).osc (W::Sine).osc (W::Sine, .7f, 2).osc (W::Sine, .6f, 3).osc (W::Sine, .5f, 4).osc (W::Sine, .4f, 6).filter (F::Lp, 4000);
    add ("Keys", "Soft Piano").N (-1, .004f, 2.4f, 0, .3f).osc (W::Sine, 1, 1, 0, 2.2f).osc (W::Sine, .4f, 2, 0, 1.2f).osc (W::Sine, .15f, 3, 0, .6f).filter (F::Lp, 2200);
    add ("Keys", "Bright Piano").N (-1, .001f, 1.8f, 0, .2f).osc (W::Sine, 1, 1, 0, 1.8f).osc (W::Sine, .6f, 2, 0, 1).osc (W::Sine, .4f, 3, 0, .7f).osc (W::Sine, .25f, 4, 0, .5f).osc (W::Sine, .15f, 5, 0, .3f);
    add ("Keys", "Honky Piano").N (-1, .001f, 1.5f, 0, .2f).osc (W::Tri, 1, 1, -8, 1.5f).osc (W::Tri, 1, 1, 8, 1.5f).osc (W::Sine, .4f, 2, 0, .6f);
    add ("Keys", "Harpsichord").N (0, .001f, .6f, 0, .05f).osc (W::Saw).osc (W::Saw, .6f, 2, 4).filter (F::Bp, 2400, 1.2f, 1, .1f);
    add ("Keys", "Celesta").N (1, .001f, 1.6f, 0, .5f).osc (W::Sine).osc (W::Sine, .3f, 4, 0, .3f).osc (W::Sine, .15f, 6, 0, .15f);
    add ("Keys", "Marimba").N (0, .001f, .7f, 0, .1f).osc (W::Sine, 1, 1, 0, .7f).osc (W::Sine, .4f, 4, 0, .12f);
    add ("Keys", "Music Box").N (1, .001f, 1.3f, 0, .6f).osc (W::Sine).osc (W::Sine, .3f, 3, 0, .5f).osc (W::Sine, .2f, 5.1f, 0, .2f);
    add ("Keys", "Accordion").N (-1, .05f, .1f, .9f, .1f).osc (W::Saw, 1, 1, -7).osc (W::Saw, 1, 1, 7).osc (W::Square, .4f, 2).filter (F::Bp, 1800, .8f).vibrato (5.5f, 8, .2f);
    add ("Keys", "Rhodes Stab").N (-1, .002f, .45f, 0, .15f).osc (W::Sine).osc (W::Sine, .3f, 2).fmod (14, .2f, .06f);
    add ("Keys", "Lofi Keys").N (-1, .003f, .9f, .15f, .35f).osc (W::Tri).osc (W::Sine, .3f, 2).filter (F::Lp, 1400).drv (.8f).vibrato (3, 18, .1f);

    // ---- Leads (+13) ----
    add ("Leads", "Detuned Saw Lead").N (0, .01f, .1f, .85f, .2f).osc (W::Saw, 1, 1, -12).osc (W::Saw, 1, 1, 0).osc (W::Saw, 1, 1, 12).filter (F::Lp, 4500, 1);
    add ("Leads", "Square Lead").N (0, .005f, .1f, .8f, .1f).osc (W::Square).osc (W::Square, .4f, 1, 7).filter (F::Lp, 3000, 1.2f).vibrato (5.5f, 8, .25f);
    add ("Leads", "Synth Brass Lead").N (0, .04f, .15f, .8f, .15f).osc (W::Saw, 1, 1, -5).osc (W::Saw, 1, 1, 5).filter (F::Lp, 1200, 1.5f, 2.5f, .25f).drv (.5f);
    add ("Leads", "Flute Lead").N (1, .07f, .1f, .85f, .15f).osc (W::Sine).osc (W::Sine, .1f, 2).noise (K::Pink, F::Bp, 4000, 1, .05f, 0).vibrato (5, 20, .3f);
    add ("Leads", "Portamento Lead").N (0, .01f, .1f, .9f, .1f).osc (W::Saw).osc (W::Sine, .6f, .5f).filter (F::Lp, 2800, 1).slide (.15f);
    add ("Leads", "Dark Trap Lead").N (0, .003f, .25f, .5f, .2f).osc (W::Saw, 1, 1, -9).osc (W::Saw, 1, 1, 9).filter (F::Lp, 1300, 2, 2, .3f);
    add ("Leads", "Eerie Lead").N (1, .15f, .2f, .8f, .5f).osc (W::Sine).osc (W::Sine, .4f, 1.5f, 8).vibrato (4, 30, .3f);
    add ("Leads", "Detune Bell Lead").N (1, .001f, 1.8f, 0, .5f).osc (W::Sine).fmod (3.5f, 2, 1).osc (W::Sine, .3f, 1, 8);
    add ("Leads", "Pan Flute").N (1, .05f, .08f, .8f, .1f).osc (W::Tri).noise (K::White, F::Bp, 3500, 2, .08f, 0).vibrato (5, 12, .25f);
    add ("Leads", "Sine Lead").N (0, .01f, .1f, .9f, .12f).osc (W::Sine).vibrato (5.5f, 14, .2f);
    add ("Leads", "Mono Lead").N (0, .005f, .1f, .85f, .08f).osc (W::Saw).osc (W::Saw, .8f, 1, 8).filter (F::Lp, 2600, 2, 1.5f, .2f).slide (.04f);
    add ("Leads", "Sync-ish Lead").N (0, .003f, .15f, .7f, .1f).osc (W::Saw).fmod (2, 2, .3f).filter (F::Lp, 3800, 1);
    add ("Leads", "Wavy Lead").N (0, .02f, .1f, .85f, .15f).osc (W::Tri).osc (W::Saw, .5f, 1, 6).filterLfo (3, 1200).filter (F::Lp, 2200, 3).vibrato (6, 10, .2f);

    // ---- Pads (+12) ----
    add ("Pads", "Lush Strings Pad").N (-1, .6f, .4f, .9f, 1.3f).osc (W::Saw, 1, 1, -9).osc (W::Saw, 1, 1, -3).osc (W::Saw, 1, 1, 3).osc (W::Saw, 1, 1, 9).filter (F::Lp, 1800, .7f).vibrato (5, 6, .5f);
    add ("Pads", "Soul Choir").N (0, .4f, .4f, .85f, 1).osc (W::Tri).osc (W::Tri, .5f, 2, 6).osc (W::Sine, .3f, 3).filter (F::Bp, 1500, 1.2f).vibrato (5, 12, .3f);
    add ("Pads", "Dream Pad").N (-1, 1.1f, .6f, .85f, 2).osc (W::Sine).osc (W::Sine, .6f, 2, 5).osc (W::Sine, .4f, 3, -5).fmod (2, .6f, 2);
    add ("Pads", "Dusty Tape Pad").N (-1, .6f, .5f, .85f, 1.4f).osc (W::Saw, .7f, 1, -8).osc (W::Saw, .7f, 1, 8).filter (F::Lp, 900).noise (K::Pink, F::Lp, 2000, .7f, .04f, 0).vibrato (.8f, 25, 0);
    add ("Pads", "Ambient Pad").N (-1, 1.5f, .8f, .8f, 2.5f).osc (W::Sine).osc (W::Sine, .6f, 1.5f).osc (W::Sine, .4f, 2).osc (W::Sine, .3f, 3);
    add ("Pads", "Sweep Pad").N (-1, .8f, .6f, .85f, 1.5f).osc (W::Saw, 1, 1, -8).osc (W::Saw, 1, 1, 8).filter (F::Lp, 500, 3, 2, 1.2f).filterLfo (.2f, 1400);
    add ("Pads", "Cinematic Pad").N (-1, 1, .5f, .9f, 2.2f).osc (W::Saw, 1, 1, -10).osc (W::Saw, 1, .5f, 10).osc (W::Tri, .6f, 2).filter (F::Lp, 1200, 1).filterLfo (.1f, 600);
    add ("Pads", "Hollow Pad").N (-1, .5f, .4f, .8f, 1.2f).osc (W::Square, 1, 1, -6).osc (W::Square, 1, 1, 6).filter (F::Lp, 900, 1);
    add ("Pads", "Bell Pad").N (0, .4f, .5f, .7f, 1.8f).osc (W::Sine).fmod (3.5f, 1.2f, 1.8f).osc (W::Sine, .4f, 2);
    add ("Pads", "Night Drive Pad").N (-1, .5f, .5f, .9f, 1.4f).osc (W::Saw, 1, 1, -12).osc (W::Saw, 1, 1, 0).osc (W::Saw, 1, 1, 12).filter (F::Lp, 1500, 1).filterLfo (.3f, 500).vibrato (5, 5, .4f);
    add ("Pads", "Wide Saw Pad").N (-1, .3f, .4f, .9f, 1).osc (W::Saw, 1, 1, -20).osc (W::Saw, 1, 1, -7).osc (W::Saw, 1, 1, 7).osc (W::Saw, 1, 1, 20).filter (F::Lp, 2400, .7f);
    add ("Pads", "Breath Pad").N (-1, .9f, .5f, .8f, 1.6f).osc (W::Sine).osc (W::Sine, .4f, 2).noise (K::Pink, F::Bp, 2500, 1.5f, .2f, 0, .3f).filter (F::Lp, 2000);

    // ---- Plucks (new category, 8) ----
    add ("Plucks", "Soft Pluck").N (0, .002f, .5f, 0, .15f).osc (W::Tri).filter (F::Lp, 2000, 1, 2, .2f);
    add ("Plucks", "Guitar-ish Pluck").N (-1, .001f, .8f, 0, .2f).osc (W::Saw).osc (W::Saw, .5f, 2).filter (F::Lp, 1800, 1.5f, 2.5f, .4f);
    add ("Plucks", "Harp Pluck").N (0, .001f, 1.2f, 0, .4f).osc (W::Sine, 1, 1, 0, 1.2f).osc (W::Sine, .4f, 2, 0, .5f).osc (W::Sine, .2f, 3, 0, .3f);
    add ("Plucks", "Koto-ish Pluck").N (0, .001f, .9f, 0, .2f).osc (W::Saw).filter (F::Bp, 2200, 3, 2, .25f).pitchEnv (2, .03f);
    add ("Plucks", "Bubble Pluck").N (0, .001f, .3f, 0, .1f).osc (W::Sine).pitchEnv (12, .06f);
    add ("Plucks", "Trap Pluck").N (0, .002f, .35f, 0, .12f).osc (W::Square).osc (W::Saw, .5f, 1, 7).filter (F::Lp, 2600, 2, 3, .2f);
    add ("Plucks", "Mallet Pluck").N (0, .001f, .6f, 0, .15f).osc (W::Sine, 1, 1, 0, .6f).osc (W::Sine, .5f, 3, 0, .1f);
    add ("Plucks", "Sitar-ish Pluck").N (-1, .001f, 1, 0, .3f).osc (W::Saw).filter (F::Bp, 1500, 6, 2, .5f).filterLfo (6, 300).drv (1.3f);

    // ---- Strings & Brass (new category, 10) ----
    add ("Strings & Brass", "Chamber Strings").N (0, .2f, .3f, .85f, .5f).osc (W::Saw, 1, 1, -6).osc (W::Saw, 1, 1, 6).filter (F::Lp, 2600, .7f).vibrato (5.5f, 10, .3f);
    add ("Strings & Brass", "Staccato Strings").N (0, .01f, .15f, 0, .1f).osc (W::Saw, 1, 1, -5).osc (W::Saw, 1, 1, 5).filter (F::Lp, 2400, .8f, 1, .1f);
    add ("Strings & Brass", "Pizzicato").N (0, .001f, .2f, 0, .08f).osc (W::Saw).filter (F::Lp, 1800, 1, 2, .1f);
    add ("Strings & Brass", "Cello Swell").N (-1, .3f, .3f, .9f, .6f).osc (W::Saw, 1, 1, -4).osc (W::Saw, 1, 1, 4).filter (F::Lp, 900, 1, 1, .5f).vibrato (5, 14, .3f);
    add ("Strings & Brass", "Violin Lead").N (0, .12f, .2f, .85f, .3f).osc (W::Saw, 1, 1, -3).osc (W::Saw, 1, 1, 3).filter (F::Lp, 3600, .8f).vibrato (6, 22, .2f);
    add ("Strings & Brass", "Trumpet Stab").N (0, .02f, .15f, .5f, .1f).osc (W::Saw).filter (F::Lp, 1500, 1.5f, 3, .15f).drv (.8f);
    add ("Strings & Brass", "Brass Section").N (-1, .05f, .2f, .75f, .2f).osc (W::Saw, 1, 1, -8).osc (W::Saw, 1, 1, 8).osc (W::Saw, .7f, 1.5f).filter (F::Lp, 1400, 1.2f, 2.5f, .3f).drv (.7f);
    add ("Strings & Brass", "Muted Horn").N (0, .03f, .2f, .6f, .15f).osc (W::Saw).filter (F::Bp, 1100, 2, 1, .2f);
    add ("Strings & Brass", "Soul Horn Stab").N (-1, .015f, .25f, 0, .12f).osc (W::Saw, 1, 1, -6).osc (W::Saw, 1, 1, 6).filter (F::Lp, 1600, 1, 2.5f, .2f).drv (.9f);
    add ("Strings & Brass", "Orchestra Hit").N (-1, .002f, .5f, 0, .25f).osc (W::Saw, 1, 1, -7).osc (W::Saw, 1, 1, 7).osc (W::Saw, .8f, 1.5f).osc (W::Saw, .6f, 2).filter (F::Lp, 2800, .8f, 1.5f, .3f).drv (1.1f);

    // ---- FX (+10) ----
    add ("FX", "Downlifter").D (200, 2).osc (W::Sine).pitchEnv (24, 1.6f).noise (K::White, F::Bp, 4000, 3, 1, 1.6f).sweepNoise (6000, 300, 1.8f);
    add ("FX", "Noise Sweep").D (200, 2.6f).rising().noise (K::White, F::Bp, 500, 4, 1.3f, 50).sweepNoise (400, 9000, 2.4f);
    add ("FX", "Sub Drop").D (60, 1.6f).osc (W::Sine).pitchEnv (36, 1.1f).drv (1.2f);
    add ("FX", "Zap").D (1500, .22f).osc (W::Saw).pitchEnv (-30, .1f).filter (F::Lp, 4000, 2).drv (1.5f);
    add ("FX", "Siren").D (700, 1.6f).osc (W::Square).pitchEnv (12, .8f).vibrato (3, 400, 0).filter (F::Lp, 3500, 1);
    add ("FX", "Tape Stop").D (400, 1.2f).osc (W::Saw).osc (W::Sine, .5f, .5f).pitchEnv (-36, 1).filter (F::Lp, 2000);
    add ("FX", "Glitch Blip").D (1200, .07f).osc (W::Square).pitchEnv (7, .04f).filter (F::Bp, 3000, 2);
    add ("FX", "Cinematic Boom").D (36, 3.2f).osc (W::Sine).pitchEnv (10, .3f).noise (K::Pink, F::Lp, 500, .7f, 1, 2.2f).drv (1.8f);
    add ("FX", "Gun Shot").D (120, .5f).osc (W::Sine).pitchEnv (20, .05f).noise (K::White, F::Hp, 1200, .6f, 1.3f, .35f).drv (2.5f);
    add ("FX", "Vinyl Pops").D (200, 1.5f).noise (K::Crackle, F::Bp, 3000, .8f, 1.3f, 50).drv (.6f);
}
} // namespace fw
