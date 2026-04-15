import os
from pathlib import Path

from app.dialogue.multi_npc import VoiceMapper
from app.memory.memory_manager import ConversationManager
from app.runtime_paths import VAR_ROOT, seeded_runtime_file
from app.safety.security import APIKeyManager


def test_runtime_root_override_is_active():
    assert VAR_ROOT.exists()
    assert VAR_ROOT == Path(os.environ["RFSN_RUNTIME_ROOT"]).resolve()


def test_default_runtime_writers_use_runtime_root():
    manager = ConversationManager("Test NPC")
    mapper = VoiceMapper()
    keys = APIKeyManager()

    assert manager.memory_file.is_relative_to(VAR_ROOT)
    assert mapper.config_file.is_relative_to(VAR_ROOT)
    assert Path(keys.keys_file).is_relative_to(VAR_ROOT)


def test_seeded_runtime_file_copies_once(tmp_path):
    source = tmp_path / "config.json"
    source.write_text('{"source": true}')

    destination = seeded_runtime_file(source, "config", "seeded.json")
    assert destination.read_text() == '{"source": true}'

    source.write_text('{"source": false}')
    seeded_again = seeded_runtime_file(source, "config", "seeded.json")

    assert seeded_again == destination
    assert seeded_again.read_text() == '{"source": true}'