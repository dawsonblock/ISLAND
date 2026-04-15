#!/bin/bash
echo "🚀 Starting RFSN Orchestrator & Web Chat..."

# Function to run a command in a new terminal tab (macOS)
run_in_new_tab() {
    local cmd="$1"
    local title="$2"
    osascript -e "tell application \"Terminal\" to do script \"cd '$(pwd)' && $cmd\""
}

# 1. Start Main Orchestrator
echo "Starting Main Orchestrator (Port 8000)..."
run_in_new_tab "cd Python && python -m uvicorn app.api.main:app --host 0.0.0.0 --port 8000" "RFSN-Orchestrator"

# 2. Start Web Chat Backend
echo "Starting Web Chat Backend (Port 3001)..."
run_in_new_tab "cd web_chat_ui/backend && uvicorn main:app --reload --port 3001" "RFSN-Chat-Backend"

# 3. Start Frontend
echo "Starting Web Chat Frontend (Port 5173)..."
run_in_new_tab "cd web_chat_ui/frontend && npm run dev" "RFSN-Chat-Frontend"

echo "✅ All services started in separate tabs!"
echo "👉 Open http://localhost:5173 to chat."
