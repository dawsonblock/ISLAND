import os
import shutil
from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parent.parent
SERVICE_ROOT = PYTHON_ROOT.parent
RUNTIME_ROOT_ENV = "RFSN_RUNTIME_ROOT"
DEFAULT_RUNTIME_ROOT = PYTHON_ROOT / "var"


def _resolve_runtime_root() -> Path:
    override = os.environ.get(RUNTIME_ROOT_ENV)
    if override:
        return Path(override).expanduser().resolve()
    return DEFAULT_RUNTIME_ROOT.resolve()


VAR_ROOT = _resolve_runtime_root()


def service_path(*parts: str) -> Path:
    return SERVICE_ROOT.joinpath(*parts)


def runtime_path(*parts: str) -> Path:
    return VAR_ROOT.joinpath(*parts)


def runtime_dir(*parts: str) -> Path:
    path = runtime_path(*parts)
    path.mkdir(parents=True, exist_ok=True)
    return path


def runtime_file(*parts: str) -> Path:
    path = runtime_path(*parts)
    path.parent.mkdir(parents=True, exist_ok=True)
    return path


def seeded_runtime_file(source: str | Path, *parts: str, overwrite: bool = False) -> Path:
    source_path = Path(source)
    destination = runtime_file(*parts)

    if destination.exists() and not overwrite:
        return destination

    if source_path.exists():
        shutil.copy2(source_path, destination)

    return destination


__all__ = [
    "DEFAULT_RUNTIME_ROOT",
    "PYTHON_ROOT",
    "RUNTIME_ROOT_ENV",
    "SERVICE_ROOT",
    "VAR_ROOT",
    "runtime_dir",
    "runtime_file",
    "runtime_path",
    "seeded_runtime_file",
    "service_path",
]