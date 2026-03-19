from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parent
VAR_ROOT = PYTHON_ROOT / "var"


def runtime_dir(*parts: str) -> Path:
    path = VAR_ROOT.joinpath(*parts)
    path.mkdir(parents=True, exist_ok=True)
    return path


def runtime_file(*parts: str) -> Path:
    path = VAR_ROOT.joinpath(*parts)
    path.parent.mkdir(parents=True, exist_ok=True)
    return path
