"""
Utilities for normalizing responses returned by the Unreal C++ bridge.
"""

from __future__ import annotations

from typing import Any, Dict, Optional


def normalize_unreal_response(response: Optional[Dict[str, Any]]) -> Dict[str, Any]:
    """
    Normalize Unreal bridge responses while preserving backwards-compatible fields.

    The C++ bridge wraps successful command payloads as:
        {"status": "success", "result": {...}}

    Many Python helpers historically consumed direct payloads such as:
        {"success": true, "actors": [...]}

    This function keeps status/result intact, adds a boolean success field, and
    promotes result fields to the top level when they do not conflict.
    """
    if not response:
        return {"success": False, "status": "error", "error": "No response from Unreal"}

    if not isinstance(response, dict):
        return {"success": False, "status": "error", "error": f"Invalid response type: {type(response).__name__}"}

    normalized = dict(response)
    status = normalized.get("status")

    if status == "success":
        normalized["success"] = True
    elif status == "error":
        normalized["success"] = False
    elif "success" in normalized:
        normalized["status"] = "success" if normalized.get("success") else "error"

    result = normalized.get("result")
    if isinstance(result, dict):
        for key, value in result.items():
            normalized.setdefault(key, value)

    if normalized.get("success") is False:
        error = normalized.get("error") or normalized.get("message")
        if error:
            normalized["error"] = error

    return normalized
