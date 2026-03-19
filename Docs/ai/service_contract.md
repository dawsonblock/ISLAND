# Dialogue service contract

## Endpoint

`POST /api/dialogue/stream`

Default local URL:

`http://127.0.0.1:8000/api/dialogue/stream`

## Request shape

The Unreal client currently sends JSON with:

- `user_input`
- `npc_state`
  - `npc_name`
  - `npc_id`
  - `affinity`
  - `mood`
  - `relationship`
  - `dialogue_tone`
  - `backstory_context`
- `tts_engine`

## Response shape

The client expects SSE lines prefixed with `data:`.

### Meta event

Exactly one meta event per request should contain fields such as:

- `player_signal`
- `bandit_key`
- `npc_action`
- `action_mode`
- `instant_bark`
- `bark_duration_ms`

### Sentence events

Sentence events should contain:

- `sentence`
- `is_final`
- `latency_ms`

## Client-side resilience

`URfsnNpcClientComponent` now:

- parses the response incrementally by newly received bytes
- deduplicates already-processed SSE lines per request
- ignores stale callbacks from cancelled requests
- falls back locally if no usable events arrive

## Compatibility note

The Python backend is being reshaped under `RFSN_NPC_AI/Python/app/`, while root-level module shims remain in place for existing tests and entrypoints.
