#!/usr/bin/env python3
"""
RFSN Orchestrator v8.1 - Optimized Launcher
One-click startup with dependency checks and auto-configuration.
"""

import subprocess
import sys
import os
from pathlib import Path


def check_python_version():
    """Ensure Python 3.12+"""
    if sys.version_info < (3, 12):
        print("❌ Python 3.12+ required")
        print(f"   Current: {sys.version}")
        return False
    print(f"✅ Python {sys.version_info.major}.{sys.version_info.minor}")
    return True


def check_dependencies():
    """Check if all required packages are installed"""
    required = [
        "fastapi",
        "uvicorn",
        "pydantic",
        "requests",
    ]
    
    missing = []
    for package in required:
        try:
            __import__(package)
        except ImportError:
            missing.append(package)
    
    if missing:
        print(f"❌ Missing packages: {', '.join(missing)}")
        print("   Run: pip install -r requirements.txt")
        return False
    
    print(f"✅ All {len(required)} core dependencies installed")
    return True


def check_optional_dependencies():
    """Check optional packages"""
    optional = {
        "llama_cpp": "llama-cpp-python (for LLM)",
        "piper": "piper-tts (for voice)",
    }
    
    installed = []
    missing = []
    
    for name, desc in optional.items():
        try:
            __import__(name)
            installed.append(desc)
        except ImportError:
            missing.append(desc)
    
    if installed:
        print(f"✅ Optional: {', '.join(installed)}")
    if missing:
        print(f"⚠️  Optional (not installed): {', '.join(missing)}")
    
    return True


def check_model():
    """Check if LLM model exists"""
    script_dir = Path(__file__).parent
    python_root = script_dir.parent
    service_root = python_root.parent
    config_path = service_root / "config.json"
    
    if config_path.exists():
        import json
        config = json.loads(config_path.read_text())
        model_path = Path(config.get("model_path", ""))

        candidates = []
        if model_path:
            if model_path.is_absolute():
                candidates.append(model_path)
            else:
                candidates.append((service_root / model_path).resolve())
                candidates.append((python_root / "var" / "models" / model_path.name).resolve())

        for candidate in candidates:
            if candidate.exists():
                size_mb = candidate.stat().st_size / 1024 / 1024
                print(f"✅ LLM model: {candidate.name} ({size_mb:.0f} MB)")
                return True
    
    print("⚠️  LLM model not found (will run in mock mode)")
    return True


def check_piper():
    """Check if Piper TTS is available"""
    import shutil
    
    if shutil.which("piper"):
        print("✅ Piper TTS: executable found in PATH")
        return True
    
    # Check Models directory
    script_dir = Path(__file__).parent
    runtime_piper_dir = script_dir.parent / "var" / "models" / "piper"
    legacy_piper_dir = script_dir.parent.parent / "Models" / "piper"
    
    for name in ["piper", "piper.exe"]:
        if (runtime_piper_dir / name).exists() or (legacy_piper_dir / name).exists():
            print("✅ Piper TTS: found in runtime or legacy model directories")
            return True
    
    print("⚠️  Piper TTS not installed (voice will be disabled)")
    return True


def start_server():
    """Start the canonical FastAPI server"""
    script_dir = Path(__file__).parent
    
    print("\n" + "=" * 60)
    print("STARTING RFSN ORCHESTRATOR v8.1")
    print("=" * 60)
    print("Server: http://127.0.0.1:8000")
    print("API Docs: http://127.0.0.1:8000/docs")
    print("Press Ctrl+C to stop")
    print("=" * 60 + "\n")
    
    # Start server
    os.chdir(script_dir.parent)
    subprocess.run([
        sys.executable,
        "-m",
        "uvicorn",
        "app.api.main:app",
        "--host",
        "127.0.0.1",
        "--port",
        "8000",
    ])


def main():
    print("\n" + "=" * 60)
    print("RFSN ORCHESTRATOR v8.1 - PRE-FLIGHT CHECK")
    print("=" * 60 + "\n")
    
    checks = [
        ("Python Version", check_python_version),
        ("Dependencies", check_dependencies),
        ("Optional Deps", check_optional_dependencies),
        ("LLM Model", check_model),
        ("Piper TTS", check_piper),
    ]
    
    all_passed = True
    for name, check_fn in checks:
        if not check_fn():
            all_passed = False
    
    print()
    
    if all_passed:
        start_server()
    else:
        print("❌ Some checks failed. Fix issues and try again.")
        sys.exit(1)


if __name__ == "__main__":
    main()
