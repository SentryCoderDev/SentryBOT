from __future__ import annotations
import logging
from collections import deque
from pathlib import Path
from typing import Dict, Iterable, Optional

try:
    import numpy as np  # type: ignore
except Exception:
    np = None  # type: ignore

try:
    from openwakeword.model import Model as OpenWakeWordModel  # type: ignore
except Exception:
    OpenWakeWordModel = None  # type: ignore

logger = logging.getLogger("wakeword.openwakeword")


def _as_float(value) -> Optional[float]:
    try:
        return float(value)
    except Exception:
        return None


def _score_value(value) -> Optional[float]:
    if value is None:
        return None
    if isinstance(value, (list, tuple)):
        if not value:
            return None
        return _as_float(value[-1])
    try:
        import numpy as _np  # type: ignore

        if isinstance(value, _np.ndarray):
            if value.size == 0:
                return None
            return _as_float(value.reshape(-1)[-1])
    except Exception:
        pass
    return _as_float(value)


def _resolve_model_paths(model_paths) -> Dict[str, str]:
    resolved: Dict[str, str] = {}
    if isinstance(model_paths, dict):
        for label, path in model_paths.items():
            if isinstance(path, str) and path:
                resolved[str(label)] = str(path)
        return resolved
    if isinstance(model_paths, list):
        for path in model_paths:
            if isinstance(path, str) and path:
                label = Path(path).stem
                resolved[label] = path
        return resolved
    if isinstance(model_paths, str) and model_paths:
        resolved[Path(model_paths).stem] = model_paths
    return resolved


class OpenWakewordRunner:
    def __init__(self, cfg: dict):
        if OpenWakeWordModel is None or np is None:
            raise RuntimeError("openwakeword and numpy are required for openwakeword engine")
        model_paths = _resolve_model_paths(cfg.get("model_paths"))
        if not model_paths:
            raise ValueError("openwakeword.model_paths is required")
        self._labels = list(model_paths.keys())
        self._model = OpenWakeWordModel(wakeword_models=list(model_paths.values()))
        self._threshold = float(cfg.get("threshold", 0.5))
        self._smooth_window = int(cfg.get("smooth_window", 3))
        self._score_history: Dict[str, deque] = {}
        # optional verifier
        self._verifier = None
        verifier_path = cfg.get("verifier_path")
        if verifier_path:
            try:
                import pickle
                p = Path(verifier_path)
                if not p.exists():
                    # try relative to module
                    p = Path(__file__).resolve().parents[1] / verifier_path
                if p.exists():
                    with open(p, "rb") as f:
                        self._verifier = pickle.load(f)
            except Exception:
                logger.debug("failed to load verifier: %s", verifier_path)

    def run(self, stream: Iterable[bytes]) -> Iterable[str]:
        for chunk in stream:
            label = self._infer_chunk(chunk)
            if label:
                yield label

    def _infer_chunk(self, chunk: bytes) -> Optional[str]:
        if not chunk:
            return None
        try:
            audio = np.frombuffer(chunk, dtype=np.int16)
            scores = self._model.predict(audio)
        except Exception as exc:
            logger.debug("openwakeword inference failed: %s", exc)
            return None
        if not isinstance(scores, dict):
            return None
        best_label = None
        best_score = 0.0
        for name, value in scores.items():
            score = _score_value(value)
            if score is None:
                continue
            history = self._score_history.setdefault(name, deque(maxlen=max(1, self._smooth_window)))
            history.append(score)
            smoothed = sum(history) / len(history)
            if smoothed > best_score:
                best_score = smoothed
                best_label = name
        if best_label and best_score >= self._threshold:
            # optional verifier step
            if self._verifier is not None:
                try:
                    # extract features for verifier using model internals
                    feats = self._model.preprocessor.get_features(self._model.model_inputs.get(best_label))
                    # The verifier expects flattened features per its training pipeline
                    ok = bool(self._verifier.predict([feats.flatten()])[0])
                    if not ok:
                        return None
                except Exception:
                    # on error, fall back to unlverified accept
                    pass
            return best_label
        return None
