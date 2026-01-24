#!/usr/bin/env python3
"""
RFSN Mock Server
Lightweight mock server for offline testing and development.
Provides canned responses without requiring the full RFSN orchestrator.

Usage:
    python mock_server.py [--port 8000]
"""

import argparse
import asyncio
import json
import random
from datetime import datetime
from typing import AsyncGenerator

try:
    from fastapi import FastAPI, Request
    from fastapi.responses import StreamingResponse
    from fastapi.middleware.cors import CORSMiddleware
    import uvicorn
except ImportError:
    print("Please install dependencies: pip install fastapi uvicorn")
    exit(1)

app = FastAPI(title="RFSN Mock Server", version="1.0.0")

# CORS for Unreal HTTP requests
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ─────────────────────────────────────────────────────────────
# Canned Responses
# ─────────────────────────────────────────────────────────────

GREETINGS = [
    "Hello there, traveler.",
    "Well met, stranger.",
    "What brings you here?",
    "I haven't seen you before.",
]

RESPONSES = {
    "hello": ["Hello! How can I help you?", "Greetings, friend."],
    "help": ["I'll do what I can.", "Tell me what you need."],
    "trade": ["Let's see what you've got.", "I have some wares you might like."],
    "goodbye": ["Safe travels.", "Until next time.", "Farewell."],
    "threat": ["You're making a mistake.", "I wouldn't do that if I were you."],
}

ACTIONS = ["Greet", "Answer", "Explain", "Help", "Trade", "Warn"]


def get_response(player_message: str) -> tuple[str, str]:
    """Get canned response based on player message keywords."""
    message_lower = player_message.lower()
    
    for keyword, responses in RESPONSES.items():
        if keyword in message_lower:
            return random.choice(responses), random.choice(ACTIONS)
    
    # Default response
    return random.choice(GREETINGS), "Greet"


async def generate_sse_stream(player_message: str, npc_name: str) -> AsyncGenerator[str, None]:
    """Generate mock SSE stream."""
    
    response_text, action = get_response(player_message)
    
    # Simulate processing delay
    await asyncio.sleep(0.3)
    
    # Meta event
    meta_event = {
        "npc_action": action,
        "player_signal": "neutral",
        "bandit_key": f"mock_{datetime.now().timestamp()}"
    }
    yield f"data: {json.dumps({'type': 'meta', 'data': meta_event})}\n\n"
    
    # Simulate typing delay
    await asyncio.sleep(0.5)
    
    # Sentence event
    sentence_event = {
        "npc_name": npc_name,
        "sentence": response_text
    }
    yield f"data: {json.dumps({'type': 'sentence', 'data': sentence_event})}\n\n"
    
    # Done event
    await asyncio.sleep(0.2)
    yield f"data: {json.dumps({'type': 'done'})}\n\n"


# ─────────────────────────────────────────────────────────────
# Endpoints
# ─────────────────────────────────────────────────────────────

@app.get("/api/health")
async def health():
    """Health check endpoint."""
    return {"status": "healthy", "mode": "mock", "timestamp": datetime.now().isoformat()}


@app.post("/api/dialogue/stream")
async def dialogue_stream(request: Request):
    """Mock dialogue streaming endpoint."""
    try:
        body = await request.json()
    except:
        body = {}
    
    player_message = body.get("player_utterance", "Hello")
    npc_name = body.get("npc_name", "MockNPC")
    
    return StreamingResponse(
        generate_sse_stream(player_message, npc_name),
        media_type="text/event-stream"
    )


@app.post("/api/director/control")
async def director_control(request: Request):
    """Mock director control endpoint."""
    try:
        body = await request.json()
    except:
        body = {}
    
    alert_level = body.get("alert_level", 0)
    intensity = body.get("intensity", 0.5)
    
    # Simple mock logic
    if alert_level > 80:
        command = "respite"
        modifier = -15
    elif alert_level < 30:
        command = "escalate"
        modifier = 10
    else:
        command = "maintain"
        modifier = 0
    
    return {
        "command": command,
        "alert_modifier": modifier,
        "timestamp": datetime.now().isoformat()
    }


# ─────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="RFSN Mock Server")
    parser.add_argument("--port", type=int, default=8000, help="Port to run on")
    parser.add_argument("--host", default="0.0.0.0", help="Host to bind to")
    args = parser.parse_args()
    
    print(f"╔══════════════════════════════════════╗")
    print(f"║     RFSN Mock Server v1.0.0          ║")
    print(f"║   http://{args.host}:{args.port}              ║")
    print(f"╚══════════════════════════════════════╝")
    print()
    print("Endpoints:")
    print(f"  GET  /api/health")
    print(f"  POST /api/dialogue/stream")
    print(f"  POST /api/director/control")
    print()
    
    uvicorn.run(app, host=args.host, port=args.port, log_level="info")
