import unittest
from pathlib import Path

from helpers.tool_manifest import get_mcp_tool_names


class ToolManifestTests(unittest.TestCase):
    def test_documented_core_tools_are_exposed(self):
        server_path = Path(__file__).resolve().parents[1] / "unreal_mcp_server_advanced.py"
        tools = set(get_mcp_tool_names(server_path))

        self.assertIn("spawn_actor", tools)
        self.assertIn("connect_nodes", tools)
        self.assertIn("disconnect_nodes", tools)
        self.assertIn("detect_fab_status", tools)
        self.assertIn("import_asset", tools)
        self.assertIn("bulk_import_assets", tools)
        self.assertIn("place_imported_asset", tools)
        self.assertIn("detect_gasp_assets", tools)
        self.assertIn("detect_als_plugin", tools)


if __name__ == "__main__":
    unittest.main()
