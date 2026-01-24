# RFSN Blueprint Integration Guide

## Setting Up Dialogue Input Action

### Step 1: Create Input Action

1. In Content Browser: **Right-click → Input → Input Action**
2. Name it: `IA_Dialogue`
3. Open and set:
   - **Value Type**: `Digital (bool)`
   - **Triggers**: `Down` or `Pressed`

### Step 2: Add to Input Mapping Context

1. Open your `IMC_Default` (or equivalent)
2. Add mapping for `IA_Dialogue`:
   - **Key**: `T` (or preferred key)
   - **Modifiers**: None

### Step 3: Assign to Character

1. Open your Character Blueprint
2. In Details panel, find **Input → DialogueAction**
3. Set to `IA_Dialogue`

---

## Creating an RFSN NPC Blueprint

### Step 1: Create NPC Blueprint

1. Content Browser: **Right-click → Blueprint Class**
2. Parent: `ShooterNPC` (Shooter) or `HorrorNPC` (Horror)
3. Name: `BP_RfsnGuard`

### Step 2: Configure RFSN Component

1. Open Blueprint, select `RfsnClient` component
2. Set in Details:
   - **Npc Name**: `Guard`
   - **Npc Id**: `guard_01`
   - **Mood**: `Neutral`
   - **Relationship**: `Stranger`
   - **Affinity**: `0.0`

### Step 3: Add Dialogue Trigger

1. **Add Component** → `RfsnNpcDialogueTrigger`
2. Configure:
   - **Trigger Mode**: `Proximity` or `Interact`
   - **Proximity Radius**: `200.0`
   - **Default Prompt**: `Hello there!`
   - **Cooldown**: `5.0`

### Step 4: Place in Level

1. Drag `BP_RfsnGuard` into your level
2. Position near player spawn

---

## Testing Dialogue Flow

1. **Start RFSN Server**:

   ```bash
   cd RFSN_NPC_AI/Python
   source .venv/bin/activate
   python -m uvicorn orchestrator:app --port 8000
   ```

2. **Play in Editor** (PIE)

3. **Approach NPC** → Walk within proximity radius

4. **Press T** (or dialogue key) → Dialogue starts

5. **Watch subtitles** at bottom of screen

---

## Blueprint Event Graph Hooks

For custom behavior, bind to these events in your NPC Blueprint:

```
Event BeginPlay
└─→ Get RfsnClient
    └─→ Bind Event to OnNpcActionReceived
        └─→ Custom Event: HandleRfsnAction
            └─→ Switch on ERfsnNpcAction
                ├─→ Attack: Run to player, attack
                ├─→ Flee: Run away
                ├─→ Greet: Play wave animation
                └─→ Help: Offer quest
```

## RFSN Dialogue Manager from Blueprint

```
Get World
└─→ Get Subsystem (RfsnDialogueManager)
    ├─→ Start Dialogue (Actor)
    ├─→ Send Player Message (String)
    └─→ End Dialogue
```
