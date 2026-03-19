# Offline mode

## Intent

The Island runtime must remain playable without the Python service.

## Current branch behavior

When `URfsnNpcClientComponent` cannot create or complete a streaming request:

- it logs degraded mode
- it emits one local fallback bark
- it still broadcasts dialogue-complete
- it avoids blocking interaction flow or gameplay progression

## What offline mode does not block

- scavenging tower parts
- repairing the tower
- transmission pressure
- extraction activation
- run success/failure resolution

## Recommended offline validation

1. leave the Python service stopped
2. launch the Island map
3. talk to an NPC that has `URfsnNpcClientComponent`
4. verify you get a fallback bark instead of a hard error
5. complete the tower/extraction loop to confirm no dependency on the backend
