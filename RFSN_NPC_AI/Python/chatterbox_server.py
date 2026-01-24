#!/usr/bin/env python3
"""
RFSN Chatterbox TTS Server
Dual-model server: Full Chatterbox + Chatterbox-Turbo
Emotion-aware synthesis with voice cloning support
"""

import asyncio
import io
import json
import logging
import os
import tempfile
import time
from pathlib import Path
from typing import Optional

import torch
import torchaudio
import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel, Field

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("chatterbox_server")

# ─────────────────────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────────────────────

DEVICE = "cuda" if torch.cuda.is_available() else "mps" if torch.backends.mps.is_available() else "cpu"
MODEL_CACHE_DIR = Path.home() / ".cache" / "chatterbox"
OUTPUT_DIR = Path(tempfile.gettempdir()) / "rfsn_tts"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# ─────────────────────────────────────────────────────────────
# Models (lazy loaded)
# ─────────────────────────────────────────────────────────────

chatterbox_full = None
chatterbox_turbo = None

def load_models():
    """Load both Chatterbox models"""
    global chatterbox_full, chatterbox_turbo
    
    try:
        from chatterbox.tts import ChatterboxTTS
        
        logger.info(f"Loading Chatterbox models on {DEVICE}...")
        
        # Load Full model
        logger.info("Loading Chatterbox Full...")
        chatterbox_full = ChatterboxTTS.from_pretrained(device=DEVICE)
        logger.info("Chatterbox Full loaded")
        
        # Load Turbo model  
        logger.info("Loading Chatterbox Turbo...")
        chatterbox_turbo = ChatterboxTTS.from_pretrained(
            device=DEVICE,
            model_id="resemble-ai/chatterbox-turbo"
        )
        logger.info("Chatterbox Turbo loaded")
        
        return True
    except ImportError:
        logger.warning("Chatterbox not installed. Install with: pip install chatterbox-tts")
        return False
    except Exception as e:
        logger.error(f"Failed to load models: {e}")
        return False

# ─────────────────────────────────────────────────────────────
# Request Models
# ─────────────────────────────────────────────────────────────

class TTSRequest(BaseModel):
    text: str = Field(..., description="Text to synthesize")
    emotion: str = Field(default="neutral", description="Emotion tag")
    intensity: float = Field(default=0.5, ge=0.0, le=1.0, description="Emotion intensity")
    pace: float = Field(default=1.0, ge=0.5, le=2.0, description="Speaking pace")
    pitch: float = Field(default=1.0, ge=0.5, le=2.0, description="Pitch modifier")
    voice_reference: Optional[str] = Field(default=None, description="Voice reference audio path")
    exaggeration: float = Field(default=0.5, ge=0.0, le=1.0, description="Emotional exaggeration")
    cfg_weight: float = Field(default=0.5, ge=0.0, le=1.0, description="CFG guidance weight")

class TTSResponse(BaseModel):
    audio_path: str
    duration_sec: float
    model_used: str
    generation_time_ms: float

# ─────────────────────────────────────────────────────────────
# Emotion to Exaggeration Mapping
# ─────────────────────────────────────────────────────────────

EMOTION_SETTINGS = {
    "neutral": {"exaggeration": 0.3, "cfg": 0.5},
    "joy": {"exaggeration": 0.6, "cfg": 0.6},
    "happiness": {"exaggeration": 0.6, "cfg": 0.6},
    "sadness": {"exaggeration": 0.7, "cfg": 0.7},
    "anger": {"exaggeration": 0.8, "cfg": 0.7},
    "fear": {"exaggeration": 0.7, "cfg": 0.6},
    "surprise": {"exaggeration": 0.6, "cfg": 0.5},
    "disgust": {"exaggeration": 0.6, "cfg": 0.6},
    "trust": {"exaggeration": 0.4, "cfg": 0.5},
    "anticipation": {"exaggeration": 0.5, "cfg": 0.5},
}

def get_emotion_params(emotion: str, intensity: float) -> tuple[float, float]:
    """Get exaggeration and CFG based on emotion and intensity"""
    settings = EMOTION_SETTINGS.get(emotion.lower(), EMOTION_SETTINGS["neutral"])
    base_exaggeration = settings["exaggeration"]
    base_cfg = settings["cfg"]
    
    # Scale by intensity
    exaggeration = base_exaggeration * intensity
    cfg = base_cfg + (intensity - 0.5) * 0.2  # Slight CFG boost for high intensity
    
    return min(exaggeration, 1.0), min(max(cfg, 0.3), 0.9)

# ─────────────────────────────────────────────────────────────
# FastAPI App
# ─────────────────────────────────────────────────────────────

app = FastAPI(
    title="RFSN Chatterbox TTS Server",
    description="Dual-model TTS with Chatterbox Full and Turbo",
    version="1.0.0"
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.on_event("startup")
async def startup():
    """Load models on startup"""
    load_models()

@app.get("/health")
async def health():
    """Health check endpoint"""
    return {
        "status": "ok",
        "device": DEVICE,
        "full_loaded": chatterbox_full is not None,
        "turbo_loaded": chatterbox_turbo is not None,
    }

@app.post("/synthesize/full", response_model=TTSResponse)
async def synthesize_full(request: TTSRequest):
    """Synthesize with full Chatterbox model (high quality, slower)"""
    if chatterbox_full is None:
        raise HTTPException(status_code=503, detail="Chatterbox Full not loaded")
    
    return await _synthesize(request, chatterbox_full, "full")

@app.post("/synthesize/turbo", response_model=TTSResponse)
async def synthesize_turbo(request: TTSRequest):
    """Synthesize with Chatterbox Turbo (fast, good quality)"""
    if chatterbox_turbo is None:
        raise HTTPException(status_code=503, detail="Chatterbox Turbo not loaded")
    
    return await _synthesize(request, chatterbox_turbo, "turbo")

@app.post("/synthesize", response_model=TTSResponse)
async def synthesize_auto(request: TTSRequest):
    """Auto-route: High intensity → Full, else → Turbo"""
    model = chatterbox_full if request.intensity > 0.7 else chatterbox_turbo
    model_name = "full" if request.intensity > 0.7 else "turbo"
    
    if model is None:
        model = chatterbox_turbo or chatterbox_full
        model_name = "turbo" if chatterbox_turbo else "full"
    
    if model is None:
        raise HTTPException(status_code=503, detail="No TTS model loaded")
    
    return await _synthesize(request, model, model_name)

async def _synthesize(request: TTSRequest, model, model_name: str) -> TTSResponse:
    """Core synthesis function"""
    start_time = time.time()
    
    # Get emotion-based params
    exaggeration, cfg = get_emotion_params(request.emotion, request.intensity)
    
    # Override with explicit values if provided
    if request.exaggeration != 0.5:
        exaggeration = request.exaggeration
    if request.cfg_weight != 0.5:
        cfg = request.cfg_weight
    
    logger.info(f"[{model_name.upper()}] Synthesizing: '{request.text[:50]}...' "
                f"emotion={request.emotion}, exag={exaggeration:.2f}, cfg={cfg:.2f}")
    
    try:
        # Load voice reference if provided
        audio_prompt = None
        if request.voice_reference and Path(request.voice_reference).exists():
            audio_prompt = request.voice_reference
        
        # Generate audio
        wav = model.generate(
            text=request.text,
            audio_prompt_path=audio_prompt,
            exaggeration=exaggeration,
            cfg_weight=cfg,
        )
        
        # Save to file
        timestamp = int(time.time() * 1000)
        output_path = OUTPUT_DIR / f"tts_{model_name}_{timestamp}.wav"
        torchaudio.save(str(output_path), wav, model.sr)
        
        # Calculate duration
        duration_sec = wav.shape[1] / model.sr
        generation_time = (time.time() - start_time) * 1000
        
        logger.info(f"[{model_name.upper()}] Generated {duration_sec:.2f}s audio in {generation_time:.0f}ms")
        
        return TTSResponse(
            audio_path=str(output_path),
            duration_sec=duration_sec,
            model_used=model_name,
            generation_time_ms=generation_time,
        )
        
    except Exception as e:
        logger.error(f"Synthesis failed: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/audio/{filename}")
async def get_audio(filename: str):
    """Serve generated audio file"""
    audio_path = OUTPUT_DIR / filename
    if not audio_path.exists():
        raise HTTPException(status_code=404, detail="Audio not found")
    return FileResponse(str(audio_path), media_type="audio/wav")

@app.delete("/cleanup")
async def cleanup_audio():
    """Remove old audio files"""
    count = 0
    cutoff = time.time() - 3600  # 1 hour old
    for f in OUTPUT_DIR.glob("*.wav"):
        if f.stat().st_mtime < cutoff:
            f.unlink()
            count += 1
    return {"deleted": count}

# ─────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="RFSN Chatterbox TTS Server")
    parser.add_argument("--host", default="0.0.0.0", help="Host to bind")
    parser.add_argument("--port", type=int, default=8001, help="Port to bind")
    parser.add_argument("--turbo-only", action="store_true", help="Load only Turbo model")
    parser.add_argument("--full-only", action="store_true", help="Load only Full model")
    args = parser.parse_args()
    
    print(f"""
╔═══════════════════════════════════════════════════════════════╗
║           RFSN Chatterbox TTS Server v1.0                    ║
║                                                               ║
║  Endpoints:                                                   ║
║    POST /synthesize/full   - Full Chatterbox (high quality)  ║
║    POST /synthesize/turbo  - Turbo (fast)                    ║
║    POST /synthesize        - Auto-route by intensity         ║
║    GET  /health            - Server status                   ║
║                                                               ║
║  Device: {DEVICE:<10}                                         ║
╚═══════════════════════════════════════════════════════════════╝
    """)
    
    uvicorn.run(app, host=args.host, port=args.port)
