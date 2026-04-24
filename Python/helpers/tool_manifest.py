"""
Build a canonical manifest of MCP tools from the Python server source.
"""

from __future__ import annotations

import ast
from pathlib import Path
from typing import List


def get_mcp_tool_names(server_path: str | Path) -> List[str]:
    """Return function names decorated with @mcp.tool() in source order."""
    source_path = Path(server_path)
    tree = ast.parse(source_path.read_text(encoding="utf-8"), filename=str(source_path))
    tools: List[str] = []

    for node in tree.body:
        if not isinstance(node, ast.FunctionDef):
            continue
        for decorator in node.decorator_list:
            if (
                isinstance(decorator, ast.Call)
                and isinstance(decorator.func, ast.Attribute)
                and decorator.func.attr == "tool"
                and isinstance(decorator.func.value, ast.Name)
                and decorator.func.value.id == "mcp"
            ):
                tools.append(node.name)
                break

    return tools
