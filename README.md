# TT Lover's Ledger

A Skyrim mod that tracks and analyzes adult framework interactions to provide detailed relationship statistics and history for NPCs and the player.

## Overview

TT Lover's Ledger is a utility mod that tracks adult framework interactions (OStim/Sexlab scenes) in your game, gathering comprehensive statistics that can be used by other mods. It tracks all interactions, whether the player is involved or not, maintaining detailed records of who did what with whom.

## Features

### Comprehensive Scene Tracking
- Records all adult framework scenes and participants
- Tracks specific actions performed and received by each actor
- Maintains detailed statistics regardless of player involvement

### Actor Statistics
For each actor (NPC or player), the mod tracks:
- Participation in OStim/Sexlab scenes
- Action counts (performed and received)
- Top 3 most frequent actions (both performed and received)
- Encounter types:
  - Solo interactions
  - Exclusive (one partner) encounters
  - Group encounters
  - Same-sex encounters
- Timestamp of last encounter

### Lover Relationships
For each actor's lovers, the mod maintains:
- Complete interaction history
- Exclusive and group encounter counts
- Last encounter timestamp
- Orgasm participation statistics
- Internal climax tracking (given and received)

### Top 3 Lovers System
Tracks each actor's top three lovers using a sophisticated scoring system that considers:
- Recency of encounters (time decay factor)
- Number of exclusive encounters
- Group encounter participation
- Orgasm contribution

#### Lover Score Formula
```
Final Score = Time Multiplier × (√(exclusive_encounters) × 6.0 + √(group_encounters) × 2.0 + √(orgasms) × 5.0)
```

Time Multiplier Values:
- 1.0: Less than a week
- 0.8: More than 1 week
- 0.6: More than 1 month
- 0.3: More than 6 months
- 0.1: Within a year
- 0.05: Over a year

### Existing Relationship Integration
The mod intelligently handles pre-existing relationships:
- Detects current spouses, courting partners, and lovers (relationship rank 4)
- Generates historically appropriate relationship data

#### Historical Data Generation
- **Spouse**: 40-100 encounters
- **Courting**: 0-10 encounters
- **Lover**: 10-40 encounters

Last encounter timing:
- Active lovers: 1-7 days ago
- Inactive relationships: 180-360 days ago

## MCM Integration

The Mod Configuration Menu (MCM) provides a comprehensive interface to:
- View tracked NPCs
- Explore detailed statistics
- Review lover relationships
- Analyze interaction history

## Framework Support

### OStim
- Full native support
- Complete statistic tracking
- Detailed action and interaction logging

### Sexlab
- Basic framework support available
- Integration possible through `TTLL_SexlabIntegration.psc`
- Can be extended via custom implementation

## Developer Information

### Integration
The mod is designed to be highly extensible and developer-friendly:
- Public functions and scripts
- Comprehensive API documentation
- Easy access to all statistics and data

### Key Components
- `TTLL_Store`: Core data management
- `TTLL_OstimIntegration`: OStim framework integration
- `TTLL_SexlabIntegration`: Sexlab framework integration template

For detailed API documentation, see:
- [TTLL_Store Functions Reference](docs/TTLL_Store_Functions.md)

### Custom Integration
To integrate Sexlab functionality:
1. Create a new mod as an overlay
2. Override `TTLL_SexlabIntegration.psc`
3. Implement the provided empty functions
4. The integration will activate automatically when Sexlab is detected

## Requirements
- Skyrim Special Edition
- SKSE
- JContainers
- OStim (for OStim features)
- Sexlab (optional, for Sexlab features)

## Installation
1. Install required dependencies
2. Install TT Lover's Ledger using your preferred mod manager
3. Load after OStim/Sexlab in your load order
