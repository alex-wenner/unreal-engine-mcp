import unittest

from helpers.response_utils import normalize_unreal_response


class NormalizeUnrealResponseTests(unittest.TestCase):
    def test_promotes_wrapped_result_fields(self):
        response = normalize_unreal_response({
            "status": "success",
            "result": {"actors": [{"name": "Cube"}]},
        })

        self.assertTrue(response["success"])
        self.assertEqual(response["actors"], [{"name": "Cube"}])
        self.assertIn("result", response)

    def test_normalizes_error_status(self):
        response = normalize_unreal_response({"status": "error", "error": "bad"})

        self.assertFalse(response["success"])
        self.assertEqual(response["error"], "bad")

    def test_handles_missing_response(self):
        response = normalize_unreal_response(None)

        self.assertFalse(response["success"])
        self.assertEqual(response["status"], "error")


if __name__ == "__main__":
    unittest.main()
