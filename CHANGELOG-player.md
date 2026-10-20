https://github.com/tetherball88/Lover-s-Ledger/releases/tag/v1.1.0

Requires SKSE, SKSE Menu Framework and OStim. Optional: SexLab AE or SexLab P+, SkyrimNet 0.25.0+.

## v0.1.1 (unreleased)

### SexLab support
- SexLab AE and SexLab P+ scenes are now recorded in the ledger, alongside OStim
- Threesomes and strap-on scenes credit the right partner
- OStim scenes now also count in SexLab's own sex statistics

### Orgasm attribution
- Fixed: orgasm credit in group scenes was uneven, favouring one partner
- A second orgasm in the same scene is now credited
- With SkyrimNet_SexLab's orgasm engine on, orgasms follow that engine, and the partner who aroused the actor most gets the most credit

### Scene counters and action names
- The ledger now keeps overall scene counts by race, gender, act and group size
- Different names for the same act (e.g. "ass" and "anal") are now counted together

### SexLab history backfill
- NPCs with SexLab history from before the ledger was installed keep it: it goes to their spouse or lover, or to an unnamed past lover
- SkyrimNet characters remember those past lovers

### First-time lines
- SexLab scenes can note a character's first times (first sex, first partner of a new race or gender, first threesome, and so on)
- Intersex characters (picked in SkyrimNet_SexLab Actor Info) count as intersex on SexLab AE too, not as the opposite sex; first-time lines say "an intersex partner"

### Bug fixes
- Fixed: loading a save during a scene could leave old scene data behind
- Fixed: a possible freeze when scoring lovers
