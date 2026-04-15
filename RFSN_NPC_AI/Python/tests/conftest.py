import os
import shutil
import sys
import tempfile
from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parent.parent
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

TEST_RUNTIME_ROOT = Path(tempfile.mkdtemp(prefix="rfsn-runtime-tests-"))
os.environ.setdefault("RFSN_RUNTIME_ROOT", str(TEST_RUNTIME_ROOT))
os.environ.setdefault("SKIP_MODELS", "1")
os.environ.setdefault("TOKENIZERS_PARALLELISM", "false")


def pytest_sessionfinish(session, exitstatus):
    shutil.rmtree(TEST_RUNTIME_ROOT, ignore_errors=True)
